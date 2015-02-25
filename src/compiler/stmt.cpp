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

#include "compiler/compiler.hpp"
#include "compiler/codegen.hpp"
#include <stdexcept>

#include <iostream> // DEBUG


namespace rho {
  
  void
  compiler::compile_expr_stmt (ast_expr_stmt *ast)
  {
    this->compile_expr (ast->get_expr ());
    this->cgen->emit_pop ();
  }
  
  
  
  void
  compiler::compile_block (ast_block *ast, bool as_expr, bool alloc_frame)
  {
    if (alloc_frame)
      this->push_frame (FT_BLOCK);
    
    auto& stmts = ast->get_stmts ();
    for (unsigned int i = 0; i < stmts.size (); ++i)
      {
        ast_stmt *stmt = stmts[i];
        
        if (as_expr && i == stmts.size () - 1)
          {
            // last statement
            
            if (stmt->get_type () == AST_EXPR_STMT)
              {
                this->compile_expr (
                  (static_cast<ast_expr_stmt *> (stmt))->get_expr ());
              }
            else
              {
                ast_expr *expr = dynamic_cast<ast_expr *> (stmt);
                if (expr)
                  this->compile_expr (expr);
                else
                  this->cgen->emit_push_nil ();
              }
            
            break;
          }
        
        this->compile_stmt (stmt);
      }
    
    if (as_expr && stmts.empty ())
      this->cgen->emit_push_nil ();
    
    if (alloc_frame)
      this->pop_frame ();
  }
  
  
  
  void
  compiler::compile_let (ast_let *ast)
  {
    if (ast->is_repl ())
      {
        frame *frm = this->frms.front ();
        variable *var = frm->get_local (ast->get_name ());
        if (!var)
          throw std::runtime_error ("invalid repl let");
        
        this->compile_expr (ast->get_value ());
        this->cgen->emit_store_repl_var (var->index);
      }
    else
      {
        frame& frm = this->top_frame ();
        variable *var = frm.add_local (ast->get_name ());
        
        this->compile_expr (ast->get_value ());
        this->cgen->emit_store_loc (var->index);
      }
  }
  
  
  
  void
  compiler::compile_stmt (ast_stmt *ast)
  {
    switch (ast->get_type ())
      {
      case AST_EXPR_STMT:
        this->compile_expr_stmt (static_cast<ast_expr_stmt *> (ast));
        break;
      
      case AST_LET:
        this->compile_let (static_cast<ast_let *> (ast));
        break;
      
      default:
        throw std::runtime_error ("invalid statement type");
      }
  }
}

