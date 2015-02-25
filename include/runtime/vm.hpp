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

#ifndef _RHO__RUNTIME__VM__H_
#define _RHO__RUNTIME__VM__H_

#include "linker/module.hpp"
#include <stdexcept>


namespace rho {

  class garbage_collector;
  struct rho_value;
  
  
  // since pretty much everything in Rho is accessed by reference (even
  // integers, since abitrary-precision arithmetic is implemented), all stack
  // values in the virtual machine are of this type.
  typedef rho_value* VALUE;
  
#define VALUE_TYPE(VAL)      (((rho_value *)VAL)->type)
#define RHO_VALUE(VAL)       ((rho_value *)VAL)
#define VALUE_IS_INTERNAL(VAL) ((((unsigned long long)(VAL)) & (1ULL << 63)) != 0)
#define GET_INTERNAL(VAL)    ((unsigned long long)(VAL) & ~(1ULL << 63))


#define RHO_STACK_SIZE      4096  
  
  
  class vm_error: public std::runtime_error
  {
  public:
    vm_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  /* 
   * The virtual machine.
   * Executes bytecode emitted by the compiler.
   */
  class virtual_machine
  {
    VALUE stack[RHO_STACK_SIZE];
    int sp;
    int bp;
    
    garbage_collector *gc;
    
  public:
    inline VALUE* get_stack () { return this->stack; }
    inline int get_sp () { return this->sp; }
    inline garbage_collector& get_gc () { return *this->gc; }
     
  public:
    virtual_machine ();
    ~virtual_machine ();
  
  public:
    int get_current_prec ();
    
  public:
    /* 
     * Allows the virtual machine to be used in a REPL configuration.
     * This creates a big initial stack frame to hold global variables created
     * during the REPL.
     */
    void init_repl ();
    
    /* 
     * Returns the global REPL value at the specified index.
     * NOTE: The value is simply a reference to the VM's stack, and thus it
     *       will become invalid as soon as the virtual machine is destroyed.
     */
    VALUE get_repl_var (int index);
    
  public:
    /* 
     * Executes the specified executable.
     */
    void run (executable& exec);
  };
}

#endif

