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

#ifndef _RHO__LINKER__LINKER__H_
#define _RHO__LINKER__LINKER__H_

#include "common/errors.hpp"
#include "linker/module.hpp"


namespace rho {
  
  /* 
   * The linker takes one or more modules as input and produces an executable
   * (a special type of module) which will contain the modules' sections
   * stringed together in such a way that offsets within the modules' code are
   * meaningfully preserved.
   */
  class linker
  {
    error_tracker& errs;
    
    std::vector<module *> mods;
    module *pmod; // primary module
    
  public:
    linker (error_tracker& errs);
    ~linker ();
    
  public:
    /* 
     * Inserts the specified module into the linker's input module list, and
     * marks it a primary module.
     * 
     * NOTE: The ownership of the module will be passed to the linker (it will
     *       be destroyed by the linker)
     */
    void add_primary_module (module *mod);
    
    /* 
     * Adds a non-primary module to the input module list.
     * 
     * NOTE: The ownership of the module will be passed to the linker (it will
     *       be destroyed by the linker)
     */
    void add_module (module *mod);
    
    
    
    /*
     * Links the inserted modules together and forms an executable.
     */
    executable* link ();
  };
}

#endif

