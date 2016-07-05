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
 *	from ex.h	7.7.1.1 (Berkeley) 8/12/86
 *
 *	Sccsid @(#)ex.h	1.57 (gritter) 8/6/05
 */

/*
 * Ex version 3 (see exact version in ex_version.c).
 *
 * Mark Horton, UC Berkeley
 * Bill Joy, UC Berkeley
 * November 1979
 *
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany
 * May 2000
 *
 * This file contains most of the declarations common to a large number
 * of routines.  The file ex_vis.h contains declarations
 * which are used only inside the screen editor.
 * The file config.h contains parameters which can be diddled per installation.
 * The file ex_tune.h contains parameters which should be changed by
 * maintainers only.
 *
 * The declarations relating to the argument list, regular expressions,
 * the temporary file data structure used by the editor
 * and the data describing terminals are each fairly substantial and
 * are kept in the files ex_{argv,re,temp,tty}.h which
 * we #include separately.
 *
 * If you are going to dig into ex, you should look at the outline of the
 * distribution of the code into files at the beginning of ex.c and ex_v.c.
 * Code which is similar to that of ed is lightly or undocumented in spots
 * (e.g. the regular expression code).  Newer code (e.g. open and visual)
 * is much more carefully documented, and still rough in spots.
 *
 * Please forward bug reports to
 *
 *	Mark Horton
 *	Computer Science Division, EECS
 *	EVANS HALL
 *	U.C. Berkeley 94704
 *	(415) 642-4948
 *	(415) 642-1024 (dept. office)
 *
 * or to csvax.mark@berkeley on the ARPA-net.  I would particularly like to hear
 * of additional terminal descriptions you add to the termcap data base.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef	BIT8
#ifndef	ISO8859_1
#include <locale.h>
#endif
#endif

#ifdef	MB
int wcwidth(wchar_t);
#include <wchar.h>
#include <wctype.h>
#endif

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#ifndef	TIOCGWINSZ
#include <sys/ioctl.h>
#endif

#include "config.h"

typedef	void	(*shand)(int);
#ifdef	signal
#undef	signal
#endif
#define	signal(a, b)	setsig((a), (b))

/*
 * Avoid clobbering of automatic variables with an ANSI C compiler.
 */
#define		CLOBBGRD(a)	(void)(&(a));

#ifndef	MB_LEN_MAX
#define	MB_LEN_MAX	1
#endif

/*
 * Feature dependency checks.
 */
#ifdef	ISO8859_1
#ifndef	BIT8
#define	BIT8
#endif
#endif

#ifndef	LISPCODE
#define	LISPCODE
#endif
#ifndef	CHDIR
#define	CHDIR
#endif

#ifndef var
#define var	extern
#endif

#ifndef	VMUNIX
typedef	short	line;
#else
typedef	int	line;
#endif

typedef	short	bool;

#ifdef	LARGEF
typedef	off_t	bloc;
#else
typedef	short	bloc;
#endif

#ifdef	VMUNIX
#ifdef	LARGEF
typedef	off_t	bbloc;
#else
typedef	int	bbloc;
#endif
#else
typedef	short	bbloc;
#endif

/*
 * The editor does not normally use the standard i/o library.  Because
 * we expect the editor to be a heavily used program and because it
 * does a substantial amount of input/output processing it is appropriate
 * for it to call low level read/write primitives directly.  In fact,
 * when debugging the editor we use the standard i/o library.  In any
 * case the editor needs a printf which prints through "putchar" ala the
 * old version 6 printf.  Thus we normally steal a copy of the "printf.c"
 * and "strout" code from the standard i/o library and mung it for our
 * purposes to avoid dragging in the stdio library headers, etc if we
 * are not debugging.  Such a modified printf exists in "printf.c" here.
 */
#ifdef TRACE
#	include <stdio.h>
	var	FILE	*trace;
	var	bool	trubble;
	var	bool	techoin;
	var	char	tracbuf[BUFSIZ];
#	undef	putchar
#	undef	getchar

#else	/* !TRACE */

#ifndef	BUFSIZ
#ifdef	LINE_MAX
#define	BUFSIZ	LINE_MAX	/* POSIX line size */
#else	/* !LINE_MAX */
#ifdef	VMUNIX
#define	BUFSIZ	1024
#else	/* !VMUNIX */
#ifdef	u370
#define	BUFSIZ	4096
#else	/* !u370 */
#define	BUFSIZ	512
#endif	/* !u370 */
#endif
#endif	/* !VMUNIX */
#endif	/* !LINE_MAX */

#ifdef	NULL
#undef	NULL
#endif
#ifdef	EOF
#undef	EOF
#endif
#ifdef	vprintf
#undef	vprintf
#endif
#ifdef	getchar
#undef	getchar
#endif
#ifdef	putchar
#undef	putchar
#endif

#define	NULL	0
#define	EOF	-1

#endif	/* !TRACE */

typedef	sigjmp_buf	JMP_BUF;
#define	SETJMP(a)	sigsetjmp(a, 1)
#define	LONGJMP(a, b)	siglongjmp(a, b)

#undef	MAXBSIZE
#define	MAXBSIZE	(2*LBSIZE)

#include "ex_tune.h"
#include "ex_vars.h"

/*
 * Options in the editor are referred to usually by "value(name)" where
 * name is all uppercase, i.e. "value(PROMPT)".  This is actually a macro
 * which expands to a fixed field in a static structure and so generates
 * very little code.  The offsets for the option names in the structure
 * are generated automagically from the structure initializing them in
 * ex_data.c... see the shell script "makeoptions".
 */
struct	option {
	char	*oname;
	char	*oabbrev;
	short	otype;		/* Types -- see below */
	short	odefault;	/* Default value */
	short	ovalue;		/* Current value */
	char	*osvalue;
};

#define	ONOFF	0
#define	NUMERIC	1
#define	STRING	2		/* SHELL or DIRECTORY */
#define	OTERM	3

#define	value(a)	options[a].ovalue
#define	svalue(a)	options[a].osvalue

extern	 struct	option options[NOPTS + 1];

/*
 * Character constants and bits
 *
 * The editor uses the QUOTE bit as a flag to pass on with characters
 * e.g. to the putchar routine.  The editor never uses a simple char variable.
 * Only arrays of and pointers to characters are used and parameters and
 * registers are never declared character.
 */
#ifdef	CTRL
#undef	CTRL
#endif
#define	CTRL(c)	((c) & 037)
#define	NL	CTRL('j')
#define	CR	CTRL('m')
#define	DELETE	0177		/* See also ATTN, QUIT in ex_tune.h */
#define	ESCAPE	033

/*
 * BIT8 and MB routines by Gunnar Ritter 2000, 2004.
 *
 * -DISO8859_1 enables all characters >= 0240 regardless of
 *  LC_CTYPE.
 */
#define	INVBIT		0x20000000
#define	MULTICOL	0x40000000

#if defined (MB)

/*
 * This type is used to represent a single character cell.
 */
typedef int	cell;
var	int	TRIM;
var	int	QUOTE;
#define	printable(c)	(((c)&INVBIT) == 0 && \
		(mb_cur_max > 1 ? iswprint((c)&TRIM) : isprint((c)&TRIM)))
