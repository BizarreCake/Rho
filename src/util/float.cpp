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

#include "util/float.hpp"
#include <cmath>
#include <sstream>


namespace rho {
  
  /* 
   * Computes how much bits are required to represent a floating point number
   * with at least :digits: decimal digits after the decimal point.
   */
  int
  prec_base10_to_bits (int digits)
  {
    int bits = std::ceil (digits * 3.5);
    bits += 16; // add a few more bits just in case...
    
    // round up to a multiple of 32.
    if (bits % 32 != 0)
      bits += 32 - bits % 32;
    
    return bits;
  }
  
  
  
  static void
  _sanitize_float_str (std::string& str)
  {
    while (!str.empty () && str.back () == '0')
      str.pop_back ();
    
    if (str.empty ())
      str = "0";
    else if (str.back () == '.')
      str.push_back ('0');
  }
  
  /* 
   * Converts the specified floating number into a string in base 10, with the
   * specified amount of digits after the decimal point.
   */
  std::string
  float_to_str (mpfr_t f, int prec10)
  {
    std::ostringstream ss;
    ss << "%." << prec10 << "RNf";
    
    char *buf;
    mpfr_asprintf (&buf, ss.str ().c_str (), f);
    
    std::string str (buf);
    mpfr_free_str (buf);
    
    _sanitize_float_str (str);
    return str;
  }
}

