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
#include <unordered_map>

#include <iostream> // DEBUG


namespace rho {
  
  void
  compiler::compile_function (ast_function *ast)
  {
    this->funcs.push_back (new function_info ());
    function_info& func = *this->funcs.back ();
    
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
                  this->push_expr_frame (true);
                  this->compile_expr (expr);
                  this->pop_expr_frame ();
                  this->cgen->emit_return ();
                  break;
                }
              else
                {
                  ast_expr *expr = dynamic_cast<ast_expr *> (stmt);
                  if (expr)
                    {
                      this->push_expr_frame (true);
                      this->compile_expr (expr);
                      this->pop_expr_frame ();
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
  
  
  
  namespace {
    
    enum builtin_type {
      BI_UNKNOWN,
      
      BI_CONS,
      BI_CAR,
      BI_CDR,
      BI_EXPAND,
      BI_EXPAND_ALL,
    };
  }
  
  static builtin_type
  _get_builtin_type (const std::string& name)
  {
    static const std::unordered_map<std::string, builtin_type> _map {
      { "cons", BI_CONS },
      { "car", BI_CAR },
      { "cdr", BI_CDR },
      { "expand", BI_EXPAND },
      { "expand_all", BI_EXPAND_ALL },
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return BI_UNKNOWN;
    return itr->second;
  }
  
  void
  compiler::compile_call (ast_call *ast)
  {
    // builtins
    if (ast->get_func ()->get_type () == AST_IDENT)
      {
        ast_ident *id = static_cast<ast_ident *> (ast->get_func ());
        switch (_get_builtin_type (id->get_name ()))
          {
          case BI_CONS: this->compile_builtin_cons (ast); return;
          case BI_CAR: this->compile_builtin_car (ast); return;
          case BI_CDR: this->compile_builtin_cdr (ast); return;
          case BI_EXPAND: this->compile_builtin_expand (ast); return;
          case BI_EXPAND_ALL: this->compile_builtin_expand_all (ast); return;
          
          case BI_UNKNOWN: ;
          }
      }
    
    frame& frm = this->top_frame ();
    
    bool tc = this->can_perform_tail_call ();
    tc = tc && (ast->get_func ()->get_type () == AST_IDENT
      && (static_cast<ast_ident *> (ast->get_func ()))->get_name () == "$$");
    
    // push arguments in reverse order
    this->push_expr_frame (false);
    auto& args = ast->get_args ();
    for (int i = args.size () - 1; i >= 0; --i)
      this->compile_expr (args[i]);
    
    this->compile_expr (ast->get_func ());
    this->pop_expr_frame ();
    
    if (tc)
      {
        std::cout << "tail call!" << std::endl;
        this->cgen->emit_tail_call ();
      }
    else
      this->cgen->emit_call (args.size ());
  }
}

