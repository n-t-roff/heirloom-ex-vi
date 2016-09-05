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
static char sccsid[] = "@(#)ex_cmds2.c	1.18 (gritter) 2/17/05";
#endif
#endif

/* from ex_cmds2.c	7.4 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

extern bool	pflag, nflag;		/* mjm: extern; also in ex_cmds.c */
extern int	poffset;		/* mjm: extern; also in ex_cmds.c */

/*
 * Subroutines for major command loop.
 */

/*
 * Is there a single letter indicating a named buffer next?
 */
int 
cmdreg(void)
{
	register int c = 0;
	register int wh = skipwh();

	if (wh && isalpha(peekchar()))
		c = getchar();
	return (c);
}

/*
 * Tell whether the character ends a command
 */
int 
endcmd(int ch)
{
	switch (ch) {
	
	case '\n':
	case EOF:
		endline = 1;
		return (1);
	
	case '|':
	case '"':
		endline = 0;
		return (1);
	}
	return (0);
}

/*
 * Insist on the end of the command.
 */
void 
eol(void)
{

	if (!skipend())
		error(catgets(catd, 1, 28,
			"Extra chars|Extra characters at end of command"));
	ignnEOF();
}

/*
 * Guts of the pre-printing error processing.
 * If in visual and catching errors, then we dont mung up the internals,
 * just fixing up the echo area for the print.
 * Otherwise we reset a number of externals, and discard unused input.
 */
void 
error0(void)
{

	if (vcatch) {
		if (splitw == 0)
			fixech();
		if (!SO || !SE)
			dingdong();
		return;
	}
	if (input && exitoneof) {
		input = strend(input) - 1;
		if (*input == '\n')
			setlastchar('\n');
		input = 0;
	}
	setoutt();
	flush();
	resetflav();
	if (!SO || !SE)
		dingdong();
	if (inopen) {
		/*
		 * We are coming out of open/visual ungracefully.
		 * Restore TCOLUMNS, undo, and fix tty mode.
		 */
		TCOLUMNS = OCOLUMNS;
		undvis();
		ostop(normf);
		/* ostop should be doing this
		putpad(VE);
		putpad(KE);
		*/
		putnl();
	}
	inopen = 0;
	holdcm = 0;
}

/*
 * Post error printing processing.
 * Close the i/o file if left open.
 * If catching in visual then throw to the visual catch,
 * else if a child after a fork, then exit.
 * Otherwise, in the normal command mode error case,
 * finish state reset, and throw to top.
 */
int 
error1(char *str)
{
	bool die;

	if (io > 0) {
		close(io);
		io = -1;
	}
	die = (getpid() != ppid);	/* Only children die */
	inappend = inglobal = 0;
	globp = NULL, vmacp = NULL, vglobp = NULL;
	if (vcatch && !die) {
		inopen = 1;
		vcatch = 0;
		if (str)
			noonl();
		fixol();
		LONGJMP(vreslab,1);
	}
	if (str && !vcatch)
		putNFL();
	if (die)
		exitex(1);
	if (exitoneof)
		lseek(0, (off_t)0, SEEK_END);
	if (inglobal)
		setlastchar('\n');
	while (lastchar() != '\n' && lastchar() != EOF)
		ignchar();
	ungetchar(0);
	endline = 1;
	reset();
}

/*
 * Print out the message in the error message file at str,
 * with i an integer argument to printf.
 */
/*VARARGS2*/
void
error(char *str, ...)
{
	va_list ap;

	error0();
	va_start(ap, str);
	vmerror(str, ap);
	va_end(ap);
	
	if (writing) {
		serror(catgets(catd, 1, 29,
				" [Warning - %s is incomplete]"), file);
		writing = 0;
	}
	error1(str);
}

/*
 * Same as error(), but using a va_list as argument.
 */
void
verror(char *str, va_list ap)
{
	error0();
	vmerror(str, ap);
	error(NULL);

	if (writing) {
		serror(catgets(catd, 1, 29,
				" [Warning - %s is incomplete]"), file);
		writing = 0;
	}
	error1(str);
}

