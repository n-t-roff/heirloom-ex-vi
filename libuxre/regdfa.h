/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regdfa.h	1.3 (gritter) 9/22/03
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

/*
* Deterministic Finite Automata.
*/

#ifndef	LIBUXRE_REGDFA_H
#define	LIBUXRE_REGDFA_H

#include <re.h>

typedef struct
{
	Bracket	*bkt;	/* extra info for ROP_BKT */
	size_t	nset;	/* number of items in the follow set */
	size_t	seti;	/* index into the follow set strip */
	w_type	op;	/* the leaf match operation */
} Posn;

#define CACHESZ	32	/* max. states to remember (must fit in uchar) */
#define NCHAR	(1 << CHAR_BIT)

struct re_dfa_ /*Dfa*/
{
	unsigned char	*posset;	/* signatures built here */
	size_t		*posfoll;	/* follow strip for posn[] */
	size_t		*sigfoll;	/* follow strip for sigi[] */
	size_t		*cursig;	/* current state's signature */
	Posn		*posn;		/* important positions */
	size_t		nposn;		/* length of posn,cursig,posset */
	size_t		used;		/* used portion of follow strip */
	size_t		avail;		/* unused part of follow strip */
	size_t		nset;		/* # items nonzero in posset[] */
	size_t		nsig[CACHESZ];	/* number of items in signature */
	size_t		sigi[CACHESZ];	/* index into sigfoll[] */
	unsigned char	acc[CACHESZ];	/* nonzero for accepting states */
	unsigned char	leftmost;	/* leftmost() start, not BOL */
	unsigned char	leftbol;	/* leftmost() start, w/BOL */
	unsigned char	anybol;		/* any match start, w/BOL */
	unsigned char	nfix;		/* number of invariant states */
	unsigned char	top;		/* next state index available */
	unsigned char	flags;		/* interesting flags */
	unsigned char	trans[CACHESZ][NCHAR];	/* goto table */
};

extern int	 regtrans(Dfa *, int, w_type, int);

#endif	/* !LIBUXRE_REGDFA_H */
