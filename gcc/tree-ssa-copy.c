/* Const/copy propagation and SSA_NAME replacement support routines.
   Copyright (C) 2004 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "flags.h"
#include "rtl.h"
#include "tm_p.h"
#include "ggc.h"
#include "basic-block.h"
#include "output.h"
#include "errors.h"
#include "expr.h"
#include "function.h"
#include "diagnostic.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-flow.h"
#include "tree-pass.h"
#include "langhooks.h"

/* This file provides a handful of interfaces for performing const/copy
   propagation and simple expression replacement which keep variable
   annotations up-to-date.

   We require that for any copy operation where the RHS and LHS have
   a non-null memory tag that the memory tag be the same.   It is OK
   for one or both of the memory tags to be NULL.

   We also require tracking if a variable is dereferenced in a load or
   store operation.

   We enforce these requirements by having all copy propagation and
   replacements of one SSA_NAME with a different SSA_NAME to use the
   APIs defined in this file.  */


/* Given two SSA_NAMEs, replace the annotations for the one referred to by OP 
   with VAR's annmoptations.

   If OP is a pointer, copy the memory tag used originally by OP into
   VAR.  This is needed in cases where VAR had never been dereferenced in the
   program.

   If FOR_PROPAGATION is true, then perform additional checks to ensure
   that const/copy propagation of var for OP is valid.  */
   
static void
replace_ssa_names_ann (tree op,
		   tree var,
		   bool for_propagation ATTRIBUTE_UNUSED)
{
#if defined ENABLE_CHECKING
  if (for_propagation && !may_propagate_copy (op, var))
    abort ();
#endif

  /* If VAR doesn't have a memory tag, copy the one from the original
     operand.  Also copy the dereferenced flags.  */
  if (POINTER_TYPE_P (TREE_TYPE (op)))
    {
      var_ann_t new_ann = var_ann (SSA_NAME_VAR (var));
      var_ann_t orig_ann = var_ann (SSA_NAME_VAR (op));

      if (new_ann->type_mem_tag == NULL_TREE)
	new_ann->type_mem_tag = orig_ann->type_mem_tag;
      else if (orig_ann->type_mem_tag == NULL_TREE)
	orig_ann->type_mem_tag = new_ann->type_mem_tag;
      else if (new_ann->type_mem_tag != orig_ann->type_mem_tag)
	abort ();
    }

}   


/* Common code for propagate_value and replace_exp.

   Replace use operand OP_P with VAL.  FOR_PROPAGATION indicates if the 
   replacement is done to propagate a value or not.  */

static void
replace_exp_1 (use_operand_p op_p, tree val, bool for_propagation)
{
  if (TREE_CODE (val) == SSA_NAME)
    {
      if (TREE_CODE (USE_FROM_PTR (op_p)) == SSA_NAME)
	replace_ssa_names_ann (USE_FROM_PTR (op_p), val, for_propagation);
      SET_USE (op_p, val);
    }
  else
    SET_USE (op_p, lhd_unsave_expr_now (val));
}


/* Propagate the value VAL (assumed to be a constant or another SSA_NAME)
   into the operand pointed by OP_P.

   Use this version for const/copy propagation as it will perform additional
   checks to ensure validity of the const/copy propagation.  */

void
propagate_value (use_operand_p op_p, tree val)
{
  replace_exp_1 (op_p, val, true);
}


/* Propagate the value VAL (assumed to be a constant or another SSA_NAME)
   into the tree pointed by OP_P.

   Use this version for const/copy propagation when SSA operands are not 
   available.  It will perform the additional checks to ensure validity of 
   the const/copy propagation, but will not update any operand information.
   Be sure to mark the stmt as modified.  */

void
propagate_tree_value (tree *op_p, tree val)
{
  if (TREE_CODE (val) == SSA_NAME)
    {
      if (TREE_CODE (*op_p) == SSA_NAME)
	replace_ssa_names_ann (*op_p, val, true);
      *op_p = val;
    }
  else
    *op_p = lhd_unsave_expr_now (val);
}


/* Replace *OP_P with value VAL (assumed to be a constant or another SSA_NAME).

   Use this version when not const/copy propagating values.  For example,
   PRE uses this version when building expressions as they would appear
   in specific blocks taking into account actions of PHI nodes.  */

void
replace_exp (use_operand_p op_p, tree val)
{
  replace_exp_1 (op_p, val, false);
}
