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

#include "runtime/builtins.hpp"
#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"
#include "runtime/value.hpp"
#include <iostream>


namespace rho {
  
  rho_value
  rho_builtin_print (rho_value& p, virtual_machine& vm)
  {
    if (p.type == RHO_STR)
      std::cout << p.val.gc->val.s.str << std::endl;
    else
      std::cout << rho_value_str (p, vm) << std::endl;
    
    return rho_value_make_nil ();
  }
  
  
  
//------------------------------------------------------------------------------
  
  static long
  _list_len (rho_value& lst)
  {
    long len = 0;
    
    rho_value cur = lst;
    while (cur.type == RHO_CONS)
      {
        ++ len;
        cur = cur.val.gc->val.p.snd;
      }
    
    if (cur.type == RHO_EMPTY_LIST)
      return len;
    return 0;
  }
  
  rho_value
  rho_builtin_len (rho_value& p, virtual_machine& vm)
  {
    switch (p.type)
      {
      case RHO_VEC:
        {
          long len = p.val.gc->val.vec.len;
          if (len <= VM_SMALL_INT_MAX)
            return vm.get_prealloced_int (len);
          
          return rho_value_make_int (len, vm.get_gc ());
        }
      
      case RHO_CONS:
        {
          long len = _list_len (p);
          if (len <= VM_SMALL_INT_MAX)
            return vm.get_prealloced_int (len);
          
          return rho_value_make_int (len, vm.get_gc ());
        }
      
      default:
        return vm.get_prealloced_int (0);
      }
  }
}

