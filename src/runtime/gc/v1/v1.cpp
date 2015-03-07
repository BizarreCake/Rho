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

#include "runtime/gc/v1/v1.hpp"
#include "runtime/vm.hpp"
#include "runtime/sop.hpp"
#include <stack>

#include <iostream> // DEBUG


namespace rho {

#define V1_ALLOCS_PER_COLLECT       400
#define V1_STEPS_PER_COLLECT         10

  // GC states
  enum {
    GC_WHITE,
    GC_GRAY,
    GC_BLACK,
  };
  
  enum {
    GC_VAL,
    GC_SOP,
  };

  
  
  v1_gc::v1_gc (virtual_machine& vm)
    : garbage_collector (vm)
  {
    this->cycles = 0;
    this->allocs = 0;
    this->head = nullptr;
    this->sop_head = nullptr;
    this->gray = nullptr;
    this->sop_roots = nullptr;
  }
  
  v1_gc::~v1_gc ()
  {
    this->collect ();
  }
  
  
  
  /* 
   * Allocates and returns a new protected Rho value.
   * The garbage collector will not attempt to reclaim the object until its
   * protection is removed.
   */
  rho_value*
  v1_gc::alloc_protected ()
  {
    if (this->allocs++ == V1_ALLOCS_PER_COLLECT)
      {
        this->collect ();
        this->allocs = 0;
      }
    
    gc_object *gobj = new gc_object;
    gobj->prev = nullptr;
    gobj->next = this->head;
    if (this->head)
      this->head->prev = gobj;
    this->head = gobj;
    
    rho_value *val = &gobj->val;
    val->gc_protect = 1;
    val->gc_state = GC_WHITE;
    
    return val;
  }
  
  /* 
   * Allocates a new protected SOP (symbolic expression).
   */
  rho_sop*
  v1_gc::alloc_sop_protected ()
  {
    gc_sop_object *gobj = new gc_sop_object;
    gobj->prev = nullptr;
    gobj->next = this->sop_head;
    if (this->sop_head)
      this->sop_head->prev = gobj;
    this->sop_head = gobj;
    
    rho_sop *sop = &gobj->sop;
    sop->gc_state = GC_WHITE;
    sop->gc_protect = 1;
    
    // add to SOP root set
    gc_object_ref *gref = new gc_object_ref;
    gref->what = GC_SOP;
    gref->data.sop = sop;
    gref->prev = nullptr;
    gref->next = this->sop_roots;
    
    if (this->sop_roots)
      this->sop_roots->prev = gref;
    this->sop_roots = gref;
    
    return sop;
  }
  
  
  /* 
   * Grants GC protection to the specified object.
   */
  void
  v1_gc::protect (rho_sop *sop)
  {
    if (sop->gc_protect)
      return;
    
    sop->gc_protect = 1;
    
    // add to SOP root set
    gc_object_ref *gref = new gc_object_ref;
    gref->what = GC_SOP;
    gref->data.sop = sop;
    gref->prev = nullptr;
    gref->next = this->sop_roots;
    if (this->sop_roots)
      this->sop_roots->prev = gref;
    this->sop_roots = gref;
  }
  
//----
  
  /* 
   * Inserts the specified Rho value into the gray set.
   */
  void
  v1_gc::paint_gray (rho_value *val)
  {
    if (val->gc_state != GC_WHITE)
      return;
    val->gc_state = GC_GRAY;
    
    gc_object_ref *gref = new gc_object_ref;
    gref->what = GC_VAL;
    gref->data.val = val;
    gref->prev = nullptr;
    gref->next = this->gray;
    this->gray = gref;
  }
  
  void
  v1_gc::paint_gray (rho_sop *sop)
  {
    if (!sop || sop->gc_state != GC_WHITE)
      return;
    sop->gc_state = GC_GRAY;
    
    gc_object_ref *gref = new gc_object_ref;
    gref->what = GC_SOP;
    gref->data.sop = sop;
    gref->prev = nullptr;
    gref->next = this->gray;
    this->gray = gref;
  }
  
  
  
  /* 
   * Traverses the VM's stack and adds all objects in the stack into the
   * gray set.
   */
  void
  v1_gc::mark_roots ()
  {
    VALUE *stack = this->vm.get_stack ();
    int sp = this->vm.get_sp ();
    
    // stack
    for (int i = 0; i < sp; ++i)
      {
        if (!stack[i])
          continue;
        if (VALUE_IS_INTERNAL(stack[i]))
          continue; // internal field
        
        rho_value *val = RHO_VALUE (stack[i]);
        this->paint_gray (val);
      }
    
    // SOP roots
    {
      gc_object_ref *gref = this->sop_roots;
      while (gref)
        {
          if (!gref->data.sop->gc_protect)
            {
              // not protected (this means that the SOP should have a pointer to
              // it if it's alive).
              // remove from list.
              
              gc_object_ref *next = gref->next;
              
              if (gref->prev)
                gref->prev->next = next;
              else
                this->sop_roots = next;
              if (gref->next)
                gref->next->prev = gref->prev;
              
              delete gref;
              
              gref = next;
              continue;
            }
          
          this->paint_gray (gref->data.sop);
          gref = gref->next;
        }
    }
  }
  
  
  /* 
   * Inserts all white objects referenced by the specified Rho value into
   * the gray set.
   */
  void
  v1_gc::mark_children (rho_value *val)
  {
    switch (val->type)
      {
      case RHO_NIL: break;
      case RHO_INT: break;
      case RHO_REAL: break;
      case RHO_FUNC: break;
      case RHO_SYM: break;
      
      case RHO_SOP:
        this->paint_gray (val->val.sop);
        break;
      
      case RHO_EMPTY_CONS: break;
      case RHO_CONS:
        this->paint_gray (val->val.cons.car);
        this->paint_gray (val->val.cons.cdr);
        break;
      }
  }
  
