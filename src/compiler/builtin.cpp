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

#include "compiler/compiler.hpp"
#include <unordered_map>
#include <sstream>


namespace rho {
  
  bool
  compiler::compile_builtin (std::shared_ptr<ast_fun_call> expr)
  {
    static const std::unordered_map<std::string,
      void (compiler:: *) (std::shared_ptr<ast_fun_call>)> _map {
      
      { "car", &compiler::compile_builtin_car },
      { "cdr", &compiler::compile_builtin_cdr },
      { "cons", &compiler::compile_builtin_cons },
      { "breakpoint", &compiler::compile_builtin_breakpoint },
      { "print", &compiler::compile_builtin_print },
      { "len", &compiler::compile_builtin_len },
    };
    
    auto name = std::static_pointer_cast<ast_ident> (expr->get_fun ())->get_value ();
    auto itr = _map.find (name);
    if (itr == _map.end ())
      return false;
    
    // TODO: handle parameters and free variables
    
    (this ->* itr->second) (expr);
    return true;
  }
  
  
  
  void
  compiler::compile_builtin_car (std::shared_ptr<ast_fun_call> expr)
  {
    if (expr->get_args ().size () != 1)
      {
        this->errs.report (ERR_ERROR,
          "builtin `car' expects exactly 1 argument",
          expr->get_location ());
        return;
      }
    
    for (auto a : expr->get_args ())
      this->compile_expr (a);
    this->cgen.emit_car ();
  }
  
  void
  compiler::compile_builtin_cdr (std::shared_ptr<ast_fun_call> expr)
  {
    if (expr->get_args ().size () != 1)
      {
        this->errs.report (ERR_ERROR,
          "builtin `cdr' expects exactly 1 argument",
          expr->get_location ());
        return;
      }
    
    for (auto a : expr->get_args ())
      this->compile_expr (a);
    this->cgen.emit_cdr ();
  }
  
  void
  compiler::compile_builtin_cons (std::shared_ptr<ast_fun_call> expr)
  {
    if (expr->get_args ().size () != 2)
      {
        this->errs.report (ERR_ERROR,
          "builtin `cons' expects exactly 2 arguments",
          expr->get_location ());
        return;
      }
    
    for (auto a : expr->get_args ())
      this->compile_expr (a);
    this->cgen.emit_cons ();
  }
  
  
  
  void
  compiler::compile_builtin_breakpoint (std::shared_ptr<ast_fun_call> expr)
  {
    std::istringstream ss (std::static_pointer_cast<ast_integer> (expr->get_args ()[0])->get_value ());
    int bp;
    ss >> bp;
    
    this->cgen.emit_breakpoint (bp);
  }
  
  
  
  void
  compiler::compile_builtin_print (std::shared_ptr<ast_fun_call> expr)
  {
    if (expr->get_args ().size () != 1)
      {
        this->errs.report (ERR_ERROR,
          "builtin `print' expects exactly 1 argument",
          expr->get_location ());
        return;
      }
    
    for (auto a : expr->get_args ())
      this->compile_expr (a);
    this->cgen.emit_call_builtin (0, expr->get_args ().size ());
  }
  
  
  
  void
  compiler::compile_builtin_len (std::shared_ptr<ast_fun_call> expr)
  {
    if (expr->get_args ().size () != 1)
      {
        this->errs.report (ERR_ERROR,
          "builtin `len' expects exactly 1 argument",
          expr->get_location ());
        return;
      }
    
    for (auto a : expr->get_args ())
      this->compile_expr (a);
    this->cgen.emit_call_builtin (1, expr->get_args ().size ());
  }
}

