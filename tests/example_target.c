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
    printf("Time passed: %ld\n", end - start);

    int count = 0;
    while(1) {
        count++;
        SET_MONITOR_POINT(count);
        sleep(0.01);
    }

    trace_manager_destroy(&g_trace_manager);

    return 0;
}
