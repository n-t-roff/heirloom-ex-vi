/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regdfa.c	1.9 (gritter) 9/22/03
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "regdfa.h"

/*
* Deterministic Finite Automata.
*/

	/*
	* Postorder traversal that returns a copy of the subtree,
	* except that ROP_BKT becomes ROP_BKTCOPY (since they
	* share the same pointed to Bracket object).
	*/
static Tree *
copy(regex_t *ep, Tree *tp)
{
	Tree *np;

	if ((np = malloc(sizeof(Tree))) == 0)
		return 0;
	switch (np->op = tp->op) /* almost always correct */
	{
	case ROP_BKT:
		np->op = ROP_BKTCOPY;
		/*FALLTHROUGH*/
	case ROP_BKTCOPY:
		np->right.info.bkt = tp->right.info.bkt;
		/*FALLTHROUGH*/
	default:
		np->left.pos = ep->re_dfa->nposn++;
		/*FALLTHROUGH*/
	case ROP_EMPTY:
		return np;
	case ROP_CAT:
	case ROP_OR:
		if ((np->right.ptr = copy(ep, tp->right.ptr)) == 0)
		{
			free(np);
			return 0;
		}
		np->right.ptr->parent = np;
		/*FALLTHROUGH*/
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
	case ROP_LP:
		if ((np->left.ptr = copy(ep, tp->left.ptr)) == 0)
			break;
		np->left.ptr->parent = np;
		return np;
	}
	libuxre_regdeltree(np, 1);
	return 0;
}

	/*
	* Postorder traversal.
	* Assign unique ascending integer values to the leaves.
	* Since the right child is traversed before the left,
	* the position for ROP_END is guaranteed to be zero.
	* The parse tree is rewritten in two cases:
	* - Each ROP_BRACE is replaced by an equivalent--sometimes
	*   large--subtree using only ROP_CAT, ROP_QUEST, and
	*   ROP_PLUS.
	* - If REG_ICASE, replace each simple character that has
	*   an uppercase equivalent with a ROP_OR subtree over the
	*   two versions.
	* Since these rewrites occur bottom up, they have already
	* been applied before any subtrees passed to copy().
	*/
