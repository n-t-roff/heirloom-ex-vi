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
static char sccsid[] = "@(#)ex_io.c	1.42 (gritter) 8/4/05";
#endif
#endif

/* from ex_io.c	7.11.1.1 (Berkeley) 8/12/86 */

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * File input/output, source, preserve and recover
 */

/*
 * Following remember where . was in the previous file for return
 * on file switching.
 */
int	altdot;
int	oldadot;
bool	wasalt;
short	isalt;

long	cntch;			/* Count of characters on unit io */
long	cntln;			/* Count of lines " */
long	cntnull;		/* Count of nulls " */
#ifndef	BIT8
long	cntodd;			/* Count of non-ascii characters " */
#endif

/*
 * Parse file name for command encoded by comm.
 * If comm is E then command is doomed and we are
 * parsing just so user won't have to retype the name.
 */
void 
filename(int comm)
{
	register int c = comm, d;
	register int i;

	d = getchar();
	if (endcmd(d)) {
		if (savedfile[0] == 0 && comm != 'f')
			error(catgets(catd, 1, 72,
					"No file|No current filename"));
		lcpy(file, savedfile, sizeof file);
		wasalt = (isalt > 0) ? isalt-1 : 0;
		isalt = 0;
		oldadot = altdot;
		if (c == 'e' || c == 'E')
			altdot = lineDOT();
		if (d == EOF)
			ungetchar(d);
	} else {
		ungetchar(d);
		getone();
		eol();
		if (savedfile[0] == 0 && c != 'E' && c != 'e') {
			c = 'e';
			edited = 0;
		}
		wasalt = strcmp(file, altfile) == 0;
		oldadot = altdot;
		switch (c) {

		case 'f':
			edited = 0;
			/* fall into ... */

		case 'e':
			if (savedfile[0]) {
				altdot = lineDOT();
				lcpy(altfile, savedfile, sizeof altfile);
			}
			lcpy(savedfile, file, sizeof savedfile);
			break;

		default:
			if (file[0]) {
				if (c != 'E')
					altdot = lineDOT();
				lcpy(altfile, file, sizeof altfile);
			}
			break;
		}
	}
	if ((hush && comm != 'f') || comm == 'E')
		return;
	if (file[0] != 0) {
		lprintf("\"%s\"", file);
		if (comm == 'f') {
			if (value(READONLY))
				ex_printf(catgets(catd, 1, 73, " [Read only]"));
			if (!edited)
				ex_printf(catgets(catd, 1, 74, " [Not edited]"));
			if (tchng)
				ex_printf(catgets(catd, 1, 75, " [Modified]"));
		}
		flush();
	} else
		ex_printf(catgets(catd, 1, 76, "No file "));
	if (comm == 'f') {
		if (!(i = lineDOL()))
			i++;
		ex_printf(catgets(catd, 1, 77,
			" line %d of %d --%ld%%--"), lineDOT(), lineDOL(),
		    (long) 100 * lineDOT() / i);
	}
}

/*
 * Get the argument words for a command into genbuf
 * expanding # and %.
 */
