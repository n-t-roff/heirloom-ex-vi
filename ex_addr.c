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
static char sccsid[] = "@(#)ex_addr.c	1.11 (gritter) 8/4/05";
#endif
#endif /* not lint */

/* from ex_addr.c	7.3 (Berkeley) 6/7/85 */

#include "ex.h"
#include "ex_re.h"

/*
 * Routines for address parsing and assignment and checking of address bounds
 * in command mode.  The routine address is called from ex_cmds.c
 * to parse each component of a command (terminated by , ; or the beginning
 * of the command itself.  It is also called by the scanning routine
 * in ex_voperate.c from within open/visual.
 *
 * Other routines here manipulate the externals addr1 and addr2.
 * These are the first and last lines for the current command.
 *
 * The variable bigmove remembers whether a non-local glitch of . was
 * involved in an address expression, so we can set the previous context
 * mark '' when such a motion occurs.
 */

static	bool bigmove;

/*
 * Set up addr1 and addr2 for commands whose default address is dot.
 */
void 
setdot(void)
{

	setdot1();
	if (bigmove)
		markDOT();
}

/*
 * Call setdot1 to set up default addresses without ever
 * setting the previous context mark.
 */
void 
setdot1(void)
{

	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2) {
		notempty();
		failed = 1;
		error(catgets(catd, 1, 6,
			"Addr1 > addr2|First address exceeds second"));
	}
}

/*
 * Ex allows you to say
 *	delete 5
 * to delete 5 lines, etc.
 * Such nonsense is implemented by setcount.
 */
void 
setcount(void)
{
	register int cnt;

	pastwh();
	if (!isdigit(peekchar())) {
		setdot();
		return;
	}
	addr1 = addr2;
	setdot();
	cnt = getnum();
	if (cnt <= 0)
		error(catgets(catd, 1, 7, "Bad count|Nonzero count required"));
	addr2 += cnt - 1;
	if (addr2 > dol)
		addr2 = dol;
	nonzero();
}

/*
 * Parse a number out of the command input stream.
 */
int 
getnum(void)
{
	register int cnt;

	for (cnt = 0; isdigit(peekcd());)
		cnt = cnt * 10 + getchar() - '0';
	return (cnt);
}

/*
 * Set the default addresses for commands which use the whole
 * buffer as default, notably write.
 */
void 
setall(void)
{

	if (addr2 == 0) {
		addr1 = one;
		addr2 = dol;
		if (dol == zero) {
			dot = zero;
			return;
		}
	}
	/*
	 * Don't want to set previous context mark so use setdot1().
	 */
	setdot1();
}

/*
 * No address allowed on, e.g. the file command.
 */
void 
setnoaddr(void)
{

	if (addr2 != 0) {
		failed = 1;
		error(catgets(catd, 1, 8,
				"No address allowed@on this command"));
	}
}

/*
 * Parse an address.
 * Just about any sequence of address characters is legal.
 *
 * If you are tricky you can use this routine and the = command
 * to do simple addition and subtraction of cardinals less
 * than the number of lines in the file.
 */
line *
address(char *in_line)
{
	register line *addr;
	register int offset, c;
	short lastsign;

	bigmove = 0;
	lastsign = 0;
	offset = 0;
	addr = 0;
	for (;;) {
		if (isdigit(peekcd())) {
			if (addr == 0) {
				addr = zero;
				bigmove = 1;
			}
			loc1 = 0;
			addr += offset;
			offset = getnum();
			if (lastsign >= 0)
				addr += offset;
			else
				addr -= offset;
			lastsign = 0;
			offset = 0;
		}
		switch (c = getcd()) {

		case '?':
		case '/':
		case '$':
		case '\'':
		case '\\':
			bigmove++;
		case '.':
			if (addr || offset)
				error(catgets(catd, 1, 9,
						"Badly formed address"));
		}
		offset += lastsign;
		lastsign = 0;
		switch (c) {

		case ' ':
		case '\t':
			continue;

		case '+':
			lastsign = 1;
			if (addr == 0)
				addr = dot;
			continue;

		case '^':
		case '-':
			lastsign = -1;
			if (addr == 0)
				addr = dot;
			continue;

		case '\\':
		case '?':
		case '/':
			c = compile(c, 1);
			notempty();
			savere(&scanre);
			addr = dot;
			if (in_line && execute(0, dot)) {
				if (c == '/') {
					while (loc1 <= in_line) {
						if (loc1 == loc2)
							loc2++;
						if (!execute(1, NULL))
							goto nope;
					}
					break;
				} else if (loc1 < in_line) {
					char *last;
doques:

					do {
						last = loc1;
						if (loc1 == loc2)
							loc2++;
						if (!execute(1, NULL))
							break;
					} while (loc1 < in_line);
					loc1 = last;
					break;
				}
			}
nope:
			for (;;) {
				if (c == '/') {
					addr++;
					if (addr > dol) {
						if (value(WRAPSCAN) == 0)
error(catgets(catd, 1, 10, "No match to BOTTOM|Address search hit BOTTOM without matching pattern"));
						addr = zero;
					}
				} else {
					addr--;
					if (addr < zero) {
						if (value(WRAPSCAN) == 0)
error(catgets(catd, 1, 11, "No match to TOP|Address search hit TOP without matching pattern"));
						addr = dol;
					}
				}
				if (execute(0, addr)) {
					if (in_line && c == '?') {
						in_line = &linebuf[LBSIZE];
						goto doques;
					}
					break;
				}
				if (addr == dot)
					error(catgets(catd, 1, 12,
						"Fail|Pattern not found"));
			}
			continue;

		case '$':
			addr = dol;
			continue;

		case '.':
			addr = dot;
			continue;

		case '\'':
			c = markreg(getchar());
			if (c == 0)
				error(catgets(catd, 1, 13,
						"Marks are ' and a-z"));
			addr = getmark(c);
			if (addr == 0)
				error(catgets(catd, 1, 14,
						"Undefined mark@referenced"));
			break;

		default:
			ungetchar(c);
			if (offset) {
				if (addr == 0)
					addr = dot;
				addr += offset;
				loc1 = 0;
			}
			if (addr == 0) {
				bigmove = 0;
				return (0);
			}
			if (addr != zero)
				notempty();
			addr += lastsign;
			if (addr < zero) {
				failed = 1;
				error(catgets(catd, 1, 15,
				"Negative address@- first buffer line is 1"));
			}
			if (addr > dol) {
				failed = 1;
				error(catgets(catd, 1, 16,
					"Not that many lines@in buffer"));
			}
			return (addr);
		}
	}
}

/*
 * Abbreviations to make code smaller
 * Left over from squashing ex version 1.1 into
 * 11/34's and 11/40's.
 */
void 
setCNL(void)
{

	setcount();
	newline();
}

void 
setNAEOL(void)
{

	setnoaddr();
	eol();
}
