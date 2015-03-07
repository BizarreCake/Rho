/*
 * Rho - A math sandbox.
 * Copyright (C) 2014 Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "parser/parser.hpp"
#include "parser/lexer.hpp"
#include "common/errors.hpp"
#include "common/types.hpp"
#include <memory>
#include <unordered_map>
#include <cassert>


namespace rho {
  
  parser::parser (error_tracker& errs)
    : errs (errs)
    { }
  
  
  
  namespace {
    
    struct parser_state
    {
      error_tracker& errs;
      token_stream& toks;
      std::string file_name;
      
    public:
      parser_state (error_tracker& errs, token_stream& toks,
        const std::string& file_name)
        : errs (errs), toks (toks), file_name (file_name)
        { }
    };
  }
  
  // foward decs:
  static ast_expr* _parse_atom (parser_state& ps);
  static ast_expr* _parse_expr (parser_state& ps);
  static ast_stmt* _parse_stmt (parser_state& ps);
  static ast_block* _parse_block (parser_state& ps);
  
  
  
  static basic_type
  _parse_basic_type (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    switch (tok.type)
      {
      case TOK_INT:       return TYPE_INT;
      case TOK_SET:       return TYPE_SET;
      
      default:
        ps.errs.add (ET_ERROR, "expected type name", tok.ln, tok.col, ps.file_name);
        return TYPE_INVALID;
      }
  }
  
  
  
  static ast_block*
  _parse_blockified_expr (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_BLOCKIFY)
      {
        ps.errs.add (ET_ERROR, "expected '{:'", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> expr { _parse_expr (ps) };
    if (!expr.get ())
      return nullptr;
    
    ast_block *blk = new ast_block ();
    blk->add_stmt (new ast_expr_stmt (expr.release ()));
    return blk;
  }
  
  ast_block*
  _parse_block (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.peek_next ();
    if (tok.type == TOK_BLOCKIFY)
      return _parse_blockified_expr (ps);
    if (tok.type != TOK_LBRACE)
      {
        ps.errs.add (ET_ERROR, "expected block ('{')", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    else
      toks.next ();
    
    std::unique_ptr<ast_block> blk { new ast_block () };
    for (;;)
      {
        tok = toks.peek_next ();
        if (tok.type == TOK_RBRACE)
          {
            toks.next ();
            break;
          }
        else if (tok.type == TOK_EOF)
          {
            ps.errs.add (ET_ERROR, "unexpected eof inside block", tok.ln, tok.col, ps.file_name);
            return nullptr;
          }
        
        ast_stmt *stmt = _parse_stmt (ps);
        if (stmt)
          blk->add_stmt (stmt);
      }
    
    return blk.release ();
  }
  
  
  
  static ast_nil*
  _parse_nil (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_NIL)
      {
        ps.errs.add (ET_ERROR, "expected nil", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    return new ast_nil ();
  }
  
  
  static ast_ident*
  _parse_ident (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_IDENT)
      {
        ps.errs.add (ET_ERROR, "expected identifier", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    return new ast_ident (tok.val.str);
  }
  
  
  static ast_integer*
  _parse_integer (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_INTEGER)
      {
        ps.errs.add (ET_ERROR, "expected integer", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    return new ast_integer (tok.val.str);
  }
  
  
  static ast_real*
  _parse_real (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_REAL)
      {
        ps.errs.add (ET_ERROR, "expected real number", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    return new ast_real (tok.val.str);
  }
  
  
  static ast_sym*
  _parse_sym (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_SYM)
      {
        ps.errs.add (ET_ERROR, "expected symbol", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    return new ast_sym (tok.val.str);
  }
  
  
  
  static ast_function*
  _parse_function (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_FUN)
      {
        ps.errs.add (ET_ERROR, "expected function", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // function name
    std::string name;
    /*
    tok = toks.peek_next ();
    if (tok.type == TOK_IDENT)
      {
        name.assign (tok.val.str);
        toks.next ();
      }
    */
    
    std::unique_ptr<ast_function> func { new ast_function (name) };
    
    // arguments
    tok = toks.peek_next ();
    if (tok.type == TOK_LPAREN)
      {
        toks.next ();
        for (;;)
          {
            tok = toks.next ();
            if (tok.type == TOK_RPAREN)
              break;
            else if (tok.type == TOK_EOF)
              {
                ps.errs.add (ET_ERROR, "unexpected eof inside function parameter list",
                  tok.ln, tok.col, ps.file_name);
                return nullptr;
              }
            else if (tok.type != TOK_IDENT)
              {
                ps.errs.add (ET_ERROR, "expected parameter name inside function parameter list",
                  tok.ln, tok.col, ps.file_name);
                return nullptr;
              }
            
            std::string param = tok.val.str;
            func->add_param (param);
            
            tok = toks.peek_next ();
            if (tok.type == TOK_COMMA)
              {
                toks.next ();
                if (toks.peek_next ().type != TOK_IDENT)
                  {
                    ps.errs.add (ET_ERROR, "expected parameter name inside function parameter list",
                      tok.ln, tok.col, ps.file_name);
                    return nullptr;
                  }
              }
            else if (tok.type != TOK_RPAREN)
              {
                ps.errs.add (ET_ERROR, "expected ',' or ')' inside function parameter list",
                  tok.ln, tok.col, ps.file_name);
                return nullptr;
              }
          }
      }
    
    tok = toks.peek_next ();
    if (tok.type != TOK_LBRACE && tok.type != TOK_BLOCKIFY)
      {
        ps.errs.add (ET_ERROR, "expected '{'", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    ast_block *body = _parse_block (ps);
    if (!body)
      return nullptr;
    func->set_body (body);
    return func.release ();
  }
  
  
  
  static ast_if*
  _parse_if (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_IF)
      {
        ps.errs.add (ET_ERROR, "expected if expression",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> test { _parse_expr (ps) };
    if (!test.get ())
      return nullptr;
    
    tok = toks.next ();
    if (tok.type != TOK_THEN)
      {
        ps.errs.add (ET_ERROR, "expected 'then'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> conseq { _parse_expr (ps) };
    if (!conseq.get ())
      return nullptr;
    
    tok = toks.next ();
    if (tok.type != TOK_ELSE && tok.type != TOK_OTHERWISE)
      {
        ps.errs.add (ET_ERROR, "expected 'else' or 'otherwise'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> alt { _parse_expr (ps) };
    if (!alt.get ())
      return nullptr;
    
    return new ast_if (test.release (), conseq.release (), alt.release ());
  }
  
  
  
  static ast_n*
  _parse_n (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_N)
      {
        ps.errs.add (ET_ERROR, "expected 'N:'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> prec { _parse_atom (ps) };
    if (!prec.get ())
      return nullptr;
    
    std::unique_ptr<ast_block> body { _parse_block (ps) };
    if (!body.get ())
      return nullptr;
    
    return new ast_n (prec.release (), body.release ());
  }
  
  
  
  static ast_sum*
  _parse_sum (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_SUM)
      {
        ps.errs.add (ET_ERROR, "expected 'sum'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // variable
    std::unique_ptr<ast_expr> var { _parse_atom (ps) };
    if (!var.get () || var->get_type () != AST_IDENT)
      {
        ps.errs.add (ET_ERROR, "expected identifier after 'sum'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // <-
    tok = toks.next ();
    if (tok.type != TOK_RARROW)
      {
        ps.errs.add (ET_ERROR, "expected '<-' after sum variable",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // range start
    std::unique_ptr<ast_expr> start { _parse_atom (ps) };
    if (!start.get ())
      {
        ps.errs.add (ET_ERROR, "expected start of range after '<-'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // ..
    tok = toks.next ();
    if (tok.type != TOK_RANGE)
      {
        ps.errs.add (ET_ERROR, "expected '..' after start of range",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // range end
    std::unique_ptr<ast_expr> end;
    tok = toks.peek_next ();
    if (tok.type != TOK_LBRACE && tok.type != TOK_BLOCKIFY)
      {
        end.reset (_parse_atom (ps));
        if (!end.get ())
          {
            ps.errs.add (ET_ERROR, "expected end of range after '..'",
              tok.ln, tok.col, ps.file_name);
            return nullptr;
          }
      }
    
    // body
    tok = toks.peek_next ();
    if (tok.type != TOK_LBRACE && tok.type != TOK_BLOCKIFY)
      {
        ps.errs.add (ET_ERROR, "expected '{'", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    ast_block *body = _parse_block (ps);
    if (!body)
      return nullptr;
    
    return new ast_sum (static_cast<ast_ident *> (var.release ()),
      start.release (), end.release (), body);
  }
  
  
  
  static ast_product*
  _parse_product (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_PRODUCT)
      {
        ps.errs.add (ET_ERROR, "expected 'product'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // variable
    std::unique_ptr<ast_expr> var { _parse_atom (ps) };
    if (!var.get () || var->get_type () != AST_IDENT)
      {
        ps.errs.add (ET_ERROR, "expected identifier after 'product'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // <-
    tok = toks.next ();
    if (tok.type != TOK_RARROW)
      {
        ps.errs.add (ET_ERROR, "expected '<-' after product variable",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // range start
    std::unique_ptr<ast_expr> start { _parse_atom (ps) };
    if (!start.get ())
      {
        ps.errs.add (ET_ERROR, "expected start of range after '<-'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // ..
    tok = toks.next ();
    if (tok.type != TOK_RANGE)
      {
        ps.errs.add (ET_ERROR, "expected '..' after start of range",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // range end
    std::unique_ptr<ast_expr> end { _parse_atom (ps) };
    if (!end.get ())
      {
        ps.errs.add (ET_ERROR, "expected end of range after '..'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // body
    tok = toks.peek_next ();
    if (tok.type != TOK_LBRACE && tok.type != TOK_BLOCKIFY)
      {
        ps.errs.add (ET_ERROR, "expected '{'", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    ast_block *body = _parse_block (ps);
    if (!body)
      return nullptr;
    
    return new ast_product (static_cast<ast_ident *> (var.release ()),
      start.release (), end.release (), body);
  }
  
  
  
  static ast_list*
  _parse_list (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_LPAREN_LIST)
      {
        ps.errs.add (ET_ERROR, "expected list",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_list> ast { new ast_list () };
    
    for (;;)
      {
        tok = toks.peek_next ();
        if (tok.type == TOK_RPAREN)
          {
            toks.next ();
            break;
          }
        
        ast_expr *expr = _parse_expr (ps);
        if (!expr)
          return nullptr;
        ast->add_expr (expr);
      }
    
    return ast.release ();
  }
  
  
  
  static ast_expr*
  _parse_atom_main (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.peek_next ();
    switch (tok.type)
      {
      case TOK_LPAREN:
        {
          toks.next ();
          std::unique_ptr<ast_expr> expr { _parse_expr (ps) };
          if ((tok = toks.next ()).type != TOK_RPAREN)
            {
              ps.errs.add (ET_ERROR, "expected matching ')'", tok.ln, tok.col, ps.file_name);
              return nullptr;
            }
          
          return expr.release ();
        }
      
      case TOK_PARENIFY:
        {
          toks.next ();
          return _parse_expr (ps);
        }
      
      case TOK_NIL:
        return _parse_nil (ps);
      
      case TOK_IDENT:
        return _parse_ident (ps);
      
      case TOK_INTEGER:
        return _parse_integer (ps);
      
      case TOK_REAL:
        return _parse_real (ps);
      
      case TOK_SYM:
        return _parse_sym (ps);
      
      case TOK_FUN:
        return _parse_function (ps);
      
      case TOK_IF:
        return _parse_if (ps);
      
      case TOK_N:
        return _parse_n (ps);
      
      case TOK_SUM:
        return _parse_sum (ps);
      
      case TOK_PRODUCT:
        return _parse_product (ps);
      
      case TOK_SUB:
        {
          toks.next ();
          std::unique_ptr<ast_expr> expr { _parse_atom (ps) };
          if (!expr.get ())
            return nullptr;
          return new ast_unop (expr.release (), AST_UNOP_NEGATE);
        }
      
      case TOK_LPAREN_LIST:
        return _parse_list (ps);
      
      default:
        toks.next (); // skip it
        ps.errs.add (ET_ERROR, "expected atom", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
  }
  
  
  
  static ast_call*
  _parse_call (std::unique_ptr<ast_expr>& func, parser_state& ps)
  {
    auto& toks = ps.toks;
    bool parenify = (toks.next ().type == TOK_PARENIFY);
    
    token tok;
    std::unique_ptr<ast_call> call { new ast_call (func.release ()) };
    for (;;)
      {
        tok = toks.peek_next ();
        if (!parenify && tok.type == TOK_RPAREN)
          {
            toks.next ();
            break;
          }
        else if (tok.type == TOK_EOF)
          {
            ps.errs.add (ET_ERROR, "unexpected eof inside function call argument list",
              tok.ln, tok.col, ps.file_name);
            return nullptr;
          }
        
        ast_expr *arg = _parse_expr (ps);
        if (arg)
          call->add_arg (arg);
        
        tok = toks.peek_next ();
        if (tok.type == TOK_COMMA)
          toks.next ();
        else if (parenify)
          break;
        else if (tok.type != TOK_RPAREN)
          {
            ps.errs.add (ET_ERROR, "expected ',' or ')' inside function call argument list",
              tok.ln, tok.col, ps.file_name);
            return nullptr;
          }
      }
    
    return call.release ();
  }
  
  
  
  static ast_if*
  _parse_if_right (std::unique_ptr<ast_expr>& conseq, parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_IF)
      {
        ps.errs.add (ET_ERROR, "expected if expression",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> test { _parse_expr (ps) };
    if (!test.get ())
      return nullptr;
    
    tok = toks.next ();
    if (tok.type != TOK_ELSE && tok.type != TOK_OTHERWISE)
      {
        ps.errs.add (ET_ERROR, "expected 'else' or 'otherwise'",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> alt { _parse_expr (ps) };
    if (!alt.get ())
      return nullptr;
    
    return new ast_if (test.release (), conseq.release (), alt.release ());
  }
  
  
  
  static ast_subst*
  _parse_subst (std::unique_ptr<ast_expr>& expr, parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_SUBST)
      {
        ps.errs.add (ET_ERROR, "expected |.",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> sym { _parse_atom (ps) };
    if (!sym.get ())
      return nullptr;
    
    tok = toks.next ();
    if (tok.type != TOK_ASSIGN)
      {
         ps.errs.add (ET_ERROR, "expected = after symbol in substitution",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    std::unique_ptr<ast_expr> val { _parse_expr (ps) };
    if (!val.get ())
      return nullptr;
    
    return new ast_subst (expr.release (), sym.release (), val.release ());
  }
  
  
  
  static ast_expr*
  _parse_atom_rest (std::unique_ptr<ast_expr>& left, parser_state& ps)
  {
    if (!left.get ())
      return nullptr;
    
    auto& toks = ps.toks;
    std::unique_ptr<ast_expr> res;
    
    token tok = toks.peek_next ();
    switch (tok.type)
      {
      case TOK_PARENIFY:
      case TOK_LPAREN:
        res.reset (_parse_call (left, ps));
        break;
      
      case TOK_IF:
        res.reset (_parse_if_right (left, ps));
        break;
      
      case TOK_BANG:
        toks.next ();
        res.reset (new ast_unop (left.release (), AST_UNOP_FACTORIAL));
        break;
      
      case TOK_SUBST:
        res.reset (_parse_subst (left, ps));
        break;
      
      default:
        return left.release ();
      }
    
    return _parse_atom_rest (res, ps);
  }
  
  ast_expr*
  _parse_atom (parser_state& ps)
  {
    std::unique_ptr<ast_expr> atom { _parse_atom_main (ps) };
    if (!atom.get ())
      return nullptr;
    
    return _parse_atom_rest (atom, ps);
  }
  
  

  /* 
   * Binary operators.
   */
//------------------------------------------------------------------------------
  
  enum op_assoc {
    ASSOC_LEFT,
    ASSOC_RIGHT,
  };
  
  
// should be one greater than the higher number in the operator precedence
// map below.
#define PRECEDENCE_LEVELS   5
  
  static const std::unordered_map<int, int> _op_prec_map {
    { AST_BINOP_POW,      4 },
    { AST_BINOP_MUL,      3 },
    { AST_BINOP_DIV,      3 },
    { AST_BINOP_IDIV,      3 },
    { AST_BINOP_MOD,      3 },
    { AST_BINOP_ADD,      2 },
    { AST_BINOP_SUB,      2 },
    { AST_BINOP_EQ,       1 },
    { AST_BINOP_NEQ,      1 },
    { AST_BINOP_LT,       1 },
    { AST_BINOP_LTE,      1 },
    { AST_BINOP_GT,       1 },
    { AST_BINOP_GTE,      1 },
    { AST_BINOP_ASSIGN,   0 },
  };
  
  static const std::unordered_map<int, op_assoc> _op_assoc_map {
    { AST_BINOP_ASSIGN, ASSOC_RIGHT },
    { AST_BINOP_ADD,    ASSOC_LEFT },
    { AST_BINOP_SUB,    ASSOC_LEFT },
    { AST_BINOP_MUL,    ASSOC_LEFT },
    { AST_BINOP_DIV,    ASSOC_LEFT },
    { AST_BINOP_IDIV,   ASSOC_LEFT },
    { AST_BINOP_MOD,    ASSOC_LEFT },
    { AST_BINOP_POW,    ASSOC_RIGHT },
    { AST_BINOP_EQ,     ASSOC_LEFT },
    { AST_BINOP_NEQ,    ASSOC_LEFT },
    { AST_BINOP_LT,     ASSOC_LEFT },
    { AST_BINOP_LTE,    ASSOC_LEFT },
    { AST_BINOP_GT,     ASSOC_LEFT },
    { AST_BINOP_GTE,    ASSOC_LEFT },
  };
  
  static ast_binop_type
  _tok_to_binop (token_type type)
  {
    switch (type)
      {
      case TOK_ASSIGN:  return AST_BINOP_ASSIGN;
      
      case TOK_ADD:     return AST_BINOP_ADD;
      case TOK_SUB:     return AST_BINOP_SUB;
      case TOK_MUL:     return AST_BINOP_MUL;
      case TOK_DIV:     return AST_BINOP_DIV;
      case TOK_IDIV:    return AST_BINOP_IDIV;
      case TOK_MOD:     return AST_BINOP_MOD;
      case TOK_CARET:   return AST_BINOP_POW;
      
      case TOK_EQ:      return AST_BINOP_EQ;
      case TOK_NEQ:     return AST_BINOP_NEQ;
      case TOK_LT:      return AST_BINOP_LT;
      case TOK_LTE:     return AST_BINOP_LTE;
      case TOK_GT:      return AST_BINOP_GT;
      case TOK_GTE:     return AST_BINOP_GTE;
      
      default:
        return AST_BINOP_INVALID;
      }
  }
  
  
  static ast_expr* _parse_expr_bops (parser_state& ps, int level);
  
  // used to enforce left-associativity.
  static ast_expr*
  _parse_expr_bops_rest (ast_expr *left, parser_state& ps, int level)
  {
    auto& toks = ps.toks;
    
    token tok = toks.peek_next ();
    ast_binop_type bop = _tok_to_binop (tok.type);
    auto itr = _op_prec_map.find (bop);
    if (itr != _op_prec_map.end () && (PRECEDENCE_LEVELS - itr->second) == level)
      {
        toks.next (); // skip operator
        
        ast_expr *rhs = _parse_expr_bops (ps, level - 1);  // next level
        if (!rhs)
          return nullptr;
        
        ast_expr *nexpr = new ast_binop (left, rhs, bop);
        return _parse_expr_bops_rest (nexpr, ps, level);
      }
    
    return left;
  }
  
  ast_expr*
  _parse_expr_bops (parser_state& ps, int level)
  {
    if (level == 0)
      return _parse_atom (ps);
    
    std::unique_ptr<ast_expr> lhs { _parse_expr_bops (ps, level - 1) };
    if (!lhs.get ())
      return nullptr;
    
    auto& toks = ps.toks;
    token tok = toks.peek_next ();
    ast_binop_type bop = _tok_to_binop (tok.type);
    auto itr = _op_prec_map.find (bop);
    if (itr != _op_prec_map.end () && (PRECEDENCE_LEVELS - itr->second) == level)
      {
        auto itr = _op_assoc_map.find (bop);
        assert (itr != _op_assoc_map.end ());
        
        if (itr->second == ASSOC_RIGHT)
          {
            toks.next (); // skip operator
            
            ast_expr *rhs = _parse_expr_bops (ps, level);  // same level
            if (!rhs)
              return nullptr;
            
            return new ast_binop (lhs.release (), rhs, bop);
          }
        else
          {
            ast_expr *nexpr = _parse_expr_bops_rest (lhs.get (), ps, level);
            if (!nexpr)
              return nullptr;
            lhs.release ();
            return nexpr;
          }
      }
    
    return lhs.release ();
  }
  
  
  ast_expr*
  _parse_expr (parser_state& ps)
  {
    return _parse_expr_bops (ps, PRECEDENCE_LEVELS);
  }

//------------------------------------------------------------------------------
  
  
  
  static void
  _skip_scol (parser_state& ps)
  {
    auto& toks = ps.toks;
    token tok = toks.peek_next ();
    if (tok.type == TOK_RBRACE)
      ;
    else if (tok.type == TOK_SCOL)
      toks.next ();
    else
      {
        ps.errs.add (ET_ERROR, "expected ';'", tok.ln, tok.col, ps.file_name);
      }
  }
  
  
  
  static ast_let*
  _parse_let (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    token tok = toks.next ();
    if (tok.type != TOK_LET)
      {
        ps.errs.add (ET_ERROR, "expected let statement", tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    
    // variable name
    tok = toks.next ();
    if (tok.type != TOK_IDENT)
      {
        ps.errs.add (ET_ERROR, "expected identifier after 'let' in let statement",
          tok.ln, tok.col, ps.file_name);
        return nullptr;
      }
    std::string name = tok.val.str;
    
    // type
    basic_type bt = TYPE_UNSPEC;
    tok = toks.peek_next ();
    if (tok.type == TOK_DCOL)
      {
        toks.next ();
        bt = _parse_basic_type (ps);
        if (bt == TYPE_INVALID)
          return nullptr;
      }
    
    // value
    std::unique_ptr<ast_expr> val;
    tok = toks.peek_next ();
    if (tok.type == TOK_ASSIGN)
      {
        toks.next ();
        val.reset (_parse_expr (ps));
        if (!val.get ())
          return nullptr;
      }
    
    _skip_scol (ps);
    return new ast_let (name, val.release (), bt);
  }
  
  
  
  ast_stmt*
  _parse_stmt (parser_state& ps)
  {
    auto& toks = ps.toks;
    
// skips all tokens until a semicolon or right brace is found in case of an error.
#define RETURN_STATEMENT(EXPR)    \
  {                               \
    auto e = (EXPR);              \
    if (!e)                       \
      while ((tok = toks.next ()).type != TOK_EOF &&            \
             (tok.type != TOK_SCOL && tok.type != TOK_RBRACE))  \
        ;                                                       \
    return e;                                                   \
  }
  
    
    token tok = toks.peek_next ();
    switch (tok.type)
      {
      case TOK_LET:
        RETURN_STATEMENT(_parse_let (ps))
      
      default:
        {
          // treat it as an expression, if there aren't any matches.
          std::unique_ptr<ast_expr> expr { _parse_expr (ps) };
          _skip_scol (ps);
          if (expr.get ())
            return new ast_expr_stmt (expr.release ());
        }
        break;
      }
    
    return nullptr;
  }
  
  
  
  static ast_program*
  _parse_program (parser_state& ps)
  {
    auto& toks = ps.toks;
    
    std::unique_ptr<ast_program> ast { new ast_program () };
    
    token tok;
    while (toks.has_next ())
      {
        tok = toks.peek_next ();
        if (tok.type == TOK_EOF)
          break;
        
        ast_stmt *stmt = _parse_stmt (ps);
        if (stmt)
          ast->add_stmt (stmt);
      }
    
    return ast.release ();
  }
  
  
  /* 
   * Parses the Rho code in the specified character stream and returns an
   * AST tree.
   * The file name is used for error-tracking purposes.
   */
  ast_program*
  parser::parse (std::istream& strm, const std::string& file_name)
  {
    lexer lex { this->errs };
    lex.tokenize (strm, file_name);
    auto toks = lex.get_tokens ();
    
    parser_state ps { this->errs, toks, file_name };
    return _parse_program (ps);
  }
}

