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

#ifndef _RHO__COMPILER__FRAME__H_
#define _RHO__COMPILER__FRAME__H_

#include "compiler/variable.hpp"
#include <string>
#include <unordered_map>


namespace rho {
  
  struct function_info;
  
  
  enum frame_type
  {
    FT_REPL,
    FT_PROGRAM,
    FT_FUNCTION,
    FT_BLOCK,
  };
  
  
  /* 
   * Frames are used to keep track of lexical scope.
   * They are created whenever something that introduces a new scope is being
   * compiled (e.g. functions, blocks, etc...)
   */
  class frame
  {
    frame_type type;
    frame *parent;
    
    std::unordered_map<std::string, variable *> locs;
    int loc_index;
    
    std::unordered_map<std::string, variable *> args;
    int arg_index;
    
  public:
    inline frame_type get_type () const { return this->type; }
    inline frame* get_parent () { return this->parent; }
    
  public:
    function_info *func;
    
  public:
    frame (frame_type type, frame *parent);
    ~frame ();
    
  private:
    /* 
     * Local variables are created only inside program or function type frames.
     * If the current frame's type is not one of those two, an index is
     * requested from the parent frame.
     */
    int get_next_loc_index ();
    
  public:
    /* 
     * Creates and returns an anonymous local variable.
     */
    int alloc_local ();
    
    
    /* 
     * Returns the local variable whose name matches the one specified.
     */
    variable* get_local (const std::string& name);
    
    /* 
     * Inserts a local variable with the specified name.
     */
    variable* add_local (const std::string& name);
    variable* add_local (const std::string& name, int index);
    
    
    /* 
     * Returns the function argument with the specified name.
     */
    variable* get_arg (const std::string& name);
    
    /* 
     * Inserts a new function argument.
     */
    variable* add_arg (const std::string& name);
  };
}

#endif

