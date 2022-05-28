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
static char sccsid[] = "@(#)ex_subr.c	1.41 (gritter) 12/25/06";
#endif
#endif

/* from ex_subr.c	7.10.1 (2.11BSD) 1996/3/22 */

#include <sys/ioctl.h>
#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

short	lastsc;

/*
 * Random routines, in alphabetical order.
 */

int 
any(int c, register char *s)
{
	register int x;

	while ((x = *s++))
		if (x == c)
			return (1);
	return (0);
}

int 
backtab(register int i)
{
	register int j;

	j = i % value(SHIFTWIDTH);
	if (j == 0)
		j = value(SHIFTWIDTH);
	i -= j;
	if (i < 0)
		i = 0;
	return (i);
}

void 
change(void)
{

	tchng++;
	chng = tchng;
	fixedzero = 0;
}

/*
 * Column returns the number of
 * columns occupied by printing the
 * characters through position cp of the
 * current line.
 */
int 
column(register char *cp)
{

	if (cp == 0)
		cp = &linebuf[LBSIZE - 2];
	return (qcolumn(cp, NULL));
}

int
lcolumn(register char *cp)
{
	return column(cp) - (lastsc - 1);
}

/*
 * Ignore a comment to the end of the line.
 * This routine eats the trailing newline so don't call newline().
 */
void 
comment(void)
{
	register int c;

	do {
		c = getchar();
	} while (c != '\n' && c != EOF);
	if (c == EOF)
		ungetchar(c);
}

/*
 * strlcpy not used since buffers may overlap.
 */
size_t
lcpy(char *dst, const char *src, const size_t dstsize) {
	size_t srclen = 0;
	size_t copylen = dstsize;
	/* avoids to access illegal memory in case
	 * of unterminated strings */
	while (srclen < copylen && src[srclen]) {
		++srclen;
	}
	if (srclen < copylen) {
		/* '<' means there is room for the final 0 byte */
		copylen = srclen;
	} else if (copylen) {
		/* src string is too long. Set size to be copied to
		 * buffer size - 1 to have room for the final 0 byte */
		--copylen;
	}
	if (copylen) {
		memcpy(dst, src, copylen);
	}
	if (dstsize) {
		dst[copylen] = 0;
	}
	return srclen; /* [sic!] see strlcpy man page */
}

size_t
lcat(char *dst, const char *src, const size_t dstsize) {
	size_t srclen = 0;
	size_t dstlen = 0;
	size_t copylen = dstsize;
	while (dstlen < copylen && dst[dstlen]) {
		++dstlen;
	}
	dst     += dstlen;
	copylen -= dstlen;
	while (srclen < copylen && src[srclen]) {
		++srclen;
	}
	if (srclen < copylen) {
		copylen = srclen;
	} else if (copylen) {
		--copylen;
	}
	if (copylen) {
		memcpy(dst, src, copylen);
	}
	if (dstsize) {
		dst[dstlen + copylen] = 0;
	}
	return dstlen + srclen; /* [sic!] see strlcat man page */
}

void 
copyw(register line *to, register line *from, register int size)
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

void 
copywR(register line *to, register line *from, register int size)
{

	while (--size >= 0)
		to[size] = from[size];
}

int 
ctlof(int c)
{

	return (c == DELETE ? '?' : c | ('A' - 1));
}

void 
dingdong(void)
{

	if (VB)
		putpad(VB);
	else if (value(ERRORBELLS))
		putch('\207');
}

int 
fixindent(int indent)
{
	register int i;
	register char *cp;

	i = whitecnt(genbuf);
	cp = vpastwh(genbuf);
	if (*cp == 0 && i == indent && linebuf[0] == 0) {
		genbuf[0] = 0;
		return (i);
	}
	CP(genindent(i), cp);
	return (i);
}

void 
filioerr(char *cp)
{
	register int oerrno = errno;

	lprintf("\"%s\"", cp);
	errno = oerrno;
	syserror();
}

