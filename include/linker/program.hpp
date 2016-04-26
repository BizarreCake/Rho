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

#ifndef _RHO__LINKER__PROGRAM__H_
#define _RHO__LINKER__PROGRAM__H_

#include <cstddef>
#include <string>


namespace rho {
  
  /* 
   * Stores a Rho program (bytecode) that can be executed by a virtual machine.
   */
  class program
  {
    unsigned char *code;
    size_t code_len;
    
  public:
    inline const unsigned char* get_code () const { return this->code; }
    inline size_t get_code_size () const { return this->code_len; }
    
  public:
    program (const unsigned char *code, size_t code_len);
    program (const program& other);
    program (program&& other);
    ~program ();
    
    /* 
     * Loads a program from the file at the specified path.
     * Throws a runtime error if the file could not be read from.
     */
    static program load_from (const std::string& path);
  };
}

#endif

