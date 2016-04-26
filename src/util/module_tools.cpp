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

#include "util/module_tools.hpp"
#include <boost/filesystem.hpp>

#include <iostream> // DEBUG


namespace rho {
  
  static std::vector<std::string>
  _name_parts (const std::string& mname)
  {
    std::vector<std::string> parts;
    
    int start = 0;
    for (;;)
      {
        auto pos = mname.find (':', start);
        if (pos == std::string::npos)
          break;
        else
          {
            parts.push_back (mname.substr (start, pos - start));
            start = pos + 1;
          }
      }
    
    parts.push_back (mname.substr (start));
    return parts;
  }
  
  
  static bool
  _find_in_directory (const std::vector<std::string>& name_parts,
                      const std::string& idir,
                      module_location& out)
  {
    boost::filesystem::path p = idir;
    for (size_t i = 0; i < name_parts.size () - 1; ++i)
      p /= name_parts[i];
    p /= name_parts.back () + ".rho";
    
    if (boost::filesystem::is_regular_file (p))
      {
        p = boost::filesystem::canonical (p);
        out.full_path = p.generic_string ();
        out.dir_path = p.parent_path ().generic_string ();
        return true;
      }
    
    return false;
  }
  
  /* 
   * Attempts to find the absolute path of the module that has the given name.
   * Searches within the specified list of include directories.
   */
  module_location
  find_module (const std::string& mname,
               const std::vector<std::string>& idirs,
               const std::string& wd)
  {
    module_location loc;
    
    auto name_parts = _name_parts (mname);
    
    // try working directory first
    if (_find_in_directory (name_parts, wd, loc))
      return loc;
    
    for (auto& idir : idirs)
      {
        if (_find_in_directory (name_parts, idir, loc))
          return loc;
      }
    
    throw module_not_found_error ("module not found: " + mname);
  }    
  
  
  
  /* 
   * Converts the specified full path into a module identifier string.
   */
  std::string
  get_module_identifier (const std::string& full_path)
    { return full_path; }
}

