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

#ifndef _RHO__LINKER__MODULE__H_
#define _RHO__LINKER__MODULE__H_

#include "common/byte_vector.hpp"
#include <string>
#include <vector>
#include <unordered_map>


namespace rho {
  
  /* 
   * Represents a single compilation unit (analogous to an .obj object file).
   * 
   * Very similar to actual .obj files, modules consist of a number of sections,
   * with the two most important ones being the "code" and "data" sections.
   * The code section contains the module's executable bytecode, and the data
   * section will typically contain strings which are used by code in the code
   * section.
   */
  class module
  {
  private:
    struct section
    {
      std::string name;
      byte_vector data;
      
    public:
      section (const std::string& name)
        : name (name)
        { }
    };
    
  private:
    std::vector<section *> sects;
    std::unordered_map<std::string, int> sect_map;
    
  public:
    inline std::vector<section *>& get_sections () { return this->sects; }
    
  public:
    module ();
    ~module ();
    
  public:
    /* 
     * Returns the section whose name matches the one specified.
     */
    section* find_section (const std::string& name);
    
    /* 
     * Creates a new section with the specified name, and returns it.
     */
    section* add_section (const std::string& name);
  };
  
  
  
  /* 
   * Executables are created in the linking stage, by combining one or more
   * regular modules together.
   */
  class executable: public module
    { };
}

#endif

