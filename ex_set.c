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
static char sccsid[] = "@(#)ex_set.c	1.11 (gritter) 11/24/04";
#endif
#endif

/* from ex_set.c	7.4 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"

/*
 * Set command.
 */
char	optname[ONMSZ];

void
set(void)
{
	register char *cp;
	register struct option *op;
	register int c;
	bool no;

	setnoaddr();
	if (skipend()) {
		if (peekchar() != EOF)
			ignchar();
		propts();
		return;
	}
	do {
		cp = optname;
		do {
			if (cp < &optname[ONMSZ - 2])
				*cp++ = getchar();
		} while (isalnum(peekchar()));
		*cp = 0;
		cp = optname;
		if (eq("all", cp)) {
			if (inopen)
				pofix();
			prall();
			goto setnext;
		}
		no = 0;
		if (cp[0] == 'n' && cp[1] == 'o') {
			cp += 2;
			no++;
		}
		/* Implement w300, w1200, and w9600 specially */
		if (eq(cp, "w300")) {
			if (ospeed >= B1200) {
dontset:
				ignore(getchar());	/* = */
				ignore(getnum());	/* value */
				continue;
			}
			cp = "window";
		} else if (eq(cp, "w1200")) {
			if (ospeed < B1200 || ospeed >= B2400)
				goto dontset;
			cp = "window";
		} else if (eq(cp, "w9600")) {
			if (ospeed < B2400)
				goto dontset;
			cp = "window";
		}
		for (op = options; op < &options[NOPTS]; op++)
			if (eq(op->oname, cp) || (op->oabbrev && eq(op->oabbrev, cp)))
				break;
		if (op->oname == 0)
			serror(catgets(catd, 1, 159,
		"%s: No such option@- 'set all' gives all option values"), cp);
		c = skipwh();
		if (peekchar() == '?') {
			ignchar();
printone:
			propt(op);
			noonl();
			goto setnext;
		}
		if (op->otype == ONOFF) {
			op->ovalue = 1 - no;
			if (op == &options[PROMPT])
				oprompt = 1 - no;
			goto setnext;
		}
		if (no)
			serror(catgets(catd, 1, 160,
				"Option %s is not a toggle"), op->oname);
		if (c != 0 || setend())
			goto printone;
		if (getchar() != '=')
			serror(catgets(catd, 1, 161,
			"Missing =@in assignment to option %s"), op->oname);
		switch (op->otype) {

		case NUMERIC:
			if (!isdigit(peekchar()))
				error(catgets(catd, 1, 162,
						"Digits required@after ="));
			op->ovalue = getnum();
			if (value(TABSTOP) <= 0)
				value(TABSTOP) = TABS;
			if (value(HARDTABS) <= 0)
				value(HARDTABS) = TABS;
			if (op == &options[WINDOW]) {
				if (value(WINDOW) >= TLINES)
					value(WINDOW) = TLINES-1;
				vsetsiz(value(WINDOW));
			}
			break;

		case STRING:
		case OTERM:
			cp = optname;
			while (!setend()) {
				if (cp >= &optname[ONMSZ])
					error(catgets(catd, 1, 163,
				"String too long@in option assignment"));
				/* adb change:  allow whitepace in strings */
				if( (*cp = getchar()) == '\\')
					if( peekchar() != EOF)
						*cp = getchar();
				cp++;
			}
			*cp = 0;
			if (op->otype == OTERM) {
/*
 * At first glance it seems like we shouldn't care if the terminal type
 * is changed inside visual mode, as long as we assume the screen is
 * a mess and redraw it. However, it's a much harder problem than that.
 * If you happen to change from 1 crt to another that both have the same
 * size screen, it's OK. But if the screen size if different, the stuff
 * that gets initialized in vop() will be wrong. This could be overcome
 * by redoing the initialization, e.g. making the first 90% of vop into
 * a subroutine. However, the most useful case is where you forgot to do
 * a setenv before you went into the editor and it thinks you're on a dumb
 * terminal. Ex treats this like hardcopy and goes into HARDOPEN mode.
 * This loses because the first part of vop calls oop in this case.
 * The problem is so hard I gave up. I'm not saying it can't be done,
 * but I am saying it probably isn't worth the effort.
 */
				if (inopen)
					error(catgets(catd, 1, 164,
		"Can't change type of terminal from within open/visual"));
				setterm(optname);
			} else {
				CP(op->osvalue, optname);
				op->odefault = 1;
			}
			break;
		}
setnext:
		flush();
	} while (!skipend());
	eol();
}

int 
setend(void)
{

	return (is_white(peekchar()) || endcmd(peekchar()));
}

void
prall(void)
{
	register int incr = (NOPTS + 2) / 3;
	register int rows = incr;
	register struct option *op = options;

	for (; rows; rows--, op++) {
		propt(op);
		tab(24);
		propt(&op[incr]);
		if (&op[2*incr] < &options[NOPTS]) {
			tab(56);
			propt(&op[2 * incr]);
		}
		putNFL();
	}
}

void
propts(void)
{
	register struct option *op;

	for (op = options; op < &options[NOPTS]; op++) {
		if (op == &options[TTYTYPE])
			continue;
		switch (op->otype) {

		case ONOFF:
		case NUMERIC:
			if (op->ovalue == op->odefault)
				continue;
			break;

		case STRING:
			if (op->odefault == 0)
				continue;
			break;
		}
		propt(op);
		putchar(' ');
	}
	noonl();
	flush();
}

void
propt(register struct option *op)
{
	register char *name;
	
	name = op->oname;

	switch (op->otype) {

	case ONOFF:
		ex_printf(catgets(catd, 1, 165, "%s%s"),
				op->ovalue ? catgets(catd, 1, 166, "")
				: catgets(catd, 1, 167, "no"), name);
		break;

	case NUMERIC:
		ex_printf(catgets(catd, 1, 168, "%s=%d"), name, op->ovalue);
		break;

	case STRING:
	case OTERM:
		ex_printf(catgets(catd, 1, 169, "%s=%s"), name, op->osvalue);
		break;
	}
}
