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

#include "compiler/code_generator.hpp"
#include <cstring>


namespace rho {
  
  code_generator::code_generator ()
  {
    this->pos = 0;
  }
  
  
  
  void
  code_generator::add_reloc (int lbl)
  {
    reloc_info rel;
    rel.lbl = lbl;
    rel.mname = this->rel_mname;
    rel.type = this->rel_type;
    rel.val = this->rel_val;
    
    this->rels.push_back (rel);
    
    this->rel_val.clear ();
    this->rel_mname.clear ();
    this->rel_type = REL_GP;
  }
  
  
  
  void
  code_generator::put_bytes (const void *data, int len)
  {
    int init = this->buf.size () - this->pos;
    if (init > len)
      init = len;
    int rem = len - init;
    
    std::memcpy (this->buf.data () + this->pos, data, init);
    
    const unsigned char *ptr = (const unsigned char *)data + init;
    for (int i = 0; i < rem; ++i)
      this->buf.push_back (ptr[i]);
    
    this->pos += len;
  }
  
  void
  code_generator::put_byte (unsigned char v)
  {
    this->put_bytes (&v, 1);
  }
  
  void
  code_generator::put_short (unsigned short v)
  {
    this->put_bytes (&v, 2);
  }
  
  void
  code_generator::put_int (unsigned int v)
  {
    this->put_bytes (&v, 4);
  }
  
  void
  code_generator::put_long (unsigned long long v)
  {
    this->put_bytes (&v, 8);
  }
  
  void
  code_generator::put_float (float v)
  {
    this->put_bytes (&v, 4);
  }
  
  void
  code_generator::put_double (double v)
  {
    this->put_bytes (&v, 8);
  }
  
  void
  code_generator::put_cstr (const std::string& str)
  {
    this->put_bytes (str.c_str (), str.length () + 1);
  }
  
  
  void
  code_generator::put_label (int lbl, int size, bool abs)
  {
    auto& inf = this->lbls[lbl];
    if (inf.marked)
      {
        switch (size)
          {
          case 1:
            if (abs)
              this->put_byte (inf.pos);
            else
              this->put_byte (inf.pos - this->pos - size);
            break;
          
          case 2:
            if (abs)
              this->put_short (inf.pos);
            else
              this->put_short (inf.pos - this->pos - size);
            break;
          
          case 4:
            if (abs)
              this->put_int (inf.pos);
            else
              this->put_int (inf.pos - this->pos - size);
            break;
          }
      }
    else
      {
        this->fixes.push_back ({ lbl, this->pos, size, abs });
        switch (size)
          {
          case 1: this->put_byte (0); break;
          case 2: this->put_short (0); break;
          case 4: this->put_int (0); break;
          }
      }
  }
  
  
  
  void
  code_generator::seek (int pos)
  {
    this->pos = pos;
  }
  
  
  
  void
  code_generator::clear ()
  {
    this->rels.clear ();
    this->buf.clear ();
    this->pos = 0;
  }
  
  
  
  /* 
   * Creates a new label and returns its ID.
   */
  int
  code_generator::make_label ()
  {
    int lbl = this->lbls.size ();
    this->lbls.push_back ({ false, -1 });
    return lbl;
  }
  
  /* 
   * Sets the position of the specified label to the current position.
   */
  void
  code_generator::mark_label (int lbl)
  {
    auto& inf = this->lbls[lbl];
    inf.marked = true;
    inf.pos = this->pos;
  }
  
  /* 
   * Convenience function - chains make_label() and mark_label() together.
   */
  int
  code_generator::make_and_mark_label ()
  {
    int lbl = this->make_label ();
    this->mark_label (lbl);
    return lbl;
  }
  
  /* 
   * Fills in the positions of known labels.
   */
  void
  code_generator::fix_labels ()
  {
    int ppos = this->pos;
    
    for (auto itr = this->fixes.begin (); itr != this->fixes.end (); )
      {
        auto& fix = *itr; 
        auto& inf = this->lbls[fix.lbl];
        if (!inf.marked)
          ++ itr;
        else
          {
            this->pos = fix.pos;
            switch (fix.size)
              {
              case 1:
                if (fix.abs)
                  this->put_byte (inf.pos);
                else
                  this->put_byte (inf.pos - fix.pos - 1);
                break;
              
              case 2:
                if (fix.abs)
                  this->put_short (inf.pos);
                else
                  this->put_short (inf.pos - fix.pos - 2);
                break;
              
              case 4:
                if (fix.abs)
                  this->put_int (inf.pos);
                else
                  this->put_int (inf.pos - fix.pos - 4);
                break;
              }
            
            itr = this->fixes.erase (itr);
          }
      }
    
    this->pos = ppos;
  }
  
