/*
 * Rho - A sandbox for mathematics.
 * Copyright (C) 2015-2016 Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "parse/parser.hpp"
#include "util/ast_tools.hpp"
#include <unordered_map>
#include <sstream>

#include <iostream> // DEBUG


namespace rho {
  
  /* 
   * Parses the specified stream of tokens.
   * Throws `parse_error' in case of a syntax error.
   */
  std::shared_ptr<ast_program>
  parser::parse (lexer::token_stream strm, const std::string& path)
  {
    this->path = path;
    return this->parse_program (strm);
  }
  
  
  
  void
  parser::expect (token_type type, lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != type)
      throw parse_error ("expected '" + token_type_to_str (type) + "'",
        tok.ln, tok.col);
    strm.next ();
  }
  
  void
  parser::consume_scol (lexer::token_stream& strm, bool in_block)
  {
    if (!in_block)
      this->expect (TOK_SCOL, strm);
    else
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_SCOL)
          strm.next ();
        else if (tok.type != TOK_RBRACE)
          throw parse_error ("expected ';'", tok.ln, tok.col);
      }
  }
  
  static void
  _set_ast_location (ast_node *node, token tok, const std::string& path)
  {
    node->set_location (path, tok.ln, tok.col);
  }
  
  
  
  std::shared_ptr<ast_program>
  parser::parse_program (lexer::token_stream& strm)
  {
    std::shared_ptr<ast_program> program { new ast_program () };
    _set_ast_location (program.get (), strm.peek_next (), this->path);
    
    while (strm.has_next ())
      {
        auto stmt = this->parse_stmt (strm);
        program->push_back (stmt);
      }
    
    return program;
  }
  
  
  
  std::shared_ptr<ast_integer>
  parser::parse_integer (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != TOK_INTEGER)
      throw parse_error ("expected integer", tok.ln, tok.col);
    
    strm.next ();
    auto ast = std::shared_ptr<ast_integer> (new ast_integer (tok.val.str));
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  std::shared_ptr<ast_float>
  parser::parse_float (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != TOK_FLOAT)
      throw parse_error ("expected float", tok.ln, tok.col);
    
    strm.next ();
    auto ast = std::shared_ptr<ast_float> (new ast_float (tok.val.str));
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  std::shared_ptr<ast_ident>
  parser::parse_ident (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != TOK_IDENT)
      throw parse_error ("expected identifier", tok.ln, tok.col);
    
    strm.next ();
    auto ast = std::shared_ptr<ast_ident> (new ast_ident (tok.val.str));
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  std::shared_ptr<ast_atom>
  parser::parse_atom (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != TOK_ATOM)
      throw parse_error ("expected atom", tok.ln, tok.col);
    
    strm.next ();
    auto ast = std::shared_ptr<ast_atom> (new ast_atom (tok.val.str));
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  std::shared_ptr<ast_string>
  parser::parse_string (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    if (tok.type != TOK_STRING)
      throw parse_error ("expected string", tok.ln, tok.col);
    
    strm.next ();
    auto ast = std::shared_ptr<ast_string> (new ast_string (tok.val.str));
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  std::shared_ptr<ast_vector>
  parser::parse_vector (lexer::token_stream& strm)
  {
    std::shared_ptr<ast_vector> vec { new ast_vector () };
    _set_ast_location (vec.get (), strm.peek_next (), this->path);
    
    // [
    this->expect (TOK_LBRACKET, strm);
    
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_RBRACKET)
          { strm.next (); break; }
        
        vec->push_back (this->parse_expr (strm));
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_RBRACKET)
          throw parse_error ("expected ',' or ']' in vector literal",
            tok.ln, tok.col);
      }
    
    return vec;
  }
  
  
  
  std::shared_ptr<ast_fun>
  parser::parse_fun (lexer::token_stream& strm)
  {
    // fun
    auto tok = strm.peek_next ();
    auto ftok = tok;
    if (tok.type != TOK_FUN)
      throw parse_error (
        "expected 'fun' at beginning of function literal",
        tok.ln, tok.col);
    strm.next ();
    
    // (
    tok = strm.peek_next ();
    if (tok.type != TOK_LPAREN)
      throw parse_error (
        "expected '(' at beginning of function parameter list",
        tok.ln, tok.col);
    strm.next ();
    
    // params
    std::vector<std::string> params;
    while ((tok = strm.peek_next ()).type != TOK_EOF && tok.type != TOK_RPAREN)
      {
        if (tok.type == TOK_MUL)
          {
            strm.next ();
            params.push_back ("*" + this->parse_ident (strm)->get_value ());
          }
        else
          params.push_back (this->parse_ident (strm)->get_value ());
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_RPAREN)
          throw parse_error ("expected ',' or ')' in function parameter list",
            tok.ln, tok.col);
      }
    if (tok.type == TOK_RPAREN)
      strm.next ();
    else if (tok.type == TOK_EOF)
      throw parse_error ("unexpected EOF in function parameter list", tok.ln, tok.col);
    
    // body
    auto body = this->parse_stmt_block (strm);
    
    std::shared_ptr<ast_fun> fun { new ast_fun () };
    _set_ast_location (fun.get (), ftok, this->path);
    fun->set_body (body);
    for (auto p : params)
      fun->add_param (p);
    return fun;
  }
  
  std::shared_ptr<ast_fun_call>
  parser::parse_fun_call (std::shared_ptr<ast_expr> expr,
                          lexer::token_stream& strm)
  {
    std::shared_ptr<ast_fun_call> fcall { new ast_fun_call (expr) };
    _set_ast_location (fcall.get (), strm.peek_next (), this->path);
    
    // (
    this->expect (TOK_LPAREN, strm);
    
    // args
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_RPAREN)
          { strm.next (); break; }
        
        fcall->add_arg (this->parse_expr (strm));
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_RPAREN)
          throw parse_error ("expected ',' or ')' in function call argument list",
            tok.ln, tok.col);
      }
    
    return fcall;
  }
  
  
  
  std::shared_ptr<ast_if>
  parser::parse_if (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    
    // if
    this->expect (TOK_IF, strm);
    
    // <test>
    auto test = this->parse_expr (strm);
    
    // then
    this->expect (TOK_THEN, strm);
    
    // <consequent>
    auto conseq = this->parse_expr (strm);
    
    // else
    this->expect (TOK_ELSE, strm);
    
    // <antecedent>
    auto ant = this->parse_expr (strm);
    
    auto ast = std::shared_ptr<ast_if> (new ast_if (test, conseq, ant));
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_expr>
  parser::parse_list (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    
    // '(
    this->expect (TOK_LIST_START, strm);
    
    auto tok = strm.peek_next ();
    if (tok.type == TOK_RPAREN)
      {
        strm.next ();
        return std::shared_ptr<ast_list> (new ast_list ());
      }
    
    auto fst = this->parse_expr (strm);
    
    tok = strm.peek_next ();
    if (tok.type == TOK_DOT)
      {
        // cons
        
        strm.next ();
        auto snd = this->parse_expr (strm);
        this->expect (TOK_RPAREN, strm);
        
        auto ast = std::shared_ptr<ast_expr> (new ast_cons (fst, snd));
        _set_ast_location (ast.get (), ftok, this->path);
        return ast;
      }
    
    std::shared_ptr<ast_list> lst { new ast_list () };
    lst->add_elem (fst);
    for (;;)
      {
        tok = strm.peek_next ();
        if (tok.type == TOK_RPAREN)
          { strm.next (); break; }
        
        lst->add_elem (this->parse_expr (strm));
      }
    
    auto ast = std::static_pointer_cast<ast_expr> (lst);
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_match>
  parser::parse_match (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    
    // match
    this->expect (TOK_MATCH, strm);
    
    // <expr>
    auto e = this->parse_expr (strm);
    std::shared_ptr<ast_match> ast { new ast_match (e) };
    _set_ast_location (ast.get (), ftok, this->path);
    
    // {
    this->expect (TOK_LBRACE, strm);
    
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_EOF)
          throw parse_error ("unexpected EOF in match expression", tok.ln, tok.col);
        else if (tok.type == TOK_RBRACE)
          { strm.next (); break; }
        else if (tok.type == TOK_CASE)
          {
            strm.next ();
            
            // <pattern>
            auto pat = this->parse_expr (strm);
            
            // =>
            this->expect (TOK_RDARROW, strm);
            
            // <body>
            auto body = this->parse_expr (strm);
            
            // ;
            this->expect (TOK_SCOL, strm);
            
            ast->add_case (pat, body);
          }
        else if (tok.type == TOK_ELSE)
          {
            strm.next ();
            
            // =>
            this->expect (TOK_RDARROW, strm);
            
            // <body>
            auto body = this->parse_expr (strm);
            
            // ;
            this->expect (TOK_SCOL, strm);
            
            ast->set_else_case (body);
          }
        else
          throw parse_error ("unexpected token in match expression", tok.ln, tok.col);
      }
    
    return ast;
  }
  
  
  
  std::shared_ptr<ast_subscript>
  parser::parse_subscript (std::shared_ptr<ast_expr> expr,
                           lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    
    // [
    this->expect (TOK_LBRACKET, strm);
    
    auto index = this->parse_expr (strm);
    
    // ]
    this->expect (TOK_RBRACKET, strm);
    
    std::shared_ptr<ast_subscript> ast { new ast_subscript (expr, index) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_unop>
  parser::parse_unary (lexer::token_stream& strm)
  {
    ast_unop_type op;
    
    auto tok = strm.peek_next ();
    switch (tok.type)
      {
      case TOK_NOT: strm.next (); op = AST_UNOP_NOT; break;
      
      default:
        throw parse_error ("expected unary expression", tok.ln, tok.col);
      }
    
    std::shared_ptr<ast_unop> ast { new ast_unop (op, this->parse_expr_atom (strm)) };
    _set_ast_location (ast.get (), tok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_let>
  parser::parse_let (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_LET, strm);
    
    std::vector<std::pair<std::string, std::shared_ptr<ast_expr>>> defs;
    token tok;
    do
      {
        tok = strm.peek_next ();
        this->expect (TOK_IDENT, strm);
        std::string name = tok.val.str;
        
        this->expect (TOK_ASSIGN, strm);
        
        auto val = this->parse_expr (strm);
        
        defs.push_back (std::make_pair (name, val));
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_IN)
          throw parse_error ("expected ',' or 'in'", tok.ln, tok.col);
      }
    while (tok.type != TOK_IN);
    strm.next ();
    
    auto body = this->parse_expr (strm);
    
    std::shared_ptr<ast_let> ast { new ast_let (body) };
    for (auto& p : defs)
      ast->add_def (p.first, p.second);
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_n>
  parser::parse_n (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_N, strm);
    
    this->expect (TOK_COL, strm);
    
    auto prec = this->parse_expr (strm);
    auto body = this->parse_expr_block (strm);
    
    std::shared_ptr<ast_n> ast { new ast_n (prec, body) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_expr>
  parser::parse_expr_atom_main (lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    switch (tok.type)
      {
      case TOK_LPAREN:
        {
          strm.next ();
          auto e = this->parse_expr (strm);
          
          tok = strm.peek_next ();
          if (tok.type != TOK_RPAREN)
            throw parse_error ("expected matching ')'", tok.ln, tok.col);
          strm.next ();
          return e;
        }
      
      case TOK_LBRACE:
        return this->parse_expr_block (strm);
      
      case TOK_INTEGER:
        return this->parse_integer (strm);
      
      case TOK_FLOAT:
        return this->parse_float (strm);
      
      case TOK_IDENT:
        return this->parse_ident (strm);
      
      case TOK_ATOM:
        return this->parse_atom (strm);
      
      case TOK_STRING:
        return this->parse_string (strm);
      
      case TOK_NIL:
        strm.next ();
        return std::shared_ptr<ast_nil> (new ast_nil ());
      
      case TOK_TRUE:
        strm.next ();
        return std::shared_ptr<ast_bool> (new ast_bool (true));
      
      case TOK_FALSE:
        strm.next ();
        return std::shared_ptr<ast_bool> (new ast_bool (false));
      
      case TOK_LBRACKET:
        return this->parse_vector (strm);
      
      case TOK_FUN:
        return this->parse_fun (strm);
      
      case TOK_IF:
        return this->parse_if (strm);
      
      case TOK_LIST_START:
        return this->parse_list (strm);
      
      case TOK_MATCH:
        return this->parse_match (strm);
      
      case TOK_LET:
        return this->parse_let (strm);
      
      case TOK_N:
        return this->parse_n (strm);
      
      
      case TOK_NOT:
        return this->parse_unary (strm);
      
      default:
        throw parse_error ("unexpected token encountered when parsing atom",
          tok.ln, tok.col);
      }
  }
  
  std::shared_ptr<ast_expr>
  parser::parse_expr_atom_rest (std::shared_ptr<ast_expr> expr,
                           lexer::token_stream& strm)
  {
    auto tok = strm.peek_next ();
    switch (tok.type)
      {
      case TOK_LPAREN:
        return this->parse_fun_call (expr, strm);
      
      case TOK_LBRACKET:
        return this->parse_subscript (expr, strm);
      
      default:
        return expr;
      }
  }
  
  std::shared_ptr<ast_expr>
  parser::parse_expr_atom (lexer::token_stream& strm)
  {
    return this->parse_expr_atom_rest (this->parse_expr_atom_main (strm), strm);
  }
  
  
  
#define MAX_PRECEDENCE 5
  
  enum op_assoc {
    ASSOC_LEFT,
    ASSOC_RIGHT,
  };
  
  struct binop_info {
    ast_binop_type op;
    int prec;
    op_assoc assoc;
  };
  
  static std::unordered_map<int, binop_info> _binop_map {
    { TOK_ASSIGN, { AST_BINOP_ASSIGN, 0, ASSOC_RIGHT } },
    { TOK_DEF,    { AST_BINOP_DEF, 0, ASSOC_RIGHT } },
    { TOK_AND,    { AST_BINOP_AND, 1, ASSOC_LEFT } },
    { TOK_OR,     { AST_BINOP_OR, 1, ASSOC_LEFT } },
    { TOK_EQ,     { AST_BINOP_EQ, 2, ASSOC_LEFT } },
    { TOK_NEQ,    { AST_BINOP_NEQ, 2, ASSOC_LEFT } },
    { TOK_GT,     { AST_BINOP_GT, 2, ASSOC_LEFT } },
    { TOK_GTE,    { AST_BINOP_GTE, 2, ASSOC_LEFT } },
    { TOK_LT,     { AST_BINOP_LT, 2, ASSOC_LEFT } },
    { TOK_LTE,    { AST_BINOP_LTE, 2, ASSOC_LEFT } },
    { TOK_ADD,    { AST_BINOP_ADD, 3, ASSOC_LEFT } },
    { TOK_SUB,    { AST_BINOP_SUB, 3, ASSOC_LEFT } },
    { TOK_MUL,    { AST_BINOP_MUL, 4, ASSOC_LEFT } },
    { TOK_DIV,    { AST_BINOP_DIV, 4, ASSOC_LEFT } },
    { TOK_PERC,   { AST_BINOP_MOD, 4, ASSOC_LEFT } },
    { TOK_POW,    { AST_BINOP_POW, 5, ASSOC_RIGHT } },
  };
  
  std::shared_ptr<ast_expr>
  parser::parse_binop (lexer::token_stream& strm, int level)
  {
    if (level > MAX_PRECEDENCE)
      return this->parse_expr_atom (strm);
    
    auto lhs = this->parse_binop (strm, level + 1);
    
    auto tok = strm.peek_next ();
    auto ftok = tok;
    auto itr = _binop_map.find (tok.type);
    if (itr == _binop_map.end ())
      return lhs;
    
    auto& op = itr->second;
    if (op.prec != level)
      return lhs;
    
    if (op.assoc == ASSOC_LEFT)
      return this->parse_binop_rest (strm, lhs, level);
    
    strm.next ();
    auto rhs = this->parse_binop (strm, level);
    
    auto ast = std::shared_ptr<ast_expr> (new ast_binop (op.op, lhs, rhs));
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  // used to enforce left-associativity.
  std::shared_ptr<ast_expr>
  parser::parse_binop_rest (lexer::token_stream& strm, std::shared_ptr<ast_expr> lhs, int level)
  {
    auto tok = strm.peek_next ();
    
    auto itr = _binop_map.find (tok.type);
    if (itr == _binop_map.end ())
      return lhs;
    
    auto& op = _binop_map[tok.type];
    if (op.prec != level)
      return lhs;
    
    strm.next ();
    
    auto rhs = this->parse_binop (strm, level + 1);
    auto nexpr = new ast_binop (op.op, lhs, rhs);
    return this->parse_binop_rest (strm, std::shared_ptr<ast_expr> (nexpr), level);
  }
  
  
  
  std::shared_ptr<ast_expr>
  parser::parse_expr (lexer::token_stream& strm)
  {
    return this->parse_binop (strm, 0);
  }
  
  
  
  std::shared_ptr<ast_expr_stmt>
  parser::parse_expr_stmt (lexer::token_stream& strm, bool in_block)
  {
    auto ftok = strm.peek_next ();
    
    auto e = this->parse_expr (strm);
    
    this->consume_scol (strm, in_block);
    
    auto ast = std::shared_ptr<ast_expr_stmt> (new ast_expr_stmt (e));
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_var_def>
  parser::parse_var_def (lexer::token_stream& strm, bool in_block)
  {
    auto ftok = strm.peek_next ();
    
    // var
    this->expect (TOK_VAR, strm);
    
    // ident
    std::string var;
    token tok = strm.peek_next ();
    if (tok.type != TOK_IDENT)
      throw parse_error ("expected identifier after 'var'", tok.ln, tok.col);
    strm.next ();
    var = tok.val.str;
    
    // =
    this->expect (TOK_ASSIGN, strm);
    
    auto val = this->parse_expr (strm);
    
    this->consume_scol (strm, in_block);
    
    auto ast = std::shared_ptr<ast_var_def> (
      new ast_var_def (std::shared_ptr<ast_ident> (new ast_ident (var)), val));
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_stmt_block>
  parser::parse_stmt_block (lexer::token_stream& strm)
  {
    std::shared_ptr<ast_stmt_block> blk { new ast_stmt_block () };
    _set_ast_location (blk.get (), strm.peek_next (), this->path);
    
    // {
    this->expect (TOK_LBRACE, strm);
    
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_RBRACE)
          { strm.next (); break; }
        
        blk->push_back (this->parse_stmt (strm, true));
      }
    
    return blk;
  }
  
  
  
  std::shared_ptr<ast_expr_block>
  parser::parse_expr_block (lexer::token_stream& strm)
  {
    std::shared_ptr<ast_expr_block> blk { new ast_expr_block () };
    _set_ast_location (blk.get (), strm.peek_next (), this->path);
    
    // {
    this->expect (TOK_LBRACE, strm);
    
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_RBRACE)
          { strm.next (); break; }
        
        blk->push_back (this->parse_stmt (strm, true));
      }
    
    return blk;
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_module (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_MODULE, strm);
    
    auto tok = strm.next ();
    if (tok.type != TOK_IDENT)
      throw parse_error ("expected module name after 'module'", tok.ln, tok.col);
    std::string mname = tok.val.str;
    
    this->consume_scol (strm);
    
    std::shared_ptr<ast_module> ast { new ast_module (mname) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_import (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_IMPORT, strm);
    
    auto tok = strm.next ();
    if (tok.type != TOK_IDENT)
      throw parse_error ("expected module name after 'import'", tok.ln, tok.col);
    std::string mname = tok.val.str;
    
    this->consume_scol (strm);
    
    std::shared_ptr<ast_import> ast { new ast_import (mname) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_export (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    std::shared_ptr<ast_export> ast { new ast_export () };
    _set_ast_location (ast.get (), ftok, this->path);
    
    this->expect (TOK_EXPORT, strm);
    
    this->expect (TOK_LPAREN, strm);
    
    for (;;)
      {
        auto tok = strm.peek_next ();
        if (tok.type == TOK_RPAREN)
          { strm.next (); break; }
        else if (tok.type != TOK_IDENT)
          throw parse_error ("unexpected token encountered in export list", tok.ln, tok.col);
          
        std::string name = tok.val.str;
        ast->add_export (name);
        strm.next ();
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_RPAREN)
          throw parse_error ("unexpected ',' or ')' in export list", tok.ln, tok.col);
      }
    
    return ast;
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_ret (lexer::token_stream& strm, bool in_block)
  {
    auto ftok = strm.peek_next ();
    
    this->expect (TOK_RET, strm);
    
    auto tok = strm.peek_next ();
    if (tok.type == TOK_EOF || tok.type == TOK_SCOL || tok.type == TOK_RBRACE)
      {
        std::shared_ptr<ast_ret> ast { new ast_ret () };
        _set_ast_location (ast.get (), ftok, this->path);
        this->consume_scol (strm, in_block);
        return ast;
      }
    else
      {
        std::shared_ptr<ast_ret> ast { new ast_ret (this->parse_expr (strm)) };
        _set_ast_location (ast.get (), ftok, this->path);
        this->consume_scol (strm, in_block);
        return ast;
      }
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_namespace (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_NAMESPACE, strm);
    
    auto tok = strm.next ();
    if (tok.type != TOK_IDENT)
      throw parse_error ("expected name after 'namespace'", tok.ln, tok.col);
    std::string name = tok.val.str;
    
    auto body = this->parse_stmt_block (strm);
    
    std::shared_ptr<ast_namespace> ast { new ast_namespace (name, body) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_atom_def>
  parser::parse_atom_def (lexer::token_stream& strm, bool in_block)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_ATOMK, strm);
    
    auto tok = strm.next ();
    if (tok.type != TOK_ATOM)
      throw parse_error ("expected atom name after 'atom'", tok.ln, tok.col);
    std::string name = tok.val.str;
    
    this->consume_scol (strm, in_block);
    
    std::shared_ptr<ast_atom_def> ast { new ast_atom_def (name) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_using>
  parser::parse_using (lexer::token_stream& strm, bool in_block)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_USING, strm);
    
    auto tok = strm.peek_next ();
    this->expect (TOK_IDENT, strm);
    std::string fst = tok.val.str;
    
    std::string snd;
    tok = strm.peek_next ();
    if (tok.type == TOK_ASSIGN)
      {
        strm.next ();
        tok = strm.peek_next ();
        this->expect (TOK_IDENT, strm);
        snd = tok.val.str;
      }
    
    this->consume_scol (strm, in_block);
    
    std::shared_ptr<ast_using> ast { snd.empty () ? new ast_using (fst) : new ast_using (snd, fst) };
    _set_ast_location (ast.get (), ftok, this->path);
    return ast;
  }
  
  
  
  std::shared_ptr<ast_fun_def>
  parser::parse_fun_def (lexer::token_stream& strm)
  {
    auto ftok = strm.peek_next ();
    this->expect (TOK_FUN, strm);
    
    auto tok = strm.peek_next ();
    if (tok.type != TOK_IDENT)
      {
        strm.prev ();
        return {};
      }
    
    strm.next ();
    std::string name = tok.val.str;
    
    // (
    tok = strm.peek_next ();
    if (tok.type != TOK_LPAREN)
      throw parse_error (
        "expected '(' at beginning of function parameter list",
        tok.ln, tok.col);
    strm.next ();
    
    // params
    std::vector<std::string> params;
    while ((tok = strm.peek_next ()).type != TOK_EOF && tok.type != TOK_RPAREN)
      {
        if (tok.type == TOK_MUL)
          {
            strm.next ();
            params.push_back ("*" + this->parse_ident (strm)->get_value ());
          }
        else
          params.push_back (this->parse_ident (strm)->get_value ());
        
        tok = strm.peek_next ();
        if (tok.type == TOK_COMMA)
          strm.next ();
        else if (tok.type != TOK_RPAREN)
          throw parse_error ("expected ',' or ')' in function parameter list",
            tok.ln, tok.col);
      }
    if (tok.type == TOK_RPAREN)
      strm.next ();
    else if (tok.type == TOK_EOF)
      throw parse_error ("unexpected EOF in function parameter list", tok.ln, tok.col);
    
    // guard
    std::shared_ptr<ast_expr> guard;
    tok = strm.peek_next ();
    if (tok.type == TOK_OR)
      {
        strm.next ();
        guard = this->parse_expr (strm);
      }
    
    // body
    auto body = this->parse_stmt_block (strm);
    
    std::shared_ptr<ast_fun_def> ast { new ast_fun_def (name) };
    _set_ast_location (ast.get (), ftok, this->path);
    ast->set_body (body);
    ast->set_guard (guard);
    for (auto p : params)
      ast->add_param (p);
    return ast;
  }
  
  
  
  static std::shared_ptr<ast_expr>
  _escape_pattern (std::shared_ptr<ast_expr> pat)
  {
    auto npat = std::static_pointer_cast<ast_expr> (pat->clone ());
    
    ast_tools::traverse_dfs (npat,
      [] (std::shared_ptr<ast_node> node) -> traverse_result {
        if (node->get_type () == AST_IDENT)
          {
            auto cn = std::static_pointer_cast<ast_ident> (node);
            cn->set_value ("___" + cn->get_value ());
          }
      
        return TR_CONTINUE;
      });
    
    return npat;
  }
  
  std::shared_ptr<ast_stmt>
  parser::convert_def (std::shared_ptr<ast_binop> bop)
  {
    if (bop->get_lhs ()->get_type () == AST_IDENT)
      {
        // <ident> := <val>
        //   -- turns into --
        // var <ident> = <val>;
        
        auto ast = std::shared_ptr<ast_var_def> (
          new ast_var_def (
            std::static_pointer_cast<ast_ident> (bop->get_lhs ()),
            bop->get_rhs ()));
        ast->set_location (bop->get_location ());
        return ast;
      }
    else if (bop->get_lhs ()->get_type () == AST_FUN_CALL)
      {
        auto fc = std::static_pointer_cast<ast_fun_call> (bop->get_lhs ());
        if (fc->get_fun ()->get_type () == AST_IDENT)
          {
            // <ident>(<a1>, ..., <aN>) := <expr>
            //   -- turns into --
            // fun <ident>(<p1>, ..., <pN>) | match <p1> { case <a1>: true; }
            //                             && match <pN> { case <aN>: true; }
            //                              { expr }
            
            std::shared_ptr<ast_fun_def> ast { new ast_fun_def (
              std::static_pointer_cast<ast_ident> (fc->get_fun ())->get_value ()) };
            ast->set_location (bop->get_location ());
            
            int mi = 0;
            std::vector<std::shared_ptr<ast_expr>> conds;
            std::unordered_map<std::string, std::shared_ptr<ast_expr>> pats;
            for (auto arg : fc->get_args ())
              {
                if (arg->get_type () == AST_IDENT)
                  ast->add_param (std::static_pointer_cast<ast_ident> (arg)->get_value ());
                else
                  {
                    std::ostringstream ss;
                    ss << "____param" << (++mi);
                    std::string arg_name = ss.str ();
                    ast->add_param (arg_name);
                    
                    // create match statement
                    int name_count = ast_tools::extract_idents (arg).size ();
                    if (name_count == 0)
                      {
                        // if there are no variables involved, just check for
                        // equality.
                        
                        conds.push_back (std::shared_ptr<ast_expr> (
                          new ast_binop (AST_BINOP_EQ,
                            std::shared_ptr<ast_ident> (new ast_ident (arg_name)),
                            arg)));
                      }
                    else
                      {
                        // create match expression
                        
                        auto match = std::shared_ptr<ast_match> (
                          new ast_match (std::shared_ptr<ast_ident> (
                            new ast_ident (arg_name))));
                        match->add_case (arg,
                          std::shared_ptr<ast_bool> (new ast_bool (true)));
                        conds.push_back (match);
                        
                        pats[arg_name] = _escape_pattern (arg);
                      }
                  }
              }
            
            // create guard expression
            if (!conds.empty ())
              {
                if (conds.size () == 1)
                  ast->set_guard (conds.front ());
                else
                  {
                    auto guard = std::static_pointer_cast<ast_expr> (conds[0]);
                    for (size_t i = 1; i < conds.size (); ++i)
                      {
                        // 
                        // TODO: Replace current And operator with a true
                        // short-circuiting And!!!!!!!!
                        //
                        
                        guard = std::shared_ptr<ast_expr> (new ast_binop (
                          AST_BINOP_AND, guard, conds[i]));
                      }
                    
                    ast->set_guard (guard);
                  }
              }
            
            // set body
            auto body = std::shared_ptr<ast_stmt_block> (new ast_stmt_block ());
            
            for (auto p : pats)
              {
                auto& arg_name = p.first;
                auto pat = p.second;
              
                auto names = ast_tools::extract_idents (pat);
                for (auto& n : names)
                  body->push_back (std::shared_ptr<ast_var_def> (new ast_var_def (
                    std::shared_ptr<ast_ident> (new ast_ident (n.substr (3))),
                    std::shared_ptr<ast_nil> (new ast_nil ()))));
                
                auto case_body = std::shared_ptr<ast_expr_block> (new ast_expr_block ());
                for (auto& n : names)
                  case_body->push_back (std::shared_ptr<ast_expr_stmt> (
                    new ast_expr_stmt (std::shared_ptr<ast_binop> (new ast_binop (
                      AST_BINOP_ASSIGN,
                      std::shared_ptr<ast_ident> (new ast_ident (n.substr (3))),
                      std::shared_ptr<ast_ident> (new ast_ident (n)))))));
                
                auto match = std::shared_ptr<ast_match> (new ast_match (
                  std::shared_ptr<ast_ident> (new ast_ident (arg_name))));
                match->add_case (pat, case_body);
                body->push_back (std::shared_ptr<ast_expr_stmt> (new ast_expr_stmt (match)));
              }
            
            body->push_back (std::shared_ptr<ast_expr_stmt> (
              new ast_expr_stmt (bop->get_rhs ())));
            ast->set_body (body);
            
            return ast;
          }
      }
    
    return std::shared_ptr<ast_stmt> ();
  }
  
  
  
  std::shared_ptr<ast_stmt>
  parser::parse_stmt (lexer::token_stream& strm, bool in_block)
  {
    auto tok = strm.peek_next ();
    switch (tok.type)
      {
      case TOK_SCOL:
        strm.next ();
        return std::shared_ptr<ast_stmt> (new ast_empty_stmt ());
      
      case TOK_VAR:
        return this->parse_var_def (strm, in_block);
      
      case TOK_LBRACE:
        return this->parse_stmt_block (strm);
      
      case TOK_MODULE:
        return this->parse_module (strm);
      
      case TOK_IMPORT:
        return this->parse_import (strm);
      
      case TOK_EXPORT:
        return this->parse_export (strm);
      
      case TOK_RET:
        return this->parse_ret (strm, in_block);
      
      case TOK_NAMESPACE:
        return this->parse_namespace (strm);
      
      case TOK_ATOMK:
        return this->parse_atom_def (strm);
      
      case TOK_USING:
        return this->parse_using (strm, in_block);
      
      case TOK_FUN:
        {
          auto ast = this->parse_fun_def (strm);
          if (ast)
            return ast;
          
          // might be an anonymous function.
          return this->parse_expr_stmt (strm, in_block);
        }
      
      default:
        {
          auto es = this->parse_expr_stmt (strm, in_block);
          if (es->get_expr ()->get_type () == AST_BINOP)
            {
              auto bop = std::static_pointer_cast<ast_binop> (es->get_expr ());
              if (bop->get_op () == AST_BINOP_DEF)
                {
                  auto res = this->convert_def (bop);
                  if (res)
                    return res;
                }
            }
          
          return es;
        }
      }
  }
}

