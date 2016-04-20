/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)colldata.h	1.5 (gritter) 5/1/04
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

#ifndef	LIBUXRE_COLLDATA_H
#define	LIBUXRE_COLLDATA_H

typedef struct
{
	long	coll_offst;	/* offset to xnd table */
	long	sub_cnt;	/* length of subnd table */
	long	sub_offst;	/* offset to subnd table */
	long	str_offst;	/* offset to strings for subnd table */
	long	flags;		/* nonzero if reg.exp. used */
} hd;

typedef struct
{
	unsigned char	ch;	/* character or number of followers */
	unsigned char	pwt;	/* primary weight */
	unsigned char	swt;	/* secondary weight */
	unsigned char	ns;	/* index of follower state list */
} xnd;

typedef struct
{
	char	*exp;	/* expression to be replaced */
	long	explen; /* length of expression */
	char	*repl;	/* replacement string */
} subnd;

/*----------------------------------*/

#include <wcharm.h>
#include <limits.h>
/*	#include <stdlock.h>	*/

/*
* Structure of a collation file:
*  1. CollHead (maintbl is 0 if CHF_ENCODED)
*   if !CHF_ENCODED then
*    2. CollElem[bytes] (256 for 8 bit bytes)
*    3. if CHF_INDEXED then
*	 CollElem[wides] (nmain-256 for 8 bit bytes)
*	else
*	 CollMult[wides]
*    4. CollMult[*] (none if multtbl is 0)
*    5. wuchar_type[*] (none if repltbl is 0)
*    6. CollSubn[*] (none if subntbl is 0)
*    7. strings (first is pathname for .so if CHF_DYNAMIC)
*
* The actual location of parts 2 through 7 is not important.
*
* The main table is in encoded value order.
*
* All indeces/offsets must be nonzero to be effective; zero is reserved
* to indicate no-such-entry.  This implies either that an unused initial
* entry is placed in each of (4) through (7), or that the "start offset"
* given by the header is artificially pushed back by an entry size.
*
* Note that if CHF_ENCODED is not set, then nweight must be positive.
*
* If an element can begin a multiple character element, it contains a
* nonzero multbeg which is the initial index into (4) for its list;
* the list is terminated by a CollMult with a ch of zero.
*
* If there are elements with the same primary weight (weight[1]), then
* for each such element, it must have a CollMult list.  The CollMult
* that terminates the list (ch==0) notes the lowest and highest basic
* weights for those elements with that same primary weight value
* respectively in weight[0] and weight[1].  If there are some basic
* weights between these values that do not have the same primary
* weight--are not in the equivalence class--then the terminator also
* has a SUBN_SPECIAL mark.  Note that this list terminator should be
* shared when the elements are not multiple character collating
* elements because they wouldn't otherwise have a CollMult list.
*
* WGHT_IGNORE is used to denote ignored collating elements for a
* particular collation ordering pass.  All main table entries other
* than for '\0' will have a non-WGHT_IGNORE weight[0].  However, it is
* possible for a CollMult entries from (4) to have a WGHT_IGNORE
* weight[0]:  If, for example, "xyz" is a multiple character collating
* element, but "xy" is not, then the CollMult for "y" will have a
* WGHT_IGNORE weight[0].  Also, WGHT_IGNORE is used to terminate each
* list of replacement weights.
*
* Within (3), it is possible to describe a sequence of unremarkable
* collating elements with a single CollMult entry.  If the SUBN_SPECIAL
* bit is set, the rest of subnbeg represents the number of collating
* elements covered by this entry.  The weight[0] values are determined
* by adding the difference between the encoded value and the entry's ch
* value to the entry's weight[0].  This value is then substituted for
* any weight[n], n>0 that has only the WGHT_SPECIAL bit set. libuxre_collelem()
* hides any match to such an entry by filling in a "spare" CollElem.
*
* If there are substitution strings, then for each character that begins
* a string, it has a nonzero subnbeg which is similarly the initial
* index into (6).  The indeces in (6) refer to offsets within (7).
*/

#define TOPBIT(t)	(((t)1) << (sizeof(t) * CHAR_BIT - 1))

