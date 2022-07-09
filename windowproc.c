// windowproc.c

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#include "textproc.h"


// create a window
// returns pointer to window or NULL if error
struct WINDOW* create_window(ALLEGRO_DISPLAY *display, int width, int height, int xpos, int ypos) {

    struct WINDOW *p;

    if (width == 0 || height == 0) {
        fprintf(stderr, "could not create window, width or height is zero\n");
        return NULL;
    }

    // allocate memory and set it all to zero
    p = calloc(1, sizeof(struct WINDOW));
    if (p == NULL) {
        fprintf(stderr, "could not create window\n");
        return NULL;
    }

    p->width = width;
    p->height = height;
    p->winposx = xpos;
    p->winposy = ypos;
    al_set_new_window_position(p->winposx, p->winposy);
    al_set_new_display_flags(DEFAULT_WINDOW_FLAGS);
    p->display = al_create_display(p->width,p->height);
    if (p->display == NULL) {
        fprintf(stderr, "could not create display\n");
        return NULL;
    }

    return (struct WINDOW*)p;
}

// return resource used by window
void destroy_window(struct WINDOW *w) {

    int i;

    if (w == NULL) {
        return;
    }

    // todo - add clearing for graphic bit maps

    al_destroy_display(w->display);
    free(w);
}

// copy window color settings to character color settings
// returns 0 if success, otherwise -1
int restore_colors(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "color not set, window pointer is NULL\n");
        return -1;
    }

    w->fcp.bgcolor = w->winbgcolor;
    w->fcp.fgcolor = w->winfgcolor;

    return 0;
}


// set colors window
// returns 0 if success, otherwise -1
int set_window_colors(struct WINDOW *w, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc) {

    if (w == NULL) {
        fprintf(stderr, "color not set, window pointer is NULL\n");
        return -1;
    }

    w->winfgcolor = fgc;
    w->winbgcolor = bgc;
    restore_colors(w);

    return 0;
}

// set cursor position
// returns 0 if success, otherwise -1
int set_window_cursor_pos(struct WINDOW *w, int x, int y) {

    if (w == NULL) {
        fprintf(stderr, "cusor position not set, window pointer is NULL\n");
        return -1;
    }
    w->xcursor = (float)x;
    w->ycursor = (float)y;
    return 0;
}

// set active font for window
// returns 0 if success, otherwise -1
int set_window_font(struct WINDOW *w, struct FONT_LUT *fntlut) {

    if (w == NULL) {
        fprintf(stderr, "font not set, window pointer is NULL\n");
        return -1;
    }
    if (fntlut == NULL) {
        fprintf(stderr, "font not set, FONT_LUT pointer is NULL\n");
        return -1;
    }

    w->flut = fntlut;
    return 0;
}

// todo set_window_scale? using set_font_scale currently - refactor

// set window defaults
// returns 0 if success, otherwise -1
int set_window_defaults(struct WINDOW *w) {

    int r;

    if (w == NULL) {
        fprintf(stderr, "window pointer is NULL\n");
        return -1;
    }
    r = set_window_colors(w, DEFAULT_WINDOW_BGCOLOR, DEFAULT_WINDOW_FGCOLOR);
    if (r == -1) {
        fprintf(stderr, "could not set window color\n");
        return -1;
    }
    r = set_window_cursor_pos(w, DEFAULT_WINDOW_HOME_X, DEFAULT_WINDOW_HOME_Y);
    if (r == -1) {
        fprintf(stderr, "could not set cursor position\n");
        return -1;
    }
    w->fcp.scale = DEFAULT_WINDOW_SCALE;
    w->fcp.style = DEFAULT_WINDOW_STYLE;

    return 0;
}

// clear window
// returns 0 if success, otherwise -1
int clear_window(struct WINDOW *w) {

    int r;
    int i;

    if (w == NULL) {
        fprintf(stderr, "window pointer is NULL\n");
        return -1;
    }

    al_clear_to_color(w->winbgcolor);

    // clear all character on display
    memset(&w->c, 0x0, (sizeof(struct CHARACTER)*MAX_CHARS_IN_WINDOW));

    w->charcnt = 0;

    // todo - add graphic clearing code here

    return 0;
}

// copies the current cursor location to the charcter structure
// returns 0 if success, otherwise -1
int cp_cursorxy_to_charxy(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    w->c[w->charcnt].x = w->xcursor;
    w->c[w->charcnt].y = w->ycursor;
    return 0;
}

// move cursor to the next new line (carriage return, line feed)
// returns 0 if success, otherwise -1
int new_line(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    w->xcursor = 0;
    w->ycursor += w->flut->rec[0].rowcnt * w->fcp.scale;
    return 0;
}

