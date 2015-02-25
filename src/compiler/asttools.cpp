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

#include "compiler/asttools.hpp"
#include <unordered_set>
#include <string>


namespace rho {
  
  namespace ast {
    
    static int _count_locs_needed_block (ast_block *ast);
    
    static int
    _count_locs_needed_rest (ast_node *ast, std::unordered_set<std::string>& vars)
    {
      int count = 0;
      
      switch (ast->get_type ())
        {
        case AST_PROGRAM:
        case AST_BLOCK:
          count += _count_locs_needed_block (static_cast<ast_block *> (ast));
          break;
        
        case AST_LET:
          {
            ast_let *let = static_cast<ast_let *> (ast);
            if (vars.find (let->get_name ()) == vars.end ())
              ++ count;
            count += _count_locs_needed_rest (let->get_value (), vars);
          }
          break;
        
        case AST_SUM:
          {
            ast_sum *sum = static_cast<ast_sum *> (ast);
            ++ count; // loop variable
            ++ count; // end of range
            count += _count_locs_needed_block (sum->get_body ());
          }
          break;
        
        case AST_N:
          {
            ast_n *n = static_cast<ast_n *> (ast);
            count += _count_locs_needed_block (n->get_body ());
          }
        
        case AST_EXPR_STMT:
          count += _count_locs_needed_rest (
            (static_cast<ast_expr_stmt *> (ast))->get_expr (), vars);
          break;
        
        case AST_BINOP:
          {
            ast_binop *binop = static_cast<ast_binop *> (ast);
            count += _count_locs_needed_rest (binop->get_lhs (), vars);
            count += _count_locs_needed_rest (binop->get_rhs (), vars);
          }
          break;
        
        case AST_UNOP:
          {
            ast_unop *unop = static_cast<ast_unop *> (ast);
            count += _count_locs_needed_rest (unop->get_expr (), vars);
          }
          break;
        
        case AST_IF:
          {
            ast_if *aif = static_cast<ast_if *> (ast);
            count += _count_locs_needed_rest (aif->get_test (), vars);
            count += _count_locs_needed_rest (aif->get_conseq (), vars);
            count += _count_locs_needed_rest (aif->get_alt (), vars);
          }
          break;
        
        default: ;
        }
      
      return count;
    }
    
    int
    _count_locs_needed_block (ast_block *ast)
    {
      int count = 0;
      
      std::unordered_set<std::string> vars;
      for (auto stmt : ast->get_stmts ())
        count += _count_locs_needed_rest (stmt, vars);
      
      return count;
    }
    
    /* 
     * Counts the number local variables required by the specified AST block.
     */
    int
    count_locs_needed (ast_block *blk)
    {
      return _count_locs_needed_block (blk);
    }
  }
}

