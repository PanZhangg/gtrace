#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "ylog_view.h"

#define DEFAULT_DELAY 15
#define MAX_LINE_LENGTH 50
#define MAX_LOG_LINES 4

int pref_panel_visible = 0;
int pref_line_selected = 0;
int pref_current_sort = 0;

int last_display_index, currently_displayed_index;

struct processtop *selected_process = NULL;
int selected_ret;

int selected_line = 0; /* select bar position */
int selected_in_list = 0; /* selection relative to the whole list */
int list_offset = 0; /* first index in the list to display (scroll) */
int nb_log_lines = 0;
char log_lines[MAX_LINE_LENGTH * MAX_LOG_LINES + MAX_LOG_LINES];

int max_elements = 80;

int toggle_threads = 1;
int toggle_virt = -1;
int toggle_pause = -1;

int filter_host_panel = 0;

int max_center_lines;
//struct header_view cputopview[6];
//struct header_view iostreamtopview[3];
//struct header_view fileview[3];
//struct header_view kprobeview[2];

WINDOW *footer;
WINDOW *header;
WINDOW *center;
WINDOW *status;
char *termtype;

static void
update_footer(void);

static void
update_header(void);

static void
print_digit(WINDOW *win, int digit);

void
print_log(char *str);

static WINDOW *
create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);

    wrefresh(local_win);

    return local_win;
}

static WINDOW *
create_newwin_no_border(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);

    wrefresh(local_win);

    return local_win;
}

void
set_window_title(WINDOW *win, char *title)
{
	wattron(win, A_BOLD);
	mvwprintw(win, 0, 1, title);
	wattroff(win, A_BOLD);
}

void
basic_header()
{
	werase(header);
	box(header, 0 , 0);
	set_window_title(header, "Statistics for interval [gathering data...[");
	wattron(header, A_BOLD);
	mvwprintw(header, 1, 4, "CPUs");
	mvwprintw(header, 2, 4, "Threads");
	mvwprintw(header, 3, 4, "FDs");
	wattroff(header, A_BOLD);
	wrefresh(header);
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
	termtype = getenv("TERM");
	if (!strcmp(termtype, "xterm") ||  !strcmp(termtype, "xterm-color") ||
			!strcmp(termtype, "vt220")) {
		define_key("\033[H", KEY_HOME);
		define_key("\033[F", KEY_END);
		define_key("\033OP", KEY_F(1));
		define_key("\033OQ", KEY_F(2));
		define_key("\033OR", KEY_F(3));
		define_key("\033OS", KEY_F(4));
		define_key("\0330U", KEY_F(6));
		define_key("\033[11~", KEY_F(1));
		define_key("\033[12~", KEY_F(2));
		define_key("\033[13~", KEY_F(3));
		define_key("\033[14~", KEY_F(4));
		define_key("\033[16~", KEY_F(6));
		define_key("\033[17;2~", KEY_F(18));
	}
	//signal(SIGTERM, handle_sigterm);
	//signal(SIGINT, handle_sigterm);
	mousemask(BUTTON1_CLICKED, NULL);
	refresh();

    //mvaddstr(3, 33, "ylog Trace Viewer");
    //refresh();
    //my_win = create_newwin(40, 40, 40, 40);
    footer = create_newwin(1, COLS - 1, LINES - 1, 0);
    header = create_newwin(5, COLS - 1, 0, 0);
	center = create_newwin(LINES - 5 - 7, COLS - 1, 5, 0);
	status = create_newwin(MAX_LOG_LINES + 2, COLS - 1, LINES - 7, 0);

	print_log("Starting display");

	basic_header();

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

void
print_digits(WINDOW *win, int first, int second)
{
	wprintw(win, "(");
	print_digit(win, first);
	wprintw(win, ", ");
	print_digit(win, second);
	wprintw(win, ")");
}

void
print_headers(int line, char *desc, int value, int first, int second)
{
	wattron(header, A_BOLD);
	mvwprintw(header, line, 4, "%s", desc);
	wattroff(header, A_BOLD);
	mvwprintw(header, line, 16, "%d", value);
	wmove(header, line, 24);
	print_digits(header, first, second);
	wmove(header, line, 40);
}

void
print_log(char *str)
{
	int i;
	int current_line = 1;
	int current_char = 1;
	char *tmp, *tmp2;
	/* rotate the line buffer */
	if (nb_log_lines >= MAX_LOG_LINES) {
		tmp = strndup(log_lines, MAX_LINE_LENGTH * MAX_LOG_LINES + MAX_LOG_LINES);
		tmp2 = strchr(tmp, '\n');
		memset(log_lines, '\0', strlen(log_lines));
		strncat(log_lines, tmp2 + 1, strlen(tmp2) - 1);
		log_lines[strlen(log_lines)] = '\n';
		log_lines[strlen(log_lines)] = '\0';
		free(tmp);
	}
	nb_log_lines++;

	strncat(log_lines, str, MAX_LINE_LENGTH - 1);

	if (nb_log_lines < MAX_LOG_LINES)
		log_lines[strlen(log_lines)] = '\n';
	log_lines[strlen(log_lines)] = '\0';

	werase(status);
	box(status, 0 , 0);
	set_window_title(status, "Status");
	for (i = 0; i < strlen(log_lines); i++) {
		if (log_lines[i] == '\n') {
			wmove(status, ++current_line, 1);
			current_char = 1;
		} else {
			mvwprintw(status, current_line, current_char++, "%c",
					log_lines[i]);
		}
	}
	wrefresh(status);
}

/*
static void
print_headers(int line, char *desc, int value, int second)
*/

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
        //sleep(10);
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
