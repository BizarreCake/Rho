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

#include "util/ast_tools.hpp"


namespace rho {
  
  namespace ast_tools {
    
    static void _traverse_dfs_node (std::shared_ptr<ast_node> node,
                                    const traverse_fn& fn);
    
    
    void
    _traverse_dfs_node (std::shared_ptr<ast_node> node, const traverse_fn& fn)
    {
      auto res = fn (node);
      if (res == TR_SKIP)
        return;
      
      switch (node->get_type ())
        {
        case AST_INTEGER:
        case AST_IDENT:
        case AST_NIL:
        case AST_MODULE:
        case AST_IMPORT:
        case AST_EXPORT:
        case AST_BOOL:
        case AST_ATOM:
        case AST_ATOM_DEF:
        case AST_EMPTY_STMT:
        case AST_STRING:
        case AST_USING:
          break;
        
        case AST_EXPR_STMT:
          _traverse_dfs_node (
            std::static_pointer_cast<ast_expr_stmt> (node)->get_expr (), fn);
          break;
        
        case AST_EXPR_BLOCK:
          {
            auto cn = std::static_pointer_cast<ast_expr_block> (node);
            for (auto s : cn->get_stmts ())
              _traverse_dfs_node (s, fn);
          }
          break;
        
        case AST_STMT_BLOCK:
          {
            auto cn = std::static_pointer_cast<ast_stmt_block> (node);
            for (auto s : cn->get_stmts ())
              _traverse_dfs_node (s, fn);
          }
          break;
        
        case AST_PROGRAM:
          {
            auto cn = std::static_pointer_cast<ast_program> (node);
            for (auto s : cn->get_stmts ())
              _traverse_dfs_node (s, fn);
          }
          break;
        
        case AST_UNOP:
          {
            auto cn = std::static_pointer_cast<ast_unop> (node);
            _traverse_dfs_node (cn->get_opr (), fn);
          }
          break;
        
        case AST_BINOP:
          {
            auto cn = std::static_pointer_cast<ast_binop> (node);
            _traverse_dfs_node (cn->get_lhs (), fn);
            _traverse_dfs_node (cn->get_rhs (), fn);
          }
          break;
        
        case AST_VAR_DEF:
          {
            auto cn = std::static_pointer_cast<ast_var_def> (node);
            _traverse_dfs_node (cn->get_val (), fn);
          }
          break;
        
        case AST_FUN:
          {
            auto cn = std::static_pointer_cast<ast_fun> (node);
            _traverse_dfs_node (cn->get_body (), fn);
          }
          break;
        
        case AST_FUN_CALL:
          {
            auto cn = std::static_pointer_cast<ast_fun_call> (node);
            _traverse_dfs_node (cn->get_fun (), fn);
            for (auto a : cn->get_args ())
              _traverse_dfs_node (a, fn);
          }
          break;
        
        case AST_IF:
          {
            auto cn = std::static_pointer_cast<ast_if> (node);
            _traverse_dfs_node (cn->get_test (), fn);
            _traverse_dfs_node (cn->get_consequent (), fn);
            _traverse_dfs_node (cn->get_antecedent (), fn);
          }
          break;
        
        case AST_CONS:
          {
            auto cn = std::static_pointer_cast<ast_cons> (node);
            _traverse_dfs_node (cn->get_fst (), fn);
            _traverse_dfs_node (cn->get_snd (), fn);
          }
          break;
        
        case AST_LIST:
          {
            auto cn = std::static_pointer_cast<ast_list> (node);
            for (auto e : cn->get_elems ())
              _traverse_dfs_node (e, fn);
          }
          break;
        
        case AST_MATCH:
          {
            auto cn = std::static_pointer_cast<ast_match> (node);
            _traverse_dfs_node (cn->get_expr (), fn);
            for (auto& c : cn->get_cases ())
              _traverse_dfs_node (c.body, fn);
            if (cn->get_else_body ())
              _traverse_dfs_node (cn->get_else_body (), fn);
          }
          break;
        
        case AST_RET:
          {
            auto cn = std::static_pointer_cast<ast_ret> (node);
            _traverse_dfs_node (cn->get_expr (), fn);
          }
          break;
        
        case AST_VECTOR:
          {
            auto cn = std::static_pointer_cast<ast_vector> (node);
            for (auto e : cn->get_exprs ())
              _traverse_dfs_node (e, fn);
          }
          break;
        
        case AST_SUBSCRIPT:
          {
            auto cn = std::static_pointer_cast<ast_subscript> (node);
            _traverse_dfs_node (cn->get_expr (), fn);
            _traverse_dfs_node (cn->get_index (), fn);
          }
          break;
        
        case AST_NAMESPACE:
          {
            auto cn = std::static_pointer_cast<ast_namespace> (node);
            auto body = cn->get_body ();
            for (auto s : body->get_stmts ())
              _traverse_dfs_node (s, fn);
          }
          break;
        
        case AST_LET:
          {
            auto cn = std::static_pointer_cast<ast_let> (node);
            for (auto& p : cn->get_defs ())
              _traverse_dfs_node (p.second, fn);
            _traverse_dfs_node (cn->get_body (), fn);
          }
          break;
        }
    }
    
    
    /* 
     * Performs a depth-first traversal on the specified AST node.
     */
    void
    traverse_dfs (std::shared_ptr<ast_node> node, traverse_fn&& fn)
    {
      _traverse_dfs_node (node, fn);
    }
    
  
  
//------------------------------------------------------------------------------
    
