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

#include "runtime/value.hpp"
#include "runtime/sop.hpp"
#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"
#include <sstream>
#include <cstring>

#include <iostream> // DEBUG


namespace rho {

#define MAX(A,B) (((A) > (B)) ? (A) : (B))

  static int
  _compute_prec (rho_value *a, rho_value *b)
  {
    switch (a->type)
      {
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            return MAX(a->val.real.prec, b->val.real.prec);
          
          case RHO_INT:
            return a->val.real.prec;
          
          default: ;
          }
        break;
      
      case RHO_INT:
        switch (b->type)
          {
          case RHO_REAL:
            return b->val.real.prec;
          
          default: ;
          }
        break;
      
      default: ;
      }
    
    return 20; 
  }
  
//------------------------------------------------------------------------------
  
  rho_value*
  rho_value_new_nil (virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  
  rho_value*
  rho_value_new_int (int num, virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_INT;
    mpz_init_set_si (val->val.i, num);
    return val;
  }
  
  rho_value*
  rho_value_new_int (const char *str, virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_INT;
    mpz_init_set_str (val->val.i, str, 10);
    return val;
  }
  
  
  
  rho_value*
  rho_value_new_real (double num, int prec, virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_REAL;
    val->val.real.prec = prec;
    mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
    mpfr_set_d (val->val.real.f, num, MPFR_RNDN);
    return val;
  }
  
  rho_value*
  rho_value_new_real (const char *str, int prec, virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_REAL;
    val->val.real.prec = prec;
    mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
    mpfr_set_str (val->val.real.f, str, 10, MPFR_RNDN);
    return val;
  }
  
  
  
  rho_value*
  rho_value_new_func (unsigned int code_size, virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_FUNC;
    
    val->val.fn.code = new unsigned char [code_size];
    
    return val;
  }
  
  
  
  rho_value*
  rho_value_new_sym (const char *str, virtual_machine& vm)
  {
    int len = std::strlen (str);
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_SYM;
    val->val.sym = new char[len + 1];
    std::strcpy (val->val.sym, str);
    return val;
  }
  
  
  
//------------------------------------------------------------------------------
  
  rho_value*
  rho_value_add (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_INT;
              mpz_init (val->val.i);
              mpz_add (val->val.i, a->val.i, b->val.i);
              return val;
            }
          
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_add_z (val->val.real.f, b->val.real.f, a->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_add (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_add (val->val.real.f, a->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_INT:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_add_z (val->val.real.f, a->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_add (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_SYM:
        {
          rho_value *val = vm.get_gc ().alloc_protected ();
          val->type = RHO_SOP;
          val->val.sop = rho_sop_add (rho_sop_new (a, vm),
            b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm), vm);
          rho_sop_unprotect (val->val.sop);
          return val;
        }
      
      case RHO_SOP:
        switch (b->type)
          {
            case RHO_SOP:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_add (rho_sop_copy (a->val.sop, vm),
                  rho_sop_copy (b->val.sop, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
            
            default:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_add (rho_sop_copy (a->val.sop, vm),
                  rho_sop_new (b, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  rho_value*
  rho_value_sub (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_INT;
              mpz_init (val->val.i);
              mpz_sub (val->val.i, a->val.i, b->val.i);
              return val;
            }
          
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_sub_z (val->val.real.f, b->val.real.f, a->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_sub (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_sub (val->val.real.f, a->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_INT:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_sub_z (val->val.real.f, a->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_sub (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_SYM:
        {
          rho_value *val = vm.get_gc ().alloc_protected ();
          val->type = RHO_SOP;
          val->val.sop = rho_sop_sub (rho_sop_new (a, vm),
            b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm), vm);
          rho_sop_unprotect (val->val.sop);
          return val;
        }
      
      case RHO_SOP:
        switch (b->type)
          {
            case RHO_SOP:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_sub (rho_sop_copy (a->val.sop, vm),
                  rho_sop_copy (b->val.sop, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
            
            default:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_sub (rho_sop_copy (a->val.sop, vm),
                  rho_sop_new (b, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  rho_value*
  rho_value_mul (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_INT;
              mpz_init (val->val.i);
              mpz_mul (val->val.i, a->val.i, b->val.i);
              return val;
            }
          
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_mul_z (val->val.real.f, b->val.real.f, a->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_mul (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_mul (val->val.real.f, a->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_INT:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_mul_z (val->val.real.f, a->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_mul (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_SYM:
        {
          rho_value *val = vm.get_gc ().alloc_protected ();
          val->type = RHO_SOP;
          val->val.sop = rho_sop_mul (rho_sop_new (a, vm),
            b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm), vm);
          rho_sop_unprotect (val->val.sop);
          return val;
        }
      
      case RHO_SOP:
        switch (b->type)
          {
            case RHO_SOP:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_mul (rho_sop_copy (a->val.sop, vm),
                  rho_sop_copy (b->val.sop, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
            
            default:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_mul (rho_sop_copy (a->val.sop, vm),
                  rho_sop_new (b, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  rho_value*
  rho_value_div (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              int prec = vm.get_current_prec ();
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_set_z (val->val.real.f, a->val.i, MPFR_RNDN);
              mpfr_div_z (val->val.real.f, val->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_set_z (val->val.real.f, a->val.i, MPFR_RNDN);
              mpfr_div (val->val.real.f, val->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_div (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_div (val->val.real.f, a->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_INT:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_div_z (val->val.real.f, a->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_div (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_SYM:
        {
          rho_value *val = vm.get_gc ().alloc_protected ();
          val->type = RHO_SOP;
          val->val.sop = rho_sop_div (rho_sop_new (a, vm),
            b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm), vm);
          rho_sop_unprotect (val->val.sop);
          return val;
        }
      
      case RHO_SOP:
        switch (b->type)
          {
            case RHO_SOP:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_div (rho_sop_copy (a->val.sop, vm),
                  rho_sop_copy (b->val.sop, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
            
            default:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_div (rho_sop_copy (a->val.sop, vm),
                  rho_sop_new (b, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  rho_value*
  rho_value_mod (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_INT;
              mpz_init (val->val.i);
              mpz_mod (val->val.i, a->val.i, b->val.i);
              return val;
            }
          
          default: ;
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  rho_value*
  rho_value_pow (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_INT;
              mpz_init (val->val.i);
              mpz_pow_ui (val->val.i, a->val.i, mpz_get_ui (b->val.i));
              return val;
            }
          
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_set_z (val->val.real.f, a->val.i, MPFR_RNDN);
              mpfr_pow (val->val.real.f, val->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_pow (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_REAL:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_pow (val->val.real.f, a->val.real.f, b->val.real.f, MPFR_RNDN);
              return val;
            }
          
          case RHO_INT:
            {
              int prec = _compute_prec (a, b);
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_REAL;
              val->val.real.prec = prec;
              mpfr_init2 (val->val.real.f, 1 + (int)((prec + 3) * 3.321928095));
              mpfr_pow_z (val->val.real.f, a->val.real.f, b->val.i, MPFR_RNDN);
              return val;
            }
          
          case RHO_SOP:
          case RHO_SYM:
            {
              rho_value *val = vm.get_gc ().alloc_protected ();
              val->type = RHO_SOP;
              val->val.sop = rho_sop_pow (rho_sop_new (a, vm),
                b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm),
                vm);
              rho_sop_unprotect (val->val.sop);
              return val;
            }
          
          default: ;
          }
        break;
      
      case RHO_SYM:
        {
          rho_value *val = vm.get_gc ().alloc_protected ();
          val->type = RHO_SOP;
          val->val.sop = rho_sop_pow (rho_sop_new (a, vm),
            b->type == RHO_SOP ? rho_sop_copy (b->val.sop, vm) : rho_sop_new (b, vm), vm);
          rho_sop_unprotect (val->val.sop);
          return val;
        }
      
      case RHO_SOP:
        switch (b->type)
          {
            case RHO_SOP:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_pow (rho_sop_copy (a->val.sop, vm),
                  rho_sop_copy (b->val.sop, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
            
            default:
              {
                rho_value *val = vm.get_gc ().alloc_protected ();
                val->type = RHO_SOP;
                val->val.sop = rho_sop_pow (rho_sop_copy (a->val.sop, vm),
                  rho_sop_new (b, vm), vm);
                rho_sop_unprotect (val->val.sop);
                return val;
              }
          }
        break;
      
      default: ;
      }
    
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_NIL;
    return val;
  }
  
  
  
  rho_value*
  rho_value_factorial (rho_value *val, virtual_machine& vm)
  {
    switch (val->type)
      {
        case RHO_INT:
          {
            rho_value *res = vm.get_gc ().alloc_protected ();
            res->type = RHO_INT;
            mpz_init (res->val.i);
            mpz_fac_ui (res->val.i, mpz_get_ui (val->val.i));
            return res;
          }
          break;
      }
    
    rho_value *res = vm.get_gc ().alloc_protected ();
    res->type = RHO_NIL;
    return res;
  }
  
  
  
  rho_value*
  rho_value_negate (rho_value *val, virtual_machine& vm)
  {
    switch (val->type)
      {
        case RHO_INT:
          {
            rho_value *res = vm.get_gc ().alloc_protected ();
            res->type = RHO_INT;
            mpz_init (res->val.i);
            mpz_ui_sub (res->val.i, 0, val->val.i);
            return res;
          }
          break;
        
        case RHO_REAL:
          {
            rho_value *res = vm.get_gc ().alloc_protected ();
            res->type = RHO_REAL;
            res->val.real.prec = val->val.real.prec;
            mpfr_init2 (res->val.real.f, 1 + (int)((val->val.real.prec + 3) * 3.321928095));
            mpfr_neg (res->val.real.f, val->val.real.f, MPFR_RNDN);
            return res;
          }
          break;
      }
    
    rho_value *res = vm.get_gc ().alloc_protected ();
    res->type = RHO_NIL;
    return res;
  }
  
  
  
  // comparison:
  
  static rho_value*
  _make_true (virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_INT;
    mpz_init_set_ui (val->val.i, 1);
    return val;
  }
  
  static rho_value*
  _make_false (virtual_machine& vm)
  {
    rho_value *val = vm.get_gc ().alloc_protected ();
    val->type = RHO_INT;
    mpz_init (val->val.i);
    return val;
  }
  
  
  rho_value*
  rho_value_eq (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) == 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) == 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) == 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  
  
  rho_value*
  rho_value_neq (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) != 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) != 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) != 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  
  
  rho_value*
  rho_value_lt (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) < 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) > 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) < 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  
  
  rho_value*
  rho_value_lte (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) <= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) >= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) <= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  
  
  rho_value*
  rho_value_gt (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) > 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) < 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) > 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  
  
  rho_value*
  rho_value_gte (rho_value *a, rho_value *b, virtual_machine& vm)
  {
    switch (a->type)
      {
      case RHO_INT:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpz_cmp (a->val.i, b->val.i) >= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          case RHO_REAL:
            {
              if (mpfr_cmp_z (b->val.real.f, a->val.i) <= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          
          default: ;
          }
        break;
      
      case RHO_REAL:
        switch (b->type)
          {
          case RHO_INT:
            {
              if (mpfr_cmp_z (a->val.real.f, b->val.i) >= 0)
                return _make_true (vm);
              return _make_false (vm);
            }
          }
        break;
      
      default: ;
      }
    
    return _make_false (vm);
  }
  
  

//------------------------------------------------------------------------------
  
  static std::string
  _real_to_str (rho_value *val)
  {
    std::ostringstream ss;
    ss << "%." << val->val.real.prec << "RNf";
    
    mpfr_exp_t e = mpfr_get_exp (val->val.real.f);
    
    char *buf = new char[val->val.real.prec + 10 + (int)(e*3.321928095)];
    mpfr_sprintf (buf, ss.str ().c_str (), val->val.real.f);
    
    std::string str = buf;
    delete[] buf;
    
    // remove trailing zeroes
    while (!str.empty () && str.back () == '0')
      str.pop_back ();
    if (str.empty () || str.back () == '.')
      str.push_back ('0');
    
    return str;
  }
  
  std::string
  rho_value_str (rho_value *val)
  {
    switch (val->type)
      {
      case RHO_NIL: return "nil";
      
      case RHO_INT:
        {
          char *s = mpz_get_str (nullptr, 10, val->val.i);
          std::string str {s};
          free (s);
          return str;
        }
      
      case RHO_REAL:
        return _real_to_str (val);
      
      case RHO_FUNC:
        {
          std::ostringstream ss;
          ss << "<func " << (void *)val->val.fn.code << ">";
          return ss.str ();
        }
      
      case RHO_SYM:
        return val->val.sym;
      
      case RHO_SOP:
        return rho_sop_str (val->val.sop);
      }
    
    return "";
  }
  
  
  
  /* 
   * Computes a 32-bit hash value for the specified value.
   */
  unsigned int
  rho_value_hash (rho_value *val)
  {
    switch (val->type)
      {
      case RHO_SYM:
        {
          unsigned int h = 31;
          const char *ptr = val->val.sym;
          while (*ptr)
            {
              h += (31 * h) ^ *ptr;
              ++ ptr;
            }
          return h;
        }
        break;
      
      case RHO_SOP:
        return rho_sop_hash (val->val.sop);
      
      default:
        return 0;
      }
    
    return 0;
  }
  
  
  
  /* 
   * Returns true if the specified value holds a number equal to the given one.
   */
  bool
  rho_value_is_number_eq (rho_value *val, int num)
  {
    switch (val->type)
      {
      case RHO_INT:
        return mpz_cmp_ui (val->val.i, num) == 0;
      
      case RHO_REAL:
        return mpfr_cmp_ui (val->val.real.f, num) == 0;
      
      default:
        return false;
      }
  }
}

