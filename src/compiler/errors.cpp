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

#include "compiler/errors.hpp"


namespace rho {
  
  /* 
   * Inserts an entry into the error list.
   */
  void
  error_list::report (error_type type, const std::string& msg,
                      const ast_node::location& loc)
  {
    this->ents.push_back ({ type, msg, loc.path, loc.ln, loc.col });
    if (type == ERR_FATAL)
      throw compiler_error ("fatal error encountered");
  }
  
  void
  error_list::report (error_type type, const std::string& msg,
                      const std::string& path)
  {
    this->ents.push_back ({ type, msg, path, -1, -1 });
    if (type == ERR_FATAL)
      throw compiler_error ("fatal error encountered");
  }
  
  
  
  /* 
   * Clears all entries.
   */
  void
  error_list::clear ()
  {
    this->ents.clear ();
  }
}