int 
getargs(void)
{
	register int c;
	register char *cp, *fp;
	static char fpatbuf[32];	/* hence limit on :next +/pat */

	pastwh();
	if (peekchar() == '+') {
		for (cp = fpatbuf;;) {
			c = *cp++ = getchar();
			if (cp >= &fpatbuf[sizeof(fpatbuf)])
				error(catgets(catd, 1, 78, "Pattern too long"));
			if (c == '\\' && isspace(peekchar()))
				c = getchar();
			if (c == EOF || isspace(c)) {
				ungetchar(c);
				*--cp = 0;
				firstpat = &fpatbuf[1];
				break;
			}
		}
	}
	if (skipend())
		return (0);
	CP(genbuf, "echo "); cp = &genbuf[5];
	for (;;) {
		c = getchar();
		if (endcmd(c)) {
			ungetchar(c);
			break;
		}
		switch (c) {

		case '\\':
			if (any(peekchar(), "#%|"))
				c = getchar();
			/* fall into... */

		default:
			if (cp > &genbuf[LBSIZE - 2])
flong:
				error(catgets(catd, 1, 79,
						"Argument buffer overflow"));
			*cp++ = c;
			break;

		case '#':
			fp = altfile;
			if (*fp == 0)
				error(catgets(catd, 1, 80,
				"No alternate filename@to substitute for #"));
			goto filexp;

		case '%':
			fp = savedfile;
			if (*fp == 0)
				error(catgets(catd, 1, 81,
				"No current filename@to substitute for %%"));
filexp:
			while (*fp) {
				if (cp > &genbuf[LBSIZE - 2])
					goto flong;
				*cp++ = *fp++;
			}
			break;
		}
	}
	*cp = 0;
	return (1);
}

/*
 * Scan genbuf for shell metacharacters.
 * Set is union of v7 shell and csh metas.
 */
int 
gscan(void)
{
	register char *cp;

	for (cp = genbuf; *cp; cp++)
		if (any(*cp, "~{[*?$`'\"\\"))
			return (1);
	return (0);
}

/*
 * Glob the argument words in genbuf, or if no globbing
 * is implied, just split them up directly.
 */
void 
gglob(struct glob *gp)
{
	int pvec[2];
	register char **argv = gp->argv;
	register char *cp = gp->argspac;
	register int c;
	char ch;
	int nleft = NCARGS;

	gp->argc0 = 0;
	if (gscan() == 0) {
		register char *v = genbuf + 5;		/* strlen("echo ") */

		for (;;) {
			while (isspace(*v&0377))
				v++;
			if (!*v)
				break;
			*argv++ = cp;
			while (*v && !isspace(*v&0377))
				*cp++ = *v++;
			*cp++ = 0;
			gp->argc0++;
		}
		*argv = 0;
		return;
	}
	if (pipe(pvec) < 0)
		error(catgets(catd, 1, 82, "Can't make pipe to glob"));
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error(catgets(catd, 1, 83, "Can't fork to do glob"));
	}
	if (pid == 0) {
		int oerrno;

		close(1);
		dup(pvec[1]);
		close(pvec[0]);
		close(2);	/* so errors don't mess up the screen */
		open("/dev/null", O_WRONLY);
		execl(svalue(SHELL), "sh", "-c", genbuf, (char *)0);
		oerrno = errno; close(1); dup(2); errno = oerrno;
		filioerr(svalue(SHELL));
	}
	close(pvec[1]);
	do {
		*argv = cp;
		for (;;) {
			if (read(io, &ch, 1) != 1) {
				close(io);
				c = -1;
			} else
				c = ch & TRIM;
			if (c <= 0 || isspace(c))
				break;
			*cp++ = c;
			if (--nleft <= 0)
				error(catgets(catd, 1, 84,
							"Arg list too long"));
		}
		if (cp != *argv) {
			--nleft;
			*cp++ = 0;
			gp->argc0++;
			if (gp->argc0 >= NARGS)
				error(catgets(catd, 1, 85,
							"Arg list too long"));
			argv++;
		}
	} while (c >= 0);
	waitfor();
	if (gp->argc0 == 0)
		error(catgets(catd, 1, 86, "No match"));
}

/*
 * Parse one filename into file.
 */
struct glob G;
void
getone(void)
{
	register char *str;

	if (getargs() == 0)
missing:
		error(catgets(catd, 1, 87, "Missing filename"));
	gglob(&G);
	if (G.argc0 > 1)
		error(catgets(catd, 1, 88, "Ambiguous|Too many file names"));
	if ((str = G.argv[G.argc0 - 1]) == NULL)
		goto missing;
	if (strlen(str) > FNSIZE - 4)
		error(catgets(catd, 1, 89, "Filename too long"));
/* samef: */
	lcpy(file, str, sizeof file);
}

