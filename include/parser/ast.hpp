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

#ifndef _RHO__PARSER__AST__H_
#define _RHO__PARSER__AST__H_

#include "common/types.hpp"
#include <vector>
#include <string>


namespace rho {
  
  enum ast_type
  {
    AST_PROGRAM,
    
    // datums:
    AST_IDENT,
    AST_INTEGER,
    AST_REAL,
    AST_SYM,    
    AST_SET,
    AST_TYPE,
    
    // expressions:
    AST_BINOP,
    AST_UNOP,
    AST_FUNCTION,
    AST_CALL,
    AST_IF,
    AST_N,
    AST_SUM,
    
    // statements:
    AST_EXPR_STMT,
    AST_LET,
    AST_BLOCK,
  };
  
  
  /* 
   * The base class of all AST tree types.
   */
  class ast_node
  {
  public:
    virtual ~ast_node () { }
    
    virtual ast_type get_type () = 0;
    
    /* 
     * Performs a deep copy.
     */
    virtual ast_node* clone () = 0;
  };
  
  
  
  /* 
   * An AST node that has some value associated with it.
   */
  class ast_expr: public ast_node
    { };
  
  
  
  /* 
   * A value-less AST node, typically representing some kind of action.
   */
  class ast_stmt: public ast_node
    { };
  
  
  
  /* 
   * A sequence of zero or more statements.
   *     { <stmt1>; <stmt2>; ... }
   * 
   * Examples:
   *     { let x = 5; let y = x - 2; }
   */
  class ast_block: public ast_node
  {
  protected:
    std::vector<ast_stmt *> stmts;
    
  public:
    virtual ast_type get_type () override { return AST_BLOCK; }
    
    inline std::vector<ast_stmt *>& get_stmts () { return this->stmts; }
    
  public:
    ~ast_block ()
    {
      for (ast_stmt *s : this->stmts)
        delete s;
    }
    
  public:
    void add_stmt (ast_stmt *stmt) { this->stmts.push_back (stmt); }
    
    virtual ast_node*
    clone () override
    {
      ast_block *blk = new ast_block ();
      for (ast_stmt *s : this->stmts)
        blk->add_stmt (static_cast<ast_stmt *> (s->clone ()));
      return blk;
    }
  };
  
  
  
  /* 
   * Datums:
   */
//------------------------------------------------------------------------------
  
  /* 
   * Arbitrary-precision integer, simply stored as a string for convenience.
   * 
   * Examples:
   *     -5, 13, 123901293102398124918294310294.
   */
  class ast_integer: public ast_expr
  {
    std::string num;
    
  public:
    virtual ast_type get_type () override { return AST_INTEGER; }
    
    inline const std::string& get_value () const { return this->num; }
    
  public:
    ast_integer (const std::string& n)
      : num (n)
      { }
      
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_integer (this->num);
    }
  };
  
  
  
  /* 
   * Floating point number, simply stored as a string for convenience.
   * 
   * Examples:
   *     3.14515926, -5.0
   */
  class ast_real: public ast_expr
  {
    std::string num;
    
  public:
    virtual ast_type get_type () override { return AST_REAL; }
    
    inline const std::string& get_value () const { return this->num; }
    
  public:
    ast_real (const std::string& n)
      : num (n)
      { }
      
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_real (this->num);
    }
  };
  
  
  
  /* 
   * Identifier.
   * 
   * Examples:
   *     x, test, _something, Test510, AnIdent__
   */
  class ast_ident: public ast_expr
  {
    std::string name;
    
  public:
    virtual ast_type get_type () override { return AST_IDENT; }
    
    inline const std::string& get_name () const { return this->name; }
    
  public:
    ast_ident (const std::string& name)
      : name (name)
      { }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_ident (this->name);
    }
  };
  
  
  
  /* 
   * Symbol.
   * 
   * Examples:
   *     'x, 'y, '_an_ident, 'someTHING7, '5
   */
  class ast_sym: public ast_expr
  {
    std::string name;
    
  public:
    virtual ast_type get_type () override { return AST_SYM; }
    
    inline const std::string& get_name () const { return this->name; }
    
  public:
    ast_sym (const std::string& name)
      : name (name)
      { }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_sym (this->name);
    }
  };
  
  
  
  /* 
   * An unordered collection of elements specified in between the ${ and }
   * delimiters.
   * 
   * Examples:
   *     ${}, ${ 1, 2 }, ${ ${} }, ${ ${ 1, x }, { 3 }, 5, x }
   */
  class ast_set: public ast_expr
  {
    std::vector<ast_expr *> elems;
    
  public:
    virtual ast_type get_type () override { return AST_SET; }
    
    inline std::vector<ast_expr *>& get_elems () { return this->elems; }
    
  public:
    ~ast_set ()
    {
      for (ast_expr *e : this->elems)
        delete e;
    }
    
  public:
    void
    add_expr (ast_expr *e)
    {
      this->elems.push_back (e);
    }
    
    virtual ast_node*
    clone () override
    {
      ast_set *s = new ast_set ();
      for (ast_expr *e : this->elems)
        s->add_expr (static_cast<ast_expr *> (e->clone ()));
      return s;
    }
  };
  
