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

#ifndef _RHO__COMPILER__CODE_GENERATOR__H_
#define _RHO__COMPILER__CODE_GENERATOR__H_

#include "linker/module.hpp"
#include <vector>
#include <string>


namespace rho {
  
  /* 
   * Stores compile-time information about a relocation.
   */
  struct reloc_info
  {
    reloc_type type;
    int lbl;
    std::string mname;
    std::string val;
  };
  
  
  /* 
   * Provides routines to emit VM instructions.
   */
  class code_generator
  {
  private:
    struct label_info
    {
      bool marked;
      int pos;
    };
  
    struct label_fix
    {
      int lbl;
      int pos;
      int size; 
      bool abs; // true if absolute position
    };
  
  private:
    std::vector<unsigned char> buf;
    int pos;
    
    std::vector<label_info> lbls; 
    std::vector<label_fix> fixes;
    
    std::vector<reloc_info> rels;
    std::string rel_mname;
    reloc_type rel_type;
    std::string rel_val;
    
  public:
    inline unsigned char* data () { return this->buf.data (); }
    inline std::vector<unsigned char>::size_type size () const { return this->buf.size (); }
    
    inline const std::vector<reloc_info>& get_relocs () const { return this->rels; }
    
  private:
    void add_reloc (int lbl);
    
  public:
    code_generator ();
    
  public:
    void put_bytes (const void *data, int len);
    void put_byte (unsigned char v);
    void put_short (unsigned short v);
    void put_int (unsigned int v);
    void put_long (unsigned long long v);
    void put_float (float v);
    void put_double (double v);
    void put_cstr (const std::string& str);
    
    void put_label (int lbl, int size, bool abs);
    
    void seek (int pos);
    inline void seek_to_end () { this->seek (this->buf.size ()); }
    inline void seek_to_beg () { this->seek (0); }
    
    void clear ();
    
  public:
    /* 
     * Creates a new label and returns its ID.
     */
    int make_label ();
    
    /* 
     * Sets the position of the specified label to the current position.
     */
    void mark_label (int lbl);
    
    /* 
     * Convenience function - chains make_label() and mark_label() together.
     */
    int make_and_mark_label ();
    
    /* 
     * Fills in the positions of known labels.
     */
    void fix_labels ();
    
    /* 
     * Returns the absolute position of the specified label in the generated
     * code.
     */
    int get_label_pos (int lbl);
    
  public:
    void
    rel_set_mname (const std::string& name)
      { this->rel_mname = name; }
    
    void
    rel_set_type (reloc_type type)
      { this->rel_type = type; }
    
    void
    rel_set_val (const std::string& val)
      { this->rel_val = val; }
    
  public:
    void emit_nop ();
    void emit_push_int32 (int val);
    void emit_push_nil ();
    void emit_dup_n (int off);
    void emit_dup ();
    void emit_pop ();
    void emit_swap ();
    void emit_pop_n (unsigned char count);
    
    void emit_add ();
    void emit_sub ();
    void emit_mul ();
    void emit_div ();
    void emit_pow ();
    void emit_mod ();
    void emit_and ();
    void emit_or ();
    void emit_not ();
    
    void emit_mk_fn (int lbl);
    void emit_call (unsigned char argc);
    void emit_ret ();
    void emit_mk_closure (unsigned char upvalc, int lbl);
    void emit_get_free (int idx);
    void emit_set_free (int idx);
    void emit_get_arg (int idx);
    void emit_set_arg (int idx);
    void emit_get_local (int idx);
    void emit_set_local (int idx);
    void emit_tail_call ();
    void emit_get_fun ();
    void emit_close (unsigned char localc);
    void emit_call0 (unsigned char argc);
    
    void emit_cmp_eq ();
    void emit_cmp_neq ();
    void emit_cmp_lt ();
    void emit_cmp_lte ();
    void emit_cmp_gt ();
    void emit_cmp_gte ();
    void emit_cmp_eq_many (int count);
    
    void emit_jmp (int lbl);
    void emit_jt (int lbl);
    void emit_jf (int lbl);
    
    void emit_push_empty_list ();
    void emit_cons ();
    void emit_car ();
    void emit_cdr ();
    
    void emit_push_pvar (int pv);
    void emit_match (int loff);
    
    void emit_call_builtin (int index, unsigned char argc);
    
    void emit_push_sint (unsigned short val);
    void emit_push_nils (unsigned char count);
    void emit_push_true ();
    void emit_push_false ();
    void emit_push_atom (int val, bool emit_reloc = true);
    void emit_push_cstr (const std::string& str);
    void emit_push_float (double val);
    
    void emit_mk_vec (unsigned short count);
    void emit_vec_get_hard (unsigned short index);
    void emit_vec_get ();
    void emit_vec_set ();
    
    void emit_alloc_globals (unsigned short page, unsigned short count, bool emit_reloc = true);
    void emit_get_global (unsigned short page, unsigned short idx, bool emit_reloc = true);
    void emit_set_global (unsigned short page, unsigned short idx, bool emit_reloc = true);
    void emit_def_atom (int val, const std::string& name, bool emit_reloc = true);
    
    void emit_push_microframe ();
    void emit_pop_microframe ();
    
    void emit_breakpoint (int bp);
    void emit_exit ();
  };
}

#endif