/*
 * Are these two really the same inode?
 */
int 
samei(struct stat *sp, char *cp)
{
	struct stat stb;

	if (stat(cp, &stb) < 0 || sp->st_dev != stb.st_dev)
		return (0);
	return (sp->st_ino == stb.st_ino);
}

/*
 * Read a file from the world.
 * C is command, 'e' if this really an edit (or a recover).
 */
void 
rop(int c)
{
	struct stat stbuf;
	static int ovro;	/* old value(READONLY) */
	static int denied;	/* 1 if READONLY was set due to file permissions */

	io = open(file, O_RDONLY);
	if (io < 0) {
		if (c == 'e' && errno == ENOENT) {
			edited++;
			/*
			 * If the user just did "ex foo" he is probably
			 * creating a new file.  Don't be an error, since
			 * this is ugly, and it screws up the + option.
			 *
			 * POSIX.2 specifies that this be done for all
			 * "edit" commands, not just for the first one.
			 */
			if (1 || !seenprompt) {
				ex_printf(catgets(catd, 1, 90, " [New file]"));
				noonl();
				return;
			}
		}
		failed = 1;
		syserror();
	}
	if (fstat(io, &stbuf))
		syserror();
	switch (stbuf.st_mode & S_IFMT) {

	case S_IFBLK:
		error(catgets(catd, 1, 91, " Block special file"));

	case S_IFCHR:
		if (isatty(io))
			error(catgets(catd, 1, 92, " Teletype"));
		if (samei(&stbuf, "/dev/null"))
			break;
		error(catgets(catd, 1, 93, " Character special file"));

	case S_IFDIR:
		error(catgets(catd, 1, 94, " Directory"));

#ifdef	S_IFSOCK
	case S_IFSOCK:
		error(catgets(catd, 1, 95, " Socket"));
#endif
#ifdef	S_IFIFO
	case S_IFIFO:
		error(catgets(catd, 1, 96, " Named pipe"));
#endif
	}
	if (c != 'r') {
		if (value(READONLY) && denied) {
			value(READONLY) = ovro;
			denied = 0;
		}
		if ((stbuf.st_mode & 0222) == 0 || access(file, W_OK) < 0) {
			ovro = value(READONLY);
			denied = 1;
			value(READONLY) = 1;
		}
	}
	if (value(READONLY) && !hush) {
		ex_printf(catgets(catd, 1, 102, " [Read only]"));
		flush();
	}
	if (c == 'r')
		setdot();
	else
		setall();
	if (FIXUNDO && inopen && c == 'r')
		undap1 = undap2 = addr1 + 1;
	rop2();
	rop3(c);
}

void 
rop2(void)
{
	line *first, *last, *a;
	struct stat statb;

	deletenone();
	clrstats();
	first = addr2 + 1;
	if (fstat(io, &statb) < 0 || statb.st_blksize > LBSIZE)
		bsize = LBSIZE;
	else {
		bsize = statb.st_blksize;
		if (bsize <= 0)
			bsize = LBSIZE;
	}
	ignore(append(getfile, addr2));
	last = dot;
	/*
	 *	if the modelines variable is set,
	 *	check the first and last five lines of the file
	 *	for a mode line.
	 */
	if (value(MODELINES)) {
		for (a=first; a<=last; a++) {
			if (a==first+5 && last-first > 10)
				a = last - 4;
			getline(*a);
			checkmodeline(linebuf);
		}
	}
}

/*
 * Io is finished, close the unit and print statistics.
 */
