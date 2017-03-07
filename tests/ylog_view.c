#include <ncurses.h>
#include <unistd.h>
#include "ylog_view.h"
#include "../ylog.h"

void welcome()
{
    mvaddstr(3, 33, "ylog Trace Viewer");
    refresh();
    endwin();
}

struct trace_manager *
ylog_view_init()
{
    struct trace_manager *tm = trace_manager_connect(SHM_KEY);
    initscr();
    return tm;
}

int main() {
    struct trace_manager *tm = ylog_view_init();

    welcome();

    list_all_trace_point(tm);

    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(tm, "user_defined_struct", user_view_fn);
    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(tm, "time_trace", time_view_fn);
    struct trace_point *tp = get_first_tp_by_track(tm, 4);
    struct trace_point *tps[5];
    uint32_t num;
/*
    while (1) {
    };
*/
    find_tp_by_track(tm, 4, tps, &num);
    list_point(tps, num);

    int i,j;
    for (i = 0;
         i < 10;
         i++) {
        if (tp->cr_fn) {
            printf("timestamp: %ld\n",tp->view_buffer[i].event.timestamp);
            tp->cr_fn((void *)&tp->view_buffer[i].data, &j);
            printf("j is %d\n", j);
        }
    }

    struct trace_point *time_tp = &tm->trace_point_list[1];
    time_tp->cr_fn((void *)&time_tp->view_buffer[0].data, &j);
    return 0;
}
