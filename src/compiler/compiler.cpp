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

#include "compiler/compiler.hpp"
#include "util/ast_tools.hpp"
#include "util/module_tools.hpp"
#include <stdexcept>
#include <gmp.h>
#include <sstream>

#include <iostream> // DEBUG


namespace rho {
  
  compiler::compiler (module_store& mstore)
    : mstore (mstore)
  {
    this->pat_on = false;
    this->match_depth = 0;
    
    this->alloc_globs = true;
    this->glob_count = -1;
    this->next_glob_idx = 0;
  }
  
  compiler::~compiler ()
  {
    
  }
  
  
  
  /* 
   * Inserts the specified path into the include directory list.
   */
  void
  compiler::add_include_dir (const std::string& path)
  {
    this->idirs.push_back (path);
  }
  
  /* 
   * Sets the working directory for the next compilation.
   */
  void
  compiler::set_working_directory (const std::string& path)
  {
    this->wd = path;
  }
  
  
  
  void
  compiler::alloc_globals (int global_count)
  {
    this->alloc_globs = true;
    this->glob_count = global_count;
  }
  
  void
  compiler::dont_alloc_globals ()
  {
    this->alloc_globs = false;
  }
  
  void
  compiler::add_known_global (const std::string& name, int idx)
  {
    this->known_globs[name] = idx;
    this->next_glob_idx = idx + 1;
  }
  
  
  
  void
  compiler::push_expr_frame (bool last)
  {
    this->expr_frames.emplace (last);
  }
  
  void
  compiler::pop_expr_frame ()
  {
    this->expr_frames.pop ();
  }
  
  bool
  compiler::can_perform_tail_call ()
  {
    return !this->expr_frames.empty () && this->expr_frames.top ().is_last ();
  }
  
  
  
  std::string
  compiler::qualify_name (const std::string& name, bool check_exists)
  {
    auto qn = this->curr_ns;
    if (qn.empty ())
      qn = name;
    else
      qn += ":" + name;
    
    auto& scope = this->scope_frames.top ();
    if (!check_exists || scope.get_var (qn).type != VAR_UNDEF)
      return qn;
    
    return name;
  }
  
  
  
  /* 
   * Compiles the specified AST program.
   */
  std::shared_ptr<module>
  compiler::compile (std::shared_ptr<ast_program> program,
                     const std::string& mident)
  {
    this->name_imps.clear ();
    this->cgen.clear ();
    this->mod = std::shared_ptr<module> (new module ());
    this->curr_ns = "";
    this->mident = mident;
    
    this->compile_program (program);
    this->cgen.fix_labels ();
    
    auto m = this->mod;
    m->set_code (this->cgen.data (), this->cgen.size ());
    
    // add relocations
    for (auto& rel : this->cgen.get_relocs ())
      {
        m->add_reloc (rel.type, this->cgen.get_label_pos (rel.lbl), rel.mname);
      }
    
    this->mod.reset ();
    return m;
  }
  
  
  
  void
  compiler::compile_program (std::shared_ptr<ast_program> program)
  {
    this->mod->set_name (this->mident);
    
    auto& ent = this->mstore.retrieve (this->mident);
    if (!ent.pfrm)
      {
        var_analyzer va;
        for (auto p : this->known_globs)
          va.add_known_global (p.first, p.second);
        ent.pfrm = std::shared_ptr<var_frame> (
          new var_frame (va.analyze_program (program)));
      }
    
    this->var_frames.push (*ent.pfrm);
    auto& pfrm = this->var_frames.top ();
    this->mods_off = pfrm.get_mods_off ();
    
    this->func_frames.emplace ();
    auto& fframe = this->func_frames.top ();
    fframe.get_block_offs () = pfrm.get_block_offs ();
    
    this->scope_frames.emplace ();
    
    int lbl_cl = this->cgen.make_label ();
    
    // push program function
    this->cgen.emit_mk_fn (lbl_cl);
    this->cgen.emit_call (0);
    
    int lbl_end = this->cgen.make_label ();
    this->cgen.emit_jmp (lbl_end);
    
    this->cgen.mark_label (lbl_cl);
    
    // make room for global variables
    if (this->alloc_globs)
      {
        if (this->glob_count != -1)
          this->cgen.emit_alloc_globals (0, this->glob_count, false);
        else
          {
            this->cgen.rel_set_type (REL_GP);
            this->cgen.emit_alloc_globals (0, pfrm.global_count ());
          }
      }
    
    auto& stmts = program->get_stmts ();
    if (stmts.empty ())
      {
        this->cgen.emit_push_nil ();
      }
    else
      {
        for (size_t i = 0; i < stmts.size () - 1; ++i)
          {
            this->compile_stmt (stmts[i]);
          }
        
        // retain value of last expression on stack.
        auto last = stmts.back ();
        if (last->get_type () == AST_EXPR_STMT)
          this->compile_expr (std::static_pointer_cast<ast_expr_stmt> (last)->get_expr ());
        else
          {
            this->compile_stmt (last);
            this->cgen.emit_push_nil ();
          }
      }
    
    this->cgen.emit_ret ();
    this->var_frames.pop ();
    this->func_frames.pop ();
    this->scope_frames.pop ();
    
    this->cgen.mark_label (lbl_end);
  }
  
  
  
