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

#include "compiler/scope.hpp"
#include "util/ast_tools.hpp"
#include <algorithm>

#include <iostream> // DEBUG


namespace rho {

  var_analyzer::var_analyzer ()
  {
    this->next_glob_idx = 0;
  }
  
  
  
  
  void
  var_analyzer::add_known_global (const std::string& name, int idx)
  {
    this->kglobs[name] = idx;
    if (idx >= this->next_glob_idx)
      this->next_glob_idx = idx + 1;
  }



  /* 
   * Analyses the specified AST program.
   */
  var_analysis&&
  var_analyzer::analyze (std::shared_ptr<ast_program> p)
  {
    new (&this->res) var_analysis ();
    
    this->analyze_program (p);
        
    return std::move (this->res);
  }
  
  
  
  void
  var_analyzer::analyze_node (std::shared_ptr<ast_node> node)
  {
    this->res.tag (node, this->scopes.top ());
    
    switch (node->get_type ())
      {
      case AST_EMPTY_STMT:
      case AST_INTEGER:
      case AST_STRING:
      case AST_ATOM:
      case AST_NIL:
      case AST_BOOL:
      case AST_PROGRAM:
      case AST_MODULE:
      case AST_IMPORT:
      case AST_EXPORT:
      case AST_ATOM_DEF:
      case AST_FLOAT:
        break;
      
      case AST_NAMESPACE:
        this->analyze_namespace (std::static_pointer_cast<ast_namespace> (node));
        break;
      
      case AST_VAR_DEF:
        this->analyze_var_def (std::static_pointer_cast<ast_var_def> (node));
        break;
      
      case AST_STMT_BLOCK:
        this->analyze_stmt_block (std::static_pointer_cast<ast_stmt_block> (node));
        break;
      
      case AST_EXPR_BLOCK:
        this->analyze_expr_block (std::static_pointer_cast<ast_expr_block> (node));
        break;
      
      case AST_IDENT:
        this->analyze_ident (std::static_pointer_cast<ast_ident> (node));
        break;
      
      case AST_FUN:
        this->analyze_fun (std::static_pointer_cast<ast_fun> (node));
        break;
      
      case AST_VECTOR:
        this->analyze_vector (std::static_pointer_cast<ast_vector> (node));
        break;
      
      case AST_EXPR_STMT:
        this->analyze_expr_stmt (std::static_pointer_cast<ast_expr_stmt> (node));
        break;
      
      case AST_UNOP:
        this->analyze_unop (std::static_pointer_cast<ast_unop> (node));
        break;
      
      case AST_BINOP:
        this->analyze_binop (std::static_pointer_cast<ast_binop> (node));
        break;
      
      case AST_FUN_CALL:
        this->analyze_fun_call (std::static_pointer_cast<ast_fun_call> (node));
        break;
      
      case AST_IF:
        this->analyze_if (std::static_pointer_cast<ast_if> (node));
        break;
      
      case AST_CONS:
        this->analyze_cons (std::static_pointer_cast<ast_cons> (node));
        break;
      
      case AST_LIST:
        this->analyze_list (std::static_pointer_cast<ast_list> (node));
        break;
      
      case AST_MATCH:
        this->analyze_match (std::static_pointer_cast<ast_match> (node));
        break;
      
      case AST_RET:
        this->analyze_ret (std::static_pointer_cast<ast_ret> (node));
        break;
      
      case AST_SUBSCRIPT:
        this->analyze_subscript (std::static_pointer_cast<ast_subscript> (node));
        break;
      
      case AST_USING:
        this->analyze_using (std::static_pointer_cast<ast_using> (node));
        break;
      
      case AST_LET:
        this->analyze_let (std::static_pointer_cast<ast_let> (node));
        break;
      
      case AST_N:
        this->analyze_n (std::static_pointer_cast<ast_n> (node));
        break;
      }
  }
  
  void
  var_analyzer::analyze_program (std::shared_ptr<ast_program> p)
  {
    this->funs.emplace (new func_frame ({}));
    this->scopes.emplace (new scope_frame ({}, this->funs.top ()));
    this->funs.top ()->push_scope (this->scopes.top ());
    
    this->scopes.top ()->set_next_glob_idx (this->next_glob_idx);
    
    this->res.tag (p, this->scopes.top ());
    
    // perform pre-pass
    this->mode = MODE_PREPASS;
    for (auto s : p->get_stmts ())
      this->analyze_node (s);
    
    // perform full pass
    this->mode = MODE_FULL;
    for (auto s : p->get_stmts ())
      this->analyze_node (s);
    
    this->funs.top ()->pop_scope ();
    this->scopes.pop ();
    this->funs.pop ();
  }
  
  
  
