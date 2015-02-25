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

#include "linker/module.hpp"


namespace rho {
  
  module::module ()
    { }
  
  module::~module ()
  {
    for (section *sect : this->sects)
      delete sect;
  }
  
  
  
  /* 
   * Returns the section whose name matches the one specified.
   */
  module::section*
  module::find_section (const std::string& name)
  {
    auto itr = this->sect_map.find (name);
    if (itr == this->sect_map.end ())
      return nullptr;
    
    return this->sects[itr->second];
  }
  
  /* 
   * Creates a new section with the specified name, and returns it.
   */
  module::section*
  module::add_section (const std::string& name)
  {
    if (this->find_section (name))
      return nullptr;
    
    section *sect = new section (name);
    this->sects.push_back (sect);
    this->sect_map[name] = this->sects.size () - 1;
    return sect;
  }
}

