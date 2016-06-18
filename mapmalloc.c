/*
 * AT&T Unix 7th Edition memory allocation routines.
 *
 * Modified by Gunnar Ritter, Freiburg i. Br., Germany, February 2005.
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
 *	Sccsid @(#)mapmalloc.c	1.7 (gritter) 8/18/05
 */

#ifdef	VMUNIX

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <inttypes.h>

#ifndef	MAP_FAILED
#define	MAP_FAILED	((void *)-1)
#endif	/* !MAP_FAILED */

#ifndef	MAP_ANON
#ifdef	MAP_ANONYMOUS
#define	MAP_ANON	MAP_ANONYMOUS
#else	/* !MAP_ANONYMOUS */
#include <sys/stat.h>
#include <fcntl.h>
#endif	/* !MAP_ANONYMOUS */
#endif	/* !MAP_ANON */

#include "config.h"

/*
 * Since ex makes use of sbrk(), the C library's version of malloc()
 * must be avoided.
 */

/*
#define debug
#define longdebug
*/

#ifdef debug
#define ASSERT(p) if(!(p))botch("p");else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int 
botch(char *s)
{
	const char msg[] = "assertion botched\n";
	write(2, msg, sizeof msg - 1);
	/*printf("assertion botched: %s\n",s);*/
	abort();
}
static int allock(void *);
#ifdef	debugprint
void dump(const char *msg, uintptr_t t)
{
	const char hex[] = "0123456789ABCDEF";
	int	i;
	write(2, msg, strlen(msg));
	write(2, ": ", 2);
	for (i = sizeof t - 1; i >= 0; i--) {
		write(2, &hex[(t & (0x0f << 8*i+4)) >> 8*i+4], 1);
		write(2, &hex[(t & (0x0f << 8*i)) >> 8*i], 1);
	}
	write(2, "\n", 1);
}
#else
#define	dump(a, b)
#endif
#else
#define ASSERT(p)
#define	dump(a, b)
#endif

#ifdef valgrind
#include <valgrind.h>
#else	/* !valgrind */
#define	VALGRIND_MALLOCLIKE_BLOCK(a, b, c, d)
#define	VALGRIND_FREELIKE_BLOCK(a, b)
#endif	/* !valgrind */

/*	avoid break bug */
#ifdef pdp11
#define GRANULE 64
#else
#define GRANULE 0
#endif
/*	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block
 *	blocks are exact number of words long 
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	this variant uses mmap() instead of sbrk()
 *	mmap() is used to allocate pools of increasing size
 *	memory is then allocated from the first possible pool
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
*/
#define	INT		intptr_t
#define	ALIGN		intptr_t
#define	NALIGN		1
#define	WORD		sizeof (union store)
#define	BLOCK		1024	/* a multiple of WORD*/
#define	BUSY		((intptr_t)1)
#ifdef	NULL
#undef	NULL
#endif
#define NULL		0
#define	testbusy(p)	((INT)(p)&BUSY)
#define	setbusy(p)	(union store *)((INT)(p)|BUSY)
#define	clearbusy(p)	(union store *)((INT)(p)&~BUSY)

static	struct pool *pool0;

union store { union store *ptr;
	      struct pool *pool;
	      ALIGN dummy[NALIGN];
	      INT callocsp;	/*calloc clears an array of integers*/
};

struct pool {
	struct pool *Next;
	union store Allocs[2];	/*initial arena*/
	union store *Allocp;	/*search ptr*/
	union store *Alloct;	/*arena top*/
	union store *Allocx;	/*for benefit of realloc*/
	char *Brk;
	char *End;
	ALIGN Dummy[NALIGN];
};

#define	allocs	(o->Allocs)
#define	allocp	(o->Allocp)
#define	alloct	(o->Alloct)
#define	allocx	(o->Allocx)

static void *
map(void *addr, size_t len)
{
#ifndef	MAP_ANON
	int flags = 0;
	static int fd = -1;

	if (fd==-1 && ((fd=open("/dev/zero",O_RDWR))<0 ||
			fcntl(fd,F_SETFD,FD_CLOEXEC)<0))
		return(MAP_FAILED);
#else	/* MAP_ANON */
	int flags = MAP_ANON;
	int fd = -1;
#endif	/* MAP_ANON */
	flags |= MAP_PRIVATE;
	if (addr)
		flags |= MAP_FIXED;
	return(mmap(addr,len,PROT_READ|PROT_WRITE,flags,fd,0));
}

