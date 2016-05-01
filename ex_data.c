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
static char sccsid[] = "@(#)ex_data.c	1.14 (gritter) 11/23/04";
#endif
#endif

/* from ex_data.c	7.5 (Berkeley) 8/29/85 */

#include "ex.h"
#include "ex_tty.h"

/*
 * Initialization of option values.
 * The option #defines in ex_vars.h are made
 * from this file by the script makeoptions.
 *
 * These initializations are done char by char instead of as strings
 * to confuse xstr so it will leave them alone.
 */
#ifdef	notdef
char	direct[ONMSZ] =
	{'/', 't', 'm', 'p'}; 
#else
char	direct[ONMSZ] =
	{'/', 'v', 'a', 'r', '/', 't', 'm', 'p' }; 
#endif
char	paragraphs[ONMSZ] = {
	'I', 'P', 'L', 'P', 'P', 'P', 'Q', 'P',		/* -ms macros */
	'P', ' ', 'L', 'I',				/* -mm macros */
	'p', 'p', 'l', 'p', 'i', 'p',			/* -me macros */
	'b', 'p'					/* bare nroff */
};
char	sections[ONMSZ] = {
	'N', 'H', 'S', 'H',				/* -ms macros */
	'H', ' ', 'H', 'U',				/* -mm macros */
	'n', 'h', 's', 'h'				/* -me macros */
};
char	shell[ONMSZ] =
	{ '/', 'b', 'i', 'n', '/', 's', 'h' };
char	tags[ONMSZ] = {
	't', 'a', 'g', 's', ' ',
	'/', 'u', 's', 'r', '/', 'l', 'i', 'b', '/', 't', 'a', 'g', 's'
};
char	ttylongname[ONMSZ] =
	{ 'd', 'u', 'm', 'b' };

short	TCOLUMNS = 80;
short	TLINES = 24;

struct	option options[NOPTS + 1] = {
	{ "autoindent",	"ai",	ONOFF,		0,	0,	0 },
	{ "autoprint",	"ap",	ONOFF,		1,	1,	0 },
	{ "autowrite",	"aw",	ONOFF,		0,	0,	0 },
	{ "beautify",	"bf",	ONOFF,		0,	0,	0 },
	{ "directory",	"dir",	STRING,		0,	0,	direct },
	{ "edcompatible","ed",	ONOFF,		0,	0,	0 },
	{ "errorbells",	"eb",	ONOFF,		0,	0,	0 },
	{ "exrc",	"ex",	ONOFF,		0,	0,	0 },
	{ "flash",	"fl",	ONOFF,		1,	1,	0 },
	{ "hardtabs",	"ht",	NUMERIC,	8,	8,	0 },
	{ "ignorecase",	"ic",	ONOFF,		0,	0,	0 },
	{ "lisp",	0,	ONOFF,		0,	0,	0 },
	{ "list",	0,	ONOFF,		0,	0,	0 },
	{ "magic",	0,	ONOFF,		1,	1,	0 },
	{ "mesg",	0,	ONOFF,		1,	1,	0 },
	{ "modelines",	"ml",	ONOFF,		0,	0,	0 },
	{ "number",	"nu",	ONOFF,		0,	0,	0 },
	{ "open",	0,	ONOFF,		1,	1,	0 },
	{ "optimize",	"opt",	ONOFF,		0,	0,	0 },
	{ "paragraphs",	"para",	STRING,		0,	0,	paragraphs },
	{ "prompt",	0,	ONOFF,		1,	1,	0 },
	{ "readonly",	"ro",	ONOFF,		0,	0,	0 },
	{ "redraw",	0,	ONOFF,		0,	0,	0 },
	{ "remap",	0,	ONOFF,		1,	1,	0 },
	{ "report",	0,	NUMERIC,	5,	5,	0 },
	{ "scroll",	"scr",	NUMERIC,	12,	12,	0 },
	{ "sections",	"sect",	STRING,		0,	0,	sections },
	{ "shell",	"sh",	STRING,		0,	0,	shell },
	{ "shiftwidth",	"sw",	NUMERIC,	TABS,	TABS,	0 },
	{ "showmatch",	"sm",	ONOFF,		0,	0,	0 },
	{ "showmode",	"smd",	ONOFF,		0,	0,	0 },
	{ "slowopen",	"slow",	ONOFF,		0,	0,	0 },
	{ "sourceany",	0,	ONOFF,		0,	0,	0 },
	{ "tabstop",	"ts",	NUMERIC,	TABS,	TABS,	0 },
	{ "taglength",	"tl",	NUMERIC,	0,	0,	0 },
	{ "tags",	"tag",	STRING,		0,	0,	tags },
	{ "term",	0,	OTERM,		0,	0,	ttylongname },
	{ "terse",	0,	ONOFF,		0,	0,	0 },
	{ "timeout",	"to",	ONOFF,		1,	1,	0 },
	{ "ttytype",	"tty",	OTERM,		0,	0,	ttylongname },
	{ "warn",	0,	ONOFF,		1,	1,	0 },
	{ "window",	"wi",	NUMERIC,	23,	23,	0 },
	{ "wrapscan",	"ws",	ONOFF,		1,	1,	0 },
	{ "wrapmargin",	"wm",	NUMERIC,	0,	0,	0 },
	{ "writeany",	"wa",	ONOFF,		0,	0,	0 },
	{ 0,		0,	0,		0,	0,	0 }
};
