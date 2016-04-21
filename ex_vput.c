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
static char sccsid[] = "@(#)ex_vput.c	1.52 (gritter) 12/25/06";
#endif
#endif

/* from ex_vput.c	7.4.1 (2.11BSD GTE) 12/9/94 */

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Deal with the screen, clearing, cursor positioning, putting characters
 * into the screen image, and deleting characters.
 * Really hard stuff here is utilizing insert character operations
 * on intelligent terminals which differs widely from terminal to terminal.
 */
void 
vclear(void)
{

#ifdef ADEBUG
	if (trace)
		tfixnl(), fprintf(trace, "------\nvclear\n");
#endif
	tputs(CL, TLINES, putch);
	destcol = 0;
	outcol = 0;
	destline = 0;
	outline = 0;
	if (inopen)
		vclrcell(vtube0, WCOLS * (WECHO - ZERO + 1));
}

/*
 * Clear memory.
 */
void 
vclrcell(register cell *cp, register int i)
{
	if (i > 0)
		do
			*cp++ = 0;
		while (--i != 0);
}

/*
 * Clear a physical display line, high level.
 */
void 
vclrlin(int l, line *tp)
{

	vigoto(l, 0);
	if ((hold & HOLDAT) == 0)
#ifndef	UCVISUAL
		putchar(tp > dol ? '~' : '@');
#else
		putchar(tp > dol ? ((UPPERCASE || xHZ) ? '^' : '~') : '@');
#endif
	if (state == HARDOPEN)
		sethard();
	vclreol();
}

/*
 * Clear to the end of the current physical line
 */
void
vclreol(void)
{
	register int i, j;
	register cell *tp;

	if (destcol == WCOLS)
		return;
	destline += destcol / WCOLS;
	destcol %= WCOLS;
	if (destline < 0 || destline > WECHO)
		error(catgets(catd, 1, 237, "Internal error: vclreol"));
	i = WCOLS - destcol;
	tp = vtube[destline] + destcol;
	if (CE) {
		if ((IN && *tp) || !ateopr()) {
			vcsync();
			vputp(CE, 1);
		}
		vclrcell(tp, i);
		return;
	}
	if (*tp == 0)
		return;
	while (i > 0 && (j = *tp & (QUOTE|TRIM|MULTICOL))) {
		if ((j != ' ' && (j & QUOTE) == 0)) {
			destcol = WCOLS - i;
			vputchar(' ');
		}
		--i, *tp++ = 0;
	}
}

/*
 * Clear the echo line.
 * If didphys then its been cleared physically (as
 * a side effect of a clear to end of display, e.g.)
 * so just do it logically.
 * If work here is being held off, just remember, in
 * heldech, if work needs to be done, don't do anything.
 */
void
vclrech(bool didphys)
{

	if (Peekkey == ATTN)
		return;
	if (hold & HOLDECH) {
		heldech = !didphys;
		return;
	}
	if (!didphys && (CD || CE)) {
		splitw++;
		/*
		 * If display is retained below, then MUST use CD or CE
		 * since we don't really know whats out there.
		 * Vigoto might decide (incorrectly) to do nothing.
		 */
		if (DB) {
			vgoto(WECHO, 0);
			vputp(CD ? CD : CE, 1);
		} else {
			if (XT) {
				/*
				 * This code basically handles the t1061
				 * where positioning at (0, 0) won't work
				 * because the terminal won't let you put
				 * the cursor on it's magic cookie.
				 *
				 * Should probably be XS above, or even a
				 * new X? glitch, but right now t1061 is the
				 * only terminal with XT.
				 */
				vgoto(WECHO, 0);
				vputp(DL, 1);
			} else {
				vigoto(WECHO, 0);
				vclreol();
			}
		}
		splitw = 0;
		didphys = 1;
	}
	if (didphys && vtube)
		vclrcell(vtube[WECHO], WCOLS);
	heldech = 0;
}

/*
 * Fix the echo area for use, setting
 * the state variable splitw so we wont rollup
 * when we move the cursor there.
 */
void
fixech(void)
{

	splitw++;
	if (state != VISUAL && state != CRTOPEN) {
		vclean();
		vcnt = 0;
	}
	vgoto(WECHO, 0); flusho();
}

/*
 * Put the cursor ``before'' cp.
 */
void
vcursbef(register char *cp)
{

	if (cp <= linebuf)
		vgotoCL(value(NUMBER) << 3);
	else
		vgotoCL(column(cp - 1) - 1);
}

/*
 * Put the cursor ``at'' cp.
 */
void
vcursat(register char *cp)
{

	if (cp <= linebuf && linebuf[0] == 0)
		vgotoCL(value(NUMBER) << 3);
	else
		vgotoCL(column(cp + skipleft(linebuf, cp)));
}

/*
 * Put the cursor ``after'' cp.
 */
void
vcursaft(register char *cp)
{

	vgotoCL(column(cp));
}

/*
 * Fix the cursor to be positioned in the correct place
 * to accept a command.
 */
void
vfixcurs(void)
{

	vsetcurs(cursor);
}

/*
 * Compute the column position implied by the cursor at ``nc'',
 * and move the cursor there.
 */
void
vsetcurs(register char *nc)
{
	register int col;

	col = column(nc);
	if (linebuf[0])
		col--;
	vgotoCL(col);
	cursor = vcolbp;
}