    /*
     * Extracts the name of the module from the specified AST program.
     */
    std::string
    extract_module_name (std::shared_ptr<ast_program> node)
    {
      for (auto s : node->get_stmts ())
        {
          if (s->get_type () == AST_MODULE)
            return std::static_pointer_cast<ast_module> (s)->get_name ();
        }
      
      return "";
    }
    
    /* 
     * Extracts the names of all imported modules in the specified AST program.
     */
    std::vector<std::string>
    extract_imports (std::shared_ptr<ast_program> node)
    {
      std::vector<std::string> imps;
      
      for (auto s : node->get_stmts ())
        {
          if (s->get_type () == AST_IMPORT)
            imps.push_back (std::static_pointer_cast<ast_import> (s)->get_name ());
        }
      
      return imps;
    }
    
    /* 
     * Extracts the names of all exports in the specified AST program, in the
     * order they appear in the program.
     */
    std::vector<std::string>
    extract_exports (std::shared_ptr<ast_program> node)
    {
      std::vector<std::string> exps;
      
      for (auto s : node->get_stmts ())
        {
          if (s->get_type () == AST_EXPORT)
            {
              auto cn = std::static_pointer_cast<ast_export> (s);
              for (auto& n : cn->get_names ())
                exps.push_back (n);
            }
        }
      
      return exps;
    }
    
    
    
    
    static void
    _global_defs_search_namespace (std::shared_ptr<ast_namespace> node,
                                   std::vector<std::string>& vars,
                                   std::string curr_ns)
    {
      if (curr_ns.empty ())
        curr_ns = node->get_name ();
      else
        curr_ns += ":" + node->get_name ();
      
      for (auto s : node->get_body ()->get_stmts ())
        {
          if (s->get_type () == AST_VAR_DEF)
            vars.push_back (
              curr_ns + ":"
                + std::static_pointer_cast<ast_var_def> (s)->get_var ()->get_value ());
          else if (s->get_type () == AST_NAMESPACE)
            _global_defs_search_namespace (std::static_pointer_cast<ast_namespace> (s), vars, curr_ns);
        }
    }
    
    /* 
     * Extracts top-level variable definitions from the specified AST program.
     */
    std::vector<std::string>
    extract_global_defs (std::shared_ptr<ast_program> node)
    {
      std::vector<std::string> vars;
      for (auto s : node->get_stmts ())
        {
          if (s->get_type () == AST_VAR_DEF)
            vars.push_back (
              std::static_pointer_cast<ast_var_def> (s)->get_var ()->get_value ());
          else if (s->get_type () == AST_NAMESPACE)
            _global_defs_search_namespace (std::static_pointer_cast<ast_namespace> (s), vars, "");
        }
      
      return vars;
    }
    
    
    
    static void
    _atom_defs_search_namespace (std::shared_ptr<ast_namespace> node,
                                 std::vector<std::string>& atoms,
                                 std::string curr_ns)
    {
      if (curr_ns.empty ())
        curr_ns = node->get_name ();
      else
        curr_ns += ":" + node->get_name ();
      
      for (auto s : node->get_body ()->get_stmts ())
        {
          if (s->get_type () == AST_ATOM_DEF)
            atoms.push_back (
              curr_ns + ":"
                + std::static_pointer_cast<ast_atom_def> (s)->get_name ());
          else if (s->get_type () == AST_NAMESPACE)
            _atom_defs_search_namespace (std::static_pointer_cast<ast_namespace> (s), atoms, curr_ns);
        }
    }
    
    /* 
     * Extracts top-level variable definitions from the specified AST program.
     */
    std::vector<std::string>
    extract_atom_defs (std::shared_ptr<ast_program> node)
    {
      std::vector<std::string> atoms;
      for (auto s : node->get_stmts ())
        {
          if (s->get_type () == AST_ATOM_DEF)
            atoms.push_back (
              std::static_pointer_cast<ast_atom_def> (s)->get_name ());
          else if (s->get_type () == AST_NAMESPACE)
            _atom_defs_search_namespace (std::static_pointer_cast<ast_namespace> (s), atoms, "");
        }
      
      return atoms;
    }
  }
}

