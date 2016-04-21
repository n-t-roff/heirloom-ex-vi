/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)bracket.c	1.14 (gritter) 10/18/03
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "re.h"

/*
* Build and match the [...] part of REs.
*
* In general, each compiled bracket construct holds a set of mapped
* wide character values and a set of character classifications.
* The mapping applied (when the current LC_COLLATE is not CHF_ENCODED)
* is the "basic" weight (cep->weight[0]); otherwise the actual wide
* character is used.
*
* To support simplified range handling, this code assumes that a w_type,
* a signed integer type, can hold all valid basic weight values (as well
* as all wide character values for CHF_ENCODED locales) and that these
* are all positive.  Negative values indicate error conditions (BKT_*);
* zero (which must be the same as WGHT_IGNORE) indicates success, but
* that the item installed is not a range endpoint.
*/

static int
addwide(Bracket *bp, wchar_t ord)
{
	unsigned int nw;

	if ((nw = bp->nwide) < NWIDE)
		bp->wide[nw] = ord;
	else
	{
		if (nw % NWIDE == 0 && (bp->exwide =
			realloc(bp->exwide, nw * sizeof(wchar_t))) == 0)
		{
			return BKT_ESPACE;
		}
		nw -= NWIDE;
		bp->exwide[nw] = ord;
	}
	bp->nwide++;
	return 0;
}

#if USHRT_MAX == 65535	/* have 16 bits */
#define PLIND(n)	((n) >> 4)
#define PLBIT(n)	(1 << ((n) & 0xf))
#else
#define PLIND(n)	((n) / CHAR_BIT)
#define PLBIT(n)	(1 << ((n) % CHAR_BIT))
#endif

#define RANGE	((wchar_t)'-')	/* separates wide chars in ranges */

static int
addrange(Bracket *bp, wchar_t ord, w_type prev)
{
	int ret;

	if (prev > 0 && prev != ord) /* try for range */
	{
		if (prev > ord)
		{
			if (bp->flags & BKT_ODDRANGE)	/* prev only - done */
				return 0;
			else if ((bp->flags & BKT_BADRANGE) == 0)
				return BKT_ERANGE;
		}
		else
		{
			if (++prev <= UCHAR_MAX) /* "prev" already there */
			{
				do
				{
					bp->byte[PLIND(prev)] |= PLBIT(prev);
					if (prev == ord)
						return 0;
				} while (++prev <= UCHAR_MAX);
			}
			if ((ret = addwide(bp, prev)) != 0)
				return ret;
			if (++prev > ord)
				return 0;
			if (prev < ord && (ret = addwide(bp, RANGE)) != 0)
				return ret;
			return addwide(bp, ord);
		}
	}
	if (ord <= UCHAR_MAX)
	{
		bp->byte[PLIND(ord)] |= PLBIT(ord);
		return 0;
	}
	if (prev == ord) /* don't bother */
		return 0;
	return addwide(bp, ord);
}

static w_type
place(Bracket *bp, wchar_t wc, w_type prev, int mb_cur_max)
{
	const CollElem *cep;
	CollElem spare;
	int ret;

	(void)mb_cur_max;
	if ((cep = libuxre_collelem(bp->col, &spare, wc)) != ELEM_ENCODED)
	{
		if (cep == ELEM_BADCHAR)
			return BKT_BADCHAR;
		wc = cep->weight[0];
	}
	if ((ret = addrange(bp, wc, prev)) != 0)
		return ret;
	return wc;
}

#ifndef CHARCLASS_NAME_MAX
#   define CHARCLASS_NAME_MAX	127
#endif