/*
 * Move the cursor invisibly, i.e. only remember to do it.
 */
void
vigoto(int y, int x)
{

	destline = y;
	destcol = x;
}

/*
 * Move the cursor to the position implied by any previous
 * vigoto (or low level hacking with destcol/destline as in readecho).
 */
void
vcsync(void)
{

	vgoto(destline, destcol);
}

/*
 * Goto column x of the current line.
 */
void
vgotoCL(register int x)
{

	if (splitw)
		vgoto(WECHO, x);
	else
		vgoto(LINE(vcline), x);
}

/*
 * Invisible goto column x of current line.
 */
void
vigotoCL(register int x)
{

	if (splitw)
		vigoto(WECHO, x);
	else
		vigoto(LINE(vcline), x);
}

/*
 * Move cursor to line y, column x, handling wraparound and scrolling.
 */
void
vgoto(register int y, register int x)
{
	register cell *tp;
	register int c;

	/*
	 * Fold the possibly too large value of x.
	 */
	if (x >= WCOLS) {
		y += x / WCOLS;
		x %= WCOLS;
	}
#ifdef	MB
	if (y >= 0 && y <= WLINES && mb_cur_max > 1 && !insmode) {
		while (x > 0 && (vtube[y][x]&(MULTICOL|TRIM)) == MULTICOL &&
				vtube[y][x-1] & MULTICOL &&
				(vtube[y][x-1]&(MULTICOL|TRIM)) != MULTICOL)
			x--;
	}
#endif	/* MB */
	if (y < 0)
		error(catgets(catd, 1, 238, "Internal error: vgoto"));
	if (outcol >= WCOLS) {
		if (AM) {
			outline += outcol / WCOLS;
			outcol %= WCOLS;
		} else
			outcol = WCOLS - 1;
	}

	/*
	 * In a hardcopy or glass crt open, print the stuff
	 * implied by a motion, or backspace.
	 */
	if (state == HARDOPEN || state == ONEOPEN) {
		if (y != outline)
			error(catgets(catd, 1, 239, "Line too long for open"));
		if (x + 1 < outcol - x || (outcol > x && !BS))
			destcol = 0, fgoto();
		tp = vtube[WBOT] + outcol;
		while (outcol != x)
			if (outcol < x) {
				if (*tp == 0)
					*tp = ' ';
				c = *tp++ & TRIM;
				vputc(c && (!OS || EO) ? c : ' ');
				outcol++;
			} else {
				if (BC)
					vputp(BC, 0);
				else
					vputc('\b');
				outcol--;
			}
		destcol = outcol = x;
		destline = outline;
		return;
	}

	/*
	 * If the destination position implies a scroll, do it.
	 */
	destline = y;
	if (destline > WBOT && (!splitw || destline > WECHO)) {
		endim();
		vrollup(destline);
	}

	/*
	 * If there really is a motion involved, do it.
	 * The check here is an optimization based on profiling.
	 */
	destcol = x;
	if ((destline - outline) * WCOLS != destcol - outcol) {
		if (!MI)
			endim();
		fgoto();
	}
}

/*
 * This is the hardest code in the editor, and deals with insert modes
 * on different kinds of intelligent terminals.  The complexity is due
 * to the cross product of three factors:
 *
 *	1. Lines may display as more than one segment on the screen.
 *	2. There are 2 kinds of intelligent terminal insert modes.
 *	3. Tabs squash when you insert characters in front of them,
 *	   in a way in which current intelligent terminals don't handle.
 *
 * The two kinds of terminals are typified by the DM2500 or HP2645 for
 * one and the CONCEPT-100 or the FOX for the other.
 *
 * The first (HP2645) kind has an insert mode where the characters
 * fall off the end of the line and the screen is shifted rigidly
 * no matter how the display came about.
 *
 * The second (CONCEPT-100) kind comes from terminals which are designed
 * for forms editing and which distinguish between blanks and ``spaces''
 * on the screen, spaces being like blank, but never having had
 * and data typed into that screen position (since, e.g. a clear operation
 * like clear screen).  On these terminals, when you insert a character,
 * the characters from where you are to the end of the screen shift
 * over till a ``space'' is found, and the null character there gets
 * eaten up.
 *
 *
 * The code here considers the line as consisting of several parts
 * the first part is the ``doomed'' part, i.e. a part of the line
 * which is being typed over.  Next comes some text up to the first
 * following tab.  The tab is the next segment of the line, and finally
 * text after the tab.
 *
 * We have to consider each of these segments and the effect of the
 * insertion of a character on them.  On terminals like HP2645's we
 * must simulate a multi-line insert mode using the primitive one
 * line insert mode.  If we are inserting in front of a tab, we have
 * to either delete characters from the tab or insert white space
 * (when the tab reaches a new spot where it gets larger) before we
 * insert the new character.
 *
 * On a terminal like a CONCEPT our strategy is to make all
 * blanks be displayed, while trying to keep the screen having ``spaces''
 * for portions of tabs.  In this way the terminal hardward does some
 * of the hacking for compression of tabs, although this tends to
 * disappear as you work on the line and spaces change into blanks.
 *
 * There are a number of boundary conditions (like typing just before
 * the first following tab) where we can avoid a lot of work.  Most
 * of them have to be dealt with explicitly because performance is
 * much, much worse if we don't.
 *
 * A final thing which is hacked here is two flavors of insert mode.
 * Datamedia's do this by an insert mode which you enter and leave
 * and by having normal motion character operate differently in this
 * mode, notably by having a newline insert a line on the screen in
 * this mode.  This generally means it is unsafe to move around
 * the screen ignoring the fact that we are in this mode.
 * This is possible on some terminals, and wins big (e.g. HP), so
 * we encode this as a ``can move in insert capability'' mi,
 * and terminals which have it can do insert mode with much less
 * work when tabs are present following the cursor on the current line.
 */

