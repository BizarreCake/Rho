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

#ifndef _RHO__COMPILER__SCOPE__H_
#define _RHO__COMPILER__SCOPE__H_

#include "parse/ast.hpp"
#include "compiler/fun.hpp"
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>


namespace rho {
  
  enum var_type
  {
    VAR_UNDEF,
    VAR_LOCAL,
    VAR_ARG,
    VAR_GLOBAL,
    VAR_UPVAL,
    VAR_ARG_PACK,
  };
  
  struct variable
  {
    var_type type;
    int idx;
  };
  
  
  
  // forward decs:
  class scope_frame;
  
  /* 
   * Stores relevant information about a function.
   */
  class func_frame
  {
    int locc;
    int argc;
    
    std::vector<int> blk_sizes;
    std::stack<std::shared_ptr<scope_frame>> scopes;
    std::stack<int> blk_scope_sizes;
    int blk_depth;
    
    // contains every name that the function borrows from upper functions.
    std::unordered_map<std::string, int> nfrees;
    int next_nfree_idx;
    
    // contains every name that the function lends to lower functions.
    std::unordered_set<std::string> cfrees;
    
    std::shared_ptr<func_frame> parent;
    
  public:
    inline int get_local_count () const { return this->locc; }
    inline int get_arg_count () const { return this->argc; }
    inline int get_scope_depth () const { return this->blk_depth; }
    
    inline const std::vector<int>& get_block_sizes () const { return this->blk_sizes; }
    
    inline const std::unordered_map<std::string, int>&
    get_nfrees () const
      { return this->nfrees; }
    
    std::vector<std::pair<std::string, int>> get_sorted_nfrees () const;
    
    inline const std::unordered_set<std::string>&
    get_cfrees () const
      { return this->cfrees; }
    
    inline std::shared_ptr<func_frame> get_parent () { return this->parent; }
    
  public:
    func_frame (std::shared_ptr<func_frame> parent);
    
  public:
    void push_scope (std::shared_ptr<scope_frame> scope);
    void pop_scope ();
    std::shared_ptr<scope_frame> top_scope ();
    
    void add_arg ();
    void add_local ();
    int add_nfree (const std::string& name);
    void add_cfree (const std::string& name);
    int get_nfree (const std::string& name);
    
    /* 
     * Returns the index of the first local variable of the block at the
     * specified depth.
     */
    int get_block_off (int depth);
    
    void set_parent (std::shared_ptr<func_frame> parent);
  };
  
  /* 
   * Stores information about a lexical scope.
   */
  class scope_frame
  {
    std::unordered_map<std::string, int> locs;
    std::unordered_map<std::string, int> args;
    std::unordered_map<std::string, int> globs;
    std::string arg_pack_name;
    std::unordered_map<std::string, std::shared_ptr<fun_prototype>> protos;
    std::shared_ptr<scope_frame> parent;
    
    std::unordered_map<std::string, std::string> u_aliases;
    std::vector<std::string> u_ns;
    
    int next_loc_idx;
    int next_glob_idx;
    
    std::shared_ptr<func_frame> fun;
    int scope_depth;
    
    std::unordered_set<std::string> proto_imps;
    
  public:
    inline std::shared_ptr<scope_frame> get_parent () { return this->parent; }
    
    inline const std::unordered_map<std::string, std::string>&
    get_aliases () const
      { return this->u_aliases; }
    
    inline const std::vector<std::string>&
    get_used_namespaces () const
      { return this->u_ns; }
    
    inline std::unordered_map<std::string, std::shared_ptr<fun_prototype>>&
    get_fun_protos ()
      { return this->protos; }
    
    inline std::unordered_map<std::string, int>&
    get_globals ()
      { return this->globs; }
    
    inline int get_local_count () const { return this->locs.size (); }
    inline int get_arg_count () const { return this->args.size (); }
    inline int get_global_count () const { return this->globs.size (); }
    
    inline std::shared_ptr<func_frame> get_fun () { return this->fun; }
    inline int get_scope_depth () const { return this->scope_depth; }
    
    void set_next_glob_idx (int idx) { this->next_glob_idx = idx; }
    
  public:
    scope_frame (std::shared_ptr<scope_frame> parent,
                 std::shared_ptr<func_frame> fun);
    scope_frame (const scope_frame& other);
    scope_frame (scope_frame&& other);
    
  public:
    int add_local (const std::string& name);
    int add_arg (const std::string& name);
    int add_global (const std::string& name);
    void add_arg_pack (const std::string& name);
    void add_fun_proto (const std::string& name,
                        std::shared_ptr<fun_prototype> proto); 
    variable get_var (const std::string& name);
    
    void add_imported_fun_proto (std::shared_ptr<fun_prototype> proto);
    
    
    void add_ualias (const std::string& ns);
    void add_ualias (const std::string& ns, const std::string& alias);
    
    void inherit (std::shared_ptr<scope_frame> parent, bool inherit_locals,
                  bool inherit_args, bool inherit_globals);
    void inherit_locals ();
    void inherit_globals_from (std::shared_ptr<scope_frame> scope);
    void inherit_usings_from (std::shared_ptr<scope_frame> scope);
  };
  
  
  
  enum analysis_level
  {
    ANL_FULL,     // the entire AST has been fully analyzed.
    ANL_TOPLEVEL, // only top-level analysis has been done.
  };
  
