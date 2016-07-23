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

#ifdef	__GNUC__
#define	UNUSED	__attribute__ ((unused))
#else
#define	UNUSED
#endif

#ifndef	lint
#ifdef	DOSCCS
char *copyright =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif
static char sccsid[] UNUSED = "@(#)exrecover.c	1.23 (gritter) 12/25/06";
#endif

/* from exrecover.c	7.9.2 (2.11BSD) 1996/10/26 */

#include <stdarg.h>
#ifdef	notdef	/* GR */
#include <stdio.h>	/* mjm: BUFSIZ: stdio = 512, VMUNIX = 1024 */
#undef	BUFSIZ		/* mjm: BUFSIZ different */
#undef	EOF		/* mjm: EOF and NULL effectively the same */
#undef	NULL
#else
#define	xstderr	(int*)0
typedef	int	xFILE;
extern void	perror(const char *);
#include <sys/types.h>
extern int	vsnprintf(char *, size_t, const char *, va_list);
#endif

#define	var

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include <dirent.h>
#include <time.h>
#if !defined(HAVE_STRLCPY) || !defined(HAVE_STRLCAT)
# include "compat.h"
#endif

#ifndef	MAXNAMLEN
#ifdef	FNSIZE
#define	MAXNAMLEN	FNSIZE
#else
#ifdef	NAME_MAX
#define	MAXNAMLEN	NAME_MAX
#else
#define	MAXNAMLEN	255
#endif
#endif
#endif

#define	TMP		"/var/tmp"

#ifdef	LANGMSG
nl_catd	catd;
#endif

char xstr[1];		/* make loader happy */
int tfile = -1;	/* ditto */

/*
 *
 * This program searches through the specified directory and then
 * the directory /usr/preserve looking for an instance of the specified
 * file from a crashed editor or a crashed system.
 * If this file is found, it is unscrambled and written to
 * the standard output.
 *
 * If this program terminates without a "broken pipe" diagnostic
 * (i.e. the editor doesn't die right away) then the buffer we are
 * writing from is removed when we finish.  This is potentially a mistake
 * as there is not enough handshaking to guarantee that the file has actually
 * been recovered, but should suffice for most cases.
 */

/*
 * Here we save the information about files, when
 * you ask us what files we have saved for you.
 * We buffer file name, number of lines, and the time
 * at which the file was saved.
 */
struct svfile {
	char	sf_name[FNSIZE + 1];
	int	sf_lines;
	char	sf_entry[MAXNAMLEN + 1];
	time_t	sf_time;
};

#define	ignorl(a)	a

/*
 * This directory definition also appears (obviously) in expreserve.c.
 * Change both if you change either.
 */
#ifdef	notdef
char	mydir[] =	"/usr/preserve";
#else
char	mydir[] =	PRESERVEDIR;
#endif

/*
 * Limit on the number of printed entries
 * when an, e.g. ``ex -r'' command is given.
 */
#define	NENTRY	50

char	nb[BUFSIZ];
int	vercnt;			/* Count number of versions of file found */

extern void error(char *, ...);
extern void listfiles(char *);
extern void enter(struct svfile *, char *, int);
extern int qucmp(struct svfile *, struct svfile *);
extern void findtmp(char *);
extern void searchdir(char *);
extern int yeah(char *);
extern int preserve(void);
extern void scrapbad(void);
extern void putfile(int);
extern void wrerror(void);
extern void clrstats(void);
extern void getline(line);
extern char *getblock(line, int);
extern void blkio(bloc, char *, ssize_t (*)(int, void *, size_t));
extern void syserror(void);
extern void xvfprintf(xFILE *, char *, va_list);
extern void xfprintf(xFILE *, char *, ...);

