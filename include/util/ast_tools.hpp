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

#ifndef _RHO__UTIL__AST_TOOLS__H_
#define _RHO__UTIL__AST_TOOLS__H_

#include "parse/ast.hpp"
#include <functional>
#include <string>


namespace rho {
  
  enum traverse_result
  {
    TR_CONTINUE,  // continue scanning child nodes
    TR_SKIP,      // skip child nodes
  };
  
  namespace ast_tools {
    
    using traverse_fn = std::function<traverse_result (std::shared_ptr<ast_node>)>;
    
    /* 
     * Performs a depth-first traversal on the specified AST node.
     */
    void traverse_dfs (std::shared_ptr<ast_node> node, traverse_fn&& fn);
    
    
    
    /*
     * Extracts the name of the module from the specified AST program.
     */
    std::string extract_module_name (std::shared_ptr<ast_program> node);
    
    /* 
     * Extracts the names of all imported modules in the specified AST program.
     */
    std::vector<std::string> extract_imports (std::shared_ptr<ast_program> node);
    
    /* 
     * Extracts the names of all exports in the specified AST program, in the
     * order they appear in the program.
     */
    std::vector<std::string> extract_exports (std::shared_ptr<ast_program> node);
    
    
    
    /* 
     * Extracts top-level variable definitions from the specified AST program.
     */
    std::vector<std::string> extract_global_defs (std::shared_ptr<ast_program> node);
  };
}

#endif