  void
  compiler::compile_expr_stmt (std::shared_ptr<ast_expr_stmt> stmt)
  {
    this->compile_expr (stmt->get_expr ());
    this->cgen.emit_pop ();
  }
  
  void
  compiler::compile_var_def (std::shared_ptr<ast_var_def> stmt)
  {
    auto name = this->qualify_name (stmt->get_var ()->get_value (), false);
    if (!this->func_frames.empty ())
      {
        auto& fr = this->func_frames.top ();
        if (fr.get_param (name) != -1)
          {
            this->errs.report (ERR_ERROR,
              "definition of variable '" + name + "' overshadows function parameter", 
              stmt->get_location ());
            return;
          }
      }
    
    auto& fs = this->var_frames.top ();
    auto var = fs.get_var (name);
    auto& scope = this->scope_frames.top ();
    scope.add_var (name, var);
    
    this->push_expr_frame (false);
    this->compile_expr (stmt->get_val ());
    this->pop_expr_frame ();
    
    switch (var.type)
      {
      case VAR_LOCAL:
        this->cgen.emit_set_local (var.idx);
        break;
      
      case VAR_GLOBAL:
        this->cgen.rel_set_type (REL_GP);
        this->cgen.emit_set_global (0, var.idx);
        break;
      
      default:
        throw std::runtime_error ("compile_var_def(): shouldn't happen");
      }
  }
  
  void
  compiler::compile_module (std::shared_ptr<ast_module> stmt)
  {
    
  }
  
  void
  compiler::compile_import (std::shared_ptr<ast_import> stmt)
  {
    auto mname = stmt->get_name ();
    auto mident = get_module_identifier (
      find_module (mname, this->idirs, this->wd).full_path); 
    
    auto& ent = this->mstore.retrieve (mident);
    
    this->mod->add_import (mident);
    
    auto exps = ast_tools::extract_exports (ent.ast);
    for (auto& exp : exps)
      {
        auto var = ent.pfrm->get_var (exp);
        if (var.type != VAR_GLOBAL)
          throw std::runtime_error ("compile_import(): shouldn't happen");
        this->name_imps[exp] = { .mident = mident, .idx = var.idx };
      }
  }
  
  void
  compiler::compile_export (std::shared_ptr<ast_export> stmt)
  {
    
  }
  
  void
  compiler::compile_ret (std::shared_ptr<ast_ret> stmt)
  {
    this->push_expr_frame (true);
    this->compile_expr (stmt->get_expr ());
    this->pop_expr_frame ();
    
    this->cgen.emit_ret ();
  }
  
  void
  compiler::compile_namespace (std::shared_ptr<ast_namespace> stmt)
  {
    auto& nname = stmt->get_name ();
    
    auto pns = this->curr_ns;
    if (this->curr_ns.empty ())
      this->curr_ns = nname;
    else
      this->curr_ns += ":" + nname;
    
    for (auto s : stmt->get_body ()->get_stmts ())
      this->compile_stmt (s);
    
    this->curr_ns = pns;
  }
  
