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
static char sccsid[] = "@(#)ex_vget.c	1.31 (gritter) 8/6/05";
#endif
#endif

/* from ex_vget.c	6.8.1 (2.11BSD GTE) 12/9/94 */

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Input routines for open/visual.
 * We handle reading from the echo area here as well as notification on 
 * large changes which appears in the echo area.
 */

/*
 * Return the key.
 */
void 
ungetkey (
    int c		/* mjm: char --> int */
)
{

	if (Peekkey != ATTN)
		Peekkey = c;
}

/*
 * Return a keystroke, but never a ^@.
 */
int 
getkey(void)
{
	register int c;		/* mjm: char --> int */

	do {
		c = getbr();
		if (c==0)
			beep();
	} while (c == 0);
	return (c);
}

/*
 * Tell whether next keystroke would be a ^@.
 */
int 
peekbr(void)
{

	Peekkey = getbr();
	return (Peekkey == 0);
}

short	precbksl;
JMP_BUF	readbuf;
int	doingread = 0;

static int
readwc(int fd, int *cp)
{
	int	c;
	char	b;

#ifdef	MB
	if (mb_cur_max > 1) {
		static char	pbuf[2][MB_LEN_MAX], *pend[2], *pcur[2];
		static mbstate_t	state[2];
		static int	incompl[2];
		int	i, rest;
		int	idx = fd ? 1 : 0;
		wchar_t	wc;
		size_t	sz;

		i = 0;
		rest = pend[idx] - pcur[idx];
		if (rest && pcur[idx] > pbuf[idx]) {
			do
				pbuf[idx][i] = pcur[idx][i];
			while (i++, --rest);
		} else if (incompl[idx]) {
			pend[idx] = pcur[idx] = NULL;
			return -1;
		}
		if (i == 0) {
			if ((c = read(fd, &b, 1)) <= 0) {
				pend[idx] = pcur[idx] = NULL;
				return c;
			}
			pbuf[idx][i++] = b;
		}
		if (pbuf[idx][0] & 0200) {
			sz = 1;
			while ((sz = mbrtowc(&wc, pbuf[idx], i, &state[idx]))
					== (size_t)-2 && i < mb_cur_max) {
				if ((c = read(fd, &b, 1)) <= 0) {
					incompl[idx] = 1;
					break;
				} else
					pbuf[idx][i++] = b;
				memset(&state[idx], 0, sizeof state[idx]);
			}
			if (sz == (size_t)-2 || sz == (size_t)-1 ||
					!widthok(wc)) {
				memset(&state[idx], 0, sizeof state[idx]);
				c = 1;
				*cp = pbuf[idx][0] | INVBIT;
			} else if (sz == 0) {
				c = 1;
				*cp = wc;
			} else {
				c = sz;
				*cp = wc;
			}
		} else {
			c = 1;
			*cp = pbuf[idx][0];
		}
		pcur[idx] = &pbuf[idx][c];
		pend[idx] = &pcur[idx][i-c];
		return c;
	} else
#endif	/* MB */
	{
		c = read(fd, &b, 1);
		*cp = b;
		return c;
	}
}

/*
 * Get a keystroke, including a ^@.
 * If an key was returned with ungetkey, that
 * comes back first.  Next comes unread input (e.g.
 * from repeating commands with .), and finally new
 * keystrokes.
 */
