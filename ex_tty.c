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
static char sccsid[] = "@(#)ex_tty.c	1.30 (gritter) 8/4/05";
#endif
#endif

/* from ex_tty.c	7.10.1 (2.11BSD GTE) 12/9/94 */

#include <sys/ioctl.h>
#include "ex.h"
#include "ex_tty.h"

int ATTN = DELETE;

/*
 * Terminal type initialization routines,
 * and calculation of flags at entry or after
 * a shell escape which may change them.
 */
/* short	ospeed = -1;	mjm: def also in tputs.c of termcap.a  */

void 
gettmode(void)
{
	speed_t pospeed;

	if (tcgetattr(1, &tty) < 0) {
		ospeed = B0;
		return;
	}
	pospeed = cfgetospeed(&tty);
	if (ospeed != pospeed)
		value(SLOWOPEN) = pospeed < B1200;
	ospeed = pospeed;
	normf = tty;
#if defined (UCVISUAL) && defined (IUCLC)
	UPPERCASE = (tty.c_iflag & IUCLC) != 0;
#endif
#if defined (TAB3)
	GT = (tty.c_oflag & TABDLY) != TAB3 && !XT;
#elif defined (XTABS)
	GT = (tty.c_oflag & TABDLY) != XTABS && !XT;
#else
	GT = !XT;
#endif	/* !TAB3, XTABS */
	/*
	 * Tabs and multi-column characters do not combine properly
	 * unless vi performs a look-ahead on the current line. Just
	 * do not use them for now.
	 */
	if (mb_cur_max > 1)
		GT = 0;
	NONL = (tty.c_oflag & ONLCR) == 0;
	ATTN = tty.c_cc[VINTR];
}

char *xPC;
char **sstrs[] = {
	&AL, &BC, &BT, &CD, &CE, &CL, &CM, &xCR, &xCS, &DC, &DL, &DM, &DO,
	&ED, &EI, &F0, &F1, &F2, &F3, &F4, &F5, &F6, &F7, &F8, &F9,
	&HO, &IC, &IM, &IP, &KD, &KE, &KH, &KL, &KR, &KS, &KU, &LL, &ND, &xNL,
	&xPC, &RC, &SC, &SE, &SF, &SO, &SR, &TA, &TE, &TI, &UP, &VB, &VS, &VE,
	&AL_PARM, &DL_PARM, &UP_PARM, &DOWN_PARM, &LEFT_PARM, &RIGHT_PARM
};
bool *sflags[] = {
	&AM, &BS, &DA, &DB, &EO, &HC,
#ifdef	UCVISUAL
	&xHZ,
#endif
	&IN, &MI, &NC, &NS, &OS, &UL,
	&XB, &XN, &XT, &XX
};
char **fkeys[10] = {
	&F0, &F1, &F2, &F3, &F4, &F5, &F6, &F7, &F8, &F9
};
void
setterm(char *type)
{
	register int unknown;
	char ltcbuf[TCBUFSIZE];

	if (type[0] == 0)
		type = "xx";
	unknown = 0;
	putpad(TE);
	if (tgetent(ltcbuf, type) != 1) {
		unknown++;
		lcpy(ltcbuf, "xx|dumb:", sizeof ltcbuf);
	}
	gettmode(); /* must call gettmode() before setsize(). GR */
	setsize();
	aoftspace = tspace;
	zap();
	/*
	 * Initialize keypad arrow keys.
	 */
	addmac1(KU, "k", "up", arrows, 1);
	addmac1(KD, "j", "down", arrows, 1);
	addmac1(KL, "h", "left", arrows, 1);
	addmac1(KR, "l", "right", arrows, 1);
	addmac1(KH, "H", "home", arrows, 1);

	/*
	 * Handle funny termcap capabilities
	 */
	if (xCS && SC && RC) {
		if (AL==NULL) AL="";
		if (DL==NULL) DL="";
	}
	if (AL_PARM && AL==NULL) AL="";
	if (DL_PARM && DL==NULL) DL="";
	if (IC && IM==NULL) IM="";
	if (IC && EI==NULL) EI="";
	if (!GT) BT=NULL;	/* If we can't tab, we can't backtab either */

#ifdef	TIOCLGET
#define	HAS_JOB_CONTROL
#endif
#ifdef	_SC_JOB_CONTROL
#define	HAS_JOB_CONTROL
#endif
#ifdef	HAS_JOB_CONTROL
	/*
	 * Now map users susp char to ^Z, being careful that the susp
	 * overrides any arrow key, but only for hackers (=new tty driver).
	 */
	{
		static char sc[2];
		int i /* , fnd */;

		if (sysconf(_SC_JOB_CONTROL) != -1)
		{
			/*
			 * If a system supports job control but no job
			 * control shell is used, only one method of
			 * detection remains: Our session id equals our
			 * process group id. Any job control shell would
			 * have created at least one new process group.
			 * But as the VSUSP key may be active, we have
			 * to override arrow keys either.
			 */
#ifndef	_CRAY	/* getsid() is a bad syscall on UNICOS */
			if (getsid(0) != getpgid(0))
#endif	/* !_CRAY */
				ldisc = 2;	/* value of NTTYDISC */
			sc[0] = tty.c_cc[VSUSP];
			sc[1] = 0;
			if (tty.c_cc[VSUSP] == CTRL('z')) {
				for (i=0; i<=4; i++)
					if (arrows[i].cap &&
					    arrows[i].cap[0] == CTRL('z'))
						addmac(sc, NULL, NULL, arrows);
			} else if (sc[0]
#ifdef	_PC_VDISABLE
					&& sc[0] != fpathconf(1, _PC_VDISABLE)
#endif
					)
				addmac(sc, "\32", "susp", arrows);
		}
	}
#endif	/* HAS_JOB_CONTROL */

	if (CM != 0) {
		if (tgoto(CM, 2, 2)[0] == 'O')	/* OOPS */
			CA = 0, CM = 0;
		else
			CA = 1, costCM = cost(tgoto(CM, 8, 10));
	} else {
		CA = 0, CM = 0;
	}
	costSR = cost(SR);
	costAL = cost(AL);
	costDP = cost(tgoto(DOWN_PARM, 10, 10));
	costLP = cost(tgoto(LEFT_PARM, 10, 10));
	costRP = cost(tgoto(RIGHT_PARM, 10, 10));
	PC = xPC ? xPC[0] : 0;
	aoftspace = tspace;
	safecp(ttylongname, gettlongname(ltcbuf, type), sizeof ttylongname,
			"Terminal name too long");
	/* proper strings to change tty type */
	termreset();
	gettmode();
	value(REDRAW) = AL && DL;
	value(OPTIMIZE) = !CA && !GT;
	if (ospeed == B1200 && !value(REDRAW))
		value(SLOWOPEN) = 1;	/* see also gettmode above */
	if (unknown)
		serror(catgets(catd, 1, 191,
				"%s: Unknown terminal type"), type);
}