/*
 * Routine to expand a tab, calling the normal Outchar routine
 * to put out each implied character.  Note that we call outchar
 * with a QUOTE.  We use QUOTE internally to represent a position
 * which is part of the expansion of a tab.
 */
void
vgotab(void)
{
	register int i = tabcol(destcol, value(TABSTOP)) - destcol;

	do
		(*Outchar)(QUOTE);
	while (--i);
}

/*
 * Variables for insert mode.
 */
int	linend;			/* The column position of end of line */
int	tabstart;		/* Column of start of first following tab */
int	tabend;			/* Column of end of following tabs */
int	tabsize;		/* Size of the following tabs */
int	tabslack;		/* Number of ``spaces'' in following tabs */
int	inssiz;			/* Number of characters to be inserted */
int	inscol;			/* Column where insertion is taking place */
int	insmc0;			/* Multi-column character before insertion */
int	insmc1;			/* Multi-column character at insertion */
int	shft;			/* Amount tab expansion shifted rest of line */
int	slakused;		/* This much of tabslack will be used up */

/*
 * This routine MUST be called before insert mode is run,
 * and brings all segments of the current line to the top
 * of the screen image buffer so it is easier for us to
 * maniuplate them.
 */
void
vprepins(void)
{
	register int i;
	register cell *cp = vtube0;

	for (i = 0; i < DEPTH(vcline); i++) {
		vmaktop(LINE(vcline) + i, cp);
		cp += WCOLS;
	}
}

void
vmaktop(register int p, cell *cp)
{
	register int i;
	cell temp[TUBECOLS];

	if (p < 0 || vtube[p] == cp)
		return;
	for (i = ZERO; i <= WECHO; i++)
		if (vtube[i] == cp) {
			copy(temp, vtube[i], WCOLS * sizeof *temp);
			copy(vtube[i], vtube[p], WCOLS * sizeof *temp);
			copy(vtube[p], temp, WCOLS * sizeof *temp);
			vtube[i] = vtube[p];
			vtube[p] = cp;
			return;
		}
	error(catgets(catd, 1, 240, "Line too long"));
}

/*
 * Insert character c at current cursor position.
 * Multi-character inserts occur only as a result
 * of expansion of tabs (i.e. inssize == 1 except
 * for tabs) and code assumes this in several place
 * to make life simpler.
 */
