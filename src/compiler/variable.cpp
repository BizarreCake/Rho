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

#include "compiler/variable.hpp"
#include "util/ast_tools.hpp"
#include <sstream>

#include <iostream> // DEBUG


namespace rho {
  
  var_analyzer::var_analyzer ()
  {
    this->next_glob_idx = 0;
  }
  
  
  
  void
  var_analyzer::clear_state ()
  {
    this->mode = OM_NORMAL;
    this->match_depth = 0;
    this->block_sizes.clear ();
    this->overshadowed.clear ();
    
    while (!this->ffrms.empty ())
      this->ffrms.pop ();
  }
  
  
  
  void
  var_analyzer::set_namespace (const std::string& ns)
  {
    this->curr_ns = ns;
  }
  
  void
  var_analyzer::add_known_global (const std::string& name, int idx)
  {
    this->known_globs[name] = idx;
  }
  
  
  
  std::string
  var_analyzer::qualify_name (const std::string& name, var_frame& tbl,
                              bool check_exists)
  {
    if (this->curr_ns.empty ())
      return name;
    
    auto n1 = this->curr_ns + ":" + name;
    if (!check_exists || tbl.get_var (n1).type != VAR_UNDEF)
      return n1;
    return name;
  }
  
  
  
  /* 
   * Performs analysis on the specified AST node.
   */
  var_frame
  var_analyzer::analyze (std::shared_ptr<ast_node> node,
                         const var_frame& parent)
  {
    this->clear_state ();
    
    var_frame frm;
    frm.set_parent (parent);
    this->analyze_node (node, frm);
    
    // allocate local variables to hold match pattern variables.
    for (int pvc : this->block_sizes)
      {
        frm.block_offs.push_back (frm.next_local_index ());
        for (int i = 0; i < pvc; ++i)
          frm.add_unnamed_local ();
      }
    
    return frm;
  }
  
  var_frame
  var_analyzer::analyze (std::shared_ptr<ast_node> node)
  {
    this->clear_state ();
    
    var_frame frm;
    this->analyze_node (node, frm);
    
    // allocate local variables to hold match pattern variables.
    for (int pvc : this->block_sizes)
      {
        frm.block_offs.push_back (frm.next_local_index ());
        for (int i = 0; i < pvc; ++i)
          frm.add_unnamed_local ();
      }
    
    return frm;
  }
  
  
  
  /* 
   * Performs analysis on the specified function.
   * Takes care to mark function parameters as free variables if they are
   * used in other nested functions.
   */
  var_frame
  var_analyzer::analyze_function (std::shared_ptr<ast_fun> node,
                                  const var_frame& parent)
  {
    this->clear_state ();
    
    var_frame frm;
    frm.set_parent (parent);
    
    // register parameters
    for (auto param : node->get_params ())
      frm.add_arg (param->get_value ());
    
    this->analyze_node (node->get_body (), frm);
    
    // allocate local variables to hold match pattern variables.
    for (int pvc : this->block_sizes)
      {
        frm.block_offs.push_back (frm.next_local_index ());
        for (int i = 0; i < pvc; ++i)
          frm.add_unnamed_local ();
      }
    
    return frm;
  }
  
  
  
  var_frame
  var_analyzer::analyze_program (std::shared_ptr<ast_program> node)
  {
    this->clear_state ();
    this->import_count = 0;
    this->mode = OM_TOPLEVEL;
    this->curr_ns = "";
    
    var_frame frm;
    
    for (auto p : this->known_globs)
      frm.add_global (p.first, p.second);
    
    this->analyze_node (node, frm);
    
    // allocate local variables to hold match pattern variables.
    for (int pvc : this->block_sizes)
      {
        frm.block_offs.push_back (frm.next_local_index ());
        for (int i = 0; i < pvc; ++i)
          frm.add_unnamed_local ();
      }
     
    // allocate local variables to store imported modules
    frm.mods_off = frm.next_local_index ();
    for (int i = 0; i < this->import_count; ++i)
      frm.add_unnamed_local ();
    
    return frm;
  }
  
  
  
