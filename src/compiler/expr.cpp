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
#include <stdexcept>
#include <sstream>


namespace rho {
  
  void
  compiler::compile_nil (ast_nil *ast)
  {
    this->cgen->emit_push_nil ();
  }
  
  
  
  void
  compiler::compile_integer (ast_integer *ast)
  {
    const std::string& num = ast->get_value ();
    if (num.length () <= 10)
      {
        std::istringstream ss { num };
        long long n;
        ss >> n;
        if (n >= -2147483648 && n < 2147483647)
          {
            this->cgen->emit_push_int_32 (n);
            return;
          }
      }
    
    this->cgen->emit_push_int (num);
  }
  
  
  
  void
  compiler::compile_real (ast_real *ast)
  {
    const std::string& num = ast->get_value ();
    this->cgen->emit_push_real (num);
  }
  
  
  
  void
  compiler::compile_ident (ast_ident *ast)
  {
    if (ast->get_name () == "$$")
      {
        // $$ - this function
        
        // make sure we're inside a function
        bool in_func = false;
        frame *frm = &this->top_frame ();
        while (frm)
          {
            if (frm->get_type () == FT_FUNCTION)
              {
                in_func = true;
                break;
              }
            frm = frm->get_parent ();
          }
        
        if (!in_func)
          {
            this->errs.add (ET_ERROR, "use of '$$' variable outside function");
            return;
          }
        
        this->cgen->emit_this_func ();
        return;
      }
    
    frame& frm = this->top_frame ();
    variable *var = frm.get_local (ast->get_name ());
    if (!var)
      {
        // function argument?
        if ((var = frm.get_arg (ast->get_name ())))
          {
            this->cgen->emit_load_arg (var->index);
            return;
          }
        
        // REPL variable?
        if (this->in_repl)
          {
            frame *frm = this->frms.front ();
            var = frm->get_local (ast->get_name ());
            if (var)
              {         
                this->cgen->emit_load_repl_var (var->index);
                return;
              }
          }
        
        this->errs.add (ET_ERROR,
          "use of undefined variable '" + ast->get_name () + "'");
        return;
      }
    
    this->cgen->emit_load_loc (var->index);
  }
  
  
  
  void
  compiler::compile_sym (ast_sym *ast)
  {
    this->cgen->emit_push_sym (ast->get_name ());
  }
  
  
  
  void
  compiler::compile_binop (ast_binop *ast)
  {
    if (ast->get_op () == AST_BINOP_ASSIGN)
      {
        this->compile_assign (ast);
        return;
      }
    
    this->push_expr_frame (false);
    this->compile_expr (ast->get_lhs ());
    this->compile_expr (ast->get_rhs ());
    this->pop_expr_frame ();
    
    switch (ast->get_op ())
      {
      case AST_BINOP_ADD:
        this->cgen->emit_add ();
        break;
      
      case AST_BINOP_SUB:
        this->cgen->emit_sub ();
        break;
      
      case AST_BINOP_MUL:
        this->cgen->emit_mul ();
        break;
      
      case AST_BINOP_DIV:
        this->cgen->emit_div ();
        break;
      
      case AST_BINOP_IDIV:
        this->cgen->emit_idiv ();
        break;
      
      case AST_BINOP_MOD:
        this->cgen->emit_mod ();
        break;
      
      case AST_BINOP_POW:
        this->cgen->emit_pow ();
        break;
      
      case AST_BINOP_EQ:
        this->cgen->emit_cmp_eq ();
        break;
      
      case AST_BINOP_NEQ:
        this->cgen->emit_cmp_ne ();
        break;
      
      case AST_BINOP_LT:
        this->cgen->emit_cmp_lt ();
        break;
      
      case AST_BINOP_LTE:
        this->cgen->emit_cmp_le ();
        break;
      
      case AST_BINOP_GT:
        this->cgen->emit_cmp_gt ();
        break;
      
      case AST_BINOP_GTE:
        this->cgen->emit_cmp_ge ();
        break;
      
      default:
        throw std::runtime_error ("invalid binop op");
      }
  }
  
  
  
  void
  compiler::compile_unop (ast_unop *ast)
  {
    this->push_expr_frame (false);
    this->compile_expr (ast->get_expr ());
    this->pop_expr_frame ();
    
    switch (ast->get_op ())
      {
      case AST_UNOP_NEGATE:
        this->cgen->emit_negate ();
        break;
        
      case AST_UNOP_FACTORIAL:
        this->cgen->emit_factorial ();
        break;
      
      default:
        throw std::runtime_error ("invalid unop op");
      }
  }
  
  
  
