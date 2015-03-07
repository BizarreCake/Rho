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

#include "compiler/codegen.hpp"


namespace rho {
  
  code_generator::code_generator (byte_vector& buf)
    : buf (buf)
    { }
  
  
  
  /* 
   * Labels:
   */
//------------------------------------------------------------------------------
  
  /* 
   * Creates a new label and returns its identifier/index.
   */
  int
  code_generator::create_label ()
  {
    int index = this->lbls.size ();
    this->lbls.push_back ({
      .marked = false,
      .pos    = 0,
    });
    return index;
  }
  
  /* 
   * Sets the position of the specified label to the current position.
   */
  void
  code_generator::mark_label (int lbl)
  {
    this->lbls[lbl].marked = true;
    this->lbls[lbl].pos = this->buf.get_pos ();
  }
  
  /* 
   * Creates a new label and immediately marks it.
   */
  int
  code_generator::create_and_mark_label ()
  {
    int index = this->lbls.size ();
    this->lbls.push_back ({
      .marked = true,
      .pos    = this->buf.get_pos (),
    });
    return index;
  }
  
  
  /* 
   * Returns the position of a label.
   */
  unsigned int
  code_generator::get_label_pos (int lbl)
  {
    return this->lbls[lbl].pos;
  }
  
  
  
  /* 
   * Replaces label usage placeholders with the positions of proper locations
   * of the marked labels.
   */
  void
  code_generator::fix_labels ()
  {
    this->buf.push ();
    
    std::vector<label_use_t> nuses;
    for (auto& use : this->lbl_uses)
      {
        auto lbl = this->lbls[use.lbl];
        if (!lbl.marked)
          {
            nuses.push_back (use);
            continue;
          }
        
        int p = lbl.pos;
        int v = use.abs
          ? p
          : (p - (use.pos + use.size));
        
        this->buf.set_pos (use.pos);
        switch (use.size)
          {
          case 1:   this->buf.put_byte (v); break;
          case 2:   this->buf.put_short (v); break;
          case 4:   this->buf.put_int (v); break;
          }
      }
    
    this->lbl_uses = nuses;
    this->buf.pop ();
  }
  
//------------------------------------------------------------------------------
  
  
  
  /* 
   * Code emission routines:
   */
//------------------------------------------------------------------------------
  
  void
  code_generator::emit_push_int_32 (int num)
  {
    this->buf.put_byte (0x00);
    this->buf.put_int (num);
  }
  
  void
  code_generator::emit_push_int (const std::string& num)
  {
    this->buf.put_byte (0x01);
    this->buf.put_bytes ((unsigned char *)num.c_str (), num.length () + 1);
  }
  
  void
  code_generator::emit_pop ()
  {
    this->buf.put_byte (0x02);
  }
  
  void
  code_generator::emit_dup ()
  {
    this->buf.put_byte (0x03);
  }
  
  void
  code_generator::emit_push_nil ()
  {
    this->buf.put_byte (0x04);
  }
  
  void
  code_generator::emit_push_real (const std::string& num)
  {
    this->buf.put_byte (0x05);
    this->buf.put_bytes ((unsigned char *)num.c_str (), num.length () + 1);
  }
  
  void
  code_generator::emit_push_sym (const std::string& num)
  {
    this->buf.put_byte (0x06);
    this->buf.put_bytes ((unsigned char *)num.c_str (), num.length () + 1);
  }
  
  void
  code_generator::emit_push_empty_cons ()
  {
    this->buf.put_byte (0x07);
  }
  
  
  
  void
  code_generator::emit_push_frame (unsigned short locs)
  {
    this->buf.put_byte (0x10);
    this->buf.put_short (locs);
  }
  
  void 
  code_generator::emit_pop_frame ()
  {
    this->buf.put_byte (0x11);
  }
  