  void
  var_analyzer::analyze_node (std::shared_ptr<ast_node> node, var_frame& tbl)
  {
    switch (node->get_type ())
      {
      case AST_NIL:
      case AST_BOOL:
      case AST_INTEGER:
        break;
      
      case AST_PROGRAM:
        this->analyze_program (std::static_pointer_cast<ast_program> (node), tbl);
        break;
      
      case AST_VAR_DEF:
        this->analyze_var_def (std::static_pointer_cast<ast_var_def> (node), tbl);
        break;
      
      case AST_IDENT:
        this->analyze_ident (std::static_pointer_cast<ast_ident> (node), tbl);
        break;
      
      case AST_STMT_BLOCK:
        this->analyze_stmt_block (std::static_pointer_cast<ast_stmt_block> (node), tbl);
        break;
      
      case AST_EXPR_BLOCK:
        this->analyze_expr_block (std::static_pointer_cast<ast_expr_block> (node), tbl);
        break;
      
      case AST_EXPR_STMT:
        this->analyze_expr_stmt (std::static_pointer_cast<ast_expr_stmt> (node), tbl);
        break;
      
      case AST_UNOP:
        this->analyze_unop (std::static_pointer_cast<ast_unop> (node), tbl);
        break;
      
      case AST_BINOP:
        this->analyze_binop (std::static_pointer_cast<ast_binop> (node), tbl);
        break;
      
      case AST_FUN:
        this->analyze_fun (std::static_pointer_cast<ast_fun> (node), tbl);
        break;
      
      case AST_FUN_CALL:
        this->analyze_fun_call (std::static_pointer_cast<ast_fun_call> (node), tbl);
        break;
      
      case AST_IF:
        this->analyze_if (std::static_pointer_cast<ast_if> (node), tbl);
        break;
      
      case AST_CONS:
        this->analyze_cons (std::static_pointer_cast<ast_cons> (node), tbl);
        break;
      
      case AST_LIST:
        this->analyze_list (std::static_pointer_cast<ast_list> (node), tbl);
        break;
      
      case AST_MATCH:
        this->analyze_match (std::static_pointer_cast<ast_match> (node), tbl);
        break;
      
      case AST_MODULE:
        this->analyze_module (std::static_pointer_cast<ast_module> (node), tbl);
        break;
      
      case AST_IMPORT:
        this->analyze_import (std::static_pointer_cast<ast_import> (node), tbl);
        break;
      
      case AST_EXPORT:
        this->analyze_export (std::static_pointer_cast<ast_export> (node), tbl);
        break;
      
      case AST_RET:
        this->analyze_ret (std::static_pointer_cast<ast_ret> (node), tbl);
        break;
      
      case AST_VECTOR:
        this->analyze_vector (std::static_pointer_cast<ast_vector> (node), tbl);
        break;
      
      case AST_SUBSCRIPT:
        this->analyze_subscript (std::static_pointer_cast<ast_subscript> (node), tbl);
        break;
      
      case AST_NAMESPACE:
        this->analyze_namespace (std::static_pointer_cast<ast_namespace> (node), tbl);
        break;
      }
  }
  
  void
  var_analyzer::analyze_program (std::shared_ptr<ast_program> node,
                                 var_frame& tbl)
  {
    for (auto stmt : node->get_stmts ())
      this->analyze_node (std::static_pointer_cast<ast_node> (stmt), tbl);
  }
  
  void
  var_analyzer::analyze_stmt_block (std::shared_ptr<ast_stmt_block> node,
                                    var_frame& tbl)
  {
    for (auto stmt : node->get_stmts ())
      this->analyze_node (std::static_pointer_cast<ast_node> (stmt), tbl);
  }
  