int 
getbr(void)
{
	int ch;
	register int c;
#ifdef	UCVISUAL
	register int d;
	register char *colp;
#endif
#ifdef BEEHIVE
	int cnt;
	static char Peek2key;
#endif
	extern short slevel, ttyindes;

getATTN:
	if (Peekkey) {
		c = Peekkey;
		Peekkey = 0;
		return (c);
	}
#ifdef BEEHIVE
	if (Peek2key) {
		c = Peek2key;
		Peek2key = 0;
		return (c);
	}
#endif
	if (vglobp) {
		if (*vglobp)
			return (lastvgk = *vglobp++);
		lastvgk = 0;
		return (ESCAPE);
	}
	if (vmacp) {
		if (*vmacp) {
			int	n;
			nextc(ch, vmacp, n);
			vmacp += n;
			return (ch);
		}
		/* End of a macro or set of nested macros */
		vmacp = 0;
		if (inopen == -1)	/* don't screw up undo for esc esc */
			vundkind = VMANY;
		inopen = 1;	/* restore old setting now that macro done */
		vch_mac = VC_NOTINMAC;
	}
	flusho();
	for (c =0; abbrevs[c].mapto; c++)
		abbrevs[c].hadthis = 0;
#ifdef	UCVISUAL
again:
#endif
	if (SETJMP(readbuf))
		goto getATTN;
	doingread = 1;
	c = readwc(slevel == 0 ? 0 : ttyindes, &ch);
	doingread = 0;
	if (c < 1) {
		if (errno == EINTR)
			goto getATTN;
		error(catgets(catd, 1, 222, "Input read error"));
	}
	c = ch & TRIM;
#ifdef BEEHIVE
	if (XB && slevel==0 && c == ESCAPE) {
		if (readwc(0, &Peek2key) < 1)
			goto getATTN;
		Peek2key &= TRIM;
		switch (Peek2key) {
		case 'C':	/* SPOW mode sometimes sends \EC for space */
			c = ' ';
			Peek2key = 0;
			break;
		case 'q':	/* f2 -> ^C */
			c = CTRL('c');
			Peek2key = 0;
			break;
		case 'p':	/* f1 -> esc */
			Peek2key = 0;
			break;
		}
	}
#endif

#ifdef UCVISUAL
        /*
         * The algorithm here is that of the UNIX kernel.
         * See the description in the programmers manual.
         */
        if (UPPERCASE) {
                if (xisupper(c))
                        c = xtolower(c);
                if (c == '\\') {
                        if (precbksl < 2)
                                precbksl++;
                        if (precbksl == 1)
                                goto again;
                } else if (precbksl) {
                        d = 0;
                        if (xislower(c))
                                d = xtoupper(c);
                        else {
                                colp = "({)}!|^~'~";
                                while ((d = *colp++))
                                        if (d == c) {
                                                d = *colp++;
                                                break;
                                        } else
                                                colp++;
                        }
                        if (precbksl == 2) {
                                if (!d) {
                                        Peekkey = c;
                                        precbksl = 0;
                                        c = '\\';
                                }
                        } else if (d)
                                c = d;
                        else {
                                Peekkey = c;
                                precbksl = 0;
                                c = '\\';
                        }
                }
                if (c != '\\')
                        precbksl = 0;
        }
#endif

#ifdef TRACE
	if (trace) {
		if (!techoin) {
			tfixnl();
			techoin = 1;
			fprintf(trace, "*** Input: ");
		}
		tracec(c);
	}
#endif
	lastvgk = 0;
	return (c);
}

/*
 * Get a key, but if a delete, quit or attention
 * is typed return 0 so we will abort a partial command.
 */
int 
getesc(void)
{
	register int c;

	c = getkey();
	if (c == ATTN)
		goto case_ATTN;
	switch (c) {

	case CTRL('v'):
	case CTRL('q'):
		c = getkey();
		return (c);

	case QUIT:
case_ATTN:
		ungetkey(c);
		return (0);

	case ESCAPE:
		return (0);
	}
	return (c);
}

/*
 * Peek at the next keystroke.
 */
int 
peekkey(void)
{

	Peekkey = getkey();
	return (Peekkey);
}

/*
 * Read a line from the echo area, with single character prompt c.
 * A return value of 1 means the user blewit or blewit away.
 */
int 
readecho(int c)
{
	register char *sc = cursor;
	register int (*OP)(int, int);
	bool waste;
	register int OPeek;

	if (WBOT == WECHO)
		vclean();
	else
		vclrech(0);
	splitw++;
	vgoto(WECHO, 0);
	putchar(c);
	vclreol();
	vgoto(WECHO, 1);
	cursor = linebuf; linebuf[0] = 0; genbuf[0] = c;
	if (peekbr()) {
		if (!INS[0] || (INS[0] & (QUOTE|TRIM)) == OVERBUF)
			goto blewit;
		vglobp = INS;
	}
	OP = Pline; Pline = normline;
	ignore(vgetline(0, genbuf + 1, &waste, c));
	if (Outchar == termchar)
		putchar('\n');
	vscrap();
	Pline = OP;
	if (Peekkey != ATTN && Peekkey != QUIT && Peekkey != CTRL('h')) {
		cursor = sc;
		vclreol();
		return (0);
	}
blewit:
	OPeek = Peekkey==CTRL('h') ? 0 : Peekkey; Peekkey = 0;
	splitw = 0;
	vclean();
	vshow(dot, NOLINE);
	vnline(sc);
	Peekkey = OPeek;
	return (1);
}

