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

#ifndef _RHO__COMPILER__VARIABLE__H_
#define _RHO__COMPILER__VARIABLE__H_

#include "compiler/fun.hpp"
#include "parse/ast.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <utility>
#include <stack>


namespace rho {
  
  enum var_type
  {
    VAR_UNDEF,
    VAR_LOCAL,
    VAR_ARG,
    VAR_UPVAL,
    VAR_GLOBAL,
  };
  
  struct variable
  {
    var_type type;
    int idx;
  };
  
  
  
  // forward decs:
  class var_analyzer;
  
  /* 
   * 
   */
  class var_frame
  {
    friend class var_analyzer;
    
    std::unique_ptr<var_frame> parent;
    std::unordered_map<std::string, int> frees;
    std::vector<std::pair<std::string, int>> nfrees;
    std::unordered_map<std::string, int> locals;
    std::unordered_map<std::string, int> args;
    std::unordered_map<std::string, int> globs;
    int next_glob_idx;
    
    std::vector<int> block_offs;
    int mods_off;
    
  public:
    inline std::vector<int>& get_block_offs () { return this->block_offs; }
    inline int get_mods_off () const { return this->mods_off; }
    
    inline int next_local_index () const { return this->locals.size (); }
    inline int free_count () const { return this->frees.size (); }
    inline int local_count () const { return this->locals.size (); }
    inline int arg_count () const { return this->args.size (); }
    inline int global_count () const { return this->globs.size (); }
    
    inline std::vector<std::pair<std::string, int>>&
    get_new_frees ()
      { return this->nfrees; }
    
    inline std::unordered_map<std::string, int>& get_locals () { return this->locals; }
    inline std::unordered_map<std::string, int>& get_args () { return this->args; }
    inline std::unordered_map<std::string, int>& get_globals () { return this->globs; }
    
    inline var_frame* get_parent () { return this->parent.get (); }
    
  public:
    var_frame ();
    var_frame (const var_frame& other);
    var_frame (var_frame&& other);
    
  public:
    variable get_var (const std::string& name);
    int add_local (const std::string& name);
    int add_upval (const std::string& name);
    int add_arg (const std::string& name);
    int add_global (const std::string& name);
    void add_global (const std::string& name, int idx);
    int add_unnamed_local ();
    
    void set_parent (const var_frame& tbl);
  };
  
  
  
  /* 
   * An AST analyzer that determines what identifiers refer to in their context
   * (i.e. local variables, function arguments, or upvalues).
   */
  class var_analyzer
  {
  private:
    /* 
     * Modes of operation.
     */
    enum op_mode
    {
      OM_NORMAL,
      
      /* 
       * The mode the analyzer is running in when scanning the top-level AST
       * program node.
       */
      OM_TOPLEVEL,
      
      /* 
       * In this mode, the analyzer is searching within a nested function.
       */
      OM_NESTED,
    };
    
  private:
    int match_depth;
    std::vector<int> block_sizes;
    int import_count;
    
    op_mode mode;
    std::unordered_set<std::string> overshadowed;
    std::stack<func_frame> ffrms;
    
    std::string curr_ns;
    int next_glob_idx;
    std::unordered_map<std::string, int> known_globs;
    
  public:
    var_analyzer ();
    
  private:
    void clear_state ();
    
    std::string qualify_name (const std::string& name, var_frame& tbl,
                              bool check_exists = true);
    
  public:
    void set_namespace (const std::string& ns);
    void add_known_global (const std::string& name, int idx);
    
  public:
    /* 
     * Performs analysis on the specified AST node.
     */
    var_frame analyze (std::shared_ptr<ast_node> node, const var_frame& parent);
    var_frame analyze (std::shared_ptr<ast_node> node);

    /* 
     * Performs analysis on the specified function.
     */
    var_frame analyze_function (std::shared_ptr<ast_fun> node,
                                const var_frame& parent);
    
    var_frame analyze_program (std::shared_ptr<ast_program> node);
    
  private:
    void analyze_node (std::shared_ptr<ast_node> node, var_frame& tbl);
    void analyze_program (std::shared_ptr<ast_program> node, var_frame& tbl);
    void analyze_var_def (std::shared_ptr<ast_var_def> node, var_frame& tbl);
    void analyze_ident (std::shared_ptr<ast_ident> node, var_frame& tbl);
    void analyze_stmt_block (std::shared_ptr<ast_stmt_block> node, var_frame& tbl);
    void analyze_expr_block (std::shared_ptr<ast_expr_block> node, var_frame& tbl);
    void analyze_expr_stmt (std::shared_ptr<ast_expr_stmt> node, var_frame& tbl);
    void analyze_unop (std::shared_ptr<ast_unop> node, var_frame& tbl);
    void analyze_binop (std::shared_ptr<ast_binop> node, var_frame& tbl);
    void analyze_fun (std::shared_ptr<ast_fun> node, var_frame& tbl);
    void analyze_fun_call (std::shared_ptr<ast_fun_call> node, var_frame& tbl);
    void analyze_if (std::shared_ptr<ast_if> node, var_frame& tbl);
    void analyze_cons (std::shared_ptr<ast_cons> node, var_frame& tbl);
    void analyze_list (std::shared_ptr<ast_list> node, var_frame& tbl);
    void analyze_match (std::shared_ptr<ast_match> node, var_frame& tbl);
    void analyze_module (std::shared_ptr<ast_module> node, var_frame& tbl);
    void analyze_import (std::shared_ptr<ast_import> node, var_frame& tbl);
    void analyze_export (std::shared_ptr<ast_export> node, var_frame& tbl);
    void analyze_ret (std::shared_ptr<ast_ret> node, var_frame& tbl);
    void analyze_vector (std::shared_ptr<ast_vector> node, var_frame& tbl);
    void analyze_subscript (std::shared_ptr<ast_subscript> node, var_frame& tbl);
    void analyze_namespace (std::shared_ptr<ast_namespace> node, var_frame& tbl);
  };
}

#endif

