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

#ifndef _RHO__COMPILER__FUNC__H_
#define _RHO__COMPILER__FUNC__H_


namespace rho {
  
  /* 
   * Holds information about a function during compilation.
   */
  struct function_info
  {
    // a label pointing to the where the function's code should be copied to.
    int lbl;
    
    // the function's name, if there is one.
    std::string name;
  };
}

#endif

