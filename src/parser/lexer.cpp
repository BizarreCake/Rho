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

#include "parser/lexer.hpp"
#include "common/errors.hpp"
#include <stack>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>


namespace rho {
  
  static void
  _destroy_token (token tok)
  {
    switch (tok.type)
      {
      case TOK_INVALID:
      case TOK_IDENT:
      case TOK_INTEGER:
      case TOK_SYM:
        delete[] tok.val.str;
        break;
      
      default: ;
      }
  }
  
  
  
//------------------------------------------------------------------------------
  
  token_stream::token_stream (const std::vector<token>& toks)
    : toks (toks)
  {
    this->pos = 0;
  }
  
  
  
  bool
  token_stream::has_next ()
    { return this->pos < (int)this->toks.size (); }
  
  token
  token_stream::next ()
  {
    if (this->pos == (int)this->toks.size () - 1)
      return this->toks.back ();
    return this->toks[this->pos++];
  }
  
  token
  token_stream::peek_next ()
    { return this->toks[this->pos]; }
  
  
  bool
  token_stream::has_prev ()
    { return this->pos > 0; }
  
  token
  token_stream::prev ()
  {
    if (this->pos == 0)
      return this->toks.front ();
    return this->toks[--this->pos];
  }
  
  token
  token_stream::peek_prev ()
    { return this->toks[this->pos - 1]; }
  
  
  
//------------------------------------------------------------------------------
  
  namespace {
    
#ifndef EOF
# define EOF (-1)
#endif
    
    struct lex_pos {
      int ln, col;
    };
    
    struct saved_lex_pos {
      lex_pos pos;
      std::istream::traits_type::pos_type strm_pos;
    };
    
    /* 
     * Keeps track of line and column numbers.
     */
    class lexer_stream
    {
      std::istream& strm;
      
      lex_pos pos;
      std::stack<lex_pos> prev;
      std::stack<saved_lex_pos> saved;
      
    public:
      inline int get_line () const { return this->pos.ln; }
      inline int get_column () const { return this->pos.col; }
      
    public:
      lexer_stream (std::istream& strm)
        : strm (strm)
      {
        this->pos = { 1, 1 };
      }
      
   public:
      int
      get ()
      {
        int c = this->strm.get ();
        if (c == std::istream::traits_type::eof ())
          return EOF;
        if (c == '\n')
          {
            this->prev.push (this->pos);
            this->pos = { this->pos.ln + 1, 1 };
          }
        else
          ++ this->pos.col;
        return c;
      }
      
      
      void
      unget ()
      {
        this->strm.seekg (-1, std::ios_base::cur);
        int c = this->strm.peek ();
        if (c == '\n')
          {
            this->pos = this->prev.top ();
            this->prev.pop ();
          }
        else
          -- this->pos.col;
      }
      
      void
      unget (int count)
      {
        while (count --> 0)
          this->unget ();
      }
      
      
      int
      peek ()
      {
        int c = this->strm.peek ();
        if (c == std::istream::traits_type::eof ())
          return EOF;
        return c;
      }
      
    //--------
      
      void
      push ()
        { this->saved.push ({ this->pos, this->strm.tellg () }); }
      
      int
      pop ()
      {
        auto s = this->saved.top ();
        this->saved.pop ();
        
        return (this->strm.tellg () - s.strm_pos);
      }
      
      void
      restore ()
      {
        auto s = this->saved.top ();
        this->saved.pop ();
        
        this->pos = s.pos;
        this->strm.seekg (s.strm_pos);
      }
    };
  }  
  


//------------------------------------------------------------------------------
  
  lexer::lexer (error_tracker& errs)
    : errs (errs)
    { }
  
  lexer::~lexer ()
  {
    for (token tok : this->toks)
      _destroy_token (tok);
  }
  
  
  
  namespace {
    
    struct lexer_state
    {
      error_tracker& errs;
      std::string file_name;
      
    public:
      lexer_state (error_tracker& errs, const std::string& file_name)
        : errs (errs), file_name (file_name)
        { }
    };
  }
  
  
  static void
  _skip_whitespace (lexer_stream& strm)
  {
    int c;
    while ((c = strm.peek ()) != EOF && std::isspace (c))
      strm.get ();
  }
  
  
  
  static bool
  _is_first_ident_char (char c)
    { return std::isalpha (c) || c == '_'; }
  
  static bool
  _is_ident_char (char c)
    { return std::isalnum (c) || c == '_'; }
  
    
  
