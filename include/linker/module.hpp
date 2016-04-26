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

#ifndef _RHO__LINKER__MODULE__H_
#define _RHO__LINKER__MODULE__H_

#include <memory>
#include <vector>


namespace rho {
  
  enum reloc_type
  {
    REL_GP,
    REL_GV,
  };
  
  struct reloc_t
  {
    reloc_type type;
    int pos;
    std::string mname;
  };
  
  
  
  /* 
   * A single compilation unit, analogous to an object file.
   */
  class module
  {
    std::string name;
    std::vector<std::string> imps;
  
    std::unique_ptr<unsigned char []> code;
    int code_len;
    
    std::vector<reloc_t> relocs;
    
  public:
    void set_code (const unsigned char *code, int len);
    inline const unsigned char* get_code () const { return this->code.get (); }
    inline int get_code_size () const { return this->code_len; }
    
    inline std::vector<reloc_t>& get_relocs () { return this->relocs; }
    inline const std::vector<reloc_t>& get_relocs () const { return this->relocs; }
    
    void
    add_reloc (reloc_type type, int pos, const std::string& mname)
      { this->relocs.push_back ({ .type = type, .pos = pos, .mname = mname }); }
    
    inline const std::string& get_name () const { return this->name; }
    inline void set_name (const std::string& name) { this->name = name; }
    
    inline const std::vector<std::string>& get_imports () const { return this->imps; }
    inline void add_import (const std::string& name) { this->imps.push_back (name); }
  };
}

#endif

