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
static char sccsid[] = "@(#)ex_put.c	1.35 (gritter) 12/25/06";
#endif
#endif

/* from ex_put.c	7.9.1 (2.11BSD GTE) 12/9/94 */

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Terminal driving and line formatting routines.
 * Basic motion optimizations are done here as well
 * as formatting of lines (printing of control characters,
 * line numbering and the like).
 */

/*
 * The routines outchar, putchar and pline are actually
 * variables, and these variables point at the current definitions
 * of the routines.  See the routine setflav.
 * We sometimes make outchar be routines which catch the characters
 * to be printed, e.g. if we want to see how long a line is.
 * During open/visual, outchar and putchar will be set to
 * routines in the file ex_vput.c (vputchar, vinschar, etc.).
 */
int	(*Outchar)(int) = termchar;
int	(*Putchar)(int) = normchar;
int	(*Pline)(int, int) = normline;

int (*
setlist(int t))(int)
{
	register int (*P)(int);

	listf = t;
	P = Putchar;
	Putchar = t ? listchar : normchar;
	return (P);
}

int (*
setnumb(int t))(int, int)
{
	register int (*P)(int, int);

	numberf = t;
	P = Pline;
	Pline = t ? numbline : normline;
	return (P);
}

/*
 * Format c for list mode; leave things in common
 * with normal print mode to be done by normchar.
 */
int 
listchar(int c)
{

	if (c & MULTICOL) {
		c &= ~MULTICOL;
		if (c == 0)
			return MULTICOL;
	}
	c &= (TRIM|QUOTE);
	switch (c) {
	case '\t':
	case '\b':
		c = ctlof(c);
		outchar('^');
		break;

	case '\n':
		break;

	default:
		if (c == ('\n' | QUOTE))
			outchar('$');
		if (c & QUOTE)
			break;
#ifndef	BIT8
		if (c < ' ' && c != '\n')
			outchar('^'), c = ctlof(c);
#else	/* !BIT8 */
		if ((!printable(c) && c != '\n') || c == DELETE)
			c = printof(c);
#endif
		break;
	}
	return normchar(c);
}

/*
 * Format c for printing. Handle funnies of upper case terminals
 * and crocky hazeltines which don't have ~.
 */
int
normchar(register int c)
{
	int	u;

#ifdef	UCVISUAL
	register char *colp;

	if (c == '~' && xHZ) {
		normchar('\\');
		c = '^';
	}
#endif

	if (c & MULTICOL) {
		c &= ~MULTICOL;
		if (c == 0)
			return MULTICOL;
	}
	c &= (TRIM|QUOTE);
	u = c & TRIM;
	if (c & QUOTE) {
		if (c == (' ' | QUOTE) || c == ('\b' | QUOTE))
			/*EMPTY*/;
		else if (c == QUOTE)
			return c;
		else
			c &= TRIM;
	}
#ifdef	BIT8
	else {
		if (!printable(c) && (u != '\b' || !OS) &&
				u != '\n' && u != '\t')
			c = printof(u);
		else {
			c = u;
			if (0)
				/*EMPTY*/;
#else	/* !BIT8 */
	else if (c < ' ' && (c != '\b' || !OS) && c != '\n' && c != '\t')
		putchar('^'), c = ctlof(c);
#endif	/* !BIT8 */
#ifdef	UCVISUAL
	else if (UPPERCASE) {
		if (xisupper(c)) {
			outchar('\\');
			c = tolower(c);
		} else {
			colp = "({)}!|^~'`";
			while (*colp++)
				if (c == *colp++) {
					outchar('\\');
					c = colp[-2];
					break;
				}
		}
	}
#endif	/* UCVISUAL */
#ifdef	BIT8
		}
	}
#endif
	outchar(c);
	return c;
}

/*
 * Given c at the beginning of a line, determine whether
 * the printing of the line will erase or otherwise obliterate
 * the prompt which was printed before.  If it won't, do it now.
 */
void
slobber(int c)
{

	shudclob = 0;
	switch (c) {

	case '\t':
		if (Putchar == listchar)
			return;
		break;

	default:
		return;

	case ' ':
	case 0:
		break;
	}
	if (OS)
		return;
	flush();
	putch(' ');
	if (BC)
		tputs(BC, 0, putch);
	else
		putch('\b');
}

