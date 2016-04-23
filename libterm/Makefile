#
# This code contains changes by
#       Gunnar Ritter, Freiburg i. Br., Germany, 2002. All rights reserved.
#
# Conditions 1, 2, and 4 and the no-warranty notice below apply
# to these changes.
#
#
# Copyright (c) 1980, 1993
#	The Regents of the University of California.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#	from Makefile	5.1 (Berkeley) 6/5/85
#
#	Sccsid @(#)Makefile	1.2 (gritter) 2/4/02
#

LIB	= libtermlib.a
DEFS	= -DCM_N -DCM_GT -DCM_B -DCM_D
SRCS	= termcap.c tgoto.c tputs.c
OBJS	= $(SRCS:.c=.o)

.c.o:
	# COPT comes from ex
	$(CC) $(CFLAGS) $(DEFS) $(COPT) -c $<

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) crs $@ $(OBJS)

clean:
	rm -f $(LIB) $(OBJS) core

# DO NOT DELETE

termcap.o: libterm.h
tgoto.o: libterm.h
tputs.o: libterm.h
