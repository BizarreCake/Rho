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

#include "parse/lexer.hpp"
#include <istream>
#include <cctype>
#include <cstring>
#include <unordered_map>


namespace rho {
  
  lexer_stream::lexer_stream (std::istream& strm)
    : strm (strm)
  {
    this->ln = this->col = this->pcol = 1;
  }
  
  
  
  int
  lexer_stream::get ()
  {
    auto c = this->peek ();
    if (c == EOF)
      return EOF;
    
    if (c == '\n')
      {
        ++ this->ln;
        this->pcol = this->col;
        this->col = 1;
      }
    else
      ++ this->col;
    
    this->strm.get ();
    return c;
  }
  
  void
  lexer_stream::unget ()
  {
    this->strm.unget ();
    -- this->col;
    
    if (this->peek () == '\n')
      {
        -- this->ln;
        this->col = this->pcol;
      }
  }
  
  int
  lexer_stream::peek ()
  {
    auto c = this->strm.peek ();
    if (c == std::istream::traits_type::eof ())
      return EOF;
    return c;
  }
  
  
  
//------------------------------------------------------------------------------
  
  lexer::token_stream::token_stream ()
    : toks (new std::vector<token> (),
        [] (std::vector<token> *v) {
          for (token& tok : *v)
            destroy_token (tok);
          delete v;
        })
  {
    this->pos = 0;
  }
  
  
  
  token
  lexer::token_stream::prev ()
  {
    return (*this->toks)[-- this->pos];
  }
  
  token
  lexer::token_stream::peek_prev ()
  {
    return (*this->toks)[this->pos - 1];
  }
  
  
  token
  lexer::token_stream::next ()
  {
    return (*this->toks)[this->pos ++];
  }
  
  token
  lexer::token_stream::peek_next ()
  {
    return (*this->toks)[this->pos];
  }
  
  
  bool
  lexer::token_stream::has_prev () const
  {
    return this->pos > 0;
  }
  
  bool
  lexer::token_stream::has_next () const
  {
    return this->pos < (int)(this->toks->size () - 1);
  }
  
  int
  lexer::token_stream::available () const
  {
    return this->toks->size () - this->pos;
  }
  
  
  
//------------------------------------------------------------------------------
  
  lexer::lexer ()
  {
    this->strm = nullptr;
    this->ws_skipped = 0;
  }
  
  lexer::~lexer ()
  {
    delete this->strm;
  }
  
  
  
  /* 
   * Tokenizes the specified stream of characters and returns a stream of
   * tokens.
   */
  lexer::token_stream
  lexer::tokenize (std::istream& strm)
  {
    token_stream ts;
    
    delete this->strm;
    this->strm = new lexer_stream (strm);
    
    for (;;)
      {
        auto tok = this->read_token ();
        if (tok.type == TOK_EOF)
          {
            ts.toks->push_back (tok);
            break;
          }
        else if (tok.type == TOK_INVALID)
          throw lexer_error ("invalid token", tok.ln, tok.col);
        
        ts.toks->push_back (tok);
      }
    
    return ts;
  }
  
  
  
  void
  lexer::skip_whitespace ()
  {
    while (std::isspace (this->strm->peek ()))
      {
        this->strm->get ();
        ++ this->ws_skipped;
      }
    
    auto c = this->strm->peek ();
    if (c == '/')
      {
        this->strm->get ();
        c = this->strm->peek ();
        if (c == '/')
          {
            // one line comment
            while (c != EOF && c != '\n')
              c = this->strm->get ();
            this->skip_whitespace ();
          }
        else if (c == '*')
          {
            // multi line comment
            this->strm->get ();
            for (;;)
              {
                c = this->strm->get ();
                if (c == EOF)
                  return;
                else if (c == '*')
                  {
                    if (this->strm->get () == '/')
                      break;
                  }
              }
            this->skip_whitespace ();
          }
        else
          {
            this->strm->unget ();
          }
      }
  }
  
  
  
