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

#ifndef _RHO__RUNTIME__VALUE__H_
#define _RHO__RUNTIME__VALUE__H_

#include <gmp.h>
#include <mpfr.h>
#include <string>


namespace rho {
  
  class virtual_machine;
  class rho_sop;
  
  /* 
   * The value types, as seen by the runtime environment.
   */
  enum rho_value_type
  {
    RHO_NIL,
    RHO_INT,
    RHO_REAL,
    RHO_FUNC,
    RHO_SYM,
    RHO_SOP,   // symbolic operation
    RHO_EMPTY_CONS,
    RHO_CONS,  // cons cell
  };
  
  
  
  /* 
   * A tagged Rho value.
   */
  struct rho_value
  {
    union
      {
        mpz_t i;
        struct
          {
            mpfr_t f;
            int prec;
          } real;
        struct
          {
            unsigned char *code;
          } fn;
        char *sym;
        rho_sop *sop;
        struct
          {
            rho_value *car, *cdr;
          } cons;
      } val;
    
    unsigned char type;
    
    // GC fields:
    unsigned gc_protect:1;
    unsigned gc_state:2;
  };
  
  
  
//------------------------------------------------------------------------------
  // 
  // GC-related.
  // 
  
  /* 
   * Removes GC protection from the specified value.
   */
  inline void
  rho_value_unprotect (rho_value *val)
    { val->gc_protect = 0; }
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
  // 
  // Value instantiation.
  // NOTE: All created values are given GC protection, and so
  //       rho_value_unprotect() must be called or else the GC will not reclaim
  //       the values once they die.
  // 
  
  rho_value* rho_value_new_nil (virtual_machine& vm);
  
  
  rho_value* rho_value_new_int (int num, virtual_machine& vm);
  
  rho_value* rho_value_new_int (const char *str, virtual_machine& vm);
  
  
  rho_value* rho_value_new_real (double num, int prec, virtual_machine& vm);
  
  rho_value* rho_value_new_real (const char *str, int prec, virtual_machine& vm);
  
  
  rho_value* rho_value_new_func (unsigned int code_size, virtual_machine& vm);
  
  
  rho_value* rho_value_new_sym (const char *str, virtual_machine& vm);
  
  
  rho_value* rho_value_new_empty_cons (virtual_machine& vm);
  
  rho_value* rho_value_new_cons (rho_value *car, rho_value *cdr,
    virtual_machine& vm);
  
  
  rho_value* rho_value_new_sop (rho_sop *sop, virtual_machine& vm);
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
  // 
  // Basic operations.
  // 
  
  // numeric:
  
  rho_value* rho_value_add (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_sub (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_mul (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_div (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_idiv (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_mod (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_pow (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_factorial (rho_value *val, virtual_machine& vm);
  
  rho_value* rho_value_negate (rho_value *val, virtual_machine& vm);
  
  
  
  // comparison:
  
  rho_value* rho_value_eq (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_neq (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_lt (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_lte (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_gt (rho_value *a, rho_value *b, virtual_machine& vm);
  
  rho_value* rho_value_gte (rho_value *a, rho_value *b, virtual_machine& vm);
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
  // 
  // Other.
  // 
  
  std::string rho_value_str (rho_value *val, bool no_sign = false);
  
  /* 
   * Computes a 32-bit hash value for the specified value.
   */
  unsigned int rho_value_hash (rho_value *val);
  
  
  
  /* 
   * Checks whether the specified value is of a numerical type.
   */
  inline bool
  rho_value_is_number (rho_value *val)
    { return val->type == RHO_INT || val->type == RHO_REAL; }
  
  /* 
   * Returns true if the specified value holds a number equal to the given one.
   */
  bool
  rho_value_is_number_eq (rho_value *val, int num);
  
//------------------------------------------------------------------------------
} 

#endif

