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

#include "compiler/module_store.hpp"


namespace rho {
  
  /* 
   * Inserts an AST tree of a module into the module store.
   */
  void
  module_store::store (const std::string& ident, std::shared_ptr<ast_program> ast)
  {
    auto& ent = this->entries[ident];
    ent.ident = ident;
    ent.ast = ast;
  }
  
  /* 
   * Checks whether the store contains an entry for the given module identifier.
   */
  bool
  module_store::contains (const std::string& ident)
  {
    return (this->entries.find (ident) != this->entries.end ());
  }
  
  /* 
   * Returns the entry associated with the specified module identifier.
   */
  module_store::entry&
  module_store::retrieve (const std::string& ident)
  {
    auto itr = this->entries.find (ident);
    return itr->second;
  }
  
  /* 
   * Removes the entry associated with the specified identifier.
   */
  void
  module_store::remove (const std::string& ident)
  {
    auto itr = this->entries.find (ident);
    if (itr != this->entries.end ())
      this->entries.erase (itr);
  }
}