static Tree *
findposn(regex_t *ep, Tree *tp, int mb_cur_max)
{
	unsigned int lo, hi;
	Tree *ptr, *par;
	w_type wc;

	switch (tp->op)
	{
	default:
		if (ep->re_flags & REG_ICASE
			&& (wc = to_upper(tp->op)) != tp->op)
		{
			if ((ptr = libuxre_reg1tree(tp->op, 0)) == 0)
				return 0;
			ptr->parent = tp;
			ptr->left.pos = ep->re_dfa->nposn++;
			tp->op = ROP_OR;
			tp->left.ptr = ptr;
			ptr = libuxre_reg1tree(wc, 0);
			if ((tp->right.ptr = ptr) == 0)
				return 0;
			ptr->parent = tp;
			ptr->left.pos = ep->re_dfa->nposn++;
			return tp;
		}
		/*FALLTHROUGH*/
	case ROP_BOL:
	case ROP_EOL:
	case ROP_ALL:
	case ROP_ANYCH:
	case ROP_NOTNL:
	case ROP_NONE:
	case ROP_BKT:
	case ROP_BKTCOPY:
	case ROP_END:
		tp->left.pos = ep->re_dfa->nposn++;
		return tp;
	case ROP_EMPTY:
		return tp;
	case ROP_OR:
	case ROP_CAT:
		if ((tp->right.ptr = findposn(ep, tp->right.ptr,
						mb_cur_max)) == 0)
			return 0;
		/*FALLTHROUGH*/
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
	case ROP_LP:
		if ((tp->left.ptr = findposn(ep, tp->left.ptr,
						mb_cur_max)) == 0)
			return 0;
		return tp;
	case ROP_BRACE:
		if ((tp->left.ptr = findposn(ep, tp->left.ptr,
						mb_cur_max)) == 0)
			return 0;
		break;
	}
	/*
	* ROP_BRACE as is cannot be handled in a DFA.  This code
	* duplicates the ROP_BRACE subtree as a left-towering
	* series of ROP_CAT nodes, the first "lo" of which are
	* direct copies of the original subtree.  The tail of
	* the series are either some number of ROP_QUESTs over
	* copies of the original subtree, or a single ROP_PLUS
	* over a copy (when "hi" is infinity).
	*
	* All interesting cases {lo,hi}:
	*  {0,0} -> ROP_EMPTY, parsing, temporary
	*  {0,1} -> ROP_QUEST, parsing
	*  {0,2} -> CAT(QUEST(left), QUEST(copy))
	*  {0,n} -> CAT({0,n-1}, QUEST(copy))
	*  {0,}  -> ROP_STAR, parsing
	*
	*  {1,1} -> ROP_NOP, parsing, temporary
	*  {1,2} -> CAT(left, QUEST(copy))
	*  {1,n} -> CAT({1,n-1}, QUEST(copy))
	*  {1,}  -> ROP_PLUS, parsing
	*
	*  {2,2} -> CAT(left, copy)
	*  {2,n} -> CAT({2,n-1}, QUEST(copy))
	*  {2,}  -> CAT(left, PLUS(copy))
	*
	*  {3,3} -> CAT({2,2}, copy)
	*  {3,n} -> CAT({3,n-1}, QUEST(copy))
	*  {3,}  -> CAT({2,2}, PLUS(copy))
	*
	*  {n,}  -> CAT({n-1,n-1}, PLUS(copy))
	*
	* In all cases, the ROP_BRACE node is turned into the
	* left-most ROP_CAT, and a copy of its original subtree
	* is connected as the right child.  Note that the bottom-
	* up nature of this duplication guarantees that copy()
	* never sees a ROP_BRACE node.
	*/
	par = tp->parent;
	lo = tp->right.info.num[0];
	hi = tp->right.info.num[1];
	if ((ptr = copy(ep, tp->left.ptr)) == 0)
		return 0;
	ptr->parent = tp;
	tp->op = ROP_CAT;
	tp->right.ptr = ptr;
	if (lo == 0)
	{
		if ((tp->left.ptr = libuxre_reg1tree(ROP_QUEST, tp->left.ptr))
				== 0)
			return 0;
		tp->left.ptr->parent = tp;
	}
	else
	{
		if (hi == BRACE_INF || (hi -= lo) == 0)
			lo--;	/* lo > 1; no extra needed */
		while (--lo != 0)
		{
			if ((tp = libuxre_reg2tree(ROP_CAT, tp, copy(ep, ptr)))
					== 0)
				return 0;
		}
	}
	if (hi == BRACE_INF)
	{
		if ((tp->right.ptr = libuxre_reg1tree(ROP_PLUS, tp->right.ptr))
				== 0)
			return 0;
		tp->right.ptr->parent = tp;
	}
	else if (hi != 0)
	{
		if ((tp->right.ptr = libuxre_reg1tree(ROP_QUEST, tp->right.ptr))
				== 0)
			return 0;
		ptr = tp->right.ptr;
		ptr->parent = tp;
		while (--hi != 0)
		{
			if ((tp = libuxre_reg2tree(ROP_CAT, tp, copy(ep, ptr)))
					== 0)
				return 0;
		}
	}
	tp->parent = par;
	return tp;
}

	/*
	* Postorder traversal, but not always entire subtree.
	* For each leaf reachable by the empty string, add it
	* to the set.  Return 0 if the subtree can match empty.
	*/
static int
first(Dfa *dp, Tree *tp)
{
	switch (tp->op)
	{
	case ROP_BOL:
		if (dp->flags & REG_NOTBOL)
			return 0;
		break;
	case ROP_EOL:
		if (dp->flags & REG_NOTEOL)
			return 0;
		break;
	case ROP_EMPTY:
		return 0;
	case ROP_OR:
		return first(dp, tp->left.ptr) & first(dp, tp->right.ptr);
	case ROP_CAT:
		if (first(dp, tp->left.ptr) != 0)
			return 1;
		return first(dp, tp->right.ptr);
	case ROP_BRACE:
		if (tp->right.info.num[0] != 0 && first(dp, tp->left.ptr) != 0)
			return 1;
		/*FALLTHROUGH*/
	case ROP_STAR:
	case ROP_QUEST:
		first(dp, tp->left.ptr);
		return 0;
	case ROP_LP:
	case ROP_PLUS:
		return first(dp, tp->left.ptr);
	}
	if (dp->posset[tp->left.pos] == 0)
	{
		dp->posset[tp->left.pos] = 1;
		dp->nset++;
	}
	return 1;
}

	/*
	* Walk from leaf up (most likely not to root).
	* Determine follow set for the leaf by filling
	* set[] with the positions reachable.
	*/
