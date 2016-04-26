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

#include "compiler/scope.hpp"


namespace rho {
  
  scope_frame::scope_frame ()
  {
  }
  
  scope_frame::scope_frame (const scope_frame& other)
    : vars (other.vars)
  {
    if (other.parent)
      this->parent.reset (new scope_frame (*other.parent));
  }
  
  scope_frame::scope_frame (scope_frame&& other)
    : parent (std::move (other.parent)),
      vars (std::move (other.vars))
  {
  }
  
  
  
  variable
  scope_frame::get_var (const std::string& name)
  {
    auto itr = this->vars.find (name);
    if (itr == this->vars.end ())
      {
        if (this->parent)
          return this->parent->get_var (name);
        return { .type = VAR_UNDEF };
      }
    
    return itr->second;
  }
  
  void
  scope_frame::add_var (const std::string& name, variable var)
  {
    this->vars[name] = var;
  }
  
  
  
  void
  scope_frame::set_parent (const scope_frame& parent)
  {
    this->parent.reset (new scope_frame (parent));
  }
}

