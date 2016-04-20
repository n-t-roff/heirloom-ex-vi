/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)_collmult.c	1.4 (gritter) 9/22/03
 */
/*  UNIX(R) Regular Expresssion Library
 *
 *  Note: Code is released under the GNU LGPL
 *
 *  Copyright (C) 2001 Caldera International, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to:
 *        Free Software Foundation, Inc.
 *        59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*	#include "synonyms.h"	*/
#include "colldata.h"
#include <stddef.h>

#define CCM(p)	((const CollMult *)(p))

LIBUXRE_STATIC const CollElem *
libuxre_collmult(struct lc_collate *col, const CollElem *cep, wchar_t wc)
{
	const char *tbl;
	size_t sz;
	w_type ch;

	if (col == 0 || cep->multbeg == 0
		|| (tbl = (const char *)col->multtbl) == 0)
	{
		return ELEM_BADCHAR;
	}
	sz = col->elemsize + (sizeof(CollMult) - sizeof(CollElem));
	tbl += sz * cep->multbeg;
	while ((ch = CCM(tbl)->ch) != wc)
	{
		if (ch == 0)
			return ELEM_BADCHAR;	/* end of list */
		tbl += sz;
	}
	return &CCM(tbl)->elem;
}
