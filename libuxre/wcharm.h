/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)wcharm.h	1.12 (gritter) 10/18/03
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
/* Stubbed-out wide character locale information */

#ifndef	LIBUXRE_WCHARM_H
#define	LIBUXRE_WCHARM_H

#ifndef	LIBUXRE_STATIC
#define	LIBUXRE_STATIC
#endif

#ifndef LIBUXRE_WUCHAR_T
#define LIBUXRE_WUCHAR_T
typedef unsigned int wuchar_type;
#endif

#ifndef	LIBUXRE_W_TYPE
#define	LIBUXRE_W_TYPE
typedef	int w_type;
#endif

#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>

#ifdef	notdef
#define ISONEBYTE(ch)	((ch), 1)

#define libuxre_mb2wc(wp, cp)	((wp), (cp), 0)
#endif	/* notdef */

#define	ISONEBYTE(ch)	(((ch) & 0200) == 0 || mb_cur_max == 1)

#define	to_lower(ch)	(mb_cur_max > 1 ? towlower(ch) : (wint_t)tolower(ch))
#define	to_upper(ch)	(mb_cur_max > 1 ? towupper(ch) : (wint_t)toupper(ch))

LIBUXRE_STATIC int	libuxre_mb2wc(w_type *, const unsigned char *);

#endif	/* !LIBUXRE_WCHARM_H */