  /* 
   * Stores the results obtained from an analysis done by the variable analyzer.
   */
  class var_analysis
  {
  public:
    struct guard_scope_info
    {
      std::shared_ptr<ast_expr> guard;
      std::shared_ptr<scope_frame> scope;
    };
  
  private:
    std::unordered_map<std::shared_ptr<ast_node>, std::shared_ptr<scope_frame>>
      scope_map;
    
    std::unordered_map<std::shared_ptr<ast_node>, std::shared_ptr<fun_prototype>>
      proto_map;
  
    std::unordered_map<std::shared_ptr<ast_ident>,
      std::unordered_map<std::string, guard_scope_info>>
      guard_scopes;
    
    std::vector<std::shared_ptr<fun_prototype>> top_protos;
    
    analysis_level level;
  
  public:
    inline analysis_level get_analysis_level () const { return this->level; }
    
    inline std::vector<std::shared_ptr<fun_prototype>>&
    get_top_level_fun_protos ()
      { return this->top_protos; }
  
  public:
    var_analysis ();
    var_analysis (analysis_level level);
    var_analysis (const var_analysis& other);
    var_analysis (var_analysis&& other);
    
  public:
    /* 
     * Annonates the specified AST node with its associated scope frame.
     */
    void tag_scope (std::shared_ptr<ast_node> node,
                    std::shared_ptr<scope_frame> scope);
    
    void tag_fun_proto (std::shared_ptr<ast_node> node,
                        std::shared_ptr<fun_prototype> proto);
    
    void tag_guard_scope (std::shared_ptr<ast_ident> node,
                          const std::string& fun_mname,
                          std::shared_ptr<ast_expr> guard,
                          std::shared_ptr<scope_frame> scope);
    
    void add_top_level_fun_proto (std::shared_ptr<fun_prototype> proto);
    
    
    /* 
     * Returns the scope associated with the specified node.
     */
    std::shared_ptr<scope_frame> get_scope (std::shared_ptr<ast_node> node);
    
    std::shared_ptr<fun_prototype> get_fun_proto (std::shared_ptr<ast_node> node);
    
    std::unordered_map<std::string, guard_scope_info>*
    get_guard_scopes (std::shared_ptr<ast_ident> node);
  };
  
  
  
  /* 
   * Variable analyzer.
   * The variable analyzer analyzes an AST program and determines the status of
   * every identifier ocurrence in respect to the scope it is in.
   */
  class var_analyzer
  {
  private:
    enum analysis_mode
      {
        MODE_PREPASS,
        MODE_FULL,
      };
  
  private:
    std::stack<std::shared_ptr<scope_frame>> scopes;
    std::stack<std::shared_ptr<func_frame>> funs;
    
    std::vector<std::shared_ptr<fun_prototype>> proto_imps;
    std::unordered_map<std::string, int> kglobs;
    int next_glob_idx;
    
    var_analysis res;
    std::string curr_ns;
    analysis_mode mode;
    
  public:
    var_analyzer ();
    
  public:
    /* 
     * Performs a full analysis on the specified AST program.
     */
    var_analysis&& analyze_full (std::shared_ptr<ast_program> p);
    
    /* 
     * Performs a lighter, top-level analysis of the specified AST program.
     */
    var_analysis&& analyze_toplevel (std::shared_ptr<ast_program> p);
    
    
    
    void add_imported_fun_proto (std::shared_ptr<fun_prototype> proto);
    
    // REPL stuff:
    
    void add_known_global (const std::string& name, int idx);
    
  private:
    void analyze_program (std::shared_ptr<ast_program> p, analysis_level level);
    
    void analyze_node (std::shared_ptr<ast_node> node);
    void analyze_namespace (std::shared_ptr<ast_namespace> node);
    void analyze_var_def (std::shared_ptr<ast_var_def> node);
    void analyze_stmt_block (std::shared_ptr<ast_stmt_block> node);
    void analyze_expr_block (std::shared_ptr<ast_expr_block> node);
    void analyze_ident (std::shared_ptr<ast_ident> node);
    void analyze_fun (std::shared_ptr<ast_fun> node);
    void analyze_vector (std::shared_ptr<ast_vector> node);
    void analyze_expr_stmt (std::shared_ptr<ast_expr_stmt> node);
    void analyze_unop (std::shared_ptr<ast_unop> node);
    void analyze_binop (std::shared_ptr<ast_binop> node);
    void analyze_fun_call (std::shared_ptr<ast_fun_call> node);
    void analyze_if (std::shared_ptr<ast_if> node);
    void analyze_cons (std::shared_ptr<ast_cons> node);
    void analyze_list (std::shared_ptr<ast_list> node);
    void analyze_match (std::shared_ptr<ast_match> node);
    void analyze_ret (std::shared_ptr<ast_ret> node);
    void analyze_subscript (std::shared_ptr<ast_subscript> node);
    void analyze_using (std::shared_ptr<ast_using> node);
    void analyze_let (std::shared_ptr<ast_let> node);
    void analyze_n (std::shared_ptr<ast_n> node);
    void analyze_fun_def (std::shared_ptr<ast_fun_def> node);
    
  private:
    std::string qualify_name (const std::string& name,
                              std::shared_ptr<scope_frame> scope);
  };
}

#endif

