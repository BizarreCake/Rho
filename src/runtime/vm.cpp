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

#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"
#include "runtime/value.hpp"
#include <cstring>

#include <iostream> // DEBUG


namespace rho {
  
  virtual_machine::virtual_machine ()
  {
    this->sp = 0;
    this->bp = 0;
    
    this->gc = garbage_collector::create ("V1", *this);
  }
  
  virtual_machine::~virtual_machine ()
  {
    this->sp = this->bp = 0;
    delete this->gc;
  }
  
  
  
  int
  virtual_machine::get_current_prec ()
  {
    return GET_INTERNAL(stack[GET_INTERNAL(stack[bp + 2]) + 1]);
  }
  
  
  
#define STACK_OVERFLOW_CHECK(N)   \
  if (sp + (N) > RHO_STACK_SIZE)  \
    throw vm_error ("stack overflow");

#define PUT_INTERNAL(SP, VAL)     \
  stack[SP] = (VALUE)((VAL) | (1ULL << 63));
  
  
  
  /* 
   * Allows the virtual machine to be used in a REPL configuration.
   * This creates a big initial stack frame to hold global variables created
   * during the REPL.
   */
  void
  virtual_machine::init_repl ()
  {
#define REPL_VAR_COUNT      1024
    STACK_OVERFLOW_CHECK(2 + REPL_VAR_COUNT)
    
    // internal: previous base pointer
    PUT_INTERNAL(sp, bp)
    bp = sp;
    ++ sp;
    
    // internal: local variable count
    PUT_INTERNAL(sp, REPL_VAR_COUNT)
    ++ sp;
    
    // internal: microframe
    PUT_INTERNAL(sp, bp + 3 + REPL_VAR_COUNT)
    ++ sp;
    
    // leave space for local variables
    for (int i = 0; i < REPL_VAR_COUNT; ++i)
      stack[sp + i] = nullptr;
    sp += REPL_VAR_COUNT;
    
    
    // default microframe
    
    PUT_INTERNAL(sp, 0); // pointer to previous microframe
    ++ sp;
    
    PUT_INTERNAL(sp, 20); // precision
    ++ sp;
  }
  
  /* 
   * Returns the global REPL value at the specified index.
   * NOTE: The value is simply a reference to the VM's stack, and thus it
   *       will become invalid as soon as the virtual machine is destroyed.
   */
  VALUE
  virtual_machine::get_repl_var (int index)
  {
    return this->stack[3 + index];
  }
  
  
  
