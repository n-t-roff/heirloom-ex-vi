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
static char sccsid[] = "@(#)ex_re.c	1.60 (gritter) 8/6/05";
#endif
#endif

/* from ex_re.c	7.5 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_re.h"

#ifdef	UXRE

#include <regex.h>

char	*braslist[NBRA];
char	*braelist[NBRA];
char	*loc1;
char	*loc2;

#else	/* !UXRE */
static int	regerrno;

#define	INIT			register char *sp = instring;
#define	GETC()			(*sp++)
#define	PEEKC()			(*sp)
#define	UNGETC(c)		(--sp)
#define	RETURN(c)		return(ep);
#define	ERROR(c)		{ regerrno = c; return 0; }

#define	compile(a, b, c, d)	_compile(a, b, c, d)
#define	regexp_h_static		static

#ifndef	NO_BE_BACKSLASH
#define	REGEXP_H_VI_BACKSLASH
#endif	/* !NO_BE_BACKSLASH */

#ifdef	MB
#define	REGEXP_H_WCHARS
#endif	/* MB */

#define	REGEXP_H_USED_FROM_VI

#include "regexp.h"

#ifndef	REG_ICASE
#define	REG_ICASE	1
#endif

static size_t
loconv(register char *dst, register const char *src)
{
	char	*odst = dst;

#ifdef	MB
	if (mb_cur_max > 1) {
		char	mb[MB_LEN_MAX];
		wchar_t wc;
		int len, i, nlen;

		for (;;) {
			if ((*src & 0200) == 0) {
				*dst++ = tolower(*src);
				if (*src++ == '\0')
					break;
			} else if ((len = mbtowc(&wc, src, mb_cur_max)) <= 0) {
				*dst++ = *src++;
			} else {
				wc = towlower(wc);
				if (len >= mb_cur_max) {
					if ((nlen = wctomb(dst, wc)) <= len) {
						dst += nlen;
						src += len;
					} else {
						*dst++ = *src++;
					}
				} else {
					if ((nlen = wctomb(mb, wc)) <= len) {
						src += len;
						for (i = 0; i < nlen; i++)
							*dst++ = mb[i];
					} else {
						*dst++ = *src++;
					}
				}
			}
		}
	} else
#endif	/* MB */
	{
		do
			*dst++ = tolower(*src & 0377);
		while (*src++);
	}
	return dst - odst;
}

#undef	compile

#endif	/* !UXRE */

/*
 * Global, substitute and regular expressions.
 * Very similar to ed, with some re extensions and
 * confirmed substitute.
 */
