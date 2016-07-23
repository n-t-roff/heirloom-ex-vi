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
static char sccsid[] UNUSED = "@(#)expreserve.c	1.23 (gritter) 11/27/04";
#endif

/* from expreserve.c	7.13.2 (2.11BSD GTE) 1996/10/26 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <time.h>
#if !defined(HAVE_STRLCPY) || !defined(HAVE_STRLCAT)
# include "compat.h"
#endif

#include "config.h"

#ifdef	LANGMSG
#include <nl_types.h>
#include <locale.h>
nl_catd	catd;
#else
#define	catgets(a, b, c, d)	(d)
#endif

#ifdef	BUFSIZ
#undef	BUFSIZ
#endif
#ifdef	LINE_MAX
#define	BUFSIZ	LINE_MAX	/* POSIX line size */
#else	/* !LINE_MAX */
#ifdef	VMUNIX
#define	BUFSIZ	1024
#else	/* !VMUNIX */
#ifdef	u370
#define	BUFSIZ	4096
#else	/* !u370 */
#define	BUFSIZ	512
#endif	/* !u370 */
#endif
#endif	/* !VMUNIX */

#ifdef	LARGEF
typedef	off_t	bloc;
#else
typedef	short	bloc;
#endif

#ifdef	VMUNIX
#ifdef	LARGEF
typedef	off_t	bbloc;
#else
typedef	int	bbloc;
#endif
#else
typedef	short	bbloc;
#endif

#ifdef	notdef
#define TMP	"/tmp"
#else
#define	TMP	"/var/tmp"
#endif

#ifndef VMUNIX
#define	LBLKS	125
#else
#ifdef	LARGEF
#define	LBLKS	20000
#else
#define	LBLKS	900
#endif
#endif
#ifdef	_POSIX_PATH_MAX
#define	FNSIZE	_POSIX_PATH_MAX
#else
#define	FNSIZE	128
#endif

#ifdef VMUNIX
#define	HBLKS	(1 + (FNSIZE + LBLKS * sizeof(bloc)) / BUFSIZ)
#else
#define HBLKS	1
#endif

char xstr[1];			/* make loader happy */

extern void notify(uid_t, char *, int, time_t);
extern int copyout(char *);
extern void mkdigits(char *);
extern void mknext(char *);

/*
 * Expreserve - preserve a file in /usr/preserve
 * Bill Joy UCB November 13, 1977
 *
 * This routine is very naive - it doesn't remove anything from
 * /usr/preserve... this may mean that we leave
 * stuff there... the danger in doing anything with /usr/preserve
 * is that the clock may be screwed up and we may get confused.
 *
 * We are called in two ways - first from the editor with no argumentss
 * and the standard input open on the temp file. Second with an argument
 * to preserve the entire contents of /tmp (root only).
 *
 * BUG: should do something about preserving Rx... (register contents)
 *      temporaries.
 */

struct 	header {
	time_t	Time;			/* Time temp file last updated */
	uid_t	Uid;
	bbloc	Flines;			/* Number of lines in file */
	char	Savedfile[FNSIZE];	/* The current file name */
	bloc	Blocks[LBLKS];		/* Blocks where line pointers stashed */
} H;

#define	ignore(a)	a
#define	ignorl(a)	a

#define eq(a, b) (strcmp(a, b) == 0)

int 
main(int argc, char **argv)
{
	register DIR *tf;
	struct dirent *dirent;
	struct stat stbuf;

	(void)argv;
#ifdef	LANGMSG
	setlocale(LC_MESSAGES, "");
	catd = catopen(CATNAME, NL_CAT_LOCALE);
#endif
	/*
	 * If only one argument, then preserve the standard input.
	 */
	if (argc == 1) {
		if (copyout((char *) 0))
			exit(1);
		exit(0);
	}

	/*
	 * If not super user, then can only preserve standard input.
	 */
	if (getuid()) {
		fprintf(stderr, catgets(catd, 3, 1, "NOT super user\n"));
		exit(1);
	}

	/*
	 * ... else preserve all the stuff in /tmp, removing
	 * it as we go.
	 */
	if (chdir(TMP) < 0) {
		perror(TMP);
		exit(1);
	}

	tf = opendir(".");
	if (tf == NULL) {
		perror(TMP);
		exit(1);
	}
	while ((dirent = readdir(tf)) != NULL) {
		/* Ex temporaries must begin with Ex. */
		if (dirent->d_name[0] != 'E' || dirent->d_name[1] != 'x')
			continue;
		if (stat(dirent->d_name, &stbuf))
			continue;
		if ((stbuf.st_mode & S_IFMT) != S_IFREG)
			continue;
		/*
		 * Save the bastard.
		 */
		ignore(copyout(dirent->d_name));
	}
	closedir(tf);
	return 0;
}

