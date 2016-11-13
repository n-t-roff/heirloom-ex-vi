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
static char sccsid[] = "@(#)ex_cmdsub.c	1.32 (gritter) 8/6/05";
#endif
#endif

/* from ex_cmdsub.c	7.7 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Command mode subroutines implementing
 *	append, args, copy, delete, join, move, put,
 *	shift, tag, yank, z and undo
 */

bool	endline = 1;
line	*tad1;
static	int jnoop(void);

/*
 * Append after line a lines returned by function f.
 * Be careful about intermediate states to avoid scramble
 * if an interrupt comes in.
 */
int 
append(int (*f)(void), line *a)
{
	register line *a1, *a2, *rdot;
	int nline;

	nline = 0;
	dot = a;
	if(FIXUNDO && !inopen && f!=getsub) {
		undap1 = undap2 = dot + 1;
		undkind = UNDCHANGE;
	}
	while ((*f)() == 0) {
		if (truedol >= endcore) {
			if (morelines() < 0) {
				if (FIXUNDO && f == getsub) {
					undap1 = addr1;
					undap2 = addr2 + 1;
				}
				error(catgets(catd, 1, 39,
				"Out of memory@- too many lines in file"));
			}
		}
		nline++;
		a1 = truedol + 1;
		a2 = a1 + 1;
		dot++;
		undap2++;
		dol++;
		unddol++;
		truedol++;
		for (rdot = dot; a1 > rdot;)
			*--a2 = *--a1;
		*rdot = 0;
		putmark(rdot);
		if (f == gettty) {
			dirtcnt++;
			TSYNC();
		}
	}
	return (nline);
}

void 
appendnone(void)
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		undap1 = undap2 = addr1;
	}
}

/*
 * Print out the argument list, with []'s around the current name.
 */
void 
pargs(void)
{
	register char **av = argv0, *as = args0;
	register int ac;

	for (ac = 0; ac < argc0; ac++) {
		if (ac != 0)
			putchar(' ' | QUOTE);
		if (ac + argc == argc0 - 1)
			ex_printf("[");
		lprintf("%s", as);
		if (ac + argc == argc0 - 1)
			ex_printf("]");
		as = av ? *++av : strend(as) + 1;
	}
	noonl();
}

/*
 * Delete lines; two cases are if we are really deleting,
 * more commonly we are just moving lines to the undo save area.
 */
void 
delete(int hush)
{
	register line *a1, *a2;

	nonzero();
	if(FIXUNDO) {
		register shand dsavint;

#ifdef TRACE
		if (trace)
			vudump("before delete");
#endif
		change();
		dsavint = signal(SIGINT, SIG_IGN);
		undkind = UNDCHANGE;
		a1 = addr1;
		squish();
		a2 = addr2;
		if (a2++ != dol) {
			reverse(a1, a2);
			reverse(a2, dol + 1);
			reverse(a1, dol + 1);
		}
		dol -= a2 - a1;
		unddel = a1 - 1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
		pkill[0] = pkill[1] = 0;
		signal(SIGINT, dsavint);
#ifdef TRACE
		if (trace)
			vudump("after delete");
#endif
	} else {
		register line *a3;
		register int i;

		change();
		a1 = addr1;
		a2 = addr2 + 1;
		a3 = truedol;
		i = a2 - a1;
		unddol -= i;
		undap2 -= i;
		dol -= i;
		truedol -= i;
		do
			*a1++ = *a2++;
		while (a2 <= a3);
		a1 = addr1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
	}
	if (!hush)
		killed();
}

void 
deletenone(void)
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		squish();
		unddel = addr1;
	}
}

/*
 * Crush out the undo save area, moving the open/visual
 * save area down in its place.
 */
void 
squish(void)
{
	register line *a1 = dol + 1, *a2 = unddol + 1, *a3 = truedol + 1;

	if(FIXUNDO) {
		if (inopen == -1)
			return;
		if (a1 < a2 && a2 < a3)
			do
				*a1++ = *a2++;
			while (a2 < a3);
		truedol -= unddol - dol;
		unddol = dol;
	}
}

static int	jcount;

/*
 * Join lines.  Special hacks put in spaces, two spaces if
 * preceding line ends with '.', or no spaces if next line starts with ).
 */
