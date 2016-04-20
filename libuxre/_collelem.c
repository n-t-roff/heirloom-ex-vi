/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)_collelem.c	1.4 (gritter) 10/18/03
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

#define CCE(p)	((const CollElem *)(p))
#define CCM(p)	((const CollMult *)(p))

LIBUXRE_STATIC const CollElem *
libuxre_collelem(struct lc_collate *col, CollElem *spare, wchar_t wc)
{
	const char *tbl;
	size_t hi, lo, cur;
	const CollMult *cmp;
	const CollElem *cep;
	long diff;
	int sz;

	/*
	* ELEM_ENCODED is returned when the collation is entirely
	* based on the encoded value of the character.
	*/
	if (col == 0 || col->flags & CHF_ENCODED
		|| (tbl = (const char *)col->maintbl) == 0)
	{
		return ELEM_ENCODED;
	}
	if ((wuchar_type)wc <= UCHAR_MAX)
	{
	indexed:;
		cep = CCE(&tbl[(wuchar_type)wc * col->elemsize]);
		if (cep->weight[0] == WGHT_SPECIAL)
			return ELEM_BADCHAR;
		return cep;
	}
	if (col->flags & CHF_INDEXED)
	{
		if ((wuchar_type)wc >= col->nmain)
			return ELEM_BADCHAR;
		goto indexed;
	}
	/*
	* Binary search for a match.  Could speed up the search if
	* some interpolation was used, but keep it simple for now.
	* Note that this is actually a table of CollMult's.
	*
	* To save space in the file, sequences of similar elements
	* are sometimes compressed into a single CollMult that
	* describes many entries.  This is denoted by a subnbeg
	* with the SUBN_SPECIAL bit set.  The rest of the bits give
	* the range covered by this entry.
	*/
	sz = col->elemsize + (sizeof(CollMult) - sizeof(CollElem));
	tbl += (1 + UCHAR_MAX) * col->elemsize;
	lo = 0;
	hi = col->nmain - UCHAR_MAX;
	while (lo < hi)
	{
		if ((cur = (hi + lo) >> 1) < lo)   /* hi+lo overflowed */
			cur |= ~(~(size_t)0 >> 1); /* lost high order bit */
		cmp = CCM(&tbl[cur * sz]);
		if ((diff = wc - cmp->ch) < 0)
			hi = cur;
		else if (cmp->elem.subnbeg & SUBN_SPECIAL)
		{
			if (diff > (long)(cmp->elem.subnbeg & ~SUBN_SPECIAL))
				lo = cur + 1;
			else /* create an entry from the sequence in spare */
			{
				spare->multbeg = cmp->elem.multbeg;
				spare->subnbeg = 0;
				spare->weight[0] = cmp->elem.weight[0] + diff;
				for (lo = 1; lo < col->nweight; lo++)
				{
					wuchar_type w;

					if ((w = cmp->elem.weight[lo])
						== WGHT_SPECIAL)
					{
						w = spare->weight[0];
					}
					spare->weight[lo] = w;
				}
				return spare;
			}
		}
		else if (diff == 0)
			return &cmp->elem;
		else
			lo = cur + 1;
	}
	return ELEM_BADCHAR;
}
