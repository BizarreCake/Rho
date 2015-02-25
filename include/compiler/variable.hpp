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

#ifndef _RHO__COMPILER__VARIABLE__H_
#define _RHO__COMPILER__VARIABLE__H_

#include <string>


namespace rho {
  
  /* 
   * Stores information about a variable.
   */
  struct variable
  {
    // the index value of the variable within its frame.
    int index;
    
    // the name of the variable.
    std::string name;
  };
}

#endif