  /* 
   * Returns the absolute position of the specified label in the generated
   * code.
   */
  int
  code_generator::get_label_pos (int lbl)
  {
    return this->lbls[lbl].pos;
  }
  
  
  
  void
  code_generator::emit_nop ()
  {
    this->put_byte (0x00);
  }
  
  void
  code_generator::emit_push_int32 (int val)
  {
    this->put_byte (0x01);
    this->put_int (val);
  }
  
  void
  code_generator::emit_push_nil ()
  {
    this->put_byte (0x02);
  }
  
  void
  code_generator::emit_dup_n (int off)
  {
    this->put_byte (0x0B);
    this->put_int (off);
  }
  
  void
  code_generator::emit_dup ()
  {
    this->put_byte (0x0C);
  }
  
  void
  code_generator::emit_pop ()
  {
    this->put_byte (0x0D);
  }
  
  void
  code_generator::emit_swap ()
  {
    this->put_byte (0x0E);
  }
  
  void
  code_generator::emit_pop_n (unsigned char count)
  {
    this->put_byte (0x0F);
  }
  
  
  void
  code_generator::emit_add ()
  {
    this->put_byte (0x10);
  }
  
  void
  code_generator::emit_sub ()
  {
    this->put_byte (0x11);
  }
  
  void
  code_generator::emit_mul ()
  {
    this->put_byte (0x12);
  }
  
  void
  code_generator::emit_div ()
  {
    this->put_byte (0x13);
  }
  
  void
  code_generator::emit_pow ()
  {
    this->put_byte (0x14);
  }
  
  void
  code_generator::emit_mod ()
  {
    this->put_byte (0x15);
  }
  
  void
  code_generator::emit_and ()
  {
    this->put_byte (0x16);
  }
  
  void
  code_generator::emit_or ()
  {
    this->put_byte (0x17);
  }
  
  void
  code_generator::emit_not ()
  {
    this->put_byte (0x18);
  }
  
  
  
  void
  code_generator::emit_get_arg_pack ()
  {
    this->put_byte (0x20);
  }
  
  void
  code_generator::emit_mk_fn (int lbl)
  {
    this->put_byte (0x21);
    this->put_label (lbl, 4, false);
  }
  
  void
  code_generator::emit_call (unsigned char argc)
  {
    this->put_byte (0x22);
    this->put_byte (argc);
  }
  
  void
  code_generator::emit_ret ()
  {
    this->put_byte (0x23);
  }
  
  void
  code_generator::emit_mk_closure (unsigned char upvalc, int lbl)
  {
    this->put_byte (0x24);
    this->put_byte (upvalc);
    this->put_label (lbl, 4, false);
  }
  
