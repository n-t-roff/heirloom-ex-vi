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
static char sccsid[] = "@(#)printf.c	1.15 (gritter) 12/1/04";
#endif
#endif

/* from printf.c	7.3 (Berkeley) 6/7/85 */

/* The pwb version this is based on */
/* from printf.c:2.2 6/5/79 */

#include "ex.h"

/*
 * This version of printf is compatible with the Version 7 C
 * printf. The differences are only minor except that this
 * printf assumes it is to print through putchar. Version 7
 * printf is more general (and is much larger) and includes
 * provisions for floating point.
 */
 
static int width, sign, fill;

int vprintf(const char *, va_list);
char *p_dconv(long, char *);
static int p_emit(char *, char *);

int
ex_printf(char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);
	return ret;
}

int
vprintf(const char *fmt, va_list ap)
{
	int	cnt = 0;
	int fcode;
	int prec;
	int length,mask1,nbits,n,m;
	long int mask2, num;
	register char *bptr;
	char *ptr;
	char buf[134];

	for (;;) {
		/* process format string first */
		while (nextc(fcode,fmt,m), fmt+=m, fcode!='%') {
			/* ordinary (non-%) character */
			if (fcode=='\0')
				return cnt;
			putchar(fcode);
			cnt += m;
		}
		/* length modifier: -1 for h, 1 for l, 0 for none */
		length = 0;
		/* check for a leading - sign */
		sign = 0;
		if (*fmt == '-') {
			sign++;
			fmt++;
		}
		/* a '0' may follow the - sign */
		/* this is the requested fill character */
		fill = 1;
		if (*fmt == '0') {
			fill--;
			fmt++;
		}
		
		/* Now comes a digit string which may be a '*' */
		if (*fmt == '*') {
			width = va_arg(ap, int);
			if (width < 0) {
				width = -width;
				sign = !sign;
			}
			fmt++;
		}
		else {
			width = 0;
			while (*fmt>='0' && *fmt<='9')
				width = width * 10 + (*fmt++ - '0');
		}
		
		/* maybe a decimal point followed by more digits (or '*') */
		if (*fmt=='.') {
			if (*++fmt == '*') {
				prec = va_arg(ap, int);
				fmt++;
			}
			else {
				prec = 0;
				while (*fmt>='0' && *fmt<='9')
					prec = prec * 10 + (*fmt++ - '0');
			}
		}
		else
			prec = -1;
		
		/*
		 * At this point, "sign" is nonzero if there was
		 * a sign, "fill" is 0 if there was a leading
		 * zero and 1 otherwise, "width" and "prec"
		 * contain numbers corresponding to the digit
		 * strings before and after the decimal point,
		 * respectively, and "fmt" addresses the next
		 * character after the whole mess. If there was
		 * no decimal point, "prec" will be -1.
		 */
		switch (*fmt) {
			case 'L':
			case 'l':
				length = 2;
				/* no break!! */
			case 'h':
			case 'H':
				length--;
				fmt++;
				break;
		}
		
		/*
		 * At exit from the following switch, we will
		 * emit the characters starting at "bptr" and
		 * ending at "ptr"-1, unless fcode is '\0'.
		 */
		switch (nextc(fcode, fmt, m), fmt += m, fcode) {
			/* process characters and strings first */
			case 'c':
				buf[0] = va_arg(ap, int);
				ptr = bptr = &buf[0];
				if (buf[0] != '\0')
					ptr++;
				break;
			case 's':
				bptr = va_arg(ap,char *);
				if (bptr==0)
					bptr = catgets(catd, 1, 248,
							"(null pointer)");
				if (prec < 0)
					prec = LRGINT;
				for (n=0; *bptr++ && n < prec; n++) ;
				ptr = --bptr;
				bptr -= n;
				break;
			case 'O':
				length = 1;
				fcode = 'o';
				/* no break */
			case 'o':
			case 'X':
			case 'x':
				if (length > 0)
					num = va_arg(ap,long);
				else
					num = (unsigned)va_arg(ap,int);
				if (fcode=='o') {
					mask1 = 0x7;
					mask2 = 0x1fffffffL;
					nbits = 3;
				}
				else {
					mask1 = 0xf;
					mask2 = 0x0fffffffL;
					nbits = 4;
				}
				n = (num!=0);
				bptr = buf + MAXOCT + 3;
				/* shift and mask for speed */
				do
				    if (((int) num & mask1) < 10)
					*--bptr = ((int) num & mask1) + 060;
				    else
					*--bptr = ((int) num & mask1) + 0127;
				while ((num = (num >> nbits) & mask2));
				
				if (fcode=='o') {
					if (n)
						*--bptr = '0';
				}
				else
					if (!sign && fill <= 0) {
						putchar('0');
						putchar(fcode);
						width -= 2;
					}
					else {
						*--bptr = fcode;
						*--bptr = '0';
					}
				ptr = buf + MAXOCT + 3;
				break;
			case 'D':
			case 'U':
			case 'I':
				length = 1;
				fcode = fcode + 'a' - 'A';
				/* no break */
			case 'd':
			case 'i':
			case 'u':
				if (length > 0)
					num = va_arg(ap,long);
				else {
					n = va_arg(ap,int);
					if (fcode=='u')
						num = (unsigned) n;
					else
						num = (long) n;
				}
				if ((n = (fcode != 'u' && num < 0)))
					num = -num;
				/* now convert to digits */
				bptr = p_dconv(num, buf);
				if (n)
					*--bptr = '-';
				if (fill == 0)
					fill = -1;
				ptr = buf + MAXDIGS + 1;
				break;
			default:
				/* not a control character, 
				 * print it.
				 */
				ptr = bptr = (char *)&fmt[-m];
				ptr++;
				break;
			}
			if (fcode != '\0')
				cnt += p_emit(bptr,ptr);
	}
}

