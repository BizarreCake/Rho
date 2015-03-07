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
  
  
  
  static rho_sop*
  _simplify_normal (rho_sop *sop)
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
        return _simplify_normal (sop);
      
      case SIMPLIFY_FULL:
        return _simplify_normal (sop);
      }
    
    return sop;
  }
  
  
  
//------------------------------------------------------------------------------
  
  /* 
   * Flattens out redundant expressions (e.g. a sum containing just one element
   * will be flattened into just that element).
   */
  rho_sop*
  rho_sop_flatten (rho_sop *sop, virtual_machine& vm)
  {
    switch (sop->type)
      {
      case SOP_ADD:
        if (rho_sop_arr_size (sop) == 0)
          {
            rho_sop_unprotect (sop);
            return rho_sop_new (rho_value_new_int (0, vm), vm);
          }
        else if (rho_sop_arr_size (sop) == 1)
          {
            rho_sop *elem = rho_sop_arr_get (sop, 0);
            rho_sop_protect (elem, vm);
            rho_sop_unprotect (sop);
            return elem;
          }
        break;
      
      case SOP_MUL:
        if (rho_sop_arr_size (sop) == 0)
          {
            rho_sop_unprotect (sop);
            return rho_sop_new (rho_value_new_int (1, vm), vm);
          }
        else if (rho_sop_arr_size (sop) == 1)
          {
            rho_sop *elem = rho_sop_arr_get (sop, 0);
            rho_sop_protect (elem, vm);
            rho_sop_unprotect (sop);
            return elem;
          }
        break;
      
      default: ;
      }
    
    return sop;
  }
  
  /* 
   * Repeatedly flattens out a SOP expression until not possible anymore.
   */
  rho_sop*
  rho_sop_deep_flatten (rho_sop *sop, virtual_machine& vm)
  {
    rho_sop *last = nullptr;
    for (;;)
      {
        sop = rho_sop_flatten (sop, vm);
        if (sop == last)
          break;
        last = sop;
      }
    
    return sop;
  }
}

