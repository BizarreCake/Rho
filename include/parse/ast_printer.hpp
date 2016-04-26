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

#ifndef _RHO__PARSE__AST_PRINTER__H_
#define _RHO__PARSE__AST_PRINTER__H_

#include "parse/ast.hpp"


namespace rho {
  
  /* 
   * AST pretty printer.
   */
  class ast_printer
  {
    int indent;
    
  public:
    ast_printer (int indent = 2);
    
  public:
    /* 
     * Pretty prints the specified AST node onto the standard stream.
     */
    void print (std::shared_ptr<ast_node> node);
    
  private:
    void print (std::shared_ptr<ast_node> node, int level);
    void print_program (std::shared_ptr<ast_program> node, int level);
    void print_expr_stmt (std::shared_ptr<ast_expr_stmt> node, int level);
    void print_expr_block (std::shared_ptr<ast_expr_block> node, int level);
    void print_stmt_block (std::shared_ptr<ast_stmt_block> node, int level);
    void print_integer (std::shared_ptr<ast_integer> node, int level);
    void print_ident (std::shared_ptr<ast_ident> node, int level);
    void print_binop (std::shared_ptr<ast_binop> node, int level);
    void print_var_def (std::shared_ptr<ast_var_def> node, int level);
    
    void output_spaces (int level);
  };
}

#endif