  void
  var_analyzer::analyze_expr_block (std::shared_ptr<ast_expr_block> node,
                                    var_frame& tbl)
  {
    for (auto expr : node->get_exprs ())
      this->analyze_node (std::static_pointer_cast<ast_node> (expr), tbl);
  }
  
  
  void
  var_analyzer::analyze_var_def (std::shared_ptr<ast_var_def> node,
                                 var_frame& tbl)
  {
    if (this->mode == OM_NORMAL)
      tbl.add_local (this->qualify_name (node->get_var ()->get_value (), tbl, false));
    else if (this->mode == OM_TOPLEVEL)
      tbl.add_global (this->qualify_name (node->get_var ()->get_value (), tbl, false));
    
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_val ()), tbl);
  }
  
  void
  var_analyzer::analyze_ident (std::shared_ptr<ast_ident> node,
                               var_frame& tbl)
  {
    auto qn = this->qualify_name (node->get_value (), tbl);
    
    switch (this->mode)
      {
      case OM_NORMAL:
        {
          auto var = tbl.get_var (qn);
          if (var.type == VAR_UNDEF)
            {
              auto p = tbl.get_parent ();
              if (!p)
                return;
              
              var = p->get_var (qn);
              if (var.type != VAR_UNDEF)
                {
                  // turn this variable into an upval.
                  int idx = tbl.add_upval (qn);
                  tbl.nfrees.push_back (std::make_pair (qn, idx));
                }
            }
        }
        break;
      
      case OM_NESTED:
        {
          if (this->overshadowed.find (qn) != this->overshadowed.end ())
            return;
          
          auto var = tbl.get_var (qn);
          if (var.type == VAR_UNDEF)
            {
              auto& ffrm = this->ffrms.top ();
              if (ffrm.get_param (qn) != -1)
                return;
            
              auto p = tbl.get_parent ();
              if (!p)
                return;
              
              var = p->get_var (qn);
              if (var.type == VAR_LOCAL || var.type == VAR_ARG)
                {
                  int idx = tbl.add_upval (qn);
                  tbl.nfrees.push_back (std::make_pair (qn, idx));
                }
            }
        }
        break;
      
      case OM_TOPLEVEL:
        break;
      }
    
  }
  
  void
  var_analyzer::analyze_expr_stmt (std::shared_ptr<ast_expr_stmt> node,
                                   var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_expr ()), tbl);
  }
  
  void
  var_analyzer::analyze_unop (std::shared_ptr<ast_unop> node,
                              var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_opr ()), tbl);
  }
  
  void
  var_analyzer::analyze_binop (std::shared_ptr<ast_binop> node,
                               var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_lhs ()), tbl);
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_rhs ()), tbl);
  }
  
  void
  var_analyzer::analyze_fun (std::shared_ptr<ast_fun> node,
                             var_frame& tbl)
  {
    this->ffrms.emplace ();
    auto& ffrm = this->ffrms.top ();
    int pi = 0;
    for (auto param : node->get_params ())
      ffrm.add_param (param->get_value (), pi++);
    
    switch (this->mode)
      {
      case OM_NORMAL:
      case OM_TOPLEVEL:
        {
          auto pmode = this->mode;
          this->mode = OM_NESTED;
          this->analyze_node (node->get_body (), tbl);
          this->mode = pmode; 
        }
        break;
      
      case OM_NESTED:
        this->analyze_node (node->get_body (), tbl);
        break;
      }
    
    this->ffrms.pop ();
  }
  
  void
  var_analyzer::analyze_fun_call (std::shared_ptr<ast_fun_call> node,
                                  var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_fun ()), tbl);
    for (auto arg : node->get_args ())
      this->analyze_node (std::static_pointer_cast<ast_node> (arg), tbl);
  }
  
  void
  var_analyzer::analyze_if (std::shared_ptr<ast_if> node, var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_test ()), tbl);
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_consequent ()), tbl);
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_antecedent ()), tbl);
  }
  
  void
  var_analyzer::analyze_cons (std::shared_ptr<ast_cons> node, var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_fst ()), tbl);
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_snd ()), tbl);
  }
  
  void
  var_analyzer::analyze_list (std::shared_ptr<ast_list> node, var_frame& tbl)
  {
    for (auto e : node->get_elems ())
      this->analyze_node (std::static_pointer_cast<ast_node> (e), tbl);
  }
  
  
  
  static std::unordered_set<std::string>
  _get_pvars (std::shared_ptr<ast_expr> node)
  {
    std::unordered_set<std::string> pvars;
    ast_tools::traverse_dfs (node,
      [&] (std::shared_ptr<ast_node> node) -> traverse_result {
        if (node->get_type () == AST_IDENT)
          pvars.insert (std::static_pointer_cast<ast_ident> (node)->get_value ());
        return TR_CONTINUE;
      });
    
    return pvars;
  }
  
  static int
  _count_pvars (std::shared_ptr<ast_expr> node)
  {
    int count = 0;
    ast_tools::traverse_dfs (node,
      [&] (std::shared_ptr<ast_node> node) -> traverse_result {
        if (node->get_type () == AST_IDENT)
          ++ count;
        return TR_CONTINUE;
      });
    
    return count;
  }
  
  void
  var_analyzer::analyze_match (std::shared_ptr<ast_match> node, var_frame& tbl)
  {
    if (this->mode == OM_NESTED)
      {
        for (auto& c : node->get_cases ())
          {
            auto prev = this->overshadowed;
            for (auto& name : _get_pvars (c.pat))
              this->overshadowed.insert (name);
            this->analyze_node (std::static_pointer_cast<ast_node> (c.body), tbl);
            this->overshadowed = prev;
          }
        
        return;
      }
    
    int max_pvc = 0;
    for (auto& c : node->get_cases ())
      {
        int pvc = _count_pvars (c.pat);
        if (pvc > max_pvc)
          max_pvc = pvc;
      }
    
    if ((int)this->block_sizes.size () <= this->match_depth)
      this->block_sizes.push_back (max_pvc);
    else if (this->block_sizes[this->match_depth] < max_pvc)
      this->block_sizes[this->match_depth] = max_pvc;
    
    ++ this->match_depth;
    for (auto& c : node->get_cases ())
      {
        this->analyze_node (std::static_pointer_cast<ast_node> (c.body), tbl);
      }
    -- this->match_depth;
  }
  
  
  
  void
  var_analyzer::analyze_module (std::shared_ptr<ast_module> node,
                                var_frame& tbl)
  {
    
  }
  
  void
  var_analyzer::analyze_import (std::shared_ptr<ast_import> node,
                                var_frame& tbl)
  {
    if (this->mode == OM_TOPLEVEL)
      ++ this->import_count;
  }
  
  void
  var_analyzer::analyze_export (std::shared_ptr<ast_export> node,
                                var_frame& tbl)
  {
    
  }
  
  void
  var_analyzer::analyze_ret (std::shared_ptr<ast_ret> node, var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_expr ()), tbl);
  }
  
  void
  var_analyzer::analyze_vector (std::shared_ptr<ast_vector> node, var_frame& tbl)
  {
    for (auto e : node->get_exprs ())
      this->analyze_node (std::static_pointer_cast<ast_node> (e), tbl);
  }
  
  void
  var_analyzer::analyze_subscript (std::shared_ptr<ast_subscript> node, var_frame& tbl)
  {
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_expr ()), tbl);
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_index ()), tbl);
  }
  
  void
  var_analyzer::analyze_namespace (std::shared_ptr<ast_namespace> node, var_frame& tbl)
  {
    auto pns = this->curr_ns;
    if (this->curr_ns.empty ())
      this->curr_ns = node->get_name ();
    else
      this->curr_ns += ":" + node->get_name ();
    
    this->analyze_node (std::static_pointer_cast<ast_node> (node->get_body ()), tbl);
    
    this->curr_ns = pns;
  }
  
  
  
