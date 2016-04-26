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

#ifndef _RHO__RUNTIME__GC__BASIC__GC__H_
#define _RHO__RUNTIME__GC__BASIC__GC__H_

#include "runtime/gc/gc.hpp"


namespace rho {
  
  enum gc_state
  {
    GC_WHITE = 0,   // candidate for recycling.
    GC_GRAY  = 1,   // alive, but child references have not been scanned yet.
    GC_BLACK = 2,   // alive.
  };
  
  
  /* 
   * Simple mark-and-sweep garbage collector.
   */
  class basic_gc: public garbage_collector
  {
  private:
    /* 
     * Stores a Rho value with pointers to the previous and next objects in the
     * chain.
     */
    struct gc_object
    {
      gc_value val;
      gc_object *next;
    };
    
    /* 
     * Stores a pointer to a Rho value.
     */
    struct gc_object_ref
    {
      gc_value *val;
      gc_object_ref *next;
    };
    
  private:
    // linked list of all allocated objects in the system.
    gc_object *head;
    
    // gray objects.
    gc_object_ref *gray;
  
    int t_alloc;
    int t_free;
    
    std::list<gc_value *> upvals;
  
  public:
    basic_gc (virtual_machine& vm);
    ~basic_gc ();
    
  private:
    /* 
     * Inserts an object into the gray set.
     */
    void paint_gray (rho_value& v);
    
    /* 
     * Pops one value from the gray set.
     */
    gc_value* pop_gray ();
    
    /* 
     * Inserts all objects the given object directly references into the gray set.
     */
    void mark_children (gc_value *v);
    
  public:
    virtual gc_value* alloc_protected () override;
    
    virtual gc_value* alloc_upvalue_protected () override;
    
    virtual const std::list<gc_value*>& get_upvalues () const override;
    
    virtual void step () override;
    
    virtual void collect () override;
  };
}

#endif