#define	ext(c)		(((c) & 0177) == 0)

#elif defined (BIT8)

typedef	short	cell;
#define	QUOTE		0400
#define	TRIM		0377
#ifndef	ISO8859_1
#define	printable(c)	isprint((c)&TRIM)
#else	/* ISO8859_1 */
#define	printable(c)	(((c) & 0140) && (c) != DELETE)
#endif	/* ISO8859_1 */

#else	/* !BIT8 */

typedef	char	cell;
#define	QUOTE		0200
#define	TRIM		0177

#endif	/* !BIT8 */

/*
 * Miscellaneous random variables used in more than one place
 */
var	bool	aiflag;		/* Append/change/insert with autoindent */
var	bool	anymarks;	/* We have used '[a-z] */
var	int	bsize;		/* Block size for disk i/o */
var	int	chng;		/* Warn "No write" */
var	char	*Command;
var	short	defwind;	/* -w# change default window size */
var	int	dirtcnt;	/* When >= MAXDIRT, should sync temporary */
var	bool	dosusp;		/* Do SIGTSTP in visual when ^Z typed */
var	bool	edited;		/* Current file is [Edited] */
var	line	*endcore;	/* Last available core location */
extern	bool	endline;	/* Last cmd mode command ended with \n */
#ifndef VMUNIX
var	short	erfile;		/* Error message file unit */
#endif
var	line	*fendcore;	/* First address in line pointer space */
var	char	file[FNSIZE];	/* Working file name */
var	bool	fixedzero;	/* zero file size was fixed (for visual) */
var	char	*genbuf;	/* Working buffer when manipulating linebuf */
var	bool	hush;		/* Command line option - was given, hush up! */
var	char	*globp;		/* (Untyped) input string to command mode */
var	bool	holdcm;		/* Don't cursor address */
var	bool	inappend;	/* in ex command append mode */
var	bool	inglobal;	/* Inside g//... or v//... */
var	char	*initev;	/* Initial : escape for visual */
var	bool	inopen;		/* Inside open or visual */
var	char	*input;		/* Current position in cmd line input buffer */
var	bool	intty;		/* Input is a tty */
var	short	io;		/* General i/o unit (auto-closed on error!) */
extern	int	lastc;		/* Last character ret'd from cmd input */
var	bool	laste;		/* Last command was an "e" (or "rec") */
var	char	lastmac;	/* Last macro called for ** */
var	char	lasttag[TAGSIZE];	/* Last argument to a tag command */
var	char	*linebp;	/* Used in substituting in \n */
var	char	*linebuf;	/* The primary line buffer */
var	int	LBSIZE;		/* Size of linebuf */
var	bool	listf;		/* Command should run in list mode */
var	line	names['z'-'a'+2];	/* Mark registers a-z,' */
var	int	notecnt;	/* Count for notify (to visual from cmd) */
var	bool	numberf;	/* Command should run in number mode */
var	char	obuf[BUFSIZ];	/* Buffer for tty output */
var	shand	oldhup;		/* Previous SIGHUP handler */
var	shand	oldquit;	/* Previous SIGQUIT handler */
#ifdef	SIGXFSZ
var	shand	oldxfsz;	/* Previous SIGXFSZ handler */
#endif
var	short	oprompt;	/* Saved during source */
extern	unsigned short	ospeed;		/* Output speed (from gtty) */
var	int	otchng;		/* Backup tchng to find changes in macros */
var	int	peekc;		/* Peek ahead character (cmd mode input) */
var	char	*pkill[2];	/* Trim for put with ragged (LISP) delete */
var	bool	pfast;		/* Have stty -nl'ed to go faster */
var	pid_t	pid;		/* Process id of child */
var	pid_t	ppid;		/* Process id of parent (e.g. main ex proc) */
var	JMP_BUF	resetlab;	/* For error throws to top level (cmd mode) */
var	pid_t	rpid;		/* Pid returned from wait() */
var	bool	recov;		/* A `n' command is executed as `recov' */
var	bool	ruptible;	/* Interruptible is normal state */
var	bool	seenprompt;	/* 1 if have gotten user input */
var	bool	shudclob;	/* Have a prompt to clobber (e.g. on ^D) */
var	int	status;		/* Status returned from wait() */
var	int	tchng;		/* If nonzero, then [Modified] */
extern	int	tfile;		/* Temporary file unit */
var	bool	tflag;		/* -t option given on command line */
var	bool	vcatch;		/* Want to catch an error (open/visual) */
var	bool	verbose;	/* -V option; print command input to stderr */
var	JMP_BUF	vreslab;	/* For error throws to a visual catch */
var	bool	writing;	/* 1 if in middle of a file write */
var	int	xchng;		/* Suppresses multiple "No writes" in !cmd */
var	int	failed;		/* exit with a non-zero status */
var	int	exitoneof;	/* exit command loop on EOF */

