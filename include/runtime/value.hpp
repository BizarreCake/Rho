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

#ifndef _RHO__RUNTIME__VALUE__H_
#define _RHO__RUNTIME__VALUE__H_

#include <string>
#include <gmp.h>
#include <mpfr.h>
#include <stdexcept>


namespace rho {
  
  /* 
   * Thrown by the virtual machine in case of a fatal error.
   */
  class vm_error: public std::runtime_error
  {
  public:
    vm_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  
#define RHO_IS_NIL(V) ((V).type == RHO_NIL)

  
  // forward decs:
  class virtual_machine;
  class garbage_collector;
  
  
  enum rho_type: int
  {
    // internal types:
    RHO_INTERNAL,
    RHO_PVAR,   // pattern variable
    RHO_UPVAL,
    
    RHO_NIL,
    RHO_BOOL,
    RHO_INTEGER,
    RHO_FUN,
    RHO_EMPTY_LIST,
    RHO_CONS,
    RHO_VEC,
    RHO_ATOM,
    RHO_STR,
    RHO_FLOAT,
  };
  
  bool rho_type_is_collectable (rho_type type);
  
  
  
  struct gc_value;
  
  /* 
   * The structure of any Rho value residing in the heap.
   */
  struct rho_value
  {
    rho_type type;
    union
      {
        bool b;
        int i32;
        long long i64;
        gc_value *gc;
      } val;
  };
  
  struct gc_value
  {
    rho_type type;
    union
      {
        mpz_t i;  // integer
        
        mpfr_t f; // float
        
        // string
        struct
          {
            char *str;
            long len;
          } s;
        
        // vector
        struct
          {
            rho_value *vals;
            long len;
            long cap;
          } vec;
        
        // function
        struct
          {
            const unsigned char *cp; // code pointer
            rho_value *env;
            int env_len;
          } fn;
        
        // pair
        struct
          {
            rho_value fst;
            rho_value snd;
          } p;
        
        // upvalue
        struct
          {
            int sp;
            rho_value val;
          } uv;
      } val;
    
    // gc stuff:
    unsigned gc_protected:1;
    unsigned gc_state:2;
  };
  
  
  
  inline void
  gc_unprotect (gc_value *v)
    { if (v) v->gc_protected = 0; }
  
  inline void
  gc_unprotect (rho_value& v)
    { if (rho_type_is_collectable (v.type) && v.val.gc) v.val.gc->gc_protected = 0; }
   
  /*
   * Applies gc_unprotect() recursively.
   */
  void gc_unprotect_rec (rho_value& v);
  
  inline void
  gc_protect (gc_value *v)
    { if (v) v->gc_protected = 1; }
  
  inline void
  gc_protect (rho_value& v)
    { if (rho_type_is_collectable (v.type) && v.val.gc) v.val.gc->gc_protected = 1; }
    
   
  
  /* 
   * Reclaims memory used by the specified Rho value (but does not free the
   * value itself).
   */
  void destroy_rho_value (rho_value& v);
  void destroy_gc_value (gc_value *v);
  
  
  
  /* 
   * Returns a textual representation of the specified Rho value.
   */
  std::string rho_value_str (rho_value& v, virtual_machine& vm);
  
  
  
  //
  // Rho value construction functions:
  // 
  
  inline rho_value
  rho_value_make_internal (int val)
    { rho_value v; v.type = RHO_INTERNAL; v.val.i64 = val; return v; }

  inline rho_value
  rho_value_make_nil ()
    { rho_value v; v.type = RHO_NIL; return v; }
  
  inline rho_value
  rho_value_make_bool (bool val)
    { rho_value v; v.type = RHO_BOOL; v.val.b = val; return v; }

  rho_value rho_value_make_int (garbage_collector& gc);

  rho_value rho_value_make_int (int val, garbage_collector& gc);
  
  rho_value rho_value_make_int (const char *str, garbage_collector& gc);
  
  rho_value rho_value_make_vec (long cap, garbage_collector& gc);
  
  rho_value rho_value_make_function (const unsigned char *cp, int env_len,
                                     garbage_collector& gc);
  
  rho_value rho_value_make_empty_list (garbage_collector& gc);
  
  rho_value rho_value_make_cons (rho_value& fst, rho_value& snd,
                                 garbage_collector& gc);
  
  inline rho_value
  rho_value_make_pvar (int pv)
    { rho_value v; v.type = RHO_PVAR; v.val.i32 = pv; return v; }
  
  rho_value rho_value_make_upvalue (garbage_collector& gc);
  
  inline rho_value
  rho_value_make_atom (int val)
    { rho_value v; v.type = RHO_ATOM; v.val.i32 = val; return v; }
  
  rho_value rho_value_make_string (const char *str, long len,
                                   garbage_collector& gc);
  
  rho_value rho_value_make_float (unsigned int prec, garbage_collector& gc);

  rho_value rho_value_make_float (double val, unsigned int prec,
                                  garbage_collector& gc);
  
  
  
  // 
  // Basic operations on Rho values:
  // 
  
  rho_value rho_value_add (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  rho_value rho_value_sub (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  rho_value rho_value_mul (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  rho_value rho_value_div (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  rho_value rho_value_pow (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  rho_value rho_value_mod (rho_value& lhs, rho_value& rhs,
    virtual_machine& vm);
  
  
  // 
  // Comparison functions:
  // 
  
  bool rho_value_cmp_eq (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_neq (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_lt (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_lte (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_gt (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_gte (rho_value& lhs, rho_value& rhs);
  
  bool rho_value_cmp_zero (rho_value& v);
  
  bool rho_value_cmp_ref_eq (rho_value& lhs, rho_value& rhs);
  
  
  
  // 
  // Pattern matching:
  // 
  
  /* 
   * Attempts to match the specified value against the given pattern.
   */
  bool rho_value_match (rho_value& pat, rho_value& val, rho_value *stack);
}

#endif