int 
main(int argc, char *argv[])
{
	register char *cp;
	register int b, i;

	/*
	 * Initialize the built-in memory allocator.
	 */
#ifdef	VMUNIX
	poolsbrk(0);
#endif
	linebuf = calloc(LBSIZE = BUFSIZ<4096?4096:BUFSIZ, sizeof *linebuf);
	genbuf = calloc(MAXBSIZE, sizeof *genbuf);
#ifdef	LANGMSG
	setlocale(LC_MESSAGES, "");
	catd = catopen(CATNAME, NL_CAT_LOCALE);
#endif

	/*
	 * Initialize as though the editor had just started.
	 */
	fendcore = (line *) sbrk(0);
	dot = zero = dol = fendcore;
	one = zero + 1;
	endcore = fendcore - 2;
	iblock = oblock = -1;

	/*
	 * If given only a -r argument, then list the saved files.
	 */
	if (argc == 2 && strcmp(argv[1], "-r") == 0) {
		listfiles(mydir);
		listfiles(TMP);
		exit(0);
	}
	if (argc != 3)
		error(catgets(catd, 2, 1,
			" Wrong number of arguments to exrecover"), 0);

	strlcpy(file, argv[2], sizeof file);

	/*
	 * Search for this file.
	 */
	findtmp(argv[1]);

	/*
	 * Got (one of the versions of) it, write it back to the editor.
	 */
	cp = ctime(&H.Time);
	cp[19] = 0;
	xfprintf(xstderr, catgets(catd, 2, 2, " [Dated: %s"), cp);
	xfprintf(xstderr, vercnt > 1 ? catgets(catd, 2, 3,
			", newest of %d saved]")
		: catgets(catd, 2, 4, "]"), vercnt);
	H.Flines++;

	/*
	 * Allocate space for the line pointers from the temp file.
	 */
	if ((char *) sbrk(H.Flines * sizeof (line)) == (char *) -1)
		/*
		 * Good grief.
		 */
		error(catgets(catd, 1, 5, " Not enough core for lines"), 0);
#ifdef DEBUG
	xfprintf(xstderr, "%d lines\n", H.Flines);
#endif

	/*
	 * Now go get the blocks of seek pointers which are scattered
	 * throughout the temp file, reconstructing the incore
	 * line pointers at point of crash.
	 */
	b = 0;
	while (H.Flines > 0) {
		ignorl(lseek(tfile, (off_t) ((blocks[b] & BLKMSK) * BUFSIZ),
					SEEK_SET));
		i = H.Flines < (ssize_t)(BUFSIZ / sizeof (line)) ?
			H.Flines * sizeof (line) : BUFSIZ;
		if (read(tfile, (char *) dot, i) != i) {
			perror(nb);
			exit(1);
		}
		dot += i / sizeof (line);
		H.Flines -= i / sizeof (line);
		b++;
	}
	dot--; dol = dot;

	/*
	 * Sigh... due to sandbagging some lines may really not be there.
	 * Find and discard such.  This shouldn't happen much.
	 */
	scrapbad();

	/*
	 * Now if there were any lines in the recovered file
	 * write them to the standard output.
	 */
	if (dol > zero) {
		addr1 = one; addr2 = dol; io = 1;
		putfile(0);
	}

	/*
	 * Trash the saved buffer.
	 * Hopefully the system won't crash before the editor
	 * syncs the new recovered buffer; i.e. for an instant here
	 * you may lose if the system crashes because this file
	 * is gone, but the editor hasn't completed reading the recovered
	 * file from the pipe from us to it.
	 *
	 * This doesn't work if we are coming from an non-absolute path
	 * name since we may have chdir'ed but what the hay, noone really
	 * ever edits with temporaries in "." anyways.
	 */
	if (nb[0] == '/')
		ignore(unlink(nb));

	/*
	 * Adieu.
	 */
	exit(0);
}

/*
 * Print an error message (notably not in error
 * message file).  If terminal is in RAW mode, then
 * we should be writing output for "vi", so don't print
 * a newline which would screw up the screen.
 */
/*VARARGS2*/
void
error(char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	xvfprintf(xstderr, str, ap);
	va_end(ap);
	tcgetattr(2, &tty);
	if (tty.c_lflag & ICANON)
		xfprintf(xstderr, "\n");
	exit(1);
}

void
listfiles(char *dirname)
{
	register DIR *dir;
	struct dirent *dirent;
	int ecount;
	register int f;
	char *cp;
	struct svfile *fp, svbuf[NENTRY];

	/*
	 * Open /usr/preserve, and go there to make things quick.
	 */
	dir = opendir(dirname);
	if (dir == NULL) {
		perror(dirname);
		return;
	}
	if (chdir(dirname) < 0) {
		perror(dirname);
		return;
	}
	xfprintf(xstderr, "%s:\n", dirname);

	/*
	 * Look at the candidate files in /usr/preserve.
	 */
	fp = &svbuf[0];
	ecount = 0;
	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] != 'E')
			continue;
#ifdef DEBUG
		xfprintf(xstderr, "considering %s\n", dirent->d_name);
