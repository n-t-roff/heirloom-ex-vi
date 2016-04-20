/* Copyright (c) 1985 Regents of the University of California */

/*
 * These routines are for faster tag lookup.  They support the binary
 * search used in tagfind() instead of the linear search.  The speedup
 * is quite noticable looking for a tag at the end of a long tags
 * file.  Define FASTTAG in the Makefile to use these routines.
 *
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

#ifdef FASTTAG
#ifndef	lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)ex_tagio.c	1.12 (gritter) 8/4/05";
#endif
#endif

/* from ex_tagio.c	7.3 (Berkeley) 1/31/86 */

#include "ex.h"

static long offset = -1;
static long block = -1;
static int bcnt = 0;
static int b_size;
static char *ibuf;

int 
topen(char *file, char *buf)
{
	int fd;
	struct stat statb;

	b_size = MAXBSIZE;
	offset = -1;
	block = -1;
	if ((fd = open(file, O_RDONLY, 0)) < 0)
		return(-1);
	if (fstat(fd, &statb) < 0) {
		close(fd);
		return(-1);
	}
	ibuf = buf;
	if (statb.st_blksize <= MAXBSIZE)
		b_size = statb.st_blksize;
	return(fd);
}

int 
tseek(int fd, off_t off)
{
	off_t nblock;

	nblock = off / b_size * b_size;
	offset = off % b_size;
	if (nblock == block)
		return(0);
	block = nblock;
	if (lseek(fd, (off_t) nblock, SEEK_SET) < 0)
		return(-1);
	if ((bcnt = read(fd, ibuf, b_size)) < 0)
		return(-1);
	return(0);
}

int 
tgets(register char *buf, int cnt, int fd)
{
	register char *cp;
	register int cc;

	cc = offset;
	if (cc == -1) {
		if ((bcnt = read(fd, ibuf, b_size)) <= 0)
			return 0;
		cc = 0;
		block = 0;
	}
	if (bcnt == 0)		/* EOF */
		return 0;
	cp = ibuf + cc;
	while (--cnt > 0) {
		if (++cc > bcnt) {
			block += b_size;
			if ((bcnt = read(fd, ibuf, b_size)) <= 0) {
				offset = cc;
				return 0;
			}
			cp = ibuf;
			cc = 1;
		}
		if ((*buf++ = *cp++) == '\n')
			break;
	}
	*--buf = 0;
	offset = cc;
	return(1);
}

void 
tclose(int fd)
{
	close(fd);
	offset = -1;
	block = -1;
	bcnt = 0;
}
#endif