void
setsize(void)
{
	register int l, i;
#ifdef	TIOCGWINSZ
	struct winsize win;
#endif

	char *e;

#ifdef	TIOCGWINSZ
	i = ioctl(0, TIOCGWINSZ, &win);
#endif
	TLINES = TCOLUMNS = 0;
	e = getenv("COLUMNS");
	if (e != NULL && *e != '\0')
		TCOLUMNS = atoi(e);
	if (TCOLUMNS <= 0) {
#ifdef	TIOCGWINSZ
		if (i >= 0 && win.ws_col != 0)
			TCOLUMNS = winsz.ws_col = win.ws_col;
		else
#endif
			TCOLUMNS = tgetnum("co");
	}
	e = getenv("LINES");
	if (e != NULL && *e != '\0')
		TLINES = atoi(e);
	if (TLINES <= 0) {
#ifdef	TIOCGWINSZ
		if (i >= 0 && win.ws_row != 0)
			TLINES = winsz.ws_row = win.ws_row;
		else
#endif
			TLINES = tgetnum("li");
	}
	i = TLINES;
	if (TLINES <= 5)
		TLINES = 24;
	l = TLINES;
	if (ospeed < B1200)
		l = 9;	/* including the message line at the bottom */
	else if (ospeed < B2400)
		l = 17;
	if (l > TLINES)
		l = TLINES;
	if (TCOLUMNS <= 4)
		TCOLUMNS = 1000;
	options[WINDOW].ovalue = options[WINDOW].odefault = l - 1;
	if (defwind) {
		options[WINDOW].ovalue = defwind;
		l = defwind + 1;
	}
	options[SCROLL].ovalue = options[SCROLL].odefault = HC ? 11 : ((l-1) / 2);
	if (i <= 0)
		TLINES = 2;
}

void
zap(void)
{
	register char *namp;
	register bool **fp;
	register char ***sp;
	int flag;
	char *string;

#ifndef	UCVISUAL
	namp = "ambsdadbeohcinmincnsosulxbxnxtxx";
#else
	namp = "ambsdadbeohchzinmincnsosulxbxnxtxx";
#endif
	fp = sflags;
	do {
		flag = tgetflag(namp);
		*(*fp++) = flag;
		namp += 2;
	} while (*namp);
	namp = "albcbtcdceclcmcrcsdcdldmdoedeik0k1k2k3k4k5k6k7k8k9hoicimipkdkekhklkrkskullndnlpcrcscsesfsosrtatetiupvbvsveALDLUPDOLERI";
	sp = sstrs;
	do {
		string = tgetstr(namp, &aoftspace);
		*(*sp++) = string;
		namp += 2;
	} while (*namp);
}

char *
gettlongname(register char *bp, char *def)
{
	register char *cp;

	while (*bp && *bp != ':' && *bp != '|')
		bp++;
	if (*bp == '|') {
		bp++;
		cp = bp;
		while (*cp && *cp != ':' && *cp != '|')
			cp++;
		*cp = 0;
		return (bp);
	}
	return (def);
}

char *
fkey(int i)
{
	if (0 <= i && i <= 9)
		return(*fkeys[i]);
	else
		return(NOSTR);
}

/*
 * cost figures out how much (in characters) it costs to send the string
 * str to the terminal.  It takes into account padding information, as
 * much as it can, for a typical case.  (Right now the typical case assumes
 * the number of lines affected is the size of the screen, since this is
 * mainly used to decide if AL or SR is better, and this always happens
 * at the top of the screen.  We assume cursor motion (CM) has little
 * padding, if any, required, so that case, which is really more important
 * than AL vs SR, won't be really affected.)
 */
static int costnum;
int
cost(char *str)
{
	if (str == NULL || *str=='O')	/* OOPS */
		return 10000;	/* infinity */
	costnum = 0;
	tputs(str, TLINES, countnum);
	return costnum;
}

/*ARGSUSED*/
int
countnum(int ch)
{
	costnum++;
	return ch;
}