/*
 * Print a line with a number.
 */
int
numbline(int i, int max)
{

	extern short	vcntcol;
	if (shudclob)
		slobber(' ');
	max -= ex_printf("%6d  ", i);
	if (max > 0)
		vcntcol = value(NUMBER) << 3;
	return normline(0, max);
}

/*
 * Normal line output, no numbering.
 */
int
normline(int unused, int max)
{
	extern short	vcntcol, lastsc;
	short	ovc = -1;
	register char *cp;
	int	(*OO)(int);
	int	c, n;
	int	ret = 0;

	(void)unused;
	if (shudclob)
		slobber(linebuf[0]);
	/* pdp-11 doprnt is not reentrant so can't use "printf" here
	   in case we are tracing */
	cp = linebuf;
	vcolbp = cp;
	while (*cp && max) {
		vcolbp = cp;
		nextc(c, cp, n);
		cp += n;
		if (max > 0) {
			if (Outchar != qcount) {
				OO = Outchar;
				Outchar = qcount;
				putchar(c);
				Outchar = OO;
			} else
				putchar(c);
			if ((vcntcol-1) % WCOLS == 0 && lastsc > 1)
				vcntcol++;
			if (vcntcol >= max) {
				putchar('@');
				vcntcol = ovc + 1;
				lastsc = 1;
				ret = 1;
				break;
			}
			ovc = vcntcol;
			if (Outchar != qcount)
				putchar(c);
		} else
			putchar(c);
	}
	if (!inopen) {
		putchar('\n' | QUOTE);
	}
	return ret;
}

/*
 * The output buffer is initialized with a useful error
 * message so we don't have to keep it in data space.
 */
static	char linb[66+MB_LEN_MAX];
char *linp = linb;

/*
 * Phadnl records when we have already had a complete line ending with \n.
 * If another line starts without a flush, and the terminal suggests it,
 * we switch into -nl mode so that we can send lineffeeds to avoid
 * a lot of spacing.
 */
static	bool phadnl;

/*
 * Indirect to current definition of putchar.
 */
int
putchar(int c)
{
	if (c & MULTICOL) {
		c &= ~MULTICOL;
		if (c == 0)
			return MULTICOL;
	}
	(*Putchar)(c);
	return c;
}

/*
 * Termchar routine for command mode.
 * Watch for possible switching to -nl mode.
 * Otherwise flush into next level of buffering when
 * small buffer fills or at a newline.
 */
int
termchar(int c)
{

	if (pfast == 0 && phadnl)
		pstart();
	if (c == '\n')
		phadnl = 1;
	else if (linp >= &linb[63])
		flush1();
#ifdef	MB
	if (mb_cur_max > 1 && c & ~(wchar_t)0177) {
		char	mb[MB_LEN_MAX];
		int	i, n;
		n = wctomb(mb, c&TRIM);
		for (i = 0; i < n; i++)
			*linp++ = mb[i];
	} else
#endif	/* MB */
		*linp++ = c;
	if (linp >= &linb[63]) {
		fgoto();
		flush1();
	}
	return c;
}

void
flush2(void)
{

	fgoto();
	flusho();
	pstop();
}

void
flush(void)
{

	flush1();
	flush2();
}

/*
 * Flush from small line buffer into output buffer.
 * Work here is destroying motion into positions, and then
 * letting fgoto do the optimized motion.
 */
void
flush1(void)
{
	register char *lp;
	int c, n;

	*linp = 0;
	lp = linb;
	while (*lp) {
		nextc(c, lp, n);
		lp += n;
		switch (c) {

		case '\r':
			destline += destcol / TCOLUMNS;
			destcol = 0;
			continue;

		case '\b':
			if (destcol)
				destcol--;
			continue;

		case ' ':
			destcol++;
			continue;

		case '\t':
			destcol += value(TABSTOP) - destcol % value(TABSTOP);
			continue;

		case '\n':
			destline += destcol / TCOLUMNS + 1;
			if (destcol != 0 && destcol % TCOLUMNS == 0)
				destline--;
			destcol = 0;
			continue;

		default:
			fgoto();
			for (;;) {
				if (AM == 0 && outcol == TCOLUMNS)
					fgoto();
				c &= TRIM;
				putch(c);
				if (c == '\b') {
					outcol--;
					destcol--;
#ifndef	BIT8
				} else if ( c >= ' ' && c != DELETE) {
#else
				} else if (printable(c)) {
#endif
#ifdef	MB
					n = colsc(c);
					outcol += n;
					destcol += n;
#else	/* !MB */
					outcol++;
					destcol++;
#endif	/* !MB */
					if (XN && outcol % TCOLUMNS == 0)
						putch('\r'), putch('\n');
				}
				nextc(c, lp, n);
				lp += n;
#ifndef BIT8
				if (c <= ' ')
#else
				if (c == ' ' || !printable(c))
#endif
					break;
			}
			--lp;
			continue;
		}
	}
	linp = linb;
}