  void
  var_analyzer::analyze_namespace (std::shared_ptr<ast_namespace> node)
  {
    auto pns = this->curr_ns;
    if (this->curr_ns.empty ())
      this->curr_ns = node->get_name ();
    else
      this->curr_ns += ":" + node->get_name ();
    
    for (auto s : node->get_body ()->get_stmts ())
      this->analyze_node (s);
    
    this->curr_ns = pns;
  }
  
  void
  var_analyzer::analyze_var_def (std::shared_ptr<ast_var_def> node)
  {
    if (this->mode == MODE_FULL)
      {
        this->analyze_node (node->get_val ());
        return;
      }
    
    auto scope = this->scopes.top ();
    auto name = node->get_var ()->get_value ();
    
    this->res.tag (node->get_var (), scope);
    if (this->scopes.size () == 1)
      {
        // top level scope
        auto qn = this->curr_ns.empty () ? name : (this->curr_ns + ":" + name);
        scope->add_global (qn);
      }
    else
      {
        scope->add_local (name);
      }
    
    this->analyze_node (node->get_val ());
  }
  
  void
  var_analyzer::analyze_stmt_block (std::shared_ptr<ast_stmt_block> node)
  {
    if (this->mode == MODE_PREPASS)
      return;
    
    this->scopes.emplace (new scope_frame (this->scopes.top (), this->funs.top ()));
    this->funs.top ()->push_scope (this->scopes.top ());
    this->scopes.top ()->inherit_locals ();
    
    this->mode = MODE_PREPASS;
    for (auto s : node->get_stmts ())
      this->analyze_node (s);
    
    this->mode = MODE_FULL;
    for (auto s : node->get_stmts ())
      this->analyze_node (s);
    
    this->funs.top ()->pop_scope ();
    this->scopes.pop ();
  }
  
  void
  var_analyzer::analyze_expr_block (std::shared_ptr<ast_expr_block> node)
  {
    if (this->mode == MODE_PREPASS)
      return;
    
    this->scopes.emplace (new scope_frame (this->scopes.top (), this->funs.top ()));
    this->funs.top ()->push_scope (this->scopes.top ());
    this->scopes.top ()->inherit_locals ();
    
    this->mode = MODE_PREPASS;
    for (auto s : node->get_stmts ())
      this->analyze_node (s);
    
    this->mode = MODE_FULL;
    for (auto s : node->get_stmts ())
      this->analyze_node (s);
    
    this->funs.top ()->pop_scope ();
    this->scopes.pop ();
  }
  
  
  
  static inline bool
  _is_undef_or_global (var_type type)
    { return type == VAR_UNDEF || type == VAR_GLOBAL; }
  
  void
  var_analyzer::analyze_ident (std::shared_ptr<ast_ident> node)
  {
    auto scope = this->scopes.top ();
    auto qn = this->qualify_name (node->get_value (), scope);
    
    auto var = scope->get_var (qn);
    if (var.type == VAR_UNDEF || var.type == VAR_GLOBAL)
      {
        // might be an upvalue, check.
        
        auto f = this->funs.top ();
        while (f && _is_undef_or_global (f->top_scope ()->get_var (qn).type))
          f = f->get_parent ();
        
        if (!f)
          {
            if (var.type == VAR_GLOBAL)
              {
                // it's a global variable
                return;
              }
            
            if (this->kglobs.find (qn) != this->kglobs.end ())
              {
                // it's a known global
                return;
              }
            
            return; // unrecognized identifier
          }
        
        var = f->top_scope ()->get_var (qn);
        if (var.type == VAR_LOCAL || var.type == VAR_ARG)
          f->add_cfree (qn);
        
        auto nf = this->funs.top ();
        while (nf != f)
          {
            nf->add_nfree (qn);
            nf = nf->get_parent ();
          }
      }
  }
  
  void
  var_analyzer::analyze_fun (std::shared_ptr<ast_fun> node)
  {
    if (this->mode == MODE_PREPASS)
      return;
    
    auto ps = this->scopes.top ();
    this->funs.emplace (new func_frame (this->funs.top ()));
    this->scopes.emplace (new scope_frame ({}, this->funs.top ()));
    
    auto scope = this->scopes.top ();
    this->funs.top ()->push_scope (scope);
    scope->inherit_globals_from (ps);
    scope->inherit_usings_from (ps);
    this->res.tag (node->get_body (), scope);
    
    // register parameters
    for (auto arg : node->get_params ())
      scope->add_arg (arg->get_value ());
    
    // perform pre-pass
    this->mode = MODE_PREPASS;
    for (auto s : node->get_body ()->get_stmts ())
      this->analyze_node (s);
    
    // perform full pass
    this->mode = MODE_FULL;
    for (auto s : node->get_body ()->get_stmts ())
      this->analyze_node (s);
    
    this->funs.top ()->pop_scope ();
    this->scopes.pop ();
    this->funs.pop ();
  }
  