  void
  compiler::compile_if (ast_if *ast)
  {
    int lbl_true = this->cgen->create_label ();
    int lbl_false = this->cgen->create_label ();
    int lbl_end = this->cgen->create_label ();
    
    this->compile_expr (ast->get_test ());
    this->cgen->emit_jf (lbl_false);
    
    // consequtive
    this->cgen->mark_label (lbl_true);
    this->compile_expr (ast->get_conseq ());
    this->cgen->emit_jmp (lbl_end);
    
    // alternative
    this->cgen->mark_label (lbl_false);
    if (ast->get_alt ())
      this->compile_expr (ast->get_alt ());
    else
      this->cgen->emit_push_nil ();
    
    this->cgen->mark_label (lbl_end);
  }
  
  
  
  void
  compiler::compile_n (ast_n *ast)
  {
    this->push_expr_frame (false);
    this->cgen->emit_push_microframe ();
    
    this->compile_expr (ast->get_prec ());
    this->cgen->emit_set_prec ();
    
    this->compile_block (ast->get_body (), true);
    
    this->cgen->emit_pop_microframe ();
    this->pop_expr_frame ();
  }
  
  
  
  void
  compiler::compile_boundless_sum (ast_sum *ast)
  {
    this->push_expr_frame (false);
    this->compile_expr (ast->get_start ());
    
    this->push_frame (FT_BLOCK);
    frame& frm = this->top_frame ();
    
    // set up loop variable
    const std::string& var_name = ast->get_var ()->get_name ();
    frm.add_local (var_name);
    int loop_var = frm.get_local (var_name)->index;
    this->cgen->emit_store_loc (loop_var);
    
    // set up sum
    int sum_var = frm.alloc_local ();
    this->cgen->emit_push_int_32 (0);
    this->cgen->emit_store_loc (sum_var);
    
    int lbl_loop = this->cgen->create_label ();
    int lbl_end = this->cgen->create_label ();
    this->cgen->mark_label (lbl_loop);
    
    // body
    this->cgen->emit_load_loc (sum_var);
    this->cgen->emit_dup ();
    this->compile_block (ast->get_body (), true, false);
    this->cgen->emit_add ();
    this->cgen->emit_store_loc (sum_var);
    this->cgen->emit_load_loc (sum_var);
    this->cgen->emit_cmp_cvg ();
    this->cgen->emit_jt (lbl_end);
    
    // increment loop variable and loop over
    this->cgen->emit_load_loc (loop_var);
    this->cgen->emit_push_int_32 (1);
    this->cgen->emit_add ();
    this->cgen->emit_store_loc (loop_var);
    this->cgen->emit_jmp (lbl_loop);
    
    this->cgen->mark_label (lbl_end);
    this->cgen->emit_load_loc (sum_var);
    
    this->pop_frame ();
    this->pop_expr_frame ();
  }
  
  void
  compiler::compile_sum (ast_sum *ast)
  {
    if (!ast->get_end ())
      {
        this->compile_boundless_sum (ast);
        return;
      }
    
    this->push_expr_frame (false);
    this->compile_expr (ast->get_end ());
    this->compile_expr (ast->get_start ());
    
    this->push_frame (FT_BLOCK);
    frame& frm = this->top_frame ();
    
    // set up loop variable
    const std::string& var_name = ast->get_var ()->get_name ();
    frm.add_local (var_name);
    int loop_var = frm.get_local (var_name)->index;
    this->cgen->emit_store_loc (loop_var);
    
    // store range end
    int end_var = frm.alloc_local ();
    this->cgen->emit_store_loc (end_var);
    
    // set up sum
    this->cgen->emit_push_int_32 (0);
    
    int lbl_loop = this->cgen->create_label ();
    int lbl_end = this->cgen->create_label ();
    this->cgen->mark_label (lbl_loop);
    
    // check if loop variable is inside range
    this->cgen->emit_load_loc (loop_var);
    this->cgen->emit_load_loc (end_var);
    this->cgen->emit_cmp_le ();
    this->cgen->emit_jf (lbl_end);
    
    // body
    this->compile_block (ast->get_body (), true, false);
    this->cgen->emit_add ();
    
    // increment loop variable and loop over
    this->cgen->emit_load_loc (loop_var);
    this->cgen->emit_push_int_32 (1);
    this->cgen->emit_add ();
    this->cgen->emit_store_loc (loop_var);
    this->cgen->emit_jmp (lbl_loop);
    
    this->cgen->mark_label (lbl_end);
    
    this->pop_frame ();
    this->pop_expr_frame ();
  }
  
  
  
