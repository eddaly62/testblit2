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
#include <assert.h>

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
static void update_blink_counter(struct WINDOW *w) {

    if (w->blinkcounter == UCHAR_MAX) {
        w->blinkcounter = 0;
    }
    else {
        w->blinkcounter++;
    }
}

// copy window color settings to character color settings
static void restore_colors(struct WINDOW *w) {

    assert(w != NULL);

    memcpy(&w->fcp.bgcolor, &w->winbgcolor, sizeof(ALLEGRO_COLOR));
    memcpy(&w->fcp.fgcolor, &w->winfgcolor, sizeof(ALLEGRO_COLOR));
}

// copies the current cursor location to the charcter structure
static void cp_cursorxy_to_charxy(struct WINDOW *w) {

    assert(w != NULL);

    w->c[w->charcnt].x = w->xcursor;
    w->c[w->charcnt].y = w->ycursor;
}

// create a window
// returns pointer to window or NULL if error
struct WINDOW* create_window(ALLEGRO_DISPLAY *display, int width, int height, int xpos, int ypos) {

    struct WINDOW *p;

    assert((width != 0) || (height != 0));

    // allocate memory and set it all to zero
    p = calloc(1, sizeof(struct WINDOW));
    assert(p != 0);

    p->width = width;
    p->height = height;
    p->winposx = xpos;
    p->winposy = ypos;
    al_set_new_window_position(p->winposx, p->winposy);
    al_set_new_display_flags(DEFAULT_WINDOW_FLAGS);
    p->display = al_create_display(p->width,p->height);
    assert(p->display != NULL);

    return (struct WINDOW*)p;
}

// return resource used by window
void destroy_window(struct WINDOW *w) {

    assert(w != NULL);

    // todo - add clearing for graphic bit maps

    al_destroy_display(w->display);
    free(w);
}

// set colors window
void set_window_colors(struct WINDOW *w, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc) {

    assert(w != NULL);

    memcpy(&w->winfgcolor, &fgc, sizeof(ALLEGRO_COLOR));
    memcpy(&w->winbgcolor, &bgc, sizeof(ALLEGRO_COLOR));

    restore_colors(w);
}

// set the window's blink rate (blink divisor)
void set_window_blinkrate(struct WINDOW *w, unsigned char bd){

    assert(w != NULL);
    assert(bd <= (BLINK_MASK_1 | BLINK_MASK_p50 | BLINK_MASK_p25 | BLINK_MASK_p125));

    w->blinkdivisor = bd;
}


// set cursor position (pixels)
void set_window_cursor_posxy(struct WINDOW *w, int x, int y) {

    assert(w != NULL);

    w->xcursor = (float)x;
    w->ycursor = (float)y;
}

// set cursor position (row, col)
// move cursor to character row and column using the currently selected font
void set_window_cursor_posrc(struct WINDOW *w, int r, int c) {

    assert(w != NULL);

    w->xcursor = (float)c * get_char_width(w);
    w->ycursor = (float)r * get_char_height(w);
}


// set tab stops
void set_window_tab_stops(struct WINDOW *w, struct TABS *hts, struct TABS *vts) {

    assert(w != NULL);

    w->hts = hts;
    w->vts = vts;
}


// set active font for window
void set_window_font(struct WINDOW *w, struct FONT_LUT *fntlut) {

    assert(w != NULL);
    assert(fntlut != NULL);

    w->flut = fntlut;
}

// todo set_window_scale? using set_font_scale currently - refactor

// set window defaults
// returns 0 if success, otherwise -1
void set_window_defaults(struct WINDOW *w) {

    assert(w != NULL);

    set_window_colors(w, DEFAULT_WINDOW_BGCOLOR, DEFAULT_WINDOW_FGCOLOR);
    set_window_cursor_posxy(w, DEFAULT_WINDOW_HOME_X, DEFAULT_WINDOW_HOME_Y);
    set_window_tab_stops(w, NULL, NULL);
    set_window_blinkrate(w, DEFAULT_WINDOW_BLINKRATE);
    w->fcp.scale = DEFAULT_WINDOW_SCALE;
    w->fcp.style = DEFAULT_WINDOW_STYLE;

}

// clear window
void clear_window(struct WINDOW *w) {

    assert(w != NULL);

    al_clear_to_color(w->winbgcolor);

    // clear all character on display
    memset(&w->c, 0x0, (sizeof(struct CHARACTER)*MAX_CHARS_IN_WINDOW));

    w->charcnt = 0;
    w->scrolloffsetx = 0;
    w->scrolloffsety = 0;
    set_window_cursor_posxy(w, 0, 0);


    // todo - add graphic clearing code here

}

