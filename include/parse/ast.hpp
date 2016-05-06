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

#ifndef _RHO__PARSE__AST__H_
#define _RHO__PARSE__AST__H_

#include <vector>
#include <memory>
#include <string>


namespace rho {
  
  enum ast_node_type
  {
    AST_INTEGER,
    AST_IDENT,
    AST_NIL,
    AST_BOOL,
    AST_VECTOR,
    AST_ATOM,
    AST_STRING,
    AST_FLOAT,
    
    AST_EMPTY_STMT,
    AST_EXPR_STMT,
    AST_EXPR_BLOCK,
    AST_STMT_BLOCK,
    AST_PROGRAM,
    AST_NAMESPACE,
    AST_UNOP,
    AST_BINOP,
    AST_VAR_DEF,
    AST_FUN,
    AST_FUN_CALL,
    AST_IF,
    AST_CONS,
    AST_LIST,
    AST_MATCH,
    AST_MODULE,
    AST_IMPORT,
    AST_EXPORT,
    AST_RET,
    AST_SUBSCRIPT,
    AST_ATOM_DEF,
    AST_USING,
    AST_LET,
    AST_N,
  };
  
  
  /* 
   * Base class for all node types in an AST.
   */
  class ast_node
  {
  public:
    struct location
    {
      std::string path;
      int ln, col;
    };
  
  protected:
    location loc;
    
  public:
    inline const location& get_location() const { return this->loc; }
    
    void
    set_location (const std::string& path, int ln, int col)
      { this->loc = { path, ln, col }; }
    
  public:
    virtual ~ast_node () { }
    
  public:
    virtual ast_node_type get_type () const = 0;
  };
  
  
  /* 
   * Represents an expression.
   */
  class ast_expr: public ast_node
  {
  };
  
  /* 
   * Represents a statement.
   */
  class ast_stmt: public ast_node
  {
  };
  
  /* 
   * ;
   */
  class ast_empty_stmt: public ast_stmt
  {
  public:
    virtual ast_node_type get_type () const override { return AST_EMPTY_STMT; }
  };
  
  /* 
   * An expression wrapped in a statement.
   */
  class ast_expr_stmt: public ast_stmt
  {
    std::shared_ptr<ast_expr> expr;
    
  public:
    inline std::shared_ptr<ast_expr> get_expr () { return this->expr; }
    
    virtual ast_node_type get_type () const override { return AST_EXPR_STMT; }
    
  public:
    ast_expr_stmt (std::shared_ptr<ast_expr> expr)
      : expr (expr)
      { }
  };
  
  
  
