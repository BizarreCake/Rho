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

#include "compiler/fun.hpp"


namespace rho {
  
  /* 
   * Registers a parameter name with the specified index.
   */
  void
  func_frame::add_param (const std::string& param, int index)
  {
    this->params[param] = index;
  }
  
  /* 
   * Returns the index of the specified parameter, or -1 if it is not found.
   */
  int
  func_frame::get_param (const std::string& param)
  {
    auto itr = this->params.find (param);
    if (itr == this->params.end ())
      return -1;
    return itr->second;
  }
}