static void *
mallock(size_t nbytes, union store *start, union store *end)
{
	union store *p, *q;
	struct pool *o;
	size_t nw;
	static size_t temp;	/*coroutines assume no auto*/
	static size_t poolblock = 0100000;

	if (nbytes == 0)
		nbytes = 1;
	if(pool0==0 || pool0==MAP_FAILED) {	/*first time*/
		if((pool0=map(NULL, poolblock))==MAP_FAILED) {
			errno = ENOMEM;
			return(NULL);
		}
		pool0->Brk = (char *)pool0->Dummy;
		pool0->End = (char *)pool0+poolblock;
	}
	o = pool0;
first:	if(allocs[0].ptr==0) {	/*first time for this pool*/
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
	}
	nw = (nbytes+2*WORD+WORD-1)/WORD;
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(allock(o));
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {
				while(!testbusy((q=p->ptr)->ptr)) {
					int ua = p->ptr==allocp;
					ASSERT(q>p&&q<alloct);
					p->ptr = q->ptr;
					if (ua)
						allocp = p->ptr;
				}
				if(q>=p+nw && p+nw>=p && (start==NULL ||
						p+nw<start || p>end ||
						p+2==start))
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if(p>q)
				ASSERT(p<=alloct);
			else if(q!=alloct || p!=allocs) {
				ASSERT(q==alloct&&p==allocs);
				errno = ENOMEM;
				return(NULL);
			} else if(++temp>1)
				break;
		}
		temp = ((nw+BLOCK/WORD)/(BLOCK/WORD))*(BLOCK/WORD);
		q = (void *)o->Brk;
		if(q+temp+GRANULE < q) {
			errno = ENOMEM;
			return(NULL);
		}
		if(o->Brk+temp*WORD>=o->End) {
			size_t new;
			if(o->Next!=0&&o->Next!=MAP_FAILED) {
				o = o->Next;
				goto first;
			}
			poolblock += poolblock/(poolblock&(poolblock-1) ? 3:2);
			new = (((nw*WORD)+poolblock)/poolblock)*poolblock;
			if ((o->Next=map(0,new))==MAP_FAILED) {
				poolblock /= 4;
				new=(((nw*WORD)+poolblock)/poolblock)*poolblock;
				if ((o->Next=map(0,new))==MAP_FAILED) {
					errno = ENOMEM;
					return(NULL);
				}
			}
			o = o->Next;
			o->Brk = (char *)o->Dummy;
			o->End = (char *)o + new;
			goto first;
		}
		o->Brk += temp*WORD;
		ASSERT(q>alloct);
		alloct->ptr = q;
		if(q!=alloct+1)
			alloct->ptr = setbusy(alloct->ptr);
		alloct = q->ptr = q+temp-1;
		alloct->ptr = setbusy(allocs);
	}
found:
	allocp = p + nw;
	ASSERT(allocp<=alloct);
	if(q>allocp) {
		allocx = allocp->ptr;
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
	p[1].pool = o;
	dump("malloc", (uintptr_t)(p + 2));
	VALGRIND_MALLOCLIKE_BLOCK(p+2,nbytes,0,0);
	return(p+2);
}

void *
malloc(size_t nbytes)
{
	return mallock(nbytes, NULL, NULL);
}

/*	freeing strategy tuned for LIFO allocation
*/
void 
free(register void *ap)
{
	union store *p = ap;
	struct pool *o;

	dump("  free", (uintptr_t)ap);
	if (ap == NULL)
		return;
	o = p[-1].pool;
	ASSERT(p>clearbusy(allocs[1].ptr)&&p<=alloct);
	ASSERT(allock(o));
	allocp = p -= 2;
	ASSERT(testbusy(p->ptr));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
	VALGRIND_FREELIKE_BLOCK(ap,0);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
*/

void *
realloc(void *ap, size_t nbytes)
{
	union store *p = ap;
	union store *q;
	struct pool *o;
	union store *s, *t;
	size_t nw;
	size_t onw;

	if (p==NULL)
		return(malloc(nbytes));
	if (nbytes==0) {
		free(p);
		return NULL;
	}
	if(testbusy(p[-2].ptr))
		free(p);
	onw = p[-2].ptr - p;
	o = p[-1].pool;
	q = mallock(nbytes, p, &p[onw]);
	if(q==NULL || q==p)
		return(q);
	s = p;
	t = q;
	nw = (nbytes+WORD-1)/WORD;
	if(nw<onw)
		onw = nw;
	while (onw) {
		onw--;
		*t++ = *s++;
	}
	if(q<p && q+nw>=p && p[-1].pool==q[-1].pool)
		(q+(q+nw-p))->ptr = allocx;
	return(q);
}

#ifdef debug
int 
allock(void *ao)
{
#ifdef longdebug
	struct pool *o = ao;
	union store *p;
	int x;
	x = 0;
	for(p= &allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		if(p==allocp)
			x++;
	}
	ASSERT(p==alloct);
	ASSERT(x==1|p==allocp);
#endif
	return(1);
}
#endif

/*	calloc - allocate and clear memory block
*/
#define CHARPERINT (sizeof(INT)/sizeof(char))

void *
calloc(size_t num, size_t size)
{
	char *mp;
	INT *q;
	size_t m;

	num *= size;
	mp = malloc(num);
	if(mp==NULL)
		return(NULL);
	q = (INT *) mp;
	m = (num+CHARPERINT-1)/CHARPERINT;
	while(m) {
		m--;
		*q++ = 0;
	}
	return(mp);
}

#ifdef	notdef
/*ARGSUSED*/
void 
cfree(char *p, size_t num, size_t size)
{
	free(p);
}

/*
 * Just in case ...
 */
char *
memalign(size_t alignment, size_t size)
{
	return NULL;
}

char *
valloc(size_t size)
{
	return NULL;
}

char *
mallinfo(void)
{
	return NULL;
}

int 
mallopt(void)
{
	return -1;
}
#endif	/* notdef */

char *
poolsbrk(intptr_t val) {
	(void)val;
	return NULL;
}

#endif	/* VMUNIX */
