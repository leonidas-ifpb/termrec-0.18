#include "config.h"
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include <langinfo.h>
#include "pty.h"
#include "compat.h"
#include "utils.h"
#include "ttyrec.h"
#include "common.h"
#include "rec_args.h"

static volatile int need_resize;
static int need_utf;
static struct termios ta;
static int ptym;
static int sx, sy;
static int record_f;
static recorder rec;

static void sigwinch(int s)
{
    need_resize=1;
}

static void sigpass(int s)
{
    kill(ptym, s);
}

static void tty_restore(void);
static void sigpipe(int s)
{
    tty_restore();
    fprintf(stderr, "Broken pipe.  Disk full?\n");
    exit(0);
}

static void setsignals(void)
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_RESTART;
    act.sa_handler=sigwinch;
    if (sigaction(SIGWINCH,&act,0))
        die("sigaction SIGWINCH");
    act.sa_handler=sigpass;
    if (sigaction(SIGINT,&act,0))
        die("sigaction SIGINT");
    if (sigaction(SIGHUP,&act,0))
        die("sigaction SIGHUP");
    if (sigaction(SIGQUIT,&act,0))
        die("sigaction SIGQUIT");
    if (sigaction(SIGTERM,&act,0))
        die("sigaction SIGTERM");
    if (sigaction(SIGTSTP,&act,0))
        die("sigaction SIGTSTP");
    act.sa_handler=sigpipe;
    if (sigaction(SIGPIPE,&act,0))
        die("sigaction SIGPIPE");
}

static void tty_get_size(void)
{
    struct winsize ts;

    if (ioctl(1,TIOCGWINSZ,&ts))
        return;
    if (!ts.ws_row || !ts.ws_col)
        return;
    if (ptym!=-1)
        ioctl(ptym,TIOCSWINSZ,&ts);
    sx=ts.ws_col;
    sy=ts.ws_row;
}

static void record_size(void)
{
    struct timeval tv;
    char buf[20], *bp;

    if (raw)
        return;
    bp = buf;
    if (need_utf)
        bp+=sprintf(bp, "\e%%G"), need_utf=0;
    if (sx && sy)
        bp+=sprintf(bp, "\e[8;%d;%dt", sy, sx);
    if (buf==bp)
        return;
    gettimeofday(&tv, 0);
    ttyrec_w_write(rec, &tv, buf, bp-buf);
}

static void tty_raw(void)
{
    struct termios rta;

    memset(&ta, 0, sizeof(ta));
    tcgetattr(0, &ta);

    rta=ta;
    pty_makeraw(&rta);
    tcsetattr(0, TCSADRAIN, &rta);
}

static void tty_restore(void)
{
    tcsetattr(0, TCSADRAIN, &ta);
}

#define BS 65536

int main(int argc, char **argv)
{
    fd_set rfds;
    char buf[BS];
    int r;
    struct timeval tv;

    setlocale(LC_CTYPE, "");
    get_rec_parms(argc, argv);
    record_f=open_out(&record_name, format_ext, append);
    if (record_f==-1)
    {
        fprintf(stderr, "Can't open %s\n", record_name);
        return 1;
    }
    if (!command)
        command=getenv("SHELL");
    if (!command)
        command="/bin/sh";

    ptym=-1;
    sx=sy=0;
    tty_get_size();
    if ((ptym=run(command, sx, sy))==-1)
    {
        fprintf(stderr, "Couldn't allocate pty.\n");
        return 1;
    }

    gettimeofday(&tv, 0);

    rec=ttyrec_w_open(record_f, format, record_name, &tv);
    need_utf=!strcmp(nl_langinfo(CODESET), "UTF-8");
    record_size();

    setsignals();
    tty_raw();

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(ptym, &rfds);
        r=select(ptym+1, &rfds, 0, 0, 0);

        if (need_resize)
        {
            need_resize=0;
            tty_get_size();
            record_size();
        }

        switch (r)
        {
        case 0:
            continue;
        case -1:
            if (errno==EINTR)
                continue;
            tty_restore();
            die("select()");
        }

        if (FD_ISSET(0, &rfds))
        {
            r=read(0, buf, BS);
            if (r<=0)
                break;
            if (write(ptym, buf, r) != r)
                break;
        }
        if (FD_ISSET(ptym, &rfds))
        {
            r=read(ptym, buf, BS);
            if (r<=0)
                break;
            gettimeofday(&tv, 0);
            ttyrec_w_write(rec, &tv, buf, r);
            if (write(1, buf, r) != r)
                break;
        }
    }

    close(ptym);
    int err=!ttyrec_w_close(rec);
    tty_restore();
    wait(0);
    return err;
}
