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

#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"
#include "runtime/builtins.hpp"
#include "util/float.hpp"
#include <cstring>

#include <iostream> // DEBUG


namespace rho {
  
  virtual_machine::virtual_machine (int stack_size, const char *gc_name)
  {
    this->stack = new rho_value [stack_size];
    this->sp = 0;
    this->bp = 0;
    
    this->gc = garbage_collector::create (gc_name, *this);
    
    this->ints = new rho_value [VM_SMALL_INT_MAX + 1];
    for (int i = 0; i <= VM_SMALL_INT_MAX; ++i)
      //this->ints[i] = rho_value_make_nil ();
      this->ints[i] = rho_value_make_int (i, *this->gc);
  }
  
  virtual_machine::~virtual_machine ()
  {
    this->reset ();
    
    for (auto& gp : this->gpages)
      for (int i = 0; i < gp.size; ++i)
        gp.vals[i].type = RHO_NIL;
    
    for (int i = 0; i <= VM_SMALL_INT_MAX; ++i)
      gc_unprotect (this->ints[i]);
    
    this->gc->collect ();
    
    delete this->gc;
    delete[] this->stack;
    delete[] this->ints;
    
    for (auto& gp : this->gpages)
      delete[] gp.vals;
  }
  
  
  
  /* 
   * Returns an object through which stack values can be retreived.
   */
  stack_provider
  virtual_machine::get_stack (bool refs_only)
  {
    return stack_provider (this->stack, this->sp, refs_only);
  }
  
  
  
  int
  virtual_machine::get_base10_prec () const
  {
    int start = GET_INTERNAL (stack[bp + 4]);
    return GET_INTERNAL(stack[start + 2]);
  }
  
  
  