static void
follow(Dfa *dp, Tree *tp)
{
	Tree *pp;

	switch ((pp = tp->parent)->op)
	{
	case ROP_CAT:
		if (pp->left.ptr == tp && first(dp, pp->right.ptr) != 0)
			break;
		/*FALLTHROUGH*/
	case ROP_OR:
	case ROP_QUEST:
	case ROP_LP:
		follow(dp, pp);
		break;
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_BRACE:
		first(dp, tp);
		follow(dp, pp);
		break;
	}
}

	/*
	* Postorder traversal.
	* At each leaf, copy it into posn[] and assign its follow set.
	* Because the left-most subtree is ROP_ALL under ROP_STAR, the
	* follow set for its leaf (position dp->nposn-1) is the same
	* as the initial state's signature (prior to any ROP_BOL).
	*/
static int
posnfoll(Dfa *dp, Tree *tp)
{
	unsigned char *s;
	size_t i, n;
	size_t *fp;
	Posn *p;
	int ret;

	switch (tp->op)
	{
	case ROP_OR:
	case ROP_CAT:
		if ((ret = posnfoll(dp, tp->right.ptr)) != 0)
			return ret;
		/*FALLTHROUGH*/
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
	case ROP_LP:
		if ((ret = posnfoll(dp, tp->left.ptr)) != 0)
			return ret;
		return 0;
	case ROP_END:	/* keeps follow() from walking above the root */
		p = &dp->posn[tp->left.pos];
		p->op = tp->op;
		p->seti = 0;
		p->nset = 0;
		return 0;
	case ROP_BKT:
	case ROP_BKTCOPY:
		p = &dp->posn[tp->left.pos];
		p->bkt = tp->right.info.bkt;
		goto skip;
	case ROP_BOL:
		dp->flags |= REG_NOTBOL; /* adjacent ROP_BOLs match empty */
		break;
	case ROP_EOL:
		dp->flags |= REG_NOTEOL; /* adjacent ROP_EOLs match empty */
		break;
	}
	p = &dp->posn[tp->left.pos];
skip:;
	p->op = tp->op;
	memset(dp->posset, 0, dp->nposn);
	dp->nset = 0;
	follow(dp, tp);
	dp->flags &= ~(REG_NOTBOL | REG_NOTEOL);
	fp = dp->posfoll;
	if ((p->nset = dp->nset) > dp->avail) /* need more */
	{
		if ((n = p->nset << 1) < dp->nposn)
			n = dp->nposn;	
		dp->avail += n;
		if ((fp = realloc(dp->posfoll,
			sizeof(size_t) * (dp->avail + dp->used))) == 0)
		{
			return REG_ESPACE;
		}
		dp->posfoll = fp;
	}
	p->seti = dp->used;
	if ((i = dp->nset) != 0)
	{
		dp->used += i;
		dp->avail -= i;
		fp += p->seti;
		s = dp->posset;
		n = 0;
		do
		{
			if (*s++ != 0)
			{
				*fp++ = n;
				if (--i == 0)
					break;
			}
		} while (++n != dp->nposn);
	}
	return 0;
}

static int
addstate(Dfa *dp) /* install state if unique; return its index */
{
	size_t *sp, *fp;
	size_t t, n, i;
	int flushed;

	/*
	* Compare dp->nset/dp->cursig[] against remembered states.
	*/
	t = dp->top;
	do
	{
		if (dp->nsig[--t] != dp->nset)
			continue;
		if ((n = dp->nset) != 0)
		{
			fp = &dp->sigfoll[dp->sigi[t]];
			sp = &dp->cursig[0];
		loop:;
			if (*fp++ != *sp++)
				continue; /* to the do-while */
			if (--n != 0)
				goto loop;
		}
		return t + 1;
	} while (t != 0);
	/*
	* Not in currently cached states; add it.
	*/
	flushed = 0;
	if ((t = dp->top) >= CACHESZ)	/* need to flush the cache */
	{
		flushed = 1;
		n = dp->anybol;
		n = dp->sigi[n] + dp->nsig[n];	/* past invariant states */
		dp->avail += dp->used - n;
		dp->used = n;
		dp->top = n = dp->nfix;
		memset((void *)&dp->trans, 0, sizeof(dp->trans));
		memset((void *)&dp->acc[n], 0, CACHESZ - n);
		t = n;
	}
	dp->top++;
	fp = dp->sigfoll;
	if ((n = dp->nset) > dp->avail)	/* grow strip */
	{
		i = (dp->avail + n) << 1;
		if ((fp = realloc(fp, sizeof(size_t) * (i + dp->used))) == 0)
			return 0;
		dp->avail = i;
		dp->sigfoll = fp;
	}
	dp->acc[t] = 0;
	if ((dp->nsig[t] = n) != 0)
	{
		sp = dp->cursig;
		if (sp[0] == 0)
			dp->acc[t] = 1;
		dp->sigi[t] = i = dp->used;
		dp->used += n;
		dp->avail -= n;
		fp += i;
		do
			*fp++ = *sp++;
		while (--n != 0);
	}
	t++;
	if (flushed)
		return -t;
	return t;
}

