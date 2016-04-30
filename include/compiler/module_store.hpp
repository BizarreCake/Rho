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

#ifndef _RHO__COMPILER__MODULE_STORE__H_
#define _RHO__COMPILER__MODULE_STORE__H_

#include "parse/ast.hpp"
#include <unordered_map>
#include <memory>
#include <string>


namespace rho {
  
  class var_analysis;
  
  /* 
   * Stores known information about modules that are being compiled (e.g. AST).
   */
  class module_store
  {
  public:
    struct entry
    {
      std::string ident;
      std::string mname;
      
      std::shared_ptr<ast_program> ast;
      std::shared_ptr<var_analysis> van;
      
      std::string full_path;
      std::string dir_path;
    };

  private:
    std::unordered_map<std::string, entry> entries;
    
  public:
    inline std::unordered_map<std::string, entry>& get_entries () { return this->entries; }
    
  public:
    /* 
     * Inserts an AST tree of a module into the module store.
     */
    void store (const std::string& ident, std::shared_ptr<ast_program> ast);
    
    /* 
     * Checks whether the store contains an entry for the given module identifier.
     */
    bool contains (const std::string& ident);
    
    /* 
     * Returns the entry associated with the specified module identifier.
     */
    entry& retrieve (const std::string& ident);
    
    /* 
     * Removes the entry associated with the specified identifier.
     */
    void remove (const std::string& ident);
  }; 
}

#endif