int
vinschar(int c)
/*	int c;		/\* mjm: char --> int */
{
	register int i;
	register cell *tp;
	char	*OIM;
	bool	OXN;
	int	noim, filler = 0;

	insmc1 = colsc(c) - 1;
	if ((!IM || !EI) && ((hold & HOLDQIK) || !value(REDRAW) || value(SLOWOPEN))) {
		/*
		 * Don't want to try to use terminal
		 * insert mode, or to try to fake it.
		 * Just put the character out; the screen
		 * will probably be wrong but we will fix it later.
		 */
		if (c == '\t') {
			vgotab();
			return c;
		}
		vputchar(c);
#ifdef	MB
		if (insmc1 == 0 && (vtube0[destcol]&(TRIM|MULTICOL))==MULTICOL)
			vtube0[destcol] = INVBIT;
#endif	/* MB */
		if (DEPTH(vcline) * WCOLS + !value(REDRAW) >
		    (destline - LINE(vcline)) * WCOLS + destcol)
			return c;
		/*
		 * The next line is about to be clobbered
		 * make space for another segment of this line
		 * (on an intelligent terminal) or just remember
		 * that next line was clobbered (on a dumb one
		 * if we don't care to redraw the tail.
		 */
		if (AL) {
			vnpins(0);
		} else {
			c = LINE(vcline) + DEPTH(vcline);
			if (c < LINE(vcline + 1) || c > WBOT)
				return c;
			i = destcol;
			vinslin(c, 1, vcline);
			DEPTH(vcline)++;
			vigoto(c, i);
			vprepins();
		}
		return c;
	}
	/*
	 * Compute the number of positions in the line image of the
	 * current line.  This is done from the physical image
	 * since that is faster.  Note that we have no memory
	 * from insertion to insertion so that routines which use
	 * us don't have to worry about moving the cursor around.
	 */
	if (*vtube0 == 0)
		linend = 0;
	else {
		/*
		 * Search backwards for a non-null character
		 * from the end of the displayed line.
		 */
		i = WCOLS * DEPTH(vcline);
		if (i == 0)
			i = WCOLS;
		tp = vtube0 + i;
		while (*--tp == 0)
			if (--i == 0)
				break;
		linend = i + insmc1;
	}

	/*
	 * We insert at a position based on the physical location
	 * of the output cursor.
	 */
	inscol = destcol + (destline - LINE(vcline)) * WCOLS;
	insmc0 = 0;
#ifdef	MB
	i = 0;
	while (inscol+i < LBSIZE && vtube0[inscol+i]&MULTICOL &&
			(vtube0[inscol+insmc0+i]&(MULTICOL|TRIM)) != MULTICOL)
		i++;
	while (inscol+insmc0+i < LBSIZE &&
			(vtube0[inscol+insmc0+i]&(MULTICOL|TRIM)) == MULTICOL)
		insmc0++;
#endif	/* MB */
	if (c == '\t') {
		/*
		 * Characters inserted from a tab must be
		 * remembered as being part of a tab, but we can't
		 * use QUOTE here since we really need to print blanks.
		 * QUOTE|' ' is the representation of this.
		 */
		inssiz = tabcol(inscol+insmc0, value(TABSTOP)) - inscol - insmc0;
		c = ' ' | QUOTE;
	} else
		inssiz = 1;

	/*
	 * If the text to be inserted is less than the number
	 * of doomed positions, then we don't need insert mode,
	 * rather we can just typeover.
	 */
	if (inssiz + insmc1 <= doomed) {
		endim();
		if (inscol + insmc0 != linend)
			doomed -= inssiz + insmc1;
#ifdef	MB
		if (insmc1 == 0 && c != '\t' &&
				vtube0[inscol+insmc0] & MULTICOL)
			vtube0[inscol+insmc0] = INVBIT;
#endif	/* MB */
		do
			vputchar(c);
		while (--inssiz);
		return c;
	}

	/*
	 * Have to really do some insertion, thus
	 * stake out the bounds of the first following
	 * group of tabs, computing starting position,
	 * ending position, and the number of ``spaces'' therein
	 * so we can tell how much it will squish.
	 */
	tp = vtube0 + inscol + insmc0;
	for (i = inscol + insmc0; i < linend; i++) {
		if (*tp++ & QUOTE) {
			--tp;
			break;
		}
	}
	tabstart = tabend = i;
	tabslack = 0;
	while (tabend < linend) {
		i = *tp++;
		if ((i & QUOTE) == 0)
			break;
		if ((i & (TRIM|MULTICOL)) == 0)
			tabslack++;
		tabsize++;
		tabend++;
	}
	tabsize = tabend - tabstart;

	/*
	 * For HP's and DM's, e.g. tabslack has no meaning.
	 */
	if (!IN)
		tabslack = 0;
#ifdef IDEBUG
	if (trace) {
		fprintf(trace, "inscol %d, inssiz %d, tabstart %d, ",
			inscol, inssiz, tabstart);
		fprintf(trace, "tabend %d, tabslack %d, linend %d\n",
			tabend, tabslack, linend);
	}
#endif
	OIM = IM;
	OXN = XN;
	noim = 0;
#ifdef	MB
	if (mb_cur_max > 1) {
		if (destcol + 1 + insmc1 == WCOLS + 1) {
			noim = 1;
			if (insmc1 == 1 && insmc0 == 0)
				filler = 1;
		}
		for (i = inscol; vtube0[i]; i++)
			if (i + 1 >= WCOLS && vtube0[i] & MULTICOL) {
				noim = 1;
				break;
			}
	}
#endif	/* MB */
	if (noim) {
		endim();
		IM = 0;
		XN = 0;
	}

	/*
	 * The real work begins.
	 */
	slakused = 0;
	shft = 0;
	if (tabsize) {
		/*
		 * There are tabs on this line.
		 * If they need to expand, then the rest of the line
		 * will have to be shifted over.  In this case,
		 * we will need to make sure there are no ``spaces''
		 * in the rest of the line (on e.g. CONCEPT-100)
		 * and then grab another segment on the screen if this
		 * line is now deeper.  We then do the shift
		 * implied by the insertion.
		 */
		if (inssiz >= doomed + tabcol(tabstart, value(TABSTOP)) - tabstart) {
			if (IN)
				vrigid();
			vneedpos(value(TABSTOP));
			vishft();
		}
	} else if (inssiz + insmc1 > doomed)
		/*
		 * No tabs, but line may still get deeper.
		 */
		vneedpos(inssiz + insmc1 - doomed);
	/*
	 * Now put in the inserted characters.
	 */
	viin(c);

	/*
	 * Now put the cursor in its final resting place.
	 */
	destline = LINE(vcline);
	destcol = inscol + inssiz + insmc1 + filler;
	vcsync();
	if (IM != OIM) {
		IM = OIM;
		XN = OXN;
	}
	return c;
}

/*
 * Rigidify the rest of the line after the first
 * group of following tabs, typing blanks over ``spaces''.
 */
void
vrigid(void)
{
	register int col;
	register cell *tp = vtube0 + tabend;

	for (col = tabend; col < linend; col++) {
		if ((*tp++ & TRIM) == 0) {
			endim();
			vgotoCL(col);
			vputchar(' ' | QUOTE);
		}
	}
}

/*
 * We need cnt more positions on this line.
 * Open up new space on the screen (this may in fact be a
 * screen rollup).
 *
 * On a dumb terminal we may infact redisplay the rest of the
 * screen here brute force to keep it pretty.
 */