static w_type
chcls(Bracket *bp, const unsigned char *s, int n)
{
	char clsstr[CHARCLASS_NAME_MAX + 1];
	unsigned int nt;
	wctype_t wct;

	if (n > CHARCLASS_NAME_MAX)
		return BKT_ECTYPE;
	(void)memcpy(clsstr, s, n);
	clsstr[n] = '\0';
	if ((wct = wctype(clsstr)) == 0)
		return BKT_ECTYPE;
	if ((nt = bp->ntype) < NTYPE)
		bp->type[nt] = wct;
	else
	{
		if (nt % NTYPE == 0 && (bp->extype =
			realloc(bp->extype, nt * sizeof(wctype_t))) == 0)
		{
			return BKT_ESPACE;
		}
		nt -= NTYPE;
		bp->extype[nt] = wct;
	}
	bp->ntype++;
	return 0; /* cannot be end point of a range */
}

	/*
	* The purpose of mcce() and its Mcce structure is to locate
	* the next full collation element from "wc" and "s".  It is
	* called both at compile and execute time.  These two differ
	* primarily in that at compile time there is an exact number
	* of bytes to be consumed, while at execute time the longest
	* valid collation element is to be found.
	*
	* When BKT_ONECASE is set, MCCEs become particularly messy.
	* There is no guarantee that all possible combinations of
	* upper/lower case are defined as MCCEs.  Thus, this code
	* tries both lower- and uppercase (in that order) for each
	* character than might be part of an MCCE.
	*/

typedef struct
{
	const unsigned char	*max;	/* restriction by caller */
	const unsigned char	*aft;	/* longest successful */
	Bracket			*bp;	/* readonly */
	struct lc_collate	*col;	/* readonly */
	const CollElem		*cep;	/* entry matching longest */
	wchar_t			ch;	/* initial character (if any) */
	w_type			wc;	/* character matching "aft" */
} Mcce;

static int
mcce(Mcce *mcp, const CollElem *cep, const unsigned char *s, int mb_cur_max,
		int compile_time)
{
	const CollElem *nxt;
	CollElem spare;
	w_type ch, wc;
	int i;

	/*
	* Get next character.
	*/
	if ((wc = mcp->ch) != '\0')
	{
		mcp->ch = '\0';
	}
	else if (ISONEBYTE(wc = *s++))
	{
		if (wc == '\0')
			return 0;
	}
	else if ((i = libuxre_mb2wc(&wc, s)) > 0)
	{
		s += i;
		if (mcp->max != 0 && s > mcp->max)
			return 0;
	}
	else if (i < 0)
		return BKT_ILLSEQ;
	/*
	* Try out the this character as part of an MCCE.
	* If BKT_ONECASE is set, this code tries both the lower- and
	* uppercase version, continuing if it matches so far.
	*/
	ch = wc;
	if (mcp->bp->flags & BKT_ONECASE)
	{
		if ((wc = to_lower(wc)) == ch)
			ch = to_upper(wc);
	}
	for (;;) /* at most twice */
	{
		if (cep == ELEM_BADCHAR) /* first character */
		{
			if ((nxt = libuxre_collelem(mcp->col, &spare, wc))
				== ELEM_ENCODED
				|| (mcp->col->flags & CHF_MULTICH) == 0
				|| s == mcp->max)
			{
				mcp->aft = s;
				mcp->cep = nxt;
				mcp->wc = wc;
				break;
			}
		}
		else
		{
			nxt = libuxre_collmult(mcp->col, cep, wc);
		} 
		if (nxt != ELEM_BADCHAR)
		{
			/*
			* Okay so far.  Record this collating element
			* if it's really one (not WGHT_IGNORE) and
			* we've reached a new high point or it's the
			* first match.
			*
			* If there's a possibility for more, call mcce()
			* recursively for the subsequent characters.
			*/
			if (nxt->weight[0] != WGHT_IGNORE
				&& (mcp->aft < s || mcp->cep == ELEM_BADCHAR))
			{
				mcp->aft = s;
				mcp->cep = nxt;
				mcp->wc = wc;
			}
			if (nxt->multbeg != 0
				&& (mcp->max == 0 || s < mcp->max))
			{
				if ((i = mcce(mcp, nxt, s, mb_cur_max,
						compile_time)) != 0)
					return i;
			}
		}
		if (wc == ch)
			break;
		wc = ch;
	}
	return 0;
}