  void
  code_generator::emit_load_loc (unsigned char index)
  {
    this->buf.put_byte (0x12);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_store_loc (unsigned char index)
  {
    this->buf.put_byte (0x13);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_load_loc_p (unsigned char frm, unsigned char index)
  {
    this->buf.put_byte (0x14);
    this->buf.put_byte (frm);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_store_loc_p (unsigned char frm, unsigned char index)
  {
    this->buf.put_byte (0x15);
    this->buf.put_byte (frm);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_load_repl_var (unsigned char index)
  {
    this->buf.put_byte (0x16);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_store_repl_var (unsigned char index)
  {
    this->buf.put_byte (0x17);
    this->buf.put_byte (index);
  }
  
  
  
  void
  code_generator::emit_add ()
  {
    this->buf.put_byte (0x20);
  }
  
  void
  code_generator::emit_sub ()
  {
    this->buf.put_byte (0x21);
  }
  
  void
  code_generator::emit_mul ()
  {
    this->buf.put_byte (0x22);
  }
  
  void
  code_generator::emit_div ()
  {
    this->buf.put_byte (0x23);
  }
  
  void
  code_generator::emit_mod ()
  {
    this->buf.put_byte (0x24);
  }
  
  void
  code_generator::emit_pow ()
  {
    this->buf.put_byte (0x25);
  }
  
  void
  code_generator::emit_factorial ()
  {
    this->buf.put_byte (0x26);
  }
  
  void
  code_generator::emit_negate ()
  {
    this->buf.put_byte (0x27);
  }
  
  void
  code_generator::emit_cons ()
  {
    this->buf.put_byte (0x28);
  }
  
  void
  code_generator::emit_car ()
  {
    this->buf.put_byte (0x29);
  }
  
  void
  code_generator::emit_cdr ()
  {
    this->buf.put_byte (0x2A);
  }
  
  void
  code_generator::emit_idiv ()
  {
    this->buf.put_byte (0x2B);
  }
  
  void
  code_generator::emit_subst ()
  {
    this->buf.put_byte (0x2C);
  }
  
  
  
  
  void
  code_generator::emit_jmp (int lbl)
  {
    this->buf.put_byte (0x30);
    this->lbl_uses.push_back ({
      .lbl = lbl,
      .pos = this->buf.get_pos (),
      .abs = false,
      .size = 2,
    });
    this->buf.put_short (0);
  }
  
  void
  code_generator::emit_jt (int lbl)
  {
    this->buf.put_byte (0x31);
    this->lbl_uses.push_back ({
      .lbl = lbl,
      .pos = this->buf.get_pos (),
      .abs = false,
      .size = 2,
    });
    this->buf.put_short (0);
  }
  
  void
  code_generator::emit_jf (int lbl)
  {
    this->buf.put_byte (0x32);
    this->lbl_uses.push_back ({
      .lbl = lbl,
      .pos = this->buf.get_pos (),
      .abs = false,
      .size = 2,
    });
    this->buf.put_short (0);
  }
  
  
  
  void
  code_generator::emit_return ()
  {
    this->buf.put_byte (0x40);
  }
  
  void
  code_generator::emit_create_func (unsigned int code_size, int code_pos)
  {
    this->buf.put_byte (0x41);
    this->buf.put_int (code_size);
    this->lbl_uses.push_back ({
      .lbl = code_pos,
      .pos = this->buf.get_pos (),
      .abs = false,
      .size = 4,
    });
    this->buf.put_int (0);
  }
  
  void
  code_generator::emit_call (unsigned char arg_count)
  {
    this->buf.put_byte (0x42);
    this->buf.put_byte (arg_count);
  }
  
  void
  code_generator::emit_load_arg (unsigned char index)
  {
    this->buf.put_byte (0x43);
    this->buf.put_byte (index);
  }
  
  void
  code_generator::emit_store_arg (unsigned char index)
  {
    this->buf.put_byte (0x44);
    this->buf.put_byte (index);
  }
  
  
  
  void
  code_generator::emit_cmp_eq ()
  {
    this->buf.put_byte (0x50);
  }
  
  void
  code_generator::emit_cmp_ne ()
  {
    this->buf.put_byte (0x51);
  }
  
  void
  code_generator::emit_cmp_lt ()
  {
    this->buf.put_byte (0x52);
  }
  
  void
  code_generator::emit_cmp_le ()
  {
    this->buf.put_byte (0x53);
  }
  
  void
  code_generator::emit_cmp_gt ()
  {
    this->buf.put_byte (0x54);
  }
  
  void
  code_generator::emit_cmp_ge ()
  {
    this->buf.put_byte (0x55);
  }
  
  void
  code_generator::emit_cmp_cvg ()
  {
    this->buf.put_byte (0x56);
  }
  
  
  
  void
  code_generator::emit_push_microframe ()
  {
    this->buf.put_byte (0x60);
  }
  
  void
  code_generator::emit_pop_microframe ()
  {
    this->buf.put_byte (0x61);
  }
  
  void
  code_generator::emit_set_prec ()
  {
    this->buf.put_byte (0x62);
  }
  
  
  
  
  void
  code_generator::emit_exit ()
  {
    this->buf.put_byte (0xF0);
  }
}

