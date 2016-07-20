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
char *copyright =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)ex.c	1.37 (gritter) 8/4/05";
#endif	/* DOSCCS */
#endif	/* !lint */

/* from ex.c	7.5.1.1 (Berkeley) 8/12/86 */

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"

#ifdef TRACE
char	tttrace[]	= { '/','d','e','v','/','t','t','y','x','x',0 };
#endif

/*
 * The code for ex is divided as follows:
 *
 * ex.c			Entry point and routines handling interrupt, hangup
 *			signals; initialization code.
 *
 * ex_addr.c		Address parsing routines for command mode decoding.
 *			Routines to set and check address ranges on commands.
 *
 * ex_cmds.c		Command mode command decoding.
 *
 * ex_cmds2.c		Subroutines for command decoding and processing of
 *			file names in the argument list.  Routines to print
 *			messages and reset state when errors occur.
 *
 * ex_cmdsub.c		Subroutines which implement command mode functions
 *			such as append, delete, join.
 *
 * ex_data.c		Initialization of options.
 *
 * ex_get.c		Command mode input routines.
 *
 * ex_io.c		General input/output processing: file i/o, unix
 *			escapes, filtering, source commands, preserving
 *			and recovering.
 *
 * ex_put.c		Terminal driving and optimizing routines for low-level
 *			output (cursor-positioning); output line formatting
 *			routines.
 *
 * ex_re.c		Global commands, substitute, regular expression
 *			compilation and execution.
 *
 * ex_set.c		The set command.
 *
 * ex_subr.c		Loads of miscellaneous subroutines.
 *
 * ex_temp.c		Editor buffer routines for main buffer and also
 *			for named buffers (Q registers if you will.)
 *
 * ex_tty.c		Terminal dependent initializations from termcap
 *			data base, grabbing of tty modes (at beginning
 *			and after escapes).
 *
 * ex_unix.c		Routines for the ! command and its variations.
 *
 * ex_v*.c		Visual/open mode routines... see ex_v.c for a
 *			guide to the overall organization.
 */

static char *progname;

void 
erropen(void)
{
	close(1);
	dup(2);
}

void 
usage(void)
{
	ex_printf(catgets(catd, 1, 1, "\
Usage: %s [- | -s] [-l] [-L] [-R] [-r [file]] [-t tag]\n\
       [-v] [-V] [-w size] [+cmd | -c cmd] file...\n"),
		progname);
	flush();
	exitex(1);
}

void
needarg(int c)
{
	erropen();
	ex_printf(catgets(catd, 1, 2,
		"%s: option requires an argument -- %c\n"), progname, c);
	usage();
}

void
invopt(int c)
{
	erropen();
	ex_printf(catgets(catd, 1, 3, "%s: illegal option -- %c\n"), progname, c);
	usage();
}

/*
 * Return last component of unix path name p.
 */
char *
tailpath(register char *p)
{
	register char *r;

	for (r=p; *p; p++)
		if (*p == '/')
			r = p+1;
	return(r);
}

/*
 * Check ownership of file.  Return nonzero if it exists and is owned by the
 * user or the option sourceany is used
 */
int 
iownit(char *file)
{
	struct stat sb;

	if (*file == '.' && value(EXRC) == 0)
		return 0;
	if (stat(file, &sb))
		return 0;
	if (value(SOURCEANY))
		return 1;
	if (sb.st_uid != getuid())
		return 0;
	if (sb.st_mode & (S_IWOTH | S_IWGRP))
		return 0;
	return 1;
}

shand 
setsig(int signum, shand handler)
{
	struct sigaction nact, oact;

	nact.sa_handler = handler;
	sigemptyset(&nact.sa_mask);
	nact.sa_flags = 0;
	if (signum == SIGALRM) {
#ifdef	SA_INTERRUPT
		nact.sa_flags |= SA_INTERRUPT;
#endif
	/*EMPTY*/ ;
	} else {
#ifdef	SA_RESTART
		nact.sa_flags |= SA_RESTART;
#endif
	/*EMPTY*/ ;
	}
	if (sigaction(signum, &nact, &oact) != 0)
		return SIG_ERR;
	return oact.sa_handler;
}