void 
global(int k)
{
	register char *gp;
	register int c, i;
	register line *a1;
	char	mb[MB_LEN_MAX+1];
	char globuf[GBSIZE], *Cwas;
	int lines = lineDOL();
	int oinglobal = inglobal;
	char *oglobp = globp;

	Cwas = Command;
	/*
	 * States of inglobal:
	 *  0: ordinary - not in a global command.
	 *  1: text coming from some buffer, not tty.
	 *  2: like 1, but the source of the buffer is a global command.
	 * Hence you're only in a global command if inglobal==2. This
	 * strange sounding convention is historically derived from
	 * everybody simulating a global command.
	 */
	if (inglobal==2)
		error(catgets(catd, 1, 121,
				"Global within global@not allowed"));
	markDOT();
	setall();
	nonzero();
	if (skipend())
		error(catgets(catd, 1, 122,
		"Global needs re|Missing regular expression for global"));
	c = GETWC(mb);
	ignore(compile(c, 1));
	savere(&scanre);
	gp = globuf;
	while ((c = GETWC(mb)) != '\n') {
		switch (c) {

		case EOF:
			c = '\n';
			goto brkwh;

		case '\\':
			c = GETWC(mb);
			switch (c) {

			case '\\':
				ungetchar(c);
				break;

			case '\n':
				break;

			default:
				*gp++ = '\\';
				break;
			}
			break;
		}
		for (i = 0; mb[i]; i++) {
			*gp++ = mb[i];
			if (gp >= &globuf[GBSIZE - 2])
				error(catgets(catd, 1, 123,
						"Global command too long"));
		}
	}
brkwh:
	ungetchar(c);
/* out: */
	newline();
	*gp++ = c;
	*gp++ = 0;
	saveall();
	inglobal = 2;
	for (a1 = one; a1 <= dol; a1++) {
		*a1 &= ~01;
		if (a1 >= addr1 && a1 <= addr2 && execute(0, a1) == k)
			*a1 |= 01;
	}
#ifdef notdef
/*
 * This code is commented out for now.  The problem is that we don't
 * fix up the undo area the way we should.  Basically, I think what has
 * to be done is to copy the undo area down (since we shrunk everything)
 * and move the various pointers into it down too.  I will do this later
 * when I have time. (Mark, 10-20-80)
 */
	/*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
	if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
#endif
	if (inopen)
		inopen = -1;
	/*
	 * Now for each marked line, set dot there and do the commands.
	 * Note the n^2 behavior here for lots of lines matching.
	 * This is really needed: in some cases you could delete lines,
	 * causing a marked line to be moved before a1 and missed if
	 * we didn't restart at zero each time.
	 */
	for (a1 = one; a1 <= dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands(1, 1);
			a1 = zero;
		}
	}
	globp = oglobp;
	inglobal = oinglobal;
	endline = 1;
	Command = Cwas;
	netchHAD(lines);
	setlastchar(EOF);
	if (inopen) {
		ungetchar(EOF);
		inopen = 1;
	}
}

/*
 * gdelete: delete inside a global command. Handles the
 * special case g/r.e./d. All lines to be deleted have
 * already been marked. Squeeze the remaining lines together.
 * Note that other cases such as g/r.e./p, g/r.e./s/r.e.2/rhs/,
 * and g/r.e./.,/r.e.2/d are not treated specially.  There is no
 * good reason for this except the question: where to you draw the line?
 */
void 
gdelete(void)
{
	register line *a1, *a2, *a3;

	a3 = dol;
	/* find first marked line. can skip all before it */
	for (a1=zero; (*a1&01)==0; a1++)
		if (a1>=a3)
			return;
	/* copy down unmarked lines, compacting as we go. */
	for (a2=a1+1; a2<=a3;) {
		if (*a2&01) {
			a2++;		/* line is marked, skip it */
			dot = a1;	/* dot left after line deletion */
		} else
			*a1++ = *a2++;	/* unmarked, copy it */
	}
	dol = a1-1;
	if (dot>dol)
		dot = dol;
	change();
}

bool	cflag;
int	scount, slines, stotal;

int 
substitute(int c)
{
	register line *addr;
	register int n;
	int gsubf, hopcount;

	gsubf = compsub(c);
	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	stotal = 0;
	slines = 0;
	for (addr = addr1; addr <= addr2; addr++) {
		scount = hopcount = 0;
		if (dosubcon(0, addr) == 0)
			continue;
		if (gsubf) {
			/*
			 * The loop can happen from s/\</&/g
			 * but we don't want to break other, reasonable cases.
			 */
			while (*loc2) {
				if (++hopcount > LBSIZE)
					error(catgets(catd, 1, 124,
							"substitution loop"));
				if (dosubcon(1, addr) == 0)
					break;
			}
		}
		if (scount) {
			stotal += scount;
			slines++;
			putmark(addr);
			n = append(getsub, addr);
			addr += n;
			addr2 += n;
		}
	}
	if (stotal == 0 && !inglobal && !cflag)
		error(catgets(catd, 1, 125,
				"Fail|Substitute pattern match failed"));
	snote(stotal, slines);
	return (stotal);
}

