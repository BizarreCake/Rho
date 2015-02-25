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

#include "common/errors.hpp"


namespace rho {
  
  void
  error_tracker::add (error_type type, const std::string& what)
  {
    this->add (type, what, -1, -1, "");
  }
  
  void
  error_tracker::add (error_type type, const std::string& what, int ln, int col,
    const std::string& file)
  {
    this->entries.push_back ({
      .type = type,
      .what = what,
      .file = file,
      .ln   = ln,
      .col  = col,
    });
  }
  
  
  
  /* 
   * Count the amount of entries of the specified type.
   */
  int
  error_tracker::count (error_type type)
  {
    int c = 0;
    for (auto& et : this->entries)
      if (et.type == type)
        ++ c;
    return c;
  }
}