/*
 * A complete command has been defined for
 * the purposes of repeat, so copy it from
 * the working to the previous command buffer.
 */
void 
setLAST(void)
{

	if (vglobp || vmacp)
		return;
	lastreg = vreg;
	lasthad = Xhadcnt;
	lastcnt = Xcnt;
	*lastcp = 0;
	cellcpy(lastcmd, workcmd);
}

/*
 * Gather up some more text from an insert.
 * If the insertion buffer oveflows, then destroy
 * the repeatability of the insert.
 */
void 
addtext(char *cp)
{

	if (vglobp)
		return;
	addto(INS, cp);
	if ((INS[0] & (QUOTE|TRIM)) == OVERBUF)
		lastcmd[0] = 0;
}

void 
setDEL(void)
{

	setBUF(DEL);
}

/*
 * Put text from cursor upto wcursor in BUF.
 */
void 
setBUF(register cell *BUF)
{
	register int c;
	register char *wp = wcursor;

	c = *wp;
	*wp = 0;
	BUF[0] = 0;
	addto(BUF, cursor);
	*wp = c;
}

void 
addto(register cell *buf, register char *str)
{

	if ((buf[0] & (QUOTE|TRIM)) == OVERBUF)
		return;
	if (cellen(buf) + strlen(str) + 1 >= VBSIZE) {
		buf[0] = OVERBUF;
		return;
	}
	while (*buf)
		buf++;
	str2cell(buf, str);
}

/*
 * Note a change affecting a lot of lines, or non-visible
 * lines.  If the parameter must is set, then we only want
 * to do this for open modes now; return and save for later
 * notification in visual.
 */
int 
noteit(int must)
{
	register int sdl = destline, sdc = destcol;

	if (notecnt < 2 || (!must && state == VISUAL))
		return (0);
	splitw++;
	if (WBOT == WECHO)
		vmoveitup(1, 1);
	vigoto(WECHO, 0);
	ex_printf(catgets(catd, 1, 223, "%d %sline"), notecnt, notesgn);
	if (notecnt > 1)
		putchar('s');
	if (*notenam) {
		ex_printf(" %s", notenam);
		if (*(strend(notenam) - 1) != 'e')
			putchar('e');
		putchar('d');
	}
	vclreol();
	notecnt = 0;
	if (state != VISUAL)
		vcnt = vcline = 0;
	splitw = 0;
	if (state == ONEOPEN || state == CRTOPEN)
		vup1();
	destline = sdl; destcol = sdc;
	return (1);
}

/*
 * Rrrrringgggggg.
 * If possible, use flash (VB).
 */
void
beep(void)
{

	if (VB && value(FLASH))
		vputp(VB, 0);
	else
		vputc(CTRL('g'));
}

/*
 * Push an integer string as a macro.
 */
static void
imacpush(int *ip, int canundo)
{
	char	buf[BUFSIZ], *bp = buf;

#ifdef	MB
	do {
		int	n;
		n = wctomb(bp, *ip&TRIM);
		bp += n;
	} while (*ip++);
#else	/* !MB */
	while (*bp++ = *ip++);
#endif	/* !MB */
	macpush(buf, canundo);
}

/*
 * Map the command input character c,
 * for keypads and labelled keys which do cursor
 * motions.  I.e. on an adm3a we might map ^K to ^P.
 * DM1520 for example has a lot of mappable characters.
 */