  void
  compiler::compile_product (ast_product *ast)
  {
    this->push_expr_frame (false);
    this->compile_expr (ast->get_end ());
    this->compile_expr (ast->get_start ());
    
    this->push_frame (FT_BLOCK);
    frame& frm = this->top_frame ();
    
    // set up loop variable
    const std::string& var_name = ast->get_var ()->get_name ();
    frm.add_local (var_name);
    int loop_var = frm.get_local (var_name)->index;
    this->cgen->emit_store_loc (loop_var);
    
    // store range end
    int end_var = frm.alloc_local ();
    this->cgen->emit_store_loc (end_var);
    
    // set up product
    this->cgen->emit_push_int_32 (1);
    
    int lbl_loop = this->cgen->create_label ();
    int lbl_end = this->cgen->create_label ();
    this->cgen->mark_label (lbl_loop);
    
    // check if loop variable is inside range
    this->cgen->emit_load_loc (loop_var);
    this->cgen->emit_load_loc (end_var);
    this->cgen->emit_cmp_le ();
    this->cgen->emit_jf (lbl_end);
    
    // body
    this->compile_block (ast->get_body (), true, false);
    this->cgen->emit_mul ();
    
    // increment loop variable and loop over
    this->cgen->emit_load_loc (loop_var);
    this->cgen->emit_push_int_32 (1);
    this->cgen->emit_add ();
    this->cgen->emit_store_loc (loop_var);
    this->cgen->emit_jmp (lbl_loop);
    
    this->cgen->mark_label (lbl_end);
    
    this->pop_frame ();
    this->pop_expr_frame ();
  }
  
  
  
  void
  compiler::compile_list (ast_list *ast)
  {
    auto& elems = ast->get_elems ();
    if (elems.empty ())
      {
        this->cgen->emit_push_empty_cons ();
        return;
      }
      
    this->push_expr_frame (false);
    for (size_t i = 0; i < elems.size (); ++i)
      this->compile_expr (elems[i]);
    this->pop_expr_frame ();
    this->cgen->emit_push_empty_cons ();
    
    for (size_t i = 0; i < elems.size (); ++i)
      this->cgen->emit_cons ();
  }
  
  
  
  void
  compiler::compile_subst (ast_subst *ast)
  {
    this->push_expr_frame (false);
    this->compile_expr (ast->get_expr ());
    this->compile_expr (ast->get_sym ());
    this->compile_expr (ast->get_val ());
    this->pop_expr_frame ();
    
    this->cgen->emit_subst ();
  }
  
  
  
  void
  compiler::compile_expr (ast_expr *ast)
  {
    switch (ast->get_type ())
      {
      case AST_NIL:
        this->compile_nil (static_cast<ast_nil *> (ast));
        break;
      
      case AST_INTEGER:
        this->compile_integer (static_cast<ast_integer *> (ast));
        break;
      
      case AST_REAL:
        this->compile_real (static_cast<ast_real *> (ast));
        break;
      
      case AST_IDENT:
        this->compile_ident (static_cast<ast_ident *> (ast));
        break;
      
      case AST_SYM:
        this->compile_sym (static_cast<ast_sym *> (ast));
        break;
      
      case AST_BINOP:
        this->compile_binop (static_cast<ast_binop *> (ast));
        break;
      
      case AST_UNOP:
        this->compile_unop (static_cast<ast_unop *> (ast));
        break;
      
      case AST_FUNCTION:
        this->compile_function (static_cast<ast_function *> (ast));
        break;
      
      case AST_CALL:
        this->compile_call (static_cast<ast_call *> (ast));
        break;
      
      case AST_IF:
        this->compile_if (static_cast<ast_if *> (ast));
        break;
      
      case AST_N:
        this->compile_n (static_cast<ast_n *> (ast));
        break;
      
      case AST_SUM:
        this->compile_sum (static_cast<ast_sum *> (ast));
        break;
      
      case AST_PRODUCT:
        this->compile_product (static_cast<ast_product *> (ast));
        break;
      
      case AST_LIST:
        this->compile_list (static_cast<ast_list *> (ast));
        break;
      
      case AST_SUBST:
        this->compile_subst (static_cast<ast_subst *> (ast));
        break;
      
      default:
        throw std::runtime_error ("invalid expression type");
      }
  }
}

