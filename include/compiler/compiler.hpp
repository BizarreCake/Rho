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

#ifndef _RHO__COMPILER__COMPILER__H_
#define _RHO__COMPILER__COMPILER__H_

#include "common/errors.hpp"
#include "parser/ast.hpp"
#include "linker/module.hpp"
#include "compiler/frame.hpp"
#include "compiler/func.hpp"
#include <deque>


namespace rho {
  
  class code_generator;
  
  /* 
   * The compiler.
   * 
   * Given an AST tree representing a program as input, the compiler outputs a
   * a module.  Note that the compiler only handles one module at a time, thus
   * compiling a program consisting of more than one module requires compiling
   * the modules separately and then linking them together into an executable
   * using the linker.
   */
  class compiler
  {
    error_tracker& errs;
    
    module *mod;
    code_generator *cgen; // wrapped around the module's code section
    
    std::deque<frame *> frms;
    bool in_repl;
    
    std::vector<function_info> funcs;
    
  public:
    compiler (error_tracker& errs);
    ~compiler ();
    
  public:
    /* 
     * Compiles the specified AST program tree into a module.
     */
    module* compile (ast_program *prog);
    
  public:
    /* 
     * Tells the compiler that this module should be compiled in a REPL
     * configuration.
     */
    void set_repl ();
    
    /* 
     * Tells the compiler that any variables with the specified name should
     * be treated as global REPL variables.
     */
    void add_repl_var (const std::string& name, int index);
    
  private:
    /* 
     * Frame manipulation.
     */
//------------------------------------------------------------------------------
    
    /* 
     * Creates a new frame of the specified type.
     */
    void push_frame (frame_type type);
    
    /* 
     * Destroys the top-most frame.
     */
    void pop_frame ();
    
    /* 
     * Returns the top-most frame.
     */
    frame& top_frame ();
    
//------------------------------------------------------------------------------
    
  private:
    /* 
     * Compilation routines:
     */
//------------------------------------------------------------------------------
    
    void compile_program (ast_program *prog);
    
    void compile_stmt (ast_stmt *ast);
    
    void compile_expr (ast_expr *ast);
    
    void compile_block (ast_block *ast, bool as_expr, bool alloc_frame = true);
    
    
    // statements:
    
    void compile_expr_stmt (ast_expr_stmt *ast);
    
    void compile_let (ast_let *ast);
    
    
    // expressions:
    
    void compile_nil (ast_nil *ast);
    
    void compile_integer (ast_integer *ast);
    
    void compile_real (ast_real *ast);
    
    void compile_ident (ast_ident *ast);
    
    void compile_sym (ast_sym *ast);
    
    void compile_list (ast_list *ast);
    
    void compile_binop (ast_binop *ast);
    
    void compile_unop (ast_unop *ast);
    
    void compile_function (ast_function *ast);
    
    void compile_call (ast_call *ast);
    
    void compile_if (ast_if *ast);
    
    void compile_n (ast_n *ast);
    
    void compile_sum (ast_sum *ast);
    void compile_boundless_sum (ast_sum *ast);
    
    void compile_product (ast_product *ast);
    
    void compile_subst (ast_subst *ast);
    
    
    void compile_assign (ast_binop *ast);
    
    // source operand is assumed to be in the stack.
    void compile_assign_in_stack (ast_expr *dest);
    
    void compile_assign_to_ident (ast_ident *dest);
    
    
    // builtins:
     
    void compile_builtin_cons (ast_call *ast);
    void compile_builtin_car (ast_call *ast);
    void compile_builtin_cdr (ast_call *ast);
     
//------------------------------------------------------------------------------
  };
}

#endif

