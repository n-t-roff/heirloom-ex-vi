/*
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
 *
 *	Sccsid @(#)ex_proto.h	1.33 (gritter) 8/6/05
 */

/*
 * Function type definitions
 */

#define	join	xjoin
#define	word	xword
#define	getline	xgetline

/* ex.c */
extern void erropen(void);
extern void usage(void);
extern void needarg(int);
extern void invopt(int);
extern char *tailpath(register char *);
extern int iownit(char *);
extern shand setsig(int, shand);
extern void init(void);
/* ex_addr.c */
extern void setdot(void);
extern void setdot1(void);
extern void setcount(void);
extern int getnum(void);
extern void setall(void);
extern void setnoaddr(void);
extern line *address(char *);
extern void setCNL(void);
extern void setNAEOL(void);
/* ex_cmds.c */
extern void commands(int, int);
/* ex_cmds2.c */
extern int cmdreg(void);
extern int endcmd(int);
extern void eol(void);
extern void error0(void);
extern int error1(char *);
extern void verror(char *, va_list);
extern void error(char *, ...);
extern void erewind(void);
extern void fixol(void);
extern int exclam(void);
extern void makargs(void);
extern void next(void);
extern void newline(void);
extern void nomore(void);
extern int quickly(void);
extern void resetflav(void);
extern void serror(char *, ...);
extern void setflav(void);
extern int skipend(void);
extern void tailspec(int);
extern void tail(char *);
extern void tail2of(char *);
extern void tailprim(register char *, int, bool);
extern void vcontin(bool);
extern void vnfl(void);
/* ex_cmdsub.c */
extern int append(int (*)(void), line *);
extern void appendnone(void);
extern void pargs(void);
extern void delete(int);
extern void deletenone(void);
extern void squish(void);
extern void join(int);
extern void move1(int, line *);
extern void move(void);
extern int getcopy(void);
extern int getput(void);
extern void put(int);
extern void pragged(int);
extern void shift(int, int);
extern void tagfind(bool);
extern void yank(int);
extern void zop(int);
extern void zop2(register int, register int);
extern void plines(line *, register line *, bool);
extern void pofix(void);
extern void somechange(void);
extern void undo(bool);
extern void mapcmd(int, int);
extern void cmdmac(char);
/* ex_data.c */
/* ex_extern.c */
/* ex_get.c */
extern void ignchar(void);
extern int getach(void);
extern int getchar(void);
extern void checkjunk(int);
extern int getcd(void);
extern int peekchar(void);
extern int peekcd(void);
extern int smunch(register int, char *);
extern int gettty(void);
extern void setin(line *);
/* ex_io.c */
extern void filename(int);
extern int getargs(void);
extern int gscan(void);
extern void getone(void);
extern int samei(struct stat *, char *);
extern void rop(int);
extern void rop2(void);
extern int iostats(void);
extern void rop3(int);
extern int edfile(void);
extern void wop(bool);
extern int getfile(void);
extern void putfile(int);
extern void wrerror(void);
extern void source(char *, bool);
extern void clrstats(void);
extern void checkmodeline(char *);
#ifdef	MB
extern int mbtowi(int *c, const char *s, size_t n);
extern int widthok(int c);
#endif	/* MB */
extern int GETWC(char *);
/* ex_put.c */
extern int (*setlist(int))(int);
extern int (*setnumb(int))(int, int);
extern int listchar(int);
extern int normchar(register int);
extern void slobber(int);
extern int numbline(int, int);
extern int normline(int, int);
extern int putchar(int);
extern int termchar(int);
extern void flush2(void);
extern void flush(void);
extern void flush1(void);
extern int plodput(int);
extern int plod(int);
extern void fgoto(void);
extern void tab(int);
extern void noteinp(void);
extern void termreset(void);
extern void draino(void);
extern void flusho(void);
extern void putnl(void);
extern void putS(char *);
extern int putch(int);
extern void putpad(char *);
extern void setoutt(void);
extern void vlprintf(char *, va_list);
extern void lprintf(char *, ...);
extern void putNFL(void);
extern void sTTY(int);
extern void pstart(void);
extern void pstop(void);
extern void ttcharoff(void);
extern struct termios ostart(void);
extern void normal(struct termios);
/* ex_re.c */
extern void global(int);
extern void gdelete(void);
extern int substitute(int);
extern int compsub(int);
extern void comprhs(int);
extern int getsub(void);
extern int dosubcon(bool, line *);
extern int confirmed(line *);
extern void ugo(int, int);
extern void dosub(void);
extern int fixcase(register int);
extern char *place(register char *, register char *, register char *);
extern void snote(register int, register int);
extern void cerror(char *);
extern struct regexp *savere(struct regexp *);
extern struct regexp *resre(struct regexp *);
extern int compile(int, int);
extern int same(register int, register int);
extern int ecmp(register char *, register char *, register int);
extern int execute(int, line *);
/* ex_set.c */
extern void set(void);
extern int setend(void);
extern void prall(void);
extern void propts(void);
extern void propt(register struct option *);
/* ex_subr.c */
extern int any(int, register char *);
extern int backtab(register int);
extern void change(void);
extern int column(register char *);
extern int lcolumn(register char *);
extern void comment(void);
size_t lcpy(char *, const char *, size_t);
size_t lcat(char *, const char *, size_t);
extern void copyw(register line *, register line *, register int);
extern void copywR(register line *, register line *, register int);
extern int ctlof(int);
extern void dingdong(void);
extern int fixindent(int);
extern void filioerr(char *);
extern char *genindent(register int);
extern void getDOT(void);
extern line *getmark(register int);
extern int getn(register char *);
extern void ignnEOF(void);
extern int is_white(int);
extern int junk(register int);
extern void killed(void);
extern void killcnt(register int);
extern int lineno(line *);
extern int lineDOL(void);
extern int lineDOT(void);
extern void markDOT(void);
extern void markpr(line *);
extern int markreg(register int);
extern char *mesg(register char *);
extern void merror1(intptr_t);
extern void vmerror(char *, va_list);
extern void merror(char *, ...);
extern int morelines(void);
extern void nonzero(void);
extern int notable(int);
extern void notempty(void);
extern void netchHAD(int);
extern void netchange(register int);
extern int printof(int);
extern void putmark(line *);
extern void putmk1(register line *, int);
extern char *plural(long);
extern int qcolumn(register char *, register char *);
extern int qcount(int);
extern void reverse(register line *, register line *);
extern void save(line *, register line *);
extern void save12(void);
extern void saveall(void);
extern int span(void);
extern void synced(void);
extern int skipwh(void);
extern void vsmerror(char *, va_list);
extern void smerror(char *, ...);
extern char *strend(register char *);
extern void strcLIN(char *);
extern void syserror(void);
extern int tabcol(int, int);
extern char *vfindcol(int);
extern char *vskipwh(register char *);
extern char *vpastwh(register char *);
extern int whitecnt(register char *);
extern void markit(line *);
extern void onhup(int);
extern void onintr(int);
extern void setrupt(void);
extern int preserve(void);
extern int exitex(int);
extern void onsusp(int);
extern void onemt(int);
extern char *safecp(char *, const char *, size_t, char *, ...);
extern char *safecat(char *, const char *, size_t, char *, ...);
extern void grow(char *, char **, char **, char **, char **);
extern void *smalloc(size_t);
/* ex_tagio.c */
extern int topen(char *, char *);
extern int tseek(int, off_t);
extern int tgets(char *, int, int);
extern void tclose(int);
/* ex_temp.c */
extern void fileinit(void);
extern void cleanup(bool);
extern void getline(line);
extern line putline(void);
extern char *getblock(line, int);
extern void blkio(bloc, char *, ssize_t (*)(int, void *, size_t));
extern void tlaste(void);
extern void tflush(void);
extern void synctmp(void);
extern void TSYNC(void);
extern void regio(short, ssize_t (*)(int, void *, size_t));
extern int REGblk(void);
extern struct strreg *mapreg(register int);
extern void KILLreg(register int);
extern ssize_t shread(void);
extern void putreg(int);
extern int partreg(int);
extern void notpart(register int);
extern int getREG(void);
extern void YANKreg(register int);
extern void kshift(void);
extern void YANKline(void);
extern void rbflush(void);
extern void regbuf(char, char *, int);
extern void tlaste(void);
/* ex_tty.c */
extern void gettmode(void);
extern void setterm(char *);
extern void setsize(void);
extern void zap(void);
extern char *gettlongname(register char *, char *);
extern char *fkey(int);
extern int cost(char *);
extern int countnum(int);
extern struct termios ostart(void);
extern void tostart(void);
extern void ostop(struct termios);
extern void tostop(void);
extern struct termios setty(struct termios);
extern void gTTY(int);
extern void noonl(void);
/* ex_unix.c */
extern void unix0(int);
extern struct termios unixex(char *, char *, int, int);
extern void unixwt(int, struct termios);
extern void filter(register int);
extern void recover(void);
extern void waitfor(void);
extern void revocer(void);
/* ex_v.c */
extern void oop(void);
extern void ovbeg(void);
extern void ovend(struct termios);
extern void vop(void);
extern void fixzero(void);
extern void savevis(void);
extern void undvis(void);
extern void setwind(void);
extern void vok(register cell *);
extern void vintr(int);
extern void vsetsiz(int);
extern void onwinch(int);
/* ex_vadj.c */
extern void vopen(line *, int);
extern int vreopen(int, int, int);
extern int vglitchup(int, int);
extern void vinslin(register int, register int, int);
extern void vopenup(int, int, int);
extern void vadjAL(int, int);
extern void vrollup(int);
extern void vup1(void);
extern void vmoveitup(register int, int);
extern void vscroll(register int);
extern void vscrap(void);
extern void vrepaint(char *);
extern void vredraw(register int);
extern void vdellin(int, int, int);
extern void vadjDL(int, int);
extern void vsyncCL(void);
extern void vsync(register int);
extern void vsync1(register int);
extern void vcloseup(int, register int);
extern void vreplace(int, int, int);
extern void sethard(void);
extern void vdirty(register int, register int);
/* ex_version.c */
extern void printver(void);
/* ex_vget.c */
extern void ungetkey(int);
extern int getkey(void);
extern int peekbr(void);
extern int getbr(void);
extern int getesc(void);
extern int peekkey(void);
extern int readecho(int);
extern void setLAST(void);
extern void addtext(char *);
extern void setDEL(void);
extern void setBUF(register cell *);
extern void addto(register cell *, register char *);
extern int noteit(int);
extern void obeep(void);
extern void macpush(char *, int);
extern int vgetcnt(void);
extern void trapalarm(int);
extern int fastpeekkey(void);
/* ex_vmain.c */
extern void vmain(void);
extern void grabtag(void);
extern void prepapp(void);
extern void vremote(int, void (*)(int), int);
extern void vsave(void);
extern void vzop(int, int, register int);
extern cell *str2cell(cell *, register char *);
extern char *cell2str(char *, register cell *);
extern cell *cellcpy(cell *, register cell *);
extern size_t cellen(register cell *);
extern cell *cellcat(cell *, register cell *);
/* ex_voper.c */
extern void operate(register int, register int);
extern int find(int);
extern int word(register void (*)(int), int);
extern void eend(register void (*)(int));
extern int wordof(int, register char *);
extern int wordch(char *);
extern int edge(void);
extern int margin(void);
/* ex_vops.c */
extern void vUndo(void);
extern void vundo(int);
extern void vmacchng(int);
extern void vnoapp(void);
extern void vmove(int);
extern void vdelete(int);
extern void vchange(int);
extern void voOpen(int, register int);
extern void vshftop(int);
extern void vfilter(int);
extern int xdw(void);
extern void vshift(int);
extern void vrep(register int);
extern void vyankit(int);
extern void setpk(void);
extern void vkillDEL(void);
/* ex_vops2.c */
extern void bleep(register int, char *);
extern int vdcMID(void);
extern void takeout(cell *);
extern int ateopr(void);
extern void showmode(int);
extern void addc(cell);
extern void vappend(int, int, int);
extern void back1(void);
extern char *vgetline(int, register char *, bool *, int);
extern void vdoappend(char *);
extern int vgetsplit(void);
extern int vmaxrep(int, register int);
/* ex_vops3.c */
extern int llfind(bool, int, void (*)(int), line *);
extern int endsent(bool);
extern int endPS(void);
extern int lindent(line *);
extern int lmatchp(line *);
extern void lsmatch(char *);
extern int ltosolid(void);
extern int ltosol1(register char *);
extern int lskipbal(register char *);
extern int lskipatom(void);
extern int lskipa1(register char *);
extern int lnext(void);
extern int lbrack(register int, void (*)(int));
extern int isa(register char *);
extern void vswitch(int);
#ifdef	MB
extern int	wskipleft(char *, char *);
extern int	wskipright(char *, char *);
extern int	wsamechar(char *, int);
extern int	xwcwidth(wint_t);
#endif	/* MB */
/* ex_vput.c */
extern void vclear(void);
extern void vclrcell(register cell *, register int);
extern void vclrlin(int, line *);
extern void vclreol(void);
extern void vclrech(bool);
extern void fixech(void);
extern void vcursbef(register char *);
extern void vcursat(register char *);
extern void vcursaft(register char *);
extern void vfixcurs(void);
extern void vsetcurs(register char *);
extern void vigoto(int, int);
extern void vcsync(void);
extern void vgotoCL(register int);
extern void vigotoCL(register int);
extern void vgoto(register int, register int);
extern void vgotab(void);
extern void vprepins(void);
extern void vmaktop(register int, cell *);
extern int vinschar(int);
extern void vrigid(void);
extern void vneedpos(int);
extern void vnpins(int);
extern void vishft(void);
extern void viin(int);
extern void godm(void);
extern void enddm(void);
extern void goim(void);
extern void endim(void);
extern int vputchar(register int);
extern void physdc(int, int);
extern int vputch(int);
/* ex_vwind.c */
extern void vmoveto(register line *, char *, int);
extern void vjumpto(register line *, char *, int);
extern void vupdown(register int, char *);
extern void vup(register int, register int, int);
extern void vdown(register int, register int, int);
extern void vcontext(register line *, int);
extern void vclean(void);
extern void vshow(line *, line *);
extern void vreset(int);
extern line *vback(register line *, register int);
extern int vfit(register line *, int);
extern void vroll(register int);
extern void vrollR(register int);
extern int vcookit(register int);
extern int vdepth(void);
extern void vnline(char *);
/* malloc.c */
/* mapmalloc.c */
extern char *poolsbrk(intptr_t);
/* printf.c */
extern int ex_printf(char *, ...);
extern int vprintf(const char *, va_list);
extern char *p_dconv(long, char *);
