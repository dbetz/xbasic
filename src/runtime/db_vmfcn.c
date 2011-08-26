/* db_vmfcn.c - intrinsic functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "db_vm.h"

static void fcn_abs(Interpreter *i);
static void fcn_rnd(Interpreter *i);

/* this table must be in the same order as the FN_xxx macros */
IntrinsicFcn * FLASH_SPACE Intrinsics[] = {
    fcn_abs,
	fcn_rnd
};

int IntrinsicCount = sizeof(Intrinsics) / sizeof(IntrinsicFcn *);

/* fcn_abs - ABS(n): return the absolute value of a number */
static void fcn_abs(Interpreter *i)
{
    i->tos = abs(Pop(i));
}

/* fcn_rnd - RND(n): return a random number between 0 and n-1 */
static void fcn_rnd(Interpreter *i)
{
    i->tos = rand() % Pop(i);
}