char *
genindent(register int indent)
{
	register char *cp;

	for (cp = genbuf; indent >= value(TABSTOP); indent -= value(TABSTOP))
		*cp++ = '\t';
	for (; indent > 0; indent--)
		*cp++ = ' ';
	return (cp);
}

void 
getDOT(void)
{

	getline(*dot);
}

line *
getmark(register int c)
{
	register line *addr;
	
	for (addr = one; addr <= dol; addr++)
		if (names[c - 'a'] == (*addr &~ 01)) {
			return (addr);
		}
	return (0);
}

int 
getn(register char *cp)
{
	register int i = 0;

	while (isdigit(*cp&0377))
		i = i * 10 + *cp++ - '0';
	if (*cp)
		return (0);
	return (i);
}

void 
ignnEOF(void)
{
	register int c = getchar();

	if (c == EOF)
		ungetchar(c);
	else if (c=='"')
		comment();
}

int 
is_white(int c)
{

#ifndef	BIT8
	return (c == ' ' || c == '\t');
#else
	return (isspace(c&0377) && c != '\n' && c != '\r'
			&& c != '\f' && c != '\v');
#endif
}

int 
junk(register int c)
{

	if (c && !value(BEAUTIFY))
		return (0);
#ifndef	BIT8
	if (c >= ' ' && c != DELETE)
#else
	if (printable(c))
#endif
		return (0);
	switch (c) {

	case '\t':
	case '\n':
	case '\f':
		return (0);

	default:
		return (1);
	}
}

void 
killed(void)
{

	killcnt(addr2 - addr1 + 1);
}

void 
killcnt(register int cnt)
{

	if (inopen) {
		notecnt = cnt;
		notenam = notesgn = "";
		return;
	}
	if (!notable(cnt))
		return;
	ex_printf(catgets(catd, 1, 170, "%d lines"), cnt);
	if (value(TERSE) == 0) {
		ex_printf(catgets(catd, 1, 171, " %c%s"),
				Command[0] | ' ', Command + 1);
		if (Command[strlen(Command) - 1] != 'e')
			putchar('e');
		putchar('d');
	}
	putNFL();
}

int 
lineno(line *a)
{

	return (a - zero);
}

int 
lineDOL(void)
{

	return (lineno(dol));
}

int 
lineDOT(void)
{

	return (lineno(dot));
}

void 
markDOT(void)
{

	markpr(dot);
}

void 
markpr(line *which)
{

	if ((inglobal == 0 || inopen) && which <= endcore) {
		names['z'-'a'+1] = *which & ~01;
		if (inopen)
			ncols['z'-'a'+1] = cursor;
	}
}

int 
markreg(register int c)
{

	if (c == '\'' || c == '`')
		return ('z' + 1);
	if (c >= 'a' && c <= 'z')
		return (c);
	return (0);
}

/*
 * Mesg decodes the terse/verbose strings. Thus
 *	'xxx@yyy' -> 'xxx' if terse, else 'xxx yyy'
 *	'xxx|yyy' -> 'xxx' if terse, else 'yyy'
 * All others map to themselves.
 */
char *
mesg(register char *str)
{
	register char *cp;

	str = strcpy(genbuf, str);
	for (cp = str; *cp; cp++)
		switch (*cp) {

		case '@':
			if (value(TERSE))
				*cp = 0;
			else
				*cp = ' ';
			break;

		case '|':
			if (value(TERSE) == 0)
				return (cp + 1);
			*cp = 0;
			break;
		}
	return (str);
}

void 
merror1(intptr_t seekpt)
{

#ifdef VMUNIX
	lcpy(linebuf, (char *)seekpt, LBSIZE);
#else
	lseek(erfile, (off_t) seekpt, SEEK_SET);
	if (read(erfile, linebuf, 128) < 2)
		lcpy(linebuf, "ERROR", LBSIZE);
#endif
}

