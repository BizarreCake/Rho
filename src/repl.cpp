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

#include "repl.hpp"
#include "common/errors.hpp"
#include "parser/parser.hpp"
#include "compiler/compiler.hpp"
#include "linker/linker.hpp"
#include "runtime/vm.hpp"
#include "runtime/value.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <cstring>


namespace rho {
  
  rho_repl::rho_repl ()
  {
    this->vm = new virtual_machine ();
    this->vm->init_repl ();
    
    this->indent = 0;
    this->next_var_index = 0;
    this->vars["that"] = this->next_var_index++;
  }

  rho_repl::~rho_repl ()
  {
    delete this->vm;
  }
  
  
  
  static void
  _print_logo ()
  {
    std::cout << "            ____/\\\\\\\\\\\\\\\\\\______/\\\\\\_______________________        \n";
    std::cout << "             __/\\\\\\///////\\\\\\___\\/\\\\\\_______________________       \n";
    std::cout << "              _\\/\\\\\\_____\\/\\\\\\___\\/\\\\\\_______________________      \n";
    std::cout << "               _\\/\\\\\\\\\\\\\\\\\\\\\\/____\\/\\\\\\_____________/\\\\\\\\\\____     \n";
    std::cout << "                _\\/\\\\\\//////\\\\\\____\\/\\\\\\\\\\\\\\\\\\\\____/\\\\\\///\\\\\\__    \n";
    std::cout << "                 _\\/\\\\\\____\\//\\\\\\___\\/\\\\\\/////\\\\\\__/\\\\\\__\\//\\\\\\_   \n";
    std::cout << "                  _\\/\\\\\\_____\\//\\\\\\__\\/\\\\\\___\\/\\\\\\_\\//\\\\\\__/\\\\\\__  \n";
    std::cout << "                   _\\/\\\\\\______\\//\\\\\\_\\/\\\\\\___\\/\\\\\\__\\///\\\\\\\\\\/___ \n";
    std::cout << "                    _\\///________\\///__\\///____\\///_____\\/////_____\n";
    std::cout << "\n";
    
    std::cout << "Copyright (C) 2014-2015  Jacob Zhitomirsky\n";
    std::cout << "This program comes with ABSOLUTELY NO WARRANTY;\n";
    std::cout << "This is free software, and you are welcome to redistribute it\n";
    std::cout << "under certain conditions;\n" << std::endl;
    
    std::cout << std::flush;
  }
  
  
  static void
  _print_errors (rho::error_tracker& errs)
  {
    for (auto& et : errs.get_entries ())
      {
        bool p = false;
        if (!et.file.empty ())
          { p = true; std::cout << et.file << ':'; }
        if (et.ln != -1)
          { p = true; std::cout << et.ln << ':'; }
        if (et.col != -1)
          { p = true; std::cout << et.col << ':'; }
        
        if (p)
          std::cout << ' ';
        switch (et.type)
          {
          case rho::ET_NOTE:     std::cout << "note: "; break;
          case rho::ET_WARNING:  std::cout << "warning: "; break;
          case rho::ET_ERROR:    std::cout << "error: "; break;
          }
        
        std::cout << et.what << std::endl;
      }
  }
  
  
  