  void
  compiler::compile_stmt (std::shared_ptr<ast_stmt> stmt)
  {
    switch (stmt->get_type ())
      {
      case AST_EXPR_STMT:
        this->compile_expr_stmt (std::static_pointer_cast<ast_expr_stmt> (stmt));
        break;
      
      case AST_VAR_DEF:
        this->compile_var_def (std::static_pointer_cast<ast_var_def> (stmt));
        break;
      
      case AST_MODULE:
        this->compile_module (std::static_pointer_cast<ast_module> (stmt));
        break;
      
      case AST_IMPORT:
        this->compile_import (std::static_pointer_cast<ast_import> (stmt));
        break;
      
      case AST_EXPORT:
        this->compile_export (std::static_pointer_cast<ast_export> (stmt));
        break;
      
      case AST_RET:
        this->compile_ret (std::static_pointer_cast<ast_ret> (stmt));
        break;
      
      case AST_NAMESPACE:
        this->compile_namespace (std::static_pointer_cast<ast_namespace> (stmt));
        break;
      
      default:
        throw std::runtime_error ("unhandled statement type");
      }
  }
  
  
  
  void
  compiler::compile_integer (std::shared_ptr<ast_integer> expr)
  {
    mpz_t num;
    mpz_init_set_str (num, expr->get_value ().c_str (), 10);
    
    if (mpz_cmp_si (num, 2147483647) <= 0 &&
        mpz_cmp_si (num, -2147483648) >= 0)
      {
        if (mpz_cmp_si (num, 10) <= 0 &&
            mpz_cmp_si (num, 0) >= 0)
          this->cgen.emit_push_sint (mpz_get_si (num));
        else
          this->cgen.emit_push_int32 (mpz_get_si (num));
      }
    else
      {
        // TODO
        throw std::runtime_error ("compile_integer: unimplemented");
      }
    
    mpz_clear (num);
  }
  
  void
  compiler::compile_ident (std::shared_ptr<ast_ident> expr)
  {
    auto name = this->qualify_name (expr->get_value ());
    
    if (this->pat_on)
      {
        // pattern variable
        
        auto itr = this->pat_pvs_unique.find (name);
        if (itr == this->pat_pvs_unique.end ())
          this->pat_pvs_unique[name] = this->pat_pvc;
        
        this->pat_pvs.push_back (name);
        this->cgen.emit_push_pvar (this->pat_pvc ++);
        
        return;
      }
    
    auto& scope = this->scope_frames.top ();
    auto var = scope.get_var (name);
    switch (var.type)
      {
      case VAR_ARG:
        //std::cout << "-- ident[" << name << "]: arg(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_arg (var.idx);
        return;
      
      case VAR_LOCAL:
        //std::cout << "-- ident[" << name << "]: local(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_local (var.idx);
        return;
      
      case VAR_UPVAL:
        //std::cout << "-- ident[" << name << "]: upval(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_free (var.idx);
        return;
      
      case VAR_GLOBAL:
        //std::cout << "-- ident[" << name << "]: global(" << var.idx << ")" << std::endl;
        this->cgen.rel_set_type (REL_GP);
        this->cgen.emit_get_global (0, var.idx);
        return;
      
      case VAR_UNDEF:
        break;
      }
    
    if (name == "$")
      {
        this->cgen.emit_get_fun ();
        return;
      }
    
    // check if it's a known global
    {
      auto itr = this->known_globs.find (name);
      if (itr != this->known_globs.end ())
        {
          //std::cout << "-- ident[" << name << "]: kglobal(" << itr->second << ")" << std::endl;
          this->cgen.emit_get_global (0, itr->second, false);
          return;
        }
    }
    
    // check if it's an imported name
    auto itr = this->name_imps.find (name);
    if (itr != this->name_imps.end ())
      {
        name_import_t& inf = itr->second;
        this->cgen.rel_set_type (REL_GV);
        this->cgen.rel_set_mname (inf.mident);
        this->cgen.emit_get_global (0, inf.idx);
        return; 
      }
    
    std::ostringstream ss;
    ss << "'" << expr->get_value () << "' was not declared in this scope";
    this->errs.report (ERR_ERROR, ss.str (), expr->get_location ());
  }
  
  void
  compiler::compile_nil (std::shared_ptr<ast_nil> expr)
  {
    this->cgen.emit_push_nil ();
  }
  
  void
  compiler::compile_bool (std::shared_ptr<ast_bool> expr)
  {
    if (expr->get_value ())
      this->cgen.emit_push_true ();
    else
      this->cgen.emit_push_false ();
  }
  