  /* 
   * Integer literal.
   */
  class ast_integer: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_INTEGER; }
    
  public:
    ast_integer (const std::string& val)
      : str (val)
      { }
  };
  
  /* 
   * Float literal.
   */
  class ast_float: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_FLOAT; }
    
  public:
    ast_float (const std::string& val)
      : str (val)
      { }
  };
  
  /* 
   * Identifier.
   */
  class ast_ident: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_IDENT; }
    
  public:
    ast_ident (const std::string& val)
      : str (val)
      { }
  };
  
  /* 
   * Null constant.
   */
  class ast_nil: public ast_expr
  {
  public:
    virtual ast_node_type get_type () const override { return AST_NIL; }
  };
  
  /* 
   * Boolean literal.
   */
  class ast_bool: public ast_expr
  {
    bool val;
   
  public:
    inline bool get_value () const { return this->val; }
    
  public:
    virtual ast_node_type get_type () const override { return AST_BOOL; }
    
  public:
    ast_bool (bool val)
      : val (val)
      { }
  };
  
  /* 
   * One dimensional vector.
   */
  class ast_vector: public ast_expr
  {
    std::vector<std::shared_ptr<ast_expr>> exprs;
    
  public:
    inline std::vector<std::shared_ptr<ast_expr>>& get_exprs () { return this->exprs; }
    
    virtual ast_node_type get_type () const override { return AST_VECTOR; }
    
  public:
    void
    push_back (std::shared_ptr<ast_expr> e)
      { this->exprs.push_back (e); }
  };
  
  
  /* 
   * A sequence of statements.
   * A block's evaluation value is that of its last expression statement.
   */
  class ast_expr_block: public ast_expr
  {
    std::vector<std::shared_ptr<ast_stmt>> stmts;
    
  public:
    inline std::vector<std::shared_ptr<ast_stmt>>& get_stmts () { return this->stmts; }
    
    virtual ast_node_type get_type () const override { return AST_EXPR_BLOCK; }
    
  public:
    void
    push_back (std::shared_ptr<ast_stmt> stmt)
      { this->stmts.push_back (stmt); }
  };
  
  /* 
   * A sequence of statements.
   */
  class ast_stmt_block: public ast_stmt
  {
    std::vector<std::shared_ptr<ast_stmt>> stmts;
    
  public:
    inline std::vector<std::shared_ptr<ast_stmt>>& get_stmts () { return this->stmts; }
    
    virtual ast_node_type get_type () const override { return AST_STMT_BLOCK; }
    
  public:
    void
    push_back (std::shared_ptr<ast_stmt> stmt)
      { this->stmts.push_back (stmt); }
  };
  
  
  /* 
   * Top-level node that stores an entire Rho program.
   */
  class ast_program: public ast_stmt_block
  {
  public:
    virtual ast_node_type get_type () const override { return AST_PROGRAM; }
  };
  
  /* 
   * A collection of statements that are defined to be in the same namespace.
   */
  class ast_namespace: public ast_stmt
  {
    std::string name;
    std::shared_ptr<ast_stmt_block> body;
    
  public:
    inline const std::string& get_name () const { return this->name; }
    inline std::shared_ptr<ast_stmt_block> get_body () { return this->body; }
  
    virtual ast_node_type get_type () const override { return AST_NAMESPACE; }
    
  public:
    ast_namespace (const std::string& name, std::shared_ptr<ast_stmt_block> body)
      : name (name), body (body)
      { }
  };
  
  
  
  enum ast_unop_type
  {
    AST_UNOP_NOT,
  };
  
  /* 
   * Unary operator applied to one operand.
   */
  class ast_unop: public ast_expr
  {
    ast_unop_type op;
    std::shared_ptr<ast_expr> opr;
    
  public:
    inline ast_unop_type get_op () const { return this->op; }
    inline std::shared_ptr<ast_expr> get_opr () { return this->opr; }
    
    virtual ast_node_type get_type () const override { return AST_UNOP; }
    
  public:
    ast_unop (ast_unop_type op, std::shared_ptr<ast_expr> opr)
      : op (op), opr (opr)
      { }
  };
  
  
  
  enum ast_binop_type
  {
    AST_BINOP_ADD,
    AST_BINOP_SUB,
    AST_BINOP_MUL,
    AST_BINOP_DIV,
    AST_BINOP_MOD,
    AST_BINOP_POW,
    AST_BINOP_EQ,
    AST_BINOP_NEQ,
    AST_BINOP_LT,
    AST_BINOP_LTE,
    AST_BINOP_GT,
    AST_BINOP_GTE,
    AST_BINOP_AND,
    AST_BINOP_OR,
    
    AST_BINOP_ASSIGN,
  };
  
  /* 
   * Binary operator applied to two operands.
   */
  class ast_binop: public ast_expr
  {
    ast_binop_type op;
    std::shared_ptr<ast_expr> lhs, rhs;
    
  public:
    inline ast_binop_type get_op () const { return this->op; }
    inline std::shared_ptr<ast_expr> get_lhs () { return this->lhs; }
    inline std::shared_ptr<ast_expr> get_rhs () { return this->rhs; }
    
    virtual ast_node_type get_type () const override { return AST_BINOP; }
    /* 
   * Identifier.
   */
  class ast_ident: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_IDENT; }
    
  public:
    ast_ident (const std::string& val)
      : str (val)
      { }
  };
  public:
    ast_binop (ast_binop_type op,
               std::shared_ptr<ast_expr> lhs,
               std::shared_ptr<ast_expr> rhs)
      : op (op), lhs (lhs), rhs (rhs)
      { }
  };
  
  
  
  /* 
   * Variable definition of the form:
   *     var <ident> = <value>;
   */
  class ast_var_def: public ast_stmt
  {
    std::shared_ptr<ast_ident> var;
    std::shared_ptr<ast_expr> val;
    
  public:
    inline std::shared_ptr<ast_ident> get_var () { return this->var; }
    inline std::shared_ptr<ast_expr> get_val () { return this->val; }
    
    virtual ast_node_type get_type () const override { return AST_VAR_DEF; }
    
  public:
    ast_var_def (std::shared_ptr<ast_ident> var, std::shared_ptr<ast_expr> val)
      : var (var), val (val)
      { }
  };
  
  
  
  /* 
   * Function literal of the form:
   *     fun (<params>) { <body> }
   */
  class ast_fun: public ast_expr
  {
    std::vector<std::shared_ptr<ast_ident>> params;
    std::shared_ptr<ast_stmt_block> body;
    
  public:
    inline std::vector<std::shared_ptr<ast_ident>>& get_params () { return this->params; }
    inline std::shared_ptr<ast_stmt_block>& get_body () { return this->body; }
    
    virtual ast_node_type get_type () const override { return AST_FUN; }
    
  public:
    void
    set_body (std::shared_ptr<ast_stmt_block> body)
      { this->body = body; }
    
    void
    add_param (std::shared_ptr<ast_ident> p)
      { this->params.push_back (p); }
  };
  
  
  
  /* 
   * Function application.
   */
  class ast_fun_call: public ast_expr
  {
    std::shared_ptr<ast_expr> fun;
    std::vector<std::shared_ptr<ast_expr>> args;
    
  public:
    inline std::vector<std::shared_ptr<ast_expr>>& get_args () { return this->args; }
    inline std::shared_ptr<ast_expr>& get_fun () { return this->fun; }
    
    virtual ast_node_type get_type () const override { return AST_FUN_CALL; }
    
  public:
    ast_fun_call (std::shared_ptr<ast_expr> fun)
      : fun (fun)
      { }
    
    void
    add_arg (std::shared_ptr<ast_expr> expr)
      { this->args.push_back (expr); }
  };
  
  
  
  /* 
   * If expression.
   */
  class ast_if: public ast_expr
  {
    std::shared_ptr<ast_expr> test;
    std::shared_ptr<ast_expr> conseq;
    std::shared_ptr<ast_expr> ant;
    
  public:
    inline std::shared_ptr<ast_expr> get_test () { return this->test; }
    inline std::shared_ptr<ast_expr> get_consequent () { return this->conseq; }
    inline std::shared_ptr<ast_expr> get_antecedent () { return this->ant; }
    
    virtual ast_node_type get_type () const override { return AST_IF; }
    
  public:
    ast_if (std::shared_ptr<ast_expr> test,
            std::shared_ptr<ast_expr> conseq,
            std::shared_ptr<ast_expr> ant)
      : test (test), conseq (conseq), ant (ant)
      { }
  };
  
  
  
  /* 
   * Pair construction.
   * '(<a> . <b>)
   */
  class ast_cons: public ast_expr
  {
    std::shared_ptr<ast_expr> fst;
    std::shared_ptr<ast_expr> snd;
    
  public:
    inline std::shared_ptr<ast_expr> get_fst () { return this->fst; }
    inline std::shared_ptr<ast_expr> get_snd () { return this->snd; }
    
    virtual ast_node_type get_type () const override { return AST_CONS; }
    
  public:
    ast_cons (std::shared_ptr<ast_expr> fst, std::shared_ptr<ast_expr> snd)
      : fst (fst), snd (snd)
      { }
  };
  
  
  
  /* 
   * '(<a> <b> <c> ...)
   */
  class ast_list: public ast_expr
  {
    std::vector<std::shared_ptr<ast_expr>> elems;
    
  public:
    inline std::vector<std::shared_ptr<ast_expr>>& get_elems () { return this->elems; }
    
    virtual ast_node_type get_type () const override { return AST_LIST; }
    
  public:
    void
    add_elem (std::shared_ptr<ast_expr> e)
      { this->elems.push_back (e); }
  };



  /* 
   * Match expression.
   */
  class ast_match: public ast_expr
  {
  public:
    struct case_entry
    {
      std::shared_ptr<ast_expr> pat;
      std::shared_ptr<ast_expr> body;
    };
    
  private:
    std::shared_ptr<ast_expr> expr;
    std::vector<case_entry> cases;
    std::shared_ptr<ast_expr> else_body;
    
  public:
    inline std::shared_ptr<ast_expr> get_expr () { return this->expr; }
    inline std::vector<case_entry>& get_cases () { return this->cases; }
    inline std::shared_ptr<ast_expr> get_else_body () { return this->else_body; }
    
    virtual ast_node_type get_type () const override { return AST_MATCH; }
    
  public:
    ast_match (std::shared_ptr<ast_expr> expr)
      : expr (expr)
      { }
    
  public:
    void
    add_case (std::shared_ptr<ast_expr> pat, std::shared_ptr<ast_expr> body)
      { this->cases.push_back ({ pat, body }); }
    
    void
    set_else_case (std::shared_ptr<ast_expr> else_body)
      { this->else_body = else_body; }
  };
  
  
  
  /* 
   * Module definition statement of the form:
   *     module <module name>;
   */
  class ast_module: public ast_stmt
  {
    std::string name;
    
  public:
    inline const std::string& get_name () const { return this->name; }
    
    virtual ast_node_type get_type () const override { return AST_MODULE; }
    
  public:
    ast_module (const std::string& name)
      : name (name)
      { }
  };
  
  
  
  /* 
   * Module import statement.
   *     import <module name>;
   */
  class ast_import: public ast_stmt
  {
    std::string name;
    
  public:
    inline const std::string& get_name () const { return this->name; }
    
    virtual ast_node_type get_type () const override { return AST_IMPORT; }
    
  public:
    ast_import (const std::string& name)
      : name (name)
      { }
  };
  
  
  
  /* 
   * Export statement.
   *     export (<obj1>, <obj2>, ...)
   */
  class ast_export: public ast_stmt
  {
    std::vector<std::string> names;
    
  public:
    inline std::vector<std::string>& get_names () { return this->names; }
    
    virtual ast_node_type get_type () const override { return AST_EXPORT; }
    
  public:
    void
    add_export (const std::string& name)
      { this->names.push_back (name); }
  };
  
  
  
  /* 
   * Return statement.
   *     ret <expr>
   *     ret
   */
  class ast_ret: public ast_stmt
  {
    std::shared_ptr<ast_expr> expr;
    
  public:
    inline std::shared_ptr<ast_expr> get_expr () { return this->expr; }
    
    virtual ast_node_type get_type () const override { return AST_RET; }
    
  public:
    ast_ret () { }
    ast_ret (std::shared_ptr<ast_expr> expr) : expr (expr) { }
  };
  
  
  
  /* 
   * Subscript expression.
   *     <expr>[<index>]
   */
  class ast_subscript: public ast_expr
  {
    std::shared_ptr<ast_expr> expr;
    std::shared_ptr<ast_expr> index;
    
  public:
    inline std::shared_ptr<ast_expr> get_expr () { return this->expr; }
    inline std::shared_ptr<ast_expr> get_index () { return this->index; }
    
    virtual ast_node_type get_type () const override { return AST_SUBSCRIPT; }
    
  public:
    ast_subscript (std::shared_ptr<ast_expr> expr,
                   std::shared_ptr<ast_expr> index)
      : expr (expr), index (index)
      { }
  };
  
  
  
  /* 
   * Atom.
   */
  class ast_atom: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_ATOM; }
    
  public:
    ast_atom (const std::string& val)
      : str (val)
      { }
  };
  
  /* 
   * Atom definition of the form:
   *     atom <atom>;
   */
  class ast_atom_def: public ast_stmt
  {
    std::string name;
    
  public:
    inline const std::string& get_name () { return this->name; }
    
    virtual ast_node_type get_type () const override { return AST_ATOM_DEF; }
    
  public:
    ast_atom_def (const std::string& name)
      : name (name)
      { }
  };
  
  
  
  /* 
   * String literal.
   */
  class ast_string: public ast_expr
  {
    std::string str;
    
  public:
    inline const std::string& get_value () const { return this->str; }
    
    virtual ast_node_type get_type () const override { return AST_STRING; }
    
  public:
    ast_string (const std::string& val)
      : str (val)
      { }
  };
  
  
  
  /* 
   * using statement.
   *     using <namespace>;
   *     using <alias> = <namespace>;
   */
  class ast_using: public ast_stmt
  {
    std::string ns;
    std::string alias;
    
  public:
    inline const std::string& get_namespace () const { return this->ns; }
    inline const std::string& get_alias () const { return this->alias; }
    
    virtual ast_node_type get_type () const override { return AST_USING; }
    
  public:
    ast_using (const std::string& ns_name)
      : ns (ns_name)
      { }
    
    ast_using (const std::string& ns_name, const std::string& alias)
      : ns (ns_name), alias (alias)
      { }
  };
  
  
  
  /* 
   * Let expression.
   *     let <var1> = <val1>, <var2> = <val2>, ..., <varN> = <valN> in <expr>
   */
  class ast_let: public ast_expr
  {
    std::shared_ptr<ast_expr> body;
    std::vector<std::pair<std::string, std::shared_ptr<ast_expr>>> defs;
    
  public:
    inline std::shared_ptr<ast_expr>& get_body () { return this->body; }
    
    inline std::vector<std::pair<std::string, std::shared_ptr<ast_expr>>>&
    get_defs ()
      { return this->defs; }
      
    virtual ast_node_type get_type () const override { return AST_LET; }
    
  public:
    ast_let (std::shared_ptr<ast_expr> body)
      : body (body)
      { }
    
  public:
    void
    add_def (const std::string& name, std::shared_ptr<ast_expr> val)
      { this->defs.push_back (std::make_pair (name, val)); }
  };
  
  
  
  /* 
   * Numeric evaluation expression.
   *     N:<expr> { <expr> }
   */
  class ast_n: public ast_expr
  {
    std::shared_ptr<ast_expr> prec;
    std::shared_ptr<ast_expr_block> body;
    
  public:
    inline std::shared_ptr<ast_expr>& get_prec () { return this->prec; }
    inline std::shared_ptr<ast_expr_block>& get_body () { return this->body; }
    
    virtual ast_node_type get_type () const override { return AST_N; }
    
  public:
    ast_n (std::shared_ptr<ast_expr> prec, std::shared_ptr<ast_expr_block> body)
      : prec (prec), body (body)
      { }
  };
}

#endif

