/*
 * AT&T Unix 7th Edition memory allocation routines.
 *
 * Modified for ex by Gunnar Ritter, Freiburg i. Br., Germany,
 * July 2000.
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
 *	@(#)malloc.c	1.19 (gritter) 2/20/05
 */

#ifdef	VMUNIX

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"

#ifdef	LANGMSG
#include <nl_types.h>
extern	nl_catd	catd;
#else
#define	catgets(a, b, c, d)	(d)
#endif

/*
 * Since ex makes use of sbrk(), the C library's version of malloc()
 * must be avoided.
 *
 * In ex, malloc() calls sbrk() only one time with an argument of
 * POOL. Ex itselves never uses malloc() internally, so POOL
 * must be sufficient for library calls like setlocale() only.
 *
 * Known problem: If linking against ncurses, changing the terminal
 * type repeatedly outruns the pool. Even that is not really a
 * problem since the work continues with the old terminal type, so
 * there is no need for a large pool here.
 */
#define	POOL	32768

#ifdef debug
#define ASSERT(p) if(!(p))botch("p");else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int 
botch(char *s)
{
	const char msg[] = "assertion botched\n";
	write(2, msg, sizeof msg - 1);
	/*printf("assertion botched: %s\n",s);*/
	abort();
}
static int allock(void);
#else
#define ASSERT(p)
#endif

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
#define NULL		 0
#define	testbusy(p)	((INT)(p)&BUSY)
#define	setbusy(p)	(union store *)((INT)(p)|BUSY)
#define	clearbusy(p)	(union store *)((INT)(p)&~BUSY)

union store { union store *ptr;
	      ALIGN dummy[NALIGN];
	      INT callocsp;	/*calloc clears an array of integers*/
};

static	union store allocs[2];	/*initial arena*/
static	union store *allocp;	/*search ptr*/
static	union store *alloct;	/*arena top*/
static	union store *allocx;	/*for benefit of realloc*/
extern int	error(char *, ...);

char *
poolsbrk(intptr_t inc)
{
	static char *pool;
	static intptr_t ps;
	intptr_t os, ns;

	if (pool == NULL)
		if ((pool = sbrk(POOL)) == (char *)-1)
			error(catgets(catd, 1, 241,
				"No memory pool"));
	if (inc == 0)
		return pool + ps;
	os = ps;
	ns = ps + inc;
	if (ns >= POOL)
		error(catgets(catd, 1, 242,
				"Memory pool exhausted"));
	ps = ns;
	return pool + os;
}

void *
malloc(size_t nbytes)
{
	register union store *p, *q;
	register int nw;
	static int temp;	/*coroutines assume no auto*/

	if(allocs[0].ptr==0) {	/*first time*/
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
	}
	nw = (nbytes+WORD+WORD-1)/WORD;
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(allock());
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
				if(q>=p+nw && p+nw>=p)
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
		q = (union store *)poolsbrk(0);
		if(q+temp+GRANULE < q) {
			errno = ENOMEM;
			return(NULL);
		}
		q = (union store *)poolsbrk(temp*WORD);
		if((INT)q == -1) {
			errno = ENOMEM;
			return(NULL);
		}
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
	return((char *)(p+1));
}

/*	freeing strategy tuned for LIFO allocation
*/
void
free(register void *ap)
{
	register union store *p = ap;

	if (ap == NULL)
		return;
	ASSERT(p>clearbusy(allocs[1].ptr)&&p<=alloct);
	ASSERT(allock());
	allocp = --p;
	ASSERT(testbusy(p->ptr));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
*/

void *
realloc(void *ap, size_t nbytes)
{
	register union store *p = ap;
	register union store *q;
	union store *s, *t;
	register size_t nw;
	size_t onw;

	if (p == NULL)
		return malloc(nbytes);
	if (nbytes == 0) {
		free(p);
		return NULL;
	}
	if(testbusy(p[-1].ptr))
		free(p);
	onw = p[-1].ptr - p;
	q = malloc(nbytes);
	if(q==NULL || q==p)
		return(q);
	s = p;
	t = q;
	nw = (nbytes+WORD-1)/WORD;
	if(nw<onw)
		onw = nw;
	while(onw--!=0)
		*t++ = *s++;
	if(q<p && q+nw>=p)
		(q+(q+nw-p))->ptr = allocx;
	return(q);
}

#ifdef debug
int 
allock(void)
{
#ifdef longdebug
	register union store *p;
	int x;
	x = 0;
	for(p= &allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		if(p==allocp)
			x++;
	}
	ASSERT(p==alloct);
	return(x==1|p==allocp);
#else
	return(1);
#endif
}
#endif

/*	calloc - allocate and clear memory block
*/
#define CHARPERINT (sizeof(INT)/sizeof(char))

void *
calloc(size_t num, size_t size)
{
	register char *mp;
	register INT *q;
	register int m;

	num *= size;
	mp = malloc(num);
	if(mp == NULL)
		return(NULL);
	q = (INT *) mp;
	m = (num+CHARPERINT-1)/CHARPERINT;
	while(--m >= 0)
		*q++ = 0;
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

#endif	/* VMUNIX */