static int plodcnt, plodflg;

/*
 * Move (slowly) to destination.
 * Hard thing here is using home cursor on really deficient terminals.
 * Otherwise just use cursor motions, hacking use of tabs and overtabbing
 * and backspace.
 */

int
plodput(int c)
{

	if (plodflg)
		plodcnt--;
	else
		putch(c);
	return c;
}

int
plod(int cnt)
{
	register int i, j, k = 0;
	register int soutcol, soutline;

	plodcnt = plodflg = cnt;
	soutcol = outcol;
	soutline = outline;
	/*
	 * Consider homing and moving down/right from there, vs moving
	 * directly with local motions to the right spot.
	 */
	if (HO) {
		/*
		 * i is the cost to home and tab/space to the right to
		 * get to the proper column.  This assumes ND space costs
		 * 1 char.  So i+destcol is cost of motion with home.
		 */
		if (GT)
			i = (destcol / value(HARDTABS)) + (destcol % value(HARDTABS));
		else
			i = destcol;
		/*
		 * j is cost to move locally without homing
		 */
		if (destcol >= outcol) {	/* if motion is to the right */
			j = destcol / value(HARDTABS) - outcol / value(HARDTABS);
			if (GT && j)
				j += destcol % value(HARDTABS);
			else
				j = destcol - outcol;
		} else
			/* leftward motion only works if we can backspace. */
			if (outcol - destcol <= i && (BS || BC))
				i = j = outcol - destcol; /* cheaper to backspace */
			else
				j = i + 1; /* impossibly expensive */

		/* k is the absolute value of vertical distance */
		k = outline - destline;
		if (k < 0)
			k = -k;
		j += k;

		/*
		 * Decision.  We may not have a choice if no UP.
		 */
		if (i + destline < j || (!UP && destline < outline)) {
			/*
			 * Cheaper to home.  Do it now and pretend it's a
			 * regular local motion.
			 */
			tputs(HO, 0, plodput);
			outcol = outline = 0;
		} else if (LL) {
			/*
			 * Quickly consider homing down and moving from there.
			 * Assume cost of LL is 2.
			 */
			k = (TLINES - 1) - destline;
			if (i + k + 2 < j && (k<=0 || UP)) {
				tputs(LL, 0, plodput);
				outcol = 0;
				outline = TLINES - 1;
			}
		}
	} else
	/*
	 * No home and no up means it's impossible, so we return an
	 * incredibly big number to make cursor motion win out.
	 */
		if (!UP && destline < outline)
			return (500);
	if (GT)
		i = destcol % value(HARDTABS)
		    + destcol / value(HARDTABS);
	else
		i = destcol;
/*
	if (BT && outcol > destcol && (j = (((outcol+7) & ~7) - destcol - 1) >> 3)) {
		j *= (k = strlen(BT));
		if ((k += (destcol&7)) > 4)
			j += 8 - (destcol&7);
		else
			j += k;
	} else
*/
		j = outcol - destcol;
	/*
	 * If we will later need a \n which will turn into a \r\n by
	 * the system or the terminal, then don't bother to try to \r.
	 */
	if ((NONL || !pfast) && outline < destline)
		goto dontcr;
	/*
	 * If the terminal will do a \r\n and there isn't room for it,
	 * then we can't afford a \r.
	 */
	if (NC && outline >= destline)
		goto dontcr;
	/*
	 * If it will be cheaper, or if we can't back up, then send
	 * a return preliminarily.
	 */
	if (j > i + 1 || (outcol > destcol && !BS && !BC)) {
		/*
		 * BUG: this doesn't take the (possibly long) length
		 * of xCR into account.
		 */
		if (ospeed != B0) {
			if (xCR)
				tputs(xCR, 0, plodput);
			else
				plodput('\r');
		}
		if (NC) {
			if (xNL)
				tputs(xNL, 0, plodput);
			else
				plodput('\n');
			outline++;
		}
		outcol = 0;
	}
dontcr:
	/* Move down, if necessary, until we are at the desired line */
	while (outline < destline) {
		j = destline - outline;
		if (j > costDP && DOWN_PARM) {
			/* Win big on Tek 4025 */
			tputs(tgoto(DOWN_PARM, 0, j), j, plodput);
			outline += j;
		}
		else {
			outline++;
			if (xNL && pfast)
				tputs(xNL, 0, plodput);
			else
				plodput('\n');
		}
		if (plodcnt < 0)
			goto out;
		if (NONL || pfast == 0)
			outcol = 0;
	}
	if (BT)
		k = strlen(BT);	/* should probably be cost(BT) and moved out */
	/* Move left, if necessary, to desired column */
	while (outcol > destcol) {
		if (plodcnt < 0)
			goto out;
		if (BT && !insmode && outcol - destcol > 4+k) {
			tputs(BT, 0, plodput);
			outcol--;
			outcol -= outcol % value(HARDTABS); /* outcol &= ~7; */
			continue;
		}
		j = outcol - destcol;
		if (j > costLP && LEFT_PARM) {
			tputs(tgoto(LEFT_PARM, 0, j), j, plodput);
			outcol -= j;
		}
		else {
			outcol--;
			if (BC)
				tputs(BC, 0, plodput);
			else
				plodput('\b');
		}
	}
	/* Move up, if necessary, to desired row */
	while (outline > destline) {
		j = outline - destline;
		if (UP_PARM && j > 1) {
			/* Win big on Tek 4025 */
			tputs(tgoto(UP_PARM, 0, j), j, plodput);
			outline -= j;
		}
		else {
			outline--;
			tputs(UP, 0, plodput);
		}
		if (plodcnt < 0)
			goto out;
	}
	/*
	 * Now move to the right, if necessary.  We first tab to
	 * as close as we can get.
	 */
	if (GT && !insmode && destcol - outcol > 1) {
		/* tab to right as far as possible without passing col */
		for (;;) {
			i = tabcol(outcol, value(HARDTABS));
			if (i > destcol)
				break;
			if (TA)
				tputs(TA, 0, plodput);
			else
				plodput('\t');
			outcol = i;
		}
		/* consider another tab and then some backspaces */
		if (destcol - outcol > 4 && i < TCOLUMNS && (BC || BS)) {
			if (TA)
				tputs(TA, 0, plodput);
			else
				plodput('\t');
			outcol = i;
			/*
			 * Back up.  Don't worry about LEFT_PARM because
			 * it's never more than 4 spaces anyway.
			 */
			while (outcol > destcol) {
				outcol--;
				if (BC)
					tputs(BC, 0, plodput);
				else
					plodput('\b');
			}
		}
	}
	/*
	 * We've tabbed as much as possible.  If we still need to go
	 * further (not exact or can't tab) space over.  This is a
	 * very common case when moving to the right with space.
	 */
	while (outcol < destcol) {
		j = destcol - outcol;
		if (j > costRP && RIGHT_PARM) {
			/*
			 * This probably happens rarely, if at all.
			 * It seems mainly useful for ANSI terminals
			 * with no hardware tabs, and I don't know
			 * of any such terminal at the moment.
			 */
			tputs(tgoto(RIGHT_PARM, 0, j), j, plodput);
			outcol += j;
		}
		else {
			/*
			 * move one char to the right.  We don't use ND space
			 * because it's better to just print the char we are
			 * moving over.  There are various exceptions, however.
			 * If !inopen, vtube contains garbage.  If the char is
			 * a null or a tab we want to print a space.  Other
			 * random chars we use space for instead, too.
			 */
			if (!inopen || vtube[outline]==NULL ||
#ifndef	BIT8
				((i=vtube[outline][outcol]) < ' ')
#else
				((i=vtube[outline][outcol]) == 0)
					|| (i!=MULTICOL && !printable(i&~INVBIT&~MULTICOL))
#endif
				)
				i = ' ';
			if((i & (QUOTE|INVBIT)) == QUOTE) /* mjm: no sign
							     extension on 3B */
				i = ' ';
			if ((insmode || i == MULTICOL) && ND)
				tputs(ND, 0, plodput);
			else if (i == MULTICOL) {
				if (BS && BC)
					tputs(BC, 0, plodput);
				else
					plodput('\b');
				plodput(vtube[outline][outcol-1]);
			} else
				plodput(i);
			outcol += i == MULTICOL ? 1 : colsc(i & ~MULTICOL);
		}
		if (plodcnt < 0)
			goto out;
	}
out:
	if (plodflg) {
		outcol = soutcol;
		outline = soutline;
	}
	return(plodcnt);
}