void
vneedpos(int npcnt)
{
	register int d = DEPTH(vcline);
	register int rmdr = d * WCOLS - linend;

	/*
	 * Delete the showmode string on wraparound to last line. Cannot use
	 * vclrech() since the mode string is printed on the echo area, but
	 * not actually a part of it.
	 */
	if (value(SHOWMODE) && (value(REDRAW) || (IM && EI)) &&
			npcnt == rmdr - IN && LINE(vcline) + d == WECHO) {
		int sdc, sdl;
		char *ocurs;

		endim();
		sdc = destcol, sdl = destline, ocurs = cursor;
		splitw++;
		vgoto(WECHO, 0);
		if (CD) {
			vputp(CD, 1);
		} else if (CE) {
			vputp(CE, 1);
		} else {
			int i;

			for (i = 1; i < WCOLS; i++)
				vputchar(' ');
		}
		destcol = sdc, destline = sdl; cursor = ocurs;
		splitw = 0;
	}
	if (npcnt <= rmdr - IN)
		return;
	endim();
	vnpins(1);
}

void
vnpins(int dosync)
{
	register int d = DEPTH(vcline);
	register int e;

	e = LINE(vcline) + DEPTH(vcline);
	if (e < LINE(vcline + 1)) {
		vigoto(e, 0);
		vclreol();
		return;
	}
	DEPTH(vcline)++;
	if (e < WECHO) {
		e = vglitchup(vcline, d);
		vigoto(e, 0); vclreol();
		if (dosync) {
			int (*Ooutchar)(int) = Outchar;
			Outchar = vputchar;
			vsync(e + 1);
			Outchar = Ooutchar;
		}
	} else {
		vup1();
		vigoto(WBOT, 0);
		vclreol();
	}
	vprepins();
}

/*
 * Do the shift of the next tabstop implied by
 * insertion so it expands.
 */
void
vishft(void)
{
	int tshft = 0;
	int j;
	register int i;
	register cell *tp = vtube0;
	register cell *up;
	short oldhold = hold;

	shft = value(TABSTOP);
	hold |= HOLDPUPD;
	if (!IM && !EI) {
		/*
		 * Dumb terminals are easy, we just have
		 * to retype the text.
		 */
		vigotoCL(tabend + shft);
		up = tp + tabend;
		for (i = tabend; i < linend; i++)
			vputchar(*up++);
	} else if (IN) {
		/*
		 * CONCEPT-like terminals do most of the work for us,
		 * we don't have to muck with simulation of multi-line
		 * insert mode.  Some of the shifting may come for free
		 * also if the tabs don't have enough slack to take up
		 * all the inserted characters.
		 */
		i = shft;
		slakused = inssiz - doomed;
		if (slakused > tabslack) {
			i -= slakused - tabslack;
			slakused -= tabslack;
		}
		if (i > 0 && tabend != linend) {
			tshft = i;
			vgotoCL(tabend);
			goim();
			do
				vputchar(' ' | QUOTE);
			while (--i);
		}
	} else {
		/*
		 * HP and Datamedia type terminals have to have multi-line
		 * insert faked.  Hack each segment after where we are
		 * (going backwards to where we are.)  We then can
		 * hack the segment where the end of the first following
		 * tab group is.
		 */
		for (j = DEPTH(vcline) - 1; j > (tabend + shft) / WCOLS; j--) {
			vgotoCL(j * WCOLS);
			goim();
			up = tp + j * WCOLS - shft;
			i = shft;
			do {
				if (*up)
					vputchar(*up++);
				else
					break;
			} while (--i);
		}
		vigotoCL(tabstart);
		i = shft - (inssiz - doomed);
		if (i > 0) {
			tabslack = inssiz - doomed;
			vcsync();
			goim();
			do
				vputchar(' ');
			while (--i);
		}
	}
	/*
	 * Now do the data moving in the internal screen
	 * image which is common to all three cases.
	 */
	tp += linend;
	up = tp + shft;
	i = linend - tabend;
	if (i > 0)
		do
			*--up = *--tp;
		while (--i);
	if (IN && tshft) {
		i = tshft;
		do
			*--up = ' ' | QUOTE;
		while (--i);
	}
	hold = oldhold;
}

/*
 * Now do the insert of the characters (finally).
 */
