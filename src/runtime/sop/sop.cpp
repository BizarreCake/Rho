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

#include <iostream> // DEBUG


namespace rho {
  
  void
  rho_sop_protect (rho_sop *sop, virtual_machine& vm)
  {
    vm.get_gc ().protect (sop);
  }
  
  
  
  rho_sop*
  rho_sop_new (rho_value *val, virtual_machine& vm)
  {
    rho_sop *sop = vm.get_gc ().alloc_sop_protected ();
    sop->type = val->type == RHO_SYM ? SOP_SYM : SOP_VAL;
    sop->val.val = val;
    rho_value_unprotect (val);
    return sop;
  }
  
  
  
  rho_sop*
  rho_sop_copy (rho_sop *sop, virtual_machine& vm)
  {
    rho_sop *copy = vm.get_gc ().alloc_sop_protected ();
    copy->type = sop->type;
    
    switch (sop->type)
      {
      case SOP_SYM:
      case SOP_VAL:
        copy->val.val = sop->val.val;
        break;
      
      case SOP_ADD:
      case SOP_MUL:
        rho_sop_arr_init (copy, sop->val.arr.cap);
        for (int i = 0; i < rho_sop_arr_size (sop); ++i)
          {
            copy->val.arr.elems[i] = rho_sop_copy (sop->val.arr.elems[i], vm);
            rho_sop_unprotect (copy->val.arr.elems[i]);
          }
        copy->val.arr.count = rho_sop_arr_size (sop);
        break;
      
      case SOP_DIV:
      case SOP_POW:
        copy->val.bop.lhs = rho_sop_copy (sop->val.bop.lhs, vm);
        copy->val.bop.rhs = rho_sop_copy (sop->val.bop.rhs, vm);
        rho_sop_unprotect (copy->val.bop.lhs);
        rho_sop_unprotect (copy->val.bop.rhs);
        break;
      }
    
    return copy;
  }
  
  rho_sop*
  rho_sop_shallow_copy (rho_sop *sop, virtual_machine& vm)
  {
    rho_sop *copy = vm.get_gc ().alloc_sop_protected ();
    copy->type = sop->type;
    
    switch (sop->type)
      {
      case SOP_SYM:
      case SOP_VAL:
        copy->val.val = sop->val.val;
        break;
      
      case SOP_ADD:
      case SOP_MUL:
        rho_sop_arr_init (copy, sop->val.arr.cap);
        for (int i = 0; i < rho_sop_arr_size (sop); ++i)
          copy->val.arr.elems[i] = sop->val.arr.elems[i];
        copy->val.arr.count = rho_sop_arr_size (sop);
        break;
      
      case SOP_DIV:
      case SOP_POW:
        copy->val.bop.lhs = sop->val.bop.lhs;
        copy->val.bop.rhs = sop->val.bop.rhs;
        break;
      }
    
    return copy;
  }
  
  
  
  /* 
   * Releases type-specific resources.
   * NOTE: The SOP itself or the SOPs it references are not destroyed.
   */
  void
  rho_sop_free (rho_sop *sop)
  {
    switch (sop->type)
      {
      case SOP_ADD:
      case SOP_MUL:
        delete[] sop->val.arr.elems;
        break;
      
      default: ;
      }
  }
  
  
  
//------------------------------------------------------------------------------
  
  static int
  _get_bop_type_prec (rho_sop_type type)
  {
    switch (type)
      {
      case SOP_ADD: return 1;
      case SOP_MUL: return 2;
      case SOP_DIV: return 2;
      case SOP_POW: return 3;
      
      default: ;
      }
    
    return 1000;
  }
  
  static int
  _get_bop_prec (rho_sop *sop)
  {
    return _get_bop_type_prec (sop->type);
  }
  
  
  
