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
#include <cstring>
#include <vector>


namespace rho {
  
  static bool
  _eq_val (rho_sop *a, rho_sop *b)
  {
    switch (a->val.val->type)
      {
      case RHO_INT:
        switch (b->val.val->type)
          {
          case RHO_INT:
            return mpz_cmp (a->val.val->val.i, b->val.val->val.i) == 0;
          
          case RHO_REAL:
            return mpfr_cmp_z (b->val.val->val.real.f, a->val.val->val.i) == 0;
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->val.val->type)
          {
          case RHO_INT:
            return mpfr_cmp_z (a->val.val->val.real.f, b->val.val->val.i) == 0;
          
          case RHO_REAL:
            return mpfr_cmp (a->val.val->val.real.f, b->val.val->val.real.f) == 0;
          
          default: ;
          }
        break;
      
      default: ;
      }
    
    return false;
  }
  
  
  
  static bool
  _eq_sym (rho_sop *a, rho_sop *b)
  {
    switch (b->type)
      {
      case SOP_SYM:
        return std::strcmp (a->val.val->val.sym, b->val.val->val.sym) == 0;
      
      default:
        return false;
      }
  }
  
  
  
  static bool
  _eq_pow (rho_sop *a, rho_sop *b)
  {
    switch (b->type)
      {
      case SOP_POW:
        return rho_sop_eq (a->val.bop.lhs, b->val.bop.lhs) &&
               rho_sop_eq (a->val.bop.rhs, b->val.bop.rhs);
      
      default:
        return false;
      }
  }
  
  
  
  /* 
   * Compares between two SOPs and checks whether they represent the same
   * expression (semantically, e.g. 5*x^2 and x^2*5 are the considered equal).
   * Note that a simple check is done, and so complex expressions that are
   * equal might not be considered as such.
   */
  bool
  rho_sop_eq (rho_sop *a, rho_sop *b)
  {
    switch (a->type)
      {
      case SOP_VAL:
        return _eq_val (a, b);
      
      case SOP_SYM:
        return _eq_sym (a, b);
      
      case SOP_POW:
        return _eq_pow (a, b);
      
      default:
        return false;
      }
  }
}

