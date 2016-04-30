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

#include "runtime/value.hpp"
#include "runtime/gc/gc.hpp"
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <vector>


namespace rho {
  
  bool
  rho_type_is_collectable (rho_type type)
  {
    switch (type)
      {
      case RHO_INTERNAL:
      case RHO_PVAR:
      case RHO_NIL:
      case RHO_BOOL:
      case RHO_ATOM:
        return false;
      
      case RHO_UPVAL:
      case RHO_VEC:
      case RHO_INTEGER:
      case RHO_FUN:
      case RHO_EMPTY_LIST:
      case RHO_CONS:
      case RHO_STR:
        return true;
      }
    
    throw std::runtime_error ("shouldn't happen");
  }
  
  
  
  /* 
   * Reclaims memory used by the specified Rho value (but does not free the
   * value itself).
   */
  void
  destroy_rho_value (rho_value& v)
  {
    if (rho_type_is_collectable (v.type))
      destroy_gc_value (v.val.gc);
  }
  
  void
  destroy_gc_value (gc_value *v)
  {
    switch (v->type)
      {
      case RHO_NIL:
      case RHO_EMPTY_LIST:
      case RHO_CONS:
      case RHO_PVAR:
      case RHO_INTERNAL:
      case RHO_BOOL:
      case RHO_UPVAL:
      case RHO_ATOM:
        break;
      
      case RHO_INTEGER:
        mpz_clear (v->val.i);
        break;
      
      case RHO_VEC:
        delete[] v->val.vec.vals;
        break;
      
      case RHO_FUN:
        delete[] v->val.fn.env;
        break;
      
      case RHO_STR:
        delete[] v->val.s.str;
        break;
      }
  }
  
  
  
  /*
   * Applies gc_unprotect() recursively.
   */
  void
  gc_unprotect_rec (rho_value& v)
  {
    gc_unprotect (v);
    
    switch (v.type)
      {
      case RHO_NIL:
      case RHO_BOOL:
      case RHO_EMPTY_LIST:
      case RHO_PVAR:
      case RHO_INTEGER:
      case RHO_INTERNAL:
      case RHO_ATOM:
      case RHO_STR:
        break;
      
      case RHO_UPVAL:
        gc_unprotect_rec (v.val.gc->val.uv.val);
        break;
      
      case RHO_FUN:
        // TODO
        break;
      
      case RHO_CONS:
        gc_unprotect_rec (v.val.gc->val.p.fst);
        gc_unprotect_rec (v.val.gc->val.p.snd);
        break;
      
      case RHO_VEC:
        for (int i = 0; i < v.val.gc->val.vec.len; ++i)
          gc_unprotect_rec (v.val.gc->val.vec.vals[i]);
        break;
      }
  }
  
  
  
  static std::string
  _escape_string (const char *str, long len)
  {
    std::string res;
    res.push_back ('"');
    
    for (long i = 0; i < len; ++i)
      {
        switch (str[i])
          {
          case '"': res.append ("\\\""); break;
          case '\n': res.append ("\\n"); break;
          case '\r': res.append ("\\r"); break;
          case '\t': res.append ("\\t"); break;
          case '\\': res.append ("\\\\"); break;
          case '\0': res.append ("\\0"); break;
          
          default:
            res.push_back (str[i]);
          }
      }
    
    res.push_back ('"');
    return res;
  }
  
