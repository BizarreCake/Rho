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


namespace rho {
  
  /*
  
  static rho_value*
  _rho_builtin_range_3 (rho_value *params, virtual_machine& vm)
  {
    auto a1 = params->val.iv.vals[0];
    if (a1->type != RHO_INTEGER)
      throw vm_error ("builtin `range':3 expects integer parameters");
    
    auto a2 = params->val.iv.vals[1];
    if (a2->type != RHO_INTEGER)
      throw vm_error ("builtin `range':3 expects integer parameters");
    
    auto a3 = params->val.iv.vals[2];
    if (a3->type != RHO_INTEGER)
      throw vm_error ("builtin `range':3 expects integer parameters");
    
    rho_value *curr = rho_value_make_empty_list (vm.get_gc ());
    
    mpz_t i;
    mpz_init_set (i, a2->val.i);
    
    mpz_t rem;
    mpz_init (rem);
    mpz_tdiv_r (rem, i, a3->val.i);
    mpz_sub (i, i, rem);
    if (mpz_sgn (rem) != 0)
      mpz_add (i, i, a3->val.i);
    
    for (;;)
      {
        mpz_sub (i, i, a3->val.i);
        if (mpz_cmp (i, a1->val.i) < 0)
          break;


        auto n = vm.get_gc ().alloc_protected ();
        n->type = RHO_INTEGER;
        mpz_init_set (n->val.i, i);
        
        curr = rho_value_make_cons (n, curr, vm.get_gc ());
      } 
    
    mpz_clear (i);
    return curr;
  }
  
  static rho_value*
  _rho_builtin_range_2 (rho_value *params, virtual_machine& vm)
  {
    auto a1 = params->val.iv.vals[0];
    if (a1->type != RHO_INTEGER)
      throw vm_error ("builtin `range':2 expects integer parameters");
    
    auto a2 = params->val.iv.vals[1];
    if (a2->type != RHO_INTEGER)
      throw vm_error ("builtin `range':2 expects integer parameters");
    
    rho_value *curr = rho_value_make_empty_list (vm.get_gc ());
    
    mpz_t i;
    mpz_init_set (i, a2->val.i);
    
    for (;;)
      {
        mpz_sub_ui (i, i, 1);
        if (mpz_cmp (i, a1->val.i) < 0)
          break;


        auto n = vm.get_gc ().alloc_protected ();
        n->type = RHO_INTEGER;
        mpz_init_set (n->val.i, i);
        
        curr = rho_value_make_cons (n, curr, vm.get_gc ());
      } 
    
    mpz_clear (i);
    return curr;
  }
  
  static rho_value*
  _rho_builtin_range_1 (rho_value *params, virtual_machine& vm)
  {
    auto a1 = params->val.iv.vals[0];
    if (a1->type != RHO_INTEGER)
      throw vm_error ("builtin `range':1 expects integer parameter");
    
    rho_value *curr = rho_value_make_empty_list (vm.get_gc ());
    
    long max = mpz_get_si (a1->val.i);
    while (--max >= 0)
      {
        curr = rho_value_make_cons (rho_value_make_int (max, vm.get_gc ()),
          curr, vm.get_gc ());
      } 
    
    return curr;
  }
  
  rho_value*
  rho_builtin_range (rho_value *params, virtual_machine& vm)
  {
    switch (params->val.iv.len)
      {
      case 1: return _rho_builtin_range_1 (params, vm);
      case 2: return _rho_builtin_range_2 (params, vm);
      case 3: return _rho_builtin_range_3 (params, vm);
     
      default:
        throw vm_error ("invalid number of arguments passed to builtin `range'");
      }
  }
  
  
  
//------------------------------------------------------------------------------
  
  */
}