//------------------------------------------------------------------------------
  
  var_frame::var_frame ()
  {
    this->mods_off = 0;
    this->next_glob_idx = 0;
  }
  
  var_frame::var_frame (const var_frame& other)
    : frees (other.frees), nfrees (other.nfrees), locals (other.locals),
      args (other.args), globs (other.globs), block_offs (other.block_offs)
  {
    this->mods_off = other.mods_off;
    if (other.parent)
      this->parent.reset (new var_frame (*other.parent));
  }
  
  var_frame::var_frame (var_frame&& other)
    : parent (std::move (other.parent)),
      frees (std::move (other.frees)),
      nfrees (std::move (other.nfrees)),
      locals (std::move (other.locals)),
      args (std::move (other.args)),
      globs (std::move (other.globs)),
      block_offs (std::move (other.block_offs))
  {
    this->mods_off = other.mods_off;
  }
  
  
  
  variable
  var_frame::get_var (const std::string& name)
  {
    auto itr = this->args.find (name);
    if (itr != this->args.end ())
      return { .type = VAR_ARG, .idx = itr->second };
    
    itr = this->frees.find (name);
    if (itr != this->frees.end ())
      return { .type = VAR_UPVAL, .idx = itr->second }; 
      
    itr = this->locals.find (name);
    if (itr != this->locals.end ())
      return { .type = VAR_LOCAL, .idx = itr->second }; 
    
    itr = this->globs.find (name);
    if (itr != this->globs.end ())
      return { .type = VAR_GLOBAL, .idx = itr->second }; 
      
    return { .type = VAR_UNDEF };
  }
  
  int
  var_frame::add_local (const std::string& name)
  {
    int idx = this->locals.size ();
    this->locals[name] = idx;
    return idx;
  }
  
  int
  var_frame::add_upval (const std::string& name)
  {
    int idx = this->frees.size ();
    this->frees[name] = idx;
    return idx;
  }
  
  int
  var_frame::add_arg (const std::string& name)
  {
    int idx = this->args.size ();
    this->args[name] = idx;
    return idx;
  }
  
  int
  var_frame::add_global (const std::string& name)
  {
    int idx = this->next_glob_idx++;
    this->globs[name] = idx;
    return idx;
  }
  
  void
  var_frame::add_global (const std::string& name, int idx)
  {
    this->globs[name] = idx;
    this->next_glob_idx = idx + 1;
  }
  
  int
  var_frame::add_unnamed_local ()
  {
    std::ostringstream ss;
    ss << "^%^%" << this->next_local_index ();
    
    int idx = this->locals.size ();
    this->locals[ss.str ()] = idx;
    return idx;
  }
  
  
  
  void
  var_frame::set_parent (const var_frame& tbl)
  {
    this->parent.reset (new var_frame (tbl));
    
    this->frees = tbl.frees;
    this->globs = tbl.globs;
  }
}