  void
  var_analyzer::analyze_vector (std::shared_ptr<ast_vector> node)
  {
    for (auto e : node->get_exprs ())
      this->analyze_node (e);
  }
  
  void
  var_analyzer::analyze_expr_stmt (std::shared_ptr<ast_expr_stmt> node)
  {
    this->analyze_node (node->get_expr ());
  }
  
  void
  var_analyzer::analyze_unop (std::shared_ptr<ast_unop> node)
  {
    this->analyze_node (node->get_opr ());
  }
  
  void
  var_analyzer::analyze_binop (std::shared_ptr<ast_binop> node)
  {
    this->analyze_node (node->get_lhs ());
    this->analyze_node (node->get_rhs ());
  }
  
  void
  var_analyzer::analyze_fun_call (std::shared_ptr<ast_fun_call> node)
  {
    this->analyze_node (node->get_fun ());
    for (auto e : node->get_args ())
      this->analyze_node (e);
  }
  
  void
  var_analyzer::analyze_if (std::shared_ptr<ast_if> node)
  {
    this->analyze_node (node->get_test ());
    this->analyze_node (node->get_consequent ());
    this->analyze_node (node->get_antecedent ());
  }
  
  void
  var_analyzer::analyze_cons (std::shared_ptr<ast_cons> node)
  {
    this->analyze_node (node->get_fst ());
    this->analyze_node (node->get_snd ());
  }
  
  void
  var_analyzer::analyze_list (std::shared_ptr<ast_list> node)
  {
    for (auto e : node->get_elems ())
      this->analyze_node (e);
  }
  
  
  static void
  _register_pvars_as_locals (std::shared_ptr<ast_expr> pexpr,
                             std::shared_ptr<scope_frame> scope)
  {
    std::vector<std::string> pvars;
    std::unordered_set<std::string> pvar_set;
    
    ast_tools::traverse_dfs (pexpr,
      [&] (std::shared_ptr<ast_node> node) -> traverse_result {
        if (node->get_type () == AST_IDENT)
          {
            auto name = std::static_pointer_cast<ast_ident> (node)->get_value ();
            if (pvar_set.find (name) == pvar_set.end ())
              {
                pvars.push_back (name);
                pvar_set.insert (name);
              }
          }
        
        return TR_CONTINUE;
      });
    
    for (auto& pvar : pvars)
      scope->add_local (pvar);
  }
  
  void
  var_analyzer::analyze_match (std::shared_ptr<ast_match> node)
  {
    this->analyze_node (node->get_expr ());
    
    if (this->mode == MODE_PREPASS)
      return;
    
    for (auto& c : node->get_cases ())
      {
        this->scopes.emplace (new scope_frame (this->scopes.top (), this->funs.top ()));
        this->funs.top ()->push_scope (this->scopes.top ());
        this->scopes.top ()->inherit_locals ();
        
        _register_pvars_as_locals (c.pat, this->scopes.top ());
        
        this->mode = MODE_PREPASS;
        this->analyze_node (c.body);
        
        this->mode = MODE_FULL;
        this->analyze_node (c.body);
        
        this->funs.top ()->pop_scope ();
        this->scopes.pop ();
      }
  }
   
  
  void
  var_analyzer::analyze_ret (std::shared_ptr<ast_ret> node)
  {
    this->analyze_node (node->get_expr ());
  }
  
  void
  var_analyzer::analyze_subscript (std::shared_ptr<ast_subscript> node)
  {
    this->analyze_node (node->get_expr ());
    this->analyze_node (node->get_index ());
  }
  
  void
  var_analyzer::analyze_using (std::shared_ptr<ast_using> node)
  {
    auto scope = this->scopes.top ();
    if (node->get_alias ().empty ())
      scope->add_ualias (node->get_namespace ());
    else
      scope->add_ualias (node->get_namespace (), node->get_alias ());
  }
  
