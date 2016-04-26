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

#include "linker/program.hpp"
#include <cstring>
#include <fstream>


namespace rho {
  
  program::program (const unsigned char *code, size_t code_len)
  {
    this->code = new unsigned char [code_len];
    std::memcpy (this->code, code, code_len);
    this->code_len = code_len;
  }
  
  program::program (program&& other)
  {
    this->code = other.code;
    this->code_len = other.code_len;
    
    other.code = nullptr;
  }
  
  program::program (const program& other)
  {
    this->code = new unsigned char [other.code_len];
    std::memcpy (this->code, other.code, other.code_len);
    this->code_len = other.code_len;
  }
  
  program::~program ()
  {
    delete[] this->code;
  }
  
  
  
  /* 
   * Loads a program from the file at the specified path.
   * Throws a runtime error if the file could not be read from.
   */
  program
  program::load_from (const std::string& path)
  {
    std::ifstream fs (path, std::ios_base::in | std::ios_base::binary);
    if (!fs)
      throw std::runtime_error ("could not open file");
    
    fs.seekg (0, std::ios_base::end);
    size_t code_len = fs.tellg ();
    fs.seekg (0, std::ios_base::beg);
    
    unsigned char *code = new unsigned char [code_len];
    fs.read ((char *)code, code_len);
    
    program prg (code, code_len);
    return prg;
  }
}

