#ifndef _TEXTPROC_H
#define _TEXTPROC_H

#ifdef __cplusplus
extern "C" {
#endif

// textproc.h

#include <allegro5/allegro.h>

// color definitions
#define BLACK   al_map_rgb(0, 0, 0)
#define WHITE   al_map_rgb(200, 200, 200)
#define RED     al_map_rgb(255, 0, 0)
#define BLUE    al_map_rgb(0, 0, 255)
#define GREEN   al_map_rgb(0, 255, 0)

// window defaults
#define DEFAULT_WINDOW_BGCOLOR  BLACK
#define DEFAULT_WINDOW_FGCOLOR  WHITE
#define DEFAULT_WINDOW_HOME_X   0
#define DEFAULT_WINDOW_HOME_Y   0
#define DEFAULT_WINDOW_SCALE    2
#define DEFAULT_WINDOW_STYLE    NO_STYLE


// folder were all "graphic" resources will located
#define RESOURCES_DIR "resources"

// maximum number of character in a font file
#define MAX_FONT_GLYPHS  150

// maximum charcters in a window
#define MAX_CHARS_IN_WINDOW (80*24)

// maximum dprint string length
#define MAX_PRINT_LINE  MAX_CHARS_IN_WINDOW

// display this character when there is no match in the font index array
#define DEFAULT_ERR_CHAR_INDEX 0

// window flags
#define DEFAULT_WINDOW_FLAGS (ALLEGRO_NOFRAME)

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
//    float x1;
//    float y1;
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
//    ALLEGRO_BITMAP *bmp;
    struct FONT_CHAR_PARAM fcp;
    struct FONT_REC fr;
    float x;
    float y;
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
    float pixperline;           // pixels per line
    unsigned char cursorstyle;  // cursor style
    unsigned char cursorchar;   // cursor shape

    // tabs
    // todo - tbd

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
int set_font_color(struct FONT_CHAR_PARAM *fcp, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc);
int set_font_style(struct FONT_CHAR_PARAM *s, unsigned char style);
int set_font_scale(struct FONT_CHAR_PARAM *fcp, float scale);
int make_character(struct FONT_REC *fr, struct FONT_CHAR_PARAM *fcp, struct POSITION *pos);


// window prototypes
struct WINDOW* create_window(ALLEGRO_DISPLAY *display, int width, int height, int xpos, int ypos);
int set_window_colors(struct WINDOW *w, ALLEGRO_COLOR bgc, ALLEGRO_COLOR fgc);
int set_window_cursor_pos(struct WINDOW *w, int x, int y);
int set_window_defaults(struct WINDOW *w);
int clear_window(struct WINDOW *w);
int set_window_font(struct WINDOW *w, struct FONT_LUT *fntlut);
int new_line(struct WINDOW *w);
int update_cursor_pos(struct WINDOW *w);
int dprint(struct WINDOW *w, char *s, unsigned char style);
int window_update(struct WINDOW *w);
void destroy_window(struct WINDOW *w);


#ifdef __cplusplus
}
#endif

#endif