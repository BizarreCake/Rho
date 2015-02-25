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

#include "compiler/frame.hpp"


namespace rho {
  
  frame::frame (frame_type type, frame *parent)
  {
    this->type = type;
    this->parent = parent;
    this->loc_index = 0;
    this->arg_index = 0;
    this->func = nullptr;
  }
  
  frame::~frame ()
  {
    for (auto p : this->locs)
      delete p.second;
    for (auto p : this->args)
      delete p.second;
  }
  
  
  
  /* 
   * Local variables are created only inside program or function type frames.
   * If the current frame's type is not one of those two, an index is
   * requested from the parent frame.
   */
  int
  frame::get_next_loc_index ()
  {
    if (!(this->type == FT_PROGRAM || this->type == FT_FUNCTION))
      return this->parent->get_next_loc_index ();
    return this->loc_index++;
  }
  
  
  /* 
   * Creates and returns an anonymous local variable.
   */
  int
  frame::alloc_local ()
  {
    return this->get_next_loc_index ();
  }
  
  
  
  /* 
   * Returns the local variable whose name matches the one specified.
   */
  variable*
  frame::get_local (const std::string& name)
  {
    auto itr = this->locs.find (name);
    if (itr == this->locs.end ())
      {
        if (this->parent &&
          !(this->type == FT_PROGRAM || this->type == FT_FUNCTION))
          return this->parent->get_local (name);
        return nullptr;
      }
    
    return itr->second;
  }
  
  /* 
   * Inserts a local variable with the specified name.
   */
  variable*
  frame::add_local (const std::string& name)
  {
    variable *var = new variable {
      .index = this->get_next_loc_index (),
      .name  = name,
    };
    this->locs[name] = var;
    return var;
  }
  
  variable*
  frame::add_local (const std::string& name, int index)
  {
    variable *var = new variable {
      .index = index,
      .name  = name,
    };
    this->locs[name] = var;
    return var;
  }
  
  
  
  /* 
   * Returns the function argument with the specified name.
   */
  variable*
  frame::get_arg (const std::string& name)
  {
    auto itr = this->args.find (name);
    if (itr == this->args.end ())
      {
        if (this->parent && this->get_type () != FT_FUNCTION)
          return this->parent->get_arg (name);
        return nullptr;
      }
    
    return itr->second;
  }
  
  /* 
   * Inserts a new function argument.
   */
  variable*
  frame::add_arg (const std::string& name)
  {
    variable *var = new variable {
      .index = this->arg_index++,
      .name  = name,
    };
    this->args[name] = var;
    return var;
  }
}