int 
map(register int c, register struct maps *maps)
{
	register int d;
	register int *p, *q;
	int b[10+MB_LEN_MAX];	/* Assumption: no keypad sends string longer than 10 */

	/*
	 * Mapping for special keys on the terminal only.
	 * BUG: if there's a long sequence and it matches
	 * some chars and then misses, we lose some chars.
	 *
	 * For this to work, some conditions must be met.
	 * 1) Keypad sends SHORT (2 or 3 char) strings
	 * 2) All strings sent are same length & similar
	 * 3) The user is unlikely to type the first few chars of
	 *    one of these strings very fast.
	 * Note: some code has been fixed up since the above was laid out,
	 * so conditions 1 & 2 are probably not required anymore.
	 * However, this hasn't been tested with any first char
	 * that means anything else except escape.
	 */
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"map(%c): ",c);
#endif
	/*
	 * If c==0, the char came from getesc typing escape.  Pass it through
	 * unchanged.  0 messes up the following code anyway.
	 */
	if (c==0)
		return(0);

	b[0] = c;
	b[1] = 0;
	for (d=0; maps[d].mapto; d++) {
#ifdef MDEBUG
		if (trace)
			fprintf(trace,"\ntry '%s', ",maps[d].cap);
#endif
		if ((p = maps[d].icap)) {
			for (q=b; *p; p++, q++) {
#ifdef MDEBUG
				if (trace)
					fprintf(trace,"q->b[%d], ",q-b);
#endif
				if (*q==0) {
					/*
					 * Is there another char waiting?
					 *
					 * This test is oversimplified, but
					 * should work mostly. It handles the
					 * case where we get an ESCAPE that
					 * wasn't part of a keypad string.
					 */
					if ((c=='#' ? peekkey() : fastpeekkey()) == 0) {
#ifdef MDEBUG
						if (trace)
							fprintf(trace,"fpk=0: will return '%c'",c);
#endif
						/*
						 * Nothing waiting.  Push back
						 * what we peeked at & return
						 * failure (c).
						 *
						 * We want to be able to undo
						 * commands, but it's nonsense
						 * to undo part of an insertion
						 * so if in input mode don't.
						 */
#ifdef MDEBUG
						if (trace)
							fprintf(trace, "Call macpush, b %d %d %d\n", b[0], b[1], b[2]);
#endif
						imacpush(&b[1],maps == arrows);
#ifdef MDEBUG
						if (trace)
							fprintf(trace, "return %d\n", c);	
#endif
						return(c);
					}
					*q = getkey();
					q[1] = 0;
				}
				if (*p != *q)
					goto contin;
			}
			macpush(maps[d].mapto,maps == arrows);
			c = getkey();
#ifdef MDEBUG
			if (trace)
				fprintf(trace,"Success: push(%s), return %c",maps[d].mapto, c);
#endif
			return(c);	/* first char of map string */
			contin:;
		}
	}
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"Fail: push(%s), return %c", &b[1], c);
#endif
	imacpush(&b[1],0);
	return(c);
}

/*
 * Push st onto the front of vmacp. This is tricky because we have to
 * worry about where vmacp was previously pointing. We also have to
 * check for overflow (which is typically from a recursive macro)
 * Finally we have to set a flag so the whole thing can be undone.
 * canundo is 1 iff we want to be able to undo the macro.  This
 * is false for, for example, pushing back lookahead from fastpeekkey(),
 * since otherwise two fast escapes can clobber our undo.
 */
void 
macpush(char *st, int canundo)
{
	char tmpbuf[BUFSIZ];

	if (st==0 || *st==0)
		return;
#ifdef MDEBUG
	if (trace)
		fprintf(trace, "macpush(%s), canundo=%d\n",st,canundo);
#endif
	if ((vmacp ? strlen(vmacp) : 0) + strlen(st) > BUFSIZ)
		error(catgets(catd, 1, 224,
				"Macro too long@ - maybe recursive?"));
	if (vmacp) {
		lcpy(tmpbuf, vmacp, sizeof tmpbuf);
		if (!FIXUNDO)
			canundo = 0;	/* can't undo inside a macro anyway */
	}
	lcpy(vmacbuf, st, sizeof vmacbuf);
	if (vmacp)
		lcat(vmacbuf, tmpbuf, sizeof vmacbuf);
	vmacp = vmacbuf;
	/* arrange to be able to undo the whole macro */
	if (canundo) {
#ifdef notdef
		otchng = tchng;
		vsave();
		saveall();
		inopen = -1;	/* no need to save since it had to be 1 or -1 before */
		vundkind = VMANY;
#endif
		vch_mac = VC_NOCHANGE;
	}
}