  /* 
   * Returns a textual representation of the specified Rho value.
   */
  std::string
  rho_value_str (rho_value& v)
  {
    switch (v.type)
      {
      case RHO_NIL:
        return "nil";
      
      case RHO_EMPTY_LIST:
        return "'()";
      
      case RHO_BOOL:
        return v.val.b ? "true" : "false";
      
      case RHO_ATOM:
        {
          std::ostringstream ss;
          ss << "<atom #" << v.val.i32 << ">";
          return ss.str ();
        }
      
      case RHO_CONS:
        {
          std::ostringstream ss;
          ss << "'(" << rho_value_str (v.val.gc->val.p.fst);
          
          auto curr = v.val.gc->val.p.snd;
          while (curr.type == RHO_CONS)
            {
              ss << " " << rho_value_str (curr.val.gc->val.p.fst);
              curr = curr.val.gc->val.p.snd;
            }
          if (curr.type == RHO_EMPTY_LIST)
            ss << ")";
          else
            ss << " . " << rho_value_str (curr) << ")";
          
          return ss.str ();
        }
      
      case RHO_INTEGER:
        {
          auto str = mpz_get_str (NULL, 10, v.val.gc->val.i);
          std::string s = str;
          
          void (*freefunc) (void *, size_t);
          mp_get_memory_functions (NULL, NULL, &freefunc);
          freefunc (str, s.length () + 1);
          
          return s;
        }
      
      case RHO_FUN:
        {
          std::ostringstream ss;
          ss << "<function " << (void *)v.val.gc << ">";
          return ss.str ();
        }
      
      case RHO_VEC:
        {
          std::ostringstream ss;
          ss << "[";
          
          auto& vec = v.val.gc->val.vec;
          for (long i = 0; i < vec.len; ++i)
            {
              ss << rho_value_str (vec.vals[i]);
              if (i != vec.len - 1)
                ss << ", ";
            }
          
          ss << "]";
          return ss.str ();
        }
      
      case RHO_STR:
        return _escape_string (v.val.gc->val.s.str, v.val.gc->val.s.len);
      
      default:
        throw std::runtime_error ("rho_value_str: unhandled value type");
      }
  }
  
  
  
