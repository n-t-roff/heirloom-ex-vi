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
static char sccsid[] = "@(#)ex_unix.c	1.17 (gritter) 8/4/05";
#endif
#endif

/* from ex_unix.c	7.6 (Berkeley) 10/22/85 */

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"
#include <sys/wait.h>

/*
 * Unix escapes, filtering
 */

/*
 * First part of a shell escape,
 * parse the line, expanding # and % and ! and printing if implied.
 */
void 
unix0(int warn)
{
	register char *up, *fp;
	register short c;
	char printub, puxb[UXBSIZE + sizeof (int)];

	printub = 0;
	lcpy(puxb, uxb, sizeof puxb);
	c = getchar();
	if (c == '\n' || c == EOF)
		error(catgets(catd, 1, 192,
	"Incomplete shell escape command@- use 'shell' to get a shell"));
	up = uxb;
	do {
		switch (c) {

		case '\\':
			if (any(peekchar(), "%#!"))
				c = getchar();
		default:
			if (up >= &uxb[UXBSIZE]) {
tunix:
				uxb[0] = 0;
				error(catgets(catd, 1, 193,
							"Command too long"));
			}
			*up++ = c;
			break;

		case '!':
			fp = puxb;
			if (*fp == 0) {
				uxb[0] = 0;
				error(catgets(catd, 1, 194,
			"No previous command@to substitute for !"));
			}
			printub++;
			while (*fp) {
				if (up >= &uxb[UXBSIZE])
					goto tunix;
				*up++ = *fp++;
			}
			break;

		case '#':
			fp = altfile;
			if (*fp == 0) {
				uxb[0] = 0;
				error(catgets(catd, 1, 195,
				"No alternate filename@to substitute for #"));
			}
			goto uexp;

		case '%':
			fp = savedfile;
			if (*fp == 0) {
				uxb[0] = 0;
				error(catgets(catd, 1, 196,
					"No filename@to substitute for %%"));
			}
uexp:
			printub++;
			while (*fp) {
				if (up >= &uxb[UXBSIZE])
					goto tunix;
#ifndef	BIT8
				*up++ = *fp++ | QUOTE;
#else
				*up++ = *fp++;
#endif
			}
			break;
		}
		c = getchar();
	} while (c == '"' || c == '|' || !endcmd(c));
	if (c == EOF)
		ungetchar(c);
	*up = 0;
	if (!inopen)
		resetflav();
	if (warn)
		ckaw();
	if (warn && hush == 0 && chng && xchng != chng && value(WARN) && dol > zero) {
		xchng = chng;
		vnfl();
		ex_printf(mesg(catgets(catd, 1, 197,
				"[No write]|[No write since last change]")));
		noonl();
		flush();
	} else
		warn = 0;
	if (printub) {
		if (uxb[0] == 0)
			error(catgets(catd, 1, 198,
					"No previous command@to repeat"));
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		if (warn)
			vnfl();
		if (hush == 0)
			lprintf("!%s", uxb);
		if (inopen && Outchar != termchar) {
			vclreol();
			vgoto(WECHO, 0);
		} else
			putnl();
		flush();
	}
}

/*
 * Do the real work for execution of a shell escape.
 * Mode is like the number passed to open system calls
 * and indicates filtering.  If input is implied, newstdin
 * must have been setup already.
 */
struct termios 
unixex(char *opt, char *up, int newstdin, int mode)
{
	int pvec[2];
	struct termios f;

	signal(SIGINT, SIG_IGN);
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, SIG_DFL);
#endif
	/*if (inopen)*/
		f = setty(normf);
	if ((mode & 1) && pipe(pvec) < 0) {
		/* Newstdin should be io so it will be closed */
		if (inopen)
			setty(f);
		error(catgets(catd, 1, 199, "Can't make pipe for filter"));
	}
#ifndef VFORK
	pid = fork();
#else
	pid = vfork();