/*
 * Rewind the argument list.
 */
void
erewind(void)
{

	argc = argc0;
	argv = argv0;
	args = args0;
	if (argc > 1 && !hush) {
		ex_printf(mesg(catgets(catd, 1, 30, "%d files@to edit")), argc);
		if (inopen)
			putchar(' ');
		else
			putNFL();
	}
}

void
fixol(void)
{
	if (Outchar != vputchar) {
		flush();
		if (state == ONEOPEN || state == HARDOPEN)
			outline = destline = 0;
		Outchar = vputchar;
		vcontin(1);
	} else {
		if (destcol)
			vclreol();
		vclean();
	}
}

/*
 * Does an ! character follow in the command stream?
 */
int
exclam(void)
{

	if (peekchar() == '!') {
		ignchar();
		return (1);
	}
	return (0);
}

/*
 * Make an argument list for e.g. next.
 */
void
makargs(void)
{

	gglob(&frob);
	argc0 = frob.argc0;
	argv0 = frob.argv;
	args0 = argv0[0];
	erewind();
}

/*
 * Advance to next file in argument list.
 */
void
next(void)
{
	extern short isalt;	/* defined in ex_io.c */

	if (argc == 0)
		error(catgets(catd, 1, 31, "No more files@to edit"));
	morargc = argc;
	isalt = (strcmp(altfile, args)==0) + 1;
	if (savedfile[0])
		lcpy(altfile, savedfile, sizeof altfile);
	safecp(savedfile, args, sizeof savedfile, "File name too long");
	argc--;
	args = argv ? *++argv : strend(args) + 1;
}

/*
 * Eat trailing flags and offsets after a command,
 * saving for possible later post-command prints.
 */
void
newline(void)
{
	register int c;

	resetflav();
	for (;;) {
		c = getchar();
		switch (c) {

		case '^':
		case '-':
			poffset--;
			break;

		case '+':
			poffset++;
			break;

		case 'l':
			listf++;
			break;

		case '#':
			nflag++;
			break;

		case 'p':
			listf = 0;
			break;

		case ' ':
		case '\t':
			continue;

		case '"':
			comment();
			setflav();
			return;

		default:
			if (!endcmd(c))
serror(catgets(catd, 1, 32,
	"Extra chars|Extra characters at end of \"%s\" command"), Command);
			if (c == EOF)
				ungetchar(c);
			setflav();
			return;
		}
		pflag++;
	}
}

/*
 * Before quit or respec of arg list, check that there are
 * no more files in the arg list.
 */
void
nomore(void)
{

	if (argc == 0 || morargc == argc)
		return;
	morargc = argc;
	merror(catgets(catd, 1, 33, "%d more file"), argc);
	serror(catgets(catd, 1, 34, "%s@to edit"), plural((long) argc));
}

/*
 * Before edit of new file check that either an ! follows
 * or the file has not been changed.
 */
int
quickly(void)
{

	if (exclam())
		return (1);
	if (chng && dol > zero) {
/*
		chng = 0;
*/
		xchng = 0;
		error(catgets(catd, 1, 35,
		"No write@since last change (:%s! overrides)"), Command);
	}
	return (0);
}

/*
 * Reset the flavor of the output to print mode with no numbering.
 */
void
resetflav(void)
{

	if (inopen)
		return;
	listf = 0;
	nflag = 0;
	pflag = 0;
	poffset = 0;
	setflav();
}

/*
 * Print an error message with a %s type argument to printf.
 * Message text comes from error message file.
 */
void
serror(char *str, ...)
{
	va_list ap;

	if (str == NULL)
		return;
	va_start(ap, str);
	error0();
	vsmerror(str, ap);
	error1(str);
	va_end(ap);
}

/*
 * Set the flavor of the output based on the flags given
 * and the number and list options to either number or not number lines
 * and either use normally decoded (ARPAnet standard) characters or list mode,
 * where end of lines are marked and tabs print as ^I.
 */
void
setflav(void)
{

	if (inopen)
		return;
	setnumb(nflag || value(NUMBER));
	setlist(listf || value(LIST));
	setoutt();
}