static w_type
eqcls(Bracket *bp, const unsigned char *s, int n, w_type prev, int mb_cur_max)
{
	w_type last;
	Mcce mcbuf;
	int err;

	mcbuf.max = &s[n];
	mcbuf.aft = &s[0];
	mcbuf.bp = bp;
	mcbuf.col = bp->col;
	mcbuf.cep = ELEM_BADCHAR;
	mcbuf.ch = '\0';
	if ((err = mcce(&mcbuf, ELEM_BADCHAR, s, mb_cur_max, 1)) != 0)
		return err;
	if (mcbuf.cep == ELEM_BADCHAR || mcbuf.aft != mcbuf.max)
		return BKT_EEQUIV;
	last = mcbuf.wc;
	if (mcbuf.cep != ELEM_ENCODED && mcbuf.col->nweight > 1)
	{
		const CollElem *cep;

		/*
		* The first and last weight[0] values for equivalence
		* classes are stuffed into the terminator for the
		* multiple character lists.  If these values are
		* scattered (elements that are not part of this
		* equivalence class have weight[0] values between the
		* two end points), then SUBN_SPECIAL is placed in
		* this terminator.  Note that weight[1] of the
		* terminator must be other than WGHT_IGNORE, too.
		*/
		last = mcbuf.cep->weight[0];
		if ((cep = libuxre_collmult(bp->col, mcbuf.cep, 0))
				!= ELEM_BADCHAR
			&& cep->weight[1] != WGHT_IGNORE)
		{
			last = cep->weight[1];
			if (cep->subnbeg == SUBN_SPECIAL)
			{
				unsigned int nq;

				/*
				* Permit ranges up to the first and
				* after the last.
				*/
				if (prev > 0 && (unsigned)prev != cep->weight[0]
					&& (prev = addrange(bp,
						cep->weight[0], prev)) != 0)
				{
					return prev;
				}
				/*
				* Record the equivalence class by storing
				* the primary weight.
				*/
				if ((nq = bp->nquiv) < NQUIV)
					bp->quiv[nq] = mcbuf.cep->weight[1];
				else
				{
					if (nq % NQUIV == 0 && (bp->exquiv = 
						realloc(bp->exquiv,
							nq * sizeof(wuchar_type)))
						== 0)
					{
						return REG_ESPACE;
					}
					nq -= NQUIV;
					bp->exquiv[nq] = mcbuf.cep->weight[1];
				}
				bp->nquiv++;
				return last;
			}
			mcbuf.cep = cep;
		}
		mcbuf.wc = mcbuf.cep->weight[0];
	}
	/*
	* Determine range, if any, to install.
	*
	* If there's a pending low (prev > 0), then try to use it.
	*
	* Otherwise, try to use mcbuf.wc as the low end of the range.
	* Since addrange() assumes that the low point has already been
	* placed, we try to fool it by using a prev of one less than
	* mcbuf.wc.  But, if that value would not look like a valid
	* low point of a range, we have to explicitly place mcbuf.wc.
	*/
	if (prev <= 0 && (prev = mcbuf.wc - 1) <= 0)
	{
		if ((prev = addrange(bp, mcbuf.wc, 0)) != 0)
			return prev;
	}
	if ((mcbuf.wc = addrange(bp, last, prev)) != 0)
		return mcbuf.wc;
	return last;
}

