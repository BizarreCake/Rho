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

#include "runtime/gc/gc.hpp"
#include "runtime/vm.hpp"
#include <unordered_map>
#include <stdexcept>

// GCs:
#include "runtime/gc/v1/v1.hpp"


namespace rho {
  
  garbage_collector::garbage_collector (virtual_machine& vm)
    : vm (vm)
    { }
  
  
  
//------------------------------------------------------------------------------
  
  namespace {
    typedef garbage_collector* (*gc_create_fn) (virtual_machine&);
  }
  
  
  static garbage_collector*
  _create_v1 (virtual_machine& vm)
    { return new v1_gc (vm); }
  
  
  /* 
   * Instantiates a garbage collector with the specified name.
   */
  garbage_collector*
  garbage_collector::create (const std::string& name, virtual_machine& vm)
  {
    static const std::unordered_map<std::string, gc_create_fn> _gc_map {
      { "V1", &_create_v1 },
    };
    
    auto itr = _gc_map.find (name);
    if (itr == _gc_map.end ())
      throw std::runtime_error ("unknown garbage collector");
    
    return itr->second (vm);
  }
}

