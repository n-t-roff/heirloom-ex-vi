/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)stubs.c	1.27 (gritter) 6/26/05
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
/* stubbed-out routines needed to complete the RE libc code */

#include "colldata.h"

struct lc_collate *
libuxre_lc_collate(struct lc_collate *cp)
{
	static struct lc_collate curinfo; /* means CHF_ENCODED */

	(void)cp;
	return &curinfo;
}

#include "wcharm.h"

LIBUXRE_STATIC int
libuxre_mb2wc(w_type *wt, const unsigned char *s)
{
	wchar_t wc;
	int len;

	if ((len = mbtowc(&wc, (const char *)&s[-1], MB_LEN_MAX)) > 0)
		*wt = wc;
	else if (len == 0)
		*wt = '\0';
	else /*if (len < 0)*/
		*wt = (w_type)WEOF;
	return len > 0 ? len - 1 : len;
}

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)libuxre.sl	1.27 (gritter) 6/26/05";
/* SLIST */
/*
_collelem.c: * Sccsid @(#)_collelem.c	1.4 (gritter) 10/18/03
_collmult.c: * Sccsid @(#)_collmult.c	1.4 (gritter) 9/22/03
bracket.c: * Sccsid @(#)bracket.c	1.14 (gritter) 10/18/03
colldata.h: * Sccsid @(#)colldata.h	1.5 (gritter) 5/1/04
onefile.c: * Sccsid @(#)onefile.c	1.1 (gritter) 9/22/03
re.h: * Sccsid @(#)re.h	1.15 (gritter) 2/6/05
regcomp.c: * Sccsid @(#)regcomp.c	1.6 (gritter) 9/22/03
regdfa.c: * Sccsid @(#)regdfa.c	1.9 (gritter) 9/22/03
regdfa.h: * Sccsid @(#)regdfa.h	1.3 (gritter) 9/22/03
regerror.c: * Sccsid @(#)regerror.c	1.4 (gritter) 3/29/03
regex.h: * Sccsid @(#)regex.h	1.13 (gritter) 2/6/05
regexec.c: * Sccsid @(#)regexec.c	1.7 (gritter) 2/6/05
regfree.c: * Sccsid @(#)regfree.c	1.3 (gritter) 9/22/03
regnfa.c: * Sccsid @(#)regnfa.c	1.8 (gritter) 2/6/05
regparse.c: * Sccsid @(#)regparse.c	1.12 (gritter) 9/22/03
wcharm.h: * Sccsid @(#)wcharm.h	1.12 (gritter) 10/18/03
*/
