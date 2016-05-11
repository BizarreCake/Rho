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
#include <sstream>


namespace rho {
  
  /* 
   * Returns a string that will uniquely represent a function given information
   * about its parameters.
   */
  std::string
  mangle_function_name (const std::string& name, int argc, int seq_n)
  {
    std::ostringstream ss;
    ss << name << "##" << argc << "@" << seq_n;
    return ss.str ();
  }
}