int 
iostats(void)
{

	fsync(io);
	close(io);
	io = -1;
	if (hush == 0) {
		if (value(TERSE))
			ex_printf(catgets(catd, 1, 103,
						" %ld/%ld"), cntln, cntch);
		else
			ex_printf(catgets(catd, 1, 104,
		" %ld line%s, %ld character%s"), cntln, plural(cntln),
			    cntch, plural(cntch));
		if (cntnull
#ifndef	BIT8
				|| cntodd
#endif
				) {
			ex_printf(catgets(catd, 1, 105, " ("));
			if (cntnull) {
				ex_printf(catgets(catd, 1, 106,
						"%d null"), (int)cntnull);
#ifndef	BIT8
				if (cntodd)
					ex_printf(catgets(catd, 1, 107, ", "));
#endif
			}
#ifndef	BIT8
			if (cntodd)
				ex_printf(catgets(catd, 1, 108,
					"%d non-ASCII"), (int)cntodd);
#endif
			putchar(')');
		}
		noonl();
		flush();
	}
	return (cntnull != 0
#ifndef	BIT8
			|| cntodd != 0
#endif
			);
}

void
rop3(int c)
{

	if (iostats() == 0 && c == 'e')
		edited++;
	if (c == 'e') {
		if (wasalt || firstpat) {
			register line *addr = zero + oldadot;

			if (addr > dol)
				addr = dol;
			if (firstpat) {
				globp = (*firstpat) ? firstpat : "$";
				commands(1,1);
				firstpat = 0;
			} else if (addr >= one) {
				if (inopen)
					dot = addr;
				markpr(addr);
			} else
				goto other;
		} else
other:
			if (dol > zero) {
				if (inopen)
					dot = one;
				markpr(one);
			}
		if(FIXUNDO)
			undkind = UNDNONE;
		if (inopen) {
			vcline = 0;
			vreplace(0, TLINES, lineDOL());
		}
	}
}

/* Returns from edited() */
#define	EDF	0		/* Edited file */
#define	NOTEDF	-1		/* Not edited file */
#define	PARTBUF	1		/* Write of partial buffer to Edited file */

/*
 * Is file the edited file?
 * Work here is that it is not considered edited
 * if this is a partial buffer, and distinguish
 * all cases.
 */
int 
edfile(void)
{

	if (!edited || !eq(file, savedfile))
		return (NOTEDF);
	return (addr1 == one && addr2 == dol ? EDF : PARTBUF);
}

/*
 * Write a file.
 */
void
wop(bool dofname)
/*bool dofname;	/\* if 1 call filename, else use savedfile */
{
	register int c, exclam, nonexist;
	line *saddr1 = NULL, *saddr2 = NULL;
	struct stat stbuf;

	c = 0;
	exclam = 0;
	if (dofname) {
		if (peekchar() == '!')
			exclam++, ignchar();
		ignore(skipwh());
		while (peekchar() == '>')
			ignchar(), c++, ignore(skipwh());
		if (c != 0 && c != 2)
			error(catgets(catd, 1, 109,
					"Write forms are 'w' and 'w>>'"));
		filename('w');
	} else {
		if (savedfile[0] == 0)
			error(catgets(catd, 1, 110,
					"No file|No current filename"));
		saddr1=addr1;
		saddr2=addr2;
		addr1=one;
		addr2=dol;
		lcpy(file, savedfile, sizeof file);
		if (inopen) {
			vclrech(0);
			splitw++;
		}
		lprintf("\"%s\"", file);
	}
	nonexist = stat(file, &stbuf);
	switch (c) {

	case 0:
		if (!exclam && (!value(WRITEANY) || value(READONLY)))
		switch (edfile()) {
		
		case NOTEDF:
			if (nonexist)
				break;
			if ((stbuf.st_mode & S_IFMT) == S_IFCHR) {
				if (samei(&stbuf, "/dev/null"))
					break;
				if (samei(&stbuf, "/dev/tty"))
					break;
			}
			io = open(file, O_WRONLY);
			if (io < 0)
				syserror();
			if (!isatty(io))
				serror(catgets(catd, 1, 111,
	" File exists| File exists - use \"w! %s\" to overwrite"), file);
			close(io);
			break;

		case EDF:
			if (value(READONLY))
				error(catgets(catd, 1, 112,
						" File is read only"));
			break;

		case PARTBUF:
			if (value(READONLY))
				error(catgets(catd, 1, 113,
						" File is read only"));
			error(catgets(catd, 1, 114,
				" Use \"w!\" to write partial buffer"));
		}
cre:
/*
		synctmp();
*/
		io = creat(file, 0666);
		if (io < 0)
			syserror();
		writing = 1;
		if (hush == 0) {
			if (nonexist)
				ex_printf(catgets(catd, 1, 115, " [New file]"));
			else if (value(WRITEANY) && edfile() != EDF)
				ex_printf(catgets(catd, 1, 116,
							" [Existing file]"));
		}
		break;

	case 2:
		io = open(file, O_WRONLY);
		if (io < 0) {
			if (exclam || value(WRITEANY))
				goto cre;
			syserror();
		}
		lseek(io, (off_t) 0, SEEK_END);
		break;
	}
	putfile(0);
	ignore(iostats());
	if (c != 2 && addr1 == one && addr2 == dol) {
		if (eq(file, savedfile))
			edited = 1;
		synced();
	}
	if (!dofname) {
		addr1 = saddr1;
		addr2 = saddr2;
	}
	writing = 0;
}