void
libuxre_regdeldfa(Dfa *dp)
{
	Posn *pp;
	size_t np;

	if (dp->posfoll != 0)
		free(dp->posfoll);
	if (dp->sigfoll != 0)
		free(dp->sigfoll);
	if (dp->cursig != 0)
		free(dp->cursig);
	if ((pp = dp->posn) != 0)
	{
		/*
		* Need to walk the positions list to free any
		* space used for ROP_BKTs.
		*/
		np = dp->nposn;
		do
		{
			if (pp->op == ROP_BKT)
			{
				libuxre_bktfree(pp->bkt);
				free(pp->bkt);
			}
		} while (++pp, --np != 0);
		free(dp->posn);
	}
	free(dp);
}

int
regtrans(Dfa *dp, int st, w_type wc, int mb_cur_max)
{
	const unsigned char *s;
	size_t *fp, *sp;
	size_t i, n;
	Posn *pp;
	int nst;

	if ((n = dp->nsig[st]) == 0)	/* dead state */
		return st + 1;		/* stay here */
	memset(dp->posset, 0, dp->nposn);
	dp->nset = 0;
	fp = &dp->sigfoll[dp->sigi[st]];
	do
	{
		pp = &dp->posn[*fp];
		switch (pp->op)
		{
		case ROP_EOL:
			if (wc == '\0' && (dp->flags & REG_NOTEOL) == 0)
				break;
			/*FALLTHROUGH*/
		case ROP_BOL:
		default:
			if (pp->op == wc)
				break;
			/*FALLTHROUGH*/
		case ROP_END:
		case ROP_NONE:
			continue;
		case ROP_NOTNL:
			if (wc == '\n')
				continue;
			/*FALLTHROUGH*/
		case ROP_ANYCH:
			if (wc <= '\0')
				continue;
			break;
		case ROP_ALL:
			if (wc == '\0')
				continue;
			break;
		case ROP_BKT:
		case ROP_BKTCOPY:
			/*
			* Note that multiple character bracket matches
			* are precluded from DFAs.  (See regparse.c and
			* regcomp.c.)  Thus, the continuation string
			* argument is not used in libuxre_bktmbexec().
			*/
			if (wc > '\0' &&
			    libuxre_bktmbexec(pp->bkt, wc, 0, mb_cur_max) == 0)
				break;
			continue;
		}
		/*
		* Current character matches this position.
		* For each position in its follow list,
		* add that position to the new state's signature.
		*/
		i = pp->nset;
		sp = &dp->posfoll[pp->seti];
		do
		{
			if (dp->posset[*sp] == 0)
			{
				dp->posset[*sp] = 1;
				dp->nset++;
			}
		} while (++sp, --i != 0);
	} while (++fp, --n != 0);
	/*
	* Move the signature (if any) into cursig[] and install it.
	*/
	if ((i = dp->nset) != 0)
	{
		fp = dp->cursig;
		s = dp->posset;
		for (n = 0;; n++)
		{
			if (*s++ != 0)
			{
				*fp++ = n;
				if (--i == 0)
					break;
			}
		}
	}
	if ((nst = addstate(dp)) < 0) /* flushed cache */
		nst = -nst;
	else if (nst > 0 && (wc & ~(long)(NCHAR - 1)) == 0)
		dp->trans[st][wc] = nst;
	return nst;
}