void
viin(int c)
/*	int c;		/\* mjm: char --> int */
{
	register cell *tp, *up;
	register int i, j;
	register bool noim = 0;
	int remdoom;
	short oldhold = hold;

	hold |= HOLDPUPD;
	if (tabsize && (IM && EI) && inssiz - doomed > tabslack) {
		/*
		 * There is a tab out there which will be affected
		 * by the insertion since there aren't enough doomed
		 * characters to take up all the insertion and we do
		 * have insert mode capability.
		 */
		if (inscol + insmc0 + doomed == tabstart) {
			/*
			 * The end of the doomed characters sits right at the
			 * start of the tabs, then we don't need to use insert
			 * mode; unless the tab has already been expanded
			 * in which case we MUST use insert mode.
			 */
			slakused = 0;
			noim = !shft;
		} else {
			/*
			 * The last really special case to handle is case
			 * where the tab is just sitting there and doesn't
			 * have enough slack to let the insertion take
			 * place without shifting the rest of the line
			 * over.  In this case we have to go out and
			 * delete some characters of the tab before we start
			 * or the answer will be wrong, as the rest of the
			 * line will have been shifted.  This code means
			 * that terminals with only insert chracter (no
			 * delete character) won't work correctly.
			 */
			i = inssiz - doomed - tabslack - slakused;
			i %= value(TABSTOP);
			if (i > 0) {
				vgotoCL(tabstart);
				godm();
				for (i = inssiz - doomed - tabslack; i > 0; i--)
					vputp(DC, DEPTH(vcline));
				enddm();
			}
		}
	}

	/* 
	 * Now put out the characters of the actual insertion.
	 */
	vigotoCL(inscol);
	remdoom = doomed;
	for (i = inssiz; i > 0; i--) {
		if (remdoom > insmc1) {
			remdoom--;
			endim();
		} else if (noim || (insmc1 && remdoom == insmc1))
			endim();
		else if (IM && EI) {
			vcsync();
			goim();
		}
		vputchar(c);
	}

	if (!IM || !EI || (remdoom && remdoom == insmc1)) {
		/*
		 * We are a dumb terminal; brute force update
		 * the rest of the line; this is very much an n^^2 process,
		 * and totally unreasonable at low speed.
		 *
		 * You asked for it, you get it.
		 */
		tp = vtube0 + inscol + doomed;
		for (i = inscol + doomed; i < tabstart; i++)
			vputchar(*tp++);
		hold = oldhold;
		vigotoCL(tabstart + inssiz + insmc0 - doomed);
		for (i = tabsize - (inssiz - insmc0 - doomed) + shft;
				i > 0; i--)
			vputchar(' ' | QUOTE);
	} else {
		if (!IN) {
			/*
			 * On terminals without multi-line
			 * insert in the hardware, we must go fix the segments
			 * between the inserted text and the following
			 * tabs, if they are on different lines.
			 *
			 * Aaargh.
			 */
			tp = vtube0;
			for (j = (inscol + insmc0 + inssiz - 1) / WCOLS + 1;
			    j <= (tabstart + inssiz - doomed - 1) / WCOLS; j++) {
				vgotoCL(j * WCOLS);
				i = inssiz - doomed + insmc1;
				up = tp + j * WCOLS - i;
				goim();
				do
					vputchar(*up++);
				while (--i && *up);
			}
		} else {
			/*
			 * On terminals with multi line inserts,
			 * life is simpler, just reflect eating of
			 * the slack.
			 */
			tp = vtube0 + tabend;
			for (i = tabsize - (inssiz + insmc1 - doomed); i >= 0; i--) {
				if ((*--tp & (QUOTE|TRIM)) == QUOTE) {
					--tabslack;
					if (tabslack >= slakused)
						continue;
				}
				*tp = ' ' | QUOTE;
			}
		}
		/*
		 * Blank out the shifted positions to be tab positions.
		 */
		if (shft) {
			tp = vtube0 + tabend + shft;
			for (i = tabsize - (inssiz - doomed) + shft; i > 0; i--)
				if ((*--tp & QUOTE) == 0)
					*tp = ' ' | QUOTE;
		}
	}

	/*
	 * Finally, complete the screen image update
	 * to reflect the insertion.
	 */
	hold = oldhold;
	tp = vtube0 + tabstart;
	up = tp + insmc1 + inssiz - doomed;
	for (i = tabstart; i > inscol + doomed; i--)
		*--up = *--tp;
#ifdef	MB
	for (i = insmc1; i > 0; i--)
		*--up = MULTICOL;
#endif
	for (i = inssiz; i > 0; i--)
		*--up = c | (insmc1 ? MULTICOL : 0);
	doomed = 0;
}

/*
 * Go into ``delete mode''.  If the
 * sequence which goes into delete mode
 * is the same as that which goes into insert
 * mode, then we are in delete mode already.
 */
void
godm(void)
{

	if (insmode) {
		if (eq(DM, IM))
			return;
		endim();
	}
	vputp(DM, 0);
}

/*
 * If we are coming out of delete mode, but
 * delete and insert mode end with the same sequence,
 * it wins to pretend we are now in insert mode,
 * since we will likely want to be there again soon
 * if we just moved over to delete space from part of
 * a tab (above).
 */
void
enddm(void)
{

	if (eq(DM, IM)) {
		insmode = 1;
		return;
	}
	vputp(ED, 0);
}

/*
 * In and out of insert mode.
 * Note that the code here demands that there be
 * a string for insert mode (the null string) even
 * if the terminal does all insertions a single character
 * at a time, since it branches based on whether IM is null.
 */
void
goim(void)
{

	if (!insmode)
		vputp(IM, 0);
	insmode = 1;
}

void
endim(void)
{

	if (insmode) {
		vputp(EI, 0);
		insmode = 0;
	}
}

/*
 * Put the character c on the screen at the current cursor position.
 * This routine handles wraparound and scrolling and understands not
 * to roll when splitw is set, i.e. we are working in the echo area.
 * There is a bunch of hacking here dealing with the difference between
 * QUOTE, QUOTE|' ', and ' ' for CONCEPT-100 like terminals, and also
 * code to deal with terminals which overstrike, including CRT's where
 * you can erase overstrikes with some work.  CRT's which do underlining
 * implicitly which has to be erased (like CONCEPTS) are also handled.
 */
