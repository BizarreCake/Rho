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

#include "runtime/repl.hpp"
#include "util/ast_tools.hpp"
#include "compiler/module_store.hpp"
#include "parse/lexer.hpp"
#include "parse/parser.hpp"
#include "compiler/compiler.hpp"
#include "linker/linker.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>


namespace rho {
  
#define GLOBAL_PREALLOC_COUNT   1024
  
  
  rho_repl::rho_repl ()
    : mstore (), comp (mstore)
  {
    this->idirs.push_back (boost::filesystem::current_path ().generic_string ());
    this->run_num = 0;
    this->next_glob = 0;
    this->next_mod = 1;
  }
  
  rho_repl::~rho_repl ()
  {
    
  }
  
  
  
  void
  rho_repl::print_intro ()
  {
    std::cout << "   ___    _                         ___     ___      ___    _     \n";
    std::cout << "  | _ \\  | |_      ___      o O O  | _ \\   | __|    | _ \\  | |    \n";
    std::cout << "  |   /  | ' \\    / _ \\    o       |   /   | _|     |  _/  | |__  \n";
    std::cout << "  |_|_\\  |_||_|   \\___/   TS__[O]  |_|_\\   |___|   _|_|_   |____| \n";
    std::cout << "_|\"\"\"\"\"|_|\"\"\"\"\"|_|\"\"\"\"\"| {======|_|\"\"\"\"\"|_|\"\"\"\"\"|_| \"\"\" |_|\"\"\"\"\"| \n";
    std::cout << "\"`-0-0-'\"`-0-0-'\"`-0-0-'./o--000'\"`-0-0-'\"`-0-0-'\"`-0-0-'\"`-0-0-' \n";
    std::cout << std::endl;
    std::cout << std::endl;
  }
  
  
  
  static void
  _handle_compiler_errors (error_list& errs)
  {
    for (auto ent : errs.get_entries ())
      {
        std::cout << ent.path << ":";
        if (ent.ln != -1 && ent.col != -1)
          std::cout << ent.ln << ":" << ent.col << ":";
        std::cout << " ";
        switch (ent.type)
          {
          case ERR_INFO: std::cout << "info: "; break;
          case ERR_WARNING: std::cout << "warning: "; break;
          case ERR_ERROR: std::cout << "error: "; break;
          case ERR_FATAL: std::cout << "fatal: "; break;
          }
        
        std::cout << ent.msg << std::endl;
      }
    
    errs.clear ();
  }
  
  bool
  rho_repl::parse_module (std::istream& strm,
                 const std::string& full_path, const std::string& dir_path,
                 std::stack<module_location>& parse_work)
  {
    if (!full_path.empty () && this->mstore.contains (get_module_identifier (full_path)))
      return true;
    
    std::string rpath = full_path;
    
    // tokenize
    rho::lexer lex;
    std::unique_ptr<rho::lexer::token_stream> ts;
    try
      {
        ts.reset (new rho::lexer::token_stream (lex.tokenize (strm)));
      }
    catch (const rho::lexer_error& ex)
      {
        std::cout << rpath << ":" << ex.get_line () << ":" << ex.get_column ()
                  << ": lexer error: " << ex.what () << std::endl;
        return false;
      }
    
    // parse
    rho::parser parser;
    std::shared_ptr<rho::ast_program> p;
    try
      {
        p = parser.parse (*ts, rpath);
      }
    catch (const rho::parse_error& ex)
      {
        std::cout << rpath << ":" << ex.get_line () << ":" << ex.get_column ()
                  << ": parse error: " << ex.what () << std::endl;
        return false;
      }
    
    std::string mname = rho::ast_tools::extract_module_name (p);
    if (mname.empty () && !full_path.empty ())
      {
        std::cout << rpath << ": fatal error: module name not specified" << std::endl;
        return false;
      }
    
    auto ident = full_path.empty () ? "#this#"
                                    : get_module_identifier (full_path);
    this->mstore.store (ident, p);
    auto& ent = this->mstore.retrieve (ident);
    ent.full_path = full_path;
    ent.dir_path = dir_path;
    ent.mname = mname;
    
    // enqueue all imported modules to be parsed.
    auto imps = ast_tools::extract_imports (p);
    for (auto& m : imps)
      {
        try
          {
            auto loc = find_module (m, this->idirs, ent.dir_path);
            parse_work.push (loc);
            
            if (full_path.empty ())
              {
                auto itr = this->imps.find (m);
                if (itr == this->imps.end ())
                  {
                    auto& ent = this->imps[m];
                    ent.name = m;
                    ent.ident = get_module_identifier (loc.full_path);
                  }
              }
          }
        catch (const rho::module_not_found_error&)
          {
            std::cout << rpath << ": fatal error: unrecognized module '" << m << "'" << std::endl;
            return false;
          }
      }
    
    return true;
  }
  
