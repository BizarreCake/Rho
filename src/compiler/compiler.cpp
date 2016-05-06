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
  compiler::add_known_atom (const std::string& name)
  {
    this->known_atoms.insert (name);
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
  compiler::qualify_name (const std::string& name,
                          std::shared_ptr<scope_frame> scope)
  {
    auto& u_ns = scope->get_used_namespaces ();
    for (auto& ns : u_ns)
      {
        std::string qn = ns + ":" + name;
        if (scope->get_var (qn).type != VAR_UNDEF
          || (this->name_imps.find (qn) != this->name_imps.end ()))
          return qn;
      }
    
    auto& u_aliases = scope->get_aliases ();
    for (auto& p : u_aliases)
      {
        auto idx = name.find (p.first);
        if (idx == 0)
          {
            std::string qn = p.second + name.substr (idx + p.first.length ());
            if (scope->get_var (qn).type != VAR_UNDEF
              || (this->name_imps.find (qn) != this->name_imps.end ()))
              return qn;
          }
      }
    
    auto qn = this->curr_ns.empty () ? name : (this->curr_ns + ":" + name);
    if (scope->get_var (qn).type != VAR_UNDEF
      || (this->name_imps.find (qn) != this->name_imps.end ()))
      return qn;
    
    return name;
  }
  
  std::string
  compiler::qualify_atom_name (const std::string& name, bool check_exists)
  {
    auto qn = this->curr_ns;
    if (qn.empty ())
      qn = name;
    else
      qn += ":" + name;
    
    if (!check_exists || (this->atoms.find (qn) != this->atoms.end ()
      || this->known_atoms.find (qn) != this->known_atoms.end ()))
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
    this->atoms.clear ();
    
    this->compile_program (program);
    this->cgen.fix_labels ();
    
    auto m = this->mod;
    m->set_code (this->cgen.data (), this->cgen.size ());
    
    // add relocations
    for (auto& rel : this->cgen.get_relocs ())
      {
        m->add_reloc (rel.type, this->cgen.get_label_pos (rel.lbl), rel.mname,
          rel.val);
      }
    
    this->mod.reset ();
    return m;
  }
  
  
  
  void
  compiler::compile_program (std::shared_ptr<ast_program> program)
  {
    this->prg_ast = program;
    this->mod->set_name (this->mident);
    
    auto& ent = this->mstore.retrieve (this->mident);
    if (ent.van)
      this->van = ent.van;
    else
      {
        var_analyzer va;
        for (auto p : this->known_globs)
          va.add_known_global (p.first, p.second);
        this->van = ent.van = std::shared_ptr<var_analysis> (
          new var_analysis (va.analyze (program)));
      }
    
    int lbl_cl = this->cgen.make_label ();
    
    // push program function
    this->cgen.emit_mk_fn (lbl_cl);
    this->cgen.emit_call0 (0);
    
    int lbl_end = this->cgen.make_label ();
    this->cgen.emit_jmp (lbl_end);
    
    this->cgen.mark_label (lbl_cl);
    
    // allocate room for local variables
    auto fun_f = this->van->get_scope (program)->get_fun ();
    this->cgen.emit_push_nils (fun_f->get_local_count ());
    
    // make room for global variables
    if (this->alloc_globs)
      {
        if (this->glob_count != -1)
          this->cgen.emit_alloc_globals (0, this->glob_count, false);
        else
          {
            this->cgen.rel_set_type (REL_GP);
            this->cgen.emit_alloc_globals (0,
              this->van->get_scope (program)->get_global_count ());
          }
      }
    
    // push initial micro-frame
    this->cgen.emit_push_int32 (10);
    this->cgen.emit_push_microframe ();
    
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
    this->cgen.mark_label (lbl_end);
  }
  
  
  
  void
  compiler::compile_empty_stmt (std::shared_ptr<ast_empty_stmt> stmt)
  {
    
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
    auto name = stmt->get_var ()->get_value ();
    auto qn = name;
    if (this->van->get_scope (stmt) == this->van->get_scope (this->prg_ast))
      qn = (this->curr_ns.empty () ? name : (this->curr_ns + ":" + name));
    
    this->push_expr_frame (false);
    this->compile_expr (stmt->get_val ());
    this->pop_expr_frame ();
    
    auto scope = this->van->get_scope (stmt->get_var ());
    auto var = scope->get_var (qn);
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
    
    if (!ent.van)
      {
        var_analyzer va;
        ent.van = std::shared_ptr<var_analysis> (
          new var_analysis (va.analyze (ent.ast)));
      }
    
    auto scope = ent.van->get_scope (ent.ast);
    auto exps = ast_tools::extract_exports (ent.ast);
    for (auto& exp : exps)
      {
        auto var = scope->get_var (exp);
        if (var.type != VAR_GLOBAL)
          {
            this->errs.report (ERR_FATAL,
              "module '" + mname + "' exports undefined function: " + exp,
              stmt->get_location());
            return;
          }
        this->name_imps[exp] = { .mident = mident, .idx = var.idx };
      }
    
    auto atoms = ast_tools::extract_atom_defs (ent.ast);
    for (auto& atom : atoms)
      this->atoms.insert (atom);
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
    
    auto fun_f = this->van->get_scope (stmt)->get_fun ();
    if (!fun_f->get_cfrees ().empty ())
        this->cgen.emit_close (fun_f->get_local_count ());
    
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
  compiler::compile_atom_def (std::shared_ptr<ast_atom_def> stmt)
  {
    auto qn = this->qualify_atom_name (stmt->get_name (), false);
    this->atoms.insert (qn);
    this->mod->add_atom (qn);
    
    this->cgen.rel_set_type (REL_A);
    this->cgen.rel_set_val (qn);
    this->cgen.emit_def_atom (0, qn);
  }
  
  
  
  void
  compiler::compile_stmt_block (std::shared_ptr<ast_stmt_block> stmt)
  {
    for (auto s : stmt->get_stmts ())
      this->compile_stmt (s);
  }
  
  
  
  void
  compiler::compile_using (std::shared_ptr<ast_using> stmt)
  {
    
  }
  
  
  
  void
  compiler::compile_stmt (std::shared_ptr<ast_stmt> stmt)
  {
    switch (stmt->get_type ())
      {
      case AST_EMPTY_STMT:
        this->compile_empty_stmt (std::static_pointer_cast<ast_empty_stmt> (stmt));
        break;
      
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
      
      case AST_ATOM_DEF:
        this->compile_atom_def (std::static_pointer_cast<ast_atom_def> (stmt));
        break;
      
      case AST_STMT_BLOCK:
        this->compile_stmt_block (std::static_pointer_cast<ast_stmt_block> (stmt));
        break;
      
      case AST_USING:
        this->compile_using (std::static_pointer_cast<ast_using> (stmt));
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
  compiler::compile_float (std::shared_ptr<ast_float> expr)
  {
    std::istringstream ss { expr->get_value () };
    double val;
    ss >> val;
    
    this->cgen.emit_push_float (val);
  }
  
  void
  compiler::compile_ident (std::shared_ptr<ast_ident> expr)
  {
    auto name = expr->get_value ();
    
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
    
    auto qn = this->qualify_name (name, this->van->get_scope (expr));
    auto scope = this->van->get_scope (expr);
    auto var = scope->get_var (qn);
    switch (var.type)
      {
      case VAR_ARG:
        //std::cout << "-- ident[" << qn << "]: arg(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_arg (var.idx);
        return;
      
      case VAR_LOCAL:
        //std::cout << "-- ident[" << qn << "]: local(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_local (var.idx);
        return;
      
      case VAR_UPVAL:
        //std::cout << "-- ident[" << qn << "]: upval(" << var.idx << ")" << std::endl;
        this->cgen.emit_get_free (var.idx);
        return;
      
      case VAR_GLOBAL:
        //std::cout << "-- ident[" << qn << "]: global(" << var.idx << ")" << std::endl;
        this->cgen.rel_set_type (REL_GP);
        this->cgen.emit_get_global (0, var.idx);
        return;
      
      case VAR_UNDEF:
        break;
      }
    
    if (qn == "$")
      {
        this->cgen.emit_get_fun ();
        return;
      }
    
    // check if it's a known global
    {
      auto itr = this->known_globs.find (qn);
      if (itr != this->known_globs.end ())
        {
          //std::cout << "-- ident[" << naqnme << "]: kglobal(" << itr->second << ")" << std::endl;
          this->cgen.emit_get_global (0, itr->second, false);
          return;
        }
    }
    
    // check if it's an imported name
    auto itr = this->name_imps.find (qn);
    if (itr != this->name_imps.end ())
      {
        name_import_t& inf = itr->second;
        this->cgen.rel_set_type (REL_GV);
        this->cgen.rel_set_mname (inf.mident);
        this->cgen.emit_get_global (0, inf.idx);
        return; 
      }
    
    std::ostringstream ss;
    ss << "'" << name << "' was not declared in this scope";
    this->errs.report (ERR_ERROR, ss.str (), expr->get_location ());
  }
  
  void
  compiler::compile_atom (std::shared_ptr<ast_atom> expr)
  {
    auto qn = this->qualify_atom_name (expr->get_value ());
    if (this->atoms.find (qn) == this->atoms.end () &&
      this->known_atoms.find (qn) == this->known_atoms.end ())
      {
        this->errs.report (ERR_ERROR, "unrecognized atom '" + qn + "'",
          expr->get_location ());
        return;
      }
    
    this->cgen.rel_set_type (REL_A);
    this->cgen.rel_set_val (qn);
    this->cgen.emit_push_atom (0);
  }
  
  void
  compiler::compile_string (std::shared_ptr<ast_string> expr)
  {
    this->cgen.emit_push_cstr (expr->get_value ());
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
    if (expr->get_op () == AST_BINOP_ASSIGN)
      {
        this->compile_assign (expr->get_lhs (), expr->get_rhs ());
        return;
      }
    
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
      
      case AST_BINOP_ASSIGN:
        break;
      }
  }
  
  void
  compiler::compile_fun (std::shared_ptr<ast_fun> expr)
  {
    int lbl_fn = this->cgen.make_label ();
    int lbl_cfn = this->cgen.make_label ();
    this->cgen.emit_jmp (lbl_cfn);
    
    auto fun_s = this->van->get_scope (expr->get_body ());
    auto fun_f = fun_s->get_fun ();
    
    {
      // function body
      
      this->cgen.mark_label (lbl_fn);
      
      // allocate room for local variables
      this->cgen.emit_push_nils (fun_f->get_local_count ());
      
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
          
          if (!fun_f->get_cfrees ().empty ())
            this->cgen.emit_close (fun_f->get_local_count ());
          
          this->cgen.emit_ret ();
          this->pop_expr_frame ();
        }
    }
    
    this->cgen.mark_label (lbl_cfn);
    
    int non_upval_count = 0;
    auto nfrees = fun_f->get_sorted_nfrees ();
    auto scope = this->van->get_scope (expr);
    for (auto p : nfrees)
      {
        auto var = scope->get_var (p.first);
        if (var.type == VAR_LOCAL || var.type == VAR_ARG)
          ++ non_upval_count;
      }
    
    this->cgen.emit_mk_closure (non_upval_count, lbl_fn);
    for (auto p : nfrees)
      {
        auto var = scope->get_var (p.first);
        switch (var.type)
          {
          case VAR_LOCAL:
            this->cgen.emit_get_local (var.idx);
            break;
          
          case VAR_ARG:
            this->cgen.emit_get_arg (var.idx);
            break;
          
          case VAR_UPVAL:
            break;
          
          default:
            throw std::runtime_error ("compile_fun(): shouldn't happen");
          }
      }
  }
  
  void
  compiler::compile_fun_call (std::shared_ptr<ast_fun_call> expr)
  {
    // check for possibility for tail calls and builtins
    bool tail = false;
    if (expr->get_fun ()->get_type () == AST_IDENT)
      {
        auto& name = std::static_pointer_cast<ast_ident> (expr->get_fun ())->get_value ();
        if (this->can_perform_tail_call () && name == "$")
          tail = true;
        
        auto qn = this->qualify_name (name, this->van->get_scope (expr));
        auto scope = this->van->get_scope (expr);
        auto var = scope->get_var (qn);
        if (var.type == VAR_UNDEF && this->name_imps.find (qn) == this->name_imps.end ())
          {
            if (this->compile_builtin (expr))
              return;
          }
      }
    
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
        
        auto fun_f = this->van->get_scope (expr)->get_fun ();
        int off = fun_f->get_block_off (this->van->get_scope (expr)->get_scope_depth () + 1);
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
          ++ this->match_depth;
          this->compile_expr (c.body);
          -- this->match_depth;
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
  compiler::compile_assign_to_ident (std::shared_ptr<ast_ident> lhs,
                                     std::shared_ptr<ast_expr> rhs)
  {
    this->push_expr_frame (false);
    this->compile_expr (rhs);
    this->pop_expr_frame ();
    this->cgen.emit_dup ();
    
    auto name = this->qualify_name (lhs->get_value (), this->van->get_scope (lhs));
    
    auto scope = this->van->get_scope (lhs);
    auto var = scope->get_var (name);
    switch (var.type)
      {
      case VAR_ARG:
        this->cgen.emit_set_arg (var.idx);
        return;
      
      case VAR_LOCAL:
        this->cgen.emit_set_local (var.idx);
        return;
      
      case VAR_UPVAL:
        this->cgen.emit_set_free (var.idx);
        return;
      
      case VAR_GLOBAL:
        this->cgen.rel_set_type (REL_GP);
        this->cgen.emit_set_global (0, var.idx);
        return;
      
      case VAR_UNDEF:
        break;
      }
    
    // check if it's a known global
    {
      auto itr = this->known_globs.find (name);
      if (itr != this->known_globs.end ())
        {
          this->cgen.emit_set_global (0, itr->second, false);
          return;
        }
    }
    
    this->errs.report (ERR_ERROR,
      "'" + lhs->get_value () + "' was not declared in this scope",
      lhs->get_location ());
  }
  
  void
  compiler::compile_assign_to_subscript (std::shared_ptr<ast_subscript> lhs,
                                         std::shared_ptr<ast_expr> rhs)
  {
    this->push_expr_frame (false);
    this->compile_expr (rhs);
    this->pop_expr_frame ();
  
    this->push_expr_frame (false);
    this->compile_expr (lhs->get_expr ());
    this->pop_expr_frame ();
    
    this->push_expr_frame (false);
    this->compile_expr (lhs->get_index ());
    this->pop_expr_frame ();
    
    this->cgen.emit_dup_n (3);
    this->cgen.emit_vec_set ();
  }
  
  void
  compiler::compile_assign (std::shared_ptr<ast_expr> lhs,
                            std::shared_ptr<ast_expr> rhs)
  {
    switch (lhs->get_type ())
      {
      case AST_IDENT:
        this->compile_assign_to_ident (std::static_pointer_cast<ast_ident> (lhs), rhs);
        break;
      
      case AST_SUBSCRIPT:
        this->compile_assign_to_subscript (std::static_pointer_cast<ast_subscript> (lhs), rhs);
        break;
      
      default:
        this->errs.report (ERR_ERROR, "invalid left-hand side type in assignment",
          lhs->get_location ());
      }
  }
  
  
  
  void
  compiler::compile_expr_block (std::shared_ptr<ast_expr_block> expr)
  {
    auto& stmts = expr->get_stmts ();
    
    this->push_expr_frame (false);
    for (size_t i = 0; i < stmts.size () - 1; ++i)
      this->compile_stmt (stmts[i]);
    this->pop_expr_frame ();
      
    // handle last statement
    auto last = stmts.back ();
    if (last->get_type () == AST_EXPR_STMT)
      this->compile_expr (std::static_pointer_cast<ast_expr_stmt> (last)->get_expr ());
    else
      {
        this->compile_stmt (last);
        this->cgen.emit_push_nil ();
      }
  }
  
  
  
  void
  compiler::compile_let (std::shared_ptr<ast_let> expr)
  {
    for (auto& p : expr->get_defs ())
      {
        this->compile_expr (p.second);
        
        auto scope = this->van->get_scope (p.second);
        auto var = scope->get_var (p.first);
        if (var.type != VAR_LOCAL)
          throw std::runtime_error ("compile_let(): shouldn't happen");
        
        this->cgen.emit_set_local (var.idx);
      }
    
    this->compile_expr (expr->get_body ());
  }
  
  
  
  void 
  compiler::compile_n (std::shared_ptr<ast_n> expr)
  {
    this->compile_expr (expr->get_prec ());
    this->cgen.emit_push_microframe ();
    this->compile_expr (expr->get_body ());
    this->cgen.emit_pop_microframe ();
  }
  
  
  
  void
  compiler::compile_expr (std::shared_ptr<ast_expr> expr)
  {
    switch (expr->get_type ())
      {
      case AST_INTEGER:
        this->compile_integer (std::static_pointer_cast<ast_integer> (expr));
        break;
      
      case AST_FLOAT:
        this->compile_float (std::static_pointer_cast<ast_float> (expr));
        break;
      
      case AST_ATOM:
        this->compile_atom (std::static_pointer_cast<ast_atom> (expr));
        break;
      
      case AST_STRING:
        this->compile_string (std::static_pointer_cast<ast_string> (expr));
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
      
      case AST_EXPR_BLOCK:
        this->compile_expr_block (std::static_pointer_cast<ast_expr_block> (expr));
        break;
      
      case AST_LET:
        this->compile_let (std::static_pointer_cast<ast_let> (expr));
        break;
      
      case AST_N:
        this->compile_n (std::static_pointer_cast<ast_n> (expr));
        break;
      
      default:
        throw std::runtime_error ("unhandled expression type");
      }
  }
}