  rho_value
  rho_value_make_int (garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_INTEGER;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_INTEGER;
    mpz_init (g->val.i);
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_int (int val, garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_INTEGER;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_INTEGER;
    mpz_init_set_si (g->val.i, val);
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_int (const char *str, garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_INTEGER;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_INTEGER;
    mpz_init_set_str (g->val.i, str, 10);
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_vec (long cap, garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_VEC;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_VEC;
    
    auto& vec = g->val.vec;
    vec.vals = new rho_value [cap];
    vec.cap = cap;
    vec.len = 0;
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_function (const unsigned char *cp, int env_len,
                           garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_FUN;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_FUN;
    g->val.fn.cp = cp;
    g->val.fn.env_len = env_len;
    g->val.fn.env = new rho_value [env_len];
    for (int i = 0; i < env_len; ++i)
      g->val.fn.env[i].type = RHO_NIL;
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_empty_list (garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_EMPTY_LIST;
  
    auto g = gc.alloc_protected ();
    g->type = RHO_EMPTY_LIST;
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_cons (rho_value& fst, rho_value& snd, garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_CONS;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_CONS;
    g->val.p.fst = fst;
    g->val.p.snd = snd;
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_upvalue (garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_UPVAL;
    
    auto g = gc.alloc_upvalue_protected ();
    
    v.val.gc = g;
    return v;
  }
  
  rho_value
  rho_value_make_string (const char *str, long len, garbage_collector& gc)
  {
    rho_value v;
    v.type = RHO_STR;
    
    auto g = gc.alloc_protected ();
    g->type = RHO_STR;
    g->val.s.len = len;
    g->val.s.str = new char [len + 1];
    std::memcpy (g->val.s.str, str, len);
    g->val.s.str[len] = '\0';
    
    v.val.gc = g;
    return v;
  }
  
  
  
  static std::string
  _format_string (gc_value *str, rho_value& args_)
  {
    std::ostringstream ss;
    
    std::vector<rho_value> args;
    if (args_.type == RHO_CONS)
      {
        auto p = args_;
        while (p.val.gc->val.p.snd.type != RHO_EMPTY_LIST)
          {
            args.push_back (p.val.gc->val.p.fst);
            p = p.val.gc->val.p.snd;
          }
        args.push_back (p.val.gc->val.p.fst);
      }
    else
      args.push_back (args_);
    
    auto& s = str->val.s;
    for (long i = 0; i < s.len; ++i)
      {
        if (s.str[i] == '{')
          {
            ++ i;
            
            int idx = 0;
            while (i < s.len)
              {
                if (s.str[i] >= '0' && s.str[i] <= '9')
                  {
                    idx = (idx * 10) + (s.str[i] - '0');
                    ++ i;
                  }
                else if (s.str[i] == '}')
                  break;
                else
                  throw vm_error ("invalid format string");
              }
            if (i == s.len || s.str[i] != '}')
              throw vm_error ("invalid format string");
            
            if (idx >= (int)args.size ())
              throw vm_error ("index out of range in format string");
            
            if (args[idx].type == RHO_STR)
              ss << args[idx].val.gc->val.s.str;
            else
              ss << rho_value_str (args[idx]);
          }
        else if (s.str[i] == '\\')
          {
            if (++i == s.len)
              throw vm_error ("invalid format string");
            ss << s.str[i];
          }
        else
          ss << s.str[i];
      }
    
    return ss.str ();
  }
  
  
  
  rho_value
  rho_value_add (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_add (dest, dest, rhs.val.gc->val.i);
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  rho_value
  rho_value_sub (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_sub (dest, dest, rhs.val.gc->val.i);
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  rho_value
  rho_value_mul (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_mul (dest, dest, rhs.val.gc->val.i);
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  rho_value
  rho_value_div (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_tdiv_q (dest, dest, rhs.val.gc->val.i);
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  rho_value
  rho_value_pow (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_pow_ui (dest, dest, mpz_get_ui (rhs.val.gc->val.i));
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  rho_value
  rho_value_mod (rho_value& lhs, rho_value& rhs, garbage_collector& gc)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            {
              auto res = rho_value_make_int (gc);
              auto& dest = res.val.gc->val.i;
              mpz_set (dest, lhs.val.gc->val.i);
              mpz_tdiv_r (dest, dest, rhs.val.gc->val.i);
              return res;
            }
          
          default:
            return rho_value_make_nil ();
          }
        break;
      
      case RHO_STR:
        {
          auto str = _format_string (lhs.val.gc, rhs);
          auto res = rho_value_make_string (str.c_str (), str.length (), gc);
          return res;
        }
        break;
      
      default:
        return rho_value_make_nil ();
      }
  }
  
  
  
  bool
  rho_value_cmp_eq (rho_value& lhs, rho_value& rhs)
  {
    switch (lhs.type)
      {
      case RHO_BOOL:
        switch (rhs.type)
          {
          case RHO_BOOL:
            return lhs.val.b == rhs.val.b;
          
          default:
            return false;
          }
        break;
      
      case RHO_ATOM:
        switch (rhs.type)
          {
          case RHO_ATOM:
            return lhs.val.i32 == rhs.val.i32;
          
          default:
            return false;
          }
        break;
      
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            return mpz_cmp (lhs.val.gc->val.i, rhs.val.gc->val.i) == 0;
            
          default:
            return false;
          }
        break;
      
      case RHO_EMPTY_LIST:
        return rhs.type == lhs.type;
      
      case RHO_STR:
        switch (rhs.type)
          {
          case RHO_STR:
            {
              auto& s1 = lhs.val.gc->val.s;
              auto& s2 = rhs.val.gc->val.s;
              return s1.len == s2.len && std::memcpy (s1.str, s2.str, s1.len) == 0;
            }
          
          default:
            return false;
          }
        break;
      
      case RHO_CONS:
        switch (rhs.type)
          {
          case RHO_CONS:
            return lhs.val.gc == rhs.val.gc;
          
          default:
            return false;
          }
        break;
      
      default:
        return false;
      }
  }
  
  bool
  rho_value_cmp_neq (rho_value& lhs, rho_value& rhs)
  {
    return !rho_value_cmp_eq (lhs, rhs);
  }
  
  bool
  rho_value_cmp_lt (rho_value& lhs, rho_value& rhs)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            return mpz_cmp (lhs.val.gc->val.i, rhs.val.gc->val.i) < 0;
            
          default:
            return false;
          }
        break;
      
      default:
        return false;
      }
  }
  
  bool
  rho_value_cmp_lte (rho_value& lhs, rho_value& rhs)
  {
    switch (lhs.type)
      {
      case RHO_INTEGER:
        switch (rhs.type)
          {
          // integer + integer
          case RHO_INTEGER:
            return mpz_cmp (lhs.val.gc->val.i, rhs.val.gc->val.i) <= 0;
            
          default:
            return false;
          }
        break;
      
      default:
        return false;
      }
  }
  
  bool
  rho_value_cmp_gt (rho_value& lhs, rho_value& rhs)
  {
    return !rho_value_cmp_lte (lhs, rhs);
  }
  
  bool
  rho_value_cmp_gte (rho_value& lhs, rho_value& rhs)
  {
    return !rho_value_cmp_lt (lhs, rhs);
  }
  
  bool
  rho_value_cmp_zero (rho_value& v)
  {
    switch (v.type)
      {
      case RHO_NIL:
      case RHO_EMPTY_LIST:
        return true;
      
      case RHO_STR:
        return v.val.gc->val.s.len == 0;
      
      case RHO_BOOL:
        return !v.val.b;
      
      case RHO_INTEGER:
        return mpz_sgn (v.val.gc->val.i) == 0;
      
      default:
        return false;
      }
  }
  
  bool
  rho_value_cmp_ref_eq (rho_value& lhs, rho_value& rhs)
  {
    if (lhs.type != rhs.type)
      return false;
    
    switch (lhs.type)
      {
      case RHO_FUN:
      case RHO_CONS:
      case RHO_UPVAL:
      case RHO_VEC:
      case RHO_STR:
        return lhs.val.gc == rhs.val.gc;
      
      case RHO_ATOM:
        return lhs.val.i32 == rhs.val.i32;
      
      case RHO_BOOL:
        return lhs.val.b == rhs.val.b;
      
      case RHO_INTEGER:
        return mpz_cmp (lhs.val.gc->val.i, rhs.val.gc->val.i) == 0;
      
      case RHO_INTERNAL:
        return lhs.val.i64 == rhs.val.i64;
      
      case RHO_PVAR:
        return lhs.val.i32 == rhs.val.i32;
      
      case RHO_NIL:
      case RHO_EMPTY_LIST:
        return true;
      }
    
    return true;
  }
  
  
  
  static bool
  _match (rho_value& pat, rho_value& val, rho_value *stack, int& idx)
  {
    if (pat.type == RHO_PVAR)
      {
        stack[idx++] = val;
        return true;
      }
    else if (pat.type != val.type)
      return false;
    
    switch (pat.type)
      {
      case RHO_ATOM:
        return pat.val.i32 == val.val.i32;
      
      case RHO_INTEGER:
        return mpz_cmp (pat.val.gc->val.i, val.val.gc->val.i) == 0;
      
      case RHO_BOOL:
        return pat.val.b == val.val.b;
      
      case RHO_STR:
        {
          auto& s1 = pat.val.gc->val.s;
          auto& s2 = val.val.gc->val.s;
          return s1.len == s2.len && std::memcpy (s1.str, s2.str, s1.len) == 0;
        }
      
      case RHO_NIL:
      case RHO_EMPTY_LIST:
        return true;
      
      case RHO_CONS:
        return _match (pat.val.gc->val.p.fst, val.val.gc->val.p.fst, stack, idx)
          && _match (pat.val.gc->val.p.snd, val.val.gc->val.p.snd, stack, idx);
       
      case RHO_VEC:
        // TODO
        return false;
       
      case RHO_PVAR:
      case RHO_FUN:
      case RHO_INTERNAL:
      case RHO_UPVAL:
        return false;
      }
    
    return true;
  }
  
  /* 
   * Attempts to match the specified value against the given pattern.
   */
  bool
  rho_value_match (rho_value& pat, rho_value& val, rho_value *stack)
  {
    int idx = 0;
    return _match (pat, val, stack, idx);
  }
}

