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
static char sccsid[] = "@(#)ex_temp.c	1.27 (gritter) 12/25/06";
#endif
#endif

/* from ex_temp.c	7.5.1.1 (Berkeley) 8/12/86 */

#include "ex.h"
#include "ex_temp.h"
#include "ex_vis.h"
#include "ex_tty.h"
#include <sys/wait.h>
#include <time.h>

/*
 * Editor temporary file routines.
 * Very similar to those of ed, except uses 2 input buffers.
 */
#define	READ	0
#define	WRITE	1

/*
 * Maximum number of attempts to create temporary file.
 */
#define	ATTEMPTS	20

char	*tfname;
char	*rfname;
int	havetmp;
int	tfile = -1;
int	rfile = -1;

void
fileinit(void)
{
	register char *p;
	struct stat stbuf;
	register int i, j;
	pid_t mypid = getpid();
	char *tfend;
	int attempts = 0;

	CLOBBGRD(attempts);
	if (tline == INCRMT * (HBLKS+2))
		return;
	cleanup(0);
	if (tfile != -1)
		close(tfile);
	tline = INCRMT * (HBLKS+2);
	blocks[0] = HBLKS;
	blocks[1] = HBLKS+1;
	blocks[2] = -1;
	dirtcnt = 0;
	iblock = -1;
	iblock2 = -1;
	oblock = -1;
	tfname = realloc(tfname, strlen(svalue(DIRECTORY)) + 14);
	CP(tfname, svalue(DIRECTORY));
	if (stat(tfname, &stbuf)) {
dumbness:
		if (setexit() == 0)
			filioerr(tfname);
		else
			putNFL();
		cleanup(1);
		exitex(1);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		goto dumbness;
	}
	ichanged = 0;
	ichang2 = 0;
#ifdef	notdef	/* GR */
	ignore(strcat(tfname, "/ExXXXXX"));
	for (p = strend(tfname), i = 5, j = getpid(); i > 0; i--, j /= 10)
		*--p = j % 10 | '0';
	tfile = creat(tfname, 0600);
#else
	ignore(strcat(tfname, "/ExXXXXXXXXXX"));
	tfend = strend(tfname);
	do {
		for (p = tfend, i = 10, j = mypid + attempts;
				i > 0; i--, j /= 10)
			*--p = j % 10 | '0';
		tfile = open(tfname, O_CREAT|O_EXCL|O_RDWR
#ifdef	O_NOFOLLOW
				|O_NOFOLLOW
#endif	/* O_NOFOLLOW */
				, 0600);
	} while (tfile < 0 && attempts++ < ATTEMPTS);
#endif	/* !notdef */
	if (tfile < 0)
		goto dumbness;
#ifdef INCORB
	{
		extern bloc stilinc;		/* see below */
		stilinc = 0;
	}
#endif
	havetmp = 1;
/* 	brk((char *)fendcore); */
}

void
cleanup(bool all)
{
	if (all) {
		putpad(TE);
		flush();
	}
	if (havetmp)
		unlink(tfname);
	havetmp = 0;
	if (all && rfile >= 0) {
		unlink(rfname);
		close(rfile);
		rfile = -1;
	}
}

void
getline(line tl)
{
	register char *bp, *lp;
	register bbloc nl;

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

line
putline(void)
{
	register char *bp, *lp;
	register bbloc nl;
	line tl;

	dirtcnt++;
	lp = linebuf;
	change();
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~OFFMSK;
	while ((*bp = *lp++)) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, WRITE);
			nl = nleft;
		}
	}
	tl = tline;
	tline += (((lp - linebuf) + BNDRY - 1) >> SHFT) & TLNMSK;
	return (tl);
}

char *
getblock(line atl, int iof)
{
	register bbloc bno, off;
	
	bno = (atl >> OFFBTS) & BLKMSK;
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS)
		error(catgets(catd, 1, 183, " Tmp file too large"));
	nleft = BUFSIZ - off;
	if (bno == iblock) {
		ichanged |= iof;
		hitin2 = 0;
		return (ibuff + off);
	}
	if (bno == iblock2) {
		ichang2 |= iof;
		hitin2 = 1;
		return (ibuff2 + off);
	}
	if (bno == oblock)
		return (obuff + off);
	if (iof == READ) {
		if (hitin2 == 0) {
			if (ichang2) {
				blkio(iblock2, ibuff2,
					(ssize_t(*)(int, void *, size_t))write);
			}
			ichang2 = 0;
			iblock2 = bno;
			blkio(bno, ibuff2,
				(ssize_t(*)(int, void *, size_t))read);
			hitin2 = 1;
			return (ibuff2 + off);
		}
		hitin2 = 0;
		if (ichanged) {
			blkio(iblock, ibuff,
				(ssize_t(*)(int, void *, size_t))write);
		}
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, (ssize_t(*)(int, void *, size_t))read);
		return (ibuff + off);
	}
	if (oblock >= 0) {
			blkio(oblock, obuff,
				(ssize_t(*)(int, void *, size_t))write);
	}
	oblock = bno;
	return (obuff + off);
}

