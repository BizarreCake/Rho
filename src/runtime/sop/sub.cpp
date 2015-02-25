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

#include "runtime/sop.hpp"
#include "runtime/value.hpp"
#include "runtime/vm.hpp"
#include "runtime/gc/gc.hpp"


namespace rho {
  
  rho_sop*
  rho_sop_sub (rho_sop *a, rho_sop *b, virtual_machine& vm)
  {
    rho_sop *sop = vm.get_gc ().alloc_sop_protected ();
    sop->type = SOP_SUB;
    rho_sop_arr_init (sop);
    rho_sop_arr_add (sop, a);
    rho_sop_arr_add (sop, b);
    rho_sop_unprotect (a);
    rho_sop_unprotect (b);
    return sop;
  }
}

