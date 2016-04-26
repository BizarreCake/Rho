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

#ifndef _RHO__UTIL__MODULE_TOOLS__H_
#define _RHO__UTIL__MODULE_TOOLS__H_

#include <string>
#include <vector>
#include <stdexcept>


namespace rho {
  
  class module_not_found_error: public std::runtime_error
  {
  public:
    module_not_found_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  
  struct module_location
  {
    std::string full_path;
    std::string dir_path;
  };
  
  /* 
   * Attempts to find the absolute path of the module that has the given name.
   * Searches within the specified list of include directories.
   */
  module_location find_module (const std::string& mname,
                               const std::vector<std::string>& idirs,
                               const std::string& wd);
  
  
  
  /* 
   * Converts the specified full path into a module identifier string.
   */
  std::string get_module_identifier (const std::string& full_path);
}

#endif