#ifdef TRACE
void 
visdump(char *s)
{
	register int i;

	if (!trace) return;

	fprintf(trace, "\n%s: basWTOP=%d, basWLINES=%d, WTOP=%d, WBOT=%d, WLINES=%d, WCOLS=%d, WECHO=%d\n",
		s, basWTOP, basWLINES, WTOP, WBOT, WLINES, WCOLS, WECHO);
	fprintf(trace, "   vcnt=%d, vcline=%d, cursor=%d, wcursor=%d, wdot=%d\n",
		vcnt, vcline, cursor-linebuf, wcursor-linebuf, wdot-zero);
	for (i=0; i<TUBELINES; i++)
		if (vtube[i] && *vtube[i])
			fprintf(trace, "%d: '%s'\n", i, vtube[i]);
	tvliny();
}

void 
vudump(char *s)
{
	register line *p;
	char savelb[1024];

	if (!trace) return;

	fprintf(trace, "\n%s: undkind=%d, vundkind=%d, unddel=%d, undap1=%d, undap2=%d,\n",
		s, undkind, vundkind, lineno(unddel), lineno(undap1), lineno(undap2));
	fprintf(trace, "  undadot=%d, dot=%d, dol=%d, unddol=%d, truedol=%d\n",
		lineno(undadot), lineno(dot), lineno(dol), lineno(unddol), lineno(truedol));
	fprintf(trace, "  [\n");
	lcpy(savelb, linebuf, sizeof savelb);
	fprintf(trace, "linebuf = '%s'\n", linebuf);
	for (p=zero+1; p<=truedol; p++) {
		fprintf(trace, "%o ", *p);
		getline(*p);
		fprintf(trace, "'%s'\n", linebuf);
	}
	fprintf(trace, "]\n");
	lcpy(linebuf, savelb, LBSIZE);
}
#endif

/*
 * Get a count from the keyed input stream.
 * A zero count is indistinguishable from no count.
 */
int 
vgetcnt(void)
{
	register int c, cnt;

	cnt = 0;
	for (;;) {
		c = getkey();
		if (!xisdigit(c))
			break;
		cnt *= 10, cnt += c - '0';
	}
	ungetkey(c);
	Xhadcnt = 1;
	Xcnt = cnt;
	return(cnt);
}

void 
trapalarm(int signum) {
	(void)signum;
	alarm(0);
	if (vcatch)
		LONGJMP(vreslab,1);
}

/*
 * fastpeekkey is just like peekkey but insists the character come in
 * fast (within 1 second). This will succeed if it is the 2nd char of
 * a machine generated sequence (such as a function pad from an escape
 * flavor terminal) but fail for a human hitting escape then waiting.
 */
int 
fastpeekkey(void)
{
	shand Oint;
	register int c;

	/*
	 * If the user has set notimeout, we wait forever for a key.
	 * If we are in a macro we do too, but since it's already
	 * buffered internally it will return immediately.
	 * In other cases we force this to die in 1 second.
	 * This is pretty reliable (VMUNIX rounds it to .5 - 1.5 secs,
	 * but UNIX truncates it to 0 - 1 secs) but due to system delays
	 * there are times when arrow keys or very fast typing get counted
	 * as separate.  notimeout is provided for people who dislike such
	 * nondeterminism.
	 */
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"\nfastpeekkey: ",c);
#endif
	Oint = signal(SIGINT, trapalarm);
	if (value(TIMEOUT) && inopen >= 0) {
		signal(SIGALRM, trapalarm);
#ifdef MDEBUG
		alarm(10);
		if (trace)
			fprintf(trace, "set alarm ");
#else
		alarm(1);
#endif
	}
	CATCH
		c = peekkey();
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"[OK]",c);
#endif
		alarm(0);
	ONERR
		c = 0;
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"[TIMEOUT]",c);
#endif
	ENDCATCH
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"[fpk:%o]",c);
#endif
	signal(SIGINT,Oint);
	return(c);
}