/*
 * Sync the position of the output cursor.
 * Most work here is rounding for terminal boundaries getting the
 * column position implied by wraparound or the lack thereof and
 * rolling up the screen to get destline on the screen.
 */
void
fgoto(void)
{
	register int l, c;

	if (destcol > TCOLUMNS - 1) {
		destline += destcol / TCOLUMNS;
		destcol %= TCOLUMNS;
	}
	if (outcol > TCOLUMNS - 1) {
		l = (outcol + 1) / TCOLUMNS;
		outline += l;
		outcol %= TCOLUMNS;
		if (AM == 0) {
			while (l > 0) {
				if (pfast && ospeed != B0) {
					if (xCR)
						tputs(xCR, 0, putch);
					else
						putch('\r');
				}
				if (xNL)
					tputs(xNL, 0, putch);
				else
					putch('\n');
				l--;
			}
			outcol = 0;
		}
		if (outline > TLINES - 1) {
			destline -= outline - (TLINES - 1);
			outline = TLINES - 1;
		}
	}
	if (destline > TLINES - 1) {
		l = destline;
		destline = TLINES - 1;
		if (outline < TLINES - 1) {
			c = destcol;
			if (pfast == 0 && (!CA || holdcm))
				destcol = 0;
			fgoto();
			destcol = c;
		}
		while (l > TLINES - 1) {
			/*
			 * The following linefeed (or simulation thereof)
			 * is supposed to scroll up the screen, since we
			 * are on the bottom line.  We make the assumption
			 * that linefeed will scroll.  If ns is in the
			 * capability list this won't work.  We should
			 * probably have an sc capability but sf will
			 * generally take the place if it works.
			 *
			 * Superbee glitch:  in the middle of the screen we
			 * have to use esc B (down) because linefeed screws up
			 * in "Efficient Paging" (what a joke) mode (which is
			 * essential in some SB's because CRLF mode puts garbage
			 * in at end of memory), but you must use linefeed to
			 * scroll since down arrow won't go past memory end.
			 * I turned this off after recieving Paul Eggert's
			 * Superbee description which wins better.
			 */
			if (xNL /* && !XB */ && pfast)
				tputs(xNL, 0, putch);
			else
				putch('\n');
			l--;
			if (pfast == 0)
				outcol = 0;
		}
	}
	if (destline < outline && !((CA && !holdcm) || UP != NOSTR))
		destline = outline;
	if (CA && !holdcm)
		if (plod(costCM) > 0)
			plod(0);
		else
			tputs(tgoto(CM, destcol, destline), 0, putch);
	else
		plod(0);
	outline = destline;
	outcol = destcol;
}

