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

#ifndef _RHO__REPL__H_
#define _RHO__REPL__H_

#include <string>
#include <unordered_map>
#include <vector>


namespace rho {
  
  class virtual_machine;
  class ast_program;
  class compiler;
  
  /* 
   * Read, eval, print, loop.
   * 
   * This class implements the interactive interface between the user and the
   * Rho interpreter.
   */
  class rho_repl
  {
    virtual_machine *vm;
    std::unordered_map<std::string, int> vars;
    int next_var_index;
    
    std::vector<std::string> buf;
    int indent;
    
  public:
    rho_repl ();
    ~rho_repl ();
    
  private:
    /* 
     * Applies numerous important changes to the specified AST tree that are
     * necessary to make the REPL work.  This includes turning the last
     * statement/expression into a let declaration in order to get its value.
     * 
     * Returns true if a value should be printed.
     */
    bool inspect_tree (ast_program *ast);
    
    /* 
     * Inserts bindings of REPL variables into the compiler.
     */
    void add_bindings (compiler& comp);
    
    /* 
     * Evaluates the specified string.
     */
    void eval (const std::string& str);
    
  public:
    /* 
     * Runs the REPL.
     */
    void run ();
  };
}

#endif