#endif
		/*
		 * Name begins with E; open it and
		 * make sure the uid in the header is our uid.
		 * If not, then don't bother with this file, it can't
		 * be ours.
		 */
		f = open(dirent->d_name, O_RDONLY);
		if (f < 0) {
#ifdef DEBUG
			xfprintf(xstderr, "open failed\n");
#endif
			continue;
		}
		if (read(f, (char *) &H, sizeof H) != sizeof H) {
#ifdef DEBUG
			xfprintf(xstderr, "culdnt read hedr\n");
#endif
			ignore(close(f));
			continue;
		}
		ignore(close(f));
		if (getuid() != H.Uid) {
#ifdef DEBUG
			xfprintf(xstderr, "uid wrong\n");
#endif
			continue;
		}

		/*
		 * Saved the day!
		 */
		enter(fp++, dirent->d_name, ecount);
		ecount++;
#ifdef DEBUG
		xfprintf(xstderr, "entered file %s\n", dirent->d_name);
#endif
	}
	ignore(closedir(dir));

	/*
	 * If any files were saved, then sort them and print
	 * them out.
	 */
	if (ecount == 0) {
		xfprintf(xstderr, catgets(catd, 2, 6, "No files saved.\n"));
		return;
	}
	qsort(&svbuf[0], ecount, sizeof svbuf[0], (int(*)(const void *, const void *)) qucmp);
	for (fp = &svbuf[0]; fp < &svbuf[ecount]; fp++) {
		cp = ctime(&fp->sf_time);
		cp[10] = 0;
		xfprintf(xstderr, catgets(catd, 2, 7, "On %s at "), cp);
 		cp[16] = 0;
		xfprintf(xstderr, &cp[11]);
		xfprintf(xstderr, catgets(catd, 2, 8,
					" saved %d lines of file \"%s\"\n"),
		    fp->sf_lines, fp->sf_name);
	}
}

/*
 * Enter a new file into the saved file information.
 */
void
enter(struct svfile *fp, char *fname, int count)
{
	register char *cp, *cp2;
	register struct svfile *f, *fl;
	time_t curtime;

	f = 0;
	if (count >= NENTRY) {
		/*
		 * My god, a huge number of saved files.
		 * Would you work on a system that crashed this
		 * often?  Hope not.  So lets trash the oldest
		 * as the most useless.
		 *
		 * (I wonder if this code has ever run?)
		 */
		fl = fp - count + NENTRY - 1;
		curtime = fl->sf_time;
		for (f = fl; --f > fp-count; )
			if (f->sf_time < curtime)
				curtime = f->sf_time;
		for (f = fl; --f > fp-count; )
			if (f->sf_time == curtime)
				break;
		fp = f;
	}

	/*
	 * Gotcha.
	 */
	fp->sf_time = H.Time;
	fp->sf_lines = H.Flines;
	cp2 = fp->sf_name, cp = savedfile;
	while ((*cp2++ = *cp++));
	for (cp2 = fp->sf_entry, cp = fname; *cp && cp-fname < 14;)
		*cp2++ = *cp++;
	*cp2++ = 0;
}

/*
 * Do the qsort compare to sort the entries first by file name,
 * then by modify time.
 */
int
qucmp(struct svfile *p1, struct svfile *p2)
{
	register int t;

	if ((t = strcmp(p1->sf_name, p2->sf_name)))
		return(t);
	if (p1->sf_time > p2->sf_time)
		return(-1);
	return(p1->sf_time < p2->sf_time);
}

/*
 * Scratch for search.
 */
char	bestnb[BUFSIZ];		/* Name of the best one */
long	besttime;		/* Time at which the best file was saved */
int	bestfd;			/* Keep best file open so it dont vanish */

/*
 * Look for a file, both in the users directory option value
 * (i.e. usually /tmp) and in /usr/preserve.
 * Want to find the newest so we search on and on.
 */
void
findtmp(char *dir)
{

	/*
	 * No name or file so far.
	 */
	bestnb[0] = 0;
	bestfd = -1;

	/*
	 * Search /usr/preserve and, if we can get there, /tmp
	 * (actually the users "directory" option).
	 */
	searchdir(dir);
	if (chdir(mydir) == 0)
		searchdir(mydir);
	if (bestfd != -1) {
		/*
		 * Gotcha.
		 * Put the file (which is already open) in the file
		 * used by the temp file routines, and save its
		 * name for later unlinking.
		 */
		tfile = bestfd;
		strlcpy(nb, bestnb, sizeof nb);
		ignorl(lseek(tfile, (off_t) 0, SEEK_SET));

		/*
		 * Gotta be able to read the header or fall through
		 * to lossage.
		 */
		if (read(tfile, (char *) &H, sizeof H) == sizeof H)
			return;
	}

	/*
	 * Extreme lossage...
	 */
	error(catgets(catd, 2, 9, " File not found"), 0);
}

