/* ====================================================================
 * Copyright (c) 1996-1998 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef APACHE_BUFF_H
#define APACHE_BUFF_H

#ifdef B_SFIO
#include "sfio.h"
#endif

#include <stdarg.h>

/* Reading is buffered */
#define B_RD     (1)
/* Writing is buffered */
#define B_WR     (2)
#define B_RDWR   (3)
/* At end of file, or closed stream; no further input allowed */
#define B_EOF    (4)
/* No further output possible */
#define B_EOUT   (8)
/* A read error has occurred */
#define B_RDERR (16)
/* A write error has occurred */
#define B_WRERR (32)
#ifdef B_ERROR  /* in SVR4: sometimes defined in /usr/include/sys/buf.h */
#undef B_ERROR
#endif
#define B_ERROR (48)
/* Use chunked writing */
#define B_CHUNK (64)
/* bflush() if a read would block */
#define B_SAFEREAD (128)
/* buffer is a socket */
#define B_SOCKET (256)
#ifdef CHARSET_EBCDIC
#define B_ASCII2EBCDIC 0x40000000  /* Enable conversion for this buffer */
#define B_EBCDIC2ASCII 0x80000000  /* Enable conversion for this buffer */
#endif /*CHARSET_EBCDIC*/

typedef struct buff_struct BUFF;

struct buff_struct {
    int flags;			/* flags */
    unsigned char *inptr;	/* pointer to next location to read */
    int incnt;			/* number of bytes left to read from input buffer;
				 * always 0 if had a read error  */
    int outchunk;		/* location of chunk header when chunking */
    int outcnt;			/* number of byte put in output buffer */
    unsigned char *inbase;
    unsigned char *outbase;
    int bufsiz;
    void (*error) (BUFF *fb, int op, void *data);
    void *error_data;
    long int bytes_sent;	/* number of bytes actually written */

    pool *pool;

/* could also put pointers to the basic I/O routines here */
    int fd;			/* the file descriptor */
    int fd_in;			/* input file descriptor, if different */

    /* transport handle, for RPC binding handle or some such */
    void *t_handle;

#ifdef B_SFIO
    Sfio_t *sf_in;
    Sfio_t *sf_out;
#endif
};

#ifdef B_SFIO
typedef struct {
    Sfdisc_t disc;
    BUFF *buff;
} apache_sfio;

extern Sfdisc_t *bsfio_new(pool *p, BUFF *b);
#endif

/* Options to bset/getopt */
#define BO_BYTECT (1)

/* Stream creation and modification */
API_EXPORT(BUFF *) bcreate(pool *p, int flags);
API_EXPORT(void) bpushfd(BUFF *fb, int fd_in, int fd_out);
API_EXPORT(int) bsetopt(BUFF *fb, int optname, const void *optval);
API_EXPORT(int) bgetopt(BUFF *fb, int optname, void *optval);
API_EXPORT(int) bsetflag(BUFF *fb, int flag, int value);
API_EXPORT(int) bclose(BUFF *fb);

#define bgetflag(fb, flag)	((fb)->flags & (flag))

/* Error handling */
API_EXPORT(void) bonerror(BUFF *fb, void (*error) (BUFF *, int, void *),
			  void *data);

/* I/O */
API_EXPORT(int) bread(BUFF *fb, void *buf, int nbyte);
API_EXPORT(int) bgets(char *s, int n, BUFF *fb);
API_EXPORT(int) blookc(char *buff, BUFF *fb);
API_EXPORT(int) bskiplf(BUFF *fb);
API_EXPORT(int) bwrite(BUFF *fb, const void *buf, int nbyte);
API_EXPORT(int) bflush(BUFF *fb);
API_EXPORT(int) bputs(const char *x, BUFF *fb);
API_EXPORT(int) bvputs(BUFF *fb,...);
API_EXPORT_NONSTD(int) bprintf(BUFF *fb, const char *fmt,...)
				__attribute__((format(printf,2,3)));
API_EXPORT_NONSTD(int) vbprintf(BUFF *fb, const char *fmt, va_list vlist);

/* Internal routines */
API_EXPORT(int) bflsbuf(int c, BUFF *fb);
API_EXPORT(int) bfilbuf(BUFF *fb);

#ifndef CHARSET_EBCDIC

#define bgetc(fb)   ( ((fb)->incnt == 0) ? bfilbuf(fb) : \
		    ((fb)->incnt--, *((fb)->inptr++)) )

#define bputc(c, fb) ((((fb)->flags & (B_EOUT|B_WRERR|B_WR)) != B_WR || \
		     (fb)->outcnt == (fb)->bufsiz) ? bflsbuf(c, (fb)) : \
		     ((fb)->outbase[(fb)->outcnt++] = (c), 0))

#else /*CHARSET_EBCDIC*/

#define bgetc(fb)   ( ((fb)->incnt == 0) ? bfilbuf(fb) : \
		    ((fb)->incnt--, (fb->flags & B_ASCII2EBCDIC)\
		    ?os_toebcdic[(unsigned char)*((fb)->inptr++)]:*((fb)->inptr++)) )

#define bputc(c, fb) ((((fb)->flags & (B_EOUT|B_WRERR|B_WR)) != B_WR || \
		     (fb)->outcnt == (fb)->bufsiz) ? bflsbuf(c, (fb)) : \
		     ((fb)->outbase[(fb)->outcnt++] = (fb->flags & B_EBCDIC2ASCII)\
		     ?os_toascii[(unsigned char)c]:(c), 0))

#endif /*CHARSET_EBCDIC*/
API_EXPORT(int) spawn_child_err_buff(pool *, int (*)(void *), void *,
		      enum kill_conditions, BUFF **pipe_in, BUFF **pipe_out,
				     BUFF **pipe_err);

/* enable non-blocking operations */
API_EXPORT(int) bnonblock(BUFF *fb, int direction);
/* and get an fd to select() on */
API_EXPORT(int) bfileno(BUFF *fb, int direction);

/* bflush() if a read now would block, but don't actually read anything */
API_EXPORT(void) bhalfduplex(BUFF *fb);

#endif	/* !APACHE_BUFF_H */