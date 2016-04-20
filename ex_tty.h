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
 *
 *	from ex_tty.h	7.5.1 (2.11BSD GTE) 12/9/94
 *
 *	Sccsid @(#)ex_tty.h	1.14 (gritter) 8/4/05
 */
#include "libterm/libterm.h"

/*
 * Capabilities from termcap
 *
 * The description of terminals is a difficult business, and we only
 * attempt to summarize the capabilities here;  for a full description
 * see the paper describing termcap.
 *
 * Capabilities from termcap are of three kinds - string valued options,
 * numeric valued options, and boolean options.  The string valued options
 * are the most complicated, since they may include padding information,
 * which we describe now.
 *
 * Intelligent terminals often require padding on intelligent operations
 * at high (and sometimes even low) speed.  This is specified by
 * a number before the string in the capability, and has meaning for the
 * capabilities which have a P at the front of their comment.
 * This normally is a number of milliseconds to pad the operation.
 * In the current system which has no true programmible delays, we
 * do this by sending a sequence of pad characters (normally nulls, but
 * specifiable as "pc").  In some cases, the pad is better computed
 * as some number of milliseconds times the number of affected lines
 * (to bottom of screen usually, except when terminals have insert modes
 * which will shift several lines.)  This is specified as '12*' e.g.
 * before the capability to say 12 milliseconds per affected whatever
 * (currently always line).  Capabilities where this makes sense say P*.
 */
#ifndef	VMUNIX
var	char	tspace[256];	/* Space for capability strings */
#else
var	char	tspace[1024];	/* Space for capability strings */
#endif
var	char	*aoftspace;	/* Address of tspace for relocation */

var	char	*AL;		/* P* Add new blank line */
var	char	*AL_PARM;	/* P* Add n new blank lines */
extern	char	*BC;		/*    Back cursor */
var	char	*BT;		/* P  Back tab */
var	char	*CD;		/* P* Clear to end of display */
var	char	*CE;		/* P  Clear to end of line */
var	char	*CL;		/* P* Clear screen */
var	char	*CM;		/* PG Cursor motion */
var	char	*xCS;		/* PG Change scrolling region (vt100) */
var	char	*xCR;		/* P  Carriage return */
var	char	*DC;		/* P* Delete character */
var	char	*DL;		/* P* Delete line sequence */
var	char	*DL_PARM;	/* P* Delete n lines */
var	char	*DM;		/*    Delete mode (enter)  */
var	char	*DO;		/*    Down line sequence */
var	char	*DOWN_PARM;	/*    Down n lines */
var	char	*ED;		/*    End delete mode */
var	char	*EI;		/*    End insert mode */
var	char	*F0,*F1,*F2,*F3,*F4,*F5,*F6,*F7,*F8,*F9;
				/*    Strings sent by various function keys */
var	char	*HO;		/*    Home cursor */
var	char	*IC;		/* P  Insert character */
var	char	*IM;		/*    Insert mode (give as ':im=:' if 'ic' */
var	char	*IP;		/* P* Insert pad after char ins'd using IM+IE */
var	char	*KD;		/*    Keypad down arrow */
var	char	*KE;		/*    Keypad don't xmit */
var	char	*KH;		/*    Keypad home key */
var	char	*KL;		/*    Keypad left arrow */
var	char	*KR;		/*    Keypad right arrow */
var	char	*KS;		/*    Keypad start xmitting */
var	char	*KU;		/*    Keypad up arrow */
var	char	*LEFT_PARM;	/*    Left n chars */
var	char	*LL;		/*    Quick to last line, column 0 */
var	char	*ND;		/*    Non-destructive space */
var	char	*RIGHT_PARM;	/*    Right n spaces */
var	char	*xNL;		/*    Line feed (new line) */
extern	char	PC;		/*    Pad character */
var	char	*RC;		/*    Restore cursor from last SC */
var	char	*SC;		/*    Save cursor */
var	char	*SE;		/*    Standout end (may leave space) */
var	char	*SF;		/* P  Scroll forwards */
var	char	*SO;		/*    Stand out begin (may leave space) */
var	char	*SR;		/* P  Scroll backwards */
var	char	*TA;		/* P  Tab (other than ^I or with padding) */
var	char	*TE;		/*    Terminal end sequence */
var	char	*TI;		/*    Terminal initial sequence */
extern	char	*UP;		/*    Upline */
var	char	*UP_PARM;	/*    Up n lines */
var	char	*VB;		/*    Visible bell */
var	char	*VE;		/*    Visual end sequence */
var	char	*VS;		/*    Visual start sequence */
var	bool	AM;		/* Automatic margins */
var	bool	BS;		/* Backspace works */
var	bool	CA;		/* Cursor addressible */
var	bool	DA;		/* Display may be retained above */
var	bool	DB;		/* Display may be retained below */
var	bool	EO;		/* Can erase overstrikes with ' ' */
var	bool	GT;		/* Gtty indicates tabs */
var	bool	HC;		/* Hard copy terminal */
#ifdef	UCVISUAL
var	bool	xHZ;		/* Hazeltine ~ braindamage */
#endif
var	bool	IN;		/* Insert-null blessing */
var	bool	MI;		/* can move in insert mode */
var	bool	NC;		/* No Cr - \r snds \r\n then eats \n (dm2500) */
var	bool	NS;		/* No scroll - linefeed at bottom won't scroll */
var	bool	OS;		/* Overstrike works */
var	bool	UL;		/* Underlining works even though !os */
var	bool	XB;		/* Beehive (no escape key, simulate with f1) */
var	bool	XN;		/* A newline gets eaten after wrap (concept) */
var	bool	XT;		/* Tabs are destructive */
var	bool	XX;		/* Tektronix 4025 insert line */
	/* X? is reserved for severely nauseous glitches */
	/* If there are enough of these we may need bit masks! */

/*
 * From the tty modes...
 */
var	bool	NONL;		/* Terminal can't hack linefeeds doing a CR */
#ifdef	UCVISUAL
var	bool	UPPERCASE;	/* Ick! */
#endif
extern	short	TLINES;		/* Number of lines on screen */
extern	short	TCOLUMNS;
var	short	OCOLUMNS;	/* Save TCOLUMNS for a hack in open mode */
#ifdef	TIOCGWINSZ
var	struct winsize winsz;	/* Save window size for stopping comparisons */
#endif

var	short	outcol;		/* Where the cursor is */
var	short	outline;

var	short	destcol;	/* Where the cursor should be */
var	short	destline;

var	struct	termios tty;	/* Use this one structure to change modes */

var	struct termios	normf;		/* Restore tty flags to this (someday) */
var	bool	normtty;	/* Have to restore normal mode from normf */

var	short	costCM;	/* # chars to output a typical CM, with padding etc. */
var	short	costSR;	/* likewise for scroll reverse */
var	short	costAL;	/* likewise for insert line */
var	short	costDP;	/* likewise for DOWN_PARM */
var	short	costLP;	/* likewise for LEFT_PARM */
var	short	costRP;	/* likewise for RIGHT_PARM */

#ifdef VMUNIX
# define MAXNOMACS	128	/* max number of macros of each kind */
# define MAXCHARMACS	2048	/* max # of chars total in macros */
#else
# define MAXNOMACS	48	/* max number of macros of each kind */
# define MAXCHARMACS	1536	/* max # of chars total in macros */
#endif
struct maps {
	char *cap;	/* pressing button that sends this.. */
	int  *icap;	/* same as int */
	char *mapto;	/* .. maps to this string */
	char *descr;	/* legible description of key */
	bool hadthis;	/* did this mapping already (avoid recursion) */
};
var	struct maps arrows[MAXNOMACS];	/* macro defs - 1st 5 built in */
var	struct maps immacs[MAXNOMACS];	/* for while in insert mode */
var	struct maps abbrevs[MAXNOMACS];	/* for word abbreviations */
var	int	ldisc;			/* line discipline for ucb tty driver */
var	char	mapspace[MAXCHARMACS];
var	int	imapspace[MAXCHARMACS];
var	char	*msnext;	/* next free location in mapspace */
var	int	*imsnext;	/* next free location in imapspace */
var	int	maphopcnt;	/* check for infinite mapping loops */
var	bool	anyabbrs;	/* true if abbr or unabbr has been done */
var	char	ttynbuf[255];	/* result of ttyname() */
var	int	ttymesg;	/* original mode of users tty */

extern int map(register int, register struct maps *);
extern void addmac1(register char *, register char *, register char *,
		register struct maps *, int);
#define	addmac(a, b, c, d)	addmac1(a, b, c, d, 0)