/*
 * Search for the file in directory dirname.
 *
 * Don't chdir here, because the users directory
 * may be ".", and we would move away before we searched it.
 * Note that we actually chdir elsewhere (because it is too slow
 * to look around in /usr/preserve without chdir'ing there) so we
 * can't win, because we don't know the name of '.' and if the path
 * name of the file we want to unlink is relative, rather than absolute
 * we won't be able to find it again.
 */
void
searchdir(char *dirname)
{
	struct dirent *dirent;
	register DIR *dir;
	/* char dbuf[BUFSIZ]; */

	dir = opendir(dirname);
	if (dir == NULL)
		return;
	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] != 'E')
			continue;
		/*
		 * Got a file in the directory starting with E...
		 * Save a consed up name for the file to unlink
		 * later, and check that this is really a file
		 * we are looking for.
		 */
		strlcpy(nb, dirname, sizeof nb);
		strlcat(nb, "/", sizeof nb);
		strlcat(nb, dirent->d_name, sizeof nb);
		if (yeah(nb)) {
			/*
			 * Well, it is the file we are looking for.
			 * Is it more recent than any version we found before?
			 */
			if (H.Time > besttime) {
				/*
				 * A winner.
				 */
				ignore(close(bestfd));
				bestfd = dup(tfile);
				besttime = H.Time;
				strlcpy(bestnb, nb, sizeof bestnb);
			}
			/*
			 * Count versions so user can be told there are
			 * ``yet more pages to be turned''.
			 */
			vercnt++;
		}
		ignore(close(tfile));
	}
	ignore(closedir(dir));
}

/*
 * Given a candidate file to be recovered, see
 * if its really an editor temporary and of this
 * user and the file specified.
 */
int
yeah(char *name)
{

	tfile = open(name, O_RDWR);
	if (tfile < 0)
		return (0);
	if (read(tfile, (char *) &H, sizeof H) != sizeof H) {
nope:
		ignore(close(tfile));
		return (0);
	}
	if (strcmp(savedfile, file))
		goto nope;
	if (getuid() != H.Uid)
		goto nope;
	/*
	 * This is old and stupid code, which
	 * puts a word LOST in the header block, so that lost lines
	 * can be made to point at it.
	 */
	ignorl(lseek(tfile, (off_t) (BUFSIZ*HBLKS-8), SEEK_SET));
	ignore(write(tfile, "LOST", 5));
	return (1);
}

int
preserve(void)
{
	return 0;
}

/*
 * Find the true end of the scratch file, and ``LOSE''
 * lines which point into thin air.  This lossage occurs
 * due to the sandbagging of i/o which can cause blocks to
 * be written in a non-obvious order, different from the order
 * in which the editor tried to write them.
 *
 * Lines which are lost are replaced with the text LOST so
 * they are easy to find.  We work hard at pretty formatting here
 * as lines tend to be lost in blocks.
 *
 * This only seems to happen on very heavily loaded systems, and
 * not very often.
 */
void
scrapbad(void)
{
	register line *ip;
	struct stat stbuf;
	off_t size, maxt;
	bbloc bno, cnt = 0, bad, was;
	char bk[BUFSIZ];

	ignore(fstat(tfile, &stbuf));
	size = stbuf.st_size;
	maxt = (size >> SHFT) | (BNDRY-1);
	bno = (maxt >> OFFBTS) & BLKMSK;
#ifdef DEBUG
	xfprintf(xstderr, "size %ld, maxt %o, bno %d\n", size, maxt, bno);
#endif

	/*
	 * Look for a null separating two lines in the temp file;
	 * if last line was split across blocks, then it is lost
	 * if the last block is.
	 */
	while (bno > 0) {
		ignorl(lseek(tfile, (off_t) (BUFSIZ * (bno & BLKMSK)),
					SEEK_SET));
		cnt = read(tfile, (char *) bk, BUFSIZ);
		while (cnt > 0)
			if (bk[--cnt] == 0)
				goto null;
		bno--;
	}
null:

	/*
	 * Magically calculate the largest valid pointer in the temp file,
	 * consing it up from the block number and the count.
	 */
	maxt = ((bno << OFFBTS) | (cnt >> SHFT)) & ~1;
#ifdef DEBUG
	xfprintf(xstderr, "bno %d, cnt %d, maxt %o\n", bno, cnt, maxt);
#endif

	/*
	 * Now cycle through the line pointers,
	 * trashing the Lusers.
	 */
	was = bad = 0;
	for (ip = one; ip <= dol; ip++)
		if (*ip > maxt) {
#ifdef DEBUG
			xfprintf(xstderr, "%d bad, %o > %o\n", ip - zero, *ip, maxt);
#endif
			if (was == 0)
				was = ip - zero;
			*ip = ((HBLKS*BUFSIZ)-8) >> SHFT;
		} else if (was) {
			if (bad == 0)
				xfprintf(xstderr, catgets(catd, 2, 10,
						" [Lost line(s):"));
			xfprintf(xstderr, catgets(catd, 2, 11,
						" %d"), was);
			if ((ip - 1) - zero > was)
				xfprintf(xstderr, catgets(catd, 2, 12, "-%d"),
						(int) ((ip - 1) - zero));
			bad++;
			was = 0;
		}
	if (was != 0) {
		if (bad == 0)
			xfprintf(xstderr, catgets(catd, 2, 13,
						" [Lost line(s):"));
		xfprintf(xstderr, catgets(catd, 2, 14, " %d"), was);
		if (dol - zero != was)
			xfprintf(xstderr, catgets(catd, 2, 15,
						"-%d"), (int) (dol - zero));
		bad++;
	}
	if (bad)
		xfprintf(xstderr, catgets(catd, 2, 16, "]"));
}