int 
compsub(int ch)
{
	register int seof, c, uselastre;
	char	mb[MB_LEN_MAX+1];
	static int gsubf;

	if (!value(EDCOMPATIBLE))
		gsubf = cflag = 0;
	uselastre = 0;
	switch (ch) {

	case 's':
		ignore(skipwh());
		seof = GETWC(mb);
		if (endcmd(seof) || any(seof, "gcr")) {
			ungetchar(seof);
			goto redo;
		}
		if (xisalnum(seof))
			error(catgets(catd, 1, 126,
	"Substitute needs re|Missing regular expression for substitute"));
		seof = compile(seof, 1);
		uselastre = 1;
		comprhs(seof);
		gsubf = 0;
		cflag = 0;
		break;

	case '~':
		uselastre = 1;
		/* fall into ... */
	case '&':
	redo:
		if (re.Patbuf == NULL || re.Patbuf[0] == 0)
			error(catgets(catd, 1, 127,
			"No previous re|No previous regular expression"));
		if (subre.Patbuf == NULL || subre.Patbuf[0] == 0)
			error(catgets(catd, 1, 128,
	"No previous substitute re|No previous substitute to repeat"));
		break;
	}
	for (;;) {
		c = getchar();
		switch (c) {

		case 'g':
			gsubf = !gsubf;
			continue;

		case 'c':
			cflag = !cflag;
			continue;

		case 'r':
			uselastre = 1;
			continue;

		default:
			ungetchar(c);
			setcount();
			newline();
			if (uselastre)
				savere(&subre);
			else
				resre(&subre);
			return (gsubf);
		}
	}
}

