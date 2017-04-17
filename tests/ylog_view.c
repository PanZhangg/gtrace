#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "ylog_view.h"

WINDOW *footer;
WINDOW *header;

static void
update_footer(void);

static void
update_header(void);

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
    delwin(footer);
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

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_WHITE, COLOR_GREEN);
        init_pair(5, COLOR_BLACK, COLOR_YELLOW);
        init_pair(6, COLOR_GREEN, COLOR_BLACK);
        init_pair(7, COLOR_RED, COLOR_YELLOW);
    }

    //mvaddstr(3, 33, "ylog Trace Viewer");
    //refresh();
    //my_win = create_newwin(40, 40, 40, 40);
    footer = create_newwin(40, 400, LINES, 0);
    header = create_newwin(40, 400, LINES, 0);
    update_footer();
    update_header();
}

static void
print_digit(WINDOW *win, int digit)
{
    if (digit < 0) {
        wattron(win, COLOR_PAIR(1));
        wprintw(win, "%d", digit);
        wattroff(win, COLOR_PAIR(1));
    } else if (digit > 0) {
        wattron(win, COLOR_PAIR(2));
        wprintw(win, "+%d", digit);
        wattroff(win, COLOR_PAIR(2));
    } else {
        wprintw(win, "0");
    }
}

/*
static void
print_headers(int line, char *desc, int value, int second)
*/

static void
set_window_title(WINDOW *win, char *title)
{
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 1, title);
    wattroff(win, A_BOLD);
}

static void
print_key(WINDOW *win, char *key, char *desc, int toggle)
{
    int pair;
    if (toggle > 0) {
        pair = 4;
    } else {
        pair = 3;
    }
    wattron(win, COLOR_PAIR(pair));
    wprintw(footer, "%s", key);
    wattroff(win, COLOR_PAIR(pair));
    wprintw(footer, ":%s", desc);
}

static void
update_footer(void)
{
    werase(footer);
    wmove(footer, 1, 1);
    print_key(footer, "F2", "TP", 1);

    wrefresh(footer);
}

static void
update_header(void)
{
    werase(header);
    wmove(header, 1, 1);
    wattron(header, COLOR_PAIR(4));
    wprintw(header, "%s", "Trace Viewer");
    wattroff(header, COLOR_PAIR(4));

    wrefresh(header);
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

    //while (1) {
        welcome();
        sleep(10);
    //}
    /*
        list_all_trace_point(tm);
        int j;
        tp->cr_fn((void *)&tp->view_buffer[2].data, &j);
        mvaddstr(10, 33, g_output_buffer);
        refresh();
        sleep(100);
    }
    */

    int i, j;
    for (i = 0; i < 10; i++) {
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