  /* 
   * Applies numerous important changes to the specified AST tree that are
   * necessary to make the REPL work.  This includes turning the last
   * statement/expression into a let declaration in order to get its value.
   * 
   * Returns true if a value should be printed.
   */
  bool
  rho_repl::inspect_tree (ast_program *ast)
  {
    auto& stmts = ast->get_stmts ();
    if (stmts.empty ())
      return false;
    
    // make all top-most let declarations refer to the REPL variables.
    for (ast_stmt *stmt : stmts)
      if (stmt->get_type () == AST_LET)
        {
          ast_let *let = static_cast<ast_let *> (stmt);
          let->set_repl ();
          
          // add new variable binding
          auto itr = this->vars.find (let->get_name ());
          if (itr == this->vars.end ())
            this->vars[let->get_name ()] = this->next_var_index++;
        }
    
    ast_stmt *last = stmts.back ();
    if (last->get_type () == AST_EXPR_STMT)
      {
        ast->remove_stmt (stmts.size () - 1, false);
        ast_expr_stmt *estmt = static_cast<ast_expr_stmt *> (last);
        ast_expr *expr = estmt->get_expr ();
        estmt->release_expr ();
        delete estmt;
        
        ast_let *let = new ast_let ("that", expr);
        let->set_repl ();
        ast->add_stmt (let);
        return true;
      }
    
    return false;
  }
  
  
  /* 
   * Inserts bindings of REPL variables into the compiler.
   */
  void
  rho_repl::add_bindings (compiler& comp)
  {
    comp.set_repl ();
    for (auto p : this->vars)
      {
        comp.add_repl_var (p.first, p.second);
      }
  }
  
  
  /* 
   * Evaluates the specified string.
   */
  void
  rho_repl::eval (const std::string& str)
  {
    std::istringstream ss {str};
    
    rho::error_tracker errs;
    rho::parser parser { errs };
    rho::ast_program *prog = parser.parse (ss, "<stdin>");
    bool print = this->inspect_tree (prog);
    
    rho::compiler comp { errs };
    this->add_bindings (comp);
    rho::module *mod = comp.compile (prog);
    delete prog;
    
    if (errs.count () > 0)
      {
        _print_errors (errs);
        if (errs.count (rho::ET_ERROR) > 0)
          {
            return;
          }
      }
    
    rho::linker lnk { errs };
    lnk.add_primary_module (mod);
    rho::executable *exec = lnk.link ();
    {
      std::ofstream fs { "code.out", std::ios_base::out | std::ios_base::binary };
      fs.write ((char *)exec->find_section ("code")->data.get_data (),
                        exec->find_section ("code")->data.get_size ());
    }
    
    this->vm->run (*exec);
    if (print)
      {
        VALUE val = this->vm->get_repl_var (this->vars["that"]);
        std::cout << " => " << rho_value_str (RHO_VALUE(val)) << '\n' << std::endl;
      }
    
    delete exec;
  }
  
  
  
  static bool
  _is_complete (const char *str)
  {
    int len = std::strlen (str);
    for (int i = len - 1; i >= 0; --i)
      {
        if (str[i] == ' ')
          continue;
        else if (str[i] == ';')
          return true;
        else
          return false;
      }
    return false;
  }
  
  static int
  _compute_indent (const char *str)
  {
    int braces = 0, parens = 0;
    const char *ptr = str;
    while (*ptr)
      {
        switch (*ptr)
          {
          case '{':
            if (ptr[1] != ':')
              ++ braces;
            break;
          case '(':
            if (ptr[1] != ':')
              ++ parens;
            break;
          case '}': --braces; break;
          case ')': --parens; break;
          }
        
        ++ ptr;
      }
    
    return braces + parens;
  }
  
  /* 
   * Runs the REPL.
   */
  void
  rho_repl::run ()
  {
    _print_logo ();
    
    char prompt[0x100];
    
    char* input;
    for (;;)
      {
        if (this->buf.empty ())
          std::strcpy (prompt, "; ");
        else
          {
            *prompt = '\0';
            std::strcat (prompt, "  ");
            for (int i = 0; i < this->indent; ++i)
              std::strcat (prompt, "  ");
          }
        
        input = readline(prompt);
        if (!input)
          {
            // EOF
            std::cout << std::endl;
            break;
          }
        
        this->indent += _compute_indent (input);
        if (this->indent == 0 && _is_complete (input))
          {
            std::string s;
            for (auto& part : this->buf)
              s.append (part);
            this->buf.clear ();
            s.append (input);
            this->eval (s.c_str ());
          }
        else
          {
            this->buf.push_back (input);
          }
        
        add_history (input);
        free (input);
      }
  }
}