/*
 * Tab to column col by flushing and then setting destcol.
 * Used by "set all".
 */
void
tab(int col)
{

	flush1();
	destcol = col;
}

/*
 * An input line arrived.
 * Calculate new (approximate) screen line position.
 * Approximate because kill character echoes newline with
 * no feedback and also because of long input lines.
 */
void
noteinp(void)
{

	outline++;
	if (outline > TLINES - 1)
		outline = TLINES - 1;
	destline = outline;
	destcol = outcol = 0;
}

/*
 * Something weird just happened and we
 * lost track of whats happening out there.
 * Since we cant, in general, read where we are
 * we just reset to some known state.
 * On cursor addressible terminals setting to unknown
 * will force a cursor address soon.
 */
void
termreset(void)
{

	endim();
	if (TI)	/* otherwise it flushes anyway, and 'set tty=dumb' vomits */
		putpad(TI);	 /*adb change -- emit terminal initial sequence */
	destcol = 0;
	destline = TLINES - 1;
	if (CA) {
		outcol = UKCOL;
		outline = UKCOL;
	} else {
		outcol = destcol;
		outline = destline;
	}
}

/*
 * Low level buffering, with the ability to drain
 * buffered output without printing it.
 */
char	*obp = obuf;

void
draino(void)
{

	obp = obuf;
}