// move cursor position one character position based on the current character
// at the cursor position. Move to the next row if position is not viewable.
// returns 0 if success, otherwise -1
int update_cursor_pos(struct WINDOW *w) {

    int charcnt;
    float scale;
    float charwidth;

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    // move the cursor in the x direction, the width of the character
    charcnt = w->charcnt;
    scale = w->fcp.scale;
    charwidth = (float)w->c[charcnt].fr.rec.colcnt * scale;

    // calculate where to move the cursor
    w->xcursor += charwidth;

    // test if we went past the width of the window
    if (w->xcursor > (w->width - charwidth)) {

        // locate cursor to the start of the next line, based on the font size
        w->xcursor = 0;
        w->ycursor += (float)w->c[charcnt].fr.rec.rowcnt * scale;
    }

    return 0;
}

// print string to window
// returns 0 if success, otherwise -1
int dprint(struct WINDOW *w, char *s, unsigned char style) {

    int r;
    int i;
    int sl;
    int charcnt;
    float height, width;
    ALLEGRO_COLOR bgc, fgc;
    char t[MAX_PRINT_LINE];
    char c;

    if (w == NULL) {
        fprintf(stderr, "print error, window pointer is NULL\n");
        return -1;
    }
    if (s == NULL) {
        fprintf(stderr, "print error, string pointer is NULL\n");
        return -1;
    }
    if (w->charcnt == MAX_CHARS_IN_WINDOW) {
        fprintf(stderr, "window full, exceeds max chars in window, nothing added\n");
        return -1;
    }

    sl = strnlen(s, MAX_PRINT_LINE);
    memcpy(t,s,sl);

    for (i = 0; i < sl; i++) {

        c = t[i];
        if (c == '\n') {
            new_line(w);
            continue;
        }
        if (c == '\t') {
            // todo add tab processing
            continue;
        }
        if (c == '\r') {
            // todo add carriage return processing
            continue;
        }

        height = w->height;
        width = w->width;
        bgc = w->fcp.bgcolor;
        fgc = w->fcp.fgcolor;
        charcnt = w->charcnt;

        r = get_font_record(c, w->flut, &w->c[charcnt].fr);
        if (r == -1) {
            printf("char not found in font index array\n");
            return -1;
        }

        r = set_font_color(&w->c[charcnt].fcp, bgc, fgc);
        if (r == -1) {
            printf("could not set font parmaters\n");
            return -1;
        }

        r = set_font_scale(&w->c[charcnt].fcp, DEFAULT_WINDOW_SCALE);
        if (r == -1) {
            printf("could not set font parmaters\n");
            return -1;
        }

        r = set_font_style(&w->c[charcnt].fcp, style);
        if (r == -1) {
            printf("could not set font parmaters\n");
            return -1;
        }

        r = cp_cursorxy_to_charxy(w);
        if (r == -1) {
            printf("could not copy cursor position\n");
            return -1;
        }

        r = update_cursor_pos(w);
        if (r == -1) {
            printf("could not update cursor position\n");
            return -1;
        }
        w->charcnt++;
        printf("charcnt = %d\n", w->charcnt); // todo - for testing, remove

    }
    return 0;
}

// window update
// place in a thread so it is called repeatively, at a known rate
// only need one of these for multiple windows
int window_update(struct WINDOW *w) {

    int i;
    int r;
    float x, y;
    struct POSITION pos;

    if (w == NULL) {
        fprintf(stderr, "update error, window pointer is NULL\n");
        return -1;
    }

    // increment blink counter
    if (w->blinkcounter == UCHAR_MAX) {
        w->blinkcounter = 0;
    }
    else {
        w->blinkcounter++;
    }

    al_set_target_backbuffer(w->display);
    al_clear_to_color(w->winbgcolor);

    for (i = 0; i < w->charcnt; i++) {

        // check style, process blinking
        if ((!(w->c[i].fcp.style & BLINK)) ||
            (w->c[i].fcp.style & BLINK) && (w->blinkcounter & BLINK_MASK_p50)) {

            // add/subtract any scrolling offsets (offsets can be negative)
            x = w->c[i].x + w->scrolloffsetx;
            y = w->c[i].y + w->scrolloffsety;

            // display character only if viewable
            if ((x >= 0) && (x < w->width) && (y >= 0) && (y < w->height)) {

                pos.x0 = x;
                pos.y0 = y;
                r = make_character(&w->c[i].fr, &w->c[i].fcp, &pos);
                if (r == -1) {
                    printf("could not make character\n");
                    return -1;
                }
            }
        }
    }

    al_flip_display();
}

