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

#ifndef _RHO__COMPILER__FUN__H_
#define _RHO__COMPILER__FUN__H_

#include <unordered_map>
#include <string>
#include <vector>


namespace rho {
  
  /* 
   * 
   */
  class func_frame
  {
    std::unordered_map<std::string, int> params;
    std::vector<int> block_offs;
    
  public:
    inline std::vector<int>& get_block_offs () { return this->block_offs; }
    inline std::unordered_map<std::string, int>& get_params () { return this->params; }
    
  public:
    /* 
     * Registers a parameter name with the specified index.
     */
    void add_param (const std::string& param, int index);
    
    /* 
     * Returns the index of the specified parameter, or -1 if it is not found.
     */
    int get_param (const std::string& param);
  };
}

#endif

