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

#include "runtime/gc/gc.hpp"
#include <unordered_map>
#include <string>
#include <stdexcept>

// gcs:
#include "runtime/gc/basic/gc.hpp"


namespace rho {
  
  garbage_collector::garbage_collector (virtual_machine& vm)
    : vm (vm)
  {
    
  }
  
  
  
  static garbage_collector*
  _create_basic (virtual_machine& vm)
  {
    return new basic_gc (vm);
  }
  
  /* 
   * Factory function for creating garbage collectors.
   */
  garbage_collector*
  garbage_collector::create (const char *name, virtual_machine& vm)
  {
    static std::unordered_map<std::string, garbage_collector* (*)(virtual_machine&)> _map {
      { "basic", &_create_basic },
    };
    
    auto itr = _map.find (name);
    if (itr == _map.end ())
      throw std::runtime_error ("unknown gc");
    return itr->second (vm);
  }
}

