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

#ifndef _RHO__PARSE__LEXER__H_
#define _RHO__PARSE__LEXER__H_

#include "parse/token.hpp"
#include <stdexcept>
#include <string>
#include <iosfwd>
#include <memory>
#include <vector>


namespace rho {
  
#ifndef EOF
# define EOF (-1)
#endif
  
  /* 
   * Thrown by the lexer when it fails to tokenize faulty input.
   */ 
  class lexer_error: public std::runtime_error
  {
    int ln, col;
    
  public:
    inline int get_line () const { return this->ln; }
    inline int get_column () const { return this->col; }
    
  public:
    lexer_error (const std::string& str, int ln, int col)
      : std::runtime_error (str), ln (ln), col (col)
      { }
  };
  
  
  
  /* 
   * Wraps on top of a regular character stream and keeps track of the current
   * line and column numbers.
   */
  class lexer_stream
  {
    std::istream& strm;
    int ln, col, pcol;
    
  public:
    inline int get_line () const { return this->ln; }
    inline int get_column () const { return this->col; }
    
  public:
    lexer_stream (std::istream& strm);
    
  public:
    int get ();
    void unget ();
    int peek ();
  };
  
  
  
  /* 
   * The lexer/tokenizer produces a stream of tokens from a sequence of
   * characters, which could then be fed to the parser as input.
   */
  class lexer
  {
    lexer_stream *strm;
    int ws_skipped;
    
  public:
    class token_stream
    {
      friend class lexer;
      
      std::shared_ptr<std::vector<token>> toks;
      int pos;
      
    private:
      token_stream ();
      
    public:
      token prev ();
      token peek_prev ();
      
      token next ();
      token peek_next ();
      
      bool has_prev () const;
      bool has_next () const;
      int available () const;
    };
  
  public:
    lexer ();
    ~lexer ();
  
  public:
    /* 
     * Tokenizes the specified stream of characters and returns a stream of
     * tokens.
     */
    token_stream tokenize (std::istream& strm);
    
  private:
    token read_token ();
    bool try_read_punctuation (token& tok);
    bool try_read_string (token& tok);
    bool try_read_integer (token& tok);
    bool try_read_atom (token& tok);
    bool try_read_ident (token& tok);
    
    void skip_whitespace ();
  };
}

#endif