/*VARARGS2*/
void
vmerror(char *seekpt, va_list ap)
{

	register char *cp = linebuf;

	if (seekpt == 0)
		return;
	merror1((intptr_t)seekpt);
	if (*cp == '\n')
		putnl(), cp++;
	if (inopen > 0 && CE)
		vclreol();
	if (SO && SE)
		putpad(SO);
	vprintf(mesg(cp), ap);
	if (SO && SE)
		putpad(SE);
}

void
merror(char *cp, ...)
{
	va_list ap;

	if (cp == NULL)
		return;
	va_start(ap, cp);
	vmerror(cp, ap);
	va_end(ap);
}

int 
morelines(void)
{
#ifdef	_SC_PAGESIZE
	static long pg;

	if (pg == 0) {
		pg = sysconf(_SC_PAGESIZE);
		if (pg <= 0 || pg >= 65536)
			pg = 4096;
		pg /= sizeof (line);
	}
	if ((char *)sbrk(pg * sizeof (line)) == (char *)-1)
		return (-1);
	endcore += pg;
	return (0);
#else	/* !_SC_PAGESIZE */
	if (sbrk(1024 * sizeof (line)) == (char *)-1)
		return (-1);
	endcore += 1024;
	return (0);
#endif	/* !_SC_PAGESIZE */
}

void 
nonzero(void)
{

	if (addr1 == zero) {
		notempty();
		error(catgets(catd, 1, 172,
			"Nonzero address required@on this command"));
	}
}

int 
notable(int i)
{

	return (hush == 0 && !inglobal && i > value(REPORT));
}


void 
notempty(void)
{

	if (dol == zero)
		error(catgets(catd, 1, 173, "No lines@in the buffer"));
}


void 
netchHAD(int cnt)
{

	netchange(lineDOL() - cnt);
}

void 
netchange(register int i)
{
	register char *cp;

	if (i > 0)
		notesgn = cp = catgets(catd, 1, 174, "more ");
	else
		notesgn = cp = catgets(catd, 1, 175, "fewer "), i = -i;
	if (inopen) {
		notecnt = i;
		notenam = catgets(catd, 1, 176, "");
		return;
	}
	if (!notable(i))
		return;
	ex_printf(mesg(catgets(catd, 1, 177, "%d %slines@in file after %s")),
			i, cp, Command);
	putNFL();
}

/*
 * Print an escape sequence corresponding to c.
 */
#ifdef	BIT8
int 
printof(int c)
{
	char *nums = "01234567";
	int d;

#ifdef	MB
	if (mb_cur_max > 1 && (c & INVBIT) == 0 && c & ~0177) {
		char	mb[MB_LEN_MAX];
		int	i, n, x = EOF;
		if ((n = wctomb(mb, c & TRIM)) <= 0) {
			n = 1;
			*mb = 0;
		}
		for (i = 0; i < n; i++) {
			x = printof(mb[i] | INVBIT);
			if (i+1 < n)
				normchar(x);
		}
		return x;
	}
#endif	/* MB */
	c &= 0377;
	if (c < 040 || c == DELETE) {
		normchar('^');
		return (c == DELETE ? '?' : c | ('A' - 1));
	}
	normchar('\\');
	normchar(nums[(c & ~077) >> 6]);
	c &= 077;
	d = c & 07;
	if (c > d)
		normchar(nums[(c - d) >> 3]);
	else
		normchar(nums[0]);
	return nums[d];
}
#endif

void 
putmark(line *addr)
{

	putmk1(addr, putline());
}

void 
putmk1(register line *addr, int n)
{
	register line *markp;
	register int oldglobmk;

	oldglobmk = *addr & 1;
	*addr &= ~1;
	for (markp = (anymarks ? names : &names['z'-'a'+1]);
	  markp <= &names['z'-'a'+1]; markp++)
		if (*markp == *addr)
			*markp = n;
	*addr = n | oldglobmk;
}