  static std::string
  _rho_sop_str_binop_add (rho_sop *sop)
  {
    int s = rho_sop_arr_size (sop);
    if (s == 0)
      return "0";
    
    std::string str;
    bool ns = false;
    
    for (int i = 0; i < s; ++i)
      {
        rho_sop *opr = sop->val.arr.elems[i];
        if (_get_bop_prec (opr) < _get_bop_type_prec (SOP_ADD))
          {
            str.push_back ('(');
            str.append (rho_sop_str (opr, ns));
            str.push_back (')');
          }
        else
          str.append (rho_sop_str (opr, ns));
        ns = false;
        
        if (i != s - 1)
          {
            rho_sop *next = rho_sop_arr_get (sop, i + 1);
            if (rho_sop_get_coeff_sign (next) < 0)
              {
                str.append (" - ");
                ns = true;
              }
            else
              str.append (" + ");
          }
      }
    
    return str;
  }
  
  
  static std::string
  _rho_sop_str_binop_mul (rho_sop *sop, bool no_sign)
  {
    int s = rho_sop_arr_size (sop);
    if (s == 0)
      return "1";
      
    std::string str;
    bool gs = false; // got rid of sign?
    
    for (int i = 0; i < s; ++i)
      {
        rho_sop *opr = sop->val.arr.elems[i];
        if (_get_bop_prec (opr) < _get_bop_type_prec (SOP_MUL))
          {
            str.push_back ('(');
            str.append (rho_sop_str (opr));
            str.push_back (')');
          }
        else
          {
            if (no_sign && !gs && rho_sop_is_number (opr))
              {
                if (rho_sop_is_number_eq (opr, -1))
                  continue;
                else
                  str.append (rho_sop_str (opr, no_sign));
                
                gs = true;
              }
            else
              str.append (rho_sop_str (opr));
          }
        
        if (i != s - 1)
          str.push_back ('*');
      }
    
    return str;
  }
  
  
  static std::string
  _rho_sop_str_binop_div (rho_sop *sop)
  {
    int s = rho_sop_arr_size (sop);
    if (s == 0)
      return "1";
      
    std::string str;
    
    if (_get_bop_prec (sop->val.bop.lhs) < _get_bop_type_prec (SOP_DIV))
      {
        str.push_back ('(');
        str.append (rho_sop_str (sop->val.bop.lhs));
        str.push_back (')');
      }
    else
      str.append (rho_sop_str (sop->val.bop.lhs));
    
    str.push_back ('/');
    
    if (_get_bop_prec (sop->val.bop.rhs) < _get_bop_type_prec (SOP_DIV))
      {
        str.push_back ('(');
        str.append (rho_sop_str (sop->val.bop.rhs));
        str.push_back (')');
      }
    else
      str.append (rho_sop_str (sop->val.bop.rhs));
    
    return str;
  }
  
  
  static std::string
  _rho_sop_str_binop_pow (rho_sop *sop)
  {
    std::string str;
    
    if (_get_bop_prec (sop->val.bop.lhs) < _get_bop_type_prec (SOP_POW))
      {
        str.push_back ('(');
        str.append (rho_sop_str (sop->val.bop.lhs));
        str.push_back (')');
      }
    else
      str.append (rho_sop_str (sop->val.bop.lhs));
    
    str.push_back ('^');
    
    if (_get_bop_prec (sop->val.bop.rhs) < _get_bop_type_prec (SOP_POW))
      {
        str.push_back ('(');
        str.append (rho_sop_str (sop->val.bop.rhs));
        str.push_back (')');
      }
    else
      str.append (rho_sop_str (sop->val.bop.rhs));
    
    return str;
  }
  
  
  std::string
  rho_sop_str (rho_sop *sop, bool no_sign)
  {
    switch (sop->type)
      {
      case SOP_VAL:
      case SOP_SYM:
        return rho_value_str (sop->val.val, no_sign);
      
      case SOP_ADD:
        return _rho_sop_str_binop_add (sop);
      
      case SOP_MUL:
        return _rho_sop_str_binop_mul (sop, no_sign);
      
      case SOP_DIV:
        return _rho_sop_str_binop_div (sop);
      
      case SOP_POW:
        return _rho_sop_str_binop_pow (sop);
      
      default: ;
      }
    
    return "<sop>";
  }
  
  
  
  /* 
   * Computes a 32-bit hash value for the specified SOP.
   */
  unsigned int
  rho_sop_hash (rho_sop *sop)
  {
    switch (sop->type)
      {
      case SOP_VAL:
      case SOP_SYM:
        return rho_value_hash (sop->val.val);
        
      default: return 0;
      }
  }
  
  
  
//------------------------------------------------------------------------------

  void
  rho_sop_arr_init (rho_sop *sop, int cap)
  {
    sop->val.arr.count = 0;
    sop->val.arr.cap = cap;
    sop->val.arr.elems = new rho_sop* [cap];
  }

  /* 
   * Inserts the specified SOP to the end of an array-type SOP.
   */
  void
  rho_sop_arr_add (rho_sop *sop, rho_sop *elem)
  {
    if (sop->val.arr.count == sop->val.arr.cap)
      {
        // expand array
        unsigned int ncap = (sop->val.arr.cap + 1) * 2;
        rho_sop **narr = new rho_sop* [ncap];
        std::memcpy (narr, sop->val.arr.elems,
          sizeof (rho_sop *) * sop->val.arr.count);
        
        delete[] sop->val.arr.elems;
        sop->val.arr.elems = narr;
        sop->val.arr.cap = ncap;
      }
    
    sop->val.arr.elems[sop->val.arr.count ++] = elem;
  }
  
  void
  rho_sop_arr_clear_nulls (rho_sop *sop)
  {
    rho_sop **narr = new rho_sop* [sop->val.arr.cap];
    int s = sop->val.arr.count;
    int pos = 0;
    for (int i = 0; i < s; ++i)
      if (sop->val.arr.elems[i])
        narr[pos++] = sop->val.arr.elems[i];
    
    sop->val.arr.count = pos;
    delete[] sop->val.arr.elems;
    sop->val.arr.elems = narr;
  }
  
  
  
  /* 
   * Returns true if the specified SOP holds a number-type value.
   */
  bool
  rho_sop_is_number (rho_sop *sop)
    { return sop->type == SOP_VAL && rho_value_is_number (sop->val.val); }
  
  /* 
   * Checks whether the specified SOP holds a number equal to the given one.
   */
  bool
  rho_sop_is_number_eq (rho_sop *sop, int num)
  {
    if (sop->type != SOP_VAL)
      return false;
    
    rho_value *val = sop->val.val;
    switch (val->type)
      {
      case RHO_INT:
        return mpz_cmp_si (val->val.i, num) == 0;
      
      case RHO_REAL:
        return mpfr_cmp_si (val->val.real.f, num) == 0;
      
      default:
        return false;
      }
  }
}