  void
  compiler::compile_unop (std::shared_ptr<ast_unop> expr)
  {
    this->push_expr_frame (false);
    this->compile_expr (expr->get_opr ());
    this->pop_expr_frame ();
    
    switch (expr->get_op ())
      {
      case AST_UNOP_NOT: this->cgen.emit_not (); break;
      }
  }
  
  void
  compiler::compile_binop (std::shared_ptr<ast_binop> expr)
  {
    this->push_expr_frame (false);
    this->compile_expr (expr->get_lhs ());
    this->compile_expr (expr->get_rhs ());
    this->pop_expr_frame ();
    
    switch (expr->get_op ())
      {
      case AST_BINOP_ADD: this->cgen.emit_add (); break;
      case AST_BINOP_SUB: this->cgen.emit_sub (); break;
      case AST_BINOP_MUL: this->cgen.emit_mul (); break;
      case AST_BINOP_DIV: this->cgen.emit_div (); break;
      case AST_BINOP_POW: this->cgen.emit_pow (); break;
      case AST_BINOP_MOD: this->cgen.emit_mod (); break;
      
      case AST_BINOP_AND: this->cgen.emit_and (); break;
      case AST_BINOP_OR: this->cgen.emit_or (); break;
      
      case AST_BINOP_EQ: this->cgen.emit_cmp_eq (); break;
      case AST_BINOP_NEQ: this->cgen.emit_cmp_neq (); break;
      case AST_BINOP_LT: this->cgen.emit_cmp_lt (); break;
      case AST_BINOP_LTE: this->cgen.emit_cmp_lte (); break;
      case AST_BINOP_GT: this->cgen.emit_cmp_gt (); break;
      case AST_BINOP_GTE: this->cgen.emit_cmp_gte (); break;
      }
  }
  
  void
  compiler::compile_fun (std::shared_ptr<ast_fun> expr)
  {
    var_analyzer va;
    va.set_namespace (this->curr_ns);
    this->var_frames.push (va.analyze_function (expr, this->var_frames.top ()));
    auto& fs = this->var_frames.top ();
    
    int lbl_fn = this->cgen.make_label ();
    int lbl_cfn = this->cgen.make_label ();
    this->cgen.emit_jmp (lbl_cfn);
    
    {
      // function body
      
      this->cgen.mark_label (lbl_fn);
      
      // allocate room for local variables
      this->cgen.emit_push_nils (fs.local_count ());
      
      this->func_frames.emplace ();
      auto& fr = this->func_frames.top ();
      fr.get_block_offs () = fs.get_block_offs ();
      
      auto pscope = this->scope_frames.empty () ? nullptr : &this->scope_frames.top ();
      this->scope_frames.emplace ();
      auto& scope = this->scope_frames.top ();
      if (pscope)
        scope.set_parent (*pscope);
      
      // register parameters
      int pi = 0;
      for (auto param : expr->get_params ())
        {
          fr.add_param (param->get_value (), pi);
          scope.add_var (param->get_value (), { .type = VAR_ARG, .idx = pi });
          ++ pi;
        }
      
      // register upvalues
      auto& nfrees = fs.get_new_frees ();
      for (auto p : nfrees)
        scope.add_var (p.first, { .type = VAR_UPVAL, .idx = p.second });
      
      auto& stmts = expr->get_body ()->get_stmts ();
      if (stmts.empty ())
        this->cgen.emit_push_nil ();
      else
        {
          this->push_expr_frame (false);
          for (size_t i = 0; i < stmts.size () - 1; ++i)
            this->compile_stmt (stmts[i]);
          this->pop_expr_frame ();
            
          // handle last statement
          this->push_expr_frame (true);
          auto last = stmts.back ();
          if (last->get_type () == AST_EXPR_STMT)
            this->compile_expr (std::static_pointer_cast<ast_expr_stmt> (last)->get_expr ());
          else
            {
              this->compile_stmt (last);
              this->cgen.emit_push_nil ();
            }
          this->cgen.emit_ret ();
          this->pop_expr_frame ();
        }
      
      this->func_frames.pop ();
      this->scope_frames.pop ();
    }
    
    this->cgen.mark_label (lbl_cfn);
    
    auto& nfrees = fs.get_new_frees ();
    this->cgen.emit_mk_closure (nfrees.size (), lbl_fn);
    for (auto p : nfrees)
      {
        auto pfs = fs.get_parent ();
        auto var = pfs->get_var (p.first);
        switch (var.type)
          {
          case VAR_LOCAL:
            this->cgen.emit_get_local (var.idx);
            break;
          
          case VAR_ARG:
            this->cgen.emit_get_arg (var.idx);
            break;
          
          default:
            throw std::runtime_error ("compile_fun(): shouldn't happen");
          }
      }
    
    
    this->var_frames.pop ();
    
  }
  
