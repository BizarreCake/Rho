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

#ifndef _RHO__RUNTIME__VM__H_
#define _RHO__RUNTIME__VM__H_

#include "linker/program.hpp"
#include "runtime/value.hpp"
#include <unordered_map>
#include <vector>


namespace rho {
  
#define VM_DEF_STACK_SIZE 8192
#define VM_SMALL_INT_MAX    10

  // forward decs:
  class garbage_collector;
  class virtual_machine;
  
  
  
#define IS_INTERNAL(V)  ((V).type == RHO_INTERNAL)
#define MK_INTERNAL(V)  (rho_value_make_internal ((long long)(V)))
#define GET_INTERNAL(V) ((V).val.i64)
  
  class stack_provider
  {
    friend class virtual_machine;
  
  public:
    class iterator
    {
      rho_value *stack;
      int sp;
      int curr;
      bool refs_only;
      
    public:
      iterator (rho_value *stack, int sp, int curr, bool refs_only);
      iterator (const iterator& other);
      
    public:
      rho_value operator* () const;
      iterator& operator++ ();
      iterator& operator= (const iterator& other);
      
      bool operator== (const iterator& other) const;
      bool operator!= (const iterator& other) const;
    };
  
  private:
    rho_value *stack;
    int sp;
    bool refs_only;
    
  public:
    inline int get_sp () const { return this->sp; }
    
  private:
    stack_provider (rho_value *stack, int sp, bool refs_only);
    
  public:
    iterator begin ();
    iterator end ();
  };
  
  
  struct glob_page
  {
    rho_value *vals;
    int size;
  };
  
  
  
  /* 
   * The bytecode executor.
   */
  class virtual_machine
  {
    rho_value *stack;
    int sp; // stack pointer
    int bp; // base pointer
    garbage_collector *gc;
    
    rho_value *ints; // pre-allocated small integers
    std::vector<glob_page> gpages;
    
  public:
    inline garbage_collector& get_gc () { return *this->gc; }
    inline std::vector<glob_page>& get_globals () { return this->gpages; }
    
  public:
    virtual_machine (int stack_size = VM_DEF_STACK_SIZE,
                     const char *gc_name = "basic");
    ~virtual_machine ();
    
  public:
    /* 
     * Executes the specified Rho program.
     * Returns the top-most value in the VM's stack on completion.
     */
    rho_value run (program& prg);
    
    /* 
     * Clears the VM's stack.
     */
    void reset ();

    /* 
     * Pops the top-most value off the stack.
     */    
    void pop_value ();
    
  public:
    // 
    // GC support:
    // 
    
    /* 
     * Returns an object through which stack values can be retreived.
     */
    stack_provider get_stack (bool refs_only = false);
    
    inline rho_value&
    get_stack_at (int sp)
      { return this->stack[sp]; }
  };
}

#endif