void
flusho(void)
{

	if (obp != obuf) {
		write(1, obuf, obp - obuf);
		obp = obuf;
	}
}

void
putnl(void)
{

	putchar('\n');
}

void
putS(char *cp)
{

	if (cp == NULL)
		return;
	while (*cp)
		putch(*cp++);
}


int
putch(int c)
{

#ifdef OLD3BTTY		/* mjm */
	if(c == '\n')	/* mjm: Fake "\n\r" for '\n' til fix in 3B firmware */
		putch('\r');	/* mjm: vi does "stty -icanon" => -onlcr !! */
#endif
	if (c & MULTICOL) {
		c &= ~MULTICOL;
		if (c == 0)
			return MULTICOL;
	}
	c &= ~INVBIT;	/* strip '~' | INVBIT multicolumn filler */
#ifdef	MB
	if (mb_cur_max > 1 && c & ~(wchar_t)0177) {
		char	mb[MB_LEN_MAX];
		int	i, n;
		n = wctomb(mb, c&TRIM);
		for (i = 0; i < n; i++) {
			*obp++ = mb[i];
			if (obp >= &obuf[sizeof obuf])
				flusho();
		}
	} else
#endif	/* MB */
		*obp++ = c & TRIM;
	if (obp >= &obuf[sizeof obuf])
		flusho();
	return c;
}

/*
 * Miscellaneous routines related to output.
 */

/*
 * Put with padding
 */
void
putpad(char *cp)
{

	flush();
	tputs(cp, 0, putch);
}

/*
 * Set output through normal command mode routine.
 */
void
setoutt(void)
{

	Outchar = termchar;
}

/*
 * Printf (temporarily) in list mode.
 */
/*VARARGS2*/
void
vlprintf(char *cp, va_list ap)
{
	register int (*P)(int);

	P = setlist(1);
	vprintf(cp, ap);
	Putchar = P;
}

void
lprintf(char *cp, ...)
{
	va_list ap;

	va_start(ap, cp);
	vlprintf(cp, ap);
	va_end(ap);
}

/*
 * Newline + flush.
 */
void
putNFL(void)
{

	putnl();
	flush();
}

/*
 * sTTY: set the tty modes on file descriptor i to be what's
 * currently in global "tty".  (Also use nttyc if needed.)
 */
void
sTTY(int i)
{

	tcsetattr(i, TCSADRAIN, &tty);
}

/*
 * Try to start -nl mode.
 */
void
pstart(void)
{

	if (NONL)
		return;
 	if (!value(OPTIMIZE))
		return;
	if (ruptible == 0 || pfast)
		return;
	fgoto();
	flusho();
	pfast = 1;
	normtty++;
	tty = normf;
	tty.c_oflag &= ~(ONLCR
#if defined (TAB3)
			| TAB3
#elif defined (XTABS)
			| XTABS
#endif
			);
	tty.c_lflag &= ~ECHO;
	sTTY(1);
}

/*
 * Stop -nl mode.
 */
void
pstop(void)
{

	if (inopen)
		return;
	phadnl = 0;
	linp = linb;
	draino();
	normal(normf);
	pfast &= ~1;
}

/*
 * Turn off start/stop chars if they aren't the default ^S/^Q.
 * This is so idiots who make esc their start/stop don't lose.
 * We always turn off quit since datamedias send ^\ for their
 * right arrow key.
 */
