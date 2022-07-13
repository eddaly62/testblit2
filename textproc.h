#ifndef _TEXTPROC_H
#define _TEXTPROC_H

#ifdef __cplusplus
extern "C" {
#endif

// textproc.h

#include <allegro5/allegro.h>

// color definitions
#define BLACK       (al_map_rgb(0, 0, 0))
#define WHITE       (al_map_rgb(200, 200, 200))
#define RED         (al_map_rgb(255, 0, 0))
#define BLUE        (al_map_rgb(0, 0, 255))
#define GREEN       (al_map_rgb(0, 255, 0))
// retro colors of older terminals
#define AMBER       (al_map_rgb(255,176,0))
#define LT_AMBER    (al_map_rgb(255,204,0))
#define APPLE2      (al_map_rgb(51,255,51))
#define APPLE2C     (al_map_rgb(102,255,102))
#define GREEN1      (al_map_rgb(51,255,0))
#define GREEN2      (al_map_rgb(0,255,51))
#define GREEN3      (al_map_rgb(0,255,102))

// window defaults
#define DEFAULT_WINDOW_BGCOLOR      BLACK
#define DEFAULT_WINDOW_FGCOLOR      LT_AMBER
#define DEFAULT_WINDOW_HOME_X       0
#define DEFAULT_WINDOW_HOME_Y       0
#define DEFAULT_WINDOW_SCALE        2
#define DEFAULT_WINDOW_STYLE        NO_STYLE
#define DEFAULT_WINDOW_BLINKRATE    BLINK_MASK_1

// maximum number of character in a font file
#define MAX_FONT_GLYPHS  128

// maximum charcters in a window
#define MAX_CHARS_IN_WINDOW (2*84*24)

// maximum dprint string length
#define MAX_PRINT_LINE  128

// display this character when there is no match in the font index array
#define DEFAULT_ERR_CHAR_INDEX 0

// window flags
#define DEFAULT_WINDOW_FLAGS (ALLEGRO_NOFRAME)

// pcg format is the same as character font
// so instead of duplicating the function
// create an alias so the same function can be accessed
// using a more decriptive name
#define build_pcg_lut  build_font_lut

// line wrap when cursor moves past right side of window
#define LINE_WRAP (true)

// deleted character marker
// this character when a CHARACTER element in the WINDOW structure is marked as deleted.
// deleted character are not actually deleted until a clear screen (formfeed) is performed.
// the deleted marked characters are not rendered by the window_update function
#define DELETED_CHARACTER   '\e'

// font style
extern const unsigned char INVERT;
extern const unsigned char UNDER_SCORE;
extern const unsigned char STRIKE_THRU;
extern const unsigned char BLINK;
extern const unsigned char NO_STYLE;

// blink rates devisor for blink counter
// fastest rate is determined by the rate that window_update() is called
extern const unsigned char BLINK_MASK_1;    // fastest rate
extern const unsigned char BLINK_MASK_p50;  // 1/2 of the fastest rate
extern const unsigned char BLINK_MASK_p25;  // 1/4 of the fastest rate
extern const unsigned char BLINK_MASK_p125; // 1/8 of the fastest rate

// generic struct to pass location values
struct POSITION {
    float x0;
    float y0;
};

struct FONT_LUT_REC {
    char c;                 // ascii character display
    int index;              // start of pattern
    int rowcnt;             // number of rows
    int colcnt;             // number of columns
};

struct FONT_LUT {
    char *fp;               // pointer to font array
    int numofchars;         // number of chars in font array
    int underscorerow;      // row to place under score, row 0 is top row of font
    int strikethrurow;      // row to place strike thru, row 0 is top row of font
    struct FONT_LUT_REC rec[MAX_FONT_GLYPHS];
};

struct FONT_REC {
    int underscorerow;          // row to place under score, row 0 is top row of font
    int strikethrurow;          // row to place strike thru, row 0 is top row of font
    char *fp;                   // pointer to font array
    struct FONT_LUT_REC rec;    // results of search
};

struct FONT_CHAR_PARAM {
    float scale;                // min value of ratio of display resolution to screen resolution 
    ALLEGRO_COLOR bgcolor;      // background color
    ALLEGRO_COLOR fgcolor;      // foreground color
    unsigned char style;        // INVERT | UNDER_SCORE | STRIKE_THRU | BLINK
    unsigned char blinkdivisor; // blink divisor, only valid if style is BLINK
};

// contains everything the text processor needs to display a character
struct CHARACTER {
    struct FONT_CHAR_PARAM fcp;
    struct FONT_REC fr;
    float x;
    float y;
};

// defintion for horizontal and vertical tab stop setting (pixels)
struct TABS {
    int numoftabstops;          // number of tab stops (starts at 0, first row is row 0)
    int ts[];                   // tab stop in character positions
};

struct WINDOW {

    // display
    ALLEGRO_DISPLAY *display;   // pointer to display object
    ALLEGRO_COLOR winbgcolor;   // window background color
    ALLEGRO_COLOR winfgcolor;   // window foreground color
    float width;                // width of window
    float height;               // height of window
    int winposx;                // position of window
    int winposy;
    unsigned char blinkcounter; // blink counter
    unsigned char blinkdivisor; // mask for selecting blink rate divisor

    // cursor
    float xcursor;              // cursor location
    float ycursor;
    float scrolloffsetx;        // scrolling offset
    float scrolloffsety;
    unsigned char cursorstyle;  // cursor style
    unsigned char cursorchar;   // cursor shape

    // tabs
    struct TABS *hts;           // pointer to horizontal tab setting struct
    struct TABS *vts;           // pointer to vertical tab setting struct

    // text
    struct FONT_LUT *flut;      // current active font
    struct FONT_CHAR_PARAM fcp; // active character parameters
    int charcnt;                // nuber of characters in window
    struct CHARACTER c[MAX_CHARS_IN_WINDOW];

    // graphics
    // TODO - add graphics
};

// prototypes
int build_font_lut(struct FONT_LUT *fi, char *font, size_t size, int rstrikethru, int runderline);
int get_font_record(char c, struct FONT_LUT *fi, struct FONT_REC *fr);
void set_font_color(struct FONT_CHAR_PARAM *fcp, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc);
void set_font_style(struct FONT_CHAR_PARAM *s, unsigned char style);
void set_font_blinkrate(struct FONT_CHAR_PARAM *s, unsigned char bd);
void set_font_scale(struct FONT_CHAR_PARAM *fcp, float scale);
void make_character(struct FONT_REC *fr, struct FONT_CHAR_PARAM *fcp, struct POSITION *pos);


// window prototypes
struct WINDOW* create_window(ALLEGRO_DISPLAY *display, int width, int height, int xpos, int ypos);
void set_window_colors(struct WINDOW *w, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc);
void set_window_blinkrate(struct WINDOW *w, unsigned char bd);
void set_window_cursor_posxy(struct WINDOW *w, int x, int y);
void set_window_cursor_posrc(struct WINDOW *w, int r, int c);
void set_window_defaults(struct WINDOW *w);
void clear_window(struct WINDOW *w);
void set_window_tab_stops(struct WINDOW *w, struct TABS *hts, struct TABS *vts);
void set_window_font(struct WINDOW *w, struct FONT_LUT *fntlut);
void new_line(struct WINDOW *w);
void carriage_return(struct WINDOW *w);
void move_cursor_fwd(struct WINDOW *w);
void move_cursor_bwd(struct WINDOW *w);
void move_cursor_up(struct WINDOW *w);
void delete_line(struct WINDOW *w);
void insert_line(struct WINDOW *w);
void htab_cursor_pos_fwd(struct WINDOW *w, struct TABS *ht);
void htab_cursor_pos_bwd(struct WINDOW *w, struct TABS *ht);
void vtab_cursor_pos(struct WINDOW *w, struct TABS *vt);
void delete_char(struct WINDOW *w);
void insert_char(struct WINDOW *w);
int dprint(struct WINDOW *w, char *s, unsigned char style);
void window_update(struct WINDOW *w);
void destroy_window(struct WINDOW *w);


#ifdef __cplusplus
}
#endif

#endif