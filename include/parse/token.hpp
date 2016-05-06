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

#ifndef _RHO__PARSE__TOKEN__H_
#define _RHO__PARSE__TOKEN__H_

#include <string>


namespace rho {
  
  enum token_type
  {
    TOK_INVALID,
    TOK_EOF,
    
    // punctuation:
    TOK_LPAREN,       // (
    TOK_RPAREN,       // )
    TOK_LBRACE,       // {
    TOK_RBRACE,       // }
    TOK_LBRACKET,     // [
    TOK_RBRACKET,     // ]
    TOK_SCOL,         // ;
    TOK_COMMA,        // ,
    TOK_ADD,          // +
    TOK_SUB,          // -
    TOK_MUL,          // *
    TOK_DIV,          // /
    TOK_POW,          // ^
    TOK_PERC,         // %
    TOK_ASSIGN,       // =
    TOK_EQ,           // ==
    TOK_NEQ,          // /=
    TOK_LT,           // <
    TOK_LTE,          // <=
    TOK_GT,           // >
    TOK_GTE,          // >=
    TOK_LIST_START,   // '(
    TOK_DOT,          // .
    TOK_RDARROW,      // =>
    TOK_AND,          // &
    TOK_OR,           // |
    TOK_NOT,          // !
    TOK_COL,          // :
    
    // datums:
    TOK_INTEGER,
    TOK_IDENT,
    TOK_NIL,
    TOK_TRUE,         // true
    TOK_FALSE,        // false
    TOK_ATOM,
    TOK_STRING,
    TOK_FLOAT,
    
    // keywords:
    TOK_VAR,
    TOK_FUN,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_MATCH,
    TOK_CASE,
    TOK_MODULE,
    TOK_IMPORT,
    TOK_EXPORT,
    TOK_RET,
    TOK_NAMESPACE,
    TOK_ATOMK,        // atom
    TOK_USING,
    TOK_LET,
    TOK_IN,
    TOK_N,
  };
  
  
  /* 
   * Returns a textual representation of the specified token type.
   */
  std::string token_type_to_str (token_type type);
  
  
  struct token
  {
    token_type type;
    int ln, col;
    
    union
      {
        char *str;
      } val;
  };
  
  /* 
   * Releases memory used by the specified token.
   * NOTE: Does not actually reclaim memory used by the token itself.
   */
  void destroy_token (token& tok);
}

#endif

