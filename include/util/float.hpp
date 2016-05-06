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

#ifndef _RHO__UTIL__FLOAT__H_
#define _RHO__UTIL__FLOAT__H_

#include <mpfr.h>
#include <string>


namespace rho {
  
  /* 
   * Computes how much bits are required to represent a floating point number
   * with at least :digits: decimal digits after the decimal point.
   */
  int prec_base10_to_bits (int digits);
  
  /* 
   * Converts the specified floating number into a string in base 10, with the
   * specified amount of digits after the decimal point.
   */
  std::string float_to_str (mpfr_t f, int prec10);
}

#endif