int
vputchar(register int c)
{
	register cell *tp;
	register int d, m, n;

#ifndef	BIT8
	c &= (QUOTE|TRIM);
#endif
#ifdef TRACE
	if (trace)
		tracec(c);
#endif
	/* Fix problem of >79 chars on echo line. */
	if (destcol >= WCOLS-1 && splitw && destline == WECHO)
		pofix();
#ifdef	MB
	if (mb_cur_max > 1) {
		if (c == MULTICOL)
			return c;
		/*
		 * If a multicolumn character extends beyond the screen
		 * width, it must be put on the next line. A tilde is
		 * printed as an indicator but must disappear when the
		 * text is moved at a later time.
		 */
		if (c == ('~'|INVBIT|QUOTE))
			c = '~'|INVBIT;
		else if (c == ('~'|INVBIT))
			return c;
		else if (destcol < WCOLS && destcol +
				colsc(c==QUOTE ? ' ' : c&TRIM&~MULTICOL) - 1
				>= WCOLS)
			vputchar('~'|INVBIT|QUOTE);
	}
#endif	/* MB */
	if (destcol >= WCOLS) {
		destline += destcol / WCOLS;
		destcol %= WCOLS;
	}
	if (destline > WBOT && (!splitw || destline > WECHO))
		vrollup(destline);
	tp = vtube[destline] + destcol;
	if (c == QUOTE) {
		if (insmode) {
			/*
			 * When in insert mode, tabs have to expand
			 * to real, printed blanks.
			 */
			c = ' ' | QUOTE;
			goto def;
		}
		if (*tp == 0) {
			/*
			 * A ``space''.
			 */
			if ((hold & HOLDPUPD) == 0)
				*tp = QUOTE;
			destcol++;
			return c;
		}
		/*
		 * A ``space'' ontop of a part of a tab.
		 */
		if (*tp & QUOTE) {
			destcol++;
			return c;
		}
		c = ' ' | QUOTE;
		goto def;
	}

#ifdef	notdef
#ifdef	BIT8
	if (c == ' ' | QUOTE) {
		c = ' ';
		goto def;
	}
#endif
#endif
	switch (c) {

	case '\t':
		vgotab();
		return c;

	case ' ':
		/*
		 * We can get away without printing a space in a number
		 * of cases, but not always.  We get away with doing nothing
		 * if we are not in insert mode, and not on a CONCEPT-100
		 * like terminal, and either not in hardcopy open or in hardcopy
		 * open on a terminal with no overstriking, provided,
		 * in all cases, that nothing has ever been displayed
		 * at this position.  Ugh.
		 */
		if (!insmode && !IN && (state != HARDOPEN || OS)
		&& (*tp&QUOTE)) {
			*tp = ' ';
			destcol++;
			return c;
		}
		goto def;

def:
	default:
		d = *tp & TRIM;
		/*
		 * Now get away with doing nothing if the characters
		 * are the same, provided we are not in insert mode
		 * and if we are in hardopen, that the terminal has overstrike.
		 */
		if ((d & ~MULTICOL) == (c & TRIM & ~MULTICOL) && !insmode &&
				(state != HARDOPEN || OS) && c != MULTICOL) {
			n = colsc(d);
			for (m = 1; m < n; m++)
				if ((tp[m] & (MULTICOL|TRIM)) != MULTICOL)
					break;
			if (m == n) {
				if ((hold & HOLDPUPD) == 0)
					*tp = c | (n > 1 ? MULTICOL : 0);
				destcol += n;
				return c;
			}
		}
		/*
		 * Backwards looking optimization.
		 * The low level cursor motion routines will use
		 * a cursor motion right sequence to step 1 character
		 * right.  On, e.g., a DM3025A this is 2 characters
		 * and printing is noticeably slower at 300 baud.
		 * Since the low level routines are not allowed to use
		 * spaces for positioning, we discover the common
		 * case of a single space here and force a space
		 * to be printed.
		 */
		if (destcol == outcol + 1 && tp[-1] == ' ' && outline == destline) {
			vputc(' ');
			outcol++;
		}

		/*
		 * This is an inline expansion a call to vcsync() dictated
		 * by high frequency in a profile.
		 */
		if (outcol != destcol || outline != destline)
			vgoto(destline, destcol);

		/*
		 * Deal with terminals which have overstrike.
		 * We handle erasing general overstrikes, erasing
		 * underlines on terminals (such as CONCEPTS) which
		 * do underlining correctly automatically (e.g. on nroff
		 * output), and remembering, in hardcopy mode,
		 * that we have overstruct something.
		 */
		if (!insmode && d && d != ' ' && d != (c & TRIM)) {
			if (EO && (OS || (UL && (c == '_' || d == '_')))) {
				vputc(' ');
				outcol++, destcol++;
				back1();
			} else
				rubble = 1;
		}

		/*
		 * Unless we are just bashing characters around for
		 * inner working of insert mode, update the display.
		 */
		if ((hold & HOLDPUPD) == 0)
			*tp = c;

		/*
		 * In insert mode, put out the IC sequence, padded
		 * based on the depth of the current line.
		 * A terminal which had no real insert mode, rather
		 * opening a character position at a time could do this.
		 * Actually should use depth to end of current line
		 * but this rarely matters.
		 */
#ifdef	notdef
		if (insmode)
#else
		/*
		 * It seems today's termcap writers consider this
		 * an either-or situation; if both im and ic
		 * are used vi puts out additional spaces.
		 *
		 * SVR4 ex does not include this change. If it hits
		 * your terminal, change back to the old way and
		 * mail me a description.
		 *
		 * GR July 2000
		 */
		if (insmode && (!IM || !*IM))
#endif	/* !notdef */
		{
			n = colsc(c&TRIM);
			for (m = 0; m < n; m++)
				vputp(IC, DEPTH(vcline));
		}
		vputc(c & TRIM);

		/*
		 * In insert mode, IP is a post insert pad.
		 */
		if (insmode)
			vputp(IP, DEPTH(vcline));
		destcol++, outcol++;

		/*
		 * CONCEPT braindamage in early models:  after a wraparound
		 * the next newline is eaten.  It's hungry so we just
		 * feed it now rather than worrying about it.
		 * Fixed to use	return linefeed to work right
		 * on vt100/tab132 as well as concept.
		 */
		if (XN && outcol % WCOLS == 0) {
			vputc('\r');
			vputc('\n');
		}
	}
#ifdef	MB
	if (mb_cur_max > 1) {
		if ((d = colsc(c&TRIM&~MULTICOL)) > 1) {
			if ((hold & HOLDPUPD) == 0)
				*tp |= MULTICOL;
			while (--d) {
				if ((hold & HOLDPUPD) == 0)
					*++tp = MULTICOL;
				destcol++;
				outcol++;
			}
		} else if (d == 0) {
			destcol--;
			outcol--;
		}
	}
#endif	/* MB */
	return c;
}

/*
 * Delete display positions stcol through endcol.
 * Amount of use of special terminal features here is limited.
 */
void
physdc(int stcol, int endcol)
{
	register cell *tp, *up;
	cell *tpe = NULL;
	register int i;
	register int nc = endcol - stcol;

#ifdef IDEBUG
	if (trace)
		tfixnl(), fprintf(trace, "physdc(%d, %d)\n", stcol, endcol);
#endif
	if (!DC || nc <= 0)
		return;
	if (IN) {
		/*
		 * CONCEPT-100 like terminal.
		 * If there are any ``spaces'' in the material to be
		 * deleted, then this is too hard, just retype.
		 */
		vprepins();
		up = vtube0 + stcol;
		i = nc;
		do {
			if ((*up++ & (QUOTE|TRIM)) == QUOTE)
				return;
		} while (--i);
		i = 2 * nc;
		do {
			if (*up == 0 || (*up++ & QUOTE) == QUOTE)
				return;
		} while (--i);
		vgotoCL(stcol);
	} else {
		/*
		 * HP like delete mode.
		 * Compute how much text we are moving over by deleting.
		 * If it appears to be faster to just retype
		 * the line, do nothing and that will be done later.
		 * We are assuming 2 output characters per deleted
		 * characters and that clear to end of line is available.
		 */
		i = stcol / WCOLS;
		if (i != endcol / WCOLS)
			return;
		i += LINE(vcline);
		stcol %= WCOLS;
		endcol %= WCOLS;
		up = vtube[i]; tp = up + endcol; tpe = up + WCOLS;
		while (tp < tpe && *tp)
			tp++;
		if (tp - (up + stcol) < 2 * nc)
			return;
		vgoto(i, stcol);
	}

	/*
	 * Go into delete mode and do the actual delete.
	 * Padding is on DC itself.
	 */
	godm();
	for (i = nc; i > 0; i--)
		vputp(DC, DEPTH(vcline));
	vputp(ED, 0);

	/*
	 * Straighten up.
	 * With CONCEPT like terminals, characters are pulled left
	 * from first following null.  HP like terminals shift rest of
	 * this (single physical) line rigidly.
	 */
	if (IN) {
		up = vtube0 + stcol;
		tp = vtube0 + endcol;
		while ((i = *tp++)) {
			if ((i & (QUOTE|TRIM)) == QUOTE)
				break;
			*up++ = i;
		}
		do
			*up++ = i;
		while (--nc);
	} else {
		copy(up + stcol, up + endcol,
				(WCOLS - endcol) * sizeof *up);
		vclrcell(tpe - nc, nc);
	}
}

#ifdef TRACE
void
tfixnl(void)
{

	if (trubble || techoin)
		fprintf(trace, "\n");
	trubble = 0, techoin = 0;
}

void
tvliny(void)
{
	register int i;

	if (!trace)
		return;
	tfixnl();
	fprintf(trace, "vcnt = %d, vcline = %d, vliny = ", vcnt, vcline);
	for (i = 0; i <= vcnt; i++) {
		fprintf(trace, "%d", LINE(i));
		if (FLAGS(i) & VDIRT)
			fprintf(trace, "*");
		if (DEPTH(i) != 1)
			fprintf(trace, "<%d>", DEPTH(i));
		if (i < vcnt)
			fprintf(trace, " ");
	}
	fprintf(trace, "\n");
}

void
tracec(int c)
/*	int c;		/\* mjm: char --> int */
{

	if (!techoin)
		trubble = 1;
	if (c == ESCAPE)
		fprintf(trace, "$");
	else if (c & QUOTE)	/* mjm: for 3B (no sign extension) */
		fprintf(trace, "~%c", ctlof(c&TRIM));
	else if (c < ' ' || c == DELETE)
		fprintf(trace, "^%c", ctlof(c));
	else
		fprintf(trace, "%c", c);
}
#endif

/*
 * Put a character with possible tracing.
 */
int
vputch(int c)
{

#ifdef TRACE
	if (trace)
		tracec(c);
#endif
	return vputc(c);
}
