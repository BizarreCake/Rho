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

#ifndef _RHO__LINKER__LINKER__H_
#define _RHO__LINKER__LINKER__H_

#include "linker/module.hpp"
#include "linker/program.hpp"
#include "compiler/code_generator.hpp"
#include <vector>
#include <unordered_map>
#include <stdexcept>


namespace rho {
  
  class link_error: public std::runtime_error
  {
  public:
    link_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };

  
  /* 
   * The linker joins one or more modules/object files into a single program.
   */
  class linker
  {
  private:
    struct module_info
    {
      std::shared_ptr<module> mod;
      int code_off;
      int idx;
    };
  
  private:
    std::vector<std::shared_ptr<module>> mods;
    std::unordered_map<std::string, std::shared_ptr<module>> mod_map;
    std::unordered_map<std::string, std::vector<std::string>> deps;
    std::vector<std::shared_ptr<module>> smods; // sorted in correct link order
    
    std::shared_ptr<module> topm;
    std::unordered_map<std::string, module_info> infos;
    std::unordered_map<std::string, int> known_mods;
    int mod_idx;
    
    code_generator cgen;
    
  private:
    bool is_known_module (const std::string& mident);
    
  public:
    inline const std::unordered_map<std::string, module_info>&
    get_infos () const
      { return this->infos; }
    
    inline int get_next_mod_idx () const { return this->mod_idx; }
    void set_next_mod_idx (int next_mod_idx);
    
  public:
    linker ();
    
  public:
    /* 
     * Inserts the specified module into the list of modules to be linked
     * together.
     */
    void add_module (std::shared_ptr<module> m);
    
    /* 
     * Links all modules inserted so far.
     */
    std::shared_ptr<program> link ();
    
    
    
    // REPL stuff:
    
    void add_known_module (const std::string& mident, int midx);
  
  private:
    void init ();
    
    void link_in (std::shared_ptr<module> m);
  
    void fix_relocations ();
  };
}

#endif

