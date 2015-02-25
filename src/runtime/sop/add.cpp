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
#include <cstring>
#include <vector>

#include <iostream> // DEBUG


namespace rho {
  
#define COMB_NOT_POSSIBLE   (rho_sop *)0
#define COMB_CANCELLED      (rho_sop *)1
  
  // forward decs:
  static rho_sop* _combine_product_with_symbol (rho_sop *a, rho_sop *b,
    virtual_machine& vm);
  
  
  
  static rho_sop*
  _combine_value_with_value (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    if (rho_value_is_number (a->val.val) && rho_value_is_number (b->val.val))
      {
        rho_value *nval = rho_value_add (a->val.val, b->val.val, vm);
        if (rho_value_is_number_eq (nval, 0))
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
    if (std::strcmp (a->val.val->val.sym, b->val.val->val.sym) == 0)
      {
        rho_value *coeff = rho_value_new_int (2, vm);
        rho_sop *csop = rho_sop_new (coeff, vm);
        rho_sop *prod = rho_sop_mul (csop, a, vm);
        
        rho_sop_unprotect (b);
        return prod;
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
      
      case SOP_MUL:
        return _combine_product_with_symbol (b, a, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  static void
  _get_mul_parts (rho_sop *sop, std::vector<rho_sop *>& out, bool coeffs)
  {
    for (int i = 0; i < rho_sop_arr_size (sop); ++i)
      {
        rho_sop *f = rho_sop_arr_get (sop, i);
        if (coeffs == rho_sop_is_number (f))
          out.push_back (f);
      }
  }
  
  static bool
  _parts_match (const std::vector<rho_sop *>& a, const std::vector<rho_sop *>& b)
  {
    if (a.size () != b.size ())
      return false;
    
    bool *match_arr = new bool [a.size ()];
    std::memset (match_arr, 0, a.size ());
    for (unsigned int i = 0; i < a.size (); ++i)
      {
        bool match = false;
        for (unsigned int j = 0; j < b.size (); ++j)
          {
            if (!match_arr[j])
              {
                if (rho_sop_eq (a[i], b[j]))
                  {
                    match_arr[j] = true;
                    match = true;
                    break;
                  }
              }
          }
        
        if (!match)
          {
            delete[] match_arr;
            return false;
          }
      }
    
    delete[] match_arr;
    return true;
  }
  
  
  static rho_sop*
  _combine_product_with_product (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    // get non-coefficient factors and make sure they match
    std::vector<rho_sop *> nc_a, nc_b;
    _get_mul_parts (a, nc_a, false);
    _get_mul_parts (b, nc_b, false);
    if (!_parts_match (nc_a, nc_b))
      return COMB_NOT_POSSIBLE;
    
    // get coefficients
    rho_sop *c_a = rho_sop_mul_get_coeff (a);
    if (!c_a)
      c_a = rho_sop_new (rho_value_new_int (1, vm), vm);
      
    rho_sop *c_b = rho_sop_mul_get_coeff (b);
    if (!c_b)
      c_b = rho_sop_new (rho_value_new_int (1, vm), vm);
    
    // build new coefficient
    rho_sop *coeff;
    {
      rho_value *cv = rho_value_add (c_a->val.val, c_b->val.val, vm);
      rho_sop_unprotect (c_a);
      rho_sop_unprotect (c_b);
      if (rho_value_is_number_eq (cv, 0))
        {
          rho_value_unprotect (cv);
          rho_sop_unprotect (a);
          rho_sop_unprotect (b);
          return COMB_CANCELLED;
        }
      coeff = rho_sop_new (cv, vm);
    }
    
    // non-coefficient part
    rho_sop *nc;
    if (nc_a.size () == 1)
      nc = nc_a[0];
    else
      {
        nc = vm.get_gc ().alloc_sop_protected ();
        nc->type = SOP_MUL;
        rho_sop_arr_init (nc);
        for (rho_sop *sop : nc_a)
          rho_sop_arr_add (nc, sop);
      }
    
    rho_sop_unprotect (a);
    rho_sop_unprotect (b);
    return rho_sop_mul (coeff, nc, vm);
  }
  
  rho_sop*
  _combine_product_with_symbol (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    std::vector<rho_sop *> nc_a;
    _get_mul_parts (a, nc_a, false);
    
    if (nc_a.size () != 1 || nc_a[0]->type != SOP_SYM
      || (std::strcmp (nc_a[0]->val.val->val.sym, b->val.val->val.sym) != 0))
      return COMB_NOT_POSSIBLE;
    
    rho_sop_unprotect (b);
    
    rho_sop *coeff = nullptr;
    int pos = -1;
    for (int i = 0; i < rho_sop_arr_size (a); ++i)
      if (rho_sop_is_number (rho_sop_arr_get (a, i)))
        {
          coeff = rho_sop_arr_get (a, i);
          pos = i;
          break;
        }
    
    rho_value *val = coeff ? coeff->val.val : rho_value_new_int (1, vm);
    rho_value *one = rho_value_new_int (1, vm);
    rho_value *nval = rho_value_add (val, one, vm);
    rho_value_unprotect (one);
    rho_sop_unprotect (coeff);
    
    rho_sop *nsop = rho_sop_new (nval, vm);
    if (pos != -1)
      rho_sop_arr_set (a, pos, nsop);
    else
      rho_sop_arr_add (a, nsop);
    rho_sop_unprotect (nsop);
    
    return a;
  }
  
  static rho_sop*
  _combine_product_with (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    switch (b->type)
      {
      case SOP_MUL:
        return _combine_product_with_product (a, b, vm);
      
      case SOP_SYM:
        return _combine_product_with_symbol (a, b, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
 
  /* 
   * Attempts to combine two SOP values into one SOP in some way
   * (e.g. 'x + 'x will be turned into 2*'x).
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
      
      case SOP_MUL:
        return _combine_product_with (a, b, vm);
      
      default: ;
      }
    
    return COMB_NOT_POSSIBLE;
  }
  
  
  
  /* 
   * Attempts to combine the specified SOP with some existing element in the
   * sum. If not possible, inserts the SOP to the end of the sum.
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
  _merge_sums (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    for (int i = 0; i < rho_sop_arr_size (b); ++i)
      _insert_or_combine (a, rho_sop_arr_get (b, i), vm);
    
    rho_sop_unprotect (b);
  }
  
  
  rho_sop*
  rho_sop_add (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    rho_sop *sop;
    
    if (rho_sop_is_number_eq (a, 0))
      {
        rho_sop_unprotect (a);
        return b;
      }
    else if (rho_sop_is_number_eq (b, 0))
      {
        rho_sop_unprotect (b);
        return a;
      }
    
    if (a->type == SOP_ADD)
      {
        sop = a;
        if (b->type == SOP_ADD)
          _merge_sums (sop, b, vm);
        else
          _insert_or_combine (sop, b, vm);
      }
    else if (b->type == SOP_ADD)
      {
        sop = b;
        _insert_or_combine (sop, a, vm);
      }
    else
      {
        sop = vm.get_gc ().alloc_sop_protected ();
        sop->type = SOP_ADD;
        rho_sop_arr_init (sop);
        _insert_or_combine (sop, a, vm);
        _insert_or_combine (sop, b, vm);
      }
    
    return sop;
  }
}