//------------------------------------------------------------------------------
  
  
  
  /* 
   * Expressions:
   */
//------------------------------------------------------------------------------
  
  enum ast_binop_type
  {
    AST_BINOP_INVALID,
    
    AST_BINOP_ASSIGN,     // =
    
    AST_BINOP_ADD,        // +
    AST_BINOP_SUB,        // -
    AST_BINOP_MUL,        // *
    AST_BINOP_DIV,        // /
    AST_BINOP_MOD,        // %
    AST_BINOP_POW,        // ^
    
    AST_BINOP_EQ,         // ==
    AST_BINOP_NEQ,        // =/=
    AST_BINOP_LT,         // <
    AST_BINOP_LTE,        // <=
    AST_BINOP_GT,         // >
    AST_BINOP_GTE,        // >=
  };
  
  /* 
   * A binary operator.
   *     <lhs> <op> <rhs>
   * 
   * Examples:
   *     4 * 5, 9 + 2, 3 - 6
   */
  class ast_binop: public ast_expr
  {
    ast_expr *lhs;
    ast_expr *rhs;
    ast_binop_type op;
    
  public:
    virtual ast_type get_type () override { return AST_BINOP; }
    
    inline ast_expr* get_lhs () { return this->lhs; }
    inline ast_expr* get_rhs () { return this->rhs; }
    inline ast_binop_type get_op () { return this->op; }
    
  public:
    ast_binop (ast_expr *lhs, ast_expr *rhs, ast_binop_type op)
      : lhs (lhs), rhs (rhs), op (op)
      { }
    
    ~ast_binop ()
    {
      delete this->lhs;
      delete this->rhs;
    }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_binop (
        static_cast<ast_expr *> (this->lhs->clone ()),
        static_cast<ast_expr *> (this->rhs->clone ()),
        this->op);
    }
  };
  
  
  
  enum ast_unop_type
  {
    AST_UNOP_INVALID,
    
    AST_UNOP_NEGATE,       // -n
    AST_UNOP_FACTORIAL,    // n!
  };
  
  /* 
   * A unary operator.
   *     <op> <expr>
   * or:
   *     <expr> <op> 
   *
   * Examples:
   *     5!
   */
  class ast_unop: public ast_expr
  {
    ast_expr *expr;
    ast_unop_type op;
  
  public:
    virtual ast_type get_type () override { return AST_UNOP; }
    
    inline ast_expr* get_expr () { return this->expr; }
    inline ast_unop_type get_op () { return this->op; }
    
  public:
    ast_unop (ast_expr *expr, ast_unop_type op)
      : expr (expr), op (op)
      { }
    
    ~ast_unop ()
    {
      delete this->expr;
    }
  
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_unop (
        static_cast<ast_expr *> (this->expr->clone ()),
        this->op);
    }
  };
  
  
  
  /* 
   * A function call.
   *     <func>(<args>)
   * 
   * Examples:
   *     sqrt(5), (fun { x^2 })(5)
   */
  class ast_call: public ast_expr
  {
    ast_expr *func;
    std::vector<ast_expr *> args;
    
  public:
    virtual ast_type get_type () override { return AST_CALL; }
    
    inline ast_expr* get_func () { return this->func; }
    inline std::vector<ast_expr *>& get_args () { return this->args; }
    
  public:
    ast_call (ast_expr *fn)
      : func (fn)
      { }
    
    ~ast_call ()
    {
      delete this->func;
      for (ast_expr *e : this->args)
        delete e;
    }
    
  public:
    void add_arg (ast_expr *arg) { this->args.push_back (arg); }
    
    virtual ast_node*
    clone () override
    {
      ast_call *call = new ast_call (static_cast<ast_expr *> (this->func->clone ()));
      for (ast_expr *e : this->args)
        call->add_arg (static_cast<ast_expr *> (e->clone ()));
      return call;
    }
  };
  
  
  
  /* 
   * Conditional expression evaluation.
   *     if <test> then <conseq> else/otherwise <alt>
   * or alternatively:
   *     <conseq> if <test> else/otherwise <alt>
   * 
   * Examples:
   *     if x < y then 5 else 7
   *     x if x > y otherwise y
   */
  class ast_if: public ast_expr
  {
    ast_expr *test;
    ast_expr *conseq;
    ast_expr *alt;
    
  public:
    virtual ast_type get_type () override { return AST_IF; }
    
    inline ast_expr* get_test () { return this->test; }
    inline ast_expr* get_conseq () { return this->conseq; }
    inline ast_expr* get_alt () { return this->alt; }
    
  public:
    ast_if (ast_expr *test, ast_expr *conseq, ast_expr *alt)
      : test (test), conseq (conseq), alt (alt)
      { }
    
    ~ast_if ()
    {
      delete this->test;
      delete this->conseq;
      delete this->alt;
    }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_if (static_cast<ast_expr *> (this->test->clone ()),
        static_cast<ast_expr *> (this->conseq->clone ()),
        static_cast<ast_expr *> (this->alt->clone ()));
    }
  };
  
  
  
  /*
   * Evaluate a block in a specified precision.
   *     N:<prec> <block>;
   * 
   * Examples:
   *     N:100 {: 2^0.5
   */
  class ast_n: public ast_expr
  {
    ast_expr *prec;
    ast_block *body;
    
  public:
    virtual ast_type get_type () override { return AST_N; }
    
    inline ast_expr* get_prec () { return this->prec; }
    inline ast_block* get_body () { return this->body; }
    
  public:
    ast_n (ast_expr *prec, ast_block *body)
      : prec (prec), body (body)
      { }
    
    ~ast_n ()
    {
      delete this->prec;
      delete this->body;
    }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_n (static_cast<ast_expr *> (this->prec->clone ()),
        static_cast<ast_block *> (this->body->clone ()));
    }
  };
  
  
  
  /* 
   * Sums a series of terms.
   *     sum <var> <- <start>..<end> {
   *         <body>
   *     }
   * 
   * Examples:
   *     sum i <- 0..10 { (-1)^i * 1^(2*i + 1) / (2*i + 1)! }
   */
  class ast_sum: public ast_expr
  {
    ast_ident *var;
    ast_expr *start;
    ast_expr *end;
    ast_block *body;
    
  public:
    virtual ast_type get_type () override { return AST_SUM; }
    
    inline ast_ident* get_var () { return this->var; }
    inline ast_expr* get_start () { return this->start; }
    inline ast_expr* get_end () { return this->end; }
    inline ast_block* get_body () { return this->body; }
    
  public:
    ast_sum (ast_ident *var, ast_expr *start, ast_expr *end, ast_block *body)
      : var (var), start (start), end (end), body (body)
      { }
    
    ~ast_sum ()
    {
      delete this->var;
      delete this->start;
      delete this->end;
      delete this->body;
    }
    
  public:
    virtual ast_node*
    clone () override
    {
      return new ast_sum (static_cast<ast_ident *> (this->var->clone ()),
        static_cast<ast_expr *> (this->start->clone ()),
        static_cast<ast_expr *> (this->end->clone ()),
        static_cast<ast_block *> (this->body->clone ()));
    }
  };
  