void 
join(int c)
{
	register line *a1;
	char *cp, *cp1;

	cp = genbuf;
	*cp = 0;
	for (a1 = addr1; a1 <= addr2; a1++) {
		getline(*a1);
		cp1 = linebuf;
		if (a1 != addr1 && c == 0) {
			while (*cp1 == ' ' || *cp1 == '\t')
				cp1++;
			if (*cp1 && cp > genbuf && cp[-1] != ' ' && cp[-1] != '\t') {
				if (*cp1 != ')') {
					*cp++ = ' ';
					if (cp[-2] == '.')
						*cp++ = ' ';
				}
			}
		}
		while ((*cp++ = *cp1++))
			if (cp > &genbuf[LBSIZE-2])
				grow(
		"Line overflow|Result line of join would be too long",
					&cp1, NULL, &cp, NULL);
		cp--;
	}
	strcLIN(genbuf);
	delete(0);
	jcount = 1;
	if (FIXUNDO)
		undap1 = undap2 = addr1;
	ignore(append(jnoop, --addr1));
	if (FIXUNDO)
		vundkind = VMANY;
}

static int
jnoop(void)
{

	return(--jcount);
}

/*
 * Move and copy lines.  Hard work is done by move1 which
 * is also called by undo.
 */

void 
move1(int cflag, line *addrt)
{
	register line *adt, *ad1, *ad2;
	int lines;

	adt = addrt;
	lines = (addr2 - addr1) + 1;
	if (cflag) {
		tad1 = addr1;
		ad1 = dol;
		ignore(append(getcopy, ad1++));
		ad2 = dol;
	} else {
		ad2 = addr2;
		for (ad1 = addr1; ad1 <= ad2;)
			*ad1++ &= ~01;
		ad1 = addr1;
	}
	ad2++;
	if (adt < ad1) {
		if (adt + 1 == ad1 && !cflag && !inglobal)
			error(catgets(catd, 1, 41,
					"That move would do nothing!"));
		dot = adt + (ad2 - ad1);
		if (++adt != ad1) {
			reverse(adt, ad1);
			reverse(ad1, ad2);
			reverse(adt, ad2);
		}
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	} else
		error(catgets(catd, 1, 42, "Move to a moved line"));
	change();
	if (!inglobal)
		if(FIXUNDO) {
			if (cflag) {
				undap1 = addrt + 1;
				undap2 = undap1 + lines;
				deletenone();
			} else {
				undkind = UNDMOVE;
				undap1 = addr1;
				undap2 = addr2;
				unddel = addrt;
				squish();
			}
		}
}

void 
move(void)
{
	register line *adt;
	bool iscopy = 0;

	if (Command[0] == 'm') {
		setdot1();
		markpr(addr2 == dot ? addr1 - 1 : addr2 + 1);
	} else {
		iscopy++;
		setdot();
	}
	nonzero();
	adt = address((char*)0);
	if (adt == 0)
		serror(catgets(catd, 1, 43,
			"%s where?|%s requires a trailing address"), Command);
	newline();
	move1(iscopy, adt);
	killed();
}

int 
getcopy(void)
{

	if (tad1 > addr2)
		return (EOF);
	getline(*tad1++);
	return (0);
}

/*
 * Put lines in the buffer from the undo save area.
 */
int 
getput(void)
{

	if (tad1 > unddol)
		return (EOF);
	getline(*tad1++);
	tad1++;
	return (0);
}

/*ARGSUSED*/
void 
put(int unused)
{
	register int cnt;

	(void)unused;
	if (!FIXUNDO)
		error(catgets(catd, 1, 44, "Cannot put inside global/macro"));
	cnt = unddol - dol;
	if (cnt && inopen && pkill[0] && pkill[1]) {
		pragged(1);
		return;
	}
	tad1 = dol + 1;
	ignore(append(getput, addr2));
	undkind = UNDPUT;
	notecnt = cnt;
	netchange(cnt);
}

/*
 * A tricky put, of a group of lines in the middle
 * of an existing line.  Only from open/visual.
 * Argument says pkills have meaning, e.g. called from
 * put; it is 0 on calls from putreg.
 */
