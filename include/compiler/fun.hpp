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

#ifndef _RHO__COMPILER__FUN__H_
#define _RHO__COMPILER__FUN__H_

#include "parse/ast.hpp"
#include <string>
#include <vector>


namespace rho {
  
  struct fun_prototype
  {
    std::string name;
    std::string mname;  // mangled name
    std::vector<std::string> params;
    std::shared_ptr<ast_expr> guard;
    int seq_n;
  };
  
  
  
  /* 
   * Returns a string that will uniquely represent a function given information
   * about its parameters.
   */
  std::string
  mangle_function_name (const std::string& name, int argc, int seq_n);
}

#endif

