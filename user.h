#ifndef __USER__
#define __USER__

//#define USER_PRINT(...) sprintf((g_output_buffer), __VA_ARGS__)

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
user_view_fn(void *arg, void* value_j)
{
    struct user_defined_struct *u =
        (struct user_defined_struct *)arg;
    fprintf(stdout, "user defined i is %d\n\
            \ruser defined j is %d\n", u->i,
            u->j);
    /*
    USER_PRINT("user defined i is %d\nuser defined j is %d\n",
                u->i, u->j);
                */
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
