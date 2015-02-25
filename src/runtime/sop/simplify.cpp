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

#include "runtime/sop.hpp"
#include "runtime/value.hpp"


namespace rho {
  
  static rho_sop*
  _simplify_basic (rho_sop *sop)
  {
    return sop;
  }
  
  
  
  rho_sop*
  rho_sop_simplify (rho_sop *sop, sop_simplify_level level)
  {
    switch (level)
      {
      case SIMPLIFY_BASIC:
        return _simplify_basic (sop);
        
      case SIMPLIFY_NORMAL:
        return _simplify_basic (sop);
      
      case SIMPLIFY_FULL:
        return _simplify_basic (sop);
      }
    
    return sop;
  }
}

