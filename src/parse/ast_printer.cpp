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

#include "parse/ast_printer.hpp"
#include <iostream>
#include <string>
#include <stdexcept>


namespace rho {
  
  ast_printer::ast_printer (int indent)
  {
    this->indent = indent;
  }
  
  
  
  /* 
   * Pretty prints the specified AST node onto the standard stream.
   */
  void
  ast_printer::print (std::shared_ptr<ast_node> node)
  {
    this->print (node, 0);
  }
  
  
  
  void
  ast_printer::print (std::shared_ptr<ast_node> node, int level)
  {
    switch (node->get_type ())
      {
      case AST_PROGRAM:
        this->print_program (std::static_pointer_cast<ast_program> (node), level);
        break;
      
      case AST_EXPR_STMT:
        this->print_expr_stmt (std::static_pointer_cast<ast_expr_stmt> (node), level);
        break;
      
      case AST_STMT_BLOCK:
        this->print_stmt_block (std::static_pointer_cast<ast_stmt_block> (node), level);
        break;
      
      case AST_EXPR_BLOCK:
        this->print_expr_block (std::static_pointer_cast<ast_expr_block> (node), level);
        break;
      
      case AST_INTEGER:
        this->print_integer (std::static_pointer_cast<ast_integer> (node), level);
        break;
      
      case AST_IDENT:
        this->print_ident (std::static_pointer_cast<ast_ident> (node), level);
        break;
      
      case AST_BINOP:
        this->print_binop (std::static_pointer_cast<ast_binop> (node), level);
        break;
      
      case AST_VAR_DEF:
        this->print_var_def (std::static_pointer_cast<ast_var_def> (node), level);
        break;
      
      default:
        throw std::runtime_error ("ast_printer: not implemented");
      }
  }
  
  void
  ast_printer::print_program (std::shared_ptr<ast_program> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Program (" << node->get_stmts ().size () << " child exprs):" << std::endl;
    
    for (auto stmt : node->get_stmts ())
      this->print (stmt, level + 1);
  }
  
  void
  ast_printer::print_expr_stmt (std::shared_ptr<ast_expr_stmt> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Expression Statement:" << std::endl;
    
    this->print (node->get_expr (), level + 1);
  }
  
  void
  ast_printer::print_stmt_block (std::shared_ptr<ast_stmt_block> node, int level)
  {
    
  }
  
  void
  ast_printer::print_expr_block (std::shared_ptr<ast_expr_block> node, int level)
  {
    
  }
  
  void
  ast_printer::print_integer (std::shared_ptr<ast_integer> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Integer: " << node->get_value () << std::endl;
  }
  
  void
  ast_printer::print_ident (std::shared_ptr<ast_ident> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Identifier: " << node->get_value () << std::endl;
  }
  
  void
  ast_printer::print_var_def (std::shared_ptr<ast_var_def> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Variable definition:" << std::endl;
    
    this->print (node->get_var (), level + 1);
    this->print (node->get_val (), level + 1);
  }
  
  
  static std::string
  _binop_type_to_str (ast_binop_type type)
  {
    switch (type)
      {
        case AST_BINOP_ADD: return "Add";
        case AST_BINOP_SUB: return "Sub";
        case AST_BINOP_MUL: return "Mul";
        case AST_BINOP_DIV: return "Div";
        case AST_BINOP_POW: return "Pow";
        case AST_BINOP_MOD: return "Mod";
        case AST_BINOP_EQ:  return "==";
        case AST_BINOP_NEQ: return "/=";
        case AST_BINOP_LT:  return "<";
        case AST_BINOP_LTE: return "<=";
        case AST_BINOP_GT:  return ">";
        case AST_BINOP_GTE: return ">=";
        case AST_BINOP_AND: return "And";
        case AST_BINOP_OR: return "Or";
      }
    
    return "Undefined";
  }
  
  void
  ast_printer::print_binop (std::shared_ptr<ast_binop> node, int level)
  {
    this->output_spaces (level);
    std::cout << "Binop (" << _binop_type_to_str (node->get_op ()) << "):" << std::endl;
    
    this->print (node->get_lhs (), level + 1);
    this->print (node->get_rhs (), level + 1);
  }
  
  
  
  void 
  ast_printer::output_spaces (int level)
  {
    std::cout << std::string (level * this->indent, ' ');
    if (level > 0)
      std::cout << " - ";
  }
}

