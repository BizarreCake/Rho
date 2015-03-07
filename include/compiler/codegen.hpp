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

#ifndef _RHO__COMPILER__CODE_GENERATOR__H_
#define _RHO__COMPILER__CODE_GENERATOR__H_

#include "common/byte_vector.hpp"
#include <string>


namespace rho {
  
  /* 
   * The code generator wraps on top of a byte vector and provides methods to
   * emit bytecode instructions.
   */
  class code_generator
  {
  private:
    // label information
    struct label_t {
      // true if the label's position is known.
      bool marked;
      
      // the position of the label within the byte vector.
      unsigned int pos;
    };
    
    // represents the usage of a label (e.g. a jump instruction)
    struct label_use_t {
      // the label being used.
      int lbl;
      
      // the position of the use within the code.
      unsigned int pos;
      
      // true if an absolute address should be emitted.
      bool abs;
      
      // the size of the offset.
      int size;
    };
  
  private:
    byte_vector& buf;
    
    std::vector<label_t> lbls;
    std::vector<label_use_t> lbl_uses;
    
  public:
    inline unsigned int get_pos () const { return this->buf.get_pos (); }
    
  public:
    code_generator (byte_vector& buf);
    
  public:
    /* 
     * Labels:
     */
//------------------------------------------------------------------------------
    /* 
     * Creates a new label and returns its identifier/index.
     */
    int create_label ();
    
    /* 
     * Sets the position of the specified label to the current position.
     */
    void mark_label (int lbl);
    
    /* 
     * Creates a new label and immediately marks it.
     */
    int create_and_mark_label ();
    
    
    
    /* 
     * Returns the position of a label.
     */
    unsigned int get_label_pos (int lbl);
    
    
    
    /* 
     * Replaces label usage placeholders with the positions of proper locations
     * of the marked labels.
     */
    void fix_labels ();
//------------------------------------------------------------------------------
    
  public:
    /* 
     * Code emission routines:
     */
//------------------------------------------------------------------------------
    void emit_push_int_32 (int num);
    void emit_push_int (const std::string& num);
    void emit_pop ();
    void emit_dup ();
    void emit_push_nil ();
    void emit_push_real (const std::string& num);
    void emit_push_sym (const std::string& num);
    void emit_push_empty_cons ();
    
    void emit_push_frame (unsigned short locs);
    void emit_pop_frame ();
    void emit_load_loc (unsigned char index);
    void emit_store_loc (unsigned char index);
    void emit_load_loc_p (unsigned char frm, unsigned char index);
    void emit_store_loc_p (unsigned char frm, unsigned char index);
    void emit_load_repl_var (unsigned char index);
    void emit_store_repl_var (unsigned char index);
    
    void emit_add ();
    void emit_sub ();
    void emit_mul ();
    void emit_div ();
    void emit_idiv ();
    void emit_mod ();
    void emit_pow ();
    void emit_factorial ();
    void emit_negate ();
    void emit_cons ();
    void emit_car ();
    void emit_cdr ();
    void emit_subst ();
    
    void emit_jmp (int lbl);
    void emit_jt (int lbl);
    void emit_jf (int lbl);
    
    void emit_return ();
    void emit_create_func (unsigned int code_size, int code_pos);
    void emit_call (unsigned char arg_count);
    void emit_load_arg (unsigned char index);
    void emit_store_arg (unsigned char index);
    
    void emit_cmp_eq ();
    void emit_cmp_ne ();
    void emit_cmp_lt ();
    void emit_cmp_le ();
    void emit_cmp_gt ();
    void emit_cmp_ge ();
    void emit_cmp_cvg ();
    
    void emit_push_microframe ();
    void emit_pop_microframe ();
    void emit_set_prec ();
    
    void emit_exit ();
//------------------------------------------------------------------------------
  };
}

#endif