#ifdef	notdef
char	pattern[] =	"/usr/preserve/Exaa`XXXXX";
#else
char	pattern[] =	PRESERVEDIR "/Exa`XXXXXXXXXX";
#endif

/*
 * Notify user uid that his file fname has been saved.
 */
void
notify(uid_t uid, char *fname, int flag, time_t time)
{
	struct passwd *pp = getpwuid(uid);
	register FILE *mf;
	char	cmd[BUFSIZ];
	struct utsname ut;
	char *hostname;
	char	croak[128];
	char	*timestamp;

	if (pp == NULL)
		return;
	uname(&ut);
	hostname = ut.nodename;
	timestamp = ctime(&time);
	timestamp[16] = 0;	/* blast from seconds on */
	putenv("MAILRC=/dev/null");
	snprintf(cmd, sizeof cmd, "/bin/mail %s", pp->pw_name);
	setuid(getuid());
	mf = popen(cmd, "w");
	if (mf == NULL)
		return;
	setbuf(mf, cmd);
	/*
	 *	flag says how the editor croaked:
	 * "the editor was killed" is perhaps still not an ideal
	 * error message.  Usually, either it was forcably terminated
	 * or the phone was hung up, but we don't know which.
	 */
	snprintf(croak, sizeof croak, flag
		? catgets(catd, 3, 2, "the system went down")
		: catgets(catd, 3, 3, "the editor was killed"));
	if (fname[0] == 0) {
		fname = "LOST";
		fprintf(mf, catgets(catd, 3, 4,
				"Subject: editor saved ``LOST''\n"));
		fprintf(mf, catgets(catd, 3, 5,
				"You were editing a file without a name\n"));
		fprintf(mf, catgets(catd, 3, 6,
			"at <%s> on the machine ``%s'' when %s.\n"),
			timestamp, hostname, croak);
		fprintf(mf, catgets(catd, 3, 7,
		"Since the file had no name, it has been named \"LOST\".\n"));
	} else {
		fprintf(mf, catgets(catd, 3, 8,
				"Subject: editor saved ``%s''\n"), fname);
		fprintf(mf, catgets(catd, 3, 9,
			"You were editing the file \"%s\"\n"), fname);
		fprintf(mf, catgets(catd, 3, 10,
			"at <%s> on the machine ``%s''\n"),
			timestamp, hostname);
		fprintf(mf, catgets(catd, 3, 11, "when %s.\n"), croak);
	}
	fprintf(mf, catgets(catd, 3, 12,
		"\nYou can retrieve most of your changes to this file\n"));
	fprintf(mf, catgets(catd, 3, 13,
			"using the \"recover\" command of the editor.\n"));
	fprintf(mf, catgets(catd, 3, 14,
"An easy way to do this is to give the command \"vi -r %s\".\n"), fname);
	fprintf(mf, catgets(catd, 3, 15,
		"This method also works using \"ex\" and \"edit\".\n"));
	pclose(mf);
}

/*
 * Copy file name into /usr/preserve/...
 * If name is (char *) 0, then do the standard input.
 * We make some checks on the input to make sure it is
 * really an editor temporary, generate a name for the
 * file (this is the slowest thing since we must stat
 * to find a unique name), and finally copy the file.
 */