#ifdef	INCORB
char	incorb[INCORB+1][BUFSIZ];
#define	pagrnd(a)	((char *)(((size_t)a)&~(BUFSIZ-1)))
bloc	stilinc;	/* up to here not written yet */
#endif

void
blkio(bloc b, char *buf, ssize_t (*iofcn)(int, void *, size_t))
{

#ifdef INCORB
	if (b < INCORB) {
		if (iofcn == (ssize_t(*)(int, void *, size_t))read) {
			copy(buf, pagrnd(incorb[b+1]), (size_t) BUFSIZ);
			return;
		}
		copy(pagrnd(incorb[b+1]), buf, (size_t) BUFSIZ);
		if (laste) {
			if (b >= stilinc)
				stilinc = b + 1;
			return;
		}
	} else if (stilinc)
		tflush();
#endif
	lseek(tfile, (off_t) ((b & BLKMSK) * BUFSIZ), SEEK_SET);
	if ((*iofcn)(tfile, buf, BUFSIZ) != BUFSIZ)
		filioerr(tfname);
}

#ifdef INCORB
void
tlaste(void)
{

	if (stilinc)
		dirtcnt = 0;
}

void
tflush(void)
{
	bbloc i = stilinc;
	
	stilinc = 0;
	lseek(tfile, (off_t) 0, SEEK_SET);
	if (write(tfile, pagrnd(incorb[1]), i * BUFSIZ) != (i * BUFSIZ))
		filioerr(tfname);
}
#endif

/*
 * Synchronize the state of the temporary file in case
 * a crash occurs.
 */
void
synctmp(void)
{
	register bbloc cnt;
	register line *a;
	register bloc *bp, *up;

#ifdef INCORB
	if (stilinc)
		return;
#endif
	if (dol == zero)
		return;
	if (ichanged)
		blkio(iblock, ibuff, (ssize_t(*)(int, void *, size_t))write);
	ichanged = 0;
	if (ichang2)
		blkio(iblock2, ibuff2, (ssize_t(*)(int, void *, size_t))write);
	ichang2 = 0;
	if (oblock != -1)
		blkio(oblock, obuff, (ssize_t(*)(int, void *, size_t))write);
	time(&H.Time);
	uid = getuid();
	*zero = (line) H.Time;
	up = blocks + LBLKS;
	for (a = zero, bp = blocks; a <= dol; a += BUFSIZ / sizeof *a, bp++) {
		if (bp >= up)
			error(catgets(catd, 1, 184, " Tmp file too large"));
		if (*bp < 0) {
			tline = (tline + OFFMSK) &~ OFFMSK;
			*bp = ((tline >> OFFBTS) & BLKMSK);
			if (*bp > NMBLKS)
				error(catgets(catd, 1, 185,
						" Tmp file too large"));
			tline += INCRMT;
			oblock = *bp + 1;
			bp[1] = -1;
		}
		lseek(tfile, (off_t) ((*bp & BLKMSK) * BUFSIZ), SEEK_SET);
		cnt = ((dol - a) + 2) * sizeof (line);
		if (cnt > BUFSIZ)
			cnt = BUFSIZ;
		if (write(tfile, (char *) a, cnt) != cnt) {
oops:
			*zero = 0;
			filioerr(tfname);
		}
		*zero = 0;
	}
	flines = lineDOL();
	lseek(tfile, (off_t) 0, SEEK_SET);
	if (write(tfile, (char *) &H, sizeof H) != sizeof H)
		goto oops;
#ifdef notdef
	/*
	 * This will insure that exrecover gets as much
	 * back after a crash as is absolutely possible,
	 * but can result in pregnant pauses between commands
	 * when the TSYNC call is made, so...
	 */
	fsync(tfile);
#endif
}

void
TSYNC(void)
{

	if (dirtcnt > MAXDIRT) {	/* mjm: 12 --> MAXDIRT */
#ifdef INCORB
		if (stilinc)
			tflush();
#endif
		dirtcnt = 0;
		synctmp();
	}
}