//------------------------------------------------------------------------------
  
  
  
  /* 
   * Statements:
   */
//------------------------------------------------------------------------------
  
  /* 
   * An expression wrapped inside of a statement, which simply throws the
   * associated value away.
   */
  class ast_expr_stmt: public ast_stmt
  {
    ast_expr *expr;
    
  public:
    virtual ast_type get_type () override { return AST_EXPR_STMT; }
    
    inline ast_expr* get_expr () { return this->expr; }
    
  public:
    ast_expr_stmt (ast_expr *e)
      : expr (e)
      { }
    
    ~ast_expr_stmt ()
      { delete this->expr; }
    
  public:
    void release_expr () { this->expr = nullptr; }
    
    virtual ast_node*
    clone () override
    {
      return new ast_expr_stmt (static_cast<ast_expr *> (this->expr->clone ()));
    }
  };
  
  
  
  /* 
   * Variable declarations of the following forms:
   *     let <name>;
   *     let <name = <expr>;
   *     let <name> :: Type;
   *     let <name> :: Type = <expr>;
   * 
   * Examples:
   *     let x = 5;
   *     let y :: Set = ${ x, ${} };
   */
  class ast_let: public ast_stmt
  {
    std::string name;
    ast_expr *val;
    basic_type bt;
    bool in_repl;
    
  public:
    virtual ast_type get_type () override { return AST_LET; }
    
    inline const std::string& get_name () const { return this->name; }
    inline ast_expr* get_value () { return this->val; }
    inline basic_type get_var_type () const { return this->bt; }
    
    inline void set_repl () { this->in_repl = true; }
    inline bool is_repl () { return this->in_repl; }
    
  public:
    ast_let (const std::string& name, ast_expr *val = nullptr,
      basic_type bt = TYPE_UNSPEC)
      : name (name), val (val), bt (bt)
      { this->in_repl = false; }
    
    ~ast_let ()
      { delete this->val; }
      
  public:
    virtual ast_node*
    clone () override
    {
      ast_let *let = new ast_let (this->name,
        static_cast<ast_expr *> (this->val->clone ()),
        this->bt);
      if (this->in_repl)
        let->set_repl ();
      return let;
    }
  };

