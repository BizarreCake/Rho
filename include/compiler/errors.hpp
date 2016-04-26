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

#ifndef _RHO__COMPILER__ERRORS__H_
#define _RHO__COMPILER__ERRORS__H_

#include "parse/ast.hpp"
#include <vector>
#include <stdexcept>


namespace rho {
  
  enum error_type
  {
    ERR_INFO,
    ERR_WARNING,
    ERR_ERROR,
    ERR_FATAL,
  };
  
  
  /* 
   * Thrown by the compiler in case a fatal error is reported to immediately
   * halt compilation.
   */
  class compiler_error: public std::runtime_error
  {
  public:
    compiler_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  /* 
   * Structure used to keep track of errors during compilation.
   */
  class error_list
  {
  public:
    struct entry
    {
      error_type type;
      std::string msg;
      std::string path;
      int ln, col;
    };
    
  private:
    std::vector<entry> ents;
    
  public:
    inline const std::vector<entry>& get_entries () const { return this->ents; }
    inline int count () const { return this->ents.size (); }
    
  public:
    /* 
     * Inserts an entry into the error list.
     */
    void report (error_type type, const std::string& msg,
                 const ast_node::location& loc);
    void report (error_type type, const std::string& msg, const std::string& path);
    
    /* 
     * Clears all entries.
     */
    void clear ();
  };
}

#endif

