/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regerror.c	1.4 (gritter) 3/29/03
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
#include <string.h>
#include "re.h"
/*	include "_locale.h"	*/

/*	#pragma weak regerror = _regerror	*/

size_t
regerror(int err, const regex_t *ep, char *str, size_t max)
{
	const struct
	{
		int		index;
		const char	*str;
	} unk = 
	{
			88,  "unknown regular expression error"
	}, msgs[] =
	{
	/*ENOSYS*/	{ 89,  "feature not implemented" },
	/*0*/		{ 0,   "" },
	/*NOMATCH*/	{ 90,  "regular expression failed to match" },
	/*BADPAT*/	{ 91,  "invalid regular expression" },
	/*ECOLLATE*/	{ 92,  "invalid collating element construct" },
	/*ECTYPE*/	{ 93,  "invalid character class construct" },
	/*EEQUIV*/	{ 94,  "invalid equivalence class construct" },
	/*EBKTCHAR*/	{ 95,  "invalid character in '[ ]' construct" },
	/*EESCAPE*/	{ 96,  "trailing \\ in pattern" },
	/*ESUBREG*/	{ 97,  "'\\digit' out of range" },
	/*EBRACK*/	{ 98,  "'[ ]' imbalance" },
	/*EMPTYSUBBKT*/	{ 99,  "empty nested '[ ]' construct" },
	/*EMPTYPAREN*/	{ 100, "empty '\\( \\)' or '( )'" },
	/*NOPAT*/	{ 101, "empty pattern" },
	/*EPAREN*/	{ 102, "'\\( \\)' or '( )' imbalance" },
	/*EBRACE*/	{ 103, "'\\{ \\} or '{ }' imbalance" },
	/*BADBR*/	{ 104, "invalid '\\{ \\}' or '{ }'" },
	/*ERANGE*/	{ 105, "invalid endpoint in range" },
	/*ESPACE*/	{ 106, "out of regular expression memory" },
	/*BADRPT*/	{ 107, "invalid *, +, ?, \\{\\} or {} operator" },
	/*BADESC*/	{ 108, "invalid escape sequence (e.g. \\0)" },
	/*ILLSEQ*/	{ 109, "illegal byte sequence"}
	};
	const char *p;
	size_t len;

	(void)ep;
	if (err < REG_ENOSYS || REG_ILLSEQ < err)
	{
		p = unk.str;
	}
	else
	{
		p = msgs[err - REG_ENOSYS].str;
	}
	len = strlen(p) + 1;
	if (max != 0)
	{
		if (max > len)
			max = len;
		else if (max < len)
			str[--max] = '\0';
		memcpy(str, p, max);
	}
	return len;
}
