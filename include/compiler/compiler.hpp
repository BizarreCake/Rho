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

#ifndef _RHO__COMPILER__COMPILER__H_
#define _RHO__COMPILER__COMPILER__H_

#include "parse/ast.hpp"
#include "linker/module.hpp"
#include "compiler/code_generator.hpp"
#include "compiler/scope.hpp"
#include "compiler/errors.hpp"
#include "compiler/module_store.hpp"
#include <memory>
#include <stack>


namespace rho {
  
  /* 
   * Used to implement tail-call optimizations.
   */
  class expr_frame
  {
    bool last;
    
  public:
    inline bool is_last () const { return this->last; }
    
  public:
    expr_frame (bool last)
      : last (last)
      { }
  };
  
  
  struct name_import_t
  {
    std::string mident;
    int idx;
  };
  
  
  
  /* 
   * The compiler.
   * Produces a module/object file given an AST created by the parser.
   */
  class compiler
  {
    code_generator cgen;
    std::stack<expr_frame> expr_frames;
    error_list errs;
    std::shared_ptr<var_analysis> van;
    std::shared_ptr<ast_program> prg_ast;
    
    bool pat_on; // true if compiling a pattern
    int pat_pvc;
    std::vector<std::string> pat_pvs;
    std::unordered_map<std::string, int> pat_pvs_unique;
    int match_depth;
    
    std::string mident;
    std::shared_ptr<module> mod;  // module being compiled
    module_store& mstore;
    std::unordered_map<std::string, name_import_t> name_imps;
    std::unordered_map<std::string, std::shared_ptr<fun_prototype>> proto_imps;
    
    std::vector<std::string> idirs;
    std::string wd;
    
    std::string curr_ns;
    
    bool alloc_globs;
    int glob_count;
    std::unordered_map<std::string, int> known_globs;
    int next_glob_idx;
  
    std::unordered_set<std::string> atoms;
    std::unordered_set<std::string> known_atoms;
    std::unordered_set<std::shared_ptr<fun_prototype>> known_protos;
  
  public:
    inline error_list& get_errors () { return this->errs; }
  
    inline const std::vector<std::string>& get_include_dirs () const { return this->idirs; }
  
  public:
    compiler (module_store& mstore);
    ~compiler ();
  
  public:
    /* 
     * Compiles the specified AST program.
     */
    std::shared_ptr<module> compile (std::shared_ptr<ast_program> program,
                                     const std::string& mident);
    
    
    
    /* 
     * Inserts the specified path into the include directory list.
     */
    void add_include_dir (const std::string& path);
    
    /* 
     * Sets the working directory for the next compilation.
     */
    void set_working_directory (const std::string& path);
    
    
    
    // REPL stuff:
    
    void alloc_globals (int global_count = -1);
    void dont_alloc_globals ();
    void add_known_global (const std::string& name, int idx);
    
    void add_known_atom (const std::string& name);
    
    void add_known_fun_proto (std::shared_ptr<fun_prototype> proto);
    
  private:
    void push_expr_frame (bool last);
    void pop_expr_frame ();
    bool can_perform_tail_call ();
    
    std::string qualify_name (const std::string& name,
                              std::shared_ptr<scope_frame> scope);
    std::string qualify_atom_name (const std::string& name,
                                   bool check_exists = true);
    
    bool try_compile_named_fun_call (std::shared_ptr<ast_fun_call> expr);
    
    void process_imports ();
    void process_import (std::shared_ptr<ast_import> stmt);
    
  private:
    void compile_program (std::shared_ptr<ast_program> program);
    
    void compile_empty_stmt (std::shared_ptr<ast_empty_stmt> stmt);
    void compile_expr_stmt (std::shared_ptr<ast_expr_stmt> stmt);
    void compile_var_def (std::shared_ptr<ast_var_def> stmt);
    void compile_module (std::shared_ptr<ast_module> stmt);
    void compile_import (std::shared_ptr<ast_import> stmt);
    void compile_export (std::shared_ptr<ast_export> stmt);
    void compile_ret (std::shared_ptr<ast_ret> stmt);
    void compile_namespace (std::shared_ptr<ast_namespace> stmt);
    void compile_atom_def (std::shared_ptr<ast_atom_def> stmt);
    void compile_stmt_block (std::shared_ptr<ast_stmt_block> stmt);
    void compile_using (std::shared_ptr<ast_using> stmt);
    void compile_fun_def (std::shared_ptr<ast_fun_def> stmt);
    void compile_stmt (std::shared_ptr<ast_stmt> stmt);
    
    void compile_integer (std::shared_ptr<ast_integer> expr);
    void compile_float (std::shared_ptr<ast_float> expr);
    void compile_ident (std::shared_ptr<ast_ident> expr);
    void compile_atom (std::shared_ptr<ast_atom> expr);
    void compile_string (std::shared_ptr<ast_string> expr);
    void compile_nil (std::shared_ptr<ast_nil> expr);
    void compile_bool (std::shared_ptr<ast_bool> expr);
    void compile_unop (std::shared_ptr<ast_unop> expr);
    void compile_binop (std::shared_ptr<ast_binop> expr);
    void compile_fun (std::shared_ptr<ast_fun> expr);
    void compile_fun_call (std::shared_ptr<ast_fun_call> expr);
    void compile_if (std::shared_ptr<ast_if> expr);
    void compile_cons (std::shared_ptr<ast_cons> expr);
    void compile_list (std::shared_ptr<ast_list> expr);
    void compile_match (std::shared_ptr<ast_match> expr);
    void compile_vector (std::shared_ptr<ast_vector> expr);
    void compile_subscript (std::shared_ptr<ast_subscript> expr);
    void compile_expr_block (std::shared_ptr<ast_expr_block> expr);
    void compile_let (std::shared_ptr<ast_let> expr);
    void compile_n (std::shared_ptr<ast_n> expr);
    void compile_expr (std::shared_ptr<ast_expr> expr);
    
    void compile_assign (std::shared_ptr<ast_expr> lhs,
                         std::shared_ptr<ast_expr> rhs);
    void compile_assign_to_ident (std::shared_ptr<ast_ident> lhs,
                                  std::shared_ptr<ast_expr> rhs);
    void compile_assign_to_subscript (std::shared_ptr<ast_subscript> lhs,
                                      std::shared_ptr<ast_expr> rhs);
    
    
    bool compile_builtin (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_car (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_cdr (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_cons (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_breakpoint (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_print (std::shared_ptr<ast_fun_call> expr);
    void compile_builtin_len (std::shared_ptr<ast_fun_call> expr);
  };
}

#endif