  bool
  lexer::try_read_punctuation (token& tok)
  {
    int c = this->strm->peek ();
    
    switch (c)
      {
      case '(': this->strm->get (); tok.type = TOK_LPAREN; return true;
      case ')': this->strm->get (); tok.type = TOK_RPAREN; return true;
      case '{': this->strm->get (); tok.type = TOK_LBRACE; return true;
      case '}': this->strm->get (); tok.type = TOK_RBRACE; return true;
      case '[': this->strm->get (); tok.type = TOK_LBRACKET; return true;
      case ']': this->strm->get (); tok.type = TOK_RBRACKET; return true;
      case ';': this->strm->get (); tok.type = TOK_SCOL; return true;
      case ',': this->strm->get (); tok.type = TOK_COMMA; return true;
      case '.': this->strm->get (); tok.type = TOK_DOT; return true;
      
      case '+': this->strm->get (); tok.type = TOK_ADD; return true;
      case '-': this->strm->get (); tok.type = TOK_SUB; return true;
      case '*': this->strm->get (); tok.type = TOK_MUL; return true;
      case '^': this->strm->get (); tok.type = TOK_POW; return true;
      case '%': this->strm->get (); tok.type = TOK_PERC; return true;
      case '&': this->strm->get (); tok.type = TOK_AND; return true;
      case '|': this->strm->get (); tok.type = TOK_OR; return true;
      case '!': this->strm->get (); tok.type = TOK_NOT; return true;
      
      case '/':
        this->strm->get (); 
        if (this->strm->peek () == '=')
          { this->strm->get (); tok.type = TOK_NEQ; }
        else
          tok.type = TOK_DIV;
        return true;
      
      case '=':
        this->strm->get (); 
        if ((c = this->strm->peek ()) == '=')
          { this->strm->get (); tok.type = TOK_EQ; }
        else if (c == '>')
          { this->strm->get (); tok.type = TOK_RDARROW; }
        else
          tok.type = TOK_ASSIGN;
        return true;
      
      case '<':
        this->strm->get (); 
        if (this->strm->peek () == '=')
          { this->strm->get (); tok.type = TOK_LTE; }
        else
          tok.type = TOK_LT;
        return true;
      
      case '>':
        this->strm->get (); 
        if (this->strm->peek () == '=')
          { this->strm->get (); tok.type = TOK_GTE; }
        else
          tok.type = TOK_GT;
        return true;
      
      case '\'':
        this->strm->get (); 
        if (this->strm->peek () == '(')
          { this->strm->get (); tok.type = TOK_LIST_START; }
        else
          {
            this->strm->unget ();
            return false;
          }
        return true;
      
      case ':':
        this->strm->get (); 
        if (this->strm->peek () == '=')
          { this->strm->get (); tok.type = TOK_DEF; }
        else
          tok.type = TOK_COL;
        return true;
      }

    return false;
  }
  
  
  bool
  lexer::try_read_string (token& tok)
  {
    int c = this->strm->peek ();
    if (c != '"')
      return false;
    this->strm->get ();
    
    std::string str;
    for (;;)
      {
        c = this->strm->get ();
        if (c == '"')
          break;
        else if (c == EOF)
          return false;
        else if (c == '\\')
          {
            c = this->strm->get ();
            switch (c)
              {
              case '"': str.push_back ('"'); break;
              case 'n': str.push_back ('\n'); break;
              case 't': str.push_back ('\t'); break;
              case 'b': str.push_back ('\b'); break;
              case 'r': str.push_back ('\r'); break;
              case '0': str.push_back ('\0'); break;
              case '\\': str.push_back ('\\'); break;
              
              default:
                // TODO: issue warning
                str.push_back (c);
                break;
              
              case EOF:
                return false;
              }
          }
        else
          str.push_back (c);
      }
    
    tok.type = TOK_STRING;
    tok.val.str = new char [str.size () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    return true;
  }
  
  
  bool
  lexer::try_read_number (token& tok)
  {
    int c = this->strm->peek ();
    if (!std::isdigit (c))
      return false;
    
    bool got_dot = false;
    std::string str;
    for (;;)
      {
        c = this->strm->peek ();
        if (std::isdigit (c))
          str.push_back (this->strm->get ());
        else if (c == '.')
          {
            if (got_dot)
              break;
            
            str.push_back (this->strm->get ());
            got_dot = true;
          }
        else
          break;
      }
    
    tok.type = got_dot ? TOK_FLOAT : TOK_INTEGER;
    tok.val.str = new char [str.size () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    
    return true;
  }
  
  
  
  static inline bool
  _is_ident_first_char (int c)
    { return std::isalpha (c) || c == '_' || c == '$'; }
  
  static inline bool
  _is_ident_char (int c)
    { return _is_ident_first_char (c) || std::isdigit (c) || c == '?' || c == '!'; }
  
  
  static token_type
  _check_keyword (const std::string& str)
  {
    static std::unordered_map<std::string, token_type> _map {
      { "var", TOK_VAR },
      { "fun", TOK_FUN },
      { "if", TOK_IF },
      { "then", TOK_THEN },
      { "else", TOK_ELSE },
      { "match", TOK_MATCH },
      { "case", TOK_CASE },
      { "nil", TOK_NIL },
      { "module", TOK_MODULE },
      { "import", TOK_IMPORT },
      { "export", TOK_EXPORT },
      { "ret", TOK_RET },
      { "namespace", TOK_NAMESPACE },
      { "true", TOK_TRUE },
      { "false", TOK_FALSE },
      { "atom", TOK_ATOMK },
      { "using", TOK_USING },
      { "let", TOK_LET },
      { "in", TOK_IN },
      { "N", TOK_N },
    };
    
    auto itr = _map.find (str);
    if (itr == _map.end ())
      return TOK_INVALID;
    return itr->second;
  }
  
  
  bool
  lexer::try_read_ident (token& tok)
  {
    int c = this->strm->peek ();
    if (!_is_ident_first_char (c))
      return false;
    
    if (c == 'N')
      {
        // handle special case
        this->strm->get ();
        if (this->strm->peek () == ':')
          {
            tok.type = TOK_N;
            return true;
          }
        else
          this->strm->unget ();
      }
    
    bool is_atom = false;
    int ccol = 0;
    std::string str;
    str.push_back (this->strm->get ());
    for (;;)
      {
        c = this->strm->peek ();
        if (_is_ident_char (c))
          {
            str.push_back (this->strm->get ());
            ccol = 0;
          }
        else if (c == ':')
          {
            if (ccol++ > 0)
              return false;
            str.push_back (this->strm->get ());
          }
        else if (c == '#' && ccol == 1)
          {
            str.push_back (this->strm->get ());
            is_atom = true;
          }
        else
          break;
      }
    
    tok.type = _check_keyword (str);
    if (tok.type == TOK_INVALID)
      {
        tok.type = is_atom ? TOK_ATOM : TOK_IDENT;
        tok.val.str = new char [str.size () + 1];
        std::strcpy (tok.val.str, str.c_str ());
      }
    
    return true;
  }
  
  
  
  bool
  lexer::try_read_atom (token& tok)
  {
    int c = this->strm->peek ();
    if (c != '#')
      return false;
    
    std::string str;
    str.push_back (this->strm->get ());
    while (_is_ident_char (this->strm->peek ()))
      str.push_back (this->strm->get ());
    
    tok.type = TOK_ATOM;
    tok.val.str = new char [str.size () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    return true;
  }
  
  
  
  token
  lexer::read_token ()
  {
    this->skip_whitespace ();
    
    token tok;
    tok.type = TOK_INVALID;
    tok.ln = this->strm->get_line ();
    tok.col = this->strm->get_column ();
    
    if (this->strm->peek () == EOF)
      {
        tok.type = TOK_EOF;
        return tok;
      }
    
#define TRY_READ(CAT) \
  if (this->try_read_##CAT (tok)) \
    { this->ws_skipped = 0; return tok; }
    
    TRY_READ (punctuation)
    TRY_READ (string)
    TRY_READ (number)
    TRY_READ (atom)
    TRY_READ (ident)
    
    return tok;
  }
}