/*
 * Macros
 */
#define	Copy(t, f, s)	memmove(t, f, s)
#define	CP(a, b)	memmove(a, b, strlen(b) + 1)
			/*
			 * FIXUNDO: do we want to mung undo vars?
			 * Usually yes unless in a macro or global.
			 */
#define FIXUNDO		(inopen >= 0 && (inopen || !inglobal))
#define ckaw()		{if (chng && value(AUTOWRITE)) wop(0);}
#define	copy(a,b,c)	Copy((char *) (a), (char *) (b), (c))
#define	eq(a, b)	((void *)(a) != NULL && (void *)(b) != NULL && strcmp(a, b) == 0)
#define	getexit(a)	copy(a, resetlab, sizeof (JMP_BUF))
#define	lastchar()	lastc
#define	outchar(c)	(*Outchar)(c)
#define	pastwh()	(ignore(skipwh()))
#define	pline(no, max)	(*Pline)(no, max)
#define	reset()		LONGJMP(resetlab,1)
#define	resexit(a)	copy(resetlab, a, sizeof (JMP_BUF))
#define	setexit()	SETJMP(resetlab)
#define	setlastchar(c)	lastc = c
#define	ungetchar(c)	peekc = c

#define	CATCH		vcatch = 1; if (SETJMP(vreslab) == 0) {
#define	ONERR		} else { vcatch = 0;
#define	ENDCATCH	} vcatch = 0;

/*
 * Environment like memory
 */
var	char	altfile[FNSIZE];	/* Alternate file name */
extern	char	direct[ONMSZ];		/* Temp file goes here */
extern	char	shell[ONMSZ];		/* Copied to be settable */
extern	char	ttylongname[ONMSZ];	/* A long and pretty name */
var	char	uxb[UXBSIZE + 2];	/* Last !command for !! */

/*
 * The editor data structure for accessing the current file consists
 * of an incore array of pointers into the temporary file tfile.
 * Each pointer is 15 bits (the low bit is used by global) and is
 * padded with zeroes to make an index into the temp file where the
 * actual text of the line is stored.
 *
 * To effect undo, copies of affected lines are saved after the last
 * line considered to be in the buffer, between dol and unddol.
 * During an open or visual, which uses the command mode undo between
 * dol and unddol, a copy of the entire, pre-command buffer state
 * is saved between unddol and truedol.
 */