/*
 * Extract the next line from the io stream.
 */
char *nextip;

int
getfile(void)
{
	register short c;
	char *lp, *fp;

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			ninbuf = read(io, genbuf, bsize) - 1;
			if (ninbuf < 0) {
				if (lp != linebuf) {
					lp++;
					ex_printf(catgets(catd, 1, 117,
						" [Incomplete last line]"));
					break;
				}
				return (EOF);
			}
			fp = genbuf;
			cntch += ninbuf+1;
		}
		if (lp >= &linebuf[LBSIZE])
			grow(" Line too long", &lp, NULL, &fp, &nextip);
		c = *fp++;
		if (c == 0) {
			cntnull++;
#ifndef	BIT8
			continue;
#else
			c = 0200;
#endif
		}
#ifndef	BIT8
		if (c & QUOTE) {
			cntodd++;
			c &= TRIM;
			if (c == 0)
				continue;
		}
#endif
		*lp++ = c;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	cntln++;
	return (0);
}

/*
 * Write a range onto the io stream.
 */
void
putfile(int isfilter)
{
	line *a1;
	register char *fp, *lp;
	register int nib;
	struct stat statb;

	(void)isfilter;
	a1 = addr1;
	clrstats();
	cntln = fixedzero ? 0 : addr2 - a1 + 1;
	if (cntln == 0 || fixedzero)
		return;
	if (fstat(io, &statb) < 0 || statb.st_blksize > LBSIZE)
		bsize = LBSIZE;
	else {
		bsize = statb.st_blksize;
		if (bsize <= 0)
			bsize = LBSIZE;
	}
	nib = bsize;
	fp = genbuf;
	do {
		getline(*a1++);
		lp = linebuf;
		for (;;) {
			if (--nib < 0) {
				nib = fp - genbuf;
				if (write(io, genbuf, nib) != nib) {
					wrerror();
				}
				cntch += nib;
				nib = bsize - 1;
				fp = genbuf;
			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	nib = fp - genbuf;
	if (write(io, genbuf, nib) != nib) {
		wrerror();
	}
	cntch += nib;
}

/*
 * A write error has occurred;  if the file being written was
 * the edited file then we consider it to have changed since it is
 * now likely scrambled.
 */
void
wrerror(void)
{

	if (eq(file, savedfile) && edited)
		change();
	syserror();
}

/*
 * Source command, handles nested sources.
 * Traps errors since it mungs unit 0 during the source.
 */
short slevel;
short ttyindes;

void
source(char *fil, bool okfail)
{
	JMP_BUF osetexit;
	register int saveinp, ointty, oerrno;
	char *saveglobp, *saveinput;
	char	saveinline[BUFSIZ];
	int savepeekc, savelastc;

	signal(SIGINT, SIG_IGN);
	saveinp = dup(0);
	savepeekc = peekc;
	savelastc = lastc;
	saveglobp = globp;
	saveinput = input;
	if (input)
		lcpy(saveinline, input, sizeof saveinline);
	peekc = 0; lastc = 0; globp = 0; input = 0;
	if (saveinp < 0)
		error(catgets(catd, 1, 119, "Too many nested sources"));
	if (slevel <= 0)
		ttyindes = saveinp;
	close(0);
	if (open(fil, O_RDONLY) < 0) {
		oerrno = errno;
		setrupt();
		dup(saveinp);
		close(saveinp);
		input = saveinput;
		if (input)
			strcpy(input, saveinline);
		lastc = savelastc;
		errno = oerrno;
		if (!okfail)
			filioerr(fil);
		return;
	}
	slevel++;
	ointty = intty;
	intty = isatty(0);
	oprompt = value(PROMPT);
	value(PROMPT) &= intty;
	getexit(osetexit);
	setrupt();
	if (setexit() == 0)
		commands(1, 1);
	else if (slevel > 1) {
		close(0);
		dup(saveinp);
		close(saveinp);
		input = saveinput;
		if (input)
			strcpy(input, saveinline);
		lastc = savelastc;
		slevel--;
		resexit(osetexit);
		reset();
	}
	intty = ointty;
	value(PROMPT) = oprompt;
	close(0);
	dup(saveinp);
	close(saveinp);
	globp = saveglobp;
	input = saveinput;
	if (input)
		strcpy(input, saveinline);
	peekc = savepeekc;
	lastc = savelastc;
	slevel--;
	resexit(osetexit);
}

/*
 * Clear io statistics before a read or write.
 */
void
clrstats(void)
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;
#ifndef	BIT8
	cntodd = 0;
#endif
}

/* It's so wonderful how we all speak the same language... */
# define index strchr
# define rindex strrchr

void
checkmodeline(char *lin)
{
	char *beg, *end;
	char cmdbuf[BUFSIZ];

	beg = index(lin, ':');
	if (beg == NULL)
		return;
	if (&beg[-2] < lin)
		return;
	if (!((beg[-2] == 'e' && beg[-1] == 'x')
	     || (beg[-2] == 'v' && beg[-1] == 'i'))) return;
	strncpy(cmdbuf, beg+1, sizeof cmdbuf);
	end = rindex(cmdbuf, ':');
	if (end == NULL)
		return;
	*end = 0;
	globp = cmdbuf;
	commands(1, 1);
}

#ifdef	MB
int
mbtowi(int *cp, const char *s, size_t n)
{
	wchar_t	wc;
	int	i;

	i = mbtowc(&wc, s, n);
	if (i >= 0 && widthok(wc) && !(wc & 0x70000000))
		*cp = wc;
	else {
		*cp = (*s & 0377) | INVBIT;
		i = 1;
	}
	return i;
}

int
widthok(int c)
{
	return inopen ? wcwidth(c) <= 2 : 1;
}
#endif	/* MB */

int
GETWC(char *mb)
{
	int	c, n;

	n = 1;
	mb[0] = c = getchar();
	mb[1] = '\0';
#ifdef	MB
	if (mb_cur_max > 1 && c & 0200 && c != EOF) {
		int	m;
		wchar_t	wc;
		while ((m = mbtowc(&wc, mb, mb_cur_max)) < 0 && n<mb_cur_max) {
			mb[n++] = c = getchar();
			mb[n] = '\0';
			if (c == '\n' || c == EOF)
				break;
		}
		if (m != n || c & 0x70000000)
			error("illegal multibyte sequence");
		return wc;
	} else
#endif	/* MB */
		return c;
}
