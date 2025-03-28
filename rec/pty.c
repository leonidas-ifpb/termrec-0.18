#define _GNU_SOURCE
#include "pty.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#ifdef HAVE_PTY_H
# include <pty.h>
#endif
#if defined(HAVE_FORKPTY) && defined(HAVE_LIBUTIL_H)
# include <libutil.h>
#endif

extern char **environ;


#ifndef HAVE_FORKPTY
# if !(defined(HAVE__GETPTY) || defined(HAVE_GRANTPT) \
    && (defined(HAVE_GETPT) || defined(HAVE_DEV_PTMX) \
    || defined(HAVE_POSIX_OPENPT)))
// if no PTYRANGE[01] is in the config file, we pick a default
#  ifndef PTYRANGE0
#   define PTYRANGE0 "qpr"
#  endif
#  ifndef PTYRANGE1
#   define PTYRANGE1 "0123456789abcdef"
#  endif
#  ifdef M_UNIX
static char PtyProto[] = "/dev/ptypXY";
static char TtyProto[] = "/dev/ttypXY";
#  else
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#  endif
# endif


int forkpty(int *amaster,char *dummy,struct termios *termp, struct winsize *wp)
{
    int master,slave;
    int pid;

#ifdef HAVE__GETPTY
    int filedes[2];
    char *line;

    line = _getpty(&filedes[0], O_RDWR|O_NDELAY, 0600, 0);
    if (0 == line)
        return -1;
    if (0 > (filedes[1] = open(line, O_RDWR)))
    {
        close(filedes[0]);
        return -1;
    }
    master=filedes[0];
    slave=filedes[1];
#elif defined(HAVE_GRANTPT) && (defined(HAVE_GETPT) || defined(HAVE_DEV_PTMX) || defined(HAVE_POSIX_OPENPT))
# ifdef HAVE_PTSNAME
    char *name;
# else
    char name[80];
# endif

# ifdef HAVE_GETPT
    master=getpt();
# elif HAVE_POSIX_OPENPT
    master=posix_openpt(O_RDWR);
# else
    master=open("/dev/ptmx", O_RDWR);
# endif

    if (master<0)
        return -1;

    if (grantpt(master)<0||unlockpt(master)<0)
        goto close_master;

# ifdef HAVE_PTSNAME
    if (!(name=ptsname(master)))
        goto close_master;
# else
    if (ptsname_r(master,name,80))
        goto close_master;
# endif

    slave=open(name,O_RDWR);
    if (slave==-1)
        goto close_master;

# ifdef HAVE_STROPTS_H
    if (isastream(slave))
        if (ioctl(slave, I_PUSH, "ptem")<0
                ||ioctl(slave, I_PUSH, "ldterm")<0)
            goto close_slave;
# endif

    goto ok;

//close_slave:
    close (slave);

close_master:
    close (master);
    return -1;

ok:
#else
  char *p, *q, *l, *d;
  char PtyName[32], TtyName[32];

  strcpy(PtyName, PtyProto);
  strcpy(TtyName, TtyProto);
  for (p = PtyName; *p != 'X'; p++)
    ;
  for (q = TtyName; *q != 'X'; q++)
    ;
  for (l = PTYRANGE0; (*p = *l) != '\0'; l++)
    {
      for (d = PTYRANGE1; (p[1] = *d) != '\0'; d++)
        {
//        tintin_printf(0,"OpenPTY tries '%s'", PtyName);
          if ((master = open(PtyName, O_RDWR | O_NOCTTY)) == -1)
            continue;
          q[0] = *l;
          q[1] = *d;
          if (access(TtyName, R_OK | W_OK))
            {
              close(master);
              continue;
            }
          if ((slave=open(TtyName, O_RDWR|O_NOCTTY))==-1)
          {
                close(master);
                continue;
          }
          goto ok;
        }
    }
  return -1;
  ok:
#endif

    if (termp)
        tcsetattr(master, TCSANOW, termp);
    if (wp)
        ioctl(master,TIOCSWINSZ,wp);
    // let's ignore errors on this ioctl silently

    pid=fork();
    switch (pid)
    {
    case -1:
        close(master);
        close(slave);
        return -1;
    case 0:
        close(master);
        setsid();
        dup2(slave,0);
        dup2(slave,1);
        dup2(slave,2);
        close(slave);
        return 0;
    default:
        close(slave);
        *amaster=master;
        return pid;
    }
}
#endif

void pty_makeraw(struct termios *ta)
{
    memset(ta, 0, sizeof(*ta));
    ta->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                    |INLCR|IGNCR|ICRNL|IXON);
    ta->c_oflag &= ~OPOST;
    ta->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    ta->c_cflag &= ~(CSIZE|PARENB);
    ta->c_cflag |= CS8;

    ta->c_cc[VMIN]=1;
    ta->c_cc[VTIME]=0;
}

int run(const char *command, int sx, int sy)
{
    int fd;

    struct winsize ws;

    ws.ws_row=sy;
    ws.ws_col=sx;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    switch (forkpty(&fd,0,0,(sx&&sy)?&ws:0))
    {
    case -1:
        return -1;
    case 0:
        {
            const char *argv[4];
            char *cmd;

            if (asprintf(&cmd, "exec %s", command) == -1)
                abort();
            argv[0]="sh";
            argv[1]="-c";
            argv[2]=cmd;
            argv[3]=0;
            execve("/bin/sh",(char*const*)argv,environ);
            fprintf(stderr,"#ERROR: Couldn't exec `%s'\n",command);
            exit(127);
        }
    default:
        return fd;
    }

    return -1;
}
