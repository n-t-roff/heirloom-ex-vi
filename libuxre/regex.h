/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regex.h	1.13 (gritter) 2/6/05
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

#ifndef LIBUXRE_REGEX_H
#define LIBUXRE_REGEX_H
/*	from unixsrc:usr/src/common/head/regex.h /main/uw7_nj/1	*/

#include <sys/types.h>	/* really only want [s]size_t */

	/*
	* Official regexec() flags.
	*/
#define REG_NOTBOL	0x000001 /* start of string does not match ^ */
#define REG_NOTEOL	0x000002 /* end of string does not match $ */

	/*
	* Additional regexec() flags.
	*/
#define REG_NONEMPTY	0x000004 /* do not match empty at start of string */

	/*
	* Extensions to provide individual control over each
	* of the differences between basic and extended REs.
	*/
#define REG_OR		0x0000001 /* enable | operator */
#define REG_PLUS	0x0000002 /* enable + operator */
#define REG_QUEST	0x0000004 /* enable ? operator */
#define REG_BRACES	0x0000008 /* use {m,n} (instead of \{m,n\}) */
#define REG_PARENS	0x0000010 /* use (...) [instead of \(...\)] */
#define REG_ANCHORS	0x0000020 /* ^ and $ are anchors anywhere */
#define REG_NOBACKREF	0x0000040 /* disable \digit */
#define REG_NOAUTOQUOTE	0x0000080 /* no automatic quoting of REG_BADRPTs */

	/*
	* Official regcomp() flags.
	*/
#define REG_EXTENDED	(REG_OR | REG_PLUS | REG_QUEST | REG_BRACES | \
				REG_PARENS | REG_ANCHORS | \
				REG_NOBACKREF | REG_NOAUTOQUOTE)
#define REG_ICASE	0x0000100 /* ignore case */
#define REG_NOSUB	0x0000200 /* only success/fail for regexec() */
#define REG_NEWLINE	0x0000400 /* take \n as line separator for ^ and $ */

	/*
	* Additional regcomp() flags.
	* Some of these assume that int is >16 bits!
	* Beware: 0x20000000 and above are used in re.h.
	*/
#define REG_ONESUB	0x0000800 /* regexec() only needs pmatch[0] */
#define REG_MTPARENFAIL	0x0001000 /* take empty \(\) or () as match failure */
#define REG_MTPARENBAD	0x0002000 /* disallow empty \(\) or () */
#define REG_BADRANGE	0x0004000 /* accept [m-a] ranges as [ma] */
#define	REG_ODDRANGE	0x0008000 /* oawk oddity: [m-a] means [m] */
#define REG_SEPRANGE	0x0010000 /* disallow [a-m-z] style ranges */
#define REG_BKTQUOTE	0x0020000 /* allow \ in []s to quote \, -, ^ or ] */
#define REG_BKTEMPTY	0x0040000 /* allow empty []s (w/BKTQUOTE, BKTESCAPE) */
#define REG_ANGLES	0x0080000 /* enable \<, \> operators */
#define REG_ESCNL	0x0100000 /* take \n as newline character */
#define REG_NLALT	0x0200000 /* take newline as alternation */
#define REG_ESCSEQ	0x0400000 /* otherwise, take \ as start of C escapes */
#define REG_BKTESCAPE	0x0800000 /* allow \ in []s to quote next anything */
#define	REG_NOBRACES	0x1000000 /* disable {n,m} */
#define	REG_ADDITIVE	0x2000000 /* a+*b means + and * additive, ^+ is valid */
#define	REG_NOI18N	0x4000000 /* disable I18N features ([::] etc.) */
#define	REG_OLDESC	0x8000000 /* recognize \b \f \n \r \t \123 only */
#define	REG_AVOIDNULL	0x10000000/* avoid null subexpression matches */
#define REG_OLDBRE	(REG_BADRANGE | REG_ANGLES | REG_ESCNL)
#define REG_OLDERE	(REG_OR | REG_PLUS | REG_QUEST | REG_NOBRACES | \
				REG_PARENS | REG_ANCHORS | REG_ODDRANGE | \
				REG_NOBACKREF | REG_ADDITIVE | REG_NOAUTOQUOTE)

	/*
	* Error return values.
	*/
#define REG_ENOSYS	(-1)	/* unsupported */
#define	REG_NOMATCH	1	/* regexec() failed to match */
#define	REG_BADPAT	2	/* invalid regular expression */
#define	REG_ECOLLATE	3	/* invalid collating element construct */
#define	REG_ECTYPE	4	/* invalid character class construct */
#define REG_EEQUIV	5	/* invalid equivalence class construct */
#define REG_EBKTCHAR	6	/* invalid character in [] construct */
#define	REG_EESCAPE	7	/* trailing \ in pattern */
#define	REG_ESUBREG	8	/* number in \digit invalid or in error */
#define	REG_EBRACK	9	/* [] imbalance */
#define REG_EMPTYSUBBKT	10	/* empty sub-bracket construct */
#define REG_EMPTYPAREN	11	/* empty \(\) or () [REG_MTPARENBAD] */
#define REG_NOPAT	12	/* no (empty) pattern */
#define	REG_EPAREN	13	/* \(\) or () imbalance */
#define	REG_EBRACE	14	/* \{\} or {} imbalance */
#define	REG_BADBR	15	/* contents of \{\} or {} invalid */
#define	REG_ERANGE	16	/* invalid endpoint in expression */
#define	REG_ESPACE	17	/* out of memory */
#define	REG_BADRPT	18	/* *,+,?,\{\} or {} not after r.e. */
#define REG_BADESC	19	/* invalid escape sequence (e.g. \0) */
#define	REG_ILLSEQ	20	/* illegal byte sequence */

typedef struct
{
	size_t		re_nsub;	/* only advertised member */
	unsigned long	re_flags;	/* augmented regcomp() flags */
	struct re_dfa_	*re_dfa;	/* DFA engine */
	struct re_nfa_	*re_nfa;	/* NFA engine */
	struct re_coll_	*re_col;	/* current collation info */
	int		re_mb_cur_max;	/* MB_CUR_MAX acceleration */
	void		*re_more;	/* just in case... */
} regex_t;

typedef ssize_t regoff_t;

typedef struct
{
	regoff_t	rm_so;
	regoff_t	rm_eo;
} regmatch_t;

#ifdef __cplusplus
extern "C" {
#endif

int	regcomp(regex_t *, const char *, int);
int	regexec(const regex_t *, const char *, size_t, regmatch_t *, int);
size_t	regerror(int, const regex_t *, char *, size_t);
void	regfree(regex_t *);

#ifdef __cplusplus
}
#endif

#endif /* !LIBUXRE_REGEX_H */
