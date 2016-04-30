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

#ifndef _RHO__PARSE__PARSER__H_
#define _RHO__PARSE__PARSER__H_

#include "parse/ast.hpp"
#include "parse/lexer.hpp"
#include <memory>
#include <string>


namespace rho {
  
  /* 
   * Thrown by the parser when a syntax error is encountered.
   */
  class parse_error: public std::runtime_error
  {
    int ln, col;
    
  public:
    inline int get_line () const { return this->ln; }
    inline int get_column () const { return this->col; }
    
  public:
    parse_error (const std::string& str, int ln, int col)
      : std::runtime_error (str), ln (ln), col (col)
      { }
  };
  
  
  
  /* 
   * The parser constructs an AST from a stream of tokens generated in a
   * previous stage by the lexer.
   *
   * The underlying implementation is a simple recursive-descent parser.
   */
  class parser
  {
    std::string path;
    
  public:
    /* 
     * Parses the specified stream of tokens.
     * Throws `parse_error' in case of a syntax error.
     */
    std::shared_ptr<ast_program> parse (lexer::token_stream strm,
                                        const std::string& path);
    
  private:
    std::shared_ptr<ast_program> parse_program (lexer::token_stream& strm);
    
    std::shared_ptr<ast_integer> parse_integer (lexer::token_stream& strm);
    std::shared_ptr<ast_ident> parse_ident (lexer::token_stream& strm);
    std::shared_ptr<ast_vector> parse_vector (lexer::token_stream& strm);
    std::shared_ptr<ast_atom> parse_atom (lexer::token_stream& strm);
    std::shared_ptr<ast_string> parse_string (lexer::token_stream& strm);
    
    std::shared_ptr<ast_expr> parse_expr_atom (lexer::token_stream& strm);
    std::shared_ptr<ast_expr> parse_expr_atom_main (lexer::token_stream& strm);
    std::shared_ptr<ast_expr> parse_expr_atom_rest (std::shared_ptr<ast_expr> expr,
                                               lexer::token_stream& strm);
    std::shared_ptr<ast_unop> parse_unary (lexer::token_stream& strm);
    std::shared_ptr<ast_expr> parse_binop (lexer::token_stream& strm, int level);
    std::shared_ptr<ast_expr> parse_binop_rest (lexer::token_stream& strm,
                                                std::shared_ptr<ast_expr> expr,
                                                int level);
    std::shared_ptr<ast_fun> parse_fun (lexer::token_stream& strm);
    std::shared_ptr<ast_fun_call> parse_fun_call (std::shared_ptr<ast_expr> expr,
                                                  lexer::token_stream& strm);
    std::shared_ptr<ast_if> parse_if (lexer::token_stream& strm);
    std::shared_ptr<ast_expr> parse_list (lexer::token_stream& strm);
    std::shared_ptr<ast_match> parse_match (lexer::token_stream& strm);
    std::shared_ptr<ast_subscript> parse_subscript (std::shared_ptr<ast_expr> expr,
                                                    lexer::token_stream& strm);
    std::shared_ptr<ast_expr> parse_expr (lexer::token_stream& strm);
    
    std::shared_ptr<ast_expr_stmt> parse_expr_stmt (lexer::token_stream& strm,
                                                    bool in_block = false);
    std::shared_ptr<ast_var_def> parse_var_def (lexer::token_stream& strm,
                                                bool in_block = false);
    std::shared_ptr<ast_stmt_block> parse_stmt_block (lexer::token_stream& strm);
    std::shared_ptr<ast_expr_block> parse_expr_block (lexer::token_stream& strm);
    std::shared_ptr<ast_stmt> parse_module (lexer::token_stream& strm);
    std::shared_ptr<ast_stmt> parse_import (lexer::token_stream& strm);
    std::shared_ptr<ast_stmt> parse_export (lexer::token_stream& strm);
    std::shared_ptr<ast_stmt> parse_ret (lexer::token_stream& strm,
                                         bool in_block = false);
    std::shared_ptr<ast_stmt> parse_namespace (lexer::token_stream& strm);
    std::shared_ptr<ast_atom_def> parse_atom_def (lexer::token_stream& strm,
                                                  bool in_block = false);
    std::shared_ptr<ast_using> parse_using (lexer::token_stream& strm,
                                            bool in_block = false);
    std::shared_ptr<ast_let> parse_let (lexer::token_stream& strm);
    std::shared_ptr<ast_stmt> parse_stmt (lexer::token_stream& strm,
                                          bool in_block = false);
    
    
    void expect (token_type type, lexer::token_stream& strm);
    void consume_scol (lexer::token_stream& strm, bool in_block = false);
  };
}

#endif

