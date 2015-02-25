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

#ifndef _RHO__COMMON__ERRORS__H_
#define _RHO__COMMON__ERRORS__H_

#include <vector>
#include <string>


namespace rho {
  
  /* 
   * Sorted in ascending order of importance.
   */
  enum error_type
  {
    ET_NOTE,
    ET_WARNING,
    ET_ERROR,
  };
  
  
  /* 
   * The error tracker.
   * 
   * Although it's named an error tracker, other "error" types can be used as
   * well, such as warnings and notes.
   */
  class error_tracker
  {
  public:
    struct entry
    {
      error_type type;
      std::string what;
      std::string file;
      int ln, col;
    };
    
  private:
    std::vector<entry> entries;
    
  public:
    inline const std::vector<entry>& get_entries () { return this->entries; }
    
    inline int count () const { return this->entries.size (); }
    
  public:
    void add (error_type type, const std::string& what);
    void add (error_type type, const std::string& what, int ln, int col,
      const std::string& file);
    
    /* 
     * Count the amount of entries of the specified type.
     */
    int count (error_type type);
  };
}

#endif