/*
 * Named buffer routines.
 * These are implemented differently than the main buffer.
 * Each named buffer has a chain of blocks in the register file.
 * Each block contains roughly 508 chars of text,
 * and a previous and next block number.  We also have information
 * about which blocks came from deletes of multiple partial lines,
 * e.g. deleting a sentence or a LISP object.
 *
 * We maintain a free map for the temp file.  To free the blocks
 * in a register we must read the blocks to find how they are chained
 * together.
 *
 * BUG:		The default savind of deleted lines in numbered
 *		buffers may be rather inefficient; it hasn't been profiled.
 */
struct	strreg {
	short	rg_flags;
	short	rg_nleft;
	short	rg_first;
	short	rg_last;
} strregs[('z'-'a'+1) + ('9'-'0'+1)], *strp;

struct	rbuf {
	short	rb_prev;
	short	rb_next;
	char	rb_text[BUFSIZ - 2 * sizeof (short)];
} *rbuf, KILLrbuf, putrbuf, YANKrbuf, regrbuf;
#ifdef	VMUNIX
#ifdef	LARGEF
short	rused[4096];
#else	/* !LARGEF */
short	rused[256];
#endif	/* !LARGEF */
#else	/* !VMUNIX */
short	rused[32];
#endif	/* !VMUNIX */
short	rnleft;
short	rblock;
short	rnext;
char	*rbufcp;

void
regio(short b, ssize_t (*iofcn)(int, void *, size_t))
{
	register char *p;
	char *rfend;
	int attempts = 0;
	register int i, j;
	pid_t mypid = getpid();

	if (rfile == -1) {
		rfname = realloc(rfname, strlen(svalue(DIRECTORY)) + 14);
		CP(rfname, tfname);
		rfend = strend(rfname);
#ifdef	notdef	/* GR */
		*(rfend - 7) = 'R';
#else
		*(rfend - 12) = 'R';
#endif
		do {
			for (p = rfend, i = 10, j = mypid + attempts;
					i > 0; i--, j /= 10)
				*--p = j % 10 | '0';
			rfile = open(rfname, O_CREAT|O_EXCL|O_RDWR
#ifdef	O_NOFOLLOW
					|O_NOFOLLOW
#endif	/* O_NOFOLLOW */
					, 0600);
		} while (rfile < 0 && attempts++ < ATTEMPTS);
		if (rfile < 0)
oops:
			filioerr(rfname);
	}
	lseek(rfile, (off_t) ((b & BLKMSK) * BUFSIZ), SEEK_SET);
	if ((*iofcn)(rfile, rbuf, BUFSIZ) != BUFSIZ)
		goto oops;
	rblock = b;
}

int
REGblk(void)
{
	unsigned int i, j, m;

	for (i = 0; i < sizeof rused / sizeof rused[0]; i++) {
		m = (rused[i] ^ 0177777) & 0177777;
		if (i == 0)
			m &= ~1;
		if (m != 0) {
			j = 0;
			while ((m & 1) == 0)
				j++, m >>= 1;
			rused[i] |= (1 << j);
#ifdef RDEBUG
			ex_printf("allocating block %d\n", i * 16 + j);
#endif
			return (i * 16 + j);
		}
	}
	error(catgets(catd, 1, 186, "Out of register space (ugh)"));
	/*NOTREACHED*/
	return 0;
}

struct	strreg *
mapreg(register int c)
{

	if (isupper(c))
		c = tolower(c);
	return (isdigit(c) ? &strregs[('z'-'a'+1)+(c-'0')] : &strregs[c-'a']);
}

void
KILLreg(register int c)
{
	register struct strreg *sp;

	rbuf = &KILLrbuf;
	sp = mapreg(c);
	rblock = sp->rg_first;
	sp->rg_first = sp->rg_last = 0;
	sp->rg_flags = sp->rg_nleft = 0;
	while (rblock != 0) {
#ifdef RDEBUG
		ex_printf("freeing block %d\n", rblock);
#endif
		rused[rblock / 16] &= ~(1 << (rblock % 16));
		regio(rblock, (ssize_t (*)(int, void *, size_t))shread);
		rblock = rbuf->rb_next;
	}
}

ssize_t
shread(void)
{
	struct front { short a; short b; };

	if (read(rfile, (char *) rbuf, sizeof (struct front)) == sizeof (struct front))
		return (sizeof (struct rbuf));
	return (0);
}

int	getREG(void);