char *
plural(long i)
{

	return (i == 1 ? catgets(catd, 1, 178, "")
			: catgets(catd, 1, 179, "s"));
}

short	vcntcol;

int 
qcolumn(register char *lim, register char *gp)
{
	register int x = 0, n = 1;
	int	c, i;
	int (*OO)(int);

	OO = Outchar;
	Outchar = qcount;
	vcntcol = 0;
	if (lim != NULL) {
		if (lim < linebuf) {
			lim = linebuf;
			n = 0;
		} else
			n = skipright(linebuf, lim);
		x = lim[n], lim[n] = 0;
	}
	pline(0, inopen ? WLINES*WCOLS : -1);
	if (lim != NULL)
		lim[n] = x;
	if (gp)
		while (*gp) {
			nextc(c, gp, i);
			putchar(c);
			gp += i;
		}
	Outchar = OO;
	return (vcntcol);
}

int 
qcount(int c)
{
	if (c == '\t') {
		vcntcol += value(TABSTOP) - vcntcol % value(TABSTOP);
		lastsc = 1;
		return c;
	}
	/*
	 * Take account of filler characters inserted at the end of
	 * the visual line if a multi-column character does not fit.
	 */
	lastsc = colsc(c&TRIM&~MULTICOL);
	while (vcntcol < WCOLS && vcntcol + lastsc - 1 >= WCOLS)
		vcntcol++;
	vcntcol += c & MULTICOL ? 1 : lastsc;
	return c;
}

