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
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#include "textproc.h"

// get character width of current font in pixels
// get glyph size from 1st character glyph in table (mono spaced font)
static float get_char_width(struct WINDOW *w) {
    return (float)w->flut->rec[0].colcnt * w->fcp.scale;
}

// get character height of current font in pixels
// get glyph size from 1st character glyph in table (mono spaced font)
static float get_char_height(struct WINDOW *w) {
    return (float)w->flut->rec[0].rowcnt * w->fcp.scale;
}

// update blink counter
// return -1 if error, otherwise 0
static void update_blink_counter(struct WINDOW *w) {

    if (w->blinkcounter == UCHAR_MAX) {
        w->blinkcounter = 0;
    }
    else {
        w->blinkcounter++;
    }
}

// copy window color settings to character color settings
// returns 0 if success, otherwise -1
static int restore_colors(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "color not set, window pointer is NULL\n");
        return -1;
    }

    //w->fcp.bgcolor = w->winbgcolor;
    memcpy(&w->fcp.bgcolor, &w->winbgcolor, sizeof(ALLEGRO_COLOR));
    //w->fcp.fgcolor = w->winfgcolor;
    memcpy(&w->fcp.fgcolor, &w->winfgcolor, sizeof(ALLEGRO_COLOR));

    return 0;
}

// copies the current cursor location to the charcter structure
// returns 0 if success, otherwise -1
static int cp_cursorxy_to_charxy(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    w->c[w->charcnt].x = w->xcursor;
    w->c[w->charcnt].y = w->ycursor;
    return 0;
}

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
        return p;
    }

    return (struct WINDOW*)p;
}

// return resource used by window
void destroy_window(struct WINDOW *w) {

    if (w == NULL) {
        return;
    }

    // todo - add clearing for graphic bit maps

    al_destroy_display(w->display);
    free(w);
}

// set colors window
// returns 0 if success, otherwise -1
int set_window_colors(struct WINDOW *w, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc) {

    if (w == NULL) {
        fprintf(stderr, "color not set, window pointer is NULL\n");
        return -1;
    }

    //w->winfgcolor = fgc;
    memcpy(&w->winfgcolor, &fgc, sizeof(ALLEGRO_COLOR));
    //w->winbgcolor = bgc;
    memcpy(&w->winbgcolor, &bgc, sizeof(ALLEGRO_COLOR));

    restore_colors(w);

    return 0;
}

// set the window's blink rate (blink divisor)
int set_window_blinkrate(struct WINDOW *w, unsigned char bd){

    if (w == NULL) {
        return -1;
    }
    if (bd > (BLINK_MASK_1 | BLINK_MASK_p50 | BLINK_MASK_p25 | BLINK_MASK_p125)) {
        return -1;
    }
    w->blinkdivisor = bd;
    return 0;
}


// set cursor position (pixels)
// returns 0 if success, otherwise -1
int set_window_cursor_posxy(struct WINDOW *w, int x, int y) {

    if (w == NULL) {
        fprintf(stderr, "cusor position not set, window pointer is NULL\n");
        return -1;
    }
    w->xcursor = (float)x;
    w->ycursor = (float)y;
    return 0;
}

// set cursor position (row, col)
// move cursor to character row and column using the currently selected font
// returns 0 if success, otherwise -1
int set_window_cursor_posrc(struct WINDOW *w, int r, int c) {

    if (w == NULL) {
        fprintf(stderr, "cusor position not set, window pointer is NULL\n");
        return -1;
    }

    w->xcursor = (float)c * get_char_width(w);
    w->ycursor = (float)r * get_char_height(w);
    return 0;
}


