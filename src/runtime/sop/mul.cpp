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
#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"


namespace rho {
  
#define COMB_NOT_POSSIBLE   (rho_sop *)0
#define COMB_CANCELLED      (rho_sop *)1
  
  // forward decs:
  static rho_sop* _combine_power_with_symbol (rho_sop *a, rho_sop *b,
    virtual_machine& vm);
  
  
  
  static rho_sop*
  _combine_value_with_value (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    if (rho_value_is_number (a->val.val) && rho_value_is_number (b->val.val))
      {
        rho_value *nval = rho_value_mul (a->val.val, b->val.val, vm);
        if (rho_value_is_number_eq (nval, 1))
          {
            rho_value_unprotect (nval);
            rho_sop_unprotect (a);
            rho_sop_unprotect (b);
            return COMB_CANCELLED;
          }
        
        rho_sop *nsop = rho_sop_new (nval, vm);
        rho_sop_unprotect (a);
        rho_sop_unprotect (b);
        return nsop;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  static rho_sop*
  _combine_value_with (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    switch (b->type)
      {
      case SOP_VAL:
        return _combine_value_with_value (a, b, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  static rho_sop*
  _combine_symbol_with_symbol (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    if (rho_sop_eq (a, b))
      {
        rho_sop *exp = rho_sop_new (rho_value_new_int (2, vm), vm);
        rho_sop *nsop = rho_sop_pow (a, exp, vm);
        rho_sop_unprotect (b);
        return nsop;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  static rho_sop*
  _combine_symbol_with (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    switch (b->type)
      {
      case SOP_SYM:
        return _combine_symbol_with_symbol (a, b, vm);
      
      case SOP_POW:
        return _combine_power_with_symbol (b, a, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  rho_sop*
  _combine_power_with_symbol (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    if (rho_sop_eq (a->val.bop.lhs, b))
      {
        // add one to exponent

        rho_sop *one = rho_sop_new (rho_value_new_int (1, vm), vm);
        rho_sop *nexp = rho_sop_add (a->val.bop.rhs, one, vm);
        rho_sop_unprotect (one);
        a->val.bop.rhs = nexp;
        rho_sop_unprotect (nexp);
        rho_sop_unprotect (b);
        return a;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  rho_sop*
  _combine_power_with_power (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    if (rho_sop_eq (a->val.bop.lhs, b->val.bop.lhs))
      {
        // add exponents together

        rho_sop *nexp = rho_sop_add (a->val.bop.rhs, b->val.bop.rhs, vm);
        a->val.bop.rhs = nexp;
        rho_sop_unprotect (nexp);
        rho_sop_unprotect (b);
        return a;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  static rho_sop*
  _combine_power_with (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    switch (b->type)
      {
      case SOP_SYM:
        return _combine_power_with_symbol (a, b, vm);
      
      case SOP_POW:
        return _combine_power_with_power (a, b, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  /* 
   * Attempts to combine two SOP values into one SOP in some way
   * (e.g. 'x*'x will be turned into 'x^2).
   */
  static rho_sop*
  _combine (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case SOP_VAL:
        return _combine_value_with (a, b, vm);
      
      case SOP_SYM:
        return _combine_symbol_with (a, b, vm);
      
      case SOP_POW:
        return _combine_power_with (a, b, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  /* 
   * Attempts to combine the specified SOP with some existing element in the
   * product. If not possible, inserts the SOP to the end of the product.
   */
  static void
  _insert_or_combine (rho_sop *dest, rho_sop *sop, virtual_machine& vm)
  {
    bool combined = false;
    
    int s = rho_sop_arr_size (dest);
    do
      {
        combined = false;
        for (int i = 0; i < s; ++i)
          {
            rho_sop *elem = rho_sop_arr_get (dest, i);
            if (elem)
              {
                rho_sop *res = _combine (elem, sop, vm);
                if (res == COMB_CANCELLED)
                  {
                    rho_sop_arr_set (dest, i, nullptr);
                    rho_sop_arr_clear_nulls (dest);
                    return;
                  }
                else if (res)
                  {
                    rho_sop_arr_set (dest, i, nullptr);
                    sop = res;
                    
                    combined = true;
                  }
              }
          }
      }
    while (combined);
    
    rho_sop_arr_clear_nulls (dest);
    rho_sop_arr_add (dest, sop);
    rho_sop_unprotect (sop);
  }
  
  static void
  _merge_products (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    for (int i = 0; i < rho_sop_arr_size (b); ++i)
      _insert_or_combine (a, rho_sop_arr_get (b, i), vm);
    
    rho_sop_unprotect (b);
  }
  
  
  
  rho_sop*
  rho_sop_mul (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    rho_sop *sop;
    
    if (rho_sop_is_number_eq (a, 1))
      {
        rho_sop_unprotect (a);
        return b;
      }
    else if (rho_sop_is_number_eq (b, 1))
      {
        rho_sop_unprotect (b);
        return a;
      }
    
    if (a->type == SOP_MUL)
      {
        sop = a;
        if (b->type == SOP_MUL)
          _merge_products (sop, b, vm);
        else
          _insert_or_combine (sop, b, vm);
      }
    else if (b->type == SOP_MUL)
      {
        sop = b;
        _insert_or_combine (sop, a, vm);
      }
    else
      {
        sop = vm.get_gc ().alloc_sop_protected ();
        sop->type = SOP_MUL;
        rho_sop_arr_init (sop);
        _insert_or_combine (sop, a, vm);
        _insert_or_combine (sop, b, vm);
      }
    
    return sop;
  }
  
  
  
  /* 
   * Returns the coefficient part of the specified product, or null if there
   * is no coefficient (coefficient equal to 1).
   */
  rho_sop*
  rho_sop_mul_get_coeff (rho_sop *sop)
  {
    int s = rho_sop_arr_size (sop);
    for (int i = 0; i < s; ++i)
      {
        rho_sop *elem = rho_sop_arr_get (sop, i);
        if (rho_sop_is_number (elem))
          {
            return elem;
          }
      }
    
    return nullptr;
  }
}

