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

#ifndef _RHO__PARSER__PARSER__H_
#define _RHO__PARSER__PARSER__H_

#include "parser/ast.hpp"
#include <istream>


namespace rho {
  
  // forward decs:
  class error_tracker;
  
  
  /* 
   * The parser.
   * Given Rho source code, the parser generates an AST tree representing the
   * program.
   */
  class parser
  {
    error_tracker& errs;
    
  public:
    parser (error_tracker& errs);
    
  public:
    /* 
     * Parses the Rho code in the specified character stream and returns an
     * AST tree.
     * The file name is used for error-tracking purposes.
     */
    ast_program* parse (std::istream& strm, const std::string& file_name);
  };
}

#endif