//------------------------------------------------------------------------------
  
  
  
  /* 
   * A special AST type that serves as the root of all subsequent nodes.
   */
  class ast_program: public ast_block
  {
  public:
    virtual ast_type get_type () override { return AST_PROGRAM; }
    
  public:
    void
    remove_stmt (int index, bool del)
    {
      if (del)
        delete this->stmts[index];
      this->stmts.erase (this->stmts.begin () + index);
    }
  };
  
  
  
  struct ast_function_param
  {
    std::string name;
    basic_type bt;
  };
  
  /* 
   * An (optionally) anonymous function.
   *     fun <name> (<args>) {
   *         <body>
   *     }
   * Both <name> and <args> are optional.
   * If no arguments are specified, then the parentheses are also optional.
   * 
   * Examples:
   *     fun (x) { x^2 }
   *     fun eq (x, y) { return x == y; }
   */
  class ast_function: public ast_expr
  {
    std::string name;
    std::vector<ast_function_param> params;
    ast_block *body;
    
  public:
    virtual ast_type get_type () override { return AST_FUNCTION; }  
    
    inline const std::string& get_name () const { return this->name; }
    inline std::vector<ast_function_param>& get_params () { return this->params; }
    inline ast_block* get_body () { return this->body; }
    
    inline bool is_anonym () { return this->name.empty (); }
    
  public:
    ast_function (const std::string& name)
      : name (name)
      { this->body = nullptr; }
    
    ~ast_function ()
    {
      delete this->body;
    }
    
  public:
    void
    add_param (const std::string& name, basic_type bt = TYPE_UNSPEC)
      { this->params.push_back ({ name, bt }); }
    
    void
    set_body (ast_block *body)
      { this->body = body; }
    
    virtual ast_node*
    clone () override
    {
      ast_function *f = new ast_function (this->name);
      f->set_body (static_cast<ast_block *> (this->body->clone ()));
      for (auto p : this->params)
        f->add_param (p.name, p.bt);
      return f;
    }
  };
} 

#endif

