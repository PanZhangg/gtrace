#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "ylog_view.h"

WINDOW *my_win;

static WINDOW *
create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);

    wrefresh(local_win);

    return local_win;
}

static void
shutdown(int sig)
{
    delwin(my_win);
    endwin();
    exit(0);
}

static void
welcome()
{
    signal(SIGINT, shutdown);
    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();
    echo();
    mvaddstr(3, 33, "ylog Trace Viewer");
    refresh();
    //my_win = create_newwin(40, 40, 40, 40);
}

struct trace_manager *
ylog_view_init()
{
    struct trace_manager *tm = trace_manager_connect(SHM_KEY);
    return tm;
}

int
main()
{
    struct trace_manager *tm = ylog_view_init();
/*
 * TODO:register callback func automatically
 */
    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(tm, "user_defined_struct", user_view_fn);
    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(tm, "time_trace", time_view_fn);
    struct trace_point *tp = get_first_tp_by_track(tm, 4);
    struct trace_point *tps[5];
    uint32_t num;
    find_tp_by_track(tm, 4, tps, &num);
    list_point(tps, num);

    while (1) {
        welcome();
        list_all_trace_point(tm);
        int j;
        tp->cr_fn((void *)&tp->view_buffer[2].data, &j);
        mvaddstr(10, 33, g_output_buffer);
        refresh();
        sleep(100);
    }

    int i, j;
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
