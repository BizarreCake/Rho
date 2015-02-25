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
#include "common/byte_vector.hpp"
#include "compiler/asttools.hpp"

#include <iostream> // DEBUG


namespace rho {
  
  void
  compiler::compile_function (ast_function *ast)
  {
    this->funcs.emplace_back ();
    function_info& func = this->funcs.back ();
    
    func.name = ast->get_name ();
    func.lbl = this->cgen->create_label ();
    
    int func_end = this->cgen->create_label ();
    this->cgen->emit_jmp (func_end);
    this->cgen->mark_label (func.lbl);
    
    // function code
    unsigned code_start = this->cgen->get_pos ();
    {
      int locs = ast::count_locs_needed (ast->get_body ());
      this->cgen->emit_push_frame (locs);
      this->push_frame (FT_FUNCTION);
      frame& frm = this->top_frame ();
      frm.func = &func;
      
      // set up parameters
      auto& params = ast->get_params ();
      for (auto param : params)
        {
          frm.add_arg (param.name);
        }
      
      // compile body
      auto& stmts = ast->get_body ()->get_stmts ();
      for (unsigned int i = 0; i < stmts.size (); ++i)
        {
          ast_stmt *stmt = stmts[i];
          if (i == stmts.size () - 1)
            {
              // last statement
              if (stmt->get_type () == AST_EXPR_STMT)
                {
                  ast_expr *expr = (static_cast<ast_expr_stmt *> (stmt))->get_expr ();
                  this->compile_expr (expr);
                  this->cgen->emit_return ();
                  break;
                }
              else
                {
                  ast_expr *expr = dynamic_cast<ast_expr *> (stmt);
                  if (expr)
                    {
                      this->compile_expr (expr);
                      this->cgen->emit_return ();
                      break;
                    }
                }
            }
          
          this->compile_stmt (stmt);
        }
      
      this->cgen->emit_push_nil ();
      this->cgen->emit_return ();
    }
    unsigned int code_size = this->cgen->get_pos () - code_start;
    
    this->cgen->mark_label (func_end);
    
    this->cgen->emit_create_func (code_size, func.lbl);
  }
  
  
  
  void
  compiler::compile_call (ast_call *ast)
  {
    // push arguments in reverse order
    auto& args = ast->get_args ();
    for (int i = args.size () - 1; i >= 0; --i)
      this->compile_expr (args[i]);
    
    this->compile_expr (ast->get_func ());
    this->cgen->emit_call (args.size ());
  }
}

