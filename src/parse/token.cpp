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

#include "parse/token.hpp"


namespace rho {
  
  /* 
   * Releases memory used by the specified token.
   * NOTE: Does not actually reclaim memory used by the token itself.
   */
  void
  destroy_token (token& tok)
  {
    switch (tok.type)
      {
      case TOK_INTEGER:
      case TOK_IDENT:
      case TOK_ATOM:
      case TOK_STRING:
      case TOK_FLOAT:
        delete[] tok.val.str;
      
      default: ;
      }
  }
  
  
  
  /* 
   * Returns a textual representation of the specified token type.
   */
  std::string
  token_type_to_str (token_type type)
  {
    switch (type)
      {
      case TOK_INVALID:         return "<invalid>";
      case TOK_EOF:             return "<eof>";
      
      case TOK_LPAREN:          return "(";
      case TOK_RPAREN:          return ")";
      case TOK_LBRACE:          return "{";
      case TOK_RBRACE:          return "}";
      case TOK_LBRACKET:        return "[";
      case TOK_RBRACKET:        return "]";
      case TOK_SCOL:            return ";";
      case TOK_COMMA:           return ",";
      case TOK_ADD:             return "+";
      case TOK_SUB:             return "-";
      case TOK_MUL:             return "*";
      case TOK_DIV:             return "/";
      case TOK_POW:             return "^";
      case TOK_PERC:            return "%";
      case TOK_ASSIGN:          return "=";
      case TOK_EQ:              return "==";
      case TOK_NEQ:             return "/=";
      case TOK_LT:              return "<";
      case TOK_LTE:             return "<=";
      case TOK_GT:              return ">";
      case TOK_GTE:             return ">=";
      case TOK_LIST_START:      return "'(";
      case TOK_DOT:             return ".";
      case TOK_RDARROW:         return "=>";
      case TOK_AND:             return "&";
      case TOK_OR:              return "|";
      case TOK_NOT:             return "!";
      case TOK_COL:             return ":";
      case TOK_DEF:             return ":=";
      
      case TOK_INTEGER:         return "<integer>";
      case TOK_IDENT:           return "<ident>";
      case TOK_NIL:             return "nil";
      case TOK_TRUE:            return "true";
      case TOK_FALSE:           return "false";
      case TOK_ATOM:            return "<atom>";
      case TOK_STRING:          return "<string>";
      case TOK_FLOAT:           return "<float>";
      
      case TOK_VAR:             return "var";
      case TOK_FUN:             return "fun";
      case TOK_IF:              return "if";
      case TOK_THEN:            return "then";
      case TOK_ELSE:            return "else";
      case TOK_MATCH:           return "match";
      case TOK_CASE:            return "case";
      case TOK_MODULE:          return "module";
      case TOK_IMPORT:          return "import";
      case TOK_EXPORT:          return "export";
      case TOK_RET:             return "ret";
      case TOK_NAMESPACE:       return "namespace";
      case TOK_ATOMK:           return "atom";
      case TOK_USING:           return "using";
      case TOK_LET:             return "let";
      case TOK_IN:              return "in";
      case TOK_N:               return "N";
      }
    
    return "";
  }
}

