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

#ifndef _RHO__COMPILER__SCOPE__H_
#define _RHO__COMPILER__SCOPE__H_

#include "compiler/variable.hpp"
#include <memory>
#include <unordered_map>
#include <string>


namespace rho {
  
  /* 
   * Stores a scope of variables.
   * Similar to a variable frame, but is built incrementally by the compiler.
   */
  class scope_frame
  {
    std::unique_ptr<scope_frame> parent;
    std::unordered_map<std::string, variable> vars;
    
  public:
    inline size_t size () const { return this->vars.size (); }
    
  public:
    scope_frame ();
    scope_frame (const scope_frame& other);
    scope_frame (scope_frame&& other);
    
  public:
    variable get_var (const std::string& name);
    void add_var (const std::string& name, variable var);
    
    void set_parent (const scope_frame& parent);
  };
}

#endif

