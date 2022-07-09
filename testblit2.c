// testblit2.c

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#include "textproc.h"

// window size and location
#define WIN_WIDTH   512
#define WIN_HEIGHT  700
#define WIN_LOC_X   400
#define WIN_LOC_Y   10
#define HOME_X      0
#define HOME_Y      0

#define FONT5X7_UNDERLINE_OFFSET    9
#define FONT5X7_STRIKETHRU_OFFSET   4

#define FONT5X7R_UNDERLINE_OFFSET   9
#define FONT5X7R_STRIKETHRU_OFFSET  4

#define FONT7X9_UNDERLINE_OFFSET    11
#define FONT7X9_STRIKETHRU_OFFSET   5

#define FONT7X9R_UNDERLINE_OFFSET   11
#define FONT7X9R_STRIKETHRU_OFFSET  0

#define FONT10X14_UNDERLINE_OFFSET  17
#define FONT10X14_STRIKETHRU_OFFSET 9

#define FONT10X14R_UNDERLINE_OFFSET 17
#define FONT10X14R_STRIKETHRU_OFFSET    9


extern char *font_design_5_7;
extern char *font_design_7_9_rulings;
extern char *font_design_5_7_rulings;
extern char *font_design_7_9;
extern char *font_design_10_14;
extern char *font_design_10_14_rulings;

struct FONT_LUT font5x7lut;
struct FONT_LUT font5x7rlut;
struct FONT_LUT font7x9lut;
struct FONT_LUT font7x9rlut;
struct FONT_LUT font10x14lut;
struct FONT_LUT font10x14rlut;

struct FONT_LUT_REC fontrec;
struct FONT_REC fresult;
struct FONT_CHAR_PARAM fcparam;
struct WINDOW *win;

enum SETIME {START = 0, END = 1};

struct timeval start, end;

// Determine elapse time
double elapsed_time(enum SETIME sts, struct timeval *start, struct timeval *end)
{
	long long startusec = 0, endusec = 0;
	double elapsed = 0;

	if (sts == START)
	{
		gettimeofday(start, NULL);
	}
	else
	{
		gettimeofday(end, NULL);
		startusec = start->tv_sec * 1000000 + start->tv_usec;
		endusec = end->tv_sec * 1000000 + end->tv_usec;
		//elapsed = (double)(endusec - startusec) / 1000000.0;	// seconds
		elapsed = (double)(endusec - startusec);				// usec
	}
	return elapsed;
}


