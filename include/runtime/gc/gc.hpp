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

#ifndef _RHO__RUNTIME__GC__H_
#define _RHO__RUNTIME__GC__H_

#include "runtime/value.hpp"
#include "runtime/sop.hpp"
#include <string>


namespace rho {
  
  class virtual_machine;
  
  /* 
   * The common interface all garbage collectors implement.
   */
  class garbage_collector
  {
  protected:
    virtual_machine& vm;
    
  public:
    garbage_collector (virtual_machine& vm);
    virtual ~garbage_collector () { }
    
  public:
    /* 
     * Instantiates a garbage collector with the specified name.
     */
    static garbage_collector* create (const std::string& name,
      virtual_machine& vm);
    
  public:
    /* 
     * Allocates and returns a new protected Rho value.
     * The garbage collector will not attempt to reclaim the object until its
     * protection is removed.
     */
    virtual rho_value* alloc_protected () = 0;
    
    /* 
     * Allocates a new protected SOP (symbolic expression).
     */
    virtual rho_sop* alloc_sop_protected () = 0;
    
    
    /* 
     * Grants GC protection to the specified object.
     */
    virtual void protect (rho_sop *sop) = 0;
    
  //----
    
    /* 
     * Performs a single collection cycle.
     * Non-incremental collectors would simply perform a full collection every
     * N step() calls.
     */
    virtual void step () = 0;
    
    /* 
     * Performs a full garbage collection.
     */
    virtual void collect () = 0;
  };
}

#endif

