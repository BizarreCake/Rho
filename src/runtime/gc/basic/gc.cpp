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

#include "runtime/gc/basic/gc.hpp"
#include "runtime/vm.hpp"
#include <stdexcept>

#include <iostream> // DEBUG


namespace rho {
  
#define ALLOCS_PER_COLLECTION     400
  
  basic_gc::basic_gc (virtual_machine& vm)
    : garbage_collector (vm)
  {
    this->head = nullptr;
    this->gray = nullptr;
    this->t_alloc = 0;
    this->t_free = 0;
  }
  
  basic_gc::~basic_gc ()
  {
    
  }
  
  
  
  gc_value*
  basic_gc::alloc_protected ()
  {
    if (this->t_alloc++ % ALLOCS_PER_COLLECTION == 0)
      this->collect ();
    
    gc_object *obj = new gc_object;
    gc_value *val = &obj->val;
    val->gc_state = GC_WHITE;
    
    // add object to object list
    obj->next = this->head;
    this->head = obj;
    
    val->gc_protected = 1;
    return val;
  }
  
  gc_value*
  basic_gc::alloc_upvalue_protected ()
  {
    auto val = this->alloc_protected ();
    val->type = RHO_UPVAL;
    val->val.uv.val.type = RHO_NIL;
    
    this->upvals.push_back (val);
    
    return val;
  }
  
  const std::list<gc_value*>&
  basic_gc::get_upvalues () const
  {
    return this->upvals;
  }
  
  
  
  /* 
   * Inserts an object into the gray set.
   */
  void
  basic_gc::paint_gray (rho_value& v_)
  {
    if (!rho_type_is_collectable (v_.type))
      return;
  
    gc_value *v = v_.val.gc;
    if (!v || v->gc_state != GC_WHITE)
      return;
      
    v->gc_state = GC_GRAY;
    
    auto ref = new gc_object_ref;
    ref->val = v;
    
    ref->next = this->gray;
    this->gray = ref;
  }
  
  /* 
   * Pops one value from the gray set.
   */
  gc_value*
  basic_gc::pop_gray ()
  {
    if (this->gray == nullptr)
      throw std::runtime_error ("gray set is empty");
    
    auto obj = this->gray;
    gc_value *v = this->gray->val;
    
    this->gray = obj->next;
    delete obj;
    return v;
  }
  
  /* 
   * Inserts all objects the given object directly references into the gray set.
   */
  void
  basic_gc::mark_children (gc_value *v)
  {
    if (!v)
      return;
    
    switch (v->type)
      {
      case RHO_NIL:
      case RHO_BOOL:
      case RHO_INTEGER:
      case RHO_EMPTY_LIST:
      case RHO_PVAR:
      case RHO_INTERNAL:
      case RHO_ATOM:
      case RHO_STR:
      case RHO_FLOAT:
        break;
      
      case RHO_VEC:
        for (int i = 0; i < v->val.vec.len; ++i)
          this->paint_gray (v->val.vec.vals[i]);
        break;
      
      case RHO_UPVAL:
        if (v->val.uv.sp == -1)
          this->paint_gray (v->val.uv.val);
        else
          this->paint_gray (this->vm.get_stack_at (v->val.uv.sp));
        break;
      
      case RHO_FUN:
        {
          auto env = v->val.fn;
          for (int i = 0; i < env.env_len; ++i)
            this->paint_gray (env.env[i]);
        }
        break;
      
      case RHO_CONS:
        this->paint_gray (v->val.p.fst);
        this->paint_gray (v->val.p.snd);
        break;
      }
  }
  
  
  
  void
  basic_gc::step ()
  {
    this->collect ();
  }
  
  void
  basic_gc::collect ()
  {
    // paint all objects white first
    auto obj = this->head;
    while (obj)
      {
        obj->val.gc_state = GC_WHITE;
        obj = obj->next;
      }
    
    // place all references in the root set into the gray set.
    auto stk = this->vm.get_stack (true);
    //std::cout << "GC collect: sp=" << stk.get_sp () << std::endl;
    for (rho_value v : stk)
      this->paint_gray (v);
    for (auto& gp : this->vm.get_globals ())
      {
        for (int i = 0; i < gp.size; ++i)
          this->paint_gray (gp.vals[i]);
      }
    
    while (this->gray)
      {
        // pick an object from the gray set.
        auto v = this->pop_gray ();
        
        // paint this object black
        v->gc_state = GC_BLACK;
        
        // paint all object's child references gray
        this->mark_children (v);
      }
    
    // remove white upvalues
    for (auto itr = this->upvals.begin (); itr != this->upvals.end (); )
      {
        auto uv = *itr;
        if (uv->gc_state == GC_WHITE && !uv->gc_protected)
          itr = this->upvals.erase (itr);
        else
          ++ itr;
      }
    
    // reclaim all objects still colored white
    obj = this->head;
    gc_object *prev = nullptr;
    while (obj)
      {
        if (obj->val.gc_state == GC_BLACK || obj->val.gc_protected)
          {
            prev = obj;
            obj = obj->next;
          }
        else
          {
            auto next = obj->next;
            destroy_gc_value (&obj->val);
            delete obj;
            ++ t_free;
            
            if (this->head == obj)
              this->head = next;
            else
              {
                if (prev)
                  prev->next = next;
              }
              
            obj = next;
          }
      }
  }
}

