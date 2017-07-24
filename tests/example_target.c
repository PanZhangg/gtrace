#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "../ylog.h"
#include "../user.h"

int main() {
    trace_manager_init(SHM_KEY);

    //struct user_defined_struct *u;

    //struct time_trace *t;

    int i, j;
    uint64_t start = trace_cpu_time_now();
    asm volatile ("" ::: "memory");
    for (i = 0, j = 0; i < 100000000; i++, j++) {
        SET_TRIGGER_TRACE_POINT(4, "user_defined_struct", example_tp,
                                struct user_defined_struct, user_trigger_fn,
                                user_record_fn, i, j);
/*
        SET_TRACE_POINT(4, "user_defined_struct", example_tp,
                        struct user_defined_struct,
                        user_record_fn, i, j);
*/
    }
    asm volatile ("" ::: "memory");
    uint64_t end = trace_cpu_time_now();
    //SET_TRACE_POINT(4, "time_trace", time_tp, t);
    //t->start = start;
    //t->end = end;
    printf("Time passed: %ld\n", end - start);
    //STOP_RECORD(time_tp);

    i = 0;
    while(1) {
        i++;
        SET_MONITOR_POINT(i);
        sleep(1);
    }

    trace_manager_destroy(&g_trace_manager);

    return 0;
}
