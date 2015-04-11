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

#include "compiler/compiler.hpp"
#include "compiler/codegen.hpp"
#include "compiler/asttools.hpp"
#include <stdexcept>

#include <iostream> // DEBUG


namespace rho {
  
  compiler::compiler (error_tracker& errs)
    : errs (errs)
  {
    this->mod = nullptr;
    this->cgen = nullptr;
    this->in_repl = false;
  }
  
  compiler::~compiler ()
  {
    delete this->cgen;
    
    for (frame *frm : this->frms)
      delete frm;
    for (expr_frame *efrm : this->efrms)
      delete efrm;
    for (function_info *fi : this->funcs)
      delete fi;
  }
  
  
  
  /* 
   * Frame manipulation.
   */
//------------------------------------------------------------------------------
  
  /* 
   * Creates a new frame of the specified type.
   */
  void
  compiler::push_frame (frame_type type)
  {
    frame *frm = new frame (type,
      this->frms.empty () ? nullptr : this->frms.back ());
    this->frms.push_back (frm);
  }
  
  /* 
   * Destroys the top-most frame.
   */
  void
  compiler::pop_frame ()
  {
    frame *frm = this->frms.back ();
    delete frm;
    this->frms.pop_back ();
  }
  
  /* 
   * Returns the top-most frame.
   */
  frame&
  compiler::top_frame ()
  {
    return *this->frms.back ();
  }
  
  
  
  void
  compiler::push_expr_frame (bool last)
  {
    expr_frame *efrm = new expr_frame ();
    efrm->set_last (last);
    this->efrms.push_back (efrm);
  }
  
  void
  compiler::pop_expr_frame ()
  {
    expr_frame *efrm = this->efrms.back ();
    delete efrm;
    this->efrms.pop_back ();
  }
  
  bool
  compiler::can_perform_tail_call ()
  {
    if (this->efrms.empty ())
      return false;
    
    return this->efrms.back ()->is_last ();
  }
  
  
  
  /* 
   * Tells the compiler that this module should be compiled in a REPL
   * configuration.
   */
  void
  compiler::set_repl ()
  {
    this->push_frame (FT_REPL);
    this->in_repl = true;
  }
  
  /* 
   * Tells the compiler that any variables with the specified name should
   * be treated as global REPL variables.
   */
  void
  compiler::add_repl_var (const std::string& name, int index)
  {
    if (!this->in_repl)
      throw std::runtime_error ("compiler not in repl mode");
    
    frame *frm = this->frms.front ();
    frm->add_local (name, index);
  }
  
//------------------------------------------------------------------------------
  
  
  
  void
  compiler::compile_program (ast_program *prog)
  {
    // create initial frame
    int locs = ast::count_locs_needed (prog);
    //std::cout << locs << " locals" << std::endl;
    this->cgen->emit_push_frame (locs);
    this->push_frame (FT_PROGRAM);
    
    auto& stmts = prog->get_stmts ();
    for (auto stmt : stmts)
      {
        this->compile_stmt (stmt);
      }
    
    this->cgen->emit_pop_frame ();
    this->pop_frame ();
    
    this->cgen->emit_exit ();
  }
  
  
  /* 
   * Compiles the specified AST program tree into a module.
   */
  module*
  compiler::compile (ast_program *prog)
  {
    this->mod = new module ();
    this->mod->add_section ("code");
    this->mod->add_section ("data");
    this->cgen = new code_generator (this->mod->find_section ("code")->data);
    
    this->compile_program (prog);
    this->cgen->fix_labels ();
    
    return this->mod;
  }
}

