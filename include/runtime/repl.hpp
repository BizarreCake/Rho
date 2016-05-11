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

#ifndef _RHO__RUNTIME__REPL__H_
#define _RHO__RUNTIME__REPL__H_

#include "runtime/vm.hpp"
#include "compiler/module_store.hpp"
#include "compiler/compiler.hpp"
#include "util/module_tools.hpp"
#include <string>
#include <unordered_map>
#include <stack>


namespace rho {
  
  // forward decs:
  class program;
  class linker;
  
  
  /* 
   * Implements a REPL (Read-Eval-Print Loop) for Rho.
   */
  class rho_repl
  {
    struct import_entry
    {
      std::string name;
      std::string ident;
    };
    
  private:
    virtual_machine vm;
    module_store mstore;
    compiler comp;
    
    std::string buf;
    std::vector<std::string> idirs;
    
    int run_num;
    std::unordered_map<std::string, int> globs;
    int next_glob;
    std::vector<std::shared_ptr<program>> progs;
    std::unordered_set<std::shared_ptr<fun_prototype>> protos;
    
    std::unordered_map<std::string, import_entry> imps;
    std::unordered_map<std::string, int> mods;
    int next_mod;
    
    std::unordered_map<std::string, int> atoms;
    std::vector<std::pair<std::string, std::string>> u_aliases;
    std::vector<std::string> u_ns;
    
  private:
    void print_intro ();
    
    
    /* 
     * Compiles the code stored in the buffer and executes it.
     */
    void compile_line ();
    
    bool parse_module (std::istream& strm,
                       const std::string& full_path, const std::string& dir_path,
                       std::stack<module_location>& parse_work);
    
    
    void handle_globals ();
    void handle_imports_pre ();
    void handle_imports (linker& lnk);
    void handle_atoms (linker& lnk);
    void handle_usings ();
    void handle_print ();
    
  public:
    rho_repl ();
    ~rho_repl ();
    
  public:
    /* 
     * Prints an intro and runs the REPL.
     */
    void run ();
  };
}

#endif