  void
  code_generator::emit_get_free (int idx)
  {
    this->put_byte (0x25);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_set_free (int idx)
  {
    this->put_byte (0x2A);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_get_arg (int idx)
  {
    this->put_byte (0x26);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_set_arg (int idx)
  {
    this->put_byte (0x27);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_get_local (int idx)
  {
    this->put_byte (0x28);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_set_local (int idx)
  {
    this->put_byte (0x29);
    this->put_byte (idx);
  }
  
  void
  code_generator::emit_tail_call ()
  {
    this->put_byte (0x2B);
  }
  
  void
  code_generator::emit_get_fun ()
  {
    this->put_byte (0x2C);
  }
  
  void
  code_generator::emit_close (unsigned char localc)
  {
    this->put_byte (0x2D);
    this->put_byte (localc);
  }
  
  void
  code_generator::emit_call0 (unsigned char argc)
  {
    this->put_byte (0x2E);
    this->put_byte (argc);
  }
  
  void
  code_generator::emit_pack_args (unsigned char start)
  {
    this->put_byte (0x2F);
    this->put_byte (start);
  }
  
  
  
  void
  code_generator::emit_cmp_eq ()
  {
    this->put_byte (0x30);
  }
  
  void
  code_generator::emit_cmp_neq ()
  {
    this->put_byte (0x31);
  }
  
  void
  code_generator::emit_cmp_lt ()
  {
    this->put_byte (0x32);
  }
  
  void
  code_generator::emit_cmp_lte ()
  {
    this->put_byte (0x33);
  }
  
  void
  code_generator::emit_cmp_gt ()
  {
    this->put_byte (0x34);
  }
  
  void
  code_generator::emit_cmp_gte ()
  {
    this->put_byte (0x35);
  }
  
  void
  code_generator::emit_cmp_eq_many (int count)
  {
    this->put_byte (0x36);
    this->put_int (count);
  }
  
  
  
  void
  code_generator::emit_jmp (int lbl)
  {
    this->put_byte (0x40);
    this->put_label (lbl, 4, false);
  }
  
  void
  code_generator::emit_jt (int lbl)
  {
    this->put_byte (0x41);
    this->put_label (lbl, 4, false);
  }
  
  void
  code_generator::emit_jf (int lbl)
  {
    this->put_byte (0x42);
    this->put_label (lbl, 4, false);
  }
  
  
  
  void
  code_generator::emit_push_empty_list ()
  {
    this->put_byte (0x50);
  }
  
  void
  code_generator::emit_cons ()
  {
    this->put_byte (0x51);
  }
  
  void
  code_generator::emit_car ()
  {
    this->put_byte (0x52);
  }
  
  void 
  code_generator::emit_cdr ()
  {
    this->put_byte (0x53);
  }
  
  
  
  void 
  code_generator::emit_push_pvar (int pv)
  {
    this->put_byte (0x60);
    this->put_int (pv);
  }
  
  void 
  code_generator::emit_match (int loff)
  {
    this->put_byte (0x61);
    this->put_int (loff);
  }
  
  
  
  void
  code_generator::emit_call_builtin (int index, unsigned char argc)
  {
    this->put_byte (0x70);
    this->put_short (index);
    this->put_byte (argc);
  }
  
  
  
  void
  code_generator::emit_push_sint (unsigned short val)
  {
    this->put_byte (0x80);
    this->put_short (val);
  }
  
  void
  code_generator::emit_push_nils (unsigned char count)
  {
    this->put_byte (0x81);
    this->put_byte (count);
  }
  
  void
  code_generator::emit_push_true ()
  {
    this->put_byte (0x82);
  }
  
  void
  code_generator::emit_push_false ()
  {
    this->put_byte (0x83);
  }
  
  void
  code_generator::emit_push_atom (int val, bool emit_reloc)
  {
    this->put_byte (0x84);
    int lbl = this->make_and_mark_label ();
    this->put_int (val);
    
    if (emit_reloc)
      this->add_reloc (lbl);
  }
  
  void
  code_generator::emit_push_cstr (const std::string& str)
  {
    this->put_byte (0x85);
    this->put_cstr (str);
  }
  
  void
  code_generator::emit_push_float (double val)
  {
    this->put_byte (0x86);
    this->put_double (val);
  }
  
  
  
  void
  code_generator::emit_mk_vec (unsigned short count)
  {
    this->put_byte (0x90);
    this->put_short (count);
  }
  
  void
  code_generator::emit_vec_get_hard (unsigned short index)
  {
    this->put_byte (0x91);
    this->put_short (index);
  }
  
  void
  code_generator::emit_vec_get ()
  {
    this->put_byte (0x92);
  }
  void
  code_generator::emit_vec_set ()
  {
    this->put_byte (0x93);
  }
  
  
  
  void
  code_generator::emit_alloc_globals (unsigned short page, unsigned short count,
                                      bool emit_reloc)
  {
    this->put_byte (0xA0);
    int lbl = this->make_and_mark_label ();
    this->put_short (page);
    this->put_short (count);
    
    if (emit_reloc)
      this->add_reloc (lbl);
  }
  
  void
  code_generator::emit_get_global (unsigned short page, unsigned short idx,
                                   bool emit_reloc)
  {
    this->put_byte (0xA1);
    int lbl = this->make_and_mark_label ();
    this->put_short (page);
    this->put_short (idx);
    
    if (emit_reloc)
      this->add_reloc (lbl);
  }
  
  void
  code_generator::emit_set_global (unsigned short page, unsigned short idx,
                                   bool emit_reloc)
  {
    this->put_byte (0xA2);
    int lbl = this->make_and_mark_label ();
    this->put_short (page);
    this->put_short (idx);
    
    if (emit_reloc)
      this->add_reloc (lbl);
  }
  
  void
  code_generator::emit_def_atom (int val, const std::string& name,
                                 bool emit_reloc)
  {
    this->put_byte (0xA3);
    int lbl = this->make_and_mark_label ();
    this->put_int (val);
    this->put_cstr (name);
    
    if (emit_reloc)
      this->add_reloc (lbl);
  }
  
  
  
  void
  code_generator::emit_push_microframe ()
  {
    this->put_byte (0xB0);
  }
  
  void
  code_generator::emit_pop_microframe ()
  {
    this->put_byte (0xB1);
  }
  
  
  
  void
  code_generator::emit_breakpoint (int bp)
  {
    this->put_byte (0xF0);
    this->put_int (bp);
  }
  
  void
  code_generator::emit_exit ()
  {
    this->put_byte (0xFF);
  }
}