  void
  compiler::compile_fun_call (std::shared_ptr<ast_fun_call> expr)
  {
    // check for possibility for tail calls
    bool tail = false;
    if (expr->get_fun ()->get_type () == AST_IDENT)
      {
        auto& name = std::static_pointer_cast<ast_ident> (expr->get_fun ())->get_value ();
        if (this->can_perform_tail_call () && name == "$")
          tail = true;
      }
    
    // check for builtins
    if (expr->get_fun ()->get_type () == AST_IDENT)
      if (this->compile_builtin (expr))
        return;
    
    // push arguments (in reverse order)
    auto& args = expr->get_args ();
    for (auto itr = args.rbegin(); itr != args.rend (); ++itr)
      this->compile_expr (*itr);
    
    // push function
    if (!tail)
      this->compile_expr (expr->get_fun ());
    
    if (tail)
      this->cgen.emit_tail_call ();
    else
      this->cgen.emit_call (args.size ());
  }
  
  void
  compiler::compile_if (std::shared_ptr<ast_if> expr)
  {
    int lbl_false = this->cgen.make_label ();
    int lbl_end = this->cgen.make_label ();
    
    this->push_expr_frame (false);
    this->compile_expr (expr->get_test ());
    this->pop_expr_frame ();
    this->cgen.emit_jf (lbl_false);
    
    this->compile_expr (expr->get_consequent ());
    this->cgen.emit_jmp (lbl_end);
    
    this->cgen.mark_label (lbl_false);
    this->compile_expr (expr->get_antecedent ());
    
    this->cgen.mark_label (lbl_end);
  }
  
  void
  compiler::compile_cons (std::shared_ptr<ast_cons> expr)
  {
    this->push_expr_frame (false);
    this->compile_expr (expr->get_fst ());
    this->compile_expr (expr->get_snd ());
    this->cgen.emit_cons ();
    this->pop_expr_frame ();
  }
  
  void
  compiler::compile_list (std::shared_ptr<ast_list> expr)
  {
    this->push_expr_frame (false);
    auto& elems = expr->get_elems ();
    for (auto e : elems)
      this->compile_expr (e);
    this->cgen.emit_push_empty_list ();
    for (int i = 0; i < (int)elems.size (); ++i)
      this->cgen.emit_cons ();
    this->pop_expr_frame ();
  }
  
  
  