// move cursor to the next new line (carriage return, line feed)
void new_line(struct WINDOW *w) {

    assert(w != NULL);

    w->xcursor = 0;
    w->ycursor += get_char_height(w);
}

// move cursor to the start of the current line (carriage return)
void carriage_return(struct WINDOW *w) {

    assert(w != NULL);

    w->xcursor = 0;
}

// move cursor position one character position forward
// If LINE_WRAP is enabled, move to the next row when at the right edge of the window
void move_cursor_fwd(struct WINDOW *w) {

    assert(w != NULL);

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

}

// move cursor position one character position backward
// Stop when we are at the left edge of the window
void move_cursor_bwd(struct WINDOW *w) {

    assert(w != NULL);

    // move the cursor in the x direction, the width of a character
    w->xcursor -= get_char_width(w);
    if (w->xcursor < 0) {
        w->xcursor = 0;
    }

}

// move cursor position one character position up
// Stop when we are at the top edge of the window
void move_cursor_up(struct WINDOW *w) {

    assert(w != NULL);

    // move the cursor in the x direction, the width of a character
    w->ycursor -= get_char_height(w);
    if (w->ycursor < 0) {
        w->ycursor = 0;
    }

}

// horizontal tab cursor backwards
void htab_cursor_pos_bwd(struct WINDOW *w, struct TABS *ht) {

    int i;
    float fstop;

    assert(w != NULL);

    // if no tabs setting, do nothing to cursor position
    if (ht == NULL) {
        return;
    }

    // tab backwards
    for (i = w->hts->numoftabstops-1; i >= 0 ; i--) {
        fstop = get_char_width(w) * (float)w->hts->ts[i];
        if (w->xcursor > fstop) {
            w->xcursor = fstop;
            return;
        }
    }

    // return cursor to the start of the line
    carriage_return(w);

    return;
}

// horizontal tab cursor foward
void htab_cursor_pos_fwd(struct WINDOW *w, struct TABS *ht) {

    int i;
    float fstop;

    assert(w != NULL);

    // if no tabs setting, do nothing to cursor position
    if (ht == NULL) {
        return;
    }

    // tab forwards
    for (i = 0; i < w->hts->numoftabstops; i++) {
        fstop = get_char_width(w) * (float)w->hts->ts[i];
        if (w->xcursor < fstop) {
            w->xcursor = fstop;
            return;
        }
    }

    // past last tab stop, move cursor to next line
    new_line(w);

    return;
}


// vertical tab cursor
void vtab_cursor_pos(struct WINDOW *w, struct TABS *vt) {

    int i;
    float fstop;

    assert(w != NULL);

    // if no tabs setting, do nothing to cursor position
    if (vt == NULL) {
        return;
    }

    // tab down
    for (i = 0; i < w->vts->numoftabstops; i++) {
        fstop = get_char_height(w) * (float)w->vts->ts[i];
        if (w->ycursor < fstop) {
            w->ycursor = fstop;
            return;
        }
    }
    // past last tab stop
    // todo - do what

    return;
}

// delete character at cursor position
void delete_char(struct WINDOW *w) {

    int i, r;

    assert(w != NULL);

    for (i = 0; i < w->charcnt; i++) {
        if ((w->c[i].x == w->xcursor) && (w->c[i].y == w->ycursor)) {
            // mark character deleted
            w->c[i].fr.rec.c = DELETED_CHARACTER;
        }
    }
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
    char t[MAX_PRINT_LINE];
    char c;

    assert(w != NULL);
    assert(s != NULL);
    assert(w->charcnt < MAX_CHARS_IN_WINDOW);

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

        charcnt = w->charcnt;

        r = get_font_record(c, w->flut, &w->c[charcnt].fr);
        if (r == -1) {
            printf("char not found in font index array\n");
            return -1;
        }

        set_font_color(&w->c[charcnt].fcp, w->fcp.bgcolor, w->fcp.fgcolor);
        set_font_scale(&w->c[charcnt].fcp, DEFAULT_WINDOW_SCALE);
        set_font_style(&w->c[charcnt].fcp, style);

        cp_cursorxy_to_charxy(w);
        move_cursor_fwd(w);
        w->charcnt++;
    }
    return 0;
}


// window update
// place in a thread so it is called repeatively, at a known rate
// only need one of these for multiple windows
void window_update(struct WINDOW *w) {

    int i;
    int r;
    float x, y;
    struct POSITION pos;

    assert(w != NULL);

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
                    make_character(&w->c[i].fr, &w->c[i].fcp, &pos);
                }
            }
        }
    }

    al_flip_display();
}