  // mostly punctuation characters
  static bool
  _try_read_punctuation (token& tok, lexer_stream& strm, lexer_state& lstate)
  {
    int c = strm.peek ();
    switch (c)
      {
      case '\'':
        strm.get ();
        if (strm.get () == '(')
          { tok.type = TOK_LPAREN_LIST; return true; }
        strm.unget (2);
        break;
      
      case '$':
        strm.get ();
        
        if (strm.peek () == '{')
          { strm.get (); tok.type = TOK_LBRACE_SET; return true; }
        else if (strm.peek () == '$')
          { strm.get (); tok.type = TOK_THIS_FUNC; return true; }
        strm.unget ();
        break;
      
      case ':':
        strm.get ();
        if (strm.get () == ':')
          { tok.type = TOK_DCOL; return true; }
        strm.unget (2);
        break;
      
      case '=':
        strm.get ();
        if (strm.peek () == '=')
          {
            strm.get ();
            tok.type = TOK_EQ;
            return true;
          }
        else if (strm.peek () == '/')
          {
            strm.get ();
            if (strm.peek () == '=')
              {
                strm.get ();
                tok.type = TOK_NEQ;
                return true;
              }
            else
              strm.unget ();
          }
        tok.type = TOK_ASSIGN;
        return true;
      
      case '<':
        strm.get ();
        if (strm.peek () == '=')
          {
            strm.get ();
            tok.type = TOK_LTE;
            return true;
          }
        else if (strm.peek () == '-')
          {
            strm.get ();
            tok.type = TOK_RARROW;
            return true;
          }
        tok.type = TOK_LT;
        return true;
      
      case '>':
        strm.get ();
        if (strm.peek () == '=')
          {
            strm.get ();
            tok.type = TOK_GTE;
            return true;
          }
        tok.type = TOK_GT;
        return true;
      
      case '-':
        strm.get ();
        if (std::isdigit (strm.peek ()))
          {
            // a number
            strm.unget ();
            return false;
          }
        tok.type = TOK_SUB;
        return true;
      
      case '{':
        strm.get ();
        if (strm.peek () == ':')
          {
            strm.get ();
            tok.type = TOK_BLOCKIFY;
            return true;
          }
        tok.type = TOK_LBRACE;
        return true;
      
      case '(':
        strm.get ();
        if (strm.peek () == ':')
          {
            strm.get ();
            tok.type = TOK_PARENIFY;
            return true;
          }
        tok.type = TOK_LPAREN;
        return true;
      
      case 'N':
        strm.get ();
        if (strm.peek () == ':')
          {
            strm.get ();
            tok.type = TOK_N;
            return true;
          }
        strm.unget ();
        return false;
      
      case '.':
        strm.get ();
        if (strm.peek () == '.')
          {
            strm.get ();
            tok.type = TOK_RANGE;
            return true;
          }
        strm.unget ();
        return false;
      
      case '/':
        strm.get ();
        if (strm.peek () == '/')
          {
            strm.get ();
            tok.type = TOK_IDIV;
          }
        else
          tok.type = TOK_DIV;
        return true;
      
      case '|':
        strm.get ();
        if (strm.peek () == '.')
          {
            strm.get ();
            tok.type = TOK_SUBST;
            return true;
          }
        strm.unget ();
        return false;
      
      case '}': strm.get (); tok.type = TOK_RBRACE; return true;
      case ')': strm.get (); tok.type = TOK_RPAREN; return true;
      case '[': strm.get (); tok.type = TOK_LBRACKET; return true;
      case ']': strm.get (); tok.type = TOK_RBRACKET; return true;
      case ';': strm.get (); tok.type = TOK_SCOL; return true;
      case ',': strm.get (); tok.type = TOK_COMMA; return true;
      case '+': strm.get (); tok.type = TOK_ADD; return true;
      case '*': strm.get (); tok.type = TOK_MUL; return true;
      case '%': strm.get (); tok.type = TOK_MOD; return true;
      case '^': strm.get (); tok.type = TOK_CARET; return true;
      case '!': strm.get (); tok.type = TOK_BANG; return true;
      }
    
    return false;
  }
  
  
  
  static bool
  _try_read_keyword (token& tok, lexer_stream& strm, lexer_state& lstate)
  {
    int c;
    
    strm.push ();
    
    std::string str;
    while (std::isalpha (c = strm.peek ()))
      str.push_back (strm.get ());
    if (str.empty ())
      { strm.pop (); return false; }
    
    static const std::unordered_map<std::string, token_type> _keyword_map {
      { "let", TOK_LET },
      { "fun", TOK_FUN },
      { "if", TOK_IF },
      { "then", TOK_THEN },
      { "else", TOK_ELSE },
      { "otherwise", TOK_OTHERWISE },
      { "sum", TOK_SUM },
      { "product", TOK_PRODUCT },
      { "nil", TOK_NIL },
      
      // data types:
      { "Int", TOK_INT },
      { "Set", TOK_SET },
    };
    auto itr = _keyword_map.find (str);
    if (itr == _keyword_map.end ())
      { strm.restore (); return false; }
    
    strm.pop ();
    tok.type = itr->second;
    return true;
  }
  
  
  