var	line	*addr1;			/* First addressed line in a command */
var	line	*addr2;			/* Second addressed line */
var	line	*dol;			/* Last line in buffer */
var	line	*dot;			/* Current line */
var	line	*one;			/* First line */
var	line	*truedol;		/* End of all lines, including saves */
var	line	*unddol;		/* End of undo saved lines */
var	line	*zero;			/* Points to empty slot before one */

/*
 * Undo information
 *
 * For most commands we save lines changed by salting them away between
 * dol and unddol before they are changed (i.e. we save the descriptors
 * into the temp file tfile which is never garbage collected).  The
 * lines put here go back after unddel, and to complete the undo
 * we delete the lines [undap1,undap2).
 *
 * Undoing a move is much easier and we treat this as a special case.
 * Similarly undoing a "put" is a special case for although there
 * are lines saved between dol and unddol we don't stick these back
 * into the buffer.
 */
var	short	undkind;

var	line	*unddel;	/* Saved deleted lines go after here */
var	line	*undap1;	/* Beginning of new lines */
var	line	*undap2;	/* New lines end before undap2 */
var	line	*undadot;	/* If we saved all lines, dot reverts here */

#define	UNDCHANGE	0
#define	UNDMOVE		1
#define	UNDALL		2
#define	UNDNONE		3
#define	UNDPUT		4

extern	int	(*Outchar)(int);
extern	int	(*Pline)(int, int);
extern	int	(*Putchar)(int);

#define	NOSTR	(char *) 0
#define	NOLINE	(line *) 0

#define	ignore(a)	a
#define	ignorf(a)	a

#ifdef	LANGMSG
#include <nl_types.h>
var	nl_catd	catd;
#else	/* !LANGMSG */
#define	catgets(a, b, c, d)	(d)
#endif	/* !LANGMSG */
var	char	*cntrlhm;

#include "ex_proto.h"

var	int	mb_cur_max;
#ifdef	MB
#define	nextc(c, s, n)	(mb_cur_max > 1 && *(s) & 0200 ? \
			((n) = mbtowi(&(c), (s), mb_cur_max), \
		 	(n) = ((n) > 0 ? (n) : (n) < 0 ? (c=WEOF, 1) : 1)) :\
		((c) = *(s) & 0377, (n) = 1))
#define	colsc(c)	(mb_cur_max > 1 && ((c)&0177) != (c) ? wcwidth(c) : 1)
#define	skipleft(l, p)	(mb_cur_max > 1 && ((p)[0]&0200 || \
				((p)>(l) && (p)[-1]&0200)) ? wskipleft(l, p) : -1)
#define	skipright(l, p)	(mb_cur_max > 1 && (p)>=(l) && (p)[0]&0200 ? \
				wskipright(l, p) : 1)
#define	samechar(cp, c)	(mb_cur_max > 1 && *(cp)&0200 ? wsamechar(cp, c) : \
				(*(cp)&0377) == c)
#define	xisdigit(c)	(mb_cur_max > 1 ? iswdigit(c) : isdigit(c))
#define	xisalpha(c)	(mb_cur_max > 1 ? iswalpha(c) : isalpha(c))
#define	xisalnum(c)	(mb_cur_max > 1 ? iswalnum(c) : isalnum(c))
#define	xisspace(c)	(mb_cur_max > 1 ? iswspace(c) : isspace(c))
#define	xisupper(c)	(mb_cur_max > 1 ? iswupper(c) : isupper(c))
#define	xislower(c)	(mb_cur_max > 1 ? iswlower(c) : islower(c))
#define	xtolower(c)	(mb_cur_max > 1 ? towlower(c) : (wint_t)tolower(c))
#define	xtoupper(c)	(mb_cur_max > 1 ? towupper(c) : (wint_t)toupper(c))
#else	/* !MB */
#define	nextc(c, s, n)	((c) = *(s) & 0377, (n) = 1)
#define	colsc(c)	(1)
#define	skipleft(l, p)	(-1)
#define	skipright(l, p)	(1)
#define	samechar(cp, c)	(*(cp)&0377 == c)
#define	xisdigit(c)	isdigit(c)
#define	xisalpha(c)	isalpha(c)
#define	xisalnum(c)	isalnum(c)
#define	xisspace(c)	isspace(c)
#define	xisupper(c)	isupper(c)
#define	xislower(c)	islower(c)
#define	xtolower(c)	tolower(c)
#define	xtoupper(c)	toupper(c)
#endif	/* !MB */