void
comprhs(int seof)
{
	register char *rp, *orp;
	char	mb[MB_LEN_MAX+1];
#ifdef	BIT8
	char *qp, *oqp;
#endif
	register int c, i;
#ifdef	BIT8
	int q;
#endif
	char orhsbuf[RHSSIZE];
#ifdef	BIT8
	char orhsquo[RHSSIZE];
#endif
	int	hashflag = 0;

	rp = rhsbuf;
#ifdef	BIT8
	qp = rhsquo;
#endif
	lcpy(orhsbuf, rp, sizeof orhsbuf);
#ifdef	BIT8
	copy(orhsquo, qp, (size_t) strlen(rp));
#endif
	for (;;) {
		c = GETWC(mb);
#ifdef	BIT8
		q = 0;
#endif
		if (c == seof)
			break;
		switch (c) {

		case '%':
			if (rp == rhsbuf)
				hashflag = 1;
			break;

		case '\\':
			c = GETWC(mb);
			if (c == EOF) {
				ungetchar(c);
				break;
			}
			if (value(MAGIC)) {
				/*
				 * When "magic", \& turns into a plain &,
				 * and all other chars work fine quoted.
				 */
				if (c != '&')
#ifndef	BIT8
					c |= QUOTE;
#else
					q = 1;
#endif
				break;
			}
magic:
			if (c == '~') {
hash:
#ifndef	BIT8
				for (orp = orhsbuf; *orp; *rp++ = *orp++) {
#else
				for (orp = orhsbuf, oqp = orhsquo;
						*orp; *rp++ = *orp++) {
					*qp++ = *oqp++;
#endif
					if (rp >= &rhsbuf[RHSSIZE - 1])
						goto toobig;
				}
				if (hashflag & 2)
					goto endrhs;
				continue;
			}
#ifndef	BIT8
			c |= QUOTE;
#else
			q = 1;
#endif
			break;

		case '\n':
		case EOF:
			if (!(globp && globp[0])) {
				ungetchar(c);
				goto endrhs;
			}

		case '~':
		case '&':
			if (value(MAGIC))
				goto magic;
			break;
		}
		if (rp >= &rhsbuf[RHSSIZE - 1]) {
toobig:
			*rp = 0;
			error(catgets(catd, 1, 129,
		"Replacement pattern too long@- limit 256 characters"));
		}
		for (i = 0; mb[i]; i++) {
			*rp++ = mb[i];
#ifdef	BIT8
			*qp++ = q;
#endif
		}
	}
endrhs:
	if (hashflag == 1 && rhsbuf[0] == '%' && rp == &rhsbuf[1]) {
		rp = rhsbuf;
		hashflag |= 2;
		goto hash;
	}
	*rp++ = 0;
}

int
getsub(void)
{
	register char *p;

	if ((p = linebp) == 0)
		return (EOF);
	strcLIN(p);
	linebp = 0;
	return (0);
}

int
dosubcon(bool f, line *a)
{

	if (execute(f, a) == 0)
		return (0);
	if (confirmed(a)) {
		dosub();
		scount++;
	}
	return (1);
}

int
confirmed(line *a)
{
	register int c;
	char *yesstr = catgets(catd, 1, 249, "y");
	int okay = -1;

	if (cflag == 0)
		return (1);
	pofix();
	pline(lineno(a), -1);
	if (inopen)
		putchar('\n' | QUOTE);
	c = column(loc1 - 1);
	ugo(c - 1 + (inopen ? 1 : 0), ' ');
	ugo(column(loc2 - 1) - c, '^');
	flush();
	c = getkey();
again:
	if (c == '\r')
		c = '\n';
	if (inopen)
		putchar(c), flush();
	if (c != '\n' && c != EOF) {
		if (okay && *yesstr) {
			if (c == (*yesstr++ & 0377))
				okay = 1;
			else
				okay = 0;
		}
		c = getkey();
		goto again;
	}
	noteinp();
	return (okay > 0);
}

#ifdef	notdef
int
ex_getch(void)
{
	char c;

	if (read(2, &c, 1) != 1)
		return (EOF);
#ifndef	BIT8
	return (c & TRIM);
#else
	return c;
#endif
}
#endif	/* notdef */

void
ugo(int cnt, int with)
{

	if (cnt > 0)
		do
			putchar(with);
		while (--cnt > 0);
}

int	casecnt;
bool	destuc;

void
dosub(void)
{
	register char *lp, *sp, *rp;
	int c, n;
#ifdef	BIT8
	register char *qp;
	int q;
#endif

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
#ifdef	BIT8
	qp = rhsquo;
#endif
	while (lp < loc1)
		*sp++ = *lp++;
	casecnt = 0;
	while (*rp) {
		nextc(c, rp, n);
		rp += n;
#ifdef	BIT8
		c &= TRIM;
		q = *qp;
		qp += n;
#endif
		/* ^V <return> from vi to split lines */
		if (c == '\r')
			c = '\n';

#ifndef	BIT8
		if (c & QUOTE)
			switch (c & TRIM) {
#else
		if (q)
			switch (c) {
#endif

			case '&':
				sp = place(sp, loc1, loc2);
				if (sp == 0)
					goto ovflo;
				continue;

			case 'l':
				casecnt = 1;
				destuc = 0;
				continue;

			case 'L':
				casecnt = LBSIZE;
				destuc = 0;
				continue;

			case 'u':
				casecnt = 1;
				destuc = 1;
				continue;

			case 'U':
				casecnt = LBSIZE;
				destuc = 1;
				continue;

			case 'E':
			case 'e':
				casecnt = 0;
				continue;
			}
#ifndef	BIT8
		if (c < 0 && (c &= TRIM) >= '1' && c < re.Nbra + '1') {
#else
		if (q && c >= '1' && c < re.Nbra + '1') {
#endif
			sp = place(sp, braslist[c - '1'], braelist[c - '1']);
			if (sp == 0)
				goto ovflo;
			continue;
		}
#ifdef	MB
		if (mb_cur_max > 1) {
			char	mb[MB_LEN_MAX+1];
			int	i, m;
			if (casecnt)
				c = fixcase(c & TRIM);
			if (c & INVBIT || (m = wctomb(mb, c)) <= 0) {
				mb[0] = rp[-n];
				m = 1;
			}
			for (i = 0; i < m; i++) {
				*sp++ = mb[i];
				if (sp >= &genbuf[LBSIZE])
					goto ovflo;
			}
		} else
#endif	/* MB */
		{
			if (casecnt)
				*sp++ = fixcase(c & TRIM);
			else
				*sp++ = c & TRIM;
		}
		if (sp >= &genbuf[LBSIZE])
ovflo:
			error(catgets(catd, 1, 130,
					"Line overflow@in substitute"));
	}
	lp = loc2;
	loc2 = sp + (linebuf - genbuf);
#ifdef	UXRE
	if (loc1 == lp) {
		nextc(c, loc2, n);
		loc2 += n;
	}
#endif	/* UXRE */
	while ((*sp++ = *lp++))
		if (sp >= &genbuf[LBSIZE])
			goto ovflo;
	strcLIN(genbuf);
}

int
fixcase(register int c)
{

	if (casecnt == 0)
		return (c);
	casecnt--;
#ifdef	MB
	if (c & INVBIT)
		return (c);
	if (mb_cur_max > 1) {
		if (destuc) {
			if (iswlower(c))
				c = towupper(c);
		} else
			if (iswupper(c))
				c = towlower(c);
	} else
#endif	/* MB */
	{
		if (destuc) {
			if (islower(c))
				c = toupper(c);
		} else
			if (isupper(c))
				c = tolower(c);
	}
	return (c);
}

char *
place(register char *sp, register char *l1, register char *l2)
{
	while (l1 < l2) {
#ifdef	MB
		if (mb_cur_max > 1) {
			char	mb[MB_LEN_MAX+1];
			int	c, i, m, n;

			nextc(c, l1, m);
			if (c & INVBIT) {
				m = n = 1;
				*mb = *l1;
			} else {
				c = fixcase(c);
				if ((n = wctomb(mb, c)) <= 0) {
					n = 1;
					*mb = *l1;
				}
			}
			l1 += m;
			for (i = 0; i < n; i++) {
				*sp++ = mb[i];
				if (sp >= &genbuf[LBSIZE])
					return (0);
			}
		} else
#endif	/* MB */
		{
			*sp++ = fixcase(*l1++);
			if (sp >= &genbuf[LBSIZE])
				return (0);
		}
	}
	return (sp);
}

void
snote(register int total, register int lines)
{

	if (!notable(total))
		return;
	ex_printf(mesg(catgets(catd, 1, 131, "%d subs|%d substitutions")), total);
	if (lines != 1 && lines != total)
		ex_printf(catgets(catd, 1, 132, " on %d lines"), lines);
	noonl();
	flush();
}

void
cerror(char *s)
{
	if (re.Patbuf != NULL)
		re.Patbuf[0] = '\0';
	error(s);
}

void
refree(struct regexp *rp)
{
	struct regexp *r1 = NULL, *r2 = NULL;
	
	if (rp->Expbuf == 0)
		return;
	if (rp == &re) {
		r1 = &scanre;
		r2 = &subre;
	} else if (rp == &scanre) {
		r1 = &re;
		r2 = &subre;
	} else if (rp == &subre) {
		r1 = &re;
		r2 = &scanre;
	}
	if ((r1->Expbuf == 0 || rp->Re_ident != r1->Re_ident) &&
			(r2->Expbuf == 0 || rp->Re_ident != r2->Re_ident)) {
#ifdef	UXRE
		regfree(rp->Expbuf);
#endif	/* UXRE */
		free(rp->Expbuf);
	}
	rp->Expbuf = 0;
}

struct regexp *
savere(struct regexp *store)
{
	refree(store);
	copy(store, &re, sizeof re);
	return store;
}

struct regexp *
resre(struct regexp *store)
{
	refree(&re);
	copy(&re, store, sizeof re);
	return store;
}

static void
compile1(void)
{
#ifdef	UXRE
	int	n;
#else	/* !UXRE */
	char	*r;
	char	*p;
#endif	/* !UXRE */

	refree(&re);
	re.Flags = value(IGNORECASE) ? REG_ICASE : 0;
#ifdef	UXRE
	re.Flags |= REG_ANGLES;
#ifndef	NO_BE_BACKSLASH
	re.Flags |= REG_BKTESCAPE | REG_BADRANGE;
#endif	/* !NO_BE_BACKSLASH */
	if (re.Expbuf == NULL)
		re.Expbuf = calloc(1, sizeof (regex_t));
	if ((n = regcomp(re.Expbuf, re.Patbuf, re.Flags)) != 0) {
		switch (n) {
		case REG_EBRACK:
			free(re.Expbuf);
			re.Expbuf = 0;
			cerror(catgets(catd, 1, 154, "Missing ]"));
			/*NOTREACHED*/
			break;
		default:
			regerror(n, re.Expbuf, &re.Patbuf[1],
					sizeof re.Patbuf - 1);
			free(re.Expbuf);
			re.Expbuf = 0;
			cerror(&re.Patbuf[1]);
		}
	}
	if ((re.Nbra = ((regex_t *)re.Expbuf)->re_nsub) > NBRA)
		re.Nbra = NBRA;
#else	/* !UXRE */
	if ((re.Expbuf = malloc(re.Length)) == NULL)
		cerror("Re too complex|Regular expression too complicated");
	if (re.Flags & REG_ICASE) {
		p = malloc(strlen(re.Patbuf) + 1);
		loconv(p, re.Patbuf);
	} else
		p = re.Patbuf;
	r = _compile(p, re.Expbuf, &((char *)re.Expbuf)[re.Length], '\0');
	if (p != re.Patbuf)
		free(p);
	if (r == 0) {
		char	*cp;
		free(re.Expbuf);
		re.Expbuf = 0;
		switch (regerrno) {
		case 11:
			cp = "Range endpoint too large|Range endpoint "
					"too large in regular expression";
			break;
		case 16:
			cp = "Bad number|Bad number in regular expression";
			break;
		case 25:
			cp = "\"\\digit\" out of range";
			break;
		case 36:
			cp = "Badly formed re|Missing closing delimiter "
				"for regular expression";
			break;
		case 42:
			cp = "\\( \\) Imbalance";
			break;
		case 43:
			cp = "Awash in \\('s!|Too many \\('d subexressions "
				"in a regular expression";
			break;
		case 44:
			cp = "More than 2 numbers given in \\{~\\}";
			break;
		case 45:
			cp = "} expected after \\";
			break;
		case 46:
			cp = "First number exceeds second in \\{~\\}";
			break;
		case 49:
			cp = "Missing ]";
			break;
		case 67:
			cp = "Illegal byte sequence|Regular expression "
				"has illegal byte sequence";
			break;
		default:
			cp = "Unknown regexp error code!!";
		}
		cerror(cp);
	}
	re.Circfl = circf;
	re.Nbra = nbra;
#endif	/* !UXRE */
	re.Re_ident++;
}

int
compile(int eof, int oknl)
{
	int c, d, i, n = 0;
	char	mb[MB_LEN_MAX+1];
	char *p, *end;
	int nomagic = value(MAGIC) ? 0 : 1, esc, rcnt = 0;
	char *rhsp;
#ifdef	BIT8
	char *rhsq;
#endif

	free(re.Patbuf);
	re.Patbuf = smalloc(2*LBSIZE + 1);
	p = re.Patbuf;
	end = &re.Patbuf[2*LBSIZE + 1];
	if (isalpha(eof) || isdigit(eof))
		error(catgets(catd, 1, 133,
	"Regular expressions cannot be delimited by letters or digits"));
	c = GETWC(mb);
	if (eof == '\\') {
		switch (c) {
		case '/':
		case '?':
			if (scanre.Patbuf[0] == 0)
				error(catgets(catd, 1, 134,
	"No previous scan re|No previous scanning regular expression"));
			resre(&scanre);
			return c;
		case '&':
			if (subre.Patbuf[0] == 0)
				error(catgets(catd, 1, 135,
	"No previous substitute re|No previous substitute regular expression"));
			resre(&subre);
			return c;
		default:
			error(catgets(catd, 1, 136,
	"Badly formed re|Regular expression \\ must be followed by / or ?"));
		}
	}
	if (c == eof || c == '\n' || c == EOF) {
		if (c == '\n' && oknl == 0)
			error(catgets(catd, 1, 138,
			"Missing closing delimiter@for regular expression"));
		if (c != eof)
			ungetchar(c);
		if (re.Expbuf == 0)
			error(catgets(catd, 1, 137,
			"No previous re|No previous regular expression"));
		return eof;
	}
	re.Nbra = re.Circfl = 0;
	if (c == '^')
		re.Circfl++;
	esc = 0;
	goto havec;
	/*
	 * Fetch the search pattern. This is quite a mess since we have
	 * to handle nomagic and ~.
	 */
	for (;;) {
		esc = 0;
		c = GETWC(mb);
	havec:	if (c == eof || c == EOF) {
			if (c == EOF)
				ungetchar(c);
			break;
		} else if (c == '\n') {
			if (!oknl)
				cerror(catgets(catd, 1, 157,
	"Badly formed re|Missing closing delimiter for regular expression"));
			ungetchar(c);
			break;
		} else if (nomagic) {
			switch (c) {
			case '.':
			case '*':
			case '[':
			case '~':
				*p++ = '\\';
				esc = 1;
				break;
			case '\\':
				c = GETWC(mb);
				if (c != '.' && c != '*' && c != '[' &&
						c != '~') {
					*p++ = '\\';
					esc = 1;
				}
			}
		} else if (c == '\\') {
			c = GETWC(mb);
			if (c != '~')
				*p++ = '\\';
			esc = 1;
		}
		if (c == EOF) {
			ungetchar(c);
			break;
		}
		if (!esc && c == '~') {
			rhsp = rhsbuf;
#ifdef	BIT8
			rhsq = rhsquo;
#endif
			while (*rhsp) {
#ifndef	BIT8
				if (*rhsp & QUOTE) {
					nextc(c, rhsp, n);
					c &= TRIM;
#else	/* BIT8 */
				if (*rhsq) {
					nextc(c, rhsp, n);
#endif	/* BIT8 */
					if (c == '&')
						error(catgets(catd, 1, 149,
			"Replacement pattern contains &@- cannot use in re"));
					if (c >= '1' && c <= '9')
						error(catgets(catd, 1, 150,
			"Replacement pattern contains \\d@- cannot use in re"));
				}
				if (p >= end - 3)
					goto complex;
				if (*rhsp == '\\' || *rhsp == '[' ||
						*rhsp == '.' ||
						*rhsp == '^' ||
						*rhsp == '*' ||
						*rhsp == '$')
					*p++ = '\\';
#ifdef	BIT8
				nextc(c, rhsp, n);
				for (i = 0; i < n; i++) {
					*p++ = *rhsp++;
					rhsq++;
				}
#else
				*p++ = *rhsp++ & TRIM;
#endif
			}
		} else if (!esc && c == '[') {
			rcnt++;
			/*
			 * Search for the end of the bracket expression
			 * since '~' may not be recognized inside.
			 */
			*p++ = (char)c;
			if (p >= end)
				goto complex;
			d = EOF;
			do {
				c = GETWC(mb);
				if (c == '\n' || c == EOF)
					cerror("Missing ]");
				for (i = 0; mb[i]; i++) {
					*p++ = mb[i];
					if (p >= end)
						goto complex;
				}
#ifdef	UXRE
				if (d == '[' && (c == ':' || c == '.' ||
							c == '=')) {
					d = c;
					do {
						c = GETWC(mb);
						if (c == '\n' || c == EOF)
							cerror("Missing ]");
						for (i = 0; mb[i]; i++) {
							*p++ = mb[i];
							if (p >= end)
								goto complex;
						}
					} while (c != d || peekchar() != ']');
					c = GETWC(mb);
					for (i = 0; mb[i]; i++) {
						*p++ = mb[i];
						if (p >= end)
							goto complex;
					}
					c = EOF; /* -> reset d and continue */
				}
#endif	/* UXRE */
				d = c;
			} while (c != ']');
		} else if (esc && c == '{') {
			/*
			 * Search for the end of the interval expression
			 * since '~' may not be recognized inside.
			 */
			for (i = 0; mb[i]; i++) {
				*p++ = mb[i];
				if (p >= end)
					goto complex;
			}
			do {
				c = GETWC(mb);
				if (c == '\n' || c == EOF)
					cerror(catgets(catd, 1, 143,
			"Bad number|Bad number in regular expression"));
				for (i = 0; mb[i]; i++) {
					*p++ = mb[i];
					if (p >= end)
						goto complex;
				}
			} while (c != '\\');
			c = GETWC(mb);
			if (c != '}')
				cerror(catgets(catd, 1, 146,
					"} expected after \\"));
			*p++ = (char)c;
		} else {
			for (i = 0; mb[i]; i++) {
				*p++ = mb[i];
				if (p >= end)
					goto complex;
			}
		}
		if (p >= end)
complex:		cerror(catgets(catd, 1, 139,
			"Re too complex|Regular expression too complicated"));
	}
	if (p == re.Patbuf)
		*p++ = '.';	/* approximate historical behavior */
	*p = '\0';
	re.Length = rcnt*32 + 2*(p-re.Patbuf) + 5;
	compile1();
	return eof;
}

#ifdef	UXRE
int
execute(int gf, line *addr)
{
	char *p;
	int c;
	int eflags = 0, nsub;
	regmatch_t bralist[NBRA + 1];

	if (gf) {
		if (re.Circfl)
			return 0;
		eflags |= REG_NOTBOL;
		p = loc2;
	} else {
		if (addr == zero)
			return 0;
		if ((value(IGNORECASE) ? 1:0) ^ (re.Flags & REG_ICASE ? 1:0))
			compile1();
		p = linebuf;
		getline(*addr);
	}
	/*
	 * Need subexpression matches only for substitute command,
	 * so don't fetch them otherwise (enables use of DFA).
	 */
	nsub = (re.Re_ident == subre.Re_ident ? NBRA : 0);
	switch (regexec(re.Expbuf, p, nsub + 1, bralist, eflags)) {
	case 0:
		break;
	case REG_NOMATCH:
		return 0;
	default:
		cerror(catgets(catd, 1, 139,
			"Re too complex|Regular expression too complicated"));
	}
	loc1 = p + bralist[0].rm_so;
	loc2 = p + bralist[0].rm_eo;
	for (c = 0; c < nsub; c++) {
		if (bralist[c + 1].rm_so != -1) {
			braslist[c] = p + bralist[c + 1].rm_so;
			braelist[c] = p + bralist[c + 1].rm_eo;
		} else
			braslist[c] = braelist[c] = NULL;
	}
	return 1;
}
#else	/* !UXRE */
int
execute(int gf, line *addr)
{
	char *p;

	if (gf) {
		if (re.Circfl)
			return 0;
		p = locs = loc2;
	} else {
		if (addr == zero)
			return 0;
		p = linebuf;
		getline(*addr);
		if ((value(IGNORECASE) ? 1:0) ^ (re.Flags & REG_ICASE ? 1:0))
			compile1();
		if (value(IGNORECASE))
			loconv(linebuf, linebuf);
		locs = 0;
	}
	circf = re.Circfl;
	return step(p, re.Expbuf);
}
#endif	/* !UXRE */