LIBUXRE_STATIC int
libuxre_regdfacomp(regex_t *ep, Tree *tp, Lex *lxp)
{
	Tree *lp;
	Dfa *dp;
	Posn *p;
	int st;

	/*
	* It's convenient to insert an STAR(ALL) subtree to the
	* immediate left of the current tree.  This makes the
	* "any match" libuxre_regdfaexec() not a special case,
	* and the initial state signature will fall out when
	* building the follow sets for all the leaves.
	*/
	if ((lp = libuxre_reg1tree(ROP_ALL, 0)) == 0
		|| (lp = libuxre_reg1tree(ROP_STAR, lp)) == 0
		|| (tp->left.ptr = lp
			= libuxre_reg2tree(ROP_CAT, lp, tp->left.ptr)) == 0)
	{
		return REG_ESPACE;
	}
	lp->parent = tp;
	if ((dp = calloc(1, sizeof(Dfa))) == 0)
		return REG_ESPACE;
	ep->re_dfa = dp;
	/*
	* Just in case null pointers aren't just all bits zero...
	*/
	dp->posfoll = 0;
	dp->sigfoll = 0;
	dp->cursig = 0;
	dp->posn = 0;
	/*
	* Assign position values to each of the tree's leaves
	* (the important parts), meanwhile potentially rewriting
	* the parse tree so that it fits within the restrictions
	* of our DFA.
	*/
	if ((tp = findposn(ep, tp, lxp->mb_cur_max)) == 0)
		goto err;
	/*
	* Get space for the array of positions and current set,
	* now that the number of positions is known.
	*/
	if ((dp->posn = malloc(sizeof(Posn) * dp->nposn + dp->nposn)) == 0)
		goto err;
	dp->posset = (unsigned char *)&dp->posn[dp->nposn];
	/*
	* Get follow sets for each position.
	*/
	if (posnfoll(dp, tp) != 0)
		goto err;
	/*
	* Set up the special invariant states:
	*  - dead state (no valid transitions); index 0.
	*  - initial state for any match [STAR(ALL) follow set]; index 1.
	*  - initial state for any match after ROP_BOL.
	*  - initial state for left-most longest if REG_NOTBOL.
	*  - initial state for left-most longest after ROP_BOL.
	* The final two are not allocated if leftmost() cannot be called.
	* The pairs of initial states are the same if there is no
	* explicit ROP_BOL transition.
	*/
	dp->avail += dp->used;
	dp->used = 0;
	if ((dp->sigfoll = malloc(sizeof(size_t) * dp->avail)) == 0)
		goto err;
	p = &dp->posn[dp->nposn - 1];	/* same as first(root) */
	dp->cursig = &dp->posfoll[p->seti];
	dp->nset = p->nset;
	dp->top = 1;	/* index 0 is dead state */
	addstate(dp);	/* must be state index 1 (returns 2) */
	if ((dp->cursig = malloc(sizeof(size_t) * dp->nposn)) == 0)
		goto err;
	dp->nfix = 2;
	if ((st = regtrans(dp, 1, ROP_BOL, lxp->mb_cur_max)) == 0)
		goto err;
	if ((dp->anybol = st - 1) == 2) /* new state */
		dp->nfix = 3;
	if ((ep->re_flags & REG_NOSUB) == 0) /* leftmost() might be called */
	{
		/*
		* leftmost() initial states are the same as the
		* "any match" ones without the STAR(ALL) position.
		*/
		dp->sigi[dp->nfix] = 0;
		dp->nsig[dp->nfix] = dp->nsig[1] - 1;
		dp->acc[dp->nfix] = dp->acc[1];
		dp->leftbol = dp->leftmost = dp->nfix;
		dp->nfix++;
		if (dp->anybol != 1)	/* distinct state w/BOL */
		{
			dp->sigi[dp->nfix] = dp->sigi[2];
			dp->nsig[dp->nfix] = dp->nsig[2] - 1;
			dp->acc[dp->nfix] = dp->acc[2];
			dp->leftbol = dp->nfix;
			dp->nfix++;
		}
		dp->top = dp->nfix;
	}
	return 0;
err:;
	libuxre_regdeldfa(dp);
	return REG_ESPACE;
}