static w_type
clsym(Bracket *bp, const unsigned char *s, int n, w_type prev, int mb_cur_max)
{
	Mcce mcbuf;
	int err;

	mcbuf.max = &s[n];
	mcbuf.aft = &s[0];
	mcbuf.bp = bp;
	mcbuf.col = bp->col;
	mcbuf.cep = ELEM_BADCHAR;
	mcbuf.ch = '\0';
	if ((err = mcce(&mcbuf, ELEM_BADCHAR, s, mb_cur_max, 1)) != 0)
		return err;
	if (mcbuf.cep == ELEM_BADCHAR || mcbuf.aft != mcbuf.max)
		return BKT_ECOLLATE;
	if (mcbuf.cep != ELEM_ENCODED)
		mcbuf.wc = mcbuf.cep->weight[0];
	if ((err = addrange(bp, mcbuf.wc, prev)) != 0)
		return err;
	return mcbuf.wc;
}

	/*
	* Scans the rest of a bracket construction within a regular
	* expression and fills in a description for it.
	* The leading [ and the optional set complement indicator
	* were handled already by the caller.
	* Returns:
	*	<0 error (a BKT_* value)
	*	>0 success; equals how many bytes were scanned.
	*/
LIBUXRE_STATIC int
libuxre_bktmbcomp(Bracket *bp, const unsigned char *pat0,
		int flags, int mb_cur_max)
{
	static const Bracket zero;
	const unsigned char *pat = pat0;
	struct lc_collate *savecol;
	w_type n, wc, prev = 0;

	/*
	* Set represented set to empty.  Easiest to copy an empty
	* version over the caller's, (re)setting col and flags.
	*/
	savecol = bp->col;
	*bp = zero;
	bp->col = savecol;
	bp->flags = flags
		& (BKT_NEGATED | BKT_ONECASE | BKT_NOTNL | BKT_BADRANGE |
				BKT_ODDRANGE);
	/*
	* Handle optional "empty" brackets; typically only used
	* in combination with BKT_QUOTE or BKT_ESCAPE.
	*/
	if ((wc = *pat) == ']' && (flags & BKT_EMPTY) != 0)
		return 1;
	/*
	* Populate *bp.
	*/
	for (;; prev = n)
	{
		switch (wc)
		{
		case '\0':
		ebrack:;
			n = BKT_EBRACK;
			goto err;
		case '\n':
			if (flags & BKT_NLBAD)
				goto ebrack;
			goto regular;
		case '/':
			if (flags & BKT_SLASHBAD)
				goto ebrack;
			goto regular;
		case '\\':
			if ((flags & (BKT_ESCAPE | BKT_QUOTE
				| BKT_ESCNL | BKT_ESCSEQ)) == 0)
			{
				goto regular;
			}
			switch (wc = *++pat)
			{
			default:
			noesc:;
				if ((flags & BKT_ESCAPE) == 0)
				{
					wc = '\\';
					pat--;
				}
				break;
			case '\\':
			case ']':
			case '-':
			case '^':
				if ((flags & BKT_QUOTE) == 0)
					goto noesc;
				break;
			case 'a':
				if ((flags & BKT_ESCSEQ) == 0 ||
						(flags & BKT_OLDESC))
					goto noesc;
				wc = '\a';
				break;
			case 'b':
				if ((flags & BKT_ESCSEQ) == 0)
					goto noesc;
				wc = '\b';
				break;
			case 'f':
				if ((flags & BKT_ESCSEQ) == 0)
					goto noesc;
				wc = '\f';
				break;
			case 'n':
				if ((flags & (BKT_ESCSEQ | BKT_ESCNL)) == 0)
					goto noesc;
				wc = '\n';
				break;
			case 'r':
				if ((flags & BKT_ESCSEQ) == 0)
					goto noesc;
				wc = '\r';
				break;
			case 't':
				if ((flags & BKT_ESCSEQ) == 0)
					goto noesc;
				wc = '\t';
				break;
			case 'v':
				if ((flags & BKT_ESCSEQ) == 0 ||
						(flags & BKT_OLDESC))
					goto noesc;
				wc = '\v';
				break;
			case 'x':
				if ((flags & BKT_ESCSEQ) == 0 ||
						(flags & BKT_OLDESC))
					goto noesc;
				if (!isxdigit(wc = *++pat))
				{
					pat--;
					goto noesc;
				}
				/*
				* Take as many hex digits as possible,
				* ignoring overflows.
				* Any positive result is okay.
				*/
				n = 0;
				do
				{
					if (isdigit(wc))
						wc -= '0';
					else if (isupper(wc))
						wc -= 'A' + 10;
					else
						wc -= 'a' + 10;
					n <<= 4;
					n |= wc;
				} while (isxdigit(wc = *++pat));
				pat--;
				if ((wc = n) <= 0)
				{
					n = BKT_BADESC;
					goto err;
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if ((flags & BKT_ESCSEQ) == 0 ||
						(flags & BKT_OLDESC))
					goto noesc;
				/*
				* For compatibility (w/awk),
				* permit "octal" 8 and 9.
				*/
				n = wc - '0';
				if ((wc = *++pat) >= '0' && wc <= '9')
				{
					n <<= 3;
					n += wc - '0';
					if ((wc = *++pat) >= '0' && wc <= '9')
					{
						n <<= 3;
						n += wc - '0';
					}
				}
				pat--;
				if ((wc = n) <= 0)
				{
					n = BKT_BADESC;
					goto err;
				}
				break;
			}
			goto regular;
		case '[':
			if (((wc = *++pat) == ':' || wc == '=' || wc == '.') &&
					(flags & BKT_NOI18N) == 0)
			{
				n = 0;
				while (*++pat != wc || pat[1] != ']')
				{
					if (*pat == '\0')
					{
					badpat:;
						n = BKT_BADPAT;
						goto err;
					}
					else if (*pat == '/')
					{
						if (flags & BKT_SLASHBAD)
							goto badpat;
					}
					else if (*pat == '\n')
					{
						if (flags & BKT_NLBAD)
							goto badpat;
					}
					n++;
				}
				if (n == 0)
				{
					n = BKT_EMPTYSUBBKT;
					goto err;
				}
				if (wc == ':')
					n = chcls(bp, &pat[-n], n);
				else if (wc == '=')
					n = eqcls(bp, &pat[-n], n, prev,
							mb_cur_max);
				else /* wc == '.' */
					n = clsym(bp, &pat[-n], n, prev,
							mb_cur_max);
				pat++;
				break;
			}
			wc = '[';
			pat--;
			goto regular;
		default:
			if (!ISONEBYTE(wc) &&
					(n = libuxre_mb2wc(&wc, pat + 1)) > 0)
				pat += n;
		regular:;
			n = place(bp, wc, prev, mb_cur_max);
			break;
		}
		if (n < 0) {
			n = BKT_ILLSEQ;
			goto err;
		}
		if ((wc = *++pat) == ']')
			break;
		if (wc == '-' && n != 0)
		{
			if (prev == 0 || (flags & BKT_SEPRANGE) == 0)
			{
				if ((wc = *++pat) != ']')
					continue; /* valid range */
				wc = '-';
				pat--;
			}
		}
		n = 0;	/* no range this time */
	}
	return pat - pat0 + 1;
err:;
	libuxre_bktfree(bp);
	return n;
}

LIBUXRE_STATIC void
libuxre_bktfree(Bracket *bp)
{
	if (bp->extype != 0)
		free(bp->extype);
	if (bp->exquiv != 0)
		free(bp->exquiv);
	if (bp->exwide != 0)
		free(bp->exwide);
}

LIBUXRE_STATIC int
libuxre_bktmbexec(Bracket *bp, wchar_t wc,
		const unsigned char *str, int mb_cur_max)
{
	unsigned int i;
	wchar_t lc, uc;
	Mcce mcbuf;

	mcbuf.aft = str; /* in case of match in character classes */
	mcbuf.ch = wc;
	/*
	* First: check the single wc against any character classes.
	* Since multiple character collating elements are not part
	* of this world, they don't apply here.
	*/
	if ((i = bp->ntype) != 0)
	{
		wctype_t *wctp = &bp->type[0];

		if (bp->flags & BKT_ONECASE)
		{
			if ((wc = to_lower(wc)) == mcbuf.ch)
				mcbuf.ch = to_upper(wc);
		}
		for (;;)
		{
			if (iswctype(mb_cur_max == 1 ? btowc(wc)
			    : (wint_t)wc, *wctp))
				goto match;
			if (wc != mcbuf.ch &&
			    iswctype(mb_cur_max == 1 ? btowc(mcbuf.ch)
			    : (wint_t)mcbuf.ch, *wctp))
				goto match;
			if (--i == 0)
				break;
			if (++wctp == &bp->type[NTYPE])
				wctp = &bp->extype[0];
		}
	}
	/*
	* The main match is determined by the weight[0] value
	* of the character (or characters, if the input can be
	* taken as a multiple character collating element).
	*/
	mcbuf.max = 0;
	mcbuf.bp = bp;
	mcbuf.col = bp->col;
	mcbuf.cep = ELEM_BADCHAR;
	mcce(&mcbuf, ELEM_BADCHAR, str, mb_cur_max, 0);
	if (mcbuf.cep == ELEM_BADCHAR)
		return -1;	/* never matches */
	if (mcbuf.cep != ELEM_ENCODED)
		mcbuf.wc = mcbuf.cep->weight[0];
	/*
	* POSIX.2 demands that both a character and its case counterpart
	* can match if REG_ICASE is set. This means that [B-z] matches
	* 'A', 'a', and '['.
	*/
	if (bp->flags & BKT_ONECASE)
	{
		lc = to_lower(mcbuf.wc);
		uc = to_upper(mcbuf.wc);
	}
	else
		lc = uc = mcbuf.wc;
	/*
	* See if it's in the set.  Note that the list of true wide
	* character values has explicit ranges.
	*/
	if (mcbuf.wc <= UCHAR_MAX)
	{
		if (bp->byte[PLIND(lc)] & PLBIT(lc))
			goto match;
		if (lc != uc && (bp->byte[PLIND(uc)] & PLBIT(uc)))
			goto match;
	}
	else if ((i = bp->nwide) != 0)
	{
		wchar_t *wcp = &bp->wide[0];
		long lcmp, ucmp;

		for (;;)
		{
			if ((lcmp = lc - *wcp) == 0)
				goto match;
			ucmp = uc - *wcp;
			if (lc != uc && ucmp == 0)
				goto match;
			if (--i == 0)
				break;
			if (++wcp == &bp->wide[NWIDE])
				wcp = &bp->exwide[0];
			if (*wcp == RANGE)
			{
				if (++wcp == &bp->wide[NWIDE])
					wcp = &bp->exwide[0];
				if (lcmp > 0 && lc <= *wcp)
					goto match;
				if (lc != uc && ucmp > 0 && uc < *wcp)
					goto match;
				if ((i -= 2) == 0)
					break;
				if (++wcp == &bp->wide[NWIDE])
					wcp = &bp->exwide[0];
			}
		}
	}
	/*
	* The last chance for a match is if an equivalence class
	* was specified for which the primary weights are scattered
	* through the weight[0]s.
	*/
	if ((i = bp->nquiv) != 0 && mcbuf.cep != ELEM_ENCODED)
	{
		wuchar_type *wucp = &bp->quiv[0];

		mcbuf.wc = mcbuf.cep->weight[1];
		for (;;)
		{
			if ((wuchar_type)mcbuf.wc == *wucp)
				goto match;
			if (--i == 0)
				break;
			if (++wucp == &bp->quiv[NQUIV])
				wucp = &bp->exquiv[0];
		}
	}
	/*
	* Only here when no match against the set was found.
	* One final special case w/r/t newline.
	*/
	if (bp->flags & BKT_NEGATED)
	{
		if (wc != '\n' || (bp->flags & BKT_NOTNL) == 0)
			return mcbuf.aft - str;
	}
	return -1;
match:;
	/*
	* Only here when a match against the described set is found.
	*/
	if (bp->flags & BKT_NEGATED)
		return -1;
	return mcbuf.aft - str;
}