/* p_dconv converts the unsigned long integer "value" to
 * printable decimal and places it in "buffer", right-justified.
 * The value returned is the address of the first non-zero character,
 * or the address of the last character if all are zero.
 * The result is NOT null terminated, and is MAXDIGS characters long,
 * starting at buffer[1] (to allow for insertion of a sign).
 *
 * This program assumes it is running on 2's complement machine
 * with reasonable overflow treatment.
 */
char *
p_dconv(long value, char *buffer)
{
	register char *bp;
	register int svalue;
	int n;
	long lval;
	
	bp = buffer;
	
	/* zero is a special case */
	if (value == 0) {
		bp += MAXDIGS;
		*bp = '0';
		return(bp);
	}
	
	/* develop the leading digit of the value in "n" */
	n = 0;
	while (value < 0) {
		value -= BIG;	/* will eventually underflow */
		n++;
	}
	while (value >= BIG && (lval = value - BIG) >= 0) {
		value = lval;
		n++;
	}
	
	/* stash it in buffer[1] to allow for a sign */
	bp[1] = n + '0';
	/*
	 * Now develop the rest of the digits. Since speed counts here,
	 * we do it in two loops. The first gets "value" down until it
	 * is no larger than LRGINT. The second one uses integer divides
	 * rather than long divides to speed it up.
	 */
	bp += MAXDIGS + 1;
	while (value > LRGINT) {
		*--bp = (value % 10) + '0';
		value /= 10;
	}
	
	/* cannot lose precision */
	svalue = value;
	while (svalue > 0) {
		*--bp = (svalue % 10) + '0';
		svalue /= 10;
	}
	
	/* fill in intermediate zeroes if needed */
	if (buffer[1] != '0') {
		while (bp > buffer + 2)
			*--bp = '0';
		--bp;
	}
	return(bp);
}

/*
 * This program sends string "s" to putchar. The character after
 * the end of "s" is given by "send". This allows the size of the
 * field to be computed; it is stored in "alen". "width" contains the
 * user specified length. If width<alen, the width will be taken to
 * be alen. "sign" is zero if the string is to be right-justified
 * in the field, nonzero if it is to be left-justified. "fill" is
 * 0 if the string is to be padded with '0', positive if it is to be
 * padded with ' ', and negative if an initial '-' should appear before
 * any padding in right-justification (to avoid printing "-3" as
 * "000-3" where "-0003" was intended).
 */
static int 
p_emit(register char *s, char *send)
{
	char cfill;
	register int alen;
	int npad;
	int	cnt = 0;
	int	c, m;
	
	alen = send - s;
	if (alen > width)
		width = alen;
	cfill = fill>0? ' ': '0';
	
	/* we may want to print a leading '-' before anything */
	if (*s == '-' && fill < 0) {
		putchar(*s++);
		cnt++;
		alen--;
		width--;
	}
	npad = width - alen;
	
	/* emit any leading pad characters */
	if (!sign)
		while (--npad >= 0) {
			putchar(cfill);
			cnt++;
		}
			
	/* emit the string itself */
	while (--alen >= 0) {
		nextc(c, s, m);
		s += m;
		putchar(c);
		cnt += m;
		alen -= m-1;
	}
		
	/* emit trailing pad characters */
	if (sign)
		while (--npad >= 0) {
			putchar(cfill);
			cnt++;
		}
	return cnt;
}