  /* 
   * Compiles the code stored in the buffer and executes it.
   */
  void
  rho_repl::compile_line ()
  {
    this->mstore.remove ("#this#");
    
    std::istringstream ss (this->buf);
    std::stack<module_location> parse_work;
    if (!this->parse_module (ss, "", "", parse_work))
      return;
    
    while (!parse_work.empty ())
      {
        auto loc = parse_work.top ();
        parse_work.pop ();
        
        std::ifstream fs (loc.full_path);
        if (!this->parse_module (fs, loc.full_path, loc.dir_path, parse_work))
          return;
      }
    
    this->handle_imports_pre ();
    this->handle_usings ();
    this->handle_print ();
    
    // next compile all parsed AST modules
    std::vector<std::shared_ptr<rho::module>> mods;
    for (auto p : this->mstore.get_entries ())
      {
        auto& ent = p.second;
        
        if (ent.ident == "#this#")
          {
            if (this->run_num == 0)
              this->comp.alloc_globals (GLOBAL_PREALLOC_COUNT);
            else
              this->comp.dont_alloc_globals ();
            
            for (auto p : this->globs)
              this->comp.add_known_global (p.first, p.second);
            for (auto proto : this->protos)
              this->comp.add_known_fun_proto (proto);
          }
        else
          this->comp.alloc_globals ();
        
        std::shared_ptr<rho::module> m;
        try
          {
            this->comp.set_working_directory (ent.dir_path);
            m = this->comp.compile (ent.ast, ent.ident);
          }
        catch (const rho::compiler_error& ex)
          {
            _handle_compiler_errors (this->comp.get_errors ());
            return;
          }
        if (this->comp.get_errors ().count () > 0)
          {
            _handle_compiler_errors (this->comp.get_errors ());
            return;
          }
        
        mods.push_back (m);
      }
    
    // link compiled modules
    rho::linker lnk;
    lnk.set_next_mod_idx (this->next_mod);
    for (auto km : this->mods)
      lnk.add_known_module (km.first, km.second);
    for (auto p : this->atoms)
      lnk.add_atom (p.first, p.second);
    for (auto m : mods)
      lnk.add_module (m);
    auto prg = lnk.link ();
    this->progs.push_back (prg);
    
    this->handle_globals ();
    this->handle_imports (lnk);
    this->handle_atoms (lnk);
    
    // run
    ++ this->run_num;
    this->vm.run (*prg.get ());
    //auto res = this->vm.run (*prg.get ());
    //std::cout << " => " << rho_value_str (res, this->vm) << std::endl << std::endl;
    this->vm.pop_value ();
  }
  
  
  
  void
  rho_repl::handle_usings ()
  {
    auto p = this->mstore.retrieve ("#this#").ast;
    
    std::vector<std::string> new_u_ns;
    std::vector<std::pair<std::string, std::string>> new_u_aliases;
    
    for (auto s : p->get_stmts ())
      if (s->get_type () == AST_USING)
        {
          auto cn = std::static_pointer_cast<ast_using> (s);
          if (cn->get_alias ().empty ())
            new_u_ns.push_back (cn->get_namespace ());
          else
            new_u_aliases.push_back (std::make_pair (cn->get_alias (), cn->get_namespace ()));
        }
    
    // re-add using statements
    auto& stmts = p->get_stmts ();
    for (auto& ns : this->u_ns)
      stmts.insert (stmts.begin (), std::shared_ptr<ast_using> (
        new ast_using (ns)));
    for (auto& p : this->u_aliases)
      stmts.insert (stmts.begin (), std::shared_ptr<ast_using> (
        new ast_using (p.second, p.first)));
    
    this->u_ns.insert (this->u_ns.end (), new_u_ns.begin (), new_u_ns.end ());
    this->u_aliases.insert (this->u_aliases.end (), new_u_aliases.begin (), new_u_aliases.end ());
  }
  
