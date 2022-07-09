// testproc.c

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

// style flags
const unsigned char NO_STYLE = 0;
const unsigned char INVERT = 1;
const unsigned char UNDER_SCORE  = 2;
const unsigned char STRIKE_THRU  = 4;
const unsigned char BLINK = 8;

// blink rate divisor
// blink rate is determined by the window_update function calling rate
// you can select slower blink rates by selecting one of these divisors
const unsigned char BLINK_MASK_1    = 1;    // fastest rate
const unsigned char BLINK_MASK_p50  = 2;    // 1/2 of the fastest rate
const unsigned char BLINK_MASK_p25  = 4;    // 1/4 of the fastest rate
const unsigned char BLINK_MASK_p125 = 8;    // 1/8 of the fastest rate

// builds a font look-up table that the bitmap functions will use
// returns -1 if error, otherwise the number of characters found in the font array
int build_font_lut(struct FONT_LUT *fi, char *font, size_t size, int rstrikethru, int runderline) {

    int n, rcnt, ccnt, nrec, idx;

    if ((fi == NULL) || (font == NULL)) {
        return -1;
    }

    nrec = 0;
    idx = -1;

    // save pointer to font
    fi->fp = font;

    // save font style params
    fi->strikethrurow = rstrikethru;
    fi->underscorerow = runderline;

    for (n = 0; n < size; n++) {

        if ((n > 0) && (font[n-1] == '@') && (font[n] == '@')) {
            // character in table is the same as the start "new character" token
            continue;
        }
        if ((n > 0) && (font[n-1] == '@') && (font[n] == '=')) {
            // character in table is the same as the "end row character" token
            continue;
        }
        if ((n > 0) && (font[n-1] == '@') && (font[n] == '*')) {
            // character in table is the same as the "end row character" token
            continue;
        }
        if ((n > 0) && (font[n-1] == '@') && (font[n] == 'x')) {
            // character in table is the same as the "pixel on" token
            continue;
        }
        if ((n > 0) && (font[n-1] == '@') && (font[n] == '-')) {
            // character in table is the same as the "pixel off" token
            continue;
        }
        if (font[n] == '@') {

            // start of new character glyph, adjust counts and indexes
            rcnt = 0;
            ccnt = 0;
            nrec ++;
            fi->numofchars = nrec;
            idx++;

            // store character
            fi->rec[idx].c = font[n+1];

            // store index to character pattern
            fi->rec[idx].index = n+2;

            if ((font[n+2] != '-') && (font[n+2] != 'x')){
                // error, malformed font table
                return -1;
            }
        }
        else if ((font[n] == '-') || (font[n] == 'x')) {
            // count columns in current row
            ccnt++;
            fi->rec[idx].colcnt = ccnt;
        }
        else if ((font[n] == '=') || (font[n] == '*')) {
            // end of row
            ccnt++;
            fi->rec[idx].colcnt = ccnt;
            ccnt = 0;
            rcnt++;
            fi->rec[idx].rowcnt = rcnt;
        }
    }

    // return the number of character glyphs forund in font file or -1 if error
    return fi->numofchars;
}


int get_font_record(char c, struct FONT_LUT *fi, struct FONT_REC *fr) {

    int i;

    for (i = 0; i < fi->numofchars; i++) {
        if (c == fi->rec[i].c) {
            // found character
            fr->strikethrurow = fi->strikethrurow;
            fr->underscorerow = fi->underscorerow;
            fr->fp = fi->fp;
            memcpy(&fr->rec, &fi->rec[i], sizeof(struct FONT_LUT_REC));
            return 0;
        }
    }

    // did not find a match
    // return a error and a default char
    fr->strikethrurow = fi->strikethrurow;
    fr->underscorerow = fi->underscorerow;
    fr->fp = fi->fp;
    memcpy(&fr->rec, &fi->rec[DEFAULT_ERR_CHAR_INDEX], sizeof(struct FONT_LUT_REC));
    return -1;
}

// set the print style to INVERT, UNDER_SCORE, STRIKE_THRU, BLINK
// these can be or'd together
int set_font_style(struct FONT_CHAR_PARAM *s, unsigned char style){

    if (s == NULL) {
        fprintf(stderr, "pointer to style structure is NULL\n");
        return -1;
    }
    if (style > (INVERT | UNDER_SCORE | STRIKE_THRU | BLINK)) {
        fprintf(stderr, "Invalid style value\n");
        return -1;
    }
    s->style = style;
    return 0;
}

int set_font_color(struct FONT_CHAR_PARAM *fcp, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc) {

    if (fcp == NULL) {
        fprintf(stderr, "null pointer to FONT_CHAR_PARAM structure\n");
        return -1;
    }

    fcp->bgcolor = bgc;
    fcp->fgcolor = fgc;
    return 0;
}

int set_font_scale(struct FONT_CHAR_PARAM *fcp, float scale) {

    if (fcp == NULL) {
        fprintf(stderr, "null pointer to FONT_CHAR_PARAM structure\n");
        return -1;
    }

    fcp->scale = scale;
    return 0;
}

// draw pixels of character in fr, using parameters in fcp
// returns 0 if successful, otherwise -1
int make_character(struct FONT_REC *fr, struct FONT_CHAR_PARAM *fcp, struct POSITION *pos) {

    int i;
    float r, c;
    float x0, x1;
    float y0, y1;
    ALLEGRO_COLOR color;

    if ((fr == NULL) || (fcp == NULL) || (pos == NULL)) {
        return -1;
    }

    i = fr->rec.index;
    for (r = 0; r < fr->rec.rowcnt; r++) {
        for (c = 0; c < fr->rec.colcnt; c++) {
            x0 = fcp->scale*(c) + pos->x0;
            x1 = fcp->scale*(c+1) + pos->x0;
            y0 = fcp->scale*(r) + pos->y0;
            y1 = fcp->scale*(r+1) + pos->y0;

            // todo - refactor
            if (fcp->style & UNDER_SCORE || fcp->style & STRIKE_THRU) {
                if (fcp->style & UNDER_SCORE) {
                    if (r == fr->underscorerow) {
                        if (fcp->style & INVERT) {
                            color = fcp->bgcolor;
                        }
                        else {
                            color = fcp->fgcolor;
                        }
                        al_draw_filled_rectangle(x0, y0, x1, y1, color);
                        i++;
                        continue;
                    }
                }
                if (fcp->style & STRIKE_THRU) {
                    if (r == fr->strikethrurow) {
                        if (fcp->style & INVERT) {
                            color = fcp->bgcolor;
                        }
                        else {
                            color = fcp->fgcolor;
                        }
                        al_draw_filled_rectangle(x0, y0, x1, y1, color);
                        i++;
                        continue;
                    }
                }
            }

            switch (fr->fp[i]) {

                case '-':
                case '=':
                if (fcp->style & INVERT) {
                    color = fcp->fgcolor;
                }
                else {
                    color = fcp->bgcolor;
                }
                al_draw_filled_rectangle(x0, y0, x1, y1, color);
                break;

                case 'x':
                case '*':
                if (fcp->style & INVERT) {
                    color = fcp->bgcolor;
                }
                else {
                    color = fcp->fgcolor;
                }
                al_draw_filled_rectangle(x0, y0, x1, y1, color);
                break;

                default:
                // print unknown pixel type, fill with background color
                fprintf(stderr,"unknown pixel type, function(%s)\n", __FUNCTION__);
                color = fcp->bgcolor;
                al_draw_filled_rectangle(x0, y0, x1, y1, color);
                break;
            }
            // index to the next pixel
            i++;
        }
    }
    return 0;
}
