/*
  (c) copyright 1988 by the Vrije Universiteit, Amsterdam, The Netherlands.
  See the copyright notice in the ACK home directory, in the file "Copyright".
*/

/* $Header: zrf_ext.c,v 1.4 88/08/04 11:16:20 ceriel Exp $ */

/*
	ZERO and return EXTEND FORMAT FLOAT
*/

#include "FP_types.h"

void
zrf_ext(e)
EXTEND	*e;
{
	e->m1 = 0;
	e->m2 = 0;
	e->exp = 0;
	e->sign = 0;
}