void 
pragged(int kill)
{
	extern char *cursor;
	register char *gp = &genbuf[cursor - linebuf];

	/*
	 * This kind of stuff is TECO's forte.
	 * We just grunge along, since it cuts
	 * across our line-oriented model of the world
	 * almost scrambling our addled brain.
	 */
	if (!kill)
		getDOT();
	strcpy(genbuf, linebuf);
	getline(*unddol);
	if (kill)
		*pkill[1] = 0;
	lcat(linebuf, gp, LBSIZE);
	putmark(unddol);
	getline(dol[1]);
	if (kill)
		strcLIN(pkill[0]);
	safecp(gp, linebuf, MAXBSIZE - (gp - genbuf), "Line too long");
	strcLIN(genbuf);
	putmark(dol+1);
	undkind = UNDCHANGE;
	undap1 = dot;
	undap2 = dot + 1;
	unddel = dot - 1;
	undo(1);
}

/*
 * Shift lines, based on c.
 * If c is neither < nor >, then this is a lisp aligning =.
 */
void 
shift(int c, int cnt)
{
	register line *addr;
	char *cp = NULL;
	char *dp;
	register int i;

	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	cnt *= value(SHIFTWIDTH);
	for (addr = addr1; addr <= addr2; addr++) {
		dot = addr;
#ifdef LISPCODE
		if (c == '=' && addr == addr1 && addr != addr2)
			continue;
#endif
		getDOT();
		i = whitecnt(linebuf);
		switch (c) {

		case '>':
			if (linebuf[0] == 0)
				continue;
			cp = genindent(i + cnt);
			break;

		case '<':
			if (i == 0)
				continue;
			i -= cnt;
			cp = i > 0 ? genindent(i) : genbuf;
			break;

#ifdef LISPCODE
		default:
			i = lindent(addr);
			getDOT();
			cp = genindent(i);
			break;
#endif
		}
		if (cp + strlen(dp = vpastwh(linebuf)) >= &genbuf[LBSIZE - 2])
			grow(
		"Line too long|Result line after shift would be too long",
				&dp, NULL, &cp, NULL);
		CP(cp, dp);
		strcLIN(genbuf);
		putmark(addr);
	}
	killed();
}

/*
 * Find a tag in the tags file.
 * Most work here is in parsing the tags file itself.
 */