  void
  var_analyzer::analyze_let (std::shared_ptr<ast_let> node)
  {
    if (this->mode == MODE_PREPASS)
      return;
    
    this->scopes.emplace (new scope_frame (this->scopes.top (), this->funs.top ()));
    this->funs.top ()->push_scope (this->scopes.top ());
    this->scopes.top ()->inherit_locals ();
    
    // register let variables
    auto scope = this->scopes.top ();
    for (auto& p : node->get_defs ())
      {
        this->res.tag (p.second, scope);
        scope->add_local (p.first);
      }
    
    this->mode = MODE_FULL;
    for (auto& p : node->get_defs ())
      this->analyze_node (p.second);
    this->analyze_node (node->get_body ());
    
    this->funs.top ()->pop_scope ();
    this->scopes.pop ();
  }
  
  void
  var_analyzer::analyze_n (std::shared_ptr<ast_n> node)
  {
    this->analyze_node (node->get_prec ());
    this->analyze_node (node->get_body ());
  }
  
  
  
  std::string
  var_analyzer::qualify_name (const std::string& name,
                              std::shared_ptr<scope_frame> scope)
  {
    auto& u_ns = scope->get_used_namespaces ();
    for (auto& ns : u_ns)
      {
        std::string qn = ns + ":" + name;
        if (scope->get_var (qn).type != VAR_UNDEF)
          return qn;
      }
    
    auto& u_aliases = scope->get_aliases ();
    for (auto& p : u_aliases)
      {
        auto idx = name.find (p.first);
        if (idx == 0)
          {
            std::string qn = p.second + name.substr (idx + p.first.length ());
            if (scope->get_var (qn).type != VAR_UNDEF)
              return qn;
          }
      }
    
    std::string qn = this->curr_ns.empty () ? name : (this->curr_ns + ":" + name);
    if (scope->get_var (qn).type != VAR_UNDEF)
      return qn;
    
    return name;
  }
  
  
  
//------------------------------------------------------------------------------
  
  var_analysis::var_analysis ()
  {
  }
  
  var_analysis::var_analysis (const var_analysis& other)
    : scope_map (other.scope_map)
  {
    
  }

  var_analysis::var_analysis (var_analysis&& other)
    : scope_map (std::move (other.scope_map))
  {
    
  }
  
  
  
  /* 
   * Annonates the specified AST node with results obtained from the analyzer.
   */
  void
  var_analysis::tag (std::shared_ptr<ast_node> node,
                     std::shared_ptr<scope_frame> scope)
  {
    this->scope_map[node] = scope;
  }
  
  
  
  /* 
   * Returns the scope associated with the specified node.
   */
  std::shared_ptr<scope_frame>
  var_analysis::get_scope (std::shared_ptr<ast_node> node)
  {
    auto itr = this->scope_map.find (node);
    if (itr == this->scope_map.end ())
      return {};
    
    return itr->second;
  }
  
  
  
//------------------------------------------------------------------------------
  
  scope_frame::scope_frame (std::shared_ptr<scope_frame> parent,
                            std::shared_ptr<func_frame> fun)
    : fun (fun)
  {
    this->next_loc_idx = 0;
    this->next_glob_idx = 0;
    this->inherit (parent, false, true, true);
    
    this->scope_depth = fun->get_scope_depth () + 1;
  }

  scope_frame::scope_frame (const scope_frame& other)
    : locs (other.locs), args (other.args), parent (other.parent),
      next_loc_idx (other.next_loc_idx), next_glob_idx (other.next_glob_idx),
      fun (other.fun)
  {
    
  }
  
  scope_frame::scope_frame (scope_frame&& other)
    : locs (std::move (other.locs)), args (std::move (other.args)),
      parent (std::move (other.parent)),
      next_loc_idx (other.next_loc_idx), next_glob_idx (other.next_glob_idx),
      fun (std::move (other.fun))
  {
    
  }
  
  
  
  int
  scope_frame::add_local (const std::string& name)
  {
    this->fun->add_local ();
    
    this->locs[name] = this->next_loc_idx;
    return this->next_loc_idx++;
  }
  
  int
  scope_frame::add_arg (const std::string& name)
  {
    int idx = this->args.size ();
    this->args[name] = idx;
    return idx;
  }
  
  int 
  scope_frame::add_global (const std::string& name)
  {
    this->globs[name] = this->next_glob_idx;
    return this->next_glob_idx++;
  }
  
  variable
  scope_frame::get_var (const std::string& name)
  {
    auto itr = this->locs.find (name);
    if (itr != this->locs.end ())
      return { .type = VAR_LOCAL, .idx = itr->second };
    
    itr = this->args.find (name);
    if (itr != this->args.end ())
      return { .type = VAR_ARG, .idx = itr->second };
    
    {
      auto itr = this->fun->get_nfrees ().find (name);
      if (itr != this->fun->get_nfrees ().end ())
        return { .type = VAR_UPVAL, .idx = itr->second };
    }
    
    itr = this->globs.find (name);
    if (itr != this->globs.end ())
      return { .type = VAR_GLOBAL, .idx = itr->second };
    
    return { .type = VAR_UNDEF, .idx = 0 };
  }
  
  
  
