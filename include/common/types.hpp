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

#ifndef _RHO__COMMON__TYPES__H_
#define _RHO__COMMON__TYPES__H_


/* 
 * Common type-related definitions.
 */
namespace rho {
  
  /* 
   * Represents a simple parameterless type.
   */
  enum basic_type
  {
    TYPE_INVALID,
    TYPE_UNSPEC,       // unspecified
    
    TYPE_SYM,
    TYPE_SET,
    TYPE_INT,
    TYPE_RAT,
    TYPE_REAL,
    TYPE_FUNC,
    TYPE_MAT,
  };
}

#endif

