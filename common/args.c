#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "sys/error.h"
#include "gettext.h"
#include "ttyrec.h"
#include "common.h"

extern char *optarg;

void get_w_format(const char **format)
{
    int i;
    const char *fn, *fe;

    if (*format)
        die(_("You can use only one format at a time.\n"));
    if (!(*format=ttyrec_w_find_format(optarg, 0, 0)))
    {
        fprintf(stderr, _("No such format: %s\n"), optarg);
        fprintf(stderr, _("Valid formats:\n"));
        for (i=0;(fn=ttyrec_w_get_format_name(i));i++)
            if ((fe=ttyrec_w_get_format_ext(fn)))
                fprintf(stderr, " %-15s (%s)\n", fn, fe);
            else
                fprintf(stderr, " %-15s\n", fn);
        exit(1);
    }
}

void get_r_format(const char **format)
{
    int i;
    const char *fn, *fe;

    if (*format)
        die(_("You can use only one format at a time.\n"));
    if (!(*format=ttyrec_r_find_format(optarg, 0, 0)))
    {
        fprintf(stderr, _("No such format: %s\n"), optarg);
        fprintf(stderr, _("Valid formats:\n"));
        for (i=0;(fn=ttyrec_r_get_format_name(i));i++)
            if ((fe=ttyrec_r_get_format_ext(fn)))
                fprintf(stderr, " %-15s (%s)\n", fn, fe);
            else
                fprintf(stderr, " %-15s\n", fn);
        exit(1);
    }
}
