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

#include "parse/lexer.hpp"
#include "parse/parser.hpp"
#include "parse/ast_printer.hpp"
#include "compiler/code_generator.hpp"
#include "compiler/compiler.hpp"
#include "linker/linker.hpp"
#include "runtime/vm.hpp"
#include "util/ast_tools.hpp"
#include "util/module_tools.hpp"
#include "runtime/repl.hpp"
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <stack>


namespace po = boost::program_options;


static void
_handle_compiler_errors (rho::error_list& errs)
{
  for (auto ent : errs.get_entries ())
    {
      std::cout << ent.path << ":";
      if (ent.ln != -1 && ent.col != -1)
        std::cout << ent.ln << ":" << ent.col << ":";
      std::cout << " ";
      switch (ent.type)
        {
        case rho::ERR_INFO: std::cout << "info: "; break;
        case rho::ERR_WARNING: std::cout << "warning: "; break;
        case rho::ERR_ERROR: std::cout << "error: "; break;
        case rho::ERR_FATAL: std::cout << "fatal: "; break;
        }
      
      std::cout << ent.msg << std::endl;
    }
}



static int
_run_repl ()
{
  rho::rho_repl repl;
  repl.run ();
  return 0;
}



int
main (int argc, char *argv[])
{
  po::options_description desc ("Options");
  desc.add_options ()
    ("help", "produce help message")
    ("input-file", po::value<std::vector<std::string>> (), "input file")
  ;
  
  po::positional_options_description p;
  p.add ("input-file", -1);
  
  po::variables_map vmap;
  try
    {
      po::store (po::command_line_parser (argc, argv).
                 options (desc).positional (p).run (), vmap);
      po::notify (vmap);
    }
  catch (const po::unknown_option& ex)
    {
      std::cout << "rho: fatal error: " << ex.what () << std::endl;
      return -1;
    }
  
  if (vmap.count ("help"))
    {
      std::cout << "Usage: rho [options] file ..." << std::endl;
      std::cout << desc << std::endl;
      return 1;
    }
  
  if (!vmap.count ("input-file"))
    return _run_repl ();
  
  auto input_files = vmap["input-file"].as<std::vector<std::string>> ();
  
  rho::module_store mstore;
  rho::compiler compiler (mstore);
  std::vector<std::shared_ptr<rho::module>> mods;
  
  // include directories
  compiler.add_include_dir (
    boost::filesystem::current_path ().generic_string ());
  
  // enqueue all input files to be parsed
  std::stack<rho::module_location> parse_work;
  for (const std::string& path : input_files)
    {
      if (!boost::filesystem::is_regular_file (path))
        {
          std::cout << "rho: fatal error: " << path << ": No such file." << std::endl;
          std::cout << "compilation terminated" << std::endl;
          return -1;
        }

      auto abs = boost::filesystem::canonical (path);
      parse_work.push ({
        .full_path = abs.generic_string (),
        .dir_path = abs.parent_path ().generic_string ()
      });
    }
  
  while (!parse_work.empty ())
    {
      auto loc = parse_work.top ();
      parse_work.pop ();
      
      //std::string rel_path = boost::filesystem::relative (loc.full_path);
      std::string rel_path = loc.full_path;
      
      std::ifstream strm (loc.full_path);
      
      // tokenize
      rho::lexer lex;
      std::unique_ptr<rho::lexer::token_stream> ts;
      try
        {
          ts.reset (new rho::lexer::token_stream (lex.tokenize (strm)));
        }
      catch (const rho::lexer_error& ex)
        {
          std::cout << rel_path << ":" << ex.get_line () << ":" << ex.get_column ()
                    << ": lexer error: " << ex.what () << std::endl;
          return -1;
        }
      
      // parse
      rho::parser parser;
      std::shared_ptr<rho::ast_program> p;
      try
        {
          p = parser.parse (*ts, rel_path);
        }
      catch (const rho::parse_error& ex)
        {
          std::cout << rel_path << ":" << ex.get_line () << ":" << ex.get_column ()
                    << ": parse error: " << ex.what () << std::endl;
          return -1;
        }
      
      std::string mname = rho::ast_tools::extract_module_name (p);
      if (mname.empty ())
        {
          std::cout << rel_path << ": fatal error: module name not specified" << std::endl;
          return -1;
        }
      
      auto ident = rho::get_module_identifier (loc.full_path);
      mstore.store (ident, p);
      auto& ent = mstore.retrieve (ident);
      ent.full_path = loc.full_path;
      ent.dir_path = loc.dir_path;
      ent.mname = mname;
      
      // enqueue all imported modules to be parsed.
      auto imps = rho::ast_tools::extract_imports (p);
      for (auto& m : imps)
        {
          try
            {
              auto loc = rho::find_module (m,
                compiler.get_include_dirs (), ent.dir_path);
              parse_work.push (loc);
            }
          catch (const rho::module_not_found_error&)
            {
              std::cout << rel_path << ": fatal error: unrecognized module '" << m << "'" << std::endl;
              return -1;
            }
        }
    }
  
  // next compile all parsed AST modules
  for (auto p : mstore.get_entries ())
    {
      auto& ent = p.second;
      
      std::shared_ptr<rho::module> m;
      try
        {
          compiler.set_working_directory (ent.dir_path);
          m = compiler.compile (ent.ast, ent.ident);
        }
      catch (const rho::compiler_error& ex)
        {
          _handle_compiler_errors (compiler.get_errors ());
          return -1;
        }
      if (compiler.get_errors ().count () > 0)
        {
          _handle_compiler_errors (compiler.get_errors ());
          return -1;
        }
      
      mods.push_back (m);
    }
  
  // link compiled modules
  rho::linker lnk;
  for (auto m : mods)
    lnk.add_module (m);
  auto prg = lnk.link ();
  {
    std::ofstream fs ("a.bin");
    fs.write ((char *)prg->get_code (), prg->get_code_size ());
  }
  
  // run
  rho::virtual_machine vm;
  auto res = vm.run (*prg.get ());
  std::cout << rho_value_str (res) << std::endl;
  
  return 0;
}