int	cntch, cntln, cntodd, cntnull;

/*
 * Following routines stolen mercilessly from ex.
 */
void
putfile(int unused)
{
	line *a1;
	register char *fp, *lp;
	register int nib;

	(void)unused;
	a1 = addr1;
	clrstats();
	cntln = addr2 - a1 + 1;
	if (cntln == 0)
		return;
	nib = BUFSIZ;
	fp = genbuf;
	do {
		getline(*a1++);
		lp = linebuf;
		for (;;) {
			if (--nib < 0) {
				nib = fp - genbuf;
				if (write(io, genbuf, nib) != nib)
					wrerror();
				cntch += nib;
				nib = MAXBSIZE - 1 /* 511 */;
				fp = genbuf;
			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	nib = fp - genbuf;
	if (write(io, genbuf, nib) != nib)
		wrerror();
	cntch += nib;
}

void
wrerror(void)
{

	syserror();
}

void
clrstats(void)
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;
	cntodd = 0;
}

#define	READ	0
#define	WRITE	1

void
getline(line tl)
{
	register char *bp, *lp;
	register int nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~OFFMSK;
	while ((*lp++ = *bp++))
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, READ);
			nl = nleft;
		}
}

char *
getblock(line atl, int iof)
{
	register bbloc bno, off;
	
	bno = (atl >> OFFBTS) & BLKMSK;
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS)
		error(catgets(catd, 2, 17, " Tmp file too large"));
	nleft = BUFSIZ - off;
	if (bno == iblock) {
		ichanged |= iof;
		return (ibuff + off);
	}
	if (bno == oblock)
		return (obuff + off);
	if (iof == READ) {
		if (ichanged)
			blkio(iblock, ibuff,
				(ssize_t(*)(int, void *, size_t))write);
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, (ssize_t(*)(int, void *, size_t))read);
		return (ibuff + off);
	}
	if (oblock >= 0)
		blkio(oblock, obuff, (ssize_t(*)(int, void *, size_t))write);
	oblock = bno;
	return (obuff + off);
}

void
blkio(bloc b, char *buf, ssize_t (*iofcn)(int, void *, size_t))
{

	lseek(tfile, (off_t) ((b & BLKMSK) * BUFSIZ), SEEK_SET);
	if ((*iofcn)(tfile, buf, BUFSIZ) != BUFSIZ)
		syserror();
}

void
syserror(void)
{

	dirtcnt = 0;
	write(2, " ", 1);
	error("%s", strerror(errno));
	exit(1);
}

/*
 * Must avoid stdio because expreserve uses sbrk to do memory
 * allocation and stdio uses malloc.
 */
/*
 * I do not know whether vsprintf() uses malloc() or not.
 * So this may be fail, too.
 */
void
xvfprintf(xFILE *fp, char *fmt, va_list ap)
{
	char buf[BUFSIZ];

	if (fp != xstderr)
		return;
	vsnprintf(buf, sizeof buf, fmt, ap);
	write(2, buf, strlen(buf));
}

void
xfprintf(xFILE *fp, char *fmt, ...)
{
	va_list ap;

	if (fp != xstderr)
		return;
	va_start(ap, fmt);
	xvfprintf(fp, fmt, ap);
	va_end(ap);
}
