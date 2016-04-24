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
static char sccsid[] = "@(#)ex_v.c	1.19 (gritter) 8/4/05";
#endif
#endif

/* from ex_v.c	7.8.1 (2.11BSD GTE) 12/9/94 */

#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Entry points to open and visual from command mode processor.
 * The open/visual code breaks down roughly as follows:
 *
 * ex_v.c	entry points, checking of terminal characteristics
 *
 * ex_vadj.c	logical screen control, use of intelligent operations
 *		insert/delete line and coordination with screen image;
 *		updating of screen after changes.
 *
 * ex_vget.c	input of single keys and reading of input lines
 *		from the echo area, handling of memory for repeated
 *		commands and small saved texts from inserts and partline
 *		deletes, notification of multi line changes in the echo
 *		area.
 *
 * ex_vmain.c	main command decoding, some command processing.
 *
 * ex_voperate.c   decoding of operator/operand sequences and
 *		contextual scans, implementation of word motions.
 *
 * ex_vops.c	major operator interfaces, undos, motions, deletes,
 *		changes, opening new lines, shifts, replacements and yanks
 *		coordinating logical and physical changes.
 *
 * ex_vops2.c	subroutines for operator interfaces in ex_vops.c,
 *		insert mode, read input line processing at lowest level.
 *
 * ex_vops3.c	structured motion definitions of ( ) { } and [ ] operators,
 *		indent for lisp routines, () and {} balancing. 
 *
 * ex_vput.c	output routines, clearing, physical mapping of logical cursor
 *		positioning, cursor motions, handling of insert character
 *		and delete character functions of intelligent and unintelligent
 *		terminals, visual mode tracing routines (for debugging),
 *		control of screen image and its updating.
 *
 * ex_vwind.c	window level control of display, forward and backward rolls,
 *		absolute motions, contextual displays, line depth determination
 */

JMP_BUF venv;
static cell *atube;

/*
 * Determine and set the size for visual mode buffers.
 */
static void
tubesizes(void)
{
	TUBELINES = TLINES;
	/*
	 * TUBECOLS should stay at 160 at least since this defines the
	 * maximum length of opening on hardcopies and allows two lines
	 * of open on terminals like adm3's (glass tty's) where it
	 * switches to pseudo hardcopy mode when a line gets longer
	 * than 80 characters.
	 */
	TUBECOLS = TCOLUMNS < 160 ? 160 : TCOLUMNS;
	TUBESIZE = TLINES * TCOLUMNS;
	free(vlinfo);
	free(vtube);
	free(atube);
	vlinfo = malloc((TUBELINES+2) * sizeof *vlinfo);
	vtube = malloc(TUBELINES * sizeof *vtube);
	atube = malloc((TUBESIZE + LBSIZE) * sizeof *atube);
	if (vlinfo == NULL || vtube == NULL || atube == NULL)
		error("Screen too large");
}

/*
 * Enter open mode
 */
void 
oop(void)
{
	register char *ic;
	struct termios f;	/* mjm: was register */
	int resize;

	resize = SETJMP(venv);
	if (resize) {
		setsize();
		initev = (char *)0;
		inopen = 0;
		addr1 = addr2 = dot;
	}
	tubesizes();
#ifdef	SIGWINCH
	signal(SIGWINCH, onwinch);
#endif
	ovbeg();
	if (peekchar() == '/') {
		ignore(compile(getchar(), 1));
		savere(&scanre);
		if (execute(0, dot) == 0)
			error(catgets(catd, 1, 207,
				"Fail|Pattern not found on addressed line"));
		ic = loc1;
		if (ic > linebuf && *ic == 0)
			ic--;
	} else {
		getDOT();
		ic = vskipwh(linebuf);
	}
	newline();

	/*
	 * If overstrike then have to HARDOPEN
	 * else if can move cursor up off current line can use CRTOPEN (~~vi1)
	 * otherwise (ugh) have to use ONEOPEN (like adm3)
	 */
	if (OS && !EO)
		bastate = HARDOPEN;
	else if (CA || UP)
		bastate = CRTOPEN;
	else
		bastate = ONEOPEN;
	setwind();

	/*
	 * To avoid bombing on glass-crt's when the line is too long
	 * pretend that such terminals are 160 columns wide.
	 * If a line is too wide for display, we will dynamically
	 * switch to hardcopy open mode.
	 */
	if (state != CRTOPEN)
		WCOLS = TUBECOLS;
	if (!inglobal)
		savevis();
	vok(atube);
	if (state != CRTOPEN)
		TCOLUMNS = WCOLS;
	Outchar = vputchar;
	f = ostart();
	if (state == CRTOPEN) {
		if (outcol == UKCOL)
			outcol = 0;
		vmoveitup(1, 1);
	} else
		outline = destline = WBOT;
	vshow(dot, NOLINE);
	vnline(ic);
	vmain();
	if (state != CRTOPEN)
		vclean();
	Command = "open";
	ovend(f);
#ifdef	SIGWINCH
	signal(SIGWINCH, SIG_DFL);
#endif
}

