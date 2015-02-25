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

#ifndef _RHO__RUNTIME__GC__V1__H_
#define _RHO__RUNTIME__GC__V1__H_

#include "runtime/gc/gc.hpp"
#include "runtime/value.hpp"
#include "runtime/sop.hpp"


namespace rho {
  
  /* 
   * The "V1" garbage collector.
   * 
   * This is the first garbage collector implemented for Rho, intended to serve
   * as a placeholder for a future garbage collector.
   * It is very basic, and only performs simple stop-the-world mark-and-sweep
   * collection.
   */
  class v1_gc: public garbage_collector
  {
  private:
    /* 
     * This GC keeps track of objects by keeping a doubly-linked of Rho values.
     */
    struct gc_object
    {
      rho_value val;
      
      gc_object *prev;
      gc_object *next;
    };
    
    struct gc_sop_object
    {
      rho_sop sop;
      
      gc_sop_object *prev;
      gc_sop_object *next;
    };
    
    // contains a pointer to the Rho value instead of owning it.
    struct gc_object_ref
    {
      unsigned char what;
      union
        {
          rho_value *val;
          rho_sop *sop;
        } data;
      
      gc_object_ref *prev;
      gc_object_ref *next;
    };
    
    
    
  private:
    unsigned int cycles;
    unsigned int allocs;
    
    gc_object *head;
    gc_sop_object *sop_head;
    gc_object_ref *gray;  // gray set/list
    
    // protected SOPs are added into this list as soon as they are allocated.
    gc_object_ref *sop_roots;
    
  public:
    v1_gc (virtual_machine& vm);
    ~v1_gc ();
    
  private:
    /* 
     * Inserts the specified Rho value into the gray set.
     */
    void paint_gray (rho_value *val);
    void paint_gray (rho_sop *sop);
    
    /* 
     * Traverses the VM's stack and adds all objects in the stack into the
     * gray set.
     */
    void mark_roots ();
    
    // DEBUG
    void print_sop_roots ();
    
    /* 
     * Inserts all white objects referenced by the specified Rho value into
     * the gray set.
     */
    void mark_children (rho_value *val);
    void mark_children (rho_sop *sop);
    
    /* 
     * Removes an object from the gray set, paint it black, and paints all
     * white objects referenced by it gray.
     * This is done until the gray set is empty.
     */
    void process_gray_set ();
    
    
    
    /* 
     * Reclaims memory used by a Rho value.
     */
    void free_object (rho_value *val);
    void free_object (rho_sop *sop);
    
    /* 
     * Iterates through all tracked objects and deletes white (unreferenced)
     * objects.  All black (live) objects are painted back to white at the end.
     */
    void sweep ();
    
  public:
    virtual rho_value* alloc_protected () override;
    
    virtual rho_sop* alloc_sop_protected () override;
    
    virtual void protect (rho_sop *sop) override;
    
  //----
    
    virtual void step () override;
    
    virtual void collect () override;
  };
}

#endif