int main()  {

    int i;
    int r;
    bool running = true;
    double elapsedt=0;

    ALLEGRO_DISPLAY *display;
    ALLEGRO_EVENT_QUEUE *q;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT event;

    al_init();
    al_init_font_addon();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();

    q = al_create_event_queue();
    al_register_event_source(q, al_get_keyboard_event_source());
    timer = al_create_timer(1.0 / 4);
    al_start_timer(timer);

    // tell allegro were the resource directory is
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
    al_append_path_component(path, RESOURCES_DIR);
    al_change_directory(al_path_cstr(path, '/'));
    al_destroy_path(path);

    // build font look-up tables
    r = build_font_lut(&font5x7lut, font_design_5_7, strlen(font_design_5_7),
                        FONT5X7_STRIKETHRU_OFFSET, FONT5X7_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }
    printf("sizeof font5x7lut = %ld\n", sizeof(struct FONT_LUT));
    r = build_font_lut(&font5x7rlut, font_design_5_7_rulings, strlen(font_design_5_7_rulings),
                        FONT5X7R_STRIKETHRU_OFFSET, FONT5X7R_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }
    r = build_font_lut(&font7x9lut, font_design_7_9, strlen(font_design_7_9),
                        FONT7X9_STRIKETHRU_OFFSET, FONT7X9_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }
    r = build_font_lut(&font7x9rlut, font_design_7_9_rulings, strlen(font_design_7_9_rulings),
                        FONT7X9R_STRIKETHRU_OFFSET, FONT7X9R_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }
    r = build_font_lut(&font10x14lut, font_design_10_14, strlen(font_design_10_14),
                        FONT10X14_STRIKETHRU_OFFSET, FONT10X14_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }
    r = build_font_lut(&font10x14rlut, font_design_10_14_rulings, strlen(font_design_10_14_rulings),
                        FONT10X14R_STRIKETHRU_OFFSET, FONT10X14R_UNDERLINE_OFFSET);
    if (r == -1) {
        printf("malformed font index table\n");
    }

    // create window (using all the apis to test them)
    win = create_window(display, WIN_WIDTH, WIN_HEIGHT, WIN_LOC_X, WIN_LOC_Y);

    r = set_window_defaults(win);
    if (r == -1) {
        fprintf(stderr, "could not set window defaults\n");
    }
    r = set_window_colors(win, BLACK, WHITE);
    if (r == -1) {
        fprintf(stderr, "could not set window color\n");
    }
    r = set_window_cursor_pos(win, HOME_X, HOME_Y);
    if (r == -1) {
        fprintf(stderr, "could not set cursor position\n");
    }
    r = set_window_font(win, &font5x7lut);
    if (r == -1) {
        fprintf(stderr, "could not set font\n");
    }

    al_register_event_source(q, al_get_display_event_source(win->display));
    al_register_event_source(q, al_get_timer_event_source(timer));

    r = dprint(win, "0123456789\n", NO_STYLE);
    r = dprint(win, " !\"#$%&'()*+,-./\n", NO_STYLE);
    r = dprint(win, "abcdefghijklmnopqrstuvwxyz\n", NO_STYLE);
    r = dprint(win, ":;<=>?@\n", NO_STYLE);
    r = dprint(win, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", NO_STYLE);
    r = dprint(win, "[\\]^_`{|}~\177\n\n", NO_STYLE);

    r = dprint(win, "0123456789\n", BLINK);
    r = dprint(win, " !\"#$%&'()*+,-./\n", BLINK);
    r = dprint(win, "abcdefghijklmnopqrstuvwxyz\n", BLINK);
    r = dprint(win, ":;<=>?@\n", BLINK);
    r = dprint(win, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", BLINK);
    r = dprint(win, "[\\]^_`{|}~\177\n\n", BLINK);

    r = dprint(win, "0123456789\n", INVERT);
    r = dprint(win, " !\"#$%&'()*+,-./\n", INVERT);
    r = dprint(win, "abcdefghijklmnopqrstuvwxyz\n", INVERT);
    r = dprint(win, ":;<=>?@\n", INVERT);
    r = dprint(win, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", INVERT);
    r = dprint(win, "[\\]^_`{|}~\177\n\n", INVERT);

    r = dprint(win, "0123456789\n", UNDER_SCORE);
    r = dprint(win, " !\"#$%&'()*+,-./\n", UNDER_SCORE);
    r = dprint(win, "abcdefghijklmnopqrstuvwxyz\n", UNDER_SCORE);
    r = dprint(win, ":;<=>?@\n", UNDER_SCORE);
    r = dprint(win, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", UNDER_SCORE);
    r = dprint(win, "[\\]^_`{|}~\177\n\n", UNDER_SCORE);

    r = dprint(win, "0123456789\n", STRIKE_THRU);
    r = dprint(win, " !\"#$%&'()*+,-./\n", STRIKE_THRU);
    r = dprint(win, "abcdefghijklmnopqrstuvwxyz\n", STRIKE_THRU);
    r = dprint(win, ":;<=>?@\n", STRIKE_THRU);
    r = dprint(win, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", STRIKE_THRU);
    r = dprint(win, "[\\]^_`{|}~\177\n\n", STRIKE_THRU);

    while (running) {

        al_wait_for_event(q, &event);

        if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            running = false;
        }

        if (event.type == ALLEGRO_EVENT_TIMER) {

            elapsedt = elapsed_time(START, &start, &end);

            r = window_update(win);
            if (r == -1) {
                fprintf(stderr, "error with window_update\n");
            }

            elapsedt = elapsed_time(END, &start, &end);

            printf(" window_update time = %.0f usec\n", elapsedt);
            fflush(stdout);
        }
    }

    // quit
    destroy_window(win);
    al_destroy_timer(timer);
    al_uninstall_keyboard();

    return 0;
}