#define CHF_ENCODED	0x1	/* collation by encoded values only */
#define CHF_INDEXED	0x2	/* main table indexed by encoded values */
#define CHF_MULTICH	0x4	/* a multiple char. coll. elem. exists */
#define CHF_DYNAMIC	0x8	/* shared object has collation functions */

#define CWF_BACKWARD	0x1	/* reversed ordering for this weight */
#define CWF_POSITION	0x2	/* weight takes position into account */

#define CLVERS		1	/* most recent version */

#define WGHT_IGNORE	0	/* ignore this collating element */
#define WGHT_SPECIAL	TOPBIT(wuchar_type)
#define SUBN_SPECIAL	TOPBIT(unsigned short)

#ifndef	COLL_WEIGHTS_MAX
#define	COLL_WEIGHTS_MAX	1
#endif

typedef struct
{
	unsigned long	maintbl;	/* start of main table */
	unsigned long	multtbl;	/* start of multi-char table */
	unsigned long	repltbl;	/* start of replacement weights */
	unsigned long	subntbl;	/* start of substitutions */
	unsigned long	strstbl;	/* start of sub. strings */
	unsigned long	nmain;		/* # entries in main table */
	unsigned short	flags;		/* CHF_* bits */
	unsigned short	version;	/* handle future changes */
	unsigned char	elemsize;	/* # bytes/element (w/padding) */
	unsigned char	nweight;	/* # weights/element */
	unsigned char	order[COLL_WEIGHTS_MAX]; /* CWF_* bits/weight */
} CollHead;

typedef struct
{
	unsigned short	multbeg;	/* start of multi-chars */
	unsigned short	subnbeg;	/* start of substitutions */
	wuchar_type	weight[COLL_WEIGHTS_MAX];
} CollElem;

typedef struct
{
	wchar_t		ch;	/* "this" character (of sequence) */
	CollElem	elem;	/* its full information */
} CollMult;

typedef struct
{
	unsigned short	strbeg;		/* start of match string */
	unsigned short	length;		/* length of match string */
	unsigned short	repbeg;		/* start of replacement */
} CollSubn;

struct lc_collate
{
	const unsigned char	*strstbl;
	const wuchar_type	*repltbl;
	const CollElem		*maintbl;
	const CollMult		*multtbl;
	const CollSubn		*subntbl;
#ifdef DSHLIB
	void	*handle;
	void	(*done)(struct lc_collate *);
	int	(*strc)(struct lc_collate *, const char *, const char *);
	int	(*wcsc)(struct lc_collate *, const wchar_t *, const wchar_t *);
	size_t	(*strx)(struct lc_collate *, char *, const char *, size_t);
	size_t	(*wcsx)(struct lc_collate *, wchar_t *, const wchar_t *, size_t);
#endif
	const char		*mapobj;
	size_t			mapsize;
	unsigned long		nmain;
	short			nuse;
	unsigned short		flags;
	unsigned char		elemsize;
	unsigned char		nweight;
	unsigned char		order[COLL_WEIGHTS_MAX];
};

#define ELEM_BADCHAR	((CollElem *)0)
#define ELEM_ENCODED	((CollElem *)-1)

/*
LIBUXRE_STATIC int	libuxre_old_collate(struct lc_collate *);
LIBUXRE_STATIC int	libuxre_strqcoll(struct lc_collate *, const char *,
				const char *);
LIBUXRE_STATIC int	libuxre_wcsqcoll(struct lc_collate *, const wchar_t *,
				const wchar_t *);
*/
extern struct lc_collate *libuxre_lc_collate(struct lc_collate *);
LIBUXRE_STATIC const CollElem	*libuxre_collelem(struct lc_collate *,
					CollElem *, wchar_t);
LIBUXRE_STATIC const CollElem	*libuxre_collmult(struct lc_collate *,
					const CollElem *, wchar_t);
/*
LIBUXRE_STATIC const CollElem	*libuxre_collmbs(struct lc_collate *,
					CollElem *, const unsigned char **);
LIBUXRE_STATIC const CollElem	*libuxre_collwcs(struct lc_collate *,
					CollElem *, const wchar_t **);
*/

#endif	/* !LIBUXRE_COLLDATA_H */