int
copyout(char *name)
{
	int i;
	char buf[BUFSIZ];
	static int reenter;

	/*
	 * The first time we put in the digits of our
	 * process number at the end of the pattern.
	 */
	if (reenter == 0) {
		mkdigits(pattern);
		reenter++;
	}

	/*
	 * If a file name was given, make it the standard
	 * input if possible.
	 */
	if (name != 0) {
		ignore(close(0));
		/*
		 * Need read/write access for arcane reasons
		 * (see below).
		 */
		if (open(name, O_RDWR) < 0)
			return (-1);
	}

	/*
	 * Get the header block.
	 */
	ignorl(lseek(0, (off_t) 0, SEEK_SET));
	if (read(0, (char *) &H, sizeof H) != sizeof H) {
format:
		if (name == 0)
			fprintf(stderr, catgets(catd, 3, 16,
						"Buffer format error\t"));
		return (-1);
	}

	/*
	 * Consistency checsks so we don't copy out garbage.
	 */
	if (H.Flines < 0) {
#ifdef DEBUG
		fprintf(stderr, "Negative number of lines\n");
#endif
		goto format;
	}
	if (H.Blocks[0] != HBLKS || H.Blocks[1] != HBLKS+1) {
#ifdef DEBUG
		fprintf(stderr, "Blocks %d %d\n", H.Blocks[0], H.Blocks[1]);
#endif
		goto format;
	}
	if (name == 0 && H.Uid != getuid()) {
#ifdef DEBUG
		fprintf(stderr, "Wrong user-id\n");
#endif
		goto format;
	}
	if (lseek(0, (off_t) 0, SEEK_SET)) {
#ifdef DEBUG
		fprintf(stderr, "Negative number of lines\n");
#endif
		goto format;
	}

	/*
	 * If no name was assigned to the file, then give it the name
	 * LOST, by putting this in the header.
	 */
	if (H.Savedfile[0] == 0) {
		memcpy(H.Savedfile, "LOST", 5);
		ignore(write(0, (char *) &H, sizeof H));
		H.Savedfile[0] = 0;
		lseek(0, (off_t) 0, SEEK_SET);
	}

	/*
	 * File is good.  Get a name and create a file for the copy.
	 */
	mknext(pattern);
	ignore(close(1));
	if (open(pattern, O_CREAT|O_EXCL|O_WRONLY|O_TRUNC
#ifdef	O_NOFOLLOW
				|O_NOFOLLOW
#endif	/* O_NOFOLLOW */
				, 0600) < 0) {
		if (name == 0)
			perror(pattern);
		return (1);
	}

	/*
	 * Make the target be owned by the owner of the file.
	 */
	ignore(chown(pattern, H.Uid, 0));

	/*
	 * Copy the file.
	 */
	for (;;) {
		i = read(0, buf, BUFSIZ);
		if (i < 0) {
			if (name)
				perror(catgets(catd, 3, 17,
						"Buffer read error"));
			ignore(unlink(pattern));
			return (-1);
		}
		if (i == 0) {
			if (name)
				ignore(unlink(name));
			notify(H.Uid, H.Savedfile, name != 0, H.Time);
			return (0);
		}
		if (write(1, buf, i) != i) {
			if (name == 0)
				perror(pattern);
			unlink(pattern);
			return (-1);
		}
	}
}

/*
 * Blast the last 5 characters of cp to be the process number.
 */
void
mkdigits(char *cp)
{
	register pid_t i;
	register int j;

#ifdef	notdef
	for (i = getpid(), j = 5, cp += strlen(cp); j > 0; i /= 10, j--)
		*--cp = i % 10 | '0';
#else
	for (i = getpid(), j = 10, cp += strlen(cp); j > 0; i /= 10, j--)
		*--cp = i % 10 | '0';
#endif
}

/*
 * Make the name in cp be unique by clobbering up to
 * three alphabetic characters into a sequence of the form 'aab', 'aac', etc.
 * Mktemp gets weird names too quickly to be useful here.
 */
void
mknext(char *cp)
{
	char *dcp;
	struct stat stb;

	dcp = cp + strlen(cp) - 1;
	while (isdigit(*dcp & 0377))
		dcp--;
whoops:
	if (dcp[0] == 'z') {
		dcp[0] = 'a';
		if (dcp[-1] == 'z') {
#ifdef	notdef
			dcp[-1] = 'a';
			if (dcp[-2] == 'z')
#endif
				fprintf(stderr, catgets(catd, 3, 18,
						"Can't find a name\t"));
#ifdef	notdef
			dcp[-2]++;
#endif
		} else
			dcp[-1]++;
	} else
		dcp[0]++;
	if (stat(cp, &stb) == 0)
		goto whoops;
}

/*
 *	people making love
 *	never exactly the same
 *	just like a snowflake 
 */

#ifdef lint
void
Ignore(int a)
{

	a = a;
}

void
Ignorl(long a)
{

	a = a;
}
#endif