void 
ovbeg(void)
{

	if (!value(OPEN))
		error(catgets(catd, 1, 208,
			"Can't use open/visual unless open option is set"));
	if (inopen)
		error(catgets(catd, 1, 209,
					"Recursive open/visual not allowed"));
	Vlines = lineDOL();
	fixzero();
	setdot();
	pastwh();
	dot = addr2;
}

void 
ovend(struct termios f)
{

	splitw++;
	vgoto(WECHO, 0);
	vclreol();
	vgoto(WECHO, 0);
	holdcm = 0;
	splitw = 0;
	ostop(f);
	setoutt();
	undvis();
	TCOLUMNS = OCOLUMNS;
	inopen = 0;
	flusho();
	netchHAD(Vlines);
}

/*
 * Enter visual mode
 */
void 
vop(void)
{
	register int c;
	struct termios f;	/* mjm: was register */
	int resize;

	if (!CA && UP == NOSTR) {
		if (initev) {
toopen:
			merror(catgets(catd, 1, 210, "[Using open mode]"));
			putNFL();
			oop();
			return;
		}
		error(catgets(catd, 1, 211,
		"Visual needs addressible cursor or upline capability"));
	}
	if (OS && !EO) {
		if (initev)
			goto toopen;
		error(catgets(catd, 1, 212,
			"Can't use visual on a terminal which overstrikes"));
	}
	if (!CL) {
		if (initev)
			goto toopen;
		error(catgets(catd, 1, 213,
			"Visual requires clear screen capability"));
	}
	if (NS && !SF) {
		if (initev)
			goto toopen;
		error(catgets(catd, 1, 214, "Visual requires scrolling"));
	}
	resize = SETJMP(venv);
	if (resize) {
		setsize();
		initev = (char *)0;
		inopen = 0;
		addr1 = addr2 = dot;
	}
	tubesizes();
#ifdef	SIGWINCH
	signal(SIGWINCH, onwinch);
#endif
	ovbeg();
	bastate = VISUAL;
	c = 0;
	if (any(peekchar(), "+-^."))
		c = getchar();
	pastwh();
	vsetsiz(isdigit(peekchar()) ? getnum() : value(WINDOW));
	setwind();
	newline();
	vok(atube);
	if (!inglobal)
		savevis();
	Outchar = vputchar;
	vmoving = 0;
	f = ostart();
	if (initev == 0) {
		vcontext(dot, c);
		vnline(NOSTR);
	}
	vmain();
	Command = "visual";
	ovend(f);
#ifdef	SIGWINCH
	signal(SIGWINCH, SIG_DFL);
#endif
}

/*
 * Hack to allow entry to visual with
 * empty buffer since routines internally
 * demand at least one line.
 */
void 
fixzero(void)
{

	if (dol == zero) {
		register bool ochng = chng;

		vdoappend("");
		if (!ochng)
			synced();
		fixedzero++;
		addr1 = addr2 = one;
	} else if (addr2 == zero)
		addr2 = one;
}

/*
 * Save lines before visual between unddol and truedol.
 * Accomplish this by throwing away current [unddol,truedol]
 * and then saving all the lines in the buffer and moving
 * unddol back to dol.  Don't do this if in a global.
 *
 * If you do
 *	g/xxx/vi.
 * and then do a
 *	:e xxxx
 * at some point, and then quit from the visual and undo
 * you get the old file back.  Somewhat weird.
 */
