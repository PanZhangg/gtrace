#ifndef __USER__H__
#define __USER__H__

#include "ylog.h"

/*===========================
 * User defined section
 * data structures and callback functions
 *==========================*/

struct time_trace {
    uint64_t start;
    uint64_t end;
};

struct user_defined_struct {
    int i;
    int j;
};

void
user_record_fn(struct user_defined_struct *uds, int i, int j)
{
    uds->i = i;
    uds->j = j * j;
}

void
user_view_fn(void *arg, void *value_j)
{
    struct user_defined_struct *u = (struct user_defined_struct *)arg;

    *(int *)value_j = u->j;

    SET_USER_CONTENT_STRING("j is %d\n", u->j);
}

void
time_view_fn(void *arg, void *value_j)
{
    struct time_trace *t = (struct time_trace *)arg;

    SET_USER_CONTENT_STRING("start time stamp %ld\n\
                            \rend time stamp %ld\n", t->start, t->end);
    value_j = NULL;
}

int
user_trigger_fn(void *arg)
{
    struct user_defined_struct *s = (struct user_defined_struct *)arg;

    if (s->i == 1000) {
        return 1;
    } else {
        return 0;
    }
}

#endif