void
tagfind(bool quick)
{
	char cmdbuf[BUFSIZ];
	char filebuf[FNSIZE];
	char tagfbuf[128];
	register int c, d;
	bool samef = 1;
	int tfcount = 0;
	int omagic;
	int owrapscan;
	char *fn, *fne;
	struct stat sbuf;
	char *savefirstpat = NULL;
	int	ofailed;
	char *ft_iofbuf = NULL;
#ifdef FASTTAG
	int ft_iof;
	off_t mid;	/* assumed byte offset */
	off_t top, bot;	/* length of tag file */
#endif

	omagic = value(MAGIC);
	owrapscan = value(WRAPSCAN);
	ofailed = failed;
	failed = 1;
	if (!skipend()) {
		register char *lp = lasttag;

		while (!is_white(peekchar()) && !endcmd(peekchar()))
			if (lp < &lasttag[sizeof lasttag - 2])
				*lp++ = getchar();
			else
				ignchar();
		*lp++ = 0;
		if (!endcmd(peekchar()))
badtag:
			error(catgets(catd, 1, 46,
					"Bad tag|Give one tag per line"));
	} else if (lasttag[0] == 0)
		error(catgets(catd, 1, 47, "No previous tag"));
	c = getchar();
	if (!endcmd(c))
		goto badtag;
	if (c == EOF)
		ungetchar(c);
	clrstats();

	/*
	 * Loop once for each file in tags "path".
	 */
	safecp(tagfbuf, svalue(TAGS), sizeof tagfbuf, "Tag too long");
	fne = tagfbuf;
	fne--;
#ifdef	FASTTAG
	ft_iofbuf = smalloc(MAXBSIZE);
#endif
	while (fne) {
		fn = ++fne;
		while (*fne && *fne != ' ')
			fne++;
		if (*fne == 0)
			fne = 0;	/* done, quit after this time */
		else
			*fne = 0;	/* null terminate filename */
#ifdef FASTTAG
		ft_iof = topen(fn, ft_iofbuf);
		if (ft_iof == -1)
			continue;
		tfcount++;
		fstat(ft_iof, &sbuf);
		top = sbuf.st_size;
		if (top == (off_t) 0 )
			top = (off_t) -1;
		bot = (off_t) 0;
		while (top >= bot) {
#else
		/*
		 * Avoid stdio and scan tag file linearly.
		 */
		io = open(fn, O_RDONLY);
		if (io<0)
			continue;
		tfcount++;
		if (fstat(io, &sbuf) < 0 || sbuf.st_blksize > LBSIZE)
			bsize = LBSIZE;
		else {
			bsize = sbuf.st_blksize;
			if (bsize <= 0)
				bsize = LBSIZE;
		}
		while (getfile() == 0) {
#endif
			/* loop for each tags file entry */
			register char *cp = linebuf;
			register char *lp = lasttag;
			char *oglobp;

#ifdef FASTTAG
			mid = (top + bot) / 2;
			tseek(ft_iof, mid);
			if (mid > 0)	/* to get first tag in file to work */
				/* scan to next \n */
				if(tgets(linebuf, LBSIZE, ft_iof)==0)
					goto goleft;
			/* get the line itself */
			if(tgets(linebuf, LBSIZE, ft_iof)==0)
				goto goleft;
#ifdef TDEBUG
			ex_printf("tag: %o %o %o %s\n", bot, mid, top, linebuf);
#endif
#endif
			while (*cp && *lp == *cp)
				cp++, lp++;
			if ((*lp || !is_white(*cp)) && (value(TAGLENGTH)==0 ||
			    lp-lasttag < value(TAGLENGTH))) {
#ifdef FASTTAG
				if (*lp > *cp)
					bot = mid + 1;
				else
goleft:
					top = mid - 1;
#endif
				/* Not this tag.  Try the next */
				continue;
			}

			/*
			 * We found the tag.  Decode the line in the file.
			 */
#ifdef FASTTAG
			tclose(ft_iof);
#else
			close(io);
#endif
			/* Rest of tag if abbreviated */
			while (!is_white(*cp))
				cp++;

			/* name of file */
			while (*cp && is_white(*cp))
				cp++;
			if (!*cp) {
badtags:
				free(ft_iofbuf);
				serror(catgets(catd, 1, 48,
					"%s: Bad tags file entry"), lasttag);
			}
			lp = filebuf;
			while (*cp && *cp != ' ' && *cp != '\t') {
				if (lp < &filebuf[sizeof filebuf - 2])
					*lp++ = *cp;
				cp++;
			}
			*lp++ = 0;

			if (*cp == 0)
				goto badtags;
			if (dol != zero) {
				/*
				 * Save current position in 't for ^^ in visual.
				 */
				names['t'-'a'] = *dot &~ 01;
				if (inopen) {
					extern char *ncols['z'-'a'+2];
					extern char *cursor;

					ncols['t'-'a'] = cursor;
				}
			}
			safecp(cmdbuf, cp, sizeof cmdbuf, "command too long");
			if (strcmp(filebuf, savedfile) || !edited) {
				char cmdbuf2[sizeof filebuf + 10];

				savefirstpat = firstpat;
				firstpat = NULL;
				/* Different file.  Do autowrite & get it. */
				if (!quick) {
					ckaw();
					if (chng && dol > zero) {
						free(ft_iofbuf);
						error(catgets(catd, 1, 49,
			"No write@since last change (:tag! overrides)"));
					}
				}
				oglobp = globp;
				lcpy(cmdbuf2, "e! ", sizeof cmdbuf2);
				lcat(cmdbuf2, filebuf, sizeof cmdbuf2);
				globp = cmdbuf2;
				d = peekc; ungetchar(0);
				commands(1, 1);
				peekc = d;
				globp = oglobp;
				value(MAGIC) = omagic;
				if (tflag > 0)
					value(WRAPSCAN) = owrapscan;
				samef = 0;
				firstpat = savefirstpat;
			}

			/*
			 * Look for pattern in the current file.
			 */
			oglobp = globp;
			globp = cmdbuf;
			d = peekc; ungetchar(0);
			if (samef)
				markpr(dot);
			/*
			 * BUG: if it isn't found (user edited header
			 * line) we get left in nomagic mode.
			 */
			value(MAGIC) = 0;
			if (tflag > 0)
				value(WRAPSCAN) = 1;
			commands(1, 1);
			failed = ofailed;
			peekc = d;
			globp = oglobp;
			value(MAGIC) = omagic;
			if (tflag > 0) {
				value(WRAPSCAN) = owrapscan;
				if (savefirstpat) {
					globp = savefirstpat;
					tflag = -1;
				} else
					tflag = 0;
			}
			free(ft_iofbuf);
			return;
		}	/* end of "for each tag in file" */

		/*
		 * No such tag in this file.  Close it and try the next.
		 */
#ifdef FASTTAG
		tclose(ft_iof);
#else
		close(io);
#endif
	}	/* end of "for each file in path" */
	free(ft_iofbuf);
	if (tfcount <= 0)
		error(catgets(catd, 1, 50, "No tags file"));
	else
		serror(catgets(catd, 1, 51,
				"%s: No such tag@in tags file"), lasttag);
}

/*
 * Save lines from addr1 thru addr2 as though
 * they had been deleted.
 */
/*ARGSUSED*/
void
yank(int unused)
{
	(void)unused;
	if (!FIXUNDO)
		error(catgets(catd, 1, 52, "Can't yank inside global/macro"));
	save12();
	undkind = UNDNONE;
	killcnt(addr2 - addr1 + 1);
}

/*
 * z command; print windows of text in the file.
 *
 * If this seems unreasonably arcane, the reasons
 * are historical.  This is one of the first commands
 * added to the first ex (then called en) and the
 * number of facilities here were the major advantage
 * of en over ed since they allowed more use to be
 * made of fast terminals w/o typing .,.22p all the time.
 */
bool	zhadpr;
bool	znoclear;
short	zweight;

void
zop(int hadpr)
{
	register int c, lines, op;
	bool excl;

	zhadpr = hadpr;
	notempty();
	znoclear = 0;
	zweight = 0;
	excl = exclam();
	switch (c = op = getchar()) {

	case '^':
		zweight = 1;
	case '-':
	case '+':
		while (peekchar() == op) {
			ignchar();
			zweight++;
		}
	case '=':
	case '.':
		c = getchar();
		break;

	case EOF:
		znoclear++;
		break;

	default:
		op = 0;
		break;
	}
	if (isdigit(c)) {
		lines = c - '0';
		for(;;) {
			c = getchar();
			if (!isdigit(c))
				break;
			lines *= 10;
			lines += c - '0';
		}
		if (lines < TLINES)
			znoclear++;
		value(WINDOW) = lines;
		if (op == '=')
			lines += 2;
	} else
		lines = op == EOF ? value(SCROLL) : excl ? TLINES - 1 : 2*value(SCROLL);
	if (inopen || c != EOF) {
		ungetchar(c);
		newline();
	}
	addr1 = addr2;
	if (addr2 == 0 && dot < dol && op == 0)
		addr1 = addr2 = dot+1;
	setdot();
	zop2(lines, op);
}

static void
splitit(void)
{
	register int l;

	for (l = TCOLUMNS > 80 ? 40 : TCOLUMNS / 2; l > 0; l--)
		putchar('-');
	putnl();
}

void
zop2(register int lines, register int op)
{
	register line *split;

	split = NULL;
	switch (op) {

	case EOF:
		if (addr2 == dol)
			error(catgets(catd, 1, 53, "\nAt EOF"));
	case '+':
		if (addr2 == dol)
			error(catgets(catd, 1, 54, "At EOF"));
		addr2 += lines * zweight;
		if (addr2 > dol)
			error(catgets(catd, 1, 55, "Hit BOTTOM"));
		addr2++;
	default:
		addr1 = addr2;
		addr2 += lines-1;
		dot = addr2;
		break;

	case '=':
	case '.':
		znoclear = 0;
		lines--;
		lines >>= 1;
		if (op == '=')
			lines--;
		addr1 = addr2 - lines;
		if (op == '=')
			dot = split = addr2;
		addr2 += lines;
		if (op == '.') {
			markDOT();
			dot = addr2;
		}
		break;

	case '^':
	case '-':
		addr2 -= lines * zweight;
		if (addr2 < one)
			error(catgets(catd, 1, 56, "Hit TOP"));
		lines--;
		addr1 = addr2 - lines;
		dot = addr2;
		break;
	}
	if (addr1 <= zero)
		addr1 = one;
	if (addr2 > dol)
		addr2 = dol;
	if (dot > dol)
		dot = dol;
	if (addr1 > addr2)
		return;
	if (op == EOF && zhadpr) {
		getline(*addr1);
		putchar('\r' | QUOTE);
		shudclob = 1;
	} else if (znoclear == 0 && CL != NOSTR && !inopen) {
		flush1();
		vclear();
	}
	if (addr2 - addr1 > 1)
		pstart();
	if (split) {
		plines(addr1, split - 1, 0);
		splitit();
		plines(split, split, 0);
		splitit();
		addr1 = split + 1;
	}
	plines(addr1, addr2, 0);
}

void
plines(line *adr1, register line *adr2, bool movedot)
{
	register line *addr;

	pofix();
	for (addr = adr1; addr <= adr2; addr++) {
		getline(*addr);
		pline(lineno(addr), -1);
		if (inopen) {
			putchar('\n' | QUOTE);
		}
		if (movedot)
			dot = addr;
	}
}

void
pofix(void)
{

	if (inopen && Outchar != termchar) {
		vnfl();
		setoutt();
	}
}

/*
 * Be (almost completely) sure there really
 * was a change, before claiming to undo.
 */
void
somechange(void)
{
	register line *ip, *jp;

	switch (undkind) {

	case UNDMOVE:
		return;

	case UNDCHANGE:
		if (undap1 == undap2 && dol == unddol)
			break;
		return;

	case UNDPUT:
		if (undap1 != undap2)
			return;
		break;

	case UNDALL:
		if (unddol - dol != lineDOL())
			return;
		for (ip = one, jp = dol + 1; ip <= dol; ip++, jp++)
			if ((*ip &~ 01) != (*jp &~ 01))
				return;
		break;

	case UNDNONE:
		error(catgets(catd, 1, 57, "Nothing to undo"));
	}
	error(catgets(catd, 1, 58,
	"Nothing changed|Last undoable command didn't change anything"));
}

/*
 * Dudley doright to the rescue.
 * Undo saves the day again.
 * A tip of the hatlo hat to Warren Teitleman
 * who made undo as useful as do.
 *
 * Command level undo works easily because
 * the editor has a unique temporary file
 * index for every line which ever existed.
 * We don't have to save large blocks of text,
 * only the indices which are small.  We do this
 * by moving them to after the last line in the
 * line buffer array, and marking down info
 * about whence they came.
 *
 * Undo is its own inverse.
 */
void
undo(bool c)
{
	register int i, j;
	register line *jp, *kp;
	line *dolp1, *newdol, *newadot;

#ifdef TRACE
	if (trace)
		vudump("before undo");
#endif
	if (inglobal && inopen <= 0)
		error(catgets(catd, 1, 59, "Can't undo in global@commands"));
	if (!c)
		somechange();
	pkill[0] = pkill[1] = 0;
	change();
	if (undkind == UNDMOVE) {
 		/*
		 * Command to be undone is a move command.
		 * This is handled as a special case by noting that
		 * a move "a,b m c" can be inverted by another move.
		 */
		if ((i = (jp = unddel) - undap2) > 0) {
			/*
			 * when c > b inverse is a+(c-b),c m a-1
			 */
			addr2 = jp;
			addr1 = (jp = undap1) + i;
			unddel = jp-1;
		} else {
			/*
			 * when b > c inverse is  c+1,c+1+(b-a) m b
			 */
			addr1 = ++jp;
			addr2 = jp + ((unddel = undap2) - undap1);
		}
		kp = undap1;
		move1(0, unddel);
		dot = kp;
		Command = "move";
		killed();
	} else {
		int cnt;

		newadot = dot;
		cnt = lineDOL();
		newdol = dol;
		dolp1 = dol + 1;
		/*
		 * If a mark is pointing to a line between undap1 and
		 * undap2-1, it would be lost (i.e. pointing into the
		 * block between dolp and undol) after the undo. Thus
		 * these marks have to be changed to point to the line
		 * after dolp1 that is restored later during this undo
		 * operation.
		 */
		if (anymarks)
			for (i = 0; &undap1[i] < undap2; i++)
				for (j = 0; j <= 'z'-'a'; j++)
					if (names[j] == (undap1[i] & ~01))
						names[j] = dolp1[i] & ~01;
		/*
		 * Command to be undone is a non-move.
		 * All such commands are treated as a combination of
		 * a delete command and a append command.
		 * We first move the lines appended by the last command
		 * from undap1 to undap2-1 so that they are just before the
		 * saved deleted lines.
		 */
		if ((i = (kp = undap2) - (jp = undap1)) > 0) {
			if (kp != dolp1) {
				reverse(jp, kp);
				reverse(kp, dolp1);
				reverse(jp, dolp1);
			}
			/*
			 * Account for possible backward motion of target
			 * for restoration of saved deleted lines.
			 */
			if (unddel >= jp)
				unddel -= i;
			newdol -= i;
			/*
			 * For the case where no lines are restored, dot
			 * is the line before the first line deleted.
			 */
			dot = jp-1;
		}
		/*
		 * Now put the deleted lines, if any, back where they were.
		 * Basic operation is: dol+1,unddol m unddel
		 */
		if (undkind == UNDPUT) {
			unddel = undap1 - 1;
			squish();
		}
		jp = unddel + 1;
		if ((i = (kp = unddol) - dol) > 0) {
			if (jp != dolp1) {
				reverse(jp, dolp1);
				reverse(dolp1, ++kp);
				reverse(jp, kp);
			}
			/*
			 * Account for possible forward motion of the target
			 * for restoration of the deleted lines.
			 */
			if (undap1 >= jp)
				undap1 += i;
			/*
			 * Dot is the first resurrected line.
			 */
			dot = jp;
			newdol += i;
		}
		/*
		 * Clean up so we are invertible
		 */
		unddel = undap1 - 1;
		undap1 = jp;
		undap2 = jp + i;
		dol = newdol;
		netchHAD(cnt);
		if (undkind == UNDALL) {
			dot = undadot;
			undadot = newadot;
		} else
			undkind = UNDCHANGE;
	}
	/*
	 * Defensive programming - after a munged undadot.
	 * Also handle empty buffer case.
	 */
	if ((dot <= zero || dot > dol) && dot != dol)
		dot = one;
#ifdef TRACE
	if (trace)
		vudump("after undo");
#endif
}

/*
 * Map command:
 * map src dest
 */
void
mapcmd(int un, int ab)
	/* int un;	/\* true if this is unmap command */
	/*int ab;	/\* true if this is abbr command */
{
	char lhs[100], rhs[100];	/* max sizes resp. */
	register char *p;
	register int c;		/* mjm: char --> int */
	char *dname;
	struct maps *mp;	/* the map structure we are working on */

	mp = ab ? abbrevs : exclam() ? immacs : arrows;
	if (skipend()) {
		int i;

		/* print current mapping values */
		if (peekchar() != EOF)
			ignchar();
		if (un)
			error(catgets(catd, 1, 60, "Missing lhs"));
		if (inopen)
			pofix();
		for (i=0; mp[i].mapto; i++)
			if (mp[i].cap) {
				lprintf("%s", mp[i].descr);
				putchar('\t');
				lprintf("%s", mp[i].cap);
				putchar('\t');
				lprintf("%s", mp[i].mapto);
				putNFL();
			}
		return;
	}

	ignore(skipwh());
	for (p=lhs; ; ) {
		c = getchar();
		if (c == CTRL('v')) {
			c = getchar();
		} else if (!un && any(c, " \t")) {
			/* End of lhs */
			break;
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			if (un) {
				newline();
				*p = 0;
				addmac(lhs, NOSTR, NOSTR, mp);
				return;
			} else
				error(catgets(catd, 1, 61, "Missing rhs"));
		}
		*p++ = c;
	}
	*p = 0;

	if (skipend())
		error(catgets(catd, 1, 62, "Missing rhs"));
	for (p=rhs; ; ) {
		c = getchar();
		if (c == CTRL('v')) {
			c = getchar();
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			break;
		}
		*p++ = c;
	}
	*p = 0;
	newline();
	/*
	 * Special hack for function keys: #1 means key f1, etc.
	 * If the terminal doesn't have function keys, we just use #1.
	 */
	if (lhs[0] == '#') {
		char *fnkey;
		char funkey[3];

		fnkey = fkey(lhs[1] - '0');
		funkey[0] = 'f'; funkey[1] = lhs[1]; funkey[2] = 0;
		if (fnkey)
			lcpy(lhs, fnkey, sizeof lhs);
		dname = funkey;
	} else {
		dname = lhs;
	}
	addmac(lhs,rhs,dname,mp);
}

/*
 * Create the integer version of a macro string, for processing in visual
 * mode. imapspace cannot overflow because an earlier overflow for mapspace
 * would have been detected already.
 */
static void
intmac(int **dp, char *cp)
{
	int	c, n;

	if (imsnext == NULL)
		imsnext = imapspace;
	*dp = imsnext;
	for (;;) {
		nextc(c, cp, n);
		*imsnext++ = c;
		if (c == 0)
			break;
		cp += n;
	}
}

/*
 * Add a macro definition to those that already exist. The sequence of
 * chars "src" is mapped into "dest". If src is already mapped into something
 * this overrides the mapping. There is no recursion. Unmap is done by
 * using NOSTR for dest.  Dname is what to show in listings.  mp is
 * the structure to affect (arrows, etc).
 */
void
addmac1(register char *src,register char *dest,register char *dname,
		register struct maps *mp, int force)
{
	register int slot, zer;

#ifdef TRACE
	if (trace)
		fprintf(trace, "addmac(src='%s', dest='%s', dname='%s', mp=%x\n", src, dest, dname, mp);
#endif
	if (dest && mp==arrows && !force) {
		/* Make sure user doesn't screw himself */
		/*
		 * Prevent tail recursion. We really should be
		 * checking to see if src is a suffix of dest
		 * but this makes mapping involving escapes that
		 * is reasonable mess up.
		 */
		if (src[1] == 0 && src[0] == dest[strlen(dest)-1])
			error(catgets(catd, 1, 63, "No tail recursion"));
		/*
		 * We don't let the user rob himself of ":", and making
		 * multi char words is a bad idea so we don't allow it.
		 * Note that if user sets mapinput and maps all of return,
		 * linefeed, and escape, he can screw himself. This is
		 * so weird I don't bother to check for it.
		 */
		if ((isalpha(src[0]&0377) && src[1]) || any(src[0],":"))
			error(catgets(catd, 1, 64,
						"Too dangerous to map that"));
	}
	else if (dest) {
		/* check for tail recursion in input mode: fussier */
		if (eq(src, dest+strlen(dest)-strlen(src)))
			error(catgets(catd, 1, 65, "No tail recursion"));
	}
	/*
	 * If the src were null it would cause the dest to
	 * be mapped always forever. This is not good.
	 */
	if (!force && (src == NOSTR || src[0] == 0))
		error(catgets(catd, 1, 66, "Missing lhs"));

	/* see if we already have a def for src */
	zer = -1;
	for (slot=0; mp[slot].mapto; slot++) {
		if (mp[slot].cap) {
			if (eq(src, mp[slot].cap) || eq(src, mp[slot].mapto))
				break;	/* if so, reuse slot */
		} else {
			zer = slot;	/* remember an empty slot */
		}
	}

	if (dest == NOSTR) {
		/* unmap */
		if (mp[slot].cap) {
			mp[slot].cap = NOSTR;
			mp[slot].descr = NOSTR;
		} else {
			error(catgets(catd, 1, 67,
				"Not mapped|That macro wasn't mapped"));
		}
		return;
	}

	/* reuse empty slot, if we found one and src isn't already defined */
	if (zer >= 0 && mp[slot].mapto == 0)
		slot = zer;

	/* if not, append to end */
	if (slot >= MAXNOMACS)
		error(catgets(catd, 1, 68, "Too many macros"));
	if (msnext == 0)	/* first time */
		msnext = mapspace;
	/* Check is a bit conservative, we charge for dname even if reusing src */
	if (msnext - mapspace + strlen(dest) + (src ? strlen(src) : 0) + strlen(dname) + 3 > MAXCHARMACS)
		error(catgets(catd, 1, 69, "Too much macro text"));
	if (src) {
		CP(msnext, src);
		mp[slot].cap = msnext;
		msnext += strlen(src) + 1;	/* plus 1 for null on the end */
		intmac(&mp[slot].icap, src);
	} else
		mp[slot].cap = NULL;
	CP(msnext, dest);
	mp[slot].mapto = msnext;
	msnext += strlen(dest) + 1;
	if (dname) {
		CP(msnext, dname);
		mp[slot].descr = msnext;
		msnext += strlen(dname) + 1;
	} else {
		/* default descr to string user enters */
		mp[slot].descr = src;
	}
}

/*
 * Implements macros from command mode. c is the buffer to
 * get the macro from.
 */
void
cmdmac(char c)
{
	char macbuf[BUFSIZ];
	line *ad, *a1, *a2;
	char *oglobp;
	short pk;
	bool oinglobal;

	lastmac = c;
	oglobp = globp;
	oinglobal = inglobal;
	pk = peekc; peekc = 0;
	if (inglobal < 2)
		inglobal = 1;
	regbuf(c, macbuf, sizeof(macbuf));
	a1 = addr1; a2 = addr2;
	for (ad=a1; ad<=a2; ad++) {
		globp = macbuf;
		dot = ad;
		commands(1,1);
	}
	globp = oglobp;
	inglobal = oinglobal;
	peekc = pk;
}