void 
savevis(void)
{

	if (inglobal)
		return;
	truedol = unddol;
	saveall();
	unddol = dol;
	undkind = UNDNONE;
}

/*
 * Restore a sensible state after a visual/open, moving the saved
 * stuff back to [unddol,dol], and killing the partial line kill indicators.
 */
void 
undvis(void)
{

	if (ruptible)
		signal(SIGINT, onintr);
	squish();
	pkill[0] = pkill[1] = 0;
	unddol = truedol;
	unddel = zero;
	undap1 = one;
	undap2 = dol + 1;
	undkind = UNDALL;
	if (undadot <= zero || undadot > dol)
		undadot = zero+1;
}

/*
 * Set the window parameters based on the base state bastate
 * and the available buffer space.
 */
void 
setwind(void)
{

	WCOLS = TCOLUMNS;
	switch (bastate) {

	case ONEOPEN:
		if (AM)
			WCOLS--;
		/* fall into ... */

	case HARDOPEN:
		basWTOP = WTOP = WBOT = WECHO = 0;
		ZERO = 0;
		holdcm++;
		break;

	case CRTOPEN:
		basWTOP = TLINES - 2;
		/* fall into */

	case VISUAL:
		ZERO = TLINES - TUBESIZE / WCOLS;
		if (ZERO < 0)
			ZERO = 0;
		if (ZERO > basWTOP)
			error(catgets(catd, 1, 215,
				"Screen too large for internal buffer"));
		WTOP = basWTOP; WBOT = TLINES - 2; WECHO = TLINES - 1;
		break;
	}
	state = bastate;
	basWLINES = WLINES = WBOT - WTOP + 1;
}

/*
 * Can we hack an open/visual on this terminal?
 * If so, then divide the screen buffer up into lines,
 * and initialize a bunch of state variables before we start.
 */
void 
vok(register cell *atube)
{
	register int i;

	if (WCOLS == 1000)
		serror(catgets(catd, 1, 216,
		"Don't know enough about your terminal to use %s"), Command);
	if (WCOLS > TUBECOLS)
		error(catgets(catd, 1, 217, "Terminal too wide"));
	if (WLINES >= TUBELINES || WCOLS * (WECHO - ZERO + 1) > TUBESIZE)
		error(catgets(catd, 1, 218, "Screen too large"));

	vtube0 = atube;
	vclrcell(atube, WCOLS * (WECHO - ZERO + 1));
	for (i = 0; i < ZERO; i++)
		vtube[i] = (cell *) 0;
	for (; i <= WECHO; i++)
		vtube[i] = atube, atube += WCOLS;
	for (; i < TUBELINES; i++)
		vtube[i] = (cell *) 0;
	vutmp = (char *)atube;
	vundkind = VNONE;
	vUNDdot = 0;
	OCOLUMNS = TCOLUMNS;
	inopen = 1;
	signal(SIGINT, vintr);
	vmoving = 0;
	splitw = 0;
	doomed = 0;
	holdupd = 0;
	Peekkey = 0;
	vcnt = vcline = 0;
	vSCROLL = value(SCROLL);
}

void 
vintr(int signum)
{
	extern JMP_BUF readbuf;
	extern int doingread;

	(void)signum;
	signal(SIGINT, vintr);
	if (vcatch)
		onintr(SIGINT);
	ungetkey(ATTN);
	draino();
	if (doingread) {
		doingread = 0;
		LONGJMP(readbuf, 1);
	}
}

/*
 * Set the size of the screen to size lines, to take effect the
 * next time the screen is redrawn.
 */
void
vsetsiz(int size)
{
	register int b;

	if (bastate != VISUAL)
		return;
	b = TLINES - 1 - size;
	if (b >= TLINES - 1)
		b = TLINES - 2;
	if (b < 0)
		b = 0;
	basWTOP = b;
	basWLINES = WBOT - b + 1;
}

#ifdef	SIGWINCH
void 
onwinch(int signum)
{
	(void)signum;
	vsave();
	setty(normf);
	LONGJMP(venv, 1);
}
#endif
