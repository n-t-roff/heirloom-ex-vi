/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regexec.c	1.7 (gritter) 2/6/05
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
#include "re.h"

/*	#pragma weak regexec = _regexec	*/

int
regexec(const regex_t *ep, const char *s, size_t n, regmatch_t *mp, int flg)
{
	Exec ex;
	int ret;

	ex.flags = flg | (ep->re_flags & (REG_NEWLINE|REG_ICASE|REG_AVOIDNULL));
	ex.str = (const unsigned char *)s;
	ex.match = mp;
	ex.mb_cur_max = ep->re_mb_cur_max;
	if ((ex.nmatch = n) != 0) /* impose limits from compile flags */
	{
		if (ep->re_flags & REG_NOSUB)
			n = ex.nmatch = 0;
		else if (ep->re_flags & REG_ONESUB)
			ex.nmatch = 1;
		else if (n > ep->re_nsub + 1)
			ex.nmatch = ep->re_nsub + 1;
	}
	if (ep->re_flags & REG_DFA && ex.nmatch <= 1)
		ret = libuxre_regdfaexec(ep->re_dfa, &ex);
	else
		ret = libuxre_regnfaexec(ep->re_nfa, &ex);
	/*
	* Fill unused part of mp[].
	*/
	if (ret != 0)
		ex.nmatch = 0;
	while (n > ex.nmatch)
	{
		n--;
		mp[n].rm_so = -1;
		mp[n].rm_eo = -1;
	}
	return ret;
}
