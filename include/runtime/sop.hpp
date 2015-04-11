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

#ifndef _RHO__RUNTIME__SOP__H_
#define _RHO__RUNTIME__SOP__H_

#include <string>


namespace rho {
  
  struct rho_value;
  class virtual_machine;
  
  enum rho_sop_type
  {
    SOP_VAL,    // Rho value
    SOP_SYM,
    
    SOP_ADD,
    SOP_MUL,
    SOP_DIV,
    SOP_POW,
  };
  
  /* 
   * Symbolic operation.
   */
  struct rho_sop
  {
    rho_sop_type type;
    union
      {
        rho_value *val;
        
        // binary operation:
        struct
          {
            rho_sop *lhs;
            rho_sop *rhs;
          } bop;
        
        struct
        {
          rho_sop **elems;
          unsigned int count;
          unsigned int cap;
        } arr;
      } val;
    
    // GC fields:
    unsigned gc_protect:1;
    unsigned gc_state:2;
  };
  
  
  
//------------------------------------------------------------------------------
// 
// Creation/Deletion:
// 

  rho_sop* rho_sop_new (rho_value *val, virtual_machine& vm);
  
  rho_sop* rho_sop_copy (rho_sop *sop, virtual_machine& vm);
  
  rho_sop* rho_sop_shallow_copy (rho_sop *sop, virtual_machine& vm);
  
  
  /* 
   * Releases type-specific resources.
   * NOTE: The SOP itself or the SOPs it references are not destroyed.
   */
  void rho_sop_free (rho_sop *sop);
  
  
  inline void
  rho_sop_unprotect (rho_sop *sop)
    { sop->gc_protect = 0; }
  
  void rho_sop_protect (rho_sop *sop, virtual_machine& vm);

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// 
// Binary operations:
// Ownership of the SOP arguments is implicitely acquired in these functions
// (i.e. the functions can dispose of or change the parameters).
// These take protected SOPs as input, and return a protected SOP as output.
// 
  
  rho_sop* rho_sop_add (rho_sop *a, rho_sop *b, virtual_machine& vm);
  
  rho_sop* rho_sop_sub (rho_sop *a, rho_sop *b, virtual_machine& vm);
  
  rho_sop* rho_sop_mul (rho_sop *a, rho_sop *b, virtual_machine& vm);
  
  rho_sop* rho_sop_div (rho_sop *a, rho_sop *b, virtual_machine& vm);
  
  rho_sop* rho_sop_pow (rho_sop *a, rho_sop *b, virtual_machine& vm);
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// 
// Common operations:
// 
  
  enum sop_simplify_level
  {
    SIMPLIFY_BASIC,
    SIMPLIFY_NORMAL,
    SIMPLIFY_FULL,
  };
  
  rho_sop* rho_sop_simplify (rho_sop *sop, sop_simplify_level level);
  
  
  /* 
   * Flattens out redundant expressions (e.g. a sum containing just one element
   * will be flattened into just that element).
   */
  rho_sop* rho_sop_flatten (rho_sop *sop, virtual_machine& vm);
  
  /* 
   * Repeatedly flattens out a SOP expression until not possible anymore.
   */
  rho_sop* rho_sop_deep_flatten (rho_sop *sop, virtual_machine& vm);
  
  
  
  /* 
   * Expands out products and positive integer powers.
   * Only expands the top-most expression.
   */
  rho_sop* rho_sop_expand (rho_sop *sop, virtual_machine& vm);
  
  /* 
   * Recursively expands out products and positive integer powers.
   */
  rho_sop* rho_sop_expand_all (rho_sop *sop, virtual_machine& vm);
  
  
  
  /* 
   * Substitutes an expression in the place of the symbol with the given name.
   */
  rho_sop* rho_sop_substitute (rho_sop *dest, const char *sym, rho_sop *src,
    virtual_machine& vm);
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// 
// Other:
// 
  
  std::string rho_sop_str (rho_sop *sop, bool no_sign = false);
  
  
  /* 
   * Computes a 32-bit hash value for the specified SOP.
   */
  unsigned int rho_sop_hash (rho_sop *sop);
  
  
  
  /* 
   * Compares between two SOPs and checks whether they represent the same
   * expression (semantically, e.g. 5*x^2 and x^2*5 are the considered equal).
   * Note that a simple check is done, and so complex expressions that are
   * equal might not be considered as such.
   */
  bool rho_sop_eq (rho_sop *a, rho_sop *b);
  
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// 
// Utility functions:
//   

  void rho_sop_arr_init (rho_sop *sop, int cap = 8);  
  
  /* 
   * Inserts the specified SOP to the end of an array-type SOP.
   */
  void rho_sop_arr_add (rho_sop *sop, rho_sop *elem);
  
  void rho_sop_arr_clear_nulls (rho_sop *sop);
  
  inline int
  rho_sop_arr_size (rho_sop *sop)
    { return sop->val.arr.count; }
  
  inline rho_sop*
  rho_sop_arr_get (rho_sop *sop, int index)
    { return sop->val.arr.elems[index]; }
  
  inline void
  rho_sop_arr_set (rho_sop *sop, int index, rho_sop *val)
    { sop->val.arr.elems[index] = val; }
  
  
  
  /* 
   * Returns true if the specified SOP holds a number-type value.
   */
  bool rho_sop_is_number (rho_sop *sop);
  
  /* 
   * Checks whether the specified SOP holds a number equal to the given one.
   */
  bool rho_sop_is_number_eq (rho_sop *sop, int num);
  
  
  
  /* 
   * Returns the coefficient part of the specified product, or null if there
   * is no coefficient (coefficient equal to 1).
   */
  rho_sop* rho_sop_get_coeff (rho_sop *sop);
  
  int rho_sop_get_coeff_sign (rho_sop *sop);
  
  
//------------------------------------------------------------------------------
}

#endif