  void
  v1_gc::mark_children (rho_sop *sop)
  {
    switch (sop->type)
      {
      case SOP_SYM:
      case SOP_VAL:
        this->paint_gray (sop->val.val);
        break;
      
      case SOP_ADD:
      case SOP_MUL:
        for (int i = 0; i < rho_sop_arr_size (sop); ++i)
          this->paint_gray (sop->val.arr.elems[i]);
        break;
      
      case SOP_DIV:
      case SOP_POW:
        this->paint_gray (sop->val.bop.lhs);
        this->paint_gray (sop->val.bop.rhs);
        break;
      }
  }
  
  
  /* 
   * Removes an object from the gray set, paint it black, and paints all
   * white objects referenced by it gray.
   * This is done until the gray set is empty.
   */
  void
  v1_gc::process_gray_set ()
  {
    while (this->gray)
      {
        gc_object_ref *gref = this->gray;
        this->gray = gref->next;
        
        if (gref->what == GC_VAL)
          {
            rho_value *val = gref->data.val;
            this->mark_children (val);
            val->gc_state = GC_BLACK;
          }
        else if (gref->what == GC_SOP)
          {
            rho_sop *sop = gref->data.sop;
            this->mark_children (sop);
            sop->gc_state = GC_BLACK;
          }
        
        delete gref;
      }
  }
  
  
  
  /* 
   * Reclaims memory used by a Rho value.
   */
  void 
  v1_gc::free_object (rho_value *val)
  {
    switch (val->type)
      {
      case RHO_NIL: break;
      case RHO_EMPTY_CONS: break;
      
      case RHO_INT:
        mpz_clear (val->val.i);
        break;
      
      case RHO_REAL:
        mpfr_clear (val->val.real.f);
        break;
      
      case RHO_FUNC:
        delete[] val->val.fn.code;
        break;
      
      case RHO_SYM:
        delete[] val->val.sym;
        break;
      
      case RHO_SOP:
        break;
      }
  }
  
  
  /* 
   * Iterates through all tracked objects and deletes white (unreferenced)
   * objects.  All black (live) objects are painted back to white at the end.
   */
  void
  v1_gc::sweep ()
  {
    // 
    // Sweep regular values.
    // 
    {
      gc_object *gobj = this->head, *next;
      while (gobj)
        {
          if (gobj->val.gc_protect || gobj->val.gc_state != GC_WHITE)
            {
              // live object
              // paint back to white
              gobj->val.gc_state = GC_WHITE;
              gobj = gobj->next;
              continue;
            }
          
          next = gobj->next;
          
          if (gobj->prev)
            gobj->prev->next = gobj->next;
          else
            this->head = gobj->next;
          if (gobj->next)
            gobj->next->prev = gobj->prev;
          
          this->free_object (&gobj->val);
          delete gobj;
          gobj = next;
        }
    }
    
    // 
    // Sweep SOPs.
    // 
    {
      gc_sop_object *gobj = this->sop_head, *next;
      while (gobj)
        {
          if (gobj->sop.gc_protect || gobj->sop.gc_state != GC_WHITE)
            {
              // live object
              // paint back to white
              gobj->sop.gc_state = GC_WHITE;
              gobj = gobj->next;
              continue;
            }
          
          next = gobj->next;
          //std::cout << "[[gc sop sweep: " << rho_sop_str (&gobj->sop) << " [[at @: " << (void *)&gobj->sop << std::endl;
          
          if (gobj->prev)
            gobj->prev->next = gobj->next;
          else
            this->sop_head = gobj->next;
          if (gobj->next)
            gobj->next->prev = gobj->prev;
          
          rho_sop_free (&gobj->sop);
          delete gobj;
          gobj = next;
        }
    }
  }
  
  
  
  // DEBUG
  void
  v1_gc::print_sop_roots ()
  {
    gc_object_ref *gref = this->sop_roots;
    while (gref)
      {
        //std::cout << "[[gc sop: " << rho_sop_str (gref->data.sop ) << " [[protected: " << gref->data.sop->gc_protect << std::endl;
        gref = gref->next;
      }
  }
  
  
  
  /* 
   * Performs a single collection cycle.
   * Non-incremental collectors would simply perform a full collection every
   * N step() calls.
   */
  void
  v1_gc::step ()
  {
    if (this->cycles++ == V1_STEPS_PER_COLLECT)
      {
        this->collect ();
        this->cycles = 0;
      }
  }
  
  /* 
   * Performs a full garbage collection.
   */
  void
  v1_gc::collect ()
  {
    this->mark_roots ();
    this->process_gray_set ();
    this->sweep ();
    
    this->print_sop_roots ();
  }
}