  void
  scope_frame::add_ualias (const std::string& ns)
  {
    this->u_ns.push_back (ns);
  }
  
  void
  scope_frame::add_ualias (const std::string& ns, const std::string& alias)
  {
    this->u_aliases[alias] = ns;
  }
  
  
  
  void
  scope_frame::inherit (std::shared_ptr<scope_frame> parent,
                        bool inherit_locals, bool inherit_args,
                        bool inherit_globals)
  {
    this->parent = parent;
    if (!parent)
      return;
    
    if (inherit_globals)
      {
        this->globs = parent->globs;
        this->next_glob_idx = parent->next_glob_idx;
      }
      
    if (inherit_locals)
      {
        this->locs = parent->locs;
        this->next_loc_idx = parent->next_loc_idx;
      }
    
    if (inherit_args)
      this->args = parent->args;
    
    this->u_aliases = parent->u_aliases;
    this->u_ns = parent->u_ns;
  }
  
  void
  scope_frame::inherit_locals ()
  {
    this->locs = this->parent->locs;
    this->next_loc_idx = this->parent->next_loc_idx;
  }
  
  void
  scope_frame::inherit_globals_from (std::shared_ptr<scope_frame> scope)
  {
    this->globs = scope->globs;
  }
  
  void
  scope_frame::inherit_usings_from (std::shared_ptr<scope_frame> scope)
  {
    this->u_aliases = scope->u_aliases;
    this->u_ns = scope->u_ns;
  }
  
  
  
//------------------------------------------------------------------------------
  
  func_frame::func_frame (std::shared_ptr<func_frame> parent)
  {
    this->locc = 0;
    this->blk_depth = -1;
    this->next_nfree_idx = 0;
    this->set_parent (parent);
  }
  
  
  
  void
  func_frame::push_scope (std::shared_ptr<scope_frame> scope)
  {
    ++ this->blk_depth;
    this->blk_scope_sizes.push (0);
    
    this->scopes.push (scope);
    if (this->blk_depth >= (int)this->blk_sizes.size ())
      this->blk_sizes.push_back (0);
  }
  
  void
  func_frame::pop_scope ()
  {
    -- this->blk_depth;
    this->blk_scope_sizes.pop ();
    this->scopes.pop ();
  }
  
  std::shared_ptr<scope_frame>
  func_frame::top_scope ()
  {
    return this->scopes.top ();
  }
  
  
  
  void
  func_frame::add_local ()
  {
    int ns = this->blk_scope_sizes.top () + 1;
    this->blk_scope_sizes.pop ();
    this->blk_scope_sizes.push (ns);
    
    if (ns > this->blk_sizes[this->blk_depth])
      {
        ++ this->blk_sizes[this->blk_depth];
        ++ this->locc;
      }
  }
  
  int
  func_frame::add_nfree (const std::string& name)
  {
    auto itr = this->nfrees.find (name);
    if (itr != this->nfrees.end ())
      return itr->second;
    
    this->nfrees[name] = this->next_nfree_idx++;
    return this->next_nfree_idx - 1;
  }
  
  void
  func_frame::add_cfree (const std::string& name)
  {
    this->cfrees.insert (name);
  }
  
  int
  func_frame::get_nfree (const std::string& name)
  {
    auto itr = this->nfrees.find (name);
    if (itr != this->nfrees.end ())
      return itr->second;
    return -1;
  }
  
  
  
  std::vector<std::pair<std::string, int>>
  func_frame::get_sorted_nfrees () const
  {
    std::vector<std::pair<std::string, int>> ps;
    for (auto& p : this->nfrees)
      ps.push_back (p);
    
    std::sort (ps.begin (), ps.end (),
      [] (const std::pair<std::string, int>& lhs,
          const std::pair<std::string, int>& rhs) {
        return lhs.second < rhs.second;
      });
    return ps;
  }
  
  
  
  /* 
   * Returns the index of the first local variable of the block at the
   * specified depth.
   */
  int
  func_frame::get_block_off (int depth)
  {
    int off = 0;
    for (int i = 0; i < depth; ++i)
      off += this->blk_sizes[i];
    return off;
  }
  
  
  
  void
  func_frame::set_parent (std::shared_ptr<func_frame> parent)
  {
    this->parent = parent;
    if (!parent)
      return;
    
    this->next_nfree_idx = parent->next_nfree_idx;
  }
}

