/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, November 2002.
 *
 * Sccsid @(#)regparse.c	1.12 (gritter) 9/22/03
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
#include <ctype.h>
#include "re.h"

LIBUXRE_STATIC void
libuxre_regdeltree(Tree *tp, int all)
{
	if (tp == 0)
		return;
	if (tp->op < 0)
	{
		switch (KIND_ROP(tp->op))
		{
		case BINARY_ROP:
			libuxre_regdeltree(tp->right.ptr, all);
			/*FALLTHROUGH*/
		case UNARY_ROP:
			libuxre_regdeltree(tp->left.ptr, all);
			break;
		default:
			if (tp->op == ROP_BKT && all)
			{
				libuxre_bktfree(tp->right.info.bkt);
				free(tp->right.info.bkt);
			}
			break;
		}
	}
	free(tp);
}

LIBUXRE_STATIC Tree *
libuxre_reg1tree(w_type op, Tree *lp)
{
	Tree *tp;

	if ((tp = malloc(sizeof(Tree))) == 0)
	{
		if (lp != 0)
			libuxre_regdeltree(lp, 1);
		return 0;
	}
	tp->op = op;
	tp->left.ptr = lp;
	if (lp != 0)
		lp->parent = tp;
	return tp;
}

LIBUXRE_STATIC Tree *
libuxre_reg2tree(w_type op, Tree *lp, Tree *rp)
{
	Tree *tp;

	if ((tp = malloc(sizeof(Tree))) == 0)
	{
		libuxre_regdeltree(lp, 1);
		libuxre_regdeltree(rp, 1);
		return 0;
	}
	tp->op = op;
	tp->left.ptr = lp;
	lp->parent = tp;
	tp->right.ptr = rp;
	rp->parent = tp;
	return tp;
}

static int
lex(Lex *lxp)
{
	size_t num;
	w_type wc;
	int n, mb_cur_max;

	mb_cur_max = lxp->mb_cur_max;
nextc:	switch (wc = *lxp->pat++) /* interesting ones are single bytes */
	{
	case '\0':
		lxp->pat--;	/* continue to report ROP_END */
		wc = ROP_END;
		break;
	case '(':
		if (lxp->flags & REG_PARENS)
		{
		leftparen:;
			/*
			* Must keep track of the closed and
			* yet-to-be closed groups as a list.
			* Consider (()a(()b(()c(()d... in which
			* at each letter another even-numbered
			* group is made available, but no
			* odd-numbered ones are.
			*/
			if ((lxp->flags & REG_NOBACKREF) == 0)
			{
				if (lxp->nleft >= lxp->nclist) /* grow it */
				{
					unsigned char *p;

					lxp->nclist += 8; /* arbitrary */
					if ((p = realloc(lxp->clist,
						lxp->nclist)) == 0)
					{
						lxp->err = REG_ESPACE;
						return -1;
					}
					lxp->clist = p;
				}
				lxp->clist[lxp->nleft] = 0; /* unavailable */
			}
			lxp->nleft++;
			wc = ROP_LP;
		}
		break;
	case ')':
		/*
		* For REG_PARENS, only take a right paren as a close
		* if there is a matching left paren.
		*/
		if (lxp->flags & REG_PARENS && lxp->nright < lxp->nleft)
		{
			lxp->nright++;
		rightparen:;
			/*
			* The group that is being closed is the highest
			* numbered as-yet-unclosed group.
			*/
			if ((lxp->flags & REG_NOBACKREF) == 0)
			{
				num = lxp->nleft;
				while (lxp->clist[--num] != 0)
					;
				lxp->clist[num] = 1;
			}
			wc = ROP_RP;
		}
		break;
	case '.':
		wc = ROP_ANYCH;
		if (lxp->flags & REG_NEWLINE)
			wc = ROP_NOTNL;
		break;
	case '*':
		if (lxp->flags & REG_ADDITIVE)
		{
	nxtstar:	switch (*lxp->pat)
			{
			case '+':
				if ((lxp->flags & REG_PLUS) == 0)
					break;
				lxp->pat++;
				goto nxtstar;
			case '?':
				if ((lxp->flags & REG_QUEST) == 0)
					break;
				/*FALLTHRU*/
			case '*':
				lxp->pat++;
				goto nxtstar;
			}
		}
		wc = ROP_STAR;
		break;
	case '^':
		/*
		* Look "behind" to see if this is an anchor.
		* Take it as an anchor if it follows an alternation
		* operator.  (lxp->tok is initially set to ROP_OR.)
		*/
		if (lxp->flags & REG_ANCHORS || lxp->tok == ROP_OR) {
			if (lxp->flags & REG_ADDITIVE)
			{
				int optional = 0;

			nxtcar:	switch (*lxp->pat)
				{
				case '+':
					if ((lxp->flags & REG_PLUS) == 0)
						break;
					lxp->pat++;
					goto nxtcar;
				case '?':
					if ((lxp->flags & REG_QUEST) == 0)
						break;
					/*FALLTHRU*/
				case '*':
					optional = 1;
					lxp->pat++;
					goto nxtcar;
				}
				if (optional)
					goto nextc;
			}
			wc = ROP_BOL;
		}
		break;
	case '$':
		/*
		* Look ahead to see if this is an anchor,
		* unless any '$' is an anchor.
		* Take it as an anchor if it occurs just before
		* the pattern end or an alternation operator.
		*/
		if (lxp->flags & REG_ANCHORS || *lxp->pat == '\0'
			|| (lxp->flags & REG_OR && *lxp->pat == '|')
			|| (lxp->flags & REG_NLALT && *lxp->pat == '\n'))
		{
			if (lxp->flags & REG_ADDITIVE)
			{
				int optional = 0;

			nxtdol:	switch (*lxp->pat)
				{
				case '+':
					if ((lxp->flags & REG_PLUS) == 0)
						break;
					lxp->pat++;
					goto nxtdol;
				case '?':
					if ((lxp->flags & REG_QUEST) == 0)
						break;
					/*FALLTHRU*/
				case '*':
					optional = 1;
					lxp->pat++;
					goto nxtdol;
				}
				if (optional)
					goto nextc;
			}
			wc = ROP_EOL;
		}
		break;
	case '+':
		if (lxp->flags & REG_PLUS)
		{
			wc = ROP_PLUS;
			if (lxp->flags & REG_ADDITIVE)
			{
		nxtplus:	switch (*lxp->pat)
				{
				case '?':
					if ((lxp->flags & REG_QUEST) == 0)
						break;
				case '*':
					wc = ROP_STAR;
					/*FALLTHRU*/
				case '+':
					lxp->pat++;
					goto nxtplus;
				}
			}
		}
		break;
	case '?':
		if (lxp->flags & REG_QUEST)
		{
			wc = ROP_QUEST;
			if (lxp->flags & REG_ADDITIVE)
			{
		nxtquest:	switch (*lxp->pat)
				{
				case '+':
					if ((lxp->flags & REG_PLUS) == 0)
						break;
				case '*':
					wc = ROP_STAR;
					/*FALLTHRU*/
				case '?':
					lxp->pat++;
					goto nxtquest;
				}
			}
		}
		break;
	case '\n':
		if (lxp->flags & REG_NLALT)
		{
			/*
			* Even when newline is an alternative separator,
			* it doesn't permit parenthesized subexpressions
			* to include it.
			*/
			if (lxp->nleft != lxp->nright)
			{
				lxp->err = REG_EPAREN;
				return -1;
			}
			wc = ROP_OR;
		}
		else if (lxp->flags & REG_NEWLINE)
			lxp->flags |= REG_NFA;
		break;
	case '|':
		if (lxp->flags & REG_OR)
			wc = ROP_OR;
		break;
	case '[':
		if ((lxp->info.bkt = malloc(sizeof(Bracket))) == 0)
		{
			lxp->err = REG_ESPACE;
			return -1;
		}
		if ((lxp->flags & REG_GOTBKT) == 0)	/* first time */
		{
			struct lc_collate *col;

			lxp->flags |= REG_GOTBKT;
			lxp->bktflags = 0;
			if (lxp->flags & REG_ICASE)
				lxp->bktflags |= BKT_ONECASE;
			if (lxp->flags & REG_NEWLINE)
				lxp->bktflags |= BKT_NOTNL;
			if (lxp->flags & REG_BADRANGE)
				lxp->bktflags |= BKT_BADRANGE;
			if (lxp->flags & REG_ODDRANGE)
				lxp->bktflags |= BKT_ODDRANGE;
			if (lxp->flags & REG_SEPRANGE)
				lxp->bktflags |= BKT_SEPRANGE;
			if (lxp->flags & REG_BKTQUOTE)
				lxp->bktflags |= BKT_QUOTE;
			if (lxp->flags & REG_BKTEMPTY)
				lxp->bktflags |= BKT_EMPTY;
			if (lxp->flags & REG_ESCNL)
				lxp->bktflags |= BKT_ESCNL;
			if (lxp->flags & REG_NLALT)
				lxp->bktflags |= BKT_NLBAD;
			if (lxp->flags & REG_ESCSEQ)
				lxp->bktflags |= BKT_ESCSEQ;
			if (lxp->flags & REG_BKTESCAPE)
				lxp->bktflags |= BKT_ESCAPE;
			if (lxp->flags & REG_NOI18N)
				lxp->bktflags |= BKT_NOI18N;
			if (lxp->flags & REG_OLDESC)
				lxp->bktflags |= BKT_OLDESC;
			if ((col = libuxre_lc_collate(0)) != 0)
			{
				if (col->maintbl == 0
					|| col->flags & CHF_ENCODED)
				{
					(void)libuxre_lc_collate(col);
					col = 0;
				}
				else if (col->flags & CHF_MULTICH)
					lxp->flags |= REG_NFA;
			}
			lxp->col = col;
		}
		n = lxp->bktflags;
		if (*lxp->pat == '^')
		{
			n |= BKT_NEGATED;
			lxp->pat++;
		}
		lxp->info.bkt->col = lxp->col;
		if ((n = libuxre_bktmbcomp(lxp->info.bkt, lxp->pat,
						n, mb_cur_max)) < 0)
		{
			free(lxp->info.bkt);
			lxp->err = -n;	/* convert to REG_* errors */
			return -1;
		}
		/*
		* NFA forced if newline can be a match and REG_NEWLINE is set.
		*/
		if ((lxp->flags & (REG_NFA | REG_NEWLINE)) == REG_NEWLINE
			&& lxp->pat[-1] == '[' /* i.e., not BKT_NEGATED */
			&& libuxre_bktmbexec(lxp->info.bkt, '\n', 0, 1) == 0)
		{
			lxp->flags |= REG_NFA;
		}
		lxp->pat += n;
		wc = ROP_BKT;
		break;
	case '{':
		if (lxp->flags & REG_NOBRACES || (lxp->flags & REG_BRACES) == 0)
			break;
	interval:;
		if (!isdigit(num = *lxp->pat))
		{
		badbr:;
			lxp->err = REG_BADBR;
			if (*lxp->pat == '\0')
				lxp->err = REG_EBRACE; /* more accurate */
			return -1;
		}
		num -= '0';
		while (isdigit(wc = *++lxp->pat))
		{
			num *= 10;
			if ((num += wc - '0') > BRACE_MAX)
				goto badbr;
		}
		lxp->info.num[0] = num;
		lxp->info.num[1] = num;
		if (wc == ',')
		{
			lxp->info.num[1] = BRACE_INF;
			if (isdigit(wc = *++lxp->pat))
			{
				num = wc - '0';
				while (isdigit(wc = *++lxp->pat))
				{
					num *= 10;
					if ((num += wc - '0') > BRACE_MAX)
						goto badbr;
				}
				if (num < lxp->info.num[0])
					goto badbr;
				lxp->info.num[1] = num;
			}
		}
		if ((lxp->flags & REG_BRACES) == 0)
		{
			if (wc != '\\')
				goto badbr;
			wc = *++lxp->pat;
		}
		if (wc != '}')
			goto badbr;
		lxp->pat++;
		wc = ROP_BRACE;
		/*
		* Replace interval with simpler equivalents where possible,
		* even when the operators are not otherwise available.
		*/
		if (lxp->info.num[1] <= 1)
		{
			if (lxp->info.num[0] == 1)
				wc = ROP_NOP;	/* {1,1} is noise */
			else if (lxp->info.num[1] == 0)
				wc = ROP_EMPTY;	/* {0,0} is empty string */
			else
				wc = ROP_QUEST;	/* {0,1} is ? */
		}
		else if (lxp->info.num[1] == BRACE_INF)
		{
			if (lxp->info.num[0] == 0)
				wc = ROP_STAR;
			else if (lxp->info.num[0] == 1)
				wc = ROP_PLUS;
			else if (lxp->info.num[0] > BRACE_DFAMAX)
				lxp->flags |= REG_NFA;
		}
		else if (lxp->info.num[1] > BRACE_DFAMAX)
		{
			lxp->flags |= REG_NFA;
		}
		break;
	case '\\':
		switch (wc = *lxp->pat++)
		{
		case '\0':
			lxp->err = REG_EESCAPE;
			return -1;
		case '<':
			if (lxp->flags & REG_ANGLES)
			{
				lxp->flags |= REG_NFA;
				wc = ROP_LT;
			}
			goto out;
		case '>':
			if (lxp->flags & REG_ANGLES)
			{
				lxp->flags |= REG_NFA;
				wc = ROP_GT;
			}
			goto out;
		case '(':
			if ((lxp->flags & REG_PARENS) == 0)
				goto leftparen;
			goto out;
		case ')':
			if ((lxp->flags & REG_PARENS) == 0)
			{
				if (++lxp->nright > lxp->nleft)
				{
					lxp->err = REG_EPAREN;
					return -1;
				}
				goto rightparen;
			}
			goto out;
		case '{':
			if (lxp->flags & (REG_BRACES|REG_NOBRACES))
				goto out;
			goto interval;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			num = wc - '0';
			if ((lxp->flags & REG_NOBACKREF) == 0)
			{
			backref:;
				if (num > lxp->nleft
					|| lxp->clist[num - 1] == 0)
				{
					lxp->err = REG_ESUBREG;
					return -1;
				}
				lxp->info.sub = num;
				if (lxp->maxref < num)
					lxp->maxref = num;
				lxp->flags |= REG_NFA;
				wc = ROP_REF;
				goto out;
			}
			/*
			* For compatibility (w/awk), permit "octal" 8 and 9.
			* Already have the value of the first digit in num.
			*
			* If REG_OLDESC, exactly three digits must be present.
			*/
		tryoctal:;
			if ((lxp->flags & REG_ESCSEQ) == 0)
				goto out;
			if ((wc = *lxp->pat) >= '0' && wc <= '9')
			{
				num <<= 3;
				num += wc - '0';
				if ((wc = *++lxp->pat) >= '0' && wc <= '9')
				{
					num <<= 3;
					num += wc - '0';
					lxp->pat++;
				}
				else if (lxp->flags & REG_OLDESC)
				{
					lxp->pat--;
					wc = lxp->pat[-1];
					goto out;
				}
			}
			else if (lxp->flags & REG_OLDESC)
			{
				wc = lxp->pat[-1];
				goto out;
			}
			if ((wc = num) <= 0)
			{
				lxp->err = REG_BADESC;
				return -1;
			}
			goto out;
		case '0':
			if ((lxp->flags & REG_NOBACKREF) == 0
				&& (num = *lxp->pat) >= '0' && num <= '9')
			{
				num -= '0';
				/*
				* This loop ignores wraparounds.
				* Keep track of number of digits in n.
				*/
				n = 1;
				while ((wc = *++lxp->pat) >= '0' && wc <= '9')
				{
					num *= 10;
					num += wc - '0';
					n++;
				}
				if (num != 0)
					goto backref;
				lxp->pat -= n;
			}
			num = 0;
			goto tryoctal;
		case 'a':
			if ((lxp->flags&(REG_ESCSEQ|REG_OLDESC)) == REG_ESCSEQ)
				wc = '\a';
			goto out;
		case 'b':
			if (lxp->flags & REG_ESCSEQ)
				wc = '\b';
			goto out;
		case 'f':
			if (lxp->flags & REG_ESCSEQ)
				wc = '\f';
			goto out;
		case 'n':
			if (lxp->flags & (REG_ESCSEQ | REG_ESCNL))
			{
				wc = '\n';
				if (lxp->flags & REG_NEWLINE)
					lxp->flags |= REG_NFA;
			}
			goto out;
		case 'r':
			if (lxp->flags & REG_ESCSEQ)
				wc = '\r';
			goto out;
		case 't':
			if (lxp->flags & REG_ESCSEQ)
				wc = '\t';
			goto out;
		case 'v':
			if ((lxp->flags&(REG_ESCSEQ|REG_OLDESC)) == REG_ESCSEQ)
				wc = '\v';
			goto out;
		case 'x':
			if ((lxp->flags&(REG_ESCSEQ|REG_OLDESC)) == REG_ESCSEQ
				&& isxdigit(num = *lxp->pat))
			{
				wc = num;
				num = 0;
				/*
				* Take as many hex digits as possible,
				* ignoring overflows.
				* If the result (squeezed into a w_type)
				* is positive, it's okay.
				*/
				do
				{
					if (isdigit(wc))
						wc -= '0';
					else if (isupper(wc))
						wc -= 'A' + 10;
					else
						wc -= 'a' + 10;
					num <<= 4;
					num |= wc;
				} while (isxdigit(wc = *++lxp->pat));
				if ((wc = num) <= 0)
				{
					lxp->err = REG_BADESC;
					return -1;
				}
			}
			goto out;
		}
		/*FALLTHROUGH*/
	default:
		if (!ISONEBYTE(wc))
		{
			if ((n = libuxre_mb2wc(&wc, lxp->pat)) > 0)
				lxp->pat += n;
			else if (n < 0)
			{
				lxp->err = REG_ILLSEQ;
				return -1;
			}
		}
		if (lxp->flags & REG_ICASE)
			wc = to_lower(wc);
		break;
	}
out:;
	lxp->tok = wc;
	return 0;
}

static Tree	*alt(Lex *);

static Tree *
leaf(Lex *lxp)
{
	Tree *tp;

	if ((tp = malloc(sizeof(Tree))) == 0)
	{
		lxp->err = REG_ESPACE;
		return 0;
	}
	switch (tp->op = lxp->tok) /* covers most cases */
	{
	default:
		if (tp->op < 0)
		{
			lxp->err = REG_BADPAT;
			tp->right.ptr = 0;
			goto badunary;
		}
		break;
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
		if ((lxp->flags & REG_NOAUTOQUOTE) == 0
			&& lxp->pat[-1] != '}')
		{
			tp->op = lxp->pat[-1];
			break;
		}
		/*FALLTHROUGH*/
	case ROP_BRACE:
	case ROP_EMPTY:	/* was {0,0} ROP_BRACE */
	case ROP_NOP:	/* was {1,1} ROP_BRACE */
		lxp->err = REG_BADRPT;
	badunary:;
		tp->left.ptr = 0;
		goto err;
	case ROP_ANYCH:
	case ROP_NOTNL:
		break;
	case ROP_BOL:
	case ROP_EOL:
	case ROP_LT:
	case ROP_GT:
		/*
		* Look ahead for what would have been taken to be
		* postfix operators.
		*/
		if (lex(lxp) != 0)
			goto err;
		switch (lxp->tok)
		{
		case ROP_STAR:
		case ROP_PLUS:
		case ROP_QUEST:
			if ((lxp->flags & REG_NOAUTOQUOTE) == 0
				&& lxp->pat[-1] != '}')
			{
				lxp->tok = lxp->pat[-1];
				break;
			}
			/*FALLTHROUGH*/
		case ROP_BRACE:
		case ROP_EMPTY:	/* was {0,0} ROP_BRACE */
		case ROP_NOP:	/* was {1,1} ROP_BRACE */
			lxp->err = REG_BADRPT;
			goto err;
		}
		return tp;
	case ROP_BKT:
		tp->right.info.bkt = lxp->info.bkt;
		break;
	case ROP_REF:
		tp->right.info.sub = lxp->info.sub;
		break;
	case ROP_LP:
		tp->right.info.sub = lxp->nleft;
		if (lex(lxp) != 0)
			goto badunary;
		if (lxp->tok == ROP_RP)	/* empty parens; choice of meaning */
		{
			if (lxp->flags & REG_MTPARENBAD)
			{
				lxp->err = REG_EMPTYPAREN;
				goto badunary;
			}
			lxp->tok = ROP_EMPTY;
			if (lxp->flags & REG_MTPARENFAIL)
				lxp->tok = ROP_NONE;
			if ((tp->left.ptr = libuxre_reg1tree(lxp->tok, 0)) == 0)
				goto badunary;
		}
		else if ((tp->left.ptr = alt(lxp)) == 0)
		{
			if (lxp->err == REG_BADPAT)
				goto parenerr;
			goto badunary;
		}
		else if (lxp->tok != ROP_RP)
		{
			lxp->err = REG_BADPAT;
		parenerr:;
			if (lxp->nleft != lxp->nright)
				lxp->err = REG_EPAREN;	/* better choice */
			goto badunary;
		}
		tp->left.ptr->parent = tp;
		break;
	}
	if (lex(lxp) != 0)
	{
	err:;
		libuxre_regdeltree(tp, 1);
		tp = 0;
	}
	return tp;
}

static Tree *
post(Lex *lxp)
{
	Tree *lp;

	if ((lp = leaf(lxp)) == 0)
		return 0;
	switch (lxp->tok)
	{
	case ROP_EMPTY:	/* this was {0,0} ROP_BRACE */
		libuxre_regdeltree(lp, 1);
		lp = 0;
		/*FALLTHROUGH*/
	case ROP_BRACE:
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
		if ((lp = libuxre_reg1tree(lxp->tok, lp)) == 0)
		{
			lxp->err = REG_ESPACE;
			return 0;
		}
		if (lxp->tok == ROP_BRACE)
			lp->right.info = lxp->info;
		/*FALLTHROUGH*/
	case ROP_NOP:	/* this was {1,1} ROP_BRACE */
		if (lex(lxp) != 0)
		{
			libuxre_regdeltree(lp, 1);
			return 0;
		}
		break;
	}
	return lp;
}

static Tree *
cat(Lex *lxp)
{
	Tree *lp, *rp;

	if ((lp = post(lxp)) == 0)
		return 0;
	for (;;)
	{
		if (lxp->tok == ROP_OR || lxp->tok == ROP_RP
			|| lxp->tok == ROP_END)
		{
			return lp;
		}
		if ((rp = post(lxp)) == 0)
			break;
		if ((lp = libuxre_reg2tree(ROP_CAT, lp, rp)) == 0)
		{
			lxp->err = REG_ESPACE;
			return 0;
		}
	}
	libuxre_regdeltree(lp, 1);
	return 0;
}

static Tree *
alt(Lex *lxp)
{
	Tree *lp, *rp;

	if ((lp = cat(lxp)) == 0)
		return 0;
	for (;;)
	{
		if (lxp->tok != ROP_OR)
			return lp;
		if (lex(lxp) != 0)
			break;
		if (lxp->tok == ROP_END)
			return lp;	/* ignore trailing '|' */
		if ((rp = cat(lxp)) == 0)
			break;
		if ((lp = libuxre_reg2tree(ROP_OR, lp, rp)) == 0)
		{
			lxp->err = REG_ESPACE;
			return 0;
		}
	}
	libuxre_regdeltree(lp, 1);
	return 0;
}

LIBUXRE_STATIC Tree *
libuxre_regparse(Lex *lxp, const unsigned char *pat, int flags)
{
	Tree *lp, *rp;

	lp = 0;			/* in case of error */
	lxp->clist = 0;
	lxp->col = 0;
	lxp->err = 0;
	lxp->maxref = 0;
	lxp->nleft = 0;
	lxp->nright = 0;
	lxp->nclist = 0;
	lxp->mb_cur_max = MB_CUR_MAX;
	if (flags & REG_OR && *pat == '|')
		pat++;	/* skip initial OR like egrep did */
	lxp->pat = pat;
	lxp->flags = flags;
	lxp->tok = ROP_OR;	/* enables ^ as anchor */
	/*
	* Get initial token.
	*/
	if (lex(lxp) != 0)
	{
	err:;
		if (lp != 0)
		{
			libuxre_regdeltree(lp, 1);
			lp = 0;
		}
		if (lxp->err == 0)
			lxp->err = REG_ESPACE;
		goto ret;
	}
	if (lxp->tok == ROP_END)
	{
		lxp->err = REG_NOPAT;
		goto err;
	}
	if ((lp = alt(lxp)) == 0)	/* parse entire RE */
		goto err;
	if (lxp->maxref != 0 || (flags & REG_NOSUB) == 0)
	{
		if ((lp = libuxre_reg1tree(ROP_LP, lp)) == 0)
			goto err;
		lp->right.info.sub = 0;
	}
	if ((rp = libuxre_reg1tree(ROP_END, 0)) == 0)
		goto err;
	if ((lp = libuxre_reg2tree(ROP_CAT, lp, rp)) == 0)
		goto err;
	lp->parent = 0;
ret:;
	if (lxp->clist != 0)
		free(lxp->clist);
	return lp;
}

#ifdef REGDEBUG

LIBUXRE_STATIC void
libuxre_regtree(Tree *tp, int n)
{
	const char *opstr;
	char buf[32];
	int kind, next;

	if (n < 0)
		next = -n + 2;
	else
		next = n + 2;
	switch (tp->op)
	{
	case ROP_OR:
		opstr = "|";
		kind = BINARY_ROP;
		break;
	case ROP_CAT:
		opstr = "&";
		kind = BINARY_ROP;
		break;
	case ROP_STAR:
		opstr = "*";
		kind = UNARY_ROP;
		break;
	case ROP_PLUS:
		opstr = "+";
		kind = UNARY_ROP;
		break;
	case ROP_QUEST:
		opstr = "?";
		kind = UNARY_ROP;
		break;
	case ROP_BRACE:
		opstr = buf;
		if (tp->right.info.num[1] == BRACE_INF)
		{
			sprintf(buf, "{%u,inf}",
				(unsigned)tp->right.info.num[0]);
		}
		else
		{
			sprintf(buf, "{%u,%u}",
				(unsigned)tp->right.info.num[0],
				(unsigned)tp->right.info.num[1]);
		}
		kind = UNARY_ROP;
		break;
	case ROP_LP:
		opstr = buf;
		sprintf(buf, "%lu(", (unsigned long)tp->right.info.sub);
		kind = UNARY_ROP;
		break;
	case ROP_RP:
		opstr = buf;
		sprintf(buf, ")%lu", (unsigned long)tp->right.info.sub);
		kind = UNARY_ROP;
		break;
	case ROP_NOP:
		opstr = "<NOP>";
		kind = LEAF_ROP;
		break;
	case ROP_BOL:
		opstr = "<BOL>";
		kind = LEAF_ROP;
		break;
	case ROP_EOL:
		opstr = "<EOL>";
		kind = LEAF_ROP;
		break;
	case ROP_ALL:
		opstr = "<ALL>";
		kind = LEAF_ROP;
		break;
	case ROP_ANYCH:
		opstr = "<ANYCH>";
		kind = LEAF_ROP;
		break;
	case ROP_NOTNL:
		opstr = "<NOTNL>";
		kind = LEAF_ROP;
		break;
	case ROP_EMPTY:
		opstr = "<MT>";
		kind = LEAF_ROP;
		break;
	case ROP_NONE:
		opstr = "<NONE>";
		kind = LEAF_ROP;
		break;
	case ROP_BKT:
		opstr = buf;
		sprintf(buf, "[%#lx]", (unsigned long)tp->right.info.bkt);
		kind = LEAF_ROP;
		break;
	case ROP_BKTCOPY:
		opstr = buf;
		sprintf(buf, "[%#lx]CPY", (unsigned long)tp->right.info.bkt);
		kind = LEAF_ROP;
		break;
	case ROP_LT:
		opstr = "\\<";
		kind = LEAF_ROP;
		break;
	case ROP_GT:
		opstr = "\\>";
		kind = LEAF_ROP;
		break;
	case ROP_REF:
		opstr = buf;
		sprintf(buf, "\\%lu", (unsigned long)tp->right.info.sub);
		kind = LEAF_ROP;
		break;
	case ROP_END:
		opstr = "<END>";
		kind = LEAF_ROP;
		break;
	default:
		opstr = buf;
		if (tp->op > UCHAR_MAX)
			sprintf(buf, "W%#x", tp->op);
		else if (tp->op <= 0)
			sprintf(buf, "UNK=%u", tp->op);
		else
			sprintf(buf, "%c", tp->op);
		kind = LEAF_ROP;
		break;
	}
	if (kind == BINARY_ROP)
		libuxre_regtree(tp->right.ptr, -next);
	printf("%*c:%s\n", next - 1, n < 0 ? 'R' : n > 0 ? 'L' : 'T', opstr);
	if (kind != LEAF_ROP)
		libuxre_regtree(tp->left.ptr, next);
}

#endif /*REGDEBUG*/