/*
 * Initialization, before editing a new file.
 * Main thing here is to get a new buffer (in fileinit),
 * rest is peripheral state resetting.
 */
void 
init(void)
{
	register int i;

	fileinit();
	dot = zero = truedol = unddol = dol = fendcore;
	one = zero+1;
	undkind = UNDNONE;
	chng = 0;
	edited = 0;
	for (i = 0; i <= 'z'-'a'+1; i++)
		names[i] = 1;
	anymarks = 0;
}

/*
 * Main procedure.  Process arguments and then
 * transfer control to the main command processing loop
 * in the routine commands.  We are entered as either "ex", "edit", "vi"
 * or "view" and the distinction is made here.  Actually, we are "vi" if
 * there is a 'v' in our name, "view" is there is a 'w', and "edit" if
 * there is a 'd' in our name.  For edit we just diddle options;
 * for vi we actually force an early visual command.
 */
int 
main(register int ac, register char *av[])
{
#ifndef VMUNIX
	char *erpath = EXSTRINGS;
#endif
	char *cp = NULL;
	register int c;
	bool ivis;
	bool fast = 0;
#ifdef TRACE
	register char *tracef;
#endif

	CLOBBGRD(ivis);
	CLOBBGRD(fast);
	CLOBBGRD(cp);

	/*
	 * Initialize the built-in memory allocator.
	 */
#ifdef	VMUNIX
	poolsbrk(0);
#endif

	/*
	 * Initialize the primary buffers which were originally static.
	 * NOTE: Most of this must be repeated in ex_recover.c.
	 */
	linebuf = calloc(LBSIZE = BUFSIZ<4096?4096:BUFSIZ, sizeof *linebuf);
	genbuf = calloc(MAXBSIZE, sizeof *genbuf);

	/*
	 * Immediately grab the tty modes so that we wont
	 * get messed up if an interrupt comes in quickly.
	 */
	gTTY(1);
	normf = tty;
	ppid = getpid();
	/*
	 * Defend against d's, v's, w's, and a's in directories of
	 * path leading to our true name.
	 */
	av[0] = tailpath(av[0]);

	/*
	 * Figure out how we were invoked: ex, edit, vi, view.
	 */
	ivis = any('v', av[0]);	/* "vi" */
	if (any('w', av[0]))	/* "view" */
		value(READONLY) = 1;
	if (any('d', av[0])) {	/* "edit" */
		value(SHOWMODE) = 1;
		/*
		 * I do not understand why novices should not
		 * switch to visual mode. So they can now. gritter
		 */
		/*value(OPEN) = 0;*/
		value(REPORT) = 1;
		value(MAGIC) = 0;
	}

#ifndef VMUNIX
	/*
	 * For debugging take files out of . if name is a.out.
	 */
	if (av[0][0] == 'a')
		erpath = tailpath(erpath);
#endif	/* !VMUNIX */

	progname = av[0];
	/*
	 * Open the error message file.
	 */
	draino();
#ifndef VMUNIX
	erfile = open(erpath+4, O_RDONLY);
	if (erfile < 0) {
		erfile = open(erpath, O_RDONLY);
	}
#endif	/* !VMUNIX */
	pstop();

	/*
	 * Initialize interrupt handling.
	 */
	oldhup = signal(SIGHUP, SIG_IGN);
	if (oldhup == SIG_DFL)
		signal(SIGHUP, onhup);
	oldquit = signal(SIGQUIT, SIG_IGN);
#ifdef	SIGXFSZ
	oldxfsz = signal(SIGXFSZ, SIG_IGN);
#endif
	ruptible = signal(SIGINT, SIG_IGN) == SIG_DFL;
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		signal(SIGTERM, onhup);
#ifdef	SIGEMT
	if (signal(SIGEMT, SIG_IGN) == SIG_DFL)
		signal(SIGEMT, onemt);
#endif

#ifdef	BIT8
#ifndef	ISO8859_1
	setlocale(LC_CTYPE, "");
#endif
#endif

#ifdef	MB_CUR_MAX
	mb_cur_max = MB_CUR_MAX;
#else
	mb_cur_max = 1;
#endif

#ifdef	MB
	TRIM = mb_cur_max > 1 ? 0x6fffffff : 0xff;
	QUOTE = mb_cur_max > 1 ? 0x10000000 : 0x100;
#endif

#ifdef	LANGMSG
	setlocale(LC_MESSAGES, "");
	catd = catopen(CATNAME, NL_CAT_LOCALE);
#endif

	/*
	 * Process flag arguments.
	 */
	ac--, av++;
	while (ac) {
		if (av[0][0] == '+') {
			firstpat = &av[0][1];
		} else if (av[0][0] == '-') {
arggroup:
		c = av[0][1];
		if (c == 0
			|| c == 's'
				) {
			hush = 1;
			value(AUTOPRINT) = 0;
			fast++;
		} else switch (c) {

		case '-':
			if (av[0][2])
				invopt('-');
			ac--, av++;
			goto argend;

		case 'R':
			value(READONLY) = 1;
			break;

#ifdef TRACE
		case 'T':
			if (av[0][2] == 0)
				tracef = "trace";
			else {
				tracef = tttrace;
				tracef[8] = av[0][2];
				if (tracef[8])
					tracef[9] = av[0][3];
				else
					tracef[9] = 0;
			}
			trace = fopen(tracef, "w");
#define tracbuf NULL
			if (trace == NULL)
				ex_printf(catgets(catd, 1, 4,
						"Trace create error\n"));
			else
				setbuf(trace, tracbuf);
			break;

#endif	/* TRACE */

		case 'c':
			if (av[0][2] == '\0' && (av[1] == NULL
					|| *av[1] == '-' || *av[1] == '+'))
				needarg('c');
			if (av[0][2]) {
				firstpat = &av[0][2];
			} else {
				firstpat = av[1];
				ac--, av++;
			}
			break;

		case 'e':
			ivis = 0;
			break;

#ifdef LISPCODE
		case 'l':
			value(LISP) = 1;
			value(SHOWMATCH) = 1;
			break;
#endif

		case 'L':
		case 'r':
			recov++;
			break;

		case 't':
			if (av[0][2]) {
				tflag = 1;
				safecp(lasttag, av[0], sizeof lasttag,
						"argument to -t too long");
			} else if (ac > 1 && av[1][0] != '-' &&
					av[1][0] != '+') {
				ac--, av++;
				tflag = 1;
				safecp(lasttag, av[0], sizeof lasttag,
						"argument to -t too long");
			} else
				needarg('t');
			break;

		case 'v':
			ivis = 1;
			break;

		case 'V':
			verbose = 1;
			break;

		case 'w':
			if (av[0][2])
				cp = &av[0][2];
			else if (ac > 1 && av[1][0] != '-' && av[1][0] != '+') {
				cp = av[1];
				ac--, av++;
			} else
				needarg('w');
			defwind = atoi(cp);
			break;

		default:
			invopt(c);
		}
		if (c && c != 'c' && c != 't' && c != 'w' && av[0][2]) {
			av[0]++;
			goto arggroup;
		}
	    } else
		    break;
		ac--, av++;
	}
argend:

	cntrlhm = catgets(catd, 1, 70, "^H discarded\n");
	/*
	 * Initialize end of core pointers.
	 * Normally we avoid breaking back to fendcore after each
	 * file since this can be expensive (much core-core copying).
	 * If your system can scatter load processes you could do
	 * this as ed does, saving a little core, but it will probably
	 * not often make much difference.
	 */
	fendcore = (line *) sbrk(0);
	endcore = fendcore - 2;

#ifdef SIGTSTP
	if (!hush && signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, onsusp), dosusp++;
#endif	/* SIGTSTP */

	/*
	 * If we are doing a recover and no filename
	 * was given, then execute an exrecover command with
	 * the -r option to type out the list of saved file names.
	 * Otherwise set the remembered file name to the first argument
	 * file name so the "recover" initial command will find it.
	 */
	if (recov) {
		if (ac == 0) {
			ppid = 0;
			setrupt();
			execl(EXRECOVER, "exrecover", "-r", (char *)0);
			filioerr(EXRECOVER);
			exitex(1);
		}
		safecp(savedfile, *av++, sizeof savedfile, "Filename too long");
		ac--;
	}

	/*
	 * Initialize the argument list.
	 */
	argv0 = av;
	argc0 = ac;
	args0 = av[0];
	erewind();

	/*
	 * Initialize a temporary file (buffer) and
	 * set up terminal environment.  Read user startup commands.
	 */
	if (setexit() == 0) {
		setrupt();
		intty = isatty(0);
		value(PROMPT) = intty;
		if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
			safecp(shell, cp, sizeof shell, "$SHELL too long");
		if (fast || !intty)
			setterm("dumb");
		else {
			gettmode();
			if ((cp = getenv("TERM")) != 0 && *cp) {
				setterm(cp);
			}
		}
	}
	if (setexit() == 0) {
		/*
		 * This is necessary because 'if (setexit() == 0 && !fast)'
		 * is rejected on the Cray.
		 */
		if (fast)
			goto skip;
		if ((globp = getenv("EXINIT")) && *globp)
			commands(1,1);
		else {
			globp = 0;
			if ((cp = getenv("HOME")) != 0 && *cp) {
				safecat(safecp(genbuf, cp, MAXBSIZE,
							"$HOME too long"),
						"/.exrc", MAXBSIZE,
						"$HOME too long");
				if (iownit(genbuf))
					source(genbuf, 1);
			}
		}
		/*
		 * Allow local .exrc too.  This loses if . is $HOME,
		 * but nobody should notice unless they do stupid things
		 * like putting a version command in .exrc.  Besides,
		 * they should be using EXINIT, not .exrc, right?
		 *
		 * This may not be done anymore. GR
		 */
		/*
		 * The getcwd() function is not present on very
		 * old Unix systems. So if this fails, comment out
		 * the following three lines or supply code e.g. from
		 * the `pwd' utility.
		 */
		if (cp == NULL || *cp == '\0'
				|| getcwd(genbuf, MAXBSIZE) == NULL
				|| strcmp(cp, genbuf) != 0)

		if (iownit(".exrc"))
			source(".exrc", 1);
	}
skip:	init();	/* moved after prev 2 chunks to fix directory option */

	/*
	 * Initial processing.  Handle tag, recover, and file argument
	 * implied next commands.  If going in as 'vi', then don't do
	 * anything, just set initev so we will do it later (from within
	 * visual).
	 */
	if (setexit() == 0) {
		if (recov)
			globp = "recover";
		else if (tflag)
			globp = ivis ? "tag" : "tag|p";
		else if (argc)
			globp = "next";
		if (ivis)
			initev = globp;
		else if (globp) {
			inglobal = 1;
			commands(1, 1);
			inglobal = 0;
		}
	}

	/*
	 * Vi command... go into visual.
	 * Strange... everything in vi usually happens
	 * before we ever "start".
	 */
	if (ivis) {
		/*
		 * Don't have to be upward compatible with stupidity
		 * of starting editing at line $.
		 */
		if (dol > zero)
			dot = one;
		globp = "visual";
		if (setexit() == 0)
			commands(1, 1);
	}

	/*
	 * Clear out trash in state accumulated by startup,
	 * and then do the main command loop for a normal edit.
	 * If you quit out of a 'vi' command by doing Q or ^\,
	 * you also fall through to here.
	 */
	seenprompt = 1;
	ungetchar(0);
	globp = 0;
	initev = 0;
	setlastchar('\n');
	setexit();
	commands(0, 0);
	cleanup(1);
	exitex(0);
	/*NOTREACHED*/
	return 0;
}