  /* 
   * Executes the specified executable.
   */
  void
  virtual_machine::run (executable& exec)
  {
    const unsigned char *code = exec.find_section ("code")->data.get_data ();
    const unsigned char *ptr = code;
    
    VALUE* stack = this->stack;
    int& sp = this->sp;
    int& bp = this->bp;
    
    for (;;)
      {
        switch (*ptr++)
          {
          // push_int_32 - pushes a signed 32-bit integer onto the stack.
          case 0x00:
            STACK_OVERFLOW_CHECK(1)
            stack[sp] = rho_value_new_int (*((int *)ptr), *this);
            rho_value_unprotect (stack[sp]);
            ptr += 4;
            ++ sp;
            break;
          
          // push_int - pushes an arbitrarily-sized integer onto the stack.
          case 0x01:
            STACK_OVERFLOW_CHECK(1)
            {
              int len = std::strlen ((char *)ptr);
              stack[sp] = rho_value_new_int ((char *)ptr, *this);
              rho_value_unprotect (stack[sp]);
              ptr += len + 1;
              ++ sp;
            }
            break;
          
          // pop - pops off the top-most element from the stack.
          case 0x02:
            -- sp;
            break;
          
          // dup - duplicates the top-most stack element.
          case 0x03:
            STACK_OVERFLOW_CHECK(1)
            stack[sp] = stack[sp - 1];
            ++ sp;
            break;
          
          // push_nil
          case 0x04:
            STACK_OVERFLOW_CHECK(1)
            stack[sp] = rho_value_new_nil (*this);
            rho_value_unprotect (stack[sp]);
            ++ sp;
            break;
          
          // push_real
          case 0x05:
            STACK_OVERFLOW_CHECK(1)
            {
              int prec = GET_INTERNAL(stack[GET_INTERNAL(stack[bp + 2]) + 1]);
              int len = std::strlen ((char *)ptr);
              stack[sp] = rho_value_new_real ((char *)ptr, prec, *this);
              rho_value_unprotect (stack[sp]);
              ptr += len + 1;
              ++ sp;
            }
            break;
          
          // push_sym
          case 0x06:
            STACK_OVERFLOW_CHECK(1)
            {
              int len = std::strlen ((char *)ptr);
              stack[sp] = rho_value_new_sym ((char *)ptr, *this);
              rho_value_unprotect (stack[sp]);
              ptr += len + 1;
              ++ sp;
            }
            break;
          
//------------------------------------------------------------------------------
          
          // push_frame - allocates a new stack frame.
          case 0x10:
            {
              int locs = *((unsigned short *)ptr);
              ptr += 2;
              
              STACK_OVERFLOW_CHECK(3 + locs)
              
              // internal: previous base pointer
              int pbp = bp;
              PUT_INTERNAL(sp, bp)
              bp = sp;
              ++ sp;
              
              // internal: local variable count
              PUT_INTERNAL(sp, locs)
              ++ sp;
              
              // internal: microframe
              PUT_INTERNAL(sp, bp + 3 + locs)
              ++ sp;
              
              // leave space for local variables
              for (int i = 0; i < locs; ++i)
                stack[sp + i] = nullptr;
              sp += locs;
              
              // create new default microframe
              
              int pmfrm = GET_INTERNAL(stack[pbp + 2]);
              
              PUT_INTERNAL(sp, pmfrm) // pointer to previous microframe
              ++ sp;
              
              // inherit precision from last microframe
              int prec = pmfrm ? GET_INTERNAL(stack[pmfrm + 1]) : 20;
              PUT_INTERNAL(sp, prec)
              ++ sp;
            }
            break;
          
          // pop_frame
          case 0x11:
            {
              sp = bp;
              bp = GET_INTERNAL(stack[sp]);
            }
            break;
          
          // load_loc
          case 0x12:
            STACK_OVERFLOW_CHECK(1)
            stack[sp++] = stack[bp + 3 + *ptr++];
            break;
          
          // store_loc
          case 0x13:
            stack[bp + 3 + *ptr++] = stack[--sp];
            break;
          
          // load_loc_p - loads a local variable from a parent frame
          case 0x14:
            STACK_OVERFLOW_CHECK(1)
            {
              int nbp = bp;
              int frm = *ptr++;
              while (frm --> 0)
                nbp = GET_INTERNAL(stack[nbp]);;
              stack[sp++] = stack[nbp + 3 + *ptr++];
            }
            break;
          
          // store_loc_p
          case 0x15:
            STACK_OVERFLOW_CHECK(1)
            {
              int nbp = bp;
              int frm = *ptr++;
              while (frm --> 0)
                nbp = GET_INTERNAL(stack[nbp]);
              stack[nbp + 3 + *ptr++] = stack[--sp];
            }
            break;
          
          // load_repl_var
          case 0x16:
            STACK_OVERFLOW_CHECK(1)
            stack[sp++] = stack[3 + *ptr++];
            break;
          
          // store_repl_var
          case 0x17:
            STACK_OVERFLOW_CHECK(1)
            stack[3 + *ptr++] = stack[-- sp];
            break;
          
//------------------------------------------------------------------------------

          // add
          case 0x20:
            stack[sp - 2] = rho_value_add (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // sub
          case 0x21:
            stack[sp - 2] = rho_value_sub (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // mul
          case 0x22:
            stack[sp - 2] = rho_value_mul (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // div
          case 0x23:
            stack[sp - 2] = rho_value_div (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // mod
          case 0x24:
            stack[sp - 2] = rho_value_mod (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // pow
          case 0x25:
            stack[sp - 2] = rho_value_pow (stack[sp - 2], stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // factorial
          case 0x26:
            stack[sp - 1] = rho_value_factorial (stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 1]);
            break;
          
          // negate
          case 0x27:
            stack[sp - 1] = rho_value_negate (stack[sp - 1], *this);
            rho_value_unprotect (stack[sp - 1]);
            break;
                    
//------------------------------------------------------------------------------
          
          // jmp
          case 0x30:
            ptr += 2 + *((short *)ptr);
            break;
          
          // jt - jump if true
          case 0x31:
            -- sp;
            if (mpz_sgn (RHO_VALUE(stack[sp])->val.i) != 0)
              ptr += 2 + *((short *)ptr);
            else
              ptr += 2;
            break;
          
          // jf - jump if false
          case 0x32:
            -- sp;
            if (mpz_sgn (RHO_VALUE(stack[sp])->val.i) == 0)
              ptr += 2 + *((short *)ptr);
            else
              ptr += 2;
            break;
          
//------------------------------------------------------------------------------
          
          // return
          case 0x40:
            {
              ptr = (const unsigned char *)GET_INTERNAL(stack[bp - 1]);
              
              int ret_sp = sp - 1;
              int arg_count = GET_INTERNAL(stack[bp - 2]);
              
              int nbp = GET_INTERNAL(stack[bp]);
              sp = bp - 3 - arg_count;
              bp = nbp;
              
              stack[sp++] = stack[ret_sp];
            }
            break;
          
          // create_func
          case 0x41:
            {
              unsigned int code_size = *((unsigned int *)ptr);
              ptr += 4;
              
              int code_off = *((unsigned int *)ptr);
              ptr += 4;
              
              rho_value *fn = rho_value_new_func (code_size, *this);
              stack[sp] = fn;
              
              // copy code
              std::memcpy (fn->val.fn.code, ptr + code_off, code_size);
              
              rho_value_unprotect (stack[sp]);
              ++ sp;
            }
            break;
          
          // call
          case 0x42:
            {
              int arg_count = *ptr++;
              
              rho_value *fn = RHO_VALUE(stack[sp - 1]);
              
              PUT_INTERNAL(sp, arg_count)
              ++ sp;
              
              PUT_INTERNAL(sp, (unsigned long long)ptr)
              ++ sp;
              
              ptr = fn->val.fn.code;
            }
            break;
          
          // load_arg
          case 0x43:
            STACK_OVERFLOW_CHECK(1)
            stack[sp++] = stack[bp - 4 - *ptr++];
            break;
          
          // store_arg
          case 0x44:
            stack[bp - 4 - *ptr++] = stack[--sp];
            break;
          
//------------------------------------------------------------------------------

          // cmp_eq
          case 0x50:
            stack[sp - 2] = rho_value_eq (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // cmp_ne
          case 0x51:
            stack[sp - 2] = rho_value_neq (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // cmp_lt
          case 0x52:
            stack[sp - 2] = rho_value_lt (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // cmp_le
          case 0x53:
            stack[sp - 2] = rho_value_lte (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // cmp_gt
          case 0x54:
            stack[sp - 2] = rho_value_gt (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;
          
          // cmp_ge
          case 0x55:
            stack[sp - 2] = rho_value_gte (RHO_VALUE (stack[sp - 2]),
              RHO_VALUE (stack[sp - 1]), *this);
            rho_value_unprotect (stack[sp - 2]);
            -- sp;
            break;

//------------------------------------------------------------------------------

          // push_microframe
          case 0x60:
            {
              int pmfrm = GET_INTERNAL(stack[bp + 2]);
              int cmfrm = sp;
              
              // internal: pointer to previous microframe
              PUT_INTERNAL(sp, pmfrm);
              ++ sp;
              
              // inherit precision
              PUT_INTERNAL(sp, GET_INTERNAL(stack[pmfrm + 1]));
              ++ sp;
              
              PUT_INTERNAL(bp + 2, cmfrm);
            }
            break;
          
         // pop_microframe
         case 0x61:
          {
            int ret_sp = sp - 1;
            int mfrm = GET_INTERNAL(stack[bp + 2]);
            
            sp = mfrm;
            
            // restore previous microframe
            PUT_INTERNAL(bp + 2, GET_INTERNAL(stack[mfrm]));
            
            stack[sp++] = stack[ret_sp];
          }
          break;
        
        // set_prec
        case 0x62:
          {
            int mfrm = GET_INTERNAL(stack[bp + 2]);
            PUT_INTERNAL(mfrm + 1, mpz_get_ui (stack[-- sp]->val.i));
          }
          break;
        
//------------------------------------------------------------------------------
          
          // exit:
          case 0xF0:
            goto done;
          }
      }
  done:;
  
    // DEBUG
    this->gc->collect ();
  }
}

