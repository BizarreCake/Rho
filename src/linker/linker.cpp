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

#include "linker/linker.hpp"
#include <stdexcept>


namespace rho {
  
  linker::linker (error_tracker& errs)
    : errs (errs)
  {
    this->pmod = nullptr;
  }
  
  linker::~linker ()
  {
    for (module *mod : this->mods)
      delete mod;
    delete this->pmod;
  }
  
  
  
  /* 
   * Inserts the specified module into the linker's input module list, and
   * marks it a primary module.
   * 
   * NOTE: The ownership of the module will be passed to the linker (it will
   *       be destroyed by the linker)
   */
  void
  linker::add_primary_module (module *mod)
  {
    this->pmod = mod;
  }
  
  /* 
   * Adds a non-primary module to the input module list.
   * 
   * NOTE: The ownership of the module will be passed to the linker (it will
   *       be destroyed by the linker)
   */
  void
  linker::add_module (module *mod)
  {
    this->mods.push_back (mod);
  }
  
  
  
  /*
   * Links the inserted modules together and forms an executable.
   */
  executable*
  linker::link ()
  {
    // TODO
    // currently the primary module is just converted into an executable
    // without considering any other modules.
    
    if (!this->pmod)
      throw std::runtime_error ("primary module not specified");
    
    executable *exec = new executable ();
    
    for (auto sect : this->pmod->get_sections ())
      {
        exec->add_section (sect->name)
          ->data.put_bytes (sect->data.get_data (), sect->data.get_size ());
      }
    
    return exec;
  }
}