void
putreg(int c)
{
	register line *odot = dot;
	register line *odol = dol;
	register int cnt;

	deletenone();
	appendnone();
	rbuf = &putrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext == 0) {
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		vreg = -1;
		error(catgets(catd, 1, 187, "Nothing in register %c"), c);
	}
	if (inopen && partreg(c)) {
		if (!FIXUNDO) {
			splitw++; vclean(); vgoto(WECHO, 0); vreg = -1;
			error(catgets(catd, 1, 188,
				"Can't put partial line inside macro"));
		}
		squish();
		addr1 = addr2 = dol;
	}
	cnt = append(getREG, addr2);
	if (inopen && partreg(c)) {
		unddol = dol;
		dol = odol;
		dot = odot;
		pragged(0);
	}
	killcnt(cnt);
	notecnt = cnt;
}

int
partreg(int c)
{

	return (mapreg(c)->rg_flags);
}

void
notpart(register int c)
{

	if (c)
		mapreg(c)->rg_flags = 0;
}

int
getREG(void)
{
	register char *lp = linebuf;
	register int c;

	for (;;) {
		if (rnleft == 0) {
			if (rnext == 0)
				return (EOF);
			regio(rnext, read);
			rnext = rbuf->rb_next;
			rbufcp = rbuf->rb_text;
			rnleft = sizeof rbuf->rb_text;
		}
		c = *rbufcp;
		if (c == 0)
			return (EOF);
		rbufcp++, --rnleft;
		if (c == '\n') {
			*lp++ = 0;
			return (0);
		}
		*lp++ = c;
	}
}

void
YANKreg(register int c)
{
	register line *addr;
	register struct strreg *sp;
	char *savelb;

	savelb = smalloc(LBSIZE);
	if (isdigit(c))
		kshift();
	if (islower(c))
		KILLreg(c);
	strp = sp = mapreg(c);
	sp->rg_flags = inopen && cursor && wcursor;
	rbuf = &YANKrbuf;
	if (sp->rg_last) {
		regio(sp->rg_last, read);
		rnleft = sp->rg_nleft;
		rbufcp = &rbuf->rb_text[sizeof rbuf->rb_text - rnleft];
	} else {
		rblock = 0;
		rnleft = 0;
	}
	lcpy(savelb,linebuf, LBSIZE);
	for (addr = addr1; addr <= addr2; addr++) {
		getline(*addr);
		if (sp->rg_flags) {
			if (addr == addr2)
				*wcursor = 0;
			if (addr == addr1)
				lcpy(linebuf, cursor, LBSIZE);
		}
		YANKline();
	}
	rbflush();
	killed();
	lcpy(linebuf,savelb, LBSIZE);
	free(savelb);
}

void
kshift(void)
{
	register int i;

	KILLreg('9');
	for (i = '8'; i >= '0'; i--)
		copy(mapreg(i+1), mapreg(i), sizeof (struct strreg));
}

void
YANKline(void)
{
	register char *lp = linebuf;
	register struct rbuf *rp = rbuf;
	register int c;

	do {
		c = *lp++;
		if (c == 0)
			c = '\n';
		if (rnleft == 0) {
			rp->rb_next = REGblk();
			rbflush();
			rblock = rp->rb_next;
			rp->rb_next = 0;
			rp->rb_prev = rblock;
			rnleft = sizeof rp->rb_text;
			rbufcp = rp->rb_text;
		}
		*rbufcp++ = c;
		--rnleft;
	} while (c != '\n');
	if (rnleft)
		*rbufcp = 0;
}

void
rbflush(void)
{
	register struct strreg *sp = strp;

	if (rblock == 0)
		return;
	regio(rblock, (ssize_t (*)(int, void *, size_t))write);
	if (sp->rg_first == 0)
		sp->rg_first = rblock;
	sp->rg_last = rblock;
	sp->rg_nleft = rnleft;
}

/* Register c to char buffer buf of size buflen */
void
regbuf(char c, char *buf, int buflen)
{
	register char *p, *lp;

	rbuf = &regrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext==0) {
		*buf = 0;
		error(catgets(catd, 1, 189, "Nothing in register %c"),c);
	}
	p = buf;
	while (getREG()==0) {
		for (lp=linebuf; *lp;) {
			if (p >= &buf[buflen])
				error(catgets(catd, 1, 190,
					"Register too long@to fit in memory"));
			*p++ = *lp++;
		}
		*p++ = '\n';
	}
	if (partreg(c)) p--;
	*p = '\0';
	getDOT();
}