void 
reverse(register line *a1, register line *a2)
{
	register line t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

void 
save(line *a1, register line *a2)
{
	register int more;

	if (!FIXUNDO)
		return;
#ifdef TRACE
	if (trace)
		vudump("before save");
#endif
	undkind = UNDNONE;
	undadot = dot;
	more = (a2 - a1 + 1) - (unddol - dol);
	while (more > (endcore - truedol))
		if (morelines() < 0)
			error(catgets(catd, 1, 180,
		"Out of memory@saving lines for undo - try using ed"));
	if (more)
		(*(more > 0 ? copywR : copyw))(unddol + more + 1, unddol + 1,
		    (truedol - unddol));
	unddol += more;
	truedol += more;
	copyw(dol + 1, a1, a2 - a1 + 1);
	undkind = UNDALL;
	unddel = a1 - 1;
	undap1 = a1;
	undap2 = a2 + 1;
#ifdef TRACE
	if (trace)
		vudump("after save");
#endif
}

void 
save12(void)
{

	save(addr1, addr2);
}

void 
saveall(void)
{

	save(one, dol);
}

int 
span(void)
{

	return (addr2 - addr1 + 1);
}

void 
synced(void)
{

	chng = 0;
	tchng = 0;
	xchng = 0;
}


int 
skipwh(void)
{
	register int wh;

	wh = 0;
	while (is_white(peekchar())) {
		wh++;
		ignchar();
	}
	return (wh);
}

void
vsmerror(char *seekpt, va_list ap)
{

	if (seekpt == 0)
		return;
	merror1((intptr_t)seekpt);
	if (inopen && CE)
		vclreol();
	if (SO && SE)
		putpad(SO);
	vlprintf(mesg(linebuf), ap);
	if (SO && SE)
		putpad(SE);
}

void
smerror(char *seekpt, ...)
{
	va_list ap;

	if (seekpt == NULL)
		return;
	va_start(ap, seekpt);
	vsmerror(seekpt, ap);
	va_end(ap);
}

char *
strend(register char *cp)
{

	while (*cp)
		cp++;
	return (cp);
}

void
strcLIN(char *dp)
{

	lcpy(linebuf, dp, LBSIZE);
}

void
syserror(void)
{

	dirtcnt = 0;
	putchar(' ');
	error("%s", strerror(errno));
}

/*
 * Return the column number that results from being in column col and
 * hitting a tab, where tabs are set every ts columns.  Work right for
 * the case where col > TCOLUMNS, even if ts does not divide TCOLUMNS.
 */
int
tabcol(int col, int ts)
{
	int offset, result;

	if (col >= TCOLUMNS) {
		offset = TCOLUMNS * (col/TCOLUMNS);
		col -= offset;
	} else
		offset = 0;
	result = col + ts - (col % ts) + offset;
	return (result);
}

char *
vfindcol(int i)
{
	register char *cp;
	register int (*OO)(int) = Outchar;
	int	c, n = 0;

	Outchar = qcount;
	ignore(qcolumn(linebuf - 1, NOSTR));
	for (cp = linebuf; *cp && vcntcol < i; cp += n) {
		nextc(c, cp, n);
		putchar(c);
	}
	if (cp != linebuf)
		cp -= n;
	Outchar = OO;
	return (cp);
}

char *
vskipwh(register char *cp)
{

	while (is_white(*cp) && cp[1])
		cp++;
	return (cp);
}


char *
vpastwh(register char *cp)
{

	while (is_white(*cp))
		cp++;
	return (cp);
}

int
whitecnt(register char *cp)
{
	register int i;

	i = 0;
	for (;;)
		switch (*cp++) {

		case '\t':
			i += value(TABSTOP) - i % value(TABSTOP);
			break;

		case ' ':
			i++;
			break;

		default:
			return (i);
		}
}

void
markit(line *addr)
{

	if (addr != dot && addr >= one && addr <= dol)
		markDOT();
}

#ifdef	SIGEMT
/*
 * The following code is defensive programming against a bug in the
 * pdp-11 overlay implementation.  Sometimes it goes nuts and asks
 * for an overlay with some garbage number, which generates an emt
 * trap.  This is a less than elegant solution, but it is somewhat
 * better than core dumping and losing your work, leaving your tty
 * in a weird state, etc.
 */
int _ovno;
void
onemt(int signum)
{
	/* int oovno; unused? */

	(void)signum;
	/* oovno = _ovno; unused? */
	/* 2 and 3 are valid on 11/40 type vi, so */
	if (_ovno < 0 || _ovno > 3)
		_ovno = 0;
	error(catgets(catd, 1, 181, "emt trap, _ovno is %d @ - try again"));
}
#endif

/*
 * When a hangup occurs our actions are similar to a preserve
 * command.  If the buffer has not been [Modified], then we do
 * nothing but remove the temporary files and exit.
 * Otherwise, we sync the temp file and then attempt a preserve.
 * If the preserve succeeds, we unlink our temp files.
 * If the preserve fails, we leave the temp files as they are
 * as they are a backup even without preservation if they
 * are not removed.
 */
void
onhup(int signum)
{
	(void)signum;
	/*
	 * USG tty driver can send multiple HUP's!!
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	if (chng == 0) {
		cleanup(1);
		exitex(0);
	}
	if (setexit() == 0) {
		if (preserve()) {
			cleanup(1);
			exitex(0);
		}
	}
	exitex(1);
}

/*
 * An interrupt occurred.  Drain any output which
 * is still in the output buffering pipeline.
 * Catch interrupts again.  Unless we are in visual
 * reset the output state (out of -nl mode, e.g).
 * Then like a normal error (with the \n before Interrupt
 * suppressed in visual mode).
 */
void
onintr(int signum)
{
	(void)signum;
	alarm(0);	/* in case we were called from map */
	draino();
	if (!inopen) {
		pstop();
		setlastchar('\n');
	}
	error(catgets(catd, 1, 182, "\nInterrupt") + inopen);
}

/*
 * If we are interruptible, enable interrupts again.
 * In some critical sections we turn interrupts off,
 * but not very often.
 */
void 
setrupt(void)
{

	if (ruptible) {
		signal(SIGINT, inopen ? vintr : onintr);
#ifdef SIGTSTP
		if (dosusp)
			signal(SIGTSTP, onsusp);
#endif
	}
}

int 
preserve(void)
{

#ifdef INCORB
	tflush();
#endif
	synctmp();
	pid = fork();
	if (pid < 0)
		return (0);
	if (pid == 0) {
		close(0);
		dup(tfile);
		execl(EXPRESERVE, "expreserve", (char *)0);
		exitex(1);
	}
	waitfor();
	if (rpid == pid && status == 0)
		return (1);
	return (0);
}

int 
exitex(int i)
{

# ifdef TRACE
	if (trace)
		fclose(trace);
# endif
	if (failed != 0 && i == 0)
		i = failed;
	_exit(i);
	/*NOTREACHED*/
	return 0;
}

#ifdef SIGTSTP
/*
 * We have just gotten a susp.  Suspend and prepare to resume.
 */
void 
onsusp(int signum)
{
	struct termios f;
	/* int omask; */
#ifdef	TIOCGWINSZ
	struct winsize win;
#endif
	sigset_t set;

	(void)signum;
	f = setty(normf);
	vnfl();
	putpad(TE);
	flush();

	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* the pc stops here */

	signal(SIGTSTP, onsusp);
	vcontin(0);
	setty(f);
	if (!inopen)
		error(0);
#ifdef	TIOCGWINSZ
	else {
		if (ioctl(0, TIOCGWINSZ, &win) >= 0)
			if (win.ws_row != winsz.ws_row ||
			    win.ws_col != winsz.ws_col)
				onwinch(SIGWINCH);
		if (vcnt < 0) {
			vcnt = -vcnt;
			if (state == VISUAL)
				vclear();
			else if (state == CRTOPEN)
				vcnt = 0;
		}
		vdirty(0, TLINES);
		vrepaint(cursor);
	}
#endif	/* TIOCGWINSZ */
}
#endif	/* SIGTSTP */

/*
 * strcpy() checking the maximum size of s1, printing msg in case of overflow.
 */
char *
safecp(char *s1, const char *s2, size_t max, char *msg, ...)
{
	va_list	ap;
	char	*cp = s1;

	while (max--)
		if ((*s1++ = *s2++) == '\0')
			return cp;
	va_start(ap, msg);
	verror(msg, ap);
	va_end(ap);
	exitex(0175);
	/*NOTREACHED*/
	return NULL;
}

/*
 * strcat() checking the maximum size of s1, printing msg in case of overflow.
 */
char *
safecat(char *s1, const char *s2, size_t max, char *msg, ...)
{
	va_list	ap;
	char	*cp = s1;

	while (max && *s1)
		max--, s1++;
	while (max--)
		if ((*s1++ = *s2++) == '\0')
			return cp;
	va_start(ap, msg);
	verror(msg, ap);
	va_end(ap);
	exitex(0175);
	/*NOTREACHED*/
	return NULL;
}

/*
 * Grow the line and generic buffers.
 */
void
grow(char *msg, char **tolb0, char **tolb1, char **togb0, char **togb1)
{
	char *nlb, *ngb = NULL;

	if ((nlb = realloc(linebuf, LBSIZE + 4096)) == NULL ||
			(ngb = realloc(genbuf, 2 * (LBSIZE + 4096))) == NULL) {
		synced();
		error(msg);
	}
	if (tolb0)
		*tolb0 += nlb - linebuf;
	if (tolb1)
		*tolb1 += nlb - linebuf;
	if (togb0)
		*togb0 += ngb - genbuf;
	if (togb1)
		*togb1 += ngb - genbuf;
	linebuf = nlb;
	genbuf = ngb;
	LBSIZE += 4096;
}

void *
smalloc(size_t size)
{
	void	*vp;

	if ((vp = malloc(size)) == NULL)
		error("no space");
	return vp;
}