  void
  rho_repl::handle_print ()
  {
    auto p = this->mstore.retrieve ("#this#").ast;
    auto& stmts = p->get_stmts ();
    
    if (stmts.empty ())
      return;
    
    auto last = stmts.back ();
    if (last->get_type () == AST_EXPR_STMT)
      {
        // wrap expression inside a print invocation
        auto expr = std::static_pointer_cast<ast_expr_stmt> (last)->get_expr ();
        
        if (expr->get_type () == AST_N)
          {
            // place print call inside the N expression.
            
            auto n = std::static_pointer_cast<ast_n> (expr);
            auto body = n->get_body ();
            
            auto print_call = std::shared_ptr<ast_fun_call> (new ast_fun_call (
              std::shared_ptr<ast_ident> (new ast_ident ("print"))));
              
            auto fmt_op = std::shared_ptr<ast_binop> (new ast_binop (AST_BINOP_MOD,
              std::shared_ptr<ast_string> (new ast_string (" => {*:E}\\n")), body));
            print_call->add_arg (fmt_op);
            
            auto nbody = std::make_shared<ast_expr_block> ();
            nbody->push_back (std::make_shared<ast_expr_stmt> (print_call));
            n->set_body (nbody);
          }
        else
          {
            stmts.pop_back ();
            
            auto print_call = std::shared_ptr<ast_fun_call> (new ast_fun_call (
              std::shared_ptr<ast_ident> (new ast_ident ("print"))));
              
            auto fmt_op = std::shared_ptr<ast_binop> (new ast_binop (AST_BINOP_MOD,
              std::shared_ptr<ast_string> (new ast_string (" => {*:E}\\n")), expr));
            print_call->add_arg (fmt_op);
            
            stmts.push_back (std::shared_ptr<ast_expr_stmt> (new ast_expr_stmt (print_call)));
          }
      }
  }
  
  void
  rho_repl::handle_globals ()
  {
    auto& ent = this->mstore.retrieve ("#this#");
    auto p = ent.ast;
    
    auto scope = ent.van->get_scope (p);
    for (auto g : scope->get_globals ())
      {
        this->globs[g.first] = g.second;
        //this->comp.add_known_global (g.first, g.second);
      }
    
    for (auto proto : ent.van->get_top_level_fun_protos ())
      //this->comp.add_known_fun_proto (proto);
      this->protos.insert (proto);
  }
  
  
  void
  rho_repl::handle_imports_pre ()
  {
    auto p = this->mstore.retrieve ("#this#").ast;
    auto& stmts = p->get_stmts ();
    
    // re-add previous imports
    for (auto p : this->imps)
      {
        auto& imp = p.second;
        stmts.insert (stmts.begin (), std::shared_ptr<ast_import> (
          new ast_import (imp.name)));
      }
  }
  
  void
  rho_repl::handle_imports (linker& lnk)
  {
    for (auto p : lnk.get_infos ())
      {
        auto& inf = p.second;
        auto& mident = inf.mod->get_name ();
        if (mident != "#this#")
          {
            this->mods[mident] = inf.idx;
          }
      }
    
    this->next_mod = lnk.get_next_mod_idx ();
  }
  
  
  void
  rho_repl::handle_atoms (linker& lnk)
  {
    auto p = this->mstore.retrieve ("#this#").ast;
    
    auto atoms = ast_tools::extract_atom_defs (p);
    for (auto& atom : atoms)
      this->comp.add_known_atom (atom);
    
    for (auto p : lnk.get_atoms ())
      this->atoms[p.first] = p.second;
  }
  
  
  
  /* 
   * Prints an intro and runs the REPL.
   */
  void
  rho_repl::run ()
  {
    this->print_intro ();
    
    char input[4096];
    for (;;)
      {
        std::cout << "; ";
        std::cin.getline (input, sizeof input);
        if (std::cin.eof ())
          {
            std::cout << std::endl;
            break;
          }
        
        this->buf.append (input);
        this->compile_line ();
        this->buf.clear ();
      }
  }
}

