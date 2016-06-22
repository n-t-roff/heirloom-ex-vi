/*
 * This code contains changes by
 *      Gunnar Ritter, Freiburg i. Br., Germany, 2002. All rights reserved.
 *
 * Conditions 1, 2, and 4 and the no-warranty notice below apply
 * to these changes.
 *
 *
 * Copyright (c) 1980, 1993
 * 	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)ex_vops3.c	1.21 (gritter) 8/4/05";
#endif
#endif

/* from ex_vops3.c	7.3 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Routines to handle structure.
 * Operations supported are:
 *	( ) { } [ ]
 *
 * These cover:		LISP		TEXT
 *	( )		s-exprs		sentences
 *	{ }		list at same	paragraphs
 *	[ ]		defuns		sections
 *
 * { and } for C used to attempt to do something with matching {}'s, but
 * I couldn't find definitions which worked intuitively very well, so I
 * scrapped this.
 *
 * The code here is very hard to understand.
 */
line	*llimit;
void	(*lf)(int);

bool	wasend;

/*
 * Find over structure, repeated count times.
 * Don't go past line limit.  F is the operation to
 * be performed eventually.  If pastatom then the user said {}
 * rather than (), implying past atoms in a list (or a paragraph
 * rather than a sentence.
 */
int
llfind(bool pastatom, int cnt, void (*f)(int), line *limit)
{
#ifdef	LISPCODE
	register int c;
#endif
	register int rc = 0;
	char *save = smalloc(LBSIZE);

	/*
	 * Initialize, saving the current line buffer state
	 * and computing the limit; a 0 argument means
	 * directional end of file.
	 */
	wasend = 0;
	lf = f;
	lcpy(save, linebuf, LBSIZE);
	if (limit == 0)
		limit = dir < 0 ? one : dol;
	llimit = limit;
	wdot = dot;
	wcursor = cursor;

	if (pastatom >= 2) {
		while (cnt > 0 && word(f, cnt))
			cnt--;
		if (pastatom == 3)
			eend(f);
		if (dot == wdot) {
			wdot = 0;
			if (cursor == wcursor)
				rc = -1;
		}
	}
#ifdef LISPCODE
	else if (!value(LISP)) {
#else
	else {
#endif
		char *icurs;
		line *idot;

		if (linebuf[0] == 0) {
			do
				if (!lnext())
					goto ret;
			while (linebuf[0] == 0);
			if (dir > 0) {
				wdot--;
				linebuf[0] = 0;
				wcursor = linebuf;
				/*
				 * If looking for sentence, next line
				 * starts one.
				 */
				if (!pastatom) {
					icurs = wcursor;
					idot = wdot;
					goto begin;
				}
			}
		}
		icurs = wcursor;
		idot = wdot;

		/*
		 * Advance so as to not find same thing again.
		 */
		if (dir > 0) {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		} else
			ignore(lskipa1(""));

		/*
		 * Count times find end of sentence/paragraph.
		 */
begin:
		for (;;) {
			while (!endsent(pastatom))
				if (!lnext())
					goto ret;
			if (!pastatom || (wcursor == linebuf && endPS()))
				if (--cnt <= 0)
					break;
			if (linebuf[0] == 0) {
				do
					if (!lnext())
						goto ret;
				while (linebuf[0] == 0);
			} else
				if (!lnext())
					goto ret;
		}

		/*
		 * If going backwards, and didn't hit the end of the buffer,
		 * then reverse direction.
		 */
		if (dir < 0 && (wdot != llimit || wcursor != linebuf)) {
			dir = 1;
			llimit = dot;
			/*
			 * Empty line needs special treatement.
			 * If moved to it from other than begining of next line,
			 * then a sentence starts on next line.
			 */
			if (linebuf[0] == 0 && !pastatom && 
			   (wdot != dot - 1 || cursor != linebuf)) {
				lnext();
				goto ret;
			}
		}

		/*
		 * If we are not at a section/paragraph division,
		 * advance to next.
		 */
		if ((wcursor == icurs && wdot == idot) || wcursor != linebuf || !endPS())
			ignore(lskipa1(""));
	}
#ifdef LISPCODE
	else {
		c = *wcursor;
		/*
		 * Startup by skipping if at a ( going left or a ) going
		 * right to keep from getting stuck immediately.
		 */
		if ((dir < 0 && c == '(') || (dir > 0 && c == ')')) {
			if (!lnext()) {
				rc = -1;
				goto ret;
			}
		}
		/*
		 * Now chew up repitition count.  Each time around
		 * if at the beginning of an s-exp (going forwards)
		 * or the end of an s-exp (going backwards)
		 * skip the s-exp.  If not at beg/end resp, then stop
		 * if we hit a higher level paren, else skip an atom,
		 * counting it unless pastatom.
		 */
		while (cnt > 0) {
			c = *wcursor;
			if ((dir < 0 && c == ')') || (dir > 0 && c == '(')) {
				if (!lskipbal("()"))
					goto ret;
				/*
 				 * Unless this is the last time going
				 * backwards, skip past the matching paren
				 * so we don't think it is a higher level paren.
				 */
				if (dir < 0 && cnt == 1)
					goto ret;
				if (!lnext() || !ltosolid())
					goto ret;
				--cnt;
			} else if ((dir < 0 && c == '(') || (dir > 0 && c == ')'))
				/* Found a higher level paren */
				goto ret;
			else {
				if (!lskipatom())
					goto ret;
				if (!pastatom)
					--cnt;
			}
		}
	}
#endif
ret:
	strcLIN(save);
	free(save);
	return (rc);
}

/*
 * Is this the end of a sentence?
 */
int
endsent(bool pastatom)
{
	register char *cp = wcursor;
	register int c, d;

	(void)pastatom;
	/*
	 * If this is the beginning of a line, then
	 * check for the end of a paragraph or section.
	 */
	if (cp == linebuf)
		return (endPS());

	/*
	 * Sentences end with . ! ? not at the beginning
	 * of the line, and must be either at the end of the line,
	 * or followed by 2 spaces.  Any number of intervening ) ] ' "
	 * characters are allowed.
	 */
	if (!any(c = *cp, ".!?"))
		goto tryps;
	do
		if ((d = *++cp) == 0)
			return (1);
	while (any(d, ")]'"));
	if (*cp == 0 || (*cp++ == ' ' && *cp == ' '))
		return (1);
tryps:
	if (cp[1] == 0)
		return (endPS());
	return (0);
}

/*
 * End of paragraphs/sections are respective
 * macros as well as blank lines and form feeds.
 */
int
endPS(void)
{

	return (linebuf[0] == 0 ||
		isa(svalue(PARAGRAPHS)) || isa(svalue(SECTIONS)));
	    
}

#ifdef LISPCODE
int
lindent(line *addr)
{
	register int i;
	char *swcurs = wcursor;
	line *swdot = wdot;

again:
	if (addr > one) {
		register char *cp;
		register int cnt = 0;

		addr--;
		getline(*addr);
		for (cp = linebuf; *cp; cp++)
			if (*cp == '(')
				cnt++;
			else if (*cp == ')')
				cnt--;
		cp = vpastwh(linebuf);
		if (*cp == 0)
			goto again;
		if (cnt == 0)
			return (whitecnt(linebuf));
		addr++;
	}
	wcursor = linebuf;
	linebuf[0] = 0;
	wdot = addr;
	dir = -1;
	llimit = one;
	lf = (void (*)(int))lindent;
	if (!lskipbal("()"))
		i = 0;
	else if (wcursor == linebuf)
		i = 2;
	else {
		register char *wp = wcursor;

		dir = 1;
		llimit = wdot;
		if (!lnext() || !ltosolid() || !lskipatom()) {
			wcursor = wp;
			i = 1;
		} else
			i = 0;
		i += column(wcursor) - 1;
		if (!inopen)
			i--;
	}
	wdot = swdot;
	wcursor = swcurs;
	return (i);
}
#endif

int
lmatchp(line *addr)
{
	register int i;
	register char *parens, *cp;

	for (cp = cursor; !any(*cp, "({[)}]");)
		if (*cp++ == 0)
			return (0);
	lf = 0;
	parens = any(*cp, "()") ? "()" : any(*cp, "[]") ? "[]" : "{}";
	if (*cp == parens[1]) {
		dir = -1;
		llimit = one;
	} else {
		dir = 1;
		llimit = dol;
	}
	if (addr)
		llimit = addr;
	if (splitw)
		llimit = dot;
	wcursor = cp;
	wdot = dot;
	i = lskipbal(parens);
	return (i);
}

void
lsmatch(char *cp)
{
	char *save = smalloc(LBSIZE);
	register char *sp = save;
	register char *scurs = cursor;

	wcursor = cp;
	lcpy(sp, linebuf, LBSIZE);
	*wcursor = 0;
	strcpy(cursor, genbuf);
	cursor = strend(linebuf) - 1;
	if (lmatchp(dot - vcline)) {
		register int i = insmode;
		register int c = outcol;
		register int l = outline;

		if (!MI)
			endim();
		vgoto(splitw ? WECHO : LINE(wdot - llimit), column(wcursor) - 1);
		flush();
		sleep(1);
		vgoto(l, c);
		if (i)
			goim();
	}
	else {
		strcLIN(sp);
		strcpy(scurs, genbuf);
		if (!lmatchp((line *) 0))
			beep();
	}
	strcLIN(sp);
	wdot = 0;
	wcursor = 0;
	cursor = scurs;
	free(save);
}

int
ltosolid(void)
{

	return (ltosol1("()"));
}

int
ltosol1(register char *parens)
{
	register char *cp;

	if (*parens && !*wcursor && !lnext())
		return (0);
	while (isspace(*wcursor&0377) || (*wcursor == 0 && *parens))
		if (!lnext())
			return (0);
	if (any(*wcursor, parens) || dir > 0)
		return (1);
	for (cp = wcursor; cp > linebuf; cp--)
		if (isspace(cp[-1]&0377) || any(cp[-1], parens))
			break;
	wcursor = cp;
	return (1);
}

int
lskipbal(register char *parens)
{
	register int level = dir;
	register int c;

	do {
		if (!lnext()) {
			wdot = NOLINE;
			return (0);
		}
		c = *wcursor;
		if (c == parens[1])
			level--;
		else if (c == parens[0])
			level++;
	} while (level);
	return (1);
}

int
lskipatom(void)
{

	return (lskipa1("()"));
}

int
lskipa1(register char *parens)
{
	register int c;

	for (;;) {
		if (dir < 0 && wcursor == linebuf) {
			if (!lnext())
				return (0);
			break;
		}
		c = *wcursor;
		if (c && (isspace(c) || any(c, parens)))
			break;
		if (!lnext())
			return (0);
		if (dir > 0 && wcursor == linebuf)
			break;
	}
	return (ltosol1(parens));
}

int
lnext(void)
{

	if (dir > 0) {
		if (*wcursor)
			wcursor += skipright(linebuf, wcursor);
		if (*wcursor)
			return (1);
		if (wdot >= llimit) {
			if (lf == vmove && wcursor > linebuf)
				wcursor += skipleft(linebuf, wcursor);
			return (0);
		}
		wdot++;
		getline(*wdot);
		wcursor = linebuf;
		return (1);
	} else {
		wcursor += skipleft(linebuf, wcursor);
		if (wcursor >= linebuf)
			return (1);
#ifdef LISPCODE
		if (lf == (void (*)(int))lindent && linebuf[0] == '(')
			llimit = wdot;
#endif
		if (wdot <= llimit) {
			wcursor = linebuf;
			return (0);
		}
		wdot--;
		getline(*wdot);
		wcursor = linebuf[0] == 0 ? linebuf : strend(linebuf) - 1;
		return (1);
	}
}

int
lbrack(register int c, void (*f)(int))
{
	register line *addr;

	addr = dot;
	for (;;) {
		addr += dir;
		if (addr < one || addr > dol) {
			addr -= dir;
			break;
		}
		getline(*addr);
		if (linebuf[0] == '{' ||
#ifdef LISPCODE
		    (value(LISP) && linebuf[0] == '(') ||
#endif
		    isa(svalue(SECTIONS))) {
			if (c == ']' && f != vmove) {
				addr--;
				getline(*addr);
			}
			break;
		}
		if (c == ']' && f != vmove && linebuf[0] == '}')
			break;
	}
	if (addr == dot)
		return (0);
	if (f != vmove)
		wcursor = c == ']' ? strend(linebuf) : linebuf;
	else
		wcursor = 0;
	wdot = addr;
	vmoving = 0;
	return (1);
}

int
isa(register char *cp)
{

	if (linebuf[0] != '.')
		return (0);
	for (; cp[0] && cp[1]; cp += 2)
		if (linebuf[1] == cp[0]) {
			if (linebuf[2] == cp[1])
				return (1);
			if (linebuf[2] == 0 && cp[1] == ' ')
				return (1);
		}
	return (0);
}

static void
cswitch(char *dst, int *dn, const char *src, int *sn)
{
	int	c;

#ifdef	MB
	if (mb_cur_max > 1) {
		nextc(c, src, *sn);
		if (c & INVBIT) {
			*dst = *src;
			*dn = *sn = 1;
		} else {
			if (iswupper(c))
				c = towlower(c);
			else if (iswlower(c))
				c = towupper(c);
			if ((*dn = wctomb(dst, c)) > *sn) {
				*dst = *src;
				*dn = *sn = 1;
			}
		}
	} else
#endif	/* MB */
	{
		c = *src & 0377;
		if (isupper(c))
			*dst = tolower(c);
		else if (islower(c))
			*dst = toupper(c);
		else
			*dst = c;
		*dn = *sn = 1;
	}
}

void
vswitch(int cnt)
{
	if (cnt <= 1) {
		char mbuf[MB_LEN_MAX+4];
		int	n0, n1;
		setLAST();
		mbuf[0] = 'r';
		cswitch(&mbuf[1], &n1, cursor, &n0);
		if (cursor[n1] != '\0')
			mbuf[1+n1++] = ' ';
		mbuf[1+n1] = '\0';
		macpush(mbuf, 1);
	} else {	/* cnt > 1 */
		char *mbuf = smalloc(MAXDIGS + cnt*(mb_cur_max+1) + 5);
		register char *p = &mbuf[MAXDIGS + 1];
		int num, n0, n1, m;

		setLAST();
		*p++ = 's';
		for (num = 0, m = 0; num < cnt && cursor[m] != '\0'; num++) {
			*p++ = CTRL('v');
			cswitch(p, &n1, &cursor[m], &n0);
			p += n1;
			m += n0;
		}
		*p++ = ESCAPE;
		if (cursor[m])
			*p++ = ' ';
		*p++ = '\0';
		macpush(p_dconv((long)num, mbuf), 1);
		lastvgk = 0;
		free(mbuf);
	}
}

#ifdef	MB
int
wskipleft(char *lp, char *pos)
{
	int	c, n;

	do {
		nextc(c, lp, n);
		lp += n;
	} while (lp < pos);
	return -n;
}

int
wskipright(char *line, char *pos)
{
	int	c, n;

	(void)line;
	nextc(c, pos, n);
	return n;
}

int
wsamechar(char *cp, int d)
{
	int	c;

	if (mbtowi(&c, cp, mb_cur_max) >= 0 && c == d)
		return 1;
	return 0;
}
#endif	/* MB */