void
ttcharoff(void)
{
#ifdef	_PC_VDISABLE
	long vdis;

	errno = 0;
#ifndef	__dietlibc__
	vdis = fpathconf(1, _PC_VDISABLE);
	if (errno)
		/*
		 * Use the old value of 0377, hope it is not
		 * the user's favourite character.
		 */
#endif	/* !__dietlibc__ */
		vdis = '\377';
#else	/* !_PC_VDISABLE */
#define	vdis	'\377';
#endif	/* !_PC_VDISABLE */
	tty.c_cc[VQUIT] = vdis;
#ifdef	VSUSP
	tty.c_cc[VSUSP] = vdis;
#endif
#ifdef	VDSUSP
	tty.c_cc[VDSUSP] = vdis;
#endif
#ifdef	VREPRINT
	tty.c_cc[VREPRINT] = vdis;
#endif
#ifdef	VDISCRD
	tty.c_cc[VDISCRD] = vdis;
#endif
#ifdef	VWERASE
	tty.c_cc[VWERASE] = vdis;
#endif
#ifdef	VLNEXT
	tty.c_cc[VLNEXT] = vdis;
#endif
#ifdef	VSTATUS
	tty.c_cc[VSTATUS] = vdis;
#endif
	/*
	 * The following is sample code if USG ever lets people change
	 * their start/stop chars.  As long as they can't we can't get
	 * into trouble so we just leave them alone.
	 */
# ifdef VSTART
	if (tty.c_cc[VSTART] != CTRL('q'))
		tty.c_cc[VSTART] = vdis;
# endif
# ifdef VSTOP
	if (tty.c_cc[VSTOP] != CTRL('s'))
		tty.c_cc[VSTOP] = vdis;
# endif
}

/*
 * Prep tty for open mode.
 */
struct termios
ostart(void)
{
	struct termios f;

	if (!intty)
		error(catgets(catd, 1, 120,
				"Open and visual must be used interactively"));
	gTTY(1);
	normtty++;
	f = tty;
	tty = normf;
	tty.c_iflag &= ~ICRNL;
	tty.c_lflag &= ~(ECHO|ICANON);
	tty.c_oflag &= ~(ONLCR
#if defined (TAB3)
			| TAB3
#elif defined (XTABS)
			| XTABS
#endif
			);
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;
	ttcharoff();
	sTTY(1);
	tostart();
	pfast |= 2;
	return (f);
}

/* actions associated with putting the terminal in open mode */
void
tostart(void)
{
	putpad(VS);
	putpad(KS);
	if (!value(MESG)) {
		if (ttynbuf[0] == 0) {
			register char *tn;
			if ((tn=ttyname(2)) == NULL &&
			    (tn=ttyname(1)) == NULL &&
			    (tn=ttyname(0)) == NULL)
				ttynbuf[0] = 1;
			else
				safecp(ttynbuf, tn, sizeof ttynbuf,
						"%s too long", tn);
		}
		if (ttynbuf[0] != 1) {
			struct stat sbuf;
			stat(ttynbuf, &sbuf);
			ttymesg = sbuf.st_mode & 0777;
			chmod(ttynbuf,
#ifdef UCBV7
	/*
	 * This applies to the UCB V7 Pdp-11 system with the
	 * -u write option only.
	 */
					0611	/* 11 = urgent only allowed */
#else
					0600
#endif
						);
		}
	}
}

/*
 * Stop open, restoring tty modes.
 */
void
ostop(struct termios f)
{

	pfast = (f.c_oflag & ONLCR) == 0;
	termreset(), fgoto(), flusho();
	normal(f);
	tostop();
}

/* Actions associated with putting the terminal in the right mode. */
void
tostop(void)
{
	putpad(VE);
	putpad(KE);
	if (!value(MESG) && ttynbuf[0]>1)
		chmod(ttynbuf, ttymesg);
}

/*
 * Restore flags to normal state f.
 */
void
normal(struct termios f)
{

	if (normtty > 0) {
		setty(f);
		normtty--;
	}
}

/*
 * Straight set of flags to state f.
 */
struct termios
setty(struct termios f)
{
	struct termios ot;
	ot = tty;

	if (tty.c_lflag & ICANON)
		ttcharoff();
	tty = f;
	sTTY(1);
	return (ot);
}

void
gTTY(int i)
{

	tcgetattr(i, &tty);
}

/*
 * Print newline, or blank if in open/visual
 */
void
noonl(void)
{

	putchar(Outchar != termchar ? ' ' : '\n');
}