  void
  compiler::compile_match (std::shared_ptr<ast_match> expr)
  {
    this->push_expr_frame (false);
    this->compile_expr (expr->get_expr ());
    this->pop_expr_frame ();
    
    int lbl_end = this->cgen.make_label ();
    int lbl_prev = -1, lbl_next;
    
    for (auto& c : expr->get_cases ())
      {
        if (lbl_prev != -1)
          this->cgen.mark_label (lbl_prev);
        lbl_next = this->cgen.make_label ();
        
        this->cgen.emit_dup ();
        
        this->pat_on = true;
        this->pat_pvc = 0;
        this->pat_pvs.clear ();
        this->pat_pvs_unique.clear ();
        this->compile_expr (c.pat);
        this->pat_on = false;
        
        auto pat_pvs = this->pat_pvs;
        auto pat_pvs_unique = this->pat_pvs_unique;
        
        auto& fr = this->func_frames.top ();
        int off = fr.get_block_offs ()[this->match_depth];
        this->cgen.emit_match (off);
        this->cgen.emit_jf (lbl_next);
        
        // make sure multiple occurrences of the same pattern variable are all
        // bound to equal objects.
        if (pat_pvs.size () > pat_pvs_unique.size ())
          {
            std::unordered_map<std::string, std::vector<int>> grps;
            for (int i = 0; i < (int)pat_pvs.size (); ++i)
              {
                auto& vec = grps[pat_pvs[i]];
                vec.push_back (i);
              }
            
            for (auto p : grps)
              {
                auto& vec = p.second;
                if (vec.size () > 1)
                  {
                    for (int i = 0; i < (int)vec.size (); ++i)
                      this->cgen.emit_get_local (off + vec[i]);
                    
                    this->cgen.emit_cmp_eq_many (vec.size ());
                    this->cgen.emit_jf (lbl_next);
                  }
              }
          }
        
        // match succeeded at this point
        {
          auto pscope = this->scope_frames.empty () ? nullptr : &this->scope_frames.top ();
          this->scope_frames.emplace ();
          auto& scope = this->scope_frames.top ();
          if (pscope)
            scope.set_parent (*pscope);
          
          for (auto& p : pat_pvs_unique)
            {
              int fidx = off + p.second;
              scope.add_var (p.first, { .type = VAR_LOCAL, .idx = fidx });
            }
          
          ++ this->match_depth;
          this->compile_expr (c.body);
          this->cgen.emit_ret ();
          -- this->match_depth;
          
          this->scope_frames.pop ();
        }
        this->cgen.emit_jmp (lbl_end);
        
        lbl_prev = lbl_next;
      }
    
    if (lbl_prev != -1)
      this->cgen.mark_label (lbl_prev);
    
    if (expr->get_else_body ())
      this->compile_expr (expr->get_else_body ());
    else
      this->cgen.emit_push_nil ();
    
    this->cgen.mark_label (lbl_end);
  }
  
  
  
  void
  compiler::compile_vector (std::shared_ptr<ast_vector> expr)
  {
    auto& elems = expr->get_exprs ();
    
    for (auto itr = elems.rbegin (); itr != elems.rend (); ++itr)
      {
        this->push_expr_frame (false);
        this->compile_expr (*itr);
        this->pop_expr_frame ();
      }
    
    this->cgen.emit_mk_vec (elems.size ());
  }
  
  
  
  void
  compiler::compile_subscript (std::shared_ptr<ast_subscript> expr)
  {
    this->push_expr_frame (false);
    this->compile_expr (expr->get_expr ());
    this->pop_expr_frame ();
    
    this->push_expr_frame (false);
    this->compile_expr (expr->get_index ());
    this->pop_expr_frame ();
    
    this->cgen.emit_vec_get ();
  }
  
  
  
  void
  compiler::compile_expr (std::shared_ptr<ast_expr> expr)
  {
    switch (expr->get_type ())
      {
      case AST_INTEGER:
        this->compile_integer (std::static_pointer_cast<ast_integer> (expr));
        break;
      
      case AST_IDENT:
        this->compile_ident (std::static_pointer_cast<ast_ident> (expr));
        break;
      
      case AST_NIL:
        this->compile_nil (std::static_pointer_cast<ast_nil> (expr));
        break;
      
      case AST_BOOL:
        this->compile_bool (std::static_pointer_cast<ast_bool> (expr));
        break;
      
      case AST_UNOP:
        this->compile_unop (std::static_pointer_cast<ast_unop> (expr));
        break;
      
      case AST_BINOP:
        this->compile_binop (std::static_pointer_cast<ast_binop> (expr));
        break;
      
      case AST_FUN:
        this->compile_fun (std::static_pointer_cast<ast_fun> (expr));
        break;
      
      case AST_FUN_CALL:
        this->compile_fun_call (std::static_pointer_cast<ast_fun_call> (expr));
        break;
      
      case AST_IF:
        this->compile_if (std::static_pointer_cast<ast_if> (expr));
        break;
      
      case AST_CONS:
        this->compile_cons (std::static_pointer_cast<ast_cons> (expr));
        break;
      
      case AST_LIST:
        this->compile_list (std::static_pointer_cast<ast_list> (expr));
        break;
      
      case AST_MATCH:
        this->compile_match (std::static_pointer_cast<ast_match> (expr));
        break;
      
      case AST_VECTOR:
        this->compile_vector (std::static_pointer_cast<ast_vector> (expr));
        break;
      
      case AST_SUBSCRIPT:
        this->compile_subscript (std::static_pointer_cast<ast_subscript> (expr));
        break;
      
      default:
        throw std::runtime_error ("unhandled expression type");
      }
  }
}