// set tab stops
// returns 0 if success, otherwise -1
int set_window_tab_stops(struct WINDOW *w, struct TABS *hts, struct TABS *vts) {

    if (w == NULL) {
        fprintf(stderr, "window pointer is NULL\n");
        return -1;
    }

    w->hts = hts;
    w->vts = vts;
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
    r = set_window_cursor_posxy(w, DEFAULT_WINDOW_HOME_X, DEFAULT_WINDOW_HOME_Y);
    if (r == -1) {
        fprintf(stderr, "could not set cursor position\n");
        return -1;
    }
    r = set_window_tab_stops(w, NULL, NULL);
    if (r == -1) {
        fprintf(stderr, "could not set tab stops\n");
        return -1;
    }
    r = set_window_blinkrate(w, DEFAULT_WINDOW_BLINKRATE);
    if (r == -1) {
        fprintf(stderr, "could not set blink rate\n");
        return -1;
    }
    w->fcp.scale = DEFAULT_WINDOW_SCALE;
    w->fcp.style = DEFAULT_WINDOW_STYLE;

    return 0;
}

// clear window
// returns 0 if success, otherwise -1
int clear_window(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "window pointer is NULL\n");
        return -1;
    }

    al_clear_to_color(w->winbgcolor);

    // clear all character on display
    memset(&w->c, 0x0, (sizeof(struct CHARACTER)*MAX_CHARS_IN_WINDOW));

    w->charcnt = 0;
    w->scrolloffsetx = 0;
    w->scrolloffsety = 0;
    set_window_cursor_posxy(w, 0, 0);


    // todo - add graphic clearing code here

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
    w->ycursor += get_char_height(w);
    return 0;
}

// move cursor to the start of the current line (carriage return)
// returns 0 if success, otherwise -1
int carriage_return(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    w->xcursor = 0;
    return 0;
}

// move cursor position one character position forward
// If LINE_WRAP is enabled, move to the next row when at the right edge of the window
// returns 0 if success, otherwise -1
int move_cursor_fwd(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    // move the cursor in the x direction, the width of the character
    // calculate where to move the cursor
    w->xcursor += get_char_width(w);

#if LINE_WRAP
    // test if we went past the width of the window
    if (w->xcursor > (w->width - get_char_width(w))) {

        // locate cursor to the start of the next line, based on the font size
        w->xcursor = 0;
        w->ycursor += get_char_height(w);
    }
#endif

    return 0;
}

// move cursor position one character position backward
// Stop when we are at the left edge of the window
// returns 0 if success, otherwise -1
int move_cursor_bwd(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    // move the cursor in the x direction, the width of a character
    w->xcursor -= get_char_width(w);
    if (w->xcursor < 0) {
        w->xcursor = 0;
    }

    return 0;
}

// move cursor position one character position up
// Stop when we are at the top edge of the window
// returns 0 if success, otherwise -1
int move_cursor_up(struct WINDOW *w) {

    if (w == NULL) {
        fprintf(stderr, "cursor position not updated, window pointer is NULL\n");
        return -1;
    }

    // move the cursor in the x direction, the width of a character
    w->ycursor -= get_char_height(w);
    if (w->ycursor < 0) {
        w->ycursor = 0;
    }

    return 0;
}

// horizontal tab cursor backwards
// returns 0 if success, otherwise -1
int htab_cursor_pos_bwd(struct WINDOW *w, struct TABS *ht) {

    int i;
    float fstop;

    if (w == NULL) {
        return -1;
    }

    // if no tabs setting, do nothing to cursor position
    if (ht == NULL) {
        return 0;
    }

    // tab backwards
    for (i = w->hts->numoftabstops-1; i >= 0 ; i--) {
        fstop = get_char_width(w) * (float)w->hts->ts[i];
        if (w->xcursor > fstop) {
            w->xcursor = fstop;
            return 0;
        }
    }

    // return cursor to the start of the line
    carriage_return(w);

    return 0;
}