/*
 * Skip white space and tell whether command ends then.
 */
int
skipend(void)
{

	pastwh();
	return (endcmd(peekchar()) && peekchar() != '"');
}

/*
 * Set the command name for non-word commands.
 */
void
tailspec(int c)
{
	static char foocmd[2];

	foocmd[0] = c;
	Command = foocmd;
}

/*
 * Try to read off the rest of the command word.
 * If alphabetics follow, then this is not the command we seek.
 */
void
tail(char *comm)
{

	tailprim(comm, 1, 0);
}

void
tail2of(char *comm)
{

	tailprim(comm, 2, 0);
}

char	tcommand[20];

void
tailprim(register char *comm, int xi, bool notinvis)
{
	register char *cp;
	register int c;
	int	i = xi;

	Command = comm;
	for (cp = tcommand; i > 0; i--)
		*cp++ = *comm++;
	while (*comm && peekchar() == *comm)
		*cp++ = getchar(), comm++;
	c = peekchar();
	if (notinvis || isalpha(c)
#ifdef	BIT8
			|| (xi == 0 && (c&(0200|QUOTE)) == 0200)
#endif
			) {
		/*
		 * Of the trailing lp funny business, only dl and dp
		 * survive the move from ed to ex.
		 */
		if (tcommand[0] == 'd' && any(c, "lp"))
			goto ret;
		if (tcommand[0] == 's' && any(c, "gcr"))
			goto ret;
		while (cp < &tcommand[19] && (c = peekchar(),
				isalpha(c)
#ifdef	BIT8
				|| (xi == 0 && (c&(0200|QUOTE)) == 0200)
#endif
				))
			*cp++ = getchar();
		*cp = 0;
		if (notinvis)
			serror(catgets(catd, 1, 36,
		"What?|%s: No such command from open/visual"), tcommand);
		else
			serror(catgets(catd, 1, 37,
				"What?|%s: Not an editor command"), tcommand);
	}
ret:
	*cp = 0;
}

/*
 * Continue after a : command from open/visual.
 */
void
vcontin(bool ask)
{

	if (vcnt > 0)
		vcnt = -vcnt;
	if (inopen) {
		if (state != VISUAL) {
			/*
			 * We don't know what a shell command may have left on
			 * the screen, so we move the cursor to the right place
			 * and then put out a newline.  But this makes an extra
			 * blank line most of the time so we only do it for :sh
			 * since the prompt gets left on the screen.
			 *
			 * BUG: :!echo longer than current line \\c
			 * will screw it up, but be reasonable!
			 */
			if (state == CRTOPEN) {
				termreset();
				vgoto(WECHO, 0);
			}
			if (!ask) {
				putch('\r');
				putch('\n');
			}
			return;
		}
		if (ask) {
			merror(catgets(catd, 1, 38,
					"[Hit return to continue] "));
			getline(*dot);
			flush();
		}
		if (ask) {
#ifdef EATQS
			/*
			 * Gobble ^Q/^S since the tty driver should be eating
			 * them (as far as the user can see)
			 */
			while (peekkey(0) == CTRL('Q')
					|| peekkey() == CTRL('S'))
				ignore(getkey());
#endif
			if(getkey() == ':') {
				/* Ugh. Extra newlines, but no other way */
				putch('\n');
				outline = WECHO;
				ungetkey(':');
			}
		}
		vclrech(1);
		if (Peekkey != ':') {
			putpad(TI);
			tostart();
			/* replaced by ostart.
			putpad(VS);
			putpad(KS);
			*/
		}
	}
}

/*
 * Put out a newline (before a shell escape)
 * if in open/visual.
 */
void
vnfl(void)
{

	if (inopen) {
		if (state != VISUAL && state != CRTOPEN && destline <= WECHO)
			vclean();
		else
			vmoveitup(1, 0);
		vgoto(WECHO, 0);
		vclrcell(vtube[WECHO], WCOLS);
		tostop();
		/* replaced by the ostop above
		putpad(VE);
		putpad(KE);
		*/
	}
	flush();
}