#endif
	if (pid < 0) {
		if (mode & 1) {
			close(pvec[0]);
			close(pvec[1]);
		}
		setrupt();
		error(catgets(catd, 1, 200, "No more processes"));
	}
	if (pid == 0) {
		if (mode & 2) {
			close(0);
			dup(newstdin);
			close(newstdin);
		}
		if (mode & 1) {
			close(pvec[0]);
			close(1);
			dup(pvec[1]);
			if (inopen) {
				close(2);
				dup(1);
			}
			close(pvec[1]);
		}
		if (io)
			close(io);
		if (tfile)
			close(tfile);
#ifndef VMUNIX
		close(erfile);
#endif
		signal(SIGHUP, oldhup);
		signal(SIGQUIT, oldquit);
#ifdef	SIGXFSZ
		signal(SIGXFSZ, oldxfsz);
#endif
		if (ruptible)
			signal(SIGINT, SIG_DFL);
		execl(svalue(SHELL), "sh", opt, up, (char *)0);
		ex_printf(catgets(catd, 1, 201, "No %s!\n"), svalue(SHELL));
		error(NOSTR);
	}
	if (mode & 1) {
		io = pvec[0];
		close(pvec[1]);
	}
	if (newstdin)
		close(newstdin);
	return (f);
}

/*
 * Wait for the command to complete.
 * F is for restoration of tty mode if from open/visual.
 * C flags suppression of printing.
 */
void 
unixwt(int c, struct termios f)
{

	waitfor();
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, onsusp);
#endif
	if (inopen)
		setty(f);
	setrupt();
	if (!inopen && c && hush == 0) {
		ex_printf("!\n");
		flush();
		termreset();
		gettmode();
	}
}

/*
 * Setup a pipeline for the filtration implied by mode
 * which is like a open number.  If input is required to
 * the filter, then a child editor is created to write it.
 * If output is catch it from io which is created by unixex.
 */
void 
filter(register int mode)
{
	static int pvec[2];
	struct termios f;	/* mjm: was register */
	register int lines = lineDOL();
	struct stat statb;

	mode++;
	if (mode & 2) {
		signal(SIGINT, SIG_IGN);
		if (pipe(pvec) < 0)
			error(catgets(catd, 1, 202, "Can't make pipe"));
		pid = fork();
		io = pvec[0];
		if (pid < 0) {
			setrupt();
			close(pvec[1]);
			error(catgets(catd, 1, 203, "No more processes"));
		}
		if (pid == 0) {
			setrupt();
			io = pvec[1];
			close(pvec[0]);
			putfile(1);
			exitex(0);
		}
		close(pvec[1]);
		io = pvec[0];
		setrupt();
	}
	f = unixex("-c", uxb, (mode & 2) ? pvec[0] : 0, mode);
	if (mode == 3) {
		delete(0);
		addr2 = addr1 - 1;
	} else if (mode == 1)
		deletenone();
	if (mode & 1) {
		if(FIXUNDO)
			undap1 = undap2 = addr2+1;
		if (fstat(io, &statb) < 0 || statb.st_blksize > LBSIZE)
			bsize = LBSIZE;
		else {
			bsize = statb.st_blksize;
			if (bsize <= 0)
				bsize = LBSIZE;
		}
		ignore(append(getfile, addr2));
#ifdef TRACE
		if (trace)
			vudump("after append in filter");
#endif
	}
	close(io);
	io = -1;
	unixwt(!inopen, f);
	netchHAD(lines);
}

/*
 * Set up to do a recover, getting io to be a pipe from
 * the recover process.
 */
void 
recover(void)
{
	static int pvec[2];

	if (pipe(pvec) < 0)
		error(catgets(catd, 1, 204, " Can't make pipe for recovery"));
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error(catgets(catd, 1, 205, " Can't fork to execute recovery"));
	}
	if (pid == 0) {
		close(2);
		dup(1);
		close(1);
		dup(pvec[1]);
	        close(pvec[1]);
		execl(EXRECOVER, "exrecover", svalue(DIRECTORY), file, (char *) 0);
		close(1);
		dup(2);
		error(catgets(catd, 1, 206, " No recovery routine"));
	}
	close(pvec[1]);
}

/*
 * Wait for the process (pid an external) to complete.
 */
void 
waitfor(void)
{
	int stat = 0;
	pid_t wpid;

	do {
		wpid = wait(&stat);
		if (wpid == pid) {
			status = stat;
			rpid = wpid;
		}
	} while (wpid != -1);
	if (status) {
		if (WIFEXITED(status))
			status = WEXITSTATUS(status);
		else
			status = 0;
	}
}

/*
 * The end of a recover operation.  If the process
 * exits non-zero, force not edited; otherwise force
 * a write.
 */
void 
revocer(void)
{

	waitfor();
	if (pid == rpid && status != 0)
		edited = 0;
	else
		change();
}