static int
leftmost(Dfa *dp, Exec *xp)
{
	const unsigned char *s, *beg, *end;
	int i, nst, st, mb_cur_max;
	w_type wc;

	mb_cur_max = xp->mb_cur_max;
	beg = s = xp->str;
	end = 0;
	st = dp->leftbol;
	if (xp->flags & REG_NOTBOL)
		st = dp->leftmost;
	if (dp->acc[st] && (xp->flags & REG_NONEMPTY) == 0)
		end = s;	/* initial empty match allowed */
	for (;;)
	{
		if ((wc = *s++) == '\n')
		{
			if (xp->flags & REG_NEWLINE)
				wc = ROP_EOL;
		}
		else if (!ISONEBYTE(wc) && (i = libuxre_mb2wc(&wc, s)) > 0)
			s += i;
		if ((wc & ~(long)(NCHAR - 1)) != 0
			|| (nst = dp->trans[st][wc]) == 0)
		{
			if ((nst=regtrans(dp, st, wc, mb_cur_max)) == 0)
				return REG_ESPACE;
			if (wc == ROP_EOL) /* REG_NEWLINE only */
			{
				if (dp->acc[nst - 1])
				{
					if (end == 0 || end < s)
						end = s;
					break;
				}
				beg = s;
				st = dp->leftbol;
				goto newst;
			}
		}
		if ((st = nst - 1) == 0) /* dead state */
		{
			if (end != 0)
				break;
			if ((wc = *beg++) == '\0')
				return REG_NOMATCH;
			else if (!ISONEBYTE(wc) &&
					(i = libuxre_mb2wc(&wc, beg)) > 0)
				beg += i;
			s = beg;
			st = dp->leftmost;
			goto newst;
		}
		if (wc == '\0')
		{
			if (dp->acc[st])
			{
				s--;	/* don't include \0 */
				if (end == 0 || end < s)
					end = s;
				break;
			}
			if (end != 0)
				break;
			return REG_NOMATCH;
		}
	newst:;
		if (dp->acc[st])
		{
			if (end == 0 || end < s)
				end = s;
		}
	}
	xp->match[0].rm_so = beg - xp->str;
	xp->match[0].rm_eo = end - xp->str;
	return 0;
}

/*
* Optimization by simplification: singlebyte locale and REG_NEWLINE not set.
* Performance gain for grep is 25% so it's worth the hack.
*/
static int
regdfaexec_opt(Dfa *dp, Exec *xp)
{
	const unsigned char *s;
	int nst, st;

	s = xp->str;
	st = dp->anybol;
	if (xp->flags & REG_NOTBOL)
		st = 1;
	if (dp->acc[st] && (xp->flags & REG_NONEMPTY) == 0)
		return 0;	/* initial empty match allowed */
	do
	{
		if ((nst = dp->trans[st][*s]) == 0)
		{
			if ((nst = regtrans(dp, st, *s, 1)) == 0)
				return REG_ESPACE;
		}
		if (dp->acc[st = nst - 1])
			return 0;
	} while (*s++ != '\0');	/* st != 0 */
	return REG_NOMATCH;
}

LIBUXRE_STATIC int
libuxre_regdfaexec(Dfa *dp, Exec *xp)
{
	const unsigned char *s;
	int i, nst, st, mb_cur_max;
	w_type wc;

	dp->flags = xp->flags & REG_NOTEOL;	/* for regtrans() */
	mb_cur_max = xp->mb_cur_max;
	if (xp->nmatch != 0)
		return leftmost(dp, xp);
	if (mb_cur_max == 1 && (xp->flags & REG_NEWLINE) == 0)
		return regdfaexec_opt(dp, xp);
	s = xp->str;
	st = dp->anybol;
	if (xp->flags & REG_NOTBOL)
		st = 1;
	if (dp->acc[st] && (xp->flags & REG_NONEMPTY) == 0)
		return 0;	/* initial empty match allowed */
	for (;;)
	{
		if ((wc = *s++) == '\n')
		{
			if (xp->flags & REG_NEWLINE)
				wc = ROP_EOL;
		}
		else if (!ISONEBYTE(wc) && (i = libuxre_mb2wc(&wc, s)) > 0)
			s += i;
		if ((wc & ~(long)(NCHAR - 1)) != 0
			|| (nst = dp->trans[st][wc]) == 0)
		{
			if ((nst=regtrans(dp, st, wc, mb_cur_max)) == 0)
				return REG_ESPACE;
			if (wc == ROP_EOL) /* REG_NEWLINE only */
			{
				if (dp->acc[nst - 1])
					return 0;
				if (dp->acc[st = dp->anybol])
					return 0;
				continue;
			}
		}
		if (dp->acc[st = nst - 1])
			return 0;
		if (wc == '\0')	/* st == 0 */
			return REG_NOMATCH;
	}
}
