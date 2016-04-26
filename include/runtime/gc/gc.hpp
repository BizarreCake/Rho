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

#ifndef _RHO__RUNTIME__GC__GC__H_
#define _RHO__RUNTIME__GC__GC__H_

#include "runtime/value.hpp"
#include <list>


namespace rho {
  
  // forward decs:
  class virtual_machine;
  
  
  /* 
   * Base class for all garbage collector implementations.
   * The garbage collector implements routines for allocating memory and
   * reclaiming memory of objects that are no longer in use.
   */
  class garbage_collector
  {
  protected:
    virtual_machine& vm;
  
  public:
    garbage_collector (virtual_machine& vm);
    virtual ~garbage_collector () { }
    
    /* 
     * Factory function for creating garbage collectors.
     */
    static garbage_collector* create (const char *name, virtual_machine& vm);
    
  public:
    /*  
     * Allocates space for a Rho value in the heap and returns a pointer to it.
     * The created object is granted protection and must be unprotected in
     * order for its memory to be reclaimed once it's no longer in use.
     */
    virtual gc_value* alloc_protected () = 0;
    
    
    /* 
     * Allocates an upvalue and inserts it into the collector's upvalue list.
     */
    virtual gc_value* alloc_upvalue_protected () = 0;
    
    virtual const std::list<gc_value*>& get_upvalues () const = 0;
    
    
    
    /* 
     * Runs a single cycle of work.
     */
    virtual void step () = 0;
    
    /* 
     * Performs a full collection (which may consist of several cycles of work).
     */
    virtual void collect () = 0;
  };
}

#endif

