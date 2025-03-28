#ifndef _TTYREC_H
#define _TTYREC_H
#include <sys/time.h>
#include <tty.h>

#ifndef _TTYREC_H_NO_TYPES
typedef struct TTYRecRecorder *recorder;
typedef struct TTYRec *ttyrec;
typedef struct
{
    struct timeval t;
    int len;
    char *data;
} *ttyrec_frame;
#endif

#define SM_READ         0
#define SM_WRITE        1
#define SM_REPREAD      2
#define SM_APPEND       3

int             open_stream(int fd, const char* url, int mode, const char **error);

const char*     ttyrec_w_find_format(const char *format, const char *filename,
                    const char *fallback);
recorder        ttyrec_w_open(int fd, const char *format, const char *filename,
                    const struct timeval *ts);
int             ttyrec_w_write(recorder r, const struct timeval *tm,
                    const char *data, int len);
int             ttyrec_w_close(recorder r);
const char*     ttyrec_w_get_format_name(int i);
const char*     ttyrec_w_get_format_ext(const char *format);

const char*     ttyrec_r_find_format(const char *format, const char *filename,
                    const char *fallback);
const char*     ttyrec_r_get_format_name(int i);
const char*     ttyrec_r_get_format_ext(const char *format);
int             ttyrec_r_play(int fd, const char *format, const char *filename,
                    void (*synch_init_wait)(const struct timeval *ts, void *arg),
                    void (*synch_wait)(const struct timeval *delay, void *arg),
                    void (*synch_print)(const char *data, int len, void *arg),
                    void *arg);

ttyrec          ttyrec_init(tty vt);
ttyrec          ttyrec_load(int fd, const char *format, const char *filename, tty vt);
void            ttyrec_free(ttyrec tr);
ttyrec_frame    ttyrec_seek(ttyrec tr, const struct timeval *t, tty *vt);
ttyrec_frame    ttyrec_next_frame(ttyrec tr, ttyrec_frame tfv);
void            ttyrec_add_frame(ttyrec tr, const struct timeval *delay,
                    const char *data, int len);
int             ttyrec_save(ttyrec tr, int fd, const char *format,
                    const char *filename, const struct timeval *selstart,
                    const struct timeval *selend);


#define tadd(t, d)      do                                              \
                        {if (((t).tv_usec+=(d).tv_usec)>=1000000)       \
                            (t).tv_usec-=1000000, (t).tv_sec++;         \
                         (t).tv_sec+=(d).tv_sec;                        \
                        } while (0)
#define tsub(t, d)      do                                              \
                        {if ((signed)((t).tv_usec-=(d).tv_usec)<0)      \
                            (t).tv_usec+=1000000, (t).tv_sec--;         \
                         (t).tv_sec-=(d).tv_sec;                        \
                        } while (0)
#define tmul1000(t, m)  do                                              \
                        {long long v;                                   \
                         v=((long long)(t).tv_usec)*(m)/1000+           \
                             ((long long)(t).tv_sec)*(m)*1000;          \
                         (t).tv_usec=v%1000000;                         \
                         (t).tv_sec=v/1000000;                          \
                         if ((t).tv_usec<0)                             \
                             (t).tv_usec+=1000000, (t).tv_sec--;        \
                        } while (0)
#define tdiv1000(t, m)  do                                              \
                        {long long v;                                   \
                         v=((long long)(t).tv_sec)*1000000+(t).tv_usec; \
                         v*=1000;                                       \
                         v/=m;                                          \
                         (t).tv_usec=v%1000000;                         \
                         (t).tv_sec=v/1000000;                          \
                         if ((t).tv_usec<0)                             \
                             (t).tv_usec+=1000000, (t).tv_sec--;        \
                        } while (0)
#define tcmp(t1, t2)    (((t1).tv_sec>(t2).tv_sec)?1:           \
                         ((t1).tv_sec<(t2).tv_sec)?-1:          \
                         ((t1).tv_usec>(t2).tv_usec)?1:         \
                         ((t1).tv_usec<(t2).tv_usec)?-1:0)
#endif