// horizontal tab cursor foward
// returns 0 if success, otherwise -1
int htab_cursor_pos_fwd(struct WINDOW *w, struct TABS *ht) {

    int i;
    float fstop;

    if (w == NULL) {
        return -1;
    }

    // if no tabs setting, do nothing to cursor position
    if (ht == NULL) {
        return 0;
    }

    // tab forwards
    for (i = 0; i < w->hts->numoftabstops; i++) {
        fstop = get_char_width(w) * (float)w->hts->ts[i];
        if (w->xcursor < fstop) {
            w->xcursor = fstop;
            return 0;
        }
    }

    // past last tab stop, move cursor to next line
    new_line(w);

    return 0;
}


// vertical tab cursor
// returns 0 if success, otherwise -1
int vtab_cursor_pos(struct WINDOW *w, struct TABS *vt) {

    int i;
    float fstop;

    if (w == NULL) {
        return -1;
    }

    // if no tabs setting, do nothing to cursor position
    if (vt == NULL) {
        return 0;
    }

    // tab down
    for (i = 0; i < w->vts->numoftabstops; i++) {
        fstop = get_char_height(w) * (float)w->vts->ts[i];
        if (w->ycursor < fstop) {
            w->ycursor = fstop;
            return 0;
        }
    }
    // past last tab stop
    // todo - do what

    return 0;
}

// delete character at cursor position
int delete_char(struct WINDOW *w) {

    int i, r;

    if (w == NULL) {
        return -1;
    }

    for (i = 0; i < w->charcnt; i++) {
        if ((w->c[i].x == w->xcursor) && (w->c[i].y == w->ycursor)) {
            // mark character deleted
            w->c[i].fr.rec.c = DELETED_CHARACTER;
        }
    }
    return 0;
}

// process format effectors
// returns true if a format effector was processed, otherwise false
bool proc_format_effectors(struct WINDOW *w, char c) {

    // process escape sequences
    if (c == '\n') {
        new_line(w);
        return true;
    }
    if (c == '\t') {
        htab_cursor_pos_fwd(w, w->hts);
        return true;
    }
    if (c == '\r') {
        carriage_return(w);
        return true;
    }
    if (c == '\f') {
        clear_window(w);
        return true;
    }
    if (c == '\v') {
        vtab_cursor_pos(w, w->vts);
        return true;
    }
    if (c == '\b') {
        move_cursor_bwd(w);
        delete_char(w);
        return true;
    }
    if (c == DELETED_CHARACTER) {
        return true;
    }
    return false;
}

// print string to window
// returns 0 if success, otherwise -1
int dprint(struct WINDOW *w, char *s, unsigned char style) {

    int r;
    int i;
    int sl;
    bool tf;;
    int charcnt;
    //float height, width;
    //ALLEGRO_COLOR bgc, fgc;
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

        // get character
        c = t[i];

        // process format effectors
        tf = proc_format_effectors(w, c);
        if (tf == true) {
            continue;
        }

        //height = w->height;
        //width = w->width;
        //bgc = w->fcp.bgcolor;
        //fgc = w->fcp.fgcolor;
        charcnt = w->charcnt;

        r = get_font_record(c, w->flut, &w->c[charcnt].fr);
        if (r == -1) {
            printf("char not found in font index array\n");
            return -1;
        }

        r = set_font_color(&w->c[charcnt].fcp, w->fcp.bgcolor, w->fcp.fgcolor);
//        r = set_font_color(&w->c[charcnt].fcp, bgc, fgc);
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

        r = move_cursor_fwd(w);
        if (r == -1) {
            printf("could not update cursor position\n");
            return -1;
        }
        w->charcnt++;
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
    update_blink_counter(w);

    al_set_target_backbuffer(w->display);
    al_clear_to_color(w->winbgcolor);

    for (i = 0; i < w->charcnt; i++) {

        // check if deleted character
        if (w->c[i].fr.rec.c != DELETED_CHARACTER) {

            // check style, process blinking
            if (!(w->c[i].fcp.style & BLINK) || (w->blinkcounter & w->blinkdivisor)) {

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
    }

    al_flip_display();
}

