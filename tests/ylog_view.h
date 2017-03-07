#ifndef __YLOG_VIEW__
#define __YLOG_VIEW__
#include <stdio.h>
#include <stdint.h>

struct time_trace {
    uint64_t start;
    uint64_t end;
};

struct user_defined_struct {
    int i;
    int j;
};


void
user_view_fn(void *arg, void* value_j)
{
    struct user_defined_struct *u =
        (struct user_defined_struct *)arg;
    fprintf(stdout, "user defined i is %d\n\
            \ruser defined j is %d\n", u->i,
            u->j);
    *(int *)value_j = u->j;
}

void
time_view_fn(void *arg, void* value_j)
{
    struct time_trace *t =
        (struct time_trace *)arg;
    fprintf(stdout, "start time stamp %ld\n\
            \rend time stamp %ld\n", t->start,
            t->end);
    value_j = NULL;
}

struct trace_manager *
ylog_view_init();

#endif
