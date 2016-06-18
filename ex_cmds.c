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
static char sccsid[] = "@(#)ex_cmds.c	1.22 (gritter) 2/18/05";
#endif
#endif

/* from ex_cmds.c	7.10.1 (2.11BSD) 1996/11/23 */

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

bool	pflag, nflag;
int	poffset;

#define	nochng()	lchng = chng

/*
 * Main loop for command mode command decoding.
 * A few commands are executed here, but main function
 * is to strip command addresses, do a little address oriented
 * processing and call command routines to do the real work.
 */
void 
commands(int noprompt, int _exitoneof)
{
	register line *addr;
	register int c;
	register int lchng;
	int given;
	int seensemi;
	int cnt;
	bool hadpr = 0;

	resetflav();
	nochng();
	for (;;) {
		exitoneof = _exitoneof;
		/*
		 * If dot at last command
		 * ended up at zero, advance to one if there is a such.
		 */
		if (dot <= zero) {
			dot = zero;
			if (dol > zero)
				dot = one;
		}
		shudclob = 0;

		/*
		 * If autoprint or trailing print flags,
		 * print the line at the specified offset
		 * before the next command.
		 */
		if (pflag ||
		    (lchng != chng && value(AUTOPRINT) && !inglobal && !inopen && endline)) {
			pflag = 0;
			nochng();
			if (dol != zero) {
				addr1 = addr2 = dot + poffset;
				if (addr1 < one || addr1 > dol)
error(catgets(catd, 1, 17,
		"Offset out-of-bounds|Offset after command too large"));
				setdot1();
				goto print;
			}
		}
		nochng();

		/*
		 * Print prompt if appropriate.
		 * If not in global flush output first to prevent
		 * going into pfast mode unreasonably.
		 */
		if (inglobal == 0) {
			flush();
			if (!hush && value(PROMPT) && !globp && !noprompt && endline) {
				putchar(':');
				hadpr = 1;
			}
			TSYNC();
		}

		/*
		 * Gobble up the address.
		 * Degenerate addresses yield ".".
		 */
		addr2 = 0;
		given = seensemi = 0;
		do {
			addr1 = addr2;
			addr = address(0);
			c = getcd();
			if (addr == 0) {
				if (c == ',')
					addr = dot;
				else if (addr1 != 0) {
					addr2 = dot;
					break;
				} else
					break;
			}
			addr2 = addr;
			given++;
			if (c == ';') {
				c = ',';
				dot = addr;
				seensemi = 1;
			}
		} while (c == ',');
		if (c == '%') {
			/* %: same as 1,$ */
			addr1 = one;
			addr2 = dol;
			given = 2;
			c = getchar();
		}
		if (addr1 == 0)
			addr1 = addr2;
		if (c == ':')
			c = getchar();

		/*
		 * Set command name for special character commands.
		 */
		tailspec(c);

		/*
		 * If called via : escape from open or visual, limit
		 * the set of available commands here to save work below.
		 */
		if (inopen) {
			if (c=='\n' || c=='\r' || c==CTRL('d') || c==EOF) {
				if (addr2)
					dot = addr2;
				if (c == EOF)
					return;
				continue;
			}
			if (any(c, "o"))
notinvis:
				tailprim(Command, 1, 1);
		}
/* choice: */
		switch (c) {

		case 'a':

			switch(peekchar()) {
			case 'b':
/* abbreviate */
				tail("abbreviate");
				setnoaddr();
				mapcmd(0, 1);
				anyabbrs = 1;
				continue;
			case 'r':
/* args */
				tail("args");
				setnoaddr();
				eol();
				pargs();
				continue;
			}

/* append */
			if (inopen)
				goto notinvis;
			tail("append");
			setdot();
			aiflag = exclam();
			newline();
			vmacchng(0);
			deletenone();
			setin(addr2);
			inappend = 1;
			ignore(append(gettty, addr2));
			inappend = 0;
			nochng();
			continue;

		case 'c':
			switch (peekchar()) {

/* copy */
			case 'o':
				tail("copy");
				vmacchng(0);
				move();
				continue;

#ifdef CHDIR
/* cd */
			case 'd':
				tail("cd");
				goto changdir;

/* chdir */
			case 'h':
				ignchar();
				if (peekchar() == 'd') {
					register char *p;
					tail2of("chdir");
changdir:
					if (savedfile[0] == '/' || !value(WARN))
						ignore(exclam());
					else
						ignore(quickly());
					if (skipend()) {
						p = getenv("HOME");
						if (p == NULL)
							error(catgets(catd, 1,
						18, "Home directory unknown"));
					} else
						getone(), p = file;
					eol();
					if (chdir(p) < 0)
						filioerr(p);
					if (savedfile[0] != '/')
						edited = 0;
					continue;
				}
				if (inopen)
					tailprim("change", 2, 1);
				tail2of("change");
				break;

#endif
			default:
				if (inopen)
					goto notinvis;
				tail("change");
				break;
			}
/* change */
			aiflag = exclam();
			setCNL();
			vmacchng(0);
			setin(addr1);
			delete(0);
			inappend = 1;
			ignore(append(gettty, addr1 - 1));
			inappend = 0;
			nochng();
			continue;

/* delete */
		case 'd':
			/*
			 * Caution: dp and dl have special meaning already.
			 */
			tail("delete");
			c = cmdreg();
			setCNL();
			vmacchng(0);
			if (c)
				YANKreg(c);
			delete(0);
			appendnone();
			vkillDEL();
			continue;

/* edit */
/* ex */
		case 'e':
			tail(peekchar() == 'x' ? "ex" : "edit");
editcmd:
			if (!exclam() && chng)
				c = 'E';
			filename(c);
			if (c == 'E') {
				ungetchar(lastchar());
				ignore(quickly());
			}
			setnoaddr();
doecmd:
			init();
			addr2 = zero;
			laste++;
			synced();
			rop(c);
#ifdef INCORB
			tlaste();
#endif
			laste = 0;
			synced();
			nochng();
			continue;

/* file */
		case 'f':
			tail("file");
			setnoaddr();
			filename(c);
			noonl();
/*
			synctmp();
*/
			continue;

/* global */
		case 'g':
			tail("global");
			global(!exclam());
			nochng();
			continue;

/* insert */
		case 'i':
			if (inopen)
				goto notinvis;
			tail("insert");
			setdot();
			nonzero();
			aiflag = exclam();
			newline();
			vmacchng(0);
			deletenone();
			setin(addr2);
			inappend = 1;
			ignore(append(gettty, addr2 - 1));
			inappend = 0;
			if (dot == zero && dol > zero)
				dot = one;
			nochng();
			continue;

/* join */
		case 'j':
			tail("join");
			c = exclam();
			setcount();
			nonzero();
			newline();
			vmacchng(0);
			if (given < 2 && addr2 != dol)
				addr2++;
			join(c);
			continue;

/* k */
		case 'k':
casek:
			pastwh();
			c = getchar();
			if (endcmd(c))
				serror(catgets(catd, 1, 19,
		"Mark what?|%s requires following letter"), Command);
			newline();
			if (!islower(c))
				error(catgets(catd, 1, 20,
				"Bad mark|Mark must specify a letter"));
			setdot();
			nonzero();
			names[c - 'a'] = *addr2 &~ 01;
			anymarks = 1;
			continue;

/* list */
		case 'l':
			tail("list");
			setCNL();
			ignorf(setlist(1));
			pflag = 0;
			goto print;

		case 'm':
			if (peekchar() == 'a') {
				ignchar();
				if (peekchar() == 'p') {
/* map */
					tail2of("map");
					setnoaddr();
					mapcmd(0, 0);
					continue;
				}
/* mark */
				tail2of("mark");
				goto casek;
			}
/* move */
			tail("move");
			vmacchng(0);
			move();
			continue;

		case 'n':
			if (peekchar() == 'u') {
				tail("number");
				goto numberit;
			}
/* next */
			tail("next");
			setnoaddr();
			if (!exclam())
				ckaw();
			ignore(quickly());
			if (getargs())
				makargs();
			next();
			c = 'e';
			filename(c);
			if (recov)
				goto recovnext;
			goto doecmd;

/* open */
		case 'o':
			tail("open");
			oop();
			pflag = 0;
			nochng();
			continue;

		case 'p':
		case 'P':
			switch (peekchar()) {

/* put */
			case 'u':
				tail("put");
				setdot();
				c = cmdreg();
				eol();
				vmacchng(0);
				if (c)
					putreg(c);
				else
					put(0);
				continue;

			case 'r':
				ignchar();
				if (peekchar() == 'e') {
/* preserve */
					tail2of("preserve");
					eol();
					if (preserve() == 0)
						error(catgets(catd, 1, 21,
							"Preserve failed!"));
					else
						error(catgets(catd, 1, 22,
							"File preserved."));
				}
				tail2of("print");
				break;

			default:
				tail("print");
				break;
			}
/* print */
			setCNL();
			pflag = 0;
print:
			nonzero();
			if (CL && span() > TLINES) {
				flush1();
				vclear();
			}
			plines(addr1, addr2, 1);
			continue;

/* quit */
		case 'q':
			tail("quit");
			setnoaddr();
			c = quickly();
			eol();
			if (!c)
quit:
				nomore();
			if (inopen) {
				vgoto(WECHO, 0);
				if (!ateopr())
					vnfl();
				else {
					tostop();
				}
				flush();
				setty(normf);
			}
			cleanup(1);
			exitex(0);

		case 'r':
			if (peekchar() == 'e') {
				ignchar();
				switch (peekchar()) {

/* rewind */
				case 'w':
					tail2of("rewind");
					setnoaddr();
					if (!exclam()) {
						ckaw();
						if (chng && dol > zero)
							error(catgets(catd,
		1, 23, "No write@since last change (:rewind! overrides)"));
					}
					eol();
					erewind();
					next();
					c = 'e';
					ungetchar(lastchar());
					filename(c);
					goto doecmd;

/* recover */
				case 'c':
					tail2of("recover");
					setnoaddr();
					c = 'e';
					if (!exclam() && chng)
						c = 'E';
					filename(c);
					if (c == 'E') {
						ungetchar(lastchar());
						ignore(quickly());
					}
recovnext:
					init();
					addr2 = zero;
					laste++;
					synced();
					recover();
					rop2();
					revocer();
					if (status == 0)
						rop3(c);
					if (dol != zero)
						change();
#ifdef INCORB
					tlaste();
#endif
					laste = 0;
					nochng();
					continue;
				}
				tail2of("read");
			} else
				tail("read");
/* read */
			if (savedfile[0] == 0 && dol == zero)
				c = 'e';
			pastwh();
			vmacchng(0);
			if (peekchar() == '!') {
				setdot();
				ignchar();
				unix0(0);
				filter(0);
				continue;
			}
			filename(c);
			rop(c);
			nochng();
			if (inopen && endline && addr1 > zero && addr1 < dol)
				dot = addr1 + 1;
			continue;

		case 's':
			switch (peekchar()) {
			/*
			 * Caution: 2nd char cannot be c, g, or r
			 * because these have meaning to substitute.
			 */

/* set */
			case 'e':
				tail("set");
				setnoaddr();
				set();
				continue;

/* shell */
			case 'h':
				tail("shell");
				setNAEOL();
				vnfl();
				putpad(TE);
				flush();
				unixwt(1, unixex("-i", (char *) 0, 0, 0));
				vcontin(0);
				continue;

/* source */
			case 'o':
#ifdef notdef
				if (inopen)
					goto notinvis;
#endif
				tail("source");
				setnoaddr();
				getone();
				eol();
				source(file, 0);
				continue;
#ifdef SIGTSTP
/* stop, suspend */
			case 't':
				tail("stop");
				goto suspend;
			case 'u':
				getchar();
				if (peekchar() == 'b') {
					tail2of("substitute");
					goto substitute;
				}
				tail2of("suspend");
suspend:
				if (!ldisc)
					error(catgets(catd, 1, 24,
			"Old tty driver|Not using new tty driver/shell"));
				c = exclam();
				eol();
				if (!c)
					ckaw();
#ifdef	SIGTSTP
				onsusp(SIGTSTP);
#endif
				continue;
#endif

			}
			/* fall into ... */

/* & */
/* ~ */
/* substitute */
		case '&':
	 	case '~':
			Command = "substitute";
			if (c == 's')
				tail(Command);
substitute:
			vmacchng(0);
			if (!substitute(c))
				pflag = 0;
			continue;

/* t */
		case 't':
			if (peekchar() == 'a') {
				tail("tag");
				tagfind(exclam());
				if (!inopen)
					lchng = chng - 1;
				else
					nochng();
				continue;
			}
			tail("t");
			vmacchng(0);
			move();
			continue;

		case 'u':
			if (peekchar() == 'n') {
				ignchar();
				switch(peekchar()) {
/* unmap */
				case 'm':
					tail2of("unmap");
					setnoaddr();
					mapcmd(1, 0);
					continue;
/* unabbreviate */
				case 'a':
					tail2of("unabbreviate");
					setnoaddr();
					mapcmd(1, 1);
					anyabbrs = 1;
					continue;
				}
/* undo */
				tail2of("undo");
			} else
				tail("undo");
			setnoaddr();
			markDOT();
			c = exclam();
			newline();
			undo(c);
			continue;

		case 'v':
			switch (peekchar()) {

			case 'e':
/* version */
				tail("version");
				setNAEOL();
				printver();
				noonl();
				continue;

/* visual */
			case 'i':
				tail("visual");
				if (inopen) {
					c = 'e';
					goto editcmd;
				}
				vop();
				pflag = 0;
				nochng();
				continue;
			}
/* v */
			tail("v");
			global(0);
			nochng();
			continue;

/* write */
		case 'w':
			c = peekchar();
			tail(c == 'q' ? "wq" : "write");
wq:
			if (skipwh() && peekchar() == '!') {
				pofix();
				ignchar();
				setall();
				unix0(0);
				filter(1);
			} else {
				setall();
				wop(1);
				nochng();
			}
			if (c == 'q')
				goto quit;
			continue;

/* xit */
		case 'x':
			tail("xit");
			if (!chng)
				goto quit;
			c = 'q';
			goto wq;

/* yank */
		case 'y':
			tail("yank");
			c = cmdreg();
			setcount();
			eol();
			vmacchng(0);
			if (c)
				YANKreg(c);
			else
				yank(0);
			vkillDEL();
			continue;

/* z */
		case 'z':
			zop(0);
			pflag = 0;
			continue;

/* * */
/* @ */
		case '*':
		case '@':
			c = getchar();
			if (c=='\n' || c=='\r')
				ungetchar(c);
			if (any(c, "@*\n\r")) {
				c = lastmac;
				if (c == 0)
					failed = 1;
			}
			if (isupper(c))
				c = tolower(c);
			if (!islower(c))
				error(catgets(catd, 1, 25, "Bad register"));
			newline();
			setdot();
			cmdmac(c);
			continue;

/* | */
		case '|':
			endline = 0;
			goto caseline;

/* \n */
		case '\n':
			endline = 1;
caseline:
			notempty();
			if (addr2 == 0) {
				if (UP != NOSTR && c == '\n' && !inglobal)
					c = CTRL('k');
				if (inglobal)
					addr1 = addr2 = dot;
				else {
					if (dot == dol)
						error(catgets(catd, 1, 26,
						"At EOF|At end-of-file"));
					addr1 = addr2 = dot + 1;
				}
			}
			setdot();
			nonzero();
			if (seensemi)
				addr1 = addr2;
			getline(*addr1);
			if (c == CTRL('k')) {
				flush1();
				destline--;
				if (hadpr)
					shudclob = 1;
			}
			plines(addr1, addr2, 1);
			continue;

/* " */
		case '"':
			comment();
			continue;

/* # */
		case '#':
numberit:
			setCNL();
			ignorf(setnumb(1));
			pflag = 0;
			goto print;

/* = */
		case '=':
			newline();
			setall();
			if (inglobal == 2)
				pofix();
			ex_printf("%d", lineno(addr2));
			noonl();
			continue;

/* ! */
		case '!':
			if (addr2 != 0) {
				vmacchng(0);
				unix0(0);
				setdot();
				filter(2);
			} else {
				unix0(1);
				pofix();
				putpad(TE);
				flush();
				unixwt(1, unixex("-c", uxb, 0, 0));
				vclrech(1);	/* vcontin(0); */
				nochng();
			}
			continue;

/* < */
/* > */
		case '<':
		case '>':
			for (cnt = 1; peekchar() == c; cnt++)
				ignchar();
			setCNL();
			vmacchng(0);
			shift(c, cnt);
			continue;

/* ^D */
/* EOF */
		case CTRL('d'):
		case EOF:
			if (exitoneof) {
				if (addr2 != 0)
					dot = addr2;
				return;
			}
			if (!isatty(0)) {
				if (intty)
					/*
					 * Chtty sys call at UCB may cause a
					 * input which was a tty to suddenly be
					 * turned into /dev/null.
					 */
					onhup(SIGHUP);
				return;
			}
			if (addr2 != 0) {
				setlastchar('\n');
				putnl();
			}
			if (dol == zero) {
				if (addr2 == 0)
					putnl();
				notempty();
			}
			ungetchar(EOF);
			zop(hadpr);
			continue;

		default:
			if (!isalpha(c) && (c&0200) == 0)
				break;
			ungetchar(c);
			tailprim("", 0, 0);
		}
		error(catgets(catd, 1, 27,
				"What?|Unknown command character '%c'"), c);
	}
}