  /* 
   * Executes the specified Rho program.
   * Returns the top-most value in the VM's stack on completion.
   */
  rho_value
  virtual_machine::run (program& prg)
  {
    auto code = prg.get_code ();
    auto ptr = code;
    
    for (;;)
      {
        switch (*ptr++)
          {
        //----------------------------------------------------------------------
        // stack manipulation
        //----------------------------------------------------------------------
          
          // nop
          case 0x00:
            break;
          
          // push_int32
          case 0x01:
            {
              auto v = rho_value_make_int (*(int *)ptr, *this->gc);
              ptr += 4;
              
              stack[sp ++] = v;
              gc_unprotect (v);
            }
            break;
            
          // push_nil
          case 0x02:
            stack[sp ++] = rho_value_make_nil ();
            break;
            
          // dup_n
          case 0x0B:
            {
              int index = *(int *)ptr;
              ptr += 4;
              
              stack[sp] = stack[sp - index];
              ++ sp;
            }
            break;
            
          // dup
          case 0x0C:
            stack[sp] = stack[sp - 1];
            ++ sp;
            break;
            
          // pop
          case 0x0D:
            -- sp;
            break;
          
          // swap
          case 0x0E:
            {
              auto t = stack[sp - 1];
              stack[sp - 1] = stack[sp - 2];
              stack[sp - 2] = t;
            }
            break;
          
          // pop
          case 0x0F:
            sp -= (unsigned char)*ptr++;
            break;
            
            
        //----------------------------------------------------------------------
        // basic arithmetic
        //----------------------------------------------------------------------
            
          // add
          case 0x10:
            stack[sp - 2] = rho_value_add (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // sub
          case 0x11:
            stack[sp - 2] = rho_value_sub (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // mul
          case 0x12:
            stack[sp - 2] = rho_value_mul (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // div
          case 0x13:
            stack[sp - 2] = rho_value_div (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // pow
          case 0x14:
            stack[sp - 2] = rho_value_pow (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // mod
          case 0x15:
            stack[sp - 2] = rho_value_mod (stack[sp - 2], stack[sp - 1], *this);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // and
          case 0x16:
            stack[sp - 2] = rho_value_make_bool (
              !(rho_value_cmp_zero (stack[sp - 2]) || rho_value_cmp_zero (stack[sp - 1])));
            -- sp;
            break;
          
          // or
          case 0x17:
            stack[sp - 2] = rho_value_make_bool (
              !(rho_value_cmp_zero (stack[sp - 2]) && rho_value_cmp_zero (stack[sp - 1])));
            -- sp;
            break;
          
          // not
          case 0x18:
            stack[sp - 1] = rho_value_make_bool (rho_value_cmp_zero (stack[sp - 1]));
            break;
          
          
          
        //----------------------------------------------------------------------
        // functions & closures
        //----------------------------------------------------------------------
          
          // get_arg_pack
          case 0x20:
            stack[sp ++] = stack[bp + 5];
            break;
          
          // mk_fn
          case 0x21:
            {
              const unsigned char *cp = ptr + 4 + *(int *)ptr;
              ptr += 4;
              
              stack[sp++] = rho_value_make_function (cp, 0, *this->gc);
              gc_unprotect (stack[sp - 1]);
            }
            break;
          
          // call
          case 0x22:
            {
              auto cl = stack[sp - 1];
              
              // push previous bp
              int pbp = bp;
              bp = sp;
              stack[sp ++] = MK_INTERNAL (pbp);
              
              // push return address
              stack[sp ++] = MK_INTERNAL (ptr + 1);
              
              // push env
              stack[sp ++] = cl;
              
              // push argument count
              stack[sp ++] = MK_INTERNAL ((unsigned char)*ptr);
              
              // push microframe pointer
              stack[sp ++] = MK_INTERNAL (GET_INTERNAL (stack[pbp + 4]));
              
              // push slot for argument pack
              stack[sp ++] = rho_value_make_nil ();
              
              ptr = cl.val.gc->val.fn.cp;
            }
            break;
          
          // ret
          case 0x23:
            {
              auto retv = stack[sp - 1];
              int argc = (int)GET_INTERNAL (stack[bp + 3]);
              
              ptr = (const unsigned char *)GET_INTERNAL (stack[bp + 1]);
              int pbp = bp;
              bp = (int)GET_INTERNAL (stack[bp]);
              sp = pbp;
              
              sp -= argc;
              stack[sp - 1] = retv;
            }
            break;
          
          // mk_closure
          case 0x24:
            {
              unsigned char upvalc = *ptr++;
              
              const unsigned char *cp = ptr + 4 + *(int *)ptr;
              ptr += 4;
              
              auto penv = stack[bp + 2].val.gc->val.fn;
              auto fn = rho_value_make_function (cp, penv.env_len + upvalc, *this->gc);
              stack[sp++] = fn;
              
              // insert upvalues
              auto env  = fn.val.gc->val.fn;
              for (int i = 0; i < penv.env_len; ++i)
                env.env[i] = penv.env[i];
              for (int i = 0; i < upvalc; ++i)
                {
                  int idx = -1;
                  switch (*ptr++)
                    {
                    // get_arg_pack
                    case 0x20:
                      idx = bp + 5;
                      break;
                    
                    // get_arg
                    case 0x26:
                      {
                        unsigned char index = *ptr++;
                        idx = bp - 2 - index;
                      }
                      break;
                    
                    // get_local
                    case 0x28:
                      {
                        unsigned char index = *ptr++;
                        idx = bp + 6 + index;
                      }
                      break;
                    
                    default:
                      throw vm_error ("a sequence of get_arg/get_local's should follow mk_closure");
                    }
                  
                  auto& target = env.env[i + penv.env_len];
                  
                  bool found = false;
                  for (auto uv_ : this->gc->get_upvalues ())
                    {
                      auto& uv = uv_->val.uv;
                      if (uv.sp == idx)
                        {
                          target.type = RHO_UPVAL;
                          target.val.gc = uv_;
                          found = true;
                          break;
                        }
                    }
                  
                  if (!found)
                    {
                      target = rho_value_make_upvalue (*this->gc);
                      target.val.gc->val.uv.sp = idx;
                      gc_unprotect (target);
                    }
                }
              
              gc_unprotect (fn);
            }
            break;
          
          // get_free
          case 0x25:
            {
              unsigned char index = *ptr++;
              auto& upv = stack[bp + 2].val.gc->val.fn.env[index];
              if (upv.val.gc->val.uv.sp == -1)
                stack[sp ++] = upv.val.gc->val.uv.val;
              else
                stack[sp ++] = stack[upv.val.gc->val.uv.sp];
            }
          break;
          
          // get_arg
          case 0x26:
            {
              unsigned char index = *ptr++;
              stack[sp ++] = stack[bp - 2 - index];
            }
            break;
          
          // set_arg
          case 0x27:
            {
              unsigned char index = *ptr++;
              stack[bp - 2 - index] = stack[-- sp];
            }
            break;
          
          // get_local
          case 0x28:
            {
              unsigned char index = *ptr++;
              stack[sp ++] = stack[bp + 6 + index];
            }
            break;
          
          // set_local
          case 0x29:
            {
              unsigned char index = *ptr++;
              stack[bp + 6 + index] = stack[-- sp];
            }
            break;
          
          // set_free
          case 0x2A:
            {
              unsigned char index = *ptr++;
              auto& upv = stack[bp + 2].val.gc->val.fn.env[index];
              if (upv.val.gc->val.uv.sp == -1)
                upv.val.gc->val.uv.val = stack[-- sp];
              else
                stack[upv.val.gc->val.uv.sp] = stack[-- sp];
            }
          break;
          
          // tail_call
          case 0x2B:
            {
              auto cl = stack[-- sp];
              stack[bp + 2] = cl;
              
              unsigned char argc = GET_INTERNAL (stack[bp + 3]);
              for (int i = 0; i < argc; ++i)
                stack[bp - 2 - i] = stack[sp - 1 - i];
              
              sp = bp + 6;
              ptr = cl.val.gc->val.fn.cp;
            }
            break;
          
          // get_fun
          case 0x2C:
            stack[sp++] = stack[bp + 2];
            break;
          
          // close
          case 0x2D:
            {
              unsigned char local_count = *ptr++;
              int argc = (int)GET_INTERNAL (stack[bp + 3]);
              
              auto& upvals = this->gc->get_upvalues ();
              for (auto uv_ : upvals)
                {
                  auto& uv = uv_->val.uv;
                  
                  // we start from bp + 5 because the argument pack
                  // (if it exists) is stored there.
                  if ((uv.sp >= bp + 5 && uv.sp < bp + 6 + local_count)
                    || (uv.sp > bp - 2 - argc && uv.sp <= bp - 2))
                    {
                      uv.val = stack[uv.sp];
                      uv.sp = -1;
                    }
                }
            }
            break;
          
         // call0
         case 0x2E:
          {
            auto cl = stack[sp - 1];
              
            // push previous bp
            int pbp = bp;
            bp = sp;
            stack[sp ++] = MK_INTERNAL (pbp);
            
            // push return address
            stack[sp ++] = MK_INTERNAL (ptr + 1);
            
            // push env
            stack[sp ++] = cl;
            
            // push argument count
            stack[sp ++] = MK_INTERNAL ((unsigned char)*ptr);
            
            // push microframe pointer
            stack[sp ++] = MK_INTERNAL (0);
            
            // push slot for argument pack
            stack[sp ++] = rho_value_make_nil ();
            
            ptr = cl.val.gc->val.fn.cp;
          }
          break;
        
        // pack_args
        case 0x2F:
          {
            unsigned char start = *ptr++;
            int argc = (int)GET_INTERNAL (stack[bp + 3]);
            
            rho_value vec = rho_value_make_vec (argc - start, *this->gc);
            auto& vec_data = vec.val.gc->val.vec;
            for (int i = start; i < argc; ++i)
              vec_data.vals[i - start] = stack[bp - 2 - i];
            vec_data.len = argc - start;
            
            stack[bp + 5] = vec;
            gc_unprotect (vec);
          }
          break;
         
          
        
        //----------------------------------------------------------------------
        // comparisons
        //----------------------------------------------------------------------
          
          // cmp_eq
          case 0x30:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_eq (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_neq
          case 0x31:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_neq (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_lt
          case 0x32:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_lt (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_lte
          case 0x33:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_lte (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_gt
          case 0x34:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_gt (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_gte
          case 0x35:
            stack[sp - 2] = rho_value_make_bool (
              rho_value_cmp_gte (stack[sp - 2], stack[sp - 1]));
            -- sp;
            break;
          
          // cmp_eq_many
          case 0x36:
            {
              int count = *(int *)ptr;
              ptr += 4;
              
              bool eq = true;
              auto fst = stack[sp - count];
              for (int i = 1; i < count; ++i)
                if (!rho_value_cmp_eq (fst, stack[sp - count + i]))
                  {
                    eq = false;
                    break;
                  }
              
              sp -= count;
              stack[sp] = rho_value_make_bool (eq);
              ++ sp;
            }
            break;
          
        
        //----------------------------------------------------------------------
        // jumps
        //----------------------------------------------------------------------
            
          // jmp
          case 0x40:
            ptr += 4 + *(int *)ptr;
            break;
            
          // jt
          case 0x41:
            if (!rho_value_cmp_zero (stack[-- sp]))
              ptr += 4 + *(int *)ptr;
            else
              ptr += 4;
            break;
          
          // jf
          case 0x42:
            if (rho_value_cmp_zero (stack[-- sp]))
              ptr += 4 + *(int *)ptr;
            else
              ptr += 4;
            break;
            
            
          
        //----------------------------------------------------------------------
        // lists
        //----------------------------------------------------------------------
          
          // push_empty_list
          case 0x50:
            stack[sp] = rho_value_make_empty_list (*this->gc);
            gc_unprotect (stack[sp]);
            ++ sp;
            break;
          
          // cons
          case 0x51:
            stack[sp - 2] = rho_value_make_cons (stack[sp - 2], stack[sp - 1],
              *this->gc);
            -- sp;
            gc_unprotect (stack[sp - 1]);
            break;
          
          // car
          case 0x52:
            stack[sp - 1] = stack[sp - 1].val.gc->val.p.fst;
            break;
          
          // cdr
          case 0x53:
            stack[sp - 1] = stack[sp - 1].val.gc->val.p.snd;
            break;
        
        
        
          
          
          
        //----------------------------------------------------------------------
        // pattern matching
        //----------------------------------------------------------------------
          
        // push_pvar
        case 0x60:
          {
            int pv = *(int *)ptr;
            ptr += 4;
            stack[sp] = rho_value_make_pvar (pv);
            ++ sp;
            gc_unprotect (stack[sp - 1]);
          }
          break;
          
        // match
        case 0x61:
          {
            int loff = *(int *)ptr;
            ptr += 4;
            
            auto res = rho_value_match (stack[sp - 1], stack[sp - 2],
              stack + bp + 6 + loff);
            -- sp;
            stack[sp - 1] = rho_value_make_bool (res);
          }
          break;
          
          
          
        //----------------------------------------------------------------------
        // builtin functions
        //----------------------------------------------------------------------
          
          // builtin
          case 0x70:
            {
              unsigned short index = *(unsigned short *)ptr;
              ptr += 2;
              unsigned char argc = *ptr++;
              
              switch (index)
                {
                // print:
                case 0:
                  stack[sp - argc] = rho_builtin_print (stack[sp - 1], *this);
                  ++ sp;
                  break;
                
                // len:
                case 1:
                  stack[sp - argc] = rho_builtin_len (stack[sp - 1], *this);
                  ++ sp;
                  break;
                }
              
              sp -= argc;
            }
            break;
          
          
          
        //----------------------------------------------------------------------
        // more stack instructions
        //----------------------------------------------------------------------
          
          // push_sint
          case 0x80:
            stack[sp++] = this->ints[*((unsigned short *)ptr)];
            ptr += 2;
            break;
          
          // push_nils
          case 0x81:
            {
              unsigned char count = *ptr++;
              while (count --> 0)
                stack[sp ++] = rho_value_make_nil ();
            }
            break;
          
          // push_true
          case 0x82:
            stack[sp ++] = rho_value_make_bool (true);
            break;
          
          // push_false
          case 0x83:
            stack[sp ++] = rho_value_make_bool (false);
            break;
          
          // push_atom
          case 0x84:
            stack[sp ++] = rho_value_make_atom (*(int *)ptr);
            ptr += 4;
            break;
          
          // push_cstr
          case 0x85:
            {
              int len = std::strlen ((char *)ptr);
              stack[sp] = rho_value_make_string ((char *)ptr, len, *this->gc);
              ++ sp;
              gc_unprotect (stack[sp - 1]);
              ptr += len + 1;
            }
            break;
          
          // push_float
          case 0x86:
            {
              int mf = GET_INTERNAL (stack[bp + 4]);
              unsigned int prec = GET_INTERNAL(stack[mf + 1]);
              
              auto v = rho_value_make_float (*(double *)ptr, prec, *this->gc);
              ptr += 8;
              
              stack[sp ++] = v;
              gc_unprotect (v);
            }
            break;
        
        
        //----------------------------------------------------------------------
        // vectors
        //----------------------------------------------------------------------
          
          // mk_vec
          case 0x90:
            {
              int len = *((unsigned short *)ptr);
              ptr += 2;
              
              auto v = rho_value_make_vec (len * 12 / 10, *this->gc);
              auto& vec = v.val.gc->val.vec;
              for (int i = 0; i < len; ++i)
                vec.vals[i] = stack[sp - 1 - i];
              vec.len = len;
              sp -= len;
              
              stack[sp ++] = v;
              gc_unprotect (v);
            }
            break;
          
          // vec_get_hard
          case 0x91:
            {
              int index = *((unsigned short *)ptr);
              ptr += 2;
              
              auto& vec = stack[sp - 1].val.gc->val.vec;
              stack[sp - 1] = vec.vals[index];
            }
            break;
          
          // vec_get
          case 0x92:
            {
              if (stack[sp - 1].type != RHO_INTEGER)
                throw vm_error ("index must be an integer");
              auto& idx = stack[sp - 1].val.gc;
              long i = mpz_get_si (idx->val.i);
              
              switch (stack[sp - 2].type)
                {
                case RHO_VEC:
                  {
                    auto& vec = stack[sp - 2].val.gc->val.vec;
                    if (i < 0 || i >= vec.len)
                      throw vm_error ("index out of range");
                    
                    -- sp;
                    stack[sp - 1] = vec.vals[i];
                  }
                  break;
                
                case RHO_CONS:
                  {
                    -- sp;
                    auto& c = stack[sp - 1].val.gc->val.p;
                    switch (i)
                      {
                      case 0:
                        stack[sp - 1] = c.fst;
                        break;
                      
                      case 1:
                        stack[sp - 1] = c.snd;
                        break;
                      
                      default:
                        throw vm_error ("index out of range (cons index must be 0 or 1)");
                      }
                  }
                  break;
                
                default:
                  throw vm_error ("invalid object to subscript");
                }
            }
            break;
          
          // vec_set
          case 0x93:
            {
              if (stack[sp - 2].type != RHO_INTEGER)
                throw vm_error ("index must be an integer");
              auto& idx = stack[sp - 2].val.gc;
              long i = mpz_get_si (idx->val.i);
              
              switch (stack[sp - 3].type)
                {
                case RHO_VEC:
                  {
                    auto& vec = stack[sp - 3].val.gc->val.vec;
                    if (i < 0 || i >= vec.len)
                      throw vm_error ("index out of range");
                    
                    vec.vals[i] = stack[sp - 1];
                    sp -= 3;
                  }
                  break;
                
                case RHO_CONS:
                  {
                    auto& c = stack[sp - 3].val.gc->val.p;
                    switch (i)
                      {
                      case 0:
                        c.fst = stack[sp - 1];
                        sp -= 3;
                        break;
                      
                      case 1:
                        c.snd = stack[sp - 1];
                        sp -= 3;
                        break;
                      
                      default:
                        throw vm_error ("index out of range (cons index must be 0 or 1)");
                      }
                  }
                  break;
                
                default:
                  throw vm_error ("invalid object to subscript");
                }
            }
            break;
          
        
        
        //----------------------------------------------------------------------
        // global variables
        //----------------------------------------------------------------------
          
          // alloc_globals
          case 0xA0:
            {
              unsigned pidx = *((unsigned short *)ptr);
              unsigned count = *((unsigned short *)(ptr + 2));
              ptr += 4;
              
              glob_page page;
              page.vals = new rho_value[count];
              page.size = count;
              for (unsigned i = 0; i < count; ++i)
                page.vals[i].type = RHO_NIL;
              
              if (pidx == this->gpages.size ())
                this->gpages.push_back (page);
              else
                {
                  if (this->gpages.size () <= pidx)
                    this->gpages.resize (pidx + 1);
                  this->gpages[pidx] = page;
                }
            }
            break;
          
          // get_global
          case 0xA1:
            {
              unsigned pidx = *((unsigned short *)ptr);
              unsigned idx = *((unsigned short *)(ptr + 2));
              ptr += 4;
              
              stack[sp ++] = this->gpages[pidx].vals[idx];
            }
            break;
          
          // set_global
          case 0xA2:
            {
              unsigned pidx = *((unsigned short *)ptr);
              unsigned idx = *((unsigned short *)(ptr + 2));
              ptr += 4;
              
              this->gpages[pidx].vals[idx] = stack[-- sp];
            }
            break;
          
          // def_atom
          case 0xA3:
            {
              int num = *(int *)ptr;
              ptr += 4;
              
              const char *name = (const char *)ptr;
              ptr += std::strlen (name) + 1;
              
              if (num == (int)this->atom_names.size ())
                this->atom_names.push_back (name);
              else
                {
                  if (num > (int)this->atom_names.size ())
                    this->atom_names.resize (num + 1);
                  this->atom_names[num] = name;
                }
            }
            break;
        
        
        
        //----------------------------------------------------------------------
        // micro-frames
        //----------------------------------------------------------------------
          
          // push_microframe
          case 0xB0:
            {
              -- sp;
              if (stack[sp].type != RHO_INTEGER)
                throw vm_error ("push_microframe: precision must be specified using an integer");
              unsigned int prec10 = mpz_get_ui (stack[sp].val.gc->val.i);
              unsigned int prec2 = prec_base10_to_bits (prec10);
              
              auto start = sp;
              
              // push pointer to parent micro-frame
              stack[sp ++] = MK_INTERNAL (GET_INTERNAL(stack[bp + 4]));
              
              // push precision in bits
              stack[sp ++] = MK_INTERNAL (prec2);
              
              // push precision in decimal digits
              stack[sp ++] = MK_INTERNAL (prec10);
              
              // update micro-frame pointer
              stack[bp + 4].val.i64 = start;
            }
            break;
          
          // pop_microframe
          case 0xB1:
            {
              int start = GET_INTERNAL (stack[bp + 4]);
              
              int pf = GET_INTERNAL (stack[start]); // pointer to previous micro-frame
              stack[bp + 4].val.i64 = pf;
              
              stack[start] = stack[sp - 1];
              sp = start + 1;
            }
            break;
          
          
          
        //----------------------------------------------------------------------
        // other
        //----------------------------------------------------------------------
           
          // breakpoint
          case 0xF0:
            {
              int bp = *((int *)ptr);
              ptr += 4;
              
              stack[sp++] = rho_value_make_nil ();
              
              std::cout << "BP#" << bp << std::endl;
              if (bp == 0)
                int a = 5;
              else if (bp == 10)
                int a = 5;
            }
            break;
          
          // exit
          case 0xFF:
            goto done;
          }
      }
    
  done:
    return stack[sp - 1];
  }
  
  
  
  /* 
   * Clears the VM's stack.
   */
  void
  virtual_machine::reset ()
  {
    this->sp = 0;
    this->gc->collect ();
  }
  
  
  
  /* 
   * Pops the top-most value off the stack.
   */    
  void
  virtual_machine::pop_value ()
  {
    -- this->sp;
  }
  
  
  
//------------------------------------------------------------------------------
  
  stack_provider::stack_provider (rho_value *stack, int sp, bool refs_only)
  {
    this->stack = stack;
    this->sp = sp;
    this->refs_only = refs_only;
  }
  
  
    
  stack_provider::iterator
  stack_provider::begin ()
    { return iterator (this->stack, this->sp, 0, this->refs_only); }
  
  stack_provider::iterator
  stack_provider::end ()
    { return iterator (this->stack, this->sp, this->sp, this->refs_only); }
  
  
  
  stack_provider::iterator::iterator (rho_value *stack, int sp, int curr,
                                      bool refs_only)
  {
    this->stack = stack;
    this->sp = sp;
    this->curr = curr;
    this->refs_only = refs_only;
    
    if (refs_only)
      {
        while (this->curr < this->sp && IS_INTERNAL(stack[this->curr]))
          ++ this->curr;
      }
  }
  
  stack_provider::iterator::iterator (const iterator& other)
  {
    this->stack = other.stack;
    this->sp = other.sp;
    this->curr = other.curr;
    this->refs_only = other.refs_only;
  }
  
  
  rho_value
  stack_provider::iterator::operator* () const
    { return this->stack[this->curr]; }
  
  stack_provider::iterator&
  stack_provider::iterator::operator++ ()
  {
    ++ this->curr;
    while (this->curr < this->sp && IS_INTERNAL(stack[this->curr]))
      ++ this->curr;
    
    return *this;
  }
  
  stack_provider::iterator&
  stack_provider::iterator::operator= (const iterator& other)
  {
    this->stack = other.stack;
    this->sp = other.sp;
    this->curr = other.curr;
    this->refs_only = other.refs_only;
    return *this;
  }
  
  bool
  stack_provider::iterator::operator== (const iterator& other) const
  {
    return this->stack == other.stack && this->sp == other.sp
      && this->curr == other.curr && this->refs_only == other.refs_only;
  }
  
  bool
  stack_provider::iterator::operator!= (const iterator& other) const
    { return !this->operator== (other); }
  
}