  static bool
  _try_read_ident (token& tok, lexer_stream& strm, lexer_state& lstate)
  {
    if  (!_is_first_ident_char (strm.peek ()))
      return false;
    
    int c;
    std::string str;
    do
      str.push_back (strm.get ());
    while (_is_ident_char (c = strm.peek ()));
    
    tok.type = TOK_IDENT;
    tok.val.str = new char[str.length () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    return true;
  }
  
  
  
  static bool
  _try_read_symbol (token& tok, lexer_stream& strm, lexer_state& lstate)
  {
    if (strm.peek () != '\'')
      return false;
    strm.get ();
    
    if  (!_is_first_ident_char (strm.peek ()))
      {
        strm.unget ();
        return false;
      }
    
    int c;
    std::string str;
    do
      str.push_back (strm.get ());
    while (_is_ident_char (c = strm.peek ()));
    
    tok.type = TOK_SYM;
    tok.val.str = new char[str.length () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    return true;
  }
  
  
  
  static bool
  _try_read_number (token& tok, lexer_stream& strm, lexer_state& lstate)
  {
    bool dot = false;
    
    int c = strm.peek ();
    std::string str;
    if (!std::isdigit (c))
      {
        if (c == '-')
          {
            strm.get ();
            if (!std::isdigit (strm.peek ()))
              {
                strm.unget ();
                return false;
              }
            
            str.push_back ('-');
          }
        else
          return false;
      }
    
    
    for (;;)
      {
        c = strm.peek ();
        if (std::isdigit (c))
          str.push_back (strm.get ());
        else if (c == '.')
          {
            if (dot)
              break;
            
            strm.get ();
            if (strm.peek () == '.')
              {
                // range ..
                strm.unget ();
                break;
              }
              
            dot = true;
            str.push_back ('.');
          }
        else
          break;
      }
    
    tok.type = dot ? TOK_REAL : TOK_INTEGER;
    tok.val.str = new char[str.length () + 1];
    std::strcpy (tok.val.str, str.c_str ());
    return true;
  }
  
  
  
  static token
  _read_token (lexer_stream& strm, lexer_state& lstate)
  {
    _skip_whitespace (strm);
    
#define RETURN_TOKEN        \
  {                         \
    tok.len = strm.pop ();  \
    return tok;             \
  }
    
    token tok;
    tok.type = TOK_INVALID;
    tok.ln = strm.get_line ();
    tok.col = strm.get_column ();
    tok.len = 0;
    strm.push (); 
    
    int c = strm.peek ();
    if (c == EOF)
      { tok.type = TOK_EOF; return tok; }
    
  //----
    if (_try_read_punctuation (tok, strm, lstate))
      RETURN_TOKEN
    
    if (_try_read_symbol (tok, strm, lstate))
      RETURN_TOKEN
    
    if (_try_read_keyword (tok, strm, lstate))
      RETURN_TOKEN
    
    if (_try_read_ident (tok, strm, lstate))
      RETURN_TOKEN
    
    if (_try_read_number (tok, strm, lstate))
      RETURN_TOKEN
  //----
    
    // invalid token
    // skip characters until whitespace is found.
    std::string str;
    while ((c = strm.peek ()) != EOF && !std::isspace (c))
      str.push_back (strm.get ());
    tok.val.str = new char [str.length() + 1];
    std::strcpy (tok.val.str, str.c_str ());
    
    RETURN_TOKEN
  }
  
  
  
  /* 
   * Generates a sequence of tokens from the specified character stream.
   * The file name is used for error-tracking purposes.
   */
  void
  lexer::tokenize (std::istream& strm, const std::string& file_name)
  {
    lexer_stream ss { strm };
    lexer_state lstate { this->errs, file_name };
    
    this->toks.clear ();
    for (;;)
      {
        token tok = _read_token (ss, lstate);
        this->toks.push_back (tok);
        if (tok.type == TOK_EOF)
          break;
      }
  }
  
  
  
  /* 
   * Returns a stream with all the generated tokens.
   */
  token_stream
  lexer::get_tokens ()
    { return token_stream (this->toks); }
}

