#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "allegro.h"
#include "vuepoint.h"
#include "vuepoint_whnd.h"
#include "vuepoint_process.h"
#include "vuepoint_detail.h"
#include "vuepoint_param.h"
#include "vuepoint_config.h"
#include "vuepoint_serial.h"
#include "vuepoint_callbacks.h"

BEGIN_GFX_DRIVER_LIST
   GFX_DRIVER_VESA1
   GFX_DRIVER_VGA
   GFX_DRIVER_MODEX
   GFX_DRIVER_VESA3
   GFX_DRIVER_VESA2L
   GFX_DRIVER_VESA2B
   GFX_DRIVER_XTENDED
END_GFX_DRIVER_LIST

BEGIN_COLOR_DEPTH_LIST
   COLOR_DEPTH_8
END_COLOR_DEPTH_LIST

BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

#define HELD_MAPS_MAX 8
BITMAP *held_maps[HELD_MAPS_MAX];

char tbuff[80];
FILE *errfile = NULL;

void err_char (char);
void DBG(char *, ...);

int update_screen = 0;

int whndQueryFontSize (int, int *, int *);

void whndCreateTestMaps (void);

int whndTestLoop (int);
void whndSetScreenSize (int, int);

//void whndDrawText(detailDef, int, int, bool);
//void whndRenderText(void);
int whndDrawPoint (int, int, int);
//int whndDrawCircle(int, int, int, int, int, int, int, int, int);
bool whndGetClock(void);
bool whndInit(void);
bool whndStart(void);
int clip (int, int, int);
int last_press = 0;
int whndCursorQuery (void);
static char key_translate(int);
static void snapshot(void);
void flash_area_add (int, int, int, int, int);
void flash_area_deactivate (int);
void flash_areas_update (int, int, int, int);
void perform_graphics_command(int *, unsigned char *, int, int);
void pulse_clock(void);
int whndBuildFont (int, char *);
//void whndBellSelect (int, int);
void whndBellStart(void);
void whndBellUpdate(void);
void whndSnapshot(int);
//int whndClearScreen(void);
void whndDestroy(void);
void whndDrawButton (int, int, int, int );
void whndDrawTouchPoint (int, int, int);
//void whndError(char *);
void whndKeyboardUpdate(void);
void whndPaletteUpdate(void);
int whndPaletteShift (int, int, int);
void whndPollMouse (int *, int *, int);
//void whndPrint (char *, int);
void whndPulseCounter(void);
void whndSetCursor(int, int, int, int, int, int);
void whndShowFailureMode(void);
void whndTouchIndicator(bool);
//void whndBuildAllFonts (void);
void whndValidateFont (char *, int, int, int, int);
int whndSetGraphicWindow (int, int, int, int);
int whndPrintChar (char, int, int, int, int);
int whndTestPattern(int);
int whndBlockHold (int, int, int, int);
int whndBlockRestore (int, int);
int whndBlockClear (int, int, int, int);
void whndReadSpecialSerial (void);
int whndBlockMove (int, int, int, int, int, int);
int whndBlockUpdate (void);
int whndDisplayHold (int);
int where_x (int, int);
int where_y (int, int);
int base_x (void);
int base_y (void);
int display_stopped = 0;

int whndDisplayHold(int _flag)
{
	display_stopped = _flag;
	return(0);
}

struct 
{
	BITMAP *map;
	coordT size;
} block_queue[8];

int block_queue_pointer = 0;

coordT visual_frame = {-1, -1};

//#define lcx(x) ((((long) x) * 640) / 512)
//#define lcy(x) ((((256 - ((long) x))) * 480) / 256)

char failure_mode[80];
int gsx = 0;
int gsy = 0;
int bell_x1, bell_x2, bell_y1, bell_y2;
int bell_active = 0;
int bell_tagged = 0;

int attribute_underline = 0;
int attribute_size = 1;
int attribute_reverse = 0;
int cursor_enable = 0;

int cursor_type = prcCursorTypeUnknown;

typedef struct
{
//	char filename[80];
	coordT size;
	BITMAP *map;
} fontT;


fontT font_maps[8] =
{
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL},
	{{-1,-1}, NULL}
};

//int whndPCGWrite (int, int, int, int);

//int whndLoadFont(int, char *, int, int);
/*
int whndLoadFont(int _index, char *_filename, int _size_x, int _size_y)
{
	if ((_index < 0) || (_index > 7))
		return(1);

	font_maps[_index].size.x = _size_x;
	font_maps[_index].size.y = _size_y;
	font_maps[_index].map = load_bitmap(_filename, NULL);
	if (NULL == font_maps[_index].map)
		return(1);
	else
		return(0);
}
*/
coordT screen_size = {-1, -1};
//coordT error = {0, 0};
coordT cursor_size = {0, 0};
coordT cursor = {0, 0};



#define MAX_FLASH 16
struct flashdef
{
	int x1;
	int y1;
	int x2;
	int y2;
	int active;
};

struct flashdef flash[MAX_FLASH];

int lcx (int);
int lcy (int);

int lcx (int _x)
{
	return((_x * 5 + 2) / 4);
}

int lcy (int _y)
{
	return (visible_area_y2 - ((_y * 5 + 2) / 4));
}

#define WHND_PULSE_FREQUENCY        30
//#define screen_size.y          480
//#define screen_size.x           640
#define WHND_SCREEN_VISUAL_HEIGHT   384
#define WHND_TEXT_HEIGHT            32
#define WHND_TEXT_WIDTH             16
#define WHND_TIME_1_SEC             2

enum
{
    WHND_COLOR_RED = 0,
    WHND_COLOR_GREEN = 1,
    WHND_COLOR_BLUE = 2
};

enum
{
    WHND_BLACK = 0,
    WHND_GRAY = 1,
    WHND_WHITE = 2
};

enum
{
	COLOR_INVIS = 0,
    COLOR_BRIGHT_STEADY,
    COLOR_BRIGHT_SLOW,
    COLOR_BRIGHT_FAST,
    COLOR_DIM_STEADY,
    COLOR_DIM_SLOW,
    COLOR_DIM_FAST,
    COLOR_INVIS_2,
    COLOR_CURSOR,
    COLOR_INDICATOR
};

enum
{
    WHND_SOUND_NONE = -2,
    WHND_SOUND_CREATE = -1
};

volatile int background_colors[3] = {0, 0, 0};
volatile int bell_frequency = 90;
volatile int bell_duration = 3;
int block_pt_1_x = 640;
int block_pt_1_y = WHND_SCREEN_VISUAL_HEIGHT;
int block_pt_2_x = 0;
int block_pt_2_y = 0;
RGB color_fields[3] =
{
    {0, 0, 0},
    {15, 15, 15},
    {31, 31, 31}
};

//#define MAP_VISUAL 5
//#define MAX_HATCH 7
//BITMAP* hatch_fill[MAX_HATCH];
int hatch_select = 1;
int cursor_held = 0;

DATAFILE* data_file;
int font_pos[2] = {0, 1};
volatile int foreground_colors[3] = {31, 31, 31};
volatile bool keyboard_redirect = False;
/*detailDef last_drawn[MAX_COLS][MAX_ROWS] =
{
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020},
    {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020}
};*/
//BITMAP* map_screen;
int polling_column = 0;
bool polling_inactive = True;
int polling_row = 0;
volatile bool pulse = False;
int pulse_counter = 0;
int sound_timer = 0;
int start_column = 0;
int start_row = 0;
int timer_counter = 0;
volatile int timer_pulses = 0;
int touch_light_visible = False;
volatile int vertical_offset = 48;
volatile int visual_touch = False;
BITMAP *cursor_block = NULL;
int cursor_active;
void whndHoldCursor(void);
void whndRestoreCursor (void);
int scaling (void);

int scaling (void)
{
	int scaling_x, scaling_y;

	scaling_x = screen_size.x / visual_frame.x;
	scaling_y = screen_size.y / visual_frame.y;
	if (scaling_x > scaling_y)
		return(scaling_y);
	else
		return(scaling_x);
}

int where_x (int _x, int _option)
{
	if (UpperLeft == _option)
		return(_x * scaling()); //((screen_size.x - (scaling() * visual_frame.x)) / 2) + (_x * scaling()));
	else
		return((_x + 1) * scaling() - 1);//((screen_size.x - (scaling() * visual_frame.x)) / 2) + ((_x + 1) * scaling()) - 1);
}

int where_y (int _y, int _option)
{
	if (UpperLeft == _option)
		return(_y * scaling()); //((screen_size.y - (scaling() * visual_frame.y)) / 2) + (_y * scaling()));
	else
		return((_y + 1) * scaling() - 1);//((screen_size.y - (scaling() * visual_frame.y)) / 2) + ((_y + 1) * scaling()) - 1);
}

void whndSetScreenSize (int _x, int _y)
{
	visual_frame.x = _x;
	visual_frame.y = _y;
}

BITMAP *displays[6];

void
whndDestroy(void)
{
#ifdef DEBUGGING
	fclose(errfile);
#endif
	destroy_bitmap(displays[0]);
	destroy_bitmap(displays[1]);
	destroy_bitmap(displays[2]);
	destroy_bitmap(displays[3]);
	destroy_bitmap(displays[4]);
	destroy_bitmap(displays[5]);

    allegro_exit();
}


void whndCreateTestMaps (void)
{
	int loop, loop_x, loop_y;
	coordT size;

	displays[0] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());
	displays[1] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());
	displays[2] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());
	displays[3] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());
	displays[4] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());
	displays[MAP_VISUAL] = create_bitmap(visual_frame.x * scaling(), visual_frame.y * scaling());

// Test Pattern 0
	static char *temp_string = "ALL DIAGNOSTICS TESTS PASSED";
	clear_to_color(displays[0], COLOR_INVIS);
	for (loop = 0; temp_string[loop]; loop++)
		blit(
			font_maps[0].map,
			displays[0],
			font_maps[0].size.x * scaling() * temp_string[loop],
			0,
			(visual_frame.x / 2 - font_maps[0].size.x * 14 + loop * font_maps[0].size.x) * scaling(),
			(font_maps[0].size.y - 1) * scaling(),
			font_maps[0].size.x * scaling(),
			font_maps[0].size.y * scaling());

// Test Pattern 1
	clear_to_color(displays[1], COLOR_INVIS);

	size.x = visual_frame.x / 32;
	size.y = visual_frame.y / 32;
	for (loop_x = visual_frame.x / size.x - 1; 0 <= loop_x; loop_x--)
		for (loop_y = visual_frame.y / size.y - 1; 0 <= loop_y; loop_y--)
			switch((loop_x + loop_y * 3) % 4)
			{
			case 0:
			case 1:
				rectfill(displays[1],
					where_x(size.x * loop_x, UpperLeft) - base_x(),
					where_y(size.y * loop_y, UpperLeft) - base_y(),
					where_x(size.x * (loop_x + 1) - 1, LowerRight) - base_x(),
					where_y(size.y * (loop_y + 1) - 1, LowerRight) - base_y(),
					COLOR_BRIGHT_STEADY);
				break;
			case 2:
				stretch_blit(font_maps[0].map, displays[1], font_maps[0].size.x * scaling() * 'E', 0,  font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling(),
					where_x(size.x * loop_x, UpperLeft) - base_x(), where_y(size.y * loop_y, UpperLeft) - base_y(), size.x * scaling(), size.y * scaling());
				break;
			case 3:
				stretch_blit(font_maps[0].map, displays[1], font_maps[0].size.x * scaling() * 'O', 0,  font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling(),
					where_x(size.x * loop_x, UpperLeft) - base_x(), where_y(size.y * loop_y, UpperLeft) - base_y(), size.x * scaling(), size.y * scaling());
					break;
			};

// Test Pattern 2
	clear_to_color(displays[2], COLOR_BRIGHT_STEADY);

// Test Pattern 3
	clear_to_color(displays[3], COLOR_BRIGHT_STEADY);
	for (loop = 0; loop < 32; loop++)
	{
		blit(font_maps[1].map, displays[3], ('`' + loop) * font_maps[1].size.x * scaling(), font_maps[1].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[1].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 1, UpperLeft) - base_y(), font_maps[1].size.x * scaling(), font_maps[1].size.y * scaling());

		blit(font_maps[0].map, displays[3], (' ' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 2, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());

		blit(font_maps[0].map, displays[3], ('@' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 3, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());

		blit(font_maps[0].map, displays[3], ('`' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 4, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());


		blit(font_maps[3].map, displays[3], ('`' + loop) * font_maps[3].size.x * scaling(), font_maps[3].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 32) * font_maps[3].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2 - font_maps[3].size.y * scaling(), font_maps[3].size.x * scaling(), font_maps[3].size.y * scaling());

		blit(font_maps[2].map, displays[3], (' ' + loop) * font_maps[3].size.x * scaling(), font_maps[2].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + loop * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2 - font_maps[2].size.y * scaling(), font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());

		blit(font_maps[2].map, displays[3], ('@' + loop) * font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 32) * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2, font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());

		blit(font_maps[2].map, displays[3], ('`' + loop) * font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + loop * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2, font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());


		blit(font_maps[7].map, displays[3], ('`' + loop) * font_maps[7].size.x * scaling(), font_maps[7].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[7].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 5, UpperLeft) - base_y(), font_maps[7].size.x * scaling(), font_maps[7].size.y * scaling());

		blit(font_maps[6].map, displays[3], (' ' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 4, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());

		blit(font_maps[6].map, displays[3], ('@' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 3, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());

		blit(font_maps[6].map, displays[3], ('`' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 4, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 2, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());
	};


// Test Pattern 4
	clear_to_color(displays[4], COLOR_INVIS);
	for (loop = 0; loop < 32; loop++)
	{
		blit(font_maps[1].map, displays[4], ('`' + loop) * font_maps[1].size.x * scaling(), font_maps[1].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[1].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 1, UpperLeft) - base_y(), font_maps[1].size.x * scaling(), font_maps[1].size.y * scaling());

		blit(font_maps[0].map, displays[4], (' ' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 2, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());

		blit(font_maps[0].map, displays[4], ('@' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 3, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());

		blit(font_maps[0].map, displays[4], ('`' + loop) * font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[0].size.x, UpperLeft) - base_x(),
			where_y(font_maps[1].size.y * 4, UpperLeft) - base_y(), font_maps[0].size.x * scaling(), font_maps[0].size.y * scaling());


		blit(font_maps[3].map, displays[4], ('`' + loop) * font_maps[3].size.x * scaling(), font_maps[3].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 32) * font_maps[3].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2 - font_maps[3].size.y * scaling(), font_maps[3].size.x * scaling(), font_maps[3].size.y * scaling());

		blit(font_maps[2].map, displays[4], (' ' + loop) * font_maps[3].size.x * scaling(), font_maps[2].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + loop * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2 - font_maps[2].size.y * scaling(), font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());

		blit(font_maps[2].map, displays[4], ('@' + loop) * font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 32) * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2, font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());

		blit(font_maps[2].map, displays[4], ('`' + loop) * font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + loop * font_maps[2].size.x, UpperLeft) - base_x(),
			(where_y(visual_frame.y, UpperLeft) - base_y()) / 2, font_maps[2].size.x * scaling(), font_maps[2].size.y * scaling());


		blit(font_maps[7].map, displays[4], ('`' + loop) * font_maps[7].size.x * scaling(), font_maps[7].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[7].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 5, UpperLeft) - base_y(), font_maps[7].size.x * scaling(), font_maps[7].size.y * scaling());

		blit(font_maps[6].map, displays[4], (' ' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 4, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());

		blit(font_maps[6].map, displays[4], ('@' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 3, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());

		blit(font_maps[6].map, displays[4], ('`' + loop) * font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling() * 0, where_x(
			(visual_frame.x / 2) + (loop - 16) * font_maps[6].size.x, UpperLeft) - base_x(),
			where_y(visual_frame.y - font_maps[7].size.y * 2, UpperLeft) - base_y(), font_maps[6].size.x * scaling(), font_maps[6].size.y * scaling());
	};

// test pattern 5
	clear_to_color(displays[MAP_VISUAL], COLOR_INVIS);


}

int base_x (void)
{
	return((screen_size.x - (scaling() * visual_frame.x)) / 2);
}

int base_y (void)
{
	return((screen_size.y - (scaling() * visual_frame.y)) / 2);
}

int whndTestPattern (int _flag)
{
	static int current_screen = -1;
	static int diag_displayed = 0;
	static int test_pattern = -1;
	static int timer = 0;
	static int last_pulse = -1;
	static int last_check = -1;
	int ports;


	if ((MAP_HATCHED == _flag) || (MAP_DIAG_PASSED == _flag))
	{
		blit(displays[_flag], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
		current_screen = _flag;
		return(0);
	};

	if ((pulse_counter / 8) != (last_pulse / 8))
	{
		ports = serialCheckTest(0);

		if (ports & SERIAL_DIGITAL_REBOOT)
			mainExit(FLAG_REBOOT);

		timer++;

		last_pulse = pulse_counter;

		if ((-1 == test_pattern) && (ports & SERIAL_DIGITAL_TEST))
			test_pattern = 0;

		if ((5 > timer) && (-1 != current_screen) && (MAP_VISUAL != current_screen))
			return(0);
	
		timer = 0;

		current_screen = MAP_VISUAL;

		switch(test_pattern)
		{
		case 0:
			blit(displays[MAP_ALL_WHITE], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
			current_screen = MAP_ALL_WHITE;
			test_pattern++;
			return(0);
		case 1:
			blit(displays[MAP_INVIS_SETS], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
			current_screen = MAP_INVIS_SETS;
			test_pattern++;
			return(0);
		case 2:
			blit(displays[MAP_NORMAL_SETS], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
			current_screen = MAP_NORMAL_SETS;
			update_screen = 1;
			test_pattern = -1;
			return(0);
		};	

		if (!diag_displayed)
		{
			blit(displays[MAP_DIAG_PASSED], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
			current_screen = MAP_DIAG_PASSED;
			update_screen = 1;
			diag_displayed = 1;
			return(0);
		};

	};

	if (last_check != pulse_counter)
	{
		last_check = pulse_counter;
		/*Removed && queueIsEmpty(parse_queue) && (!display_stopped)*/
		if (update_screen && (MAP_VISUAL == current_screen))
		{
			blit(displays[MAP_VISUAL], screen, 0, 0, base_x(), base_y(), visual_frame.x * scaling(), visual_frame.y * scaling());
			if (cursor_enable)
				switch (cursor_type)
				{
				case prcCursorTypeLShape:
					line(screen, base_x() + cursor.x, base_y() + cursor.y, base_x() + cursor.x, base_y() + cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
					line(screen, base_x() + cursor.x, base_y() + cursor.y, base_x() + cursor.x + cursor_size.x - 1, base_y() + cursor.y, COLOR_BRIGHT_SLOW);
					break;
				case prcCursorTypeCrosshair:
					line(screen, base_x() + cursor.x + 1, base_y() + cursor.y + (cursor_size.y / 2) - 1, base_x() + cursor.x + cursor_size.x - 2, base_y() + cursor.y + (cursor_size.y / 2) - 1, COLOR_BRIGHT_SLOW);
					line(screen, base_x() + cursor.x + (cursor_size.x / 2) - 1, base_y() + cursor.y + 1, base_x() + cursor.x + (cursor_size.x / 2) - 1, base_y() + cursor.y + cursor_size.y - 2, COLOR_BRIGHT_SLOW);
					break;
				case prcCursorTypeUnderscore:
					line(screen, base_x() + cursor.x, base_y() + cursor.y + cursor_size.y - 1, base_x() + cursor.x + cursor_size.x - 1, base_y() + cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
					break;
				case prcCursorTypeBlock:
					rectfill(screen, base_x() + cursor.x, base_y() + cursor.y, base_x() + cursor.x + cursor_size.x - 1, base_y() + cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
					break;
				};
			update_screen = 0;
		};
	};
//	current_screen = MAP_VISUAL;

	return(0);
}

/*
int whndTestLoop (int _reset)
{
	return(0);

	static int counter = -1;
	static int toggle = 0;

	if (-2 == _reset)
	{
		if ((-1 == counter) && (!toggle))
			return(0);
		else
			return(1);
	}
	else if (-1 == _reset)
	{
		toggle = !toggle;
	}
	else if (1 == _reset)
	{
		if (-1 == counter)
			counter = 0;
	}
	else if (-1 != counter)
	{
		if (0 == (counter % 12))
		{
			switch(counter / 12)
			{
			case 0: whndTestPattern(2); break;
			case 1: whndTestPattern(4); break;
			case 2: whndTestPattern(3); break;
			case 3: whndTestPattern(MAP_VISUAL); counter = -1; return(0);
			};
		};
		counter++;
	};
	return(0);
}
*/
int whndBlockMove (int _x1, int _y1, int _x2, int _y2, int _width, int _height)
{
	if (-1 == _x1)
	{
		blit(displays[MAP_VISUAL], displays[MAP_HOLD], 0, 0, 0, 0, visual_frame.x * scaling(), visual_frame.y * scaling());
		return(0);
	}

	blit(displays[MAP_VISUAL], displays[MAP_HOLD], _x1 * scaling(), _y1 * scaling(), _x2 * scaling(), _y2 * scaling(), _width * scaling(), _height * scaling());
	return(0);
}

int whndBlockUpdate (void)
{
	blit(displays[MAP_HOLD], displays[MAP_VISUAL], 0, 0, 0, 0, visual_frame.x * scaling(), visual_frame.y * scaling());
	update_screen = 1;
	return(0);
}

int whndBlockHold (int _x1, int _y1, int _x2, int _y2)
{

	if ((_x1 >= _x2) || (_y1 >= _y2))
	{
		DBG("Hold Block: Too narrow");
		return(1);
	};

	if (5 < block_queue_pointer)
	{
		DBG("Hold Block: Buffer filled");
		return(1);
	};

	
	block_queue[block_queue_pointer].size.x = (_x2 - _x1 + 1) * scaling();
	block_queue[block_queue_pointer].size.y = (_y2 - _y1 + 1) * scaling();
DBG("Hold Block: %d %d %d %d (%d %d)", _x1, _y1, _x2, _y2, block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
//	DBG("\tHoldBlock #%d: %d(%d),%d(%d) %d,%d %d,%d", block_queue_pointer, _x1, whndQueryX(_x1), _y1, whndQueryY(_y1), _x2, _y2, block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
	block_queue[block_queue_pointer].map = create_bitmap(block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
	if (NULL == block_queue[block_queue_pointer].map)
		DBG("Cannot create bitmap.");
	else
	{
		blit(displays[MAP_VISUAL], block_queue[block_queue_pointer].map, where_x(_x1, UpperLeft), where_y(_y1, UpperLeft), 0, 0, block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
		update_screen = 1;
	};
	block_queue_pointer++;
	return(0);
}

int whndBlockRestore (int _x, int _y)
{
	if (0 >= block_queue_pointer)
	{
		DBG("Restore Block: No held block");
		return(1);
	};

	if ((_x < 0) ||
		(_y < 0) ||
		(where_x(_x,LowerRight) + block_queue[block_queue_pointer - 1].size.x - 1) > where_x(visual_frame.x - 1,LowerRight) ||
		(where_y(_y,LowerRight) + block_queue[block_queue_pointer - 1].size.y - 1) > where_y(visual_frame.y - 1,LowerRight))
	{
		DBG("Restore Block: Block off-screen %d,%d %d,%d;%d,%d ",
			_x, _y, block_queue[block_queue_pointer - 1].size.x, block_queue[block_queue_pointer - 1].size.y,
			where_x(visual_frame.x - 1,LowerRight),
			where_y(visual_frame.y - 1,LowerRight));
		return(1);
	};

	block_queue_pointer--;
//	DBG("\tRestoreBlock #%d: %d,%d %d,%d", block_queue_pointer, _x, _y, block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
	blit(block_queue[block_queue_pointer].map, displays[MAP_VISUAL], 0, 0, where_x(_x, UpperLeft), where_y(_y, UpperLeft), block_queue[block_queue_pointer].size.x, block_queue[block_queue_pointer].size.y);
	update_screen = 1;
	destroy_bitmap(block_queue[block_queue_pointer].map);
	return(0);
}

int whndBlockClear (int _x1, int _y1, int _x2, int _y2)
{
	if ((-1 == screen_size.x) || (-1 == screen_size.y))
	{
		return(1);
	};

	if ((_x1 >= _x2) || (_y1 >= _y2))
	{
		DBG("Block Clear: Too narrow %d, %d %d, %d", _x1, _y1, _x2, _y2);
		return(1);
	};

	if ((_x1 < 0) || (_y1 < 0) || (_x2 >= visual_frame.x) || (_y2 >= visual_frame.y))
	{
		DBG("Block Clear: Not on screen %d,%d %d,%d", _x1, _y1, _x2, _y2);
		return(1);
	};

	DBG("Block Clear: %d,%d %d, %d",where_x(_x1, UpperLeft), where_y(_y1, UpperLeft), where_x(_x2, LowerRight), where_y(_y2, LowerRight));
//	DBG("\tClearBlock: %d,%d %d,%d", _x1, _y1, _x2, _y2);
	rectfill(displays[MAP_VISUAL], where_x(_x1, UpperLeft), where_y(_y1, UpperLeft), where_x(_x2, LowerRight), where_y(_y2, LowerRight), COLOR_INVIS);
	update_screen = 1;
	return(0);
}

int whndDrawPoint (int _x, int _y, int _mask)
{
	switch(_mask)
	{
	case -1:
		xor_mode(1);
		rectfill(displays[MAP_VISUAL], where_x(_x, UpperLeft), where_y(_y, UpperLeft), where_x(_x, LowerRight), where_y(_y, LowerRight), COLOR_BRIGHT_STEADY);
		xor_mode(0);
		update_screen = 1;
		return(0);
	case 0:
		rectfill(displays[MAP_VISUAL], where_x(_x, UpperLeft), where_y(_y, UpperLeft), where_x(_x, LowerRight), where_y(_y, LowerRight), COLOR_INVIS);
		update_screen = 1;
		return(0);
	case 1:
		rectfill(displays[MAP_VISUAL], where_x(_x, UpperLeft), where_y(_y, UpperLeft), where_x(_x, LowerRight), where_y(_y, LowerRight), COLOR_BRIGHT_STEADY);
		update_screen = 1;
		return(0);
	};
	return(1);
}


int whndSetGraphicWindow (int _x, int _y, int _w, int _h)
{
	static BITMAP *saved_screen = NULL;
	static int x_offset = -1;
	static int y_offset = -1;
	static int height = -1;
	static int width = -1;

	whndHoldCursor();
	if (-1 == _x)
	{
		if (NULL != saved_screen)
		{
			blit(saved_screen, displays[MAP_VISUAL], 0,0, x_offset, y_offset, width, height);
			update_screen = 1;
			destroy_bitmap(saved_screen);
			saved_screen = NULL;
			return(0);
		}
		else
			return(1);
	}
	else
	{
		if (NULL == saved_screen)
		{
			width = _w * scaling();
			height = _h * scaling();
			x_offset = where_x(_x, UpperLeft);
			y_offset = where_y(_y, UpperLeft);
			saved_screen = create_bitmap(width, height);
			blit(displays[MAP_VISUAL], saved_screen, x_offset, y_offset, 0, 0, width, height);
			rectfill(displays[MAP_VISUAL], x_offset, y_offset, x_offset + width, y_offset + height, COLOR_INVIS);
			update_screen = 1;
			return(0);
		}
		else
			return(1);
	};
	whndRestoreCursor();
}

int whndPrintChar(char _input, int _x, int _y, int _font, int _attr)
{
	attributeT attributes;
	int attr_select = 0;
	int value_select;
	int color_check, loop_x, loop_y;
	coordT pos, offset, size;

	if (' ' > _input)
		return(0);

	value_select = _input;

//	whndHoldCursor();
	attributes.word = _attr;

	if (attributes.bit.reverse)
		attr_select |= 4;
	if (attributes.bit.blink)
		attr_select |= 2;
	if (attributes.bit.underline)
		attr_select |= 1;

//	DBG("\tAttribute key %d", attr_select);

	if (attributes.bit.overstrike)
	{
		offset.y = font_maps[_font].size.y * scaling() * attr_select;
		offset.x = _input * scaling() * font_maps[_font].size.x;
		for (loop_x = (font_maps[_font].size.x - 1) * scaling() + 1; loop_x >= 0; loop_x--)
			for (loop_y = (font_maps[_font].size.y - 1) * scaling() + 1; loop_y >= 0; loop_y--)
			{
				pos.x = where_x(_x, UpperLeft) + loop_x;
				pos.y = where_y(_y, UpperLeft) + loop_y;
				if ((base_x() <= pos.x) && (where_x(visual_frame.x - 1, LowerRight) >= pos.x) &&
					(base_y() <= pos.y) && (where_y(visual_frame.y - 1, LowerRight) >= pos.y))
				{
					color_check = getpixel(font_maps[_font].map, offset.x + loop_x, offset.y + loop_y);
					if (COLOR_INVIS != color_check)
					{
						putpixel(displays[MAP_VISUAL], pos.x, pos.y, color_check);
						update_screen = 1;
					};
				}
			};
	}
	else
	{
		if (_x < 0)
		{
			offset.x = -_x;
			_x = 0;
		}
		else if ((visual_frame.x - _x) < font_maps[_font].size.x)
		{
			size.x = visual_frame.x - _x;
		}
		else
		{
			offset.x = 0;
			size.x = font_maps[_font].size.x;
		};

		if (_y < 0)
		{
			offset.y = -_y;
			_y = 0;
		}
		else if ((visual_frame.y - _y) < font_maps[_font].size.y)
		{
			size.y = visual_frame.y - _y;
		}
		else
		{
			offset.y = 0;
			size.y = font_maps[_font].size.y;
		};

		blit(
			font_maps[_font].map,
			displays[MAP_VISUAL],
			(value_select * font_maps[_font].size.x + offset.x) * scaling(),
			(font_maps[_font].size.y * attr_select + offset.y) * scaling(),
			where_x(_x + offset.x, UpperLeft),
			where_y(_y + offset.y, UpperLeft),
			size.x * scaling(),
			size.y * scaling());
		update_screen = 1;
	};
//	if (attributes.bit.overstrike)
//	{
//		masked_blit(font_map[], screen, x, y, destx, desy, width, height)


//	whndRestoreCursor();
	return(0);
}
/*
int whndPCGWrite (int _bitfield, int _index, int _vert, int _horiz)
{
	int x, y, base_y, loop, color;

	x = (_horiz + _index * font_maps[4].size.x) * scaling();
	base_y = (font_maps[4].size.y - (_vert * 6)) * scaling() - 2;
	for (loop = 0; loop < 6 ; loop++)
	{
		y = base_y - (loop * scaling());
		if (y >= 0)
		{
			if (_bitfield & 1)
				color = COLOR_BRIGHT_STEADY;
			else
				color = COLOR_INVIS;
			rectfill(font_maps[4].map, whndQueryX(x), whndQueryY(x), whndQueryX(x) + scaling() - 1, whndQueryY(y) + scaling() - 1, color);
//			putpixel(font_maps[4].map, x, y, color);
//			putpixel(font_maps[4].map, x + 1, y, color);
//			putpixel(font_maps[4].map, x, y + 1, color);
//			putpixel(font_maps[4].map, x + 1, y + 1, color);
		};
	};
	return(0);
}


*/


void whndHoldCursor(void)
{
	return;
	if (NULL == cursor_block)
		return;

	if (cursor_held)
	{
		DBG("[HC:invalid]");
		return;
	};

//	DBG("[HC%d:%d,%d %d,%d]", cursor_type, cursor.x, cursor.y, cursor_size.x, cursor_size.y);

	blit(cursor_block, screen, 0, 0, cursor.x, cursor.y, cursor_size.x, cursor_size.y);
	cursor_held = 1;
}

void whndRestoreCursor(void)
{
//	whndTestPattern(5);
	return;
//	if (!screen_changed)
//		return;
//	if (NULL == cursor_block)
//		return;

//	if (!cursor_held)
//	{
//		DBG("[RC:invalid]");
//		return;
//	};
//	DBG("[RC%d:%d,%d %d,%d]", cursor_type, cursor.x, cursor.y, cursor_size.x, cursor_size.y);
	blit(
		displays[MAP_VISUAL],
		screen,
		0,
		0,
		(screen_size.x - (scaling() * visual_frame.x)) / 2,
		(screen_size.y - (scaling() * visual_frame.y)) / 2,
		visual_frame.x * scaling(),
		visual_frame.y * scaling());


//	blit(screen, cursor_block, cursor.x, cursor.y, 0, 0, cursor_size.x, cursor_size.y);
	if (cursor_enable)
		switch (cursor_type)
		{
		case prcCursorTypeLShape:
			line(screen, cursor.x, cursor.y, cursor.x, cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
			line(screen, cursor.x, cursor.y, cursor.x + cursor_size.x - 1, cursor.y, COLOR_BRIGHT_SLOW);
			break;
		case prcCursorTypeCrosshair:
			line(screen, cursor.x + 1, cursor.y + (cursor_size.y / 2) - 1, cursor.x + cursor_size.x - 2, cursor.y + (cursor_size.y / 2) - 1, COLOR_BRIGHT_SLOW);
			line(screen, cursor.x + (cursor_size.x / 2) - 1, cursor.y + 1, cursor.x + (cursor_size.x / 2) - 1, cursor.y + cursor_size.y - 2, COLOR_BRIGHT_SLOW);
			break;
		case prcCursorTypeUnderscore:
			line(screen, cursor.x, cursor.y + cursor_size.y - 1, cursor.x + cursor_size.x - 1, cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
			break;
		case prcCursorTypeBlock:
			rectfill(screen, cursor.x, cursor.y, cursor.x + cursor_size.x - 1, cursor.y + cursor_size.y - 1, COLOR_BRIGHT_SLOW);
			break;
		};

	cursor_held = 0;
//	screen_changed = 0;
}

int whndCursorQuery (void)
{
	return(cursor_type);
}

void err_char (char _input)
{
	static int screen_coord = 0;
	char buffer[2];

	if ((-1 == screen_size.x) || (-1 == screen_size.y))
	{
//		printf("%s\n", _report);
		return;
	};

	buffer[1] = 0;
	if (NULL != errfile)
		fprintf(errfile, "%c", _input);

	if ('\t' == _input)
		screen_coord = (screen_coord & 0xFFFC0) + 64;
	else if ('\n' == _input)
		screen_coord += 1024;
	else
	{
		buffer[0] = _input;
		textout(screen, font, buffer, screen_coord, screen_size.y - 8, COLOR_BRIGHT_STEADY);
		screen_coord += 8;
	};

	if (screen_size.x <= screen_coord)
	{
		blit(screen,screen,0,8,0,0,screen_size.x, 120);
		blit(screen,screen,0, 640, 0, 120, screen_size.x, 8);
		blit(screen,screen,0, 640 + 8, 0, 640, screen_size.x, 120);
		rectfill(screen,0,762,screen_size.x - 1, 767, COLOR_INVIS);
		screen_coord = 0;
	};
}


void DBG (char *_buffer, ...)
{
	static char buffer[256];
	static int active = 0;
	int loop;
//	, flag = 0, loop_inner, flag_inner, digit, value;
	va_list arguments;


	if ((-1 == screen_size.x) || (-1 == screen_size.y))
	{
//		printf("%s", _buffer);
		return;
	};

	if (_buffer[0] == 0)
	{
		if (active)
		{
			clear_to_color(screen, COLOR_INVIS);
			active = 0;
		}
		else
			active = 1;
	};

	if (!active)
		return;

	va_start(arguments, _buffer);

	vsprintf(buffer, _buffer, arguments);

	va_end(arguments);

	for (loop = 0; buffer[loop]; loop++)
		if ((buffer[loop] < ' ') && ('\n' != buffer[loop]) && ('\t' != buffer[loop]))
		{
			err_char('<');
			if (9 < (buffer[loop] / 16))
				err_char('A' + (buffer[loop] / 16) - 10);
			else
				err_char('0' + (buffer[loop] / 16));
			if (9 < (buffer[loop] % 16))
				err_char('A' + (buffer[loop] % 16) - 10);
			else
				err_char('0' + (buffer[loop] % 16));
			err_char('>');
		}
		else
			err_char(buffer[loop]);
/*
	for (loop = 0; _buffer[loop]; loop++)
	{
		switch(_buffer[loop])
		{
		case '%':
			if (flag)
				err_char('%');
			else
				flag = 1;
			break;
		case 'c':
			if (flag)
			{
				value = va_arg(arguments, int);
				if (' ' > value)
				{
					err_char('<');
					if (10 <= (value / 16))
						err_char('A' + (value / 16) - 10);
					else
						err_char('0' + (value / 16));
					if (10 <= (value % 16))
						err_char('A' + (value % 16) - 10);
					else
						err_char('0' + (value / 16));
					err_char('>');
				}
				else
					err_char(value & 0x7F);
				flag = 0;
			}
			else
				err_char('c');
			break;
		case 'i':
		case 'd':
			if (flag)
			{
				value = va_arg(arguments, int);
				if (value < 0)
				{
					err_char('-');
					value = -value;
				};
				flag_inner = 0;
				for (loop_inner = 10000; loop_inner > 0; loop_inner /= 10)
				{
					digit = (value / loop_inner) % 10;
					if (flag_inner || digit)
					{
						err_char('0' + ((value / loop_inner) % 10));
						flag_inner = 1;
					};
				};
				if (!flag_inner)
					err_char('0');
				flag = 0;
			}
			else
				err_char(_buffer[loop]);
			break;
		default:
			err_char(_buffer[loop]);
		};
	};
	va_end(arguments);
	*/
}

bool
whndGetClock (void)
{
    if (timer_pulses < 29)
        return(False);
    timer_pulses = 0;
    return(True);
}
/*
void
whndDrawText(
    detailDef _detail,
    int _column,
    int _row,
    bool _cursor)
{
    if ((0 > _column) || (0 > _row) || (MAX_COLS <= _column) ||
        (MAX_ROWS <= _row))
        return;

    if (DETAIL_FIELD_BLINK == (_detail & DETAIL_FIELD_BLINK))
        _detail &= (~DETAIL_FIELD_INTENSITY);

    if (!isgraph(DETAIL_FIELD_DATA & _detail))
    {
        _detail &= (~DETAIL_FIELD_DATA);
        _detail |= ' ';
    };

    _detail &= (DETAIL_FIELD_BLINK | DETAIL_FIELD_INTENSITY |
        DETAIL_FIELD_FONT | DETAIL_FIELD_DATA);

    if (_cursor)
        _detail |= DETAIL_FIELD_CURSOR;

    if (_detail != last_drawn[_column][_row])
    {
        if (polling_inactive)
        {
            polling_column = _column;
            polling_row = _row;
            polling_inactive = False;
        };
        last_drawn[_column][_row] = _detail | DETAIL_FIELD_REPAINT;
    };
}

void
whndRenderText (void)
{
    char buffer[MAX_COLS + 1];
    int buffer_count = 0,
        detail = 0,
        x,
        y,
        w;

    if (polling_inactive)
        return;

    while (True)
    {
        if (last_drawn[polling_column][polling_row] & DETAIL_FIELD_REPAINT)
            if (buffer_count)
                if (last_drawn[polling_column][polling_row] == detail)
                {
                    buffer[buffer_count++] = last_drawn[polling_column][polling_row] & DETAIL_FIELD_DATA;
                    last_drawn[polling_column][polling_row] &= (~DETAIL_FIELD_REPAINT);
                }
                else
                    break;
            else
            {
                start_row = polling_row;
                start_column = polling_column;
                detail = last_drawn[polling_column][polling_row];
                buffer[buffer_count++] = last_drawn[polling_column][polling_row] & DETAIL_FIELD_DATA;
                last_drawn[polling_column][polling_row] &= (~DETAIL_FIELD_REPAINT);
            }
        else if (buffer_count)
            break;

        polling_column++;
        if (MAX_COLS <= polling_column)
        {
            polling_column = 0;
            polling_row++;
            if (MAX_ROWS <= polling_row)
                polling_row = 0;
            if (buffer_count)
                break;
        };

        if ((polling_column == start_column) && (polling_row == start_row))
        {
    	    blit(map_screen,
	            screen,
	            block_pt_1_x,
                block_pt_1_y,
                block_pt_1_x,
                block_pt_1_y + vertical_offset,
                block_pt_2_x - block_pt_1_x,
                block_pt_2_y - block_pt_1_y);

            block_pt_1_x = screen_size.x;
            block_pt_2_x = 0;
            block_pt_1_y = WHND_SCREEN_VISUAL_HEIGHT;
            block_pt_2_y = 0;
            polling_inactive = True;
            return;
        };
    };

    buffer[buffer_count] = 0;

    if (whatCursor(detail))
        text_mode(COLOR_CURSOR);
    else
        text_mode(COLOR_INVIS);

    x = start_column * WHND_TEXT_WIDTH;
    y = start_row * WHND_TEXT_HEIGHT;
    w = buffer_count * WHND_TEXT_WIDTH;

    textout(map_screen,
        data_file[font_pos[whatFont(detail)]].dat,
        buffer,
        x,
        y,
        whatColor(detail));

    if (x < block_pt_1_x)
        block_pt_1_x = x;
    if ((x + w) > block_pt_2_x)
        block_pt_2_x = x + w;
    if (y < block_pt_1_y)
        block_pt_1_y = y;
    if ((y + WHND_TEXT_HEIGHT) > block_pt_2_y)
        block_pt_2_y = y + WHND_TEXT_HEIGHT;
}
*/

int whndQueryFontSize (int _font, int *_x, int *_y)
{
	if ((0 > _font) || (8 <= _font))
	{
		(*_x) = -1;
		(*_y) = -1;
		return(1);
	};

	(*_x) = font_maps[_font].size.x;
	(*_y) = font_maps[_font].size.y;
	return(0);
}

void whndSnapshot(int _count)
{/*
	PALETTE pal;
	char filename[80];
	sprintf(filename, "ss%04d.bmp", _count);
	get_palette(pal);
	save_bmp(filename, screen, pal); */
}

static void
snapshot (void)
{
 //   FILE *fptr;
 //   int row;
 //   int column;

  //  fptr = fopen("vpscrn.txt" "a");
  //  if (NULL == fptr)
        return;

   // for (row = 0; MAX_ROWS > row; row++)
   // {
   //     for (column = 0; MAX_COLS > column; column++)
   //         if (isgraph(last_drawn[column][row] & DETAIL_FIELD_DATA))
     //           fprintf(fptr, "%c" last_drawn[column][row] & DETAIL_FIELD_DATA);
     //       else
      //          fprintf(fptr, ".");
     //   fprintf(fptr, "\n");
   // };

   // fprintf(fptr, "\n\n");

   // fclose(fptr);
}

bool
whndInit(void)
{
	int loop;

//	printf("7x9:");
//	whndValidateFont(font_design_7_9, 8, 16, 32, 127);
//	printf("\n5x7r:");
//	whndValidateFont(font_design_5_7_rulings, 6, 10, 96, 127);
//	printf("\n7x9r:");
//	whndValidateFont(font_design_7_9_rulings, 8, 16, 96, 127);
//return(0);

	param_fonts = paramAddTag(NULL, "SYMBOLIC", 0);
    param_fonts = paramAddTag(param_fonts, "NORMAL", 1);
    param_fonts = paramAddTag(param_fonts, "UNDERLINE", 2);

    configAddOption(CONFIG_OPTION_VISUAL_TOUCH, param_enable, &visual_touch);
    configAddOption(CONFIG_OPTION_FORE_RED, param_32, foreground_colors + WHND_COLOR_RED);
    configAddOption(CONFIG_OPTION_FORE_GREEN, param_32, foreground_colors + WHND_COLOR_GREEN);
    configAddOption(CONFIG_OPTION_FORE_BLUE, param_32, foreground_colors + WHND_COLOR_BLUE);
    configAddOption(CONFIG_OPTION_BACK_RED, param_32, background_colors + WHND_COLOR_RED);
    configAddOption(CONFIG_OPTION_BACK_GREEN, param_32, background_colors + WHND_COLOR_GREEN);
    configAddOption(CONFIG_OPTION_BACK_BLUE, param_32, background_colors + WHND_COLOR_BLUE);
    configAddOption(CONFIG_OPTION_VOFFSET, param_96, &vertical_offset);
    configAddOption(CONFIG_OPTION_BELL_FRQ, param_96, &bell_frequency);
    configAddOption(CONFIG_OPTION_BELL_TIME, param_16, &bell_duration);
    configAddOption(CONFIG_OPTION_KB_REDIRECT, param_bool, &keyboard_redirect);
    configAddOption(CONFIG_OPTION_FONT_STANDARD, param_fonts, font_pos);
    configAddOption(CONFIG_OPTION_FONT_ALTERNATE, param_fonts, font_pos + 1);

	for (loop = 0; loop < MAX_FLASH; loop++)
		flash[loop].active = 0;

	for (loop = 0; loop < HELD_MAPS_MAX; loop++)
		held_maps[loop] = NULL;

	return(True);
}

void pulse_clock(void)
{
    timer_pulses++;
}

END_OF_FUNCTION(pulse_clock)



bool
whndStart (void)
{
    if (-1 == allegro_init())
	{
		sprintf(failure_mode, "Allegro failed to initialize.");
	    return (False);
	};

    LOCK_VARIABLE(timer_pulses);
    LOCK_FUNCTION(pulse_clock);

    data_file = load_datafile("vp_graph.dat");

    if (NULL == data_file)
	{
		sprintf(failure_mode, "Data file not present.");
	    return (False);
	};


    set_color_depth(8);


//	printf("Starting screen");

	if (0 != set_gfx_mode(GFX_AUTODETECT, 1024, 768, 0, 0))
	{
		sprintf(failure_mode, "Cannot set graphics mode.");
		return (False); //set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, screen_size.x, screen_size.y, 0, 0);
	};

	screen_size.x = SCREEN_W;
	screen_size.y = SCREEN_H;


//	map_screen = create_bitmap(screen_size.x, screen_size.y);
	cursor_block = create_bitmap(32,40); // ???? Put in variables for maximum possible cursor size
	clear_to_color(cursor_block, COLOR_INVIS);

    if (-1 == install_timer())
	{
		sprintf(failure_mode, "Timer cannot install. %dx%d", screen_size.x, screen_size.y);
	    return (False);
	};

    if (0 != install_int(pulse_clock, 1))
	{
		sprintf(failure_mode, "Clock pulse not installed. %dx%d", screen_size.x, screen_size.y);
        return(False);
	};

    if (-1 == install_keyboard())
	{
		sprintf(failure_mode, "Keyboard not installed. %dx%d", screen_size.x, screen_size.y);
		return (False);
	};


	if (-1 == install_mouse())
	{
		sprintf(failure_mode, "Mouse not installed. %dx%d", screen_size.x, screen_size.y);
		return (False);
	};

    whndPaletteUpdate();

//    clear_to_color(map_screen, COLOR_INVIS);
//	return(False);
    clear_to_color(screen, COLOR_INVIS);
    set_color(COLOR_INDICATOR, color_fields + WHND_BLACK);
//    circlefill(screen, vertical_offset >> 1, screen_size.y - (vertical_offset >> 1), vertical_offset >> 2, COLOR_INDICATOR);

//	line (screen, visible_area_x1, visible_area_y1 -1 , visible_area_x2, visible_area_y1 - 1, COLOR_BRIGHT_STEADY);
//	line (screen, visible_area_x1, visible_area_y2 + 1, visible_area_x2, visible_area_y2 + 1, COLOR_BRIGHT_STEADY);
//	rectfill(screen, visible_area_x1, visible_area_y1 - 1, visible_area_x2, visible_area_y2 + 1, COLOR_BRIGHT_STEADY);
/*
	hatch_select = 1;

	hatch_fill[0] = create_bitmap(4,4);
	clear_to_color(hatch_fill[0], COLOR_INVIS);

	hatch_fill[1] = create_bitmap(4,4);
	clear_to_color(hatch_fill[1], COLOR_BRIGHT_STEADY);

	hatch_fill[2] = create_bitmap(4,4);
	clear_to_color(hatch_fill[2], COLOR_INVIS);
	line(hatch_fill[2], 1, 0, 1, 3, COLOR_BRIGHT_STEADY);

	hatch_fill[3] = create_bitmap(4,4);
	clear_to_color(hatch_fill[3], COLOR_INVIS);
	line(hatch_fill[3], 0, 1, 3, 1, COLOR_BRIGHT_STEADY);

	hatch_fill[4] = create_bitmap(4,4);
	clear_to_color(hatch_fill[4], COLOR_INVIS);
	line(hatch_fill[4], 3, 0, 0, 3, COLOR_BRIGHT_STEADY);

	hatch_fill[5] = create_bitmap(4,4);
	clear_to_color(hatch_fill[5], COLOR_INVIS);
	line(hatch_fill[5], 0, 3, 3, 0, COLOR_BRIGHT_STEADY);

	hatch_fill[6] = create_bitmap(4,4);
	clear_to_color(hatch_fill[6], COLOR_INVIS);
	putpixel(hatch_fill[6], 1, 1, COLOR_BRIGHT_STEADY);
*/
	bell_x1 = 590;
	bell_x2 = 620;
	bell_y1 = 425;
	bell_y2 = 455;

//	blit((BITMAP *) data_file[4].dat, screen, 0, 0, bell_x1, bell_y1, 30, 30);

	bell_active = 0;


//	BITMAP* test_buf;
//	test_buf = create_bitmap(200,200);
//	textout(test_buf, font, "This is a test" 10, 10, COLOR_BRIGHT_STEADY);
//	stretch_blit(test_buf, screen, 10, 10, 30, 8, 60, 10, 60, 16);

//	destroy_bitmap(test_buf);

//	flash_area_add(5, 10, 10, 80, 20);
//	blit(font_maps[0].map, screen, 0, 0, 0, 256, 1024, 160);

#ifdef DEBUGGING
//	show_mouse(screen);
	errfile = fopen("ferror.txt","w");
#endif

//	whndBuildAllFonts();
	DBG("Graphics %d,%c,%c", 230, 75, 27);
	//whndTestPattern(2);
	int loop;
	for (loop = 0; loop < 8; loop++)
		DBG("\tFont #%d: %dx%d", loop, font_maps[loop].size.x, font_maps[loop].size.y);
  return (True);
}
/*
void whndPrint (char *_report, int _inverse_flag)
{
	static int screen_coord = 0;
	char tbuff[2];
	int loop;

	if ((-1 == screen_size.x) || (-1 == screen_size.y))
	{
//		printf("%s\n", _report);
		return;
	};

	tbuff[1] = 0;
	for (loop = 0; _report[loop]; loop++)
	{
		if (NULL != errfile)
			fprintf(errfile, "%c", _report[loop]);

		if ('\t' == _report[loop])
			screen_coord = (screen_coord & 0xFFFC0) + 64;
		else if ('\n' == _report[loop])
			screen_coord += 1024;
		else
		{
			tbuff[0] = _report[loop];
			if (_inverse_flag)
			{
				text_mode(COLOR_BRIGHT_STEADY);
				textout(screen, font, tbuff, screen_coord, screen_size.y - 8, COLOR_INVIS);
				text_mode(COLOR_INVIS);
			}
			else
			{
				text_mode(COLOR_INVIS);
				textout(screen, font, tbuff, screen_coord, screen_size.y - 8, COLOR_BRIGHT_STEADY);
			};
			screen_coord += 8;
		};

		if (screen_size.x <= screen_coord)
		{
			blit(screen,screen,0,8,0,0,screen_size.x,whndQueryY(0) - 8);
			blit(screen,screen,0,whndQueryY(visual_frame.y - 1) + 1, 0, whndQueryY(0) - 8, screen_size.x, 8);
			blit(screen,screen,0,whndQueryY(visual_frame.y - 1) + 9, 0, whndQueryY(visual_frame.y - 1) + 1, screen_size.x, screen_size.y - whndQueryY(visual_frame.y - 1) - 8);
			rectfill(screen,0,screen_size.y - 8,screen_size.x, screen_size.y - 1, COLOR_INVIS);
			screen_coord = 0;
		};
	};
}
*/
/*
void
whndError(
    char *_report)
{

	return;

    if (_report[0] == 0)
    {
        error.x = 0;
        error.y = 256 * 2;
        return;
    };

    text_mode(COLOR_INVIS);
//    if (error.y >= vertical_offset)
//        y = error.y + WHND_SCREEN_VISUAL_HEIGHT;
//    else
//        y = error.y;

	textout(screen, font, _report, error.x, error.y, COLOR_BRIGHT_STEADY);
    error.x += (screen_size.x >> 2);

    if (error.x >= screen_size.x)
    {
        error.x = 0;
        error.y += 8;
        if (error.y >= screen_size.y)
            error.y = 256 * 2;
    };
}
*/

void whndDrawTouchPoint (int _x, int _y, int _flag)
{
/*	int color;
	if (_flag)
		color = COLOR_BRIGHT_STEADY;
	else 
		color = COLOR_INVIS;

	line(screen, _x - 10, _y, _x + 10, _y, color);
	line(screen, _x, _y - 10, _x, _y + 10, color);
*/
}

void whndDrawButton (int _x1, int _y1, int _x2, int _y2)
{
//	int x1,x2,y1,y2;

//	x1 = clip(lcx((min(_x1,_x2) * 8) + 1), 0, screen_size.x - 1);
//	x2 = clip(lcx((max(_x1,_x2) * 8) - 2), 0, screen_size.x - 1);
//	y1 = clip(lcy((max(_y1,_y2) * 8) + 1), 0, screen_size.y - 1);
//	y2 = clip(lcy((min(_y1,_y2) * 8) - 2), 0, screen_size.y - 1);

//	line(screen, x1 + 1, y1, x2 - 1, y1, COLOR_BRIGHT_STEADY);
//	line(screen, x1, y1 + 1, x1, y2 - 1, COLOR_BRIGHT_STEADY);
//	line(screen, x1 + 1, y2, x2 - 1, y2, COLOR_BRIGHT_STEADY);
//	line(screen, x2, y1 + 1, x2, y2 - 1, COLOR_BRIGHT_STEADY);
}

/*

void perform_graphics_command(int *_values, unsigned char *_string, int _count, int _index)
{
	int x1,y1,x2,y2;
//	char tbuff[80];
//	int loop;

	int start_y;
	BITMAP* write_buffer;
	

	switch(_values[0])
	{
		case 0:
			rectfill(screen, visible_area_x1, visible_area_y1, visible_area_x2, visible_area_y2, COLOR_INVIS);
			return;
		case 1:
			gsx = clip(lcx(_values[2]), 0, (screen_size.x - 1));
			gsy = clip(lcy(_values[1]), 0, (screen_size.y - 1));
			return;
		case 8:
			x1 = clip(min(lcx(_values[2]), lcx(_values[4])), 0, (screen_size.x - 1));
			y1 = clip(min(lcy(_values[3]), lcy(_values[1])), 0, (screen_size.y - 1));
			x2 = clip(max(lcx(_values[2]), lcx(_values[4])), 0, (screen_size.x - 1));
			y2 = clip(max(lcy(_values[3]), lcy(_values[1])), 0, (screen_size.y - 1));

			drawing_mode(DRAW_MODE_COPY_PATTERN, hatch_fill[hatch_select], 0, 0);
			rectfill(screen, x1,y1 ,x2,y2, COLOR_BRIGHT_STEADY);
			drawing_mode(DRAW_MODE_SOLID, hatch_fill[1], 0, 0);

			if (0 == hatch_select)
			{
			line(screen, x1, y1 - 1, x2, y1 - 1, COLOR_INVIS);
			line(screen, x1 - 1, y1, x1 - 1, y2, COLOR_INVIS);
			line(screen, x2 + 1, y1, x2 + 1, y2, COLOR_INVIS);
			line(screen, x1, y2 + 1, x2, y2 + 1, COLOR_INVIS);
			}
			else
				{
			line(screen, x1, y1 - 1, x2, y1 - 1, COLOR_BRIGHT_STEADY);
			line(screen, x1 - 1, y1, x1 - 1, y2, COLOR_BRIGHT_STEADY);
			line(screen, x2 + 1, y1, x2 + 1, y2, COLOR_BRIGHT_STEADY);
			line(screen, x1, y2 + 1, x2, y2 + 1, COLOR_BRIGHT_STEADY);
				};

			flash_areas_update(x1, y1, x2, y2);

			return;
		case 14:
			if (_count > 1)
			{
				gsx = clip(lcx(_values[2]), 0, (screen_size.x - 1));
				gsy = clip(lcy(_values[1]), 0, (screen_size.y - 1));
			};
//			gsx_start = gsx;

//			if (((_index * font_width[font_size]) + gsx) > 639)
//					_index = (639 - gsx) / font_width[font_size];

//	test_buf = create_bitmap(200,200);
//	textout(test_buf, font, "This is a test" 10, 10, COLOR_BRIGHT_STEADY);
//	stretch_blit(test_buf, screen, 10, 10, 30, 8, 60, 10, 60, 16);

			_string[_index] = 0;

			write_buffer = create_bitmap(30 * _index, 40);
			clear_to_color(write_buffer, COLOR_INVIS);
			if (attribute_reverse)
			{
				text_mode(COLOR_BRIGHT_STEADY);
				textout(write_buffer, data_file[2].dat, _string, 0, 0, COLOR_INVIS);
				text_mode(COLOR_INVIS);
			}
			else
				textout(write_buffer, data_file[2].dat, _string, 0, 0, COLOR_BRIGHT_STEADY);

			start_y = gsy - (attribute_size * 10);

			stretch_blit(write_buffer, screen, 0, 0, _index * 30, 40,
				gsx, start_y + 2, (_index * 30 * attribute_size) / 4, (attribute_size * 10) - 2);

			if (attribute_reverse)
				rectfill(screen, gsx, start_y, gsx + (_index * 30 * attribute_size) / 4, start_y + 1, COLOR_BRIGHT_STEADY);
			else
				rectfill(screen, gsx, start_y, gsx + (_index * 30 * attribute_size) / 4, start_y + 1, COLOR_INVIS);

			
			destroy_bitmap(write_buffer);


/*			if (attribute_reverse)
			{
				text_mode(COLOR_BRIGHT_STEADY);
				if (2 == attribute_size)
					textout(screen, data_file[0].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_INVIS);
				else if (3 == attribute_size)
					textout(screen, data_file[1].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_INVIS);
				else if (4 == attribute_size)
					textout(screen, data_file[2].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_INVIS);
				else
					textout(screen, font, _string, gsx, gsy + 5 - (attribute_size * 15) / 2, COLOR_INVIS);
				text_mode(COLOR_INVIS);
			}
			else
				if (2 == attribute_size)
					textout(screen, data_file[0].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_BRIGHT_STEADY);
				else if (3 == attribute_size)
					textout(screen, data_file[1].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_BRIGHT_STEADY);
				else if (4 == attribute_size)
					textout(screen, data_file[2].dat, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_BRIGHT_STEADY);
				else
					textout(screen, font, _string, gsx, gsy + 5 - (attribute_size * 25) / 2, COLOR_BRIGHT_STEADY);
*/
/*			if (attribute_underline)
				line(screen, gsx, start_y + (attribute_size * 10) - 1, gsx + (_index * 30 * attribute_size) / 4, start_y + (attribute_size * 10) - 1, COLOR_BRIGHT_STEADY);

			flash_areas_update(gsx, start_y, gsx + ((_index * 30 * attribute_size) / 4) - 1, start_y + (attribute_size * 10) - 1);

			gsx = gsx + ((_index * 30 * attribute_size) / 4);


//			textout(screen, data_file[1].dat, _string, gsx, gsy, COLOR_BRIGHT_STEADY); return;
//			gsx += (_index * 8);
			return;
		case 16: 
			if (1 < _count)
			{
				attribute_size = _values[1] + 1;
				if (2 < _count)
				{
					attribute_reverse = (_values[2] != 0);
					if (4 < _count)
						attribute_underline = (_values[4] != 0);
				};
			};
//			sprintf(tbuff, "Set %d,sz%d,rv%d,ud%d" _count, attribute_size, attribute_reverse, attribute_underline);
//			whndError(tbuff);
		return; // Character attribute
		case 19:
//			sprintf(tbuff, "Set Hatch #%d" _values[1]);
//			whndError(tbuff);
			if ((1 <= _values[1]) && (MAX_HATCH > _values[1]))
				hatch_select = _values[1];
			return;
		case 20: return; // Raster mode select
		case 30:

			flash_area_add(_values[1],
				lcx(_values[4]),
				lcy(_values[5]),
				lcx(_values[6]),
				lcy(_values[3]) );

			return; // Define a flash area
		case 32:
			flash_area_deactivate(_values[1]);

//			sprintf(tbuff, "Stop Flash Area %d" _values[1]); whndError(tbuff);
			return; // Stop an area flashing
	};
}		

*/
/*
void whndBellSelect (int _x, int _y)
{

	if ((-1 == _x) && (-1 == _y))
	{
		if (bell_tagged)
		{
			if (bell_active)
			{
				blit((BITMAP *) data_file[4].dat, screen, 0, 0, bell_x1, bell_y1, 30, 30);
				bell_active = 0;
			}
			else
			{

				blit((BITMAP *) data_file[3].dat, screen, 0, 0, bell_x1, bell_y1, 30, 30);
				bell_active = 1;
			};
			bell_tagged = 0;
		};
	}
	else if ((_x >= bell_x1) && (_x <= bell_x2) && (_y >= bell_y1) && (_y <= bell_y2))
	{
		bell_tagged = 1;
	}
	else
		bell_tagged = 0;
}
*/

int clip (int _v, int _low, int _high)
{
	if (_v < _low)
		return (_low);
	else if (_v > _high)
		return (_high);
	else
		return (_v);
}
/*
void flash_area_add (int _index, int _x1, int _y1, int _x2, int _y2)
{
	if (MAX_FLASH <= _index)
		return;

	flash[_index].x1 = clip(min(_x1,_x2), 0, (screen_size.x - 1));
	flash[_index].y1 = clip(min(_y1,_y2), 0, (screen_size.y - 1));
	flash[_index].x2 = clip(max(_x1,_x2), 0, (screen_size.x - 1));
	flash[_index].y2 = clip(max(_y1,_y2), 0, (screen_size.y - 1));
	flash[_index].active = 1;
	
	flash_areas_update(flash[_index].x1, flash[_index].x2, flash[_index].y1, flash[_index].y2);
}

void flash_areas_update (int _x1, int _y1, int _x2, int _y2)
{
	int loop, loop_x, loop_y;
	int within_x, within_y;
	int check_color;

	for (loop = 0; MAX_FLASH > loop; loop++)
	{
		if (flash[loop].active)
		{
			within_y = ((_y1 >= flash[loop].y1) && (_y1 <= flash[loop].y2)) ||
				((_y2 >= flash[loop].y1) && (_y2 <= flash[loop].y2));
			within_x = ((_x1 >= flash[loop].x1) && (_x1 <= flash[loop].x2)) ||
				((_x2 >= flash[loop].x1) && (_x2 <= flash[loop].x2));
			if (within_y && within_x)
				for (loop_x = flash[loop].x1; loop_x <= flash[loop].x2; loop_x++)
					for (loop_y = flash[loop].y1; loop_y <= flash[loop].y2; loop_y++)
					{
						check_color = _getpixel(screen, loop_x, loop_y);
						if ((COLOR_BRIGHT_STEADY == check_color) || (COLOR_BRIGHT_SLOW == check_color))
							_putpixel(screen, loop_x, loop_y, COLOR_BRIGHT_SLOW);
						else
							_putpixel(screen, loop_x, loop_y, COLOR_CURSOR);
					};
		};
	};
}

void flash_area_deactivate (int _index)
{
	int loop_x, loop_y, color_check;

	if (MAX_FLASH <= _index)
		return;

	flash[_index].active = 0;
	for (loop_x = flash[_index].x1; loop_x <= flash[_index].x2; loop_x++)
		for (loop_y = flash[_index].y1; loop_y <= flash[_index].y2; loop_y++)
		{
			color_check = _getpixel(screen, loop_x, loop_y);
			if (COLOR_BRIGHT_SLOW == color_check)
				_putpixel(screen, loop_x, loop_y, COLOR_BRIGHT_STEADY);
			else
				_putpixel(screen, loop_x, loop_y, COLOR_INVIS);
		};

}

*/

static char
key_translate(
    int _key)
{
    if (0 == (_key & 0xFF))
	{
	    switch (_key >> 8)
	    {
	    case KEY_LEFT:
	        return (COMMAND_LEFT_CURSOR);
	    case KEY_RIGHT:
	        return (COMMAND_RIGHT_CURSOR);
	    case KEY_UP:
	        return (COMMAND_UP_CURSOR);
	    case KEY_DOWN:
	        return (COMMAND_DOWN_CURSOR);
	    case KEY_DEL:
	        return (COMMAND_RUBOUT);
        case KEY_END:
            mainExit(FLAG_SHELL);
            return(0);
        case KEY_PGUP:
            snapshot();
            return(0);
	    default:
	        return (0);
	    };
	}
    else
    	return (_key & 0xFF);
}

void
whndPulseCounter (void)
{
    pulse_counter++;
    pulse_counter &= 0xFFFF;
}

void
whndBellStart (void)
{
    if (bell_active)
		sound_timer = bell_duration;
}

void
whndBellUpdate (void)
{
    if (sound_timer > 0)
    {
        if (sound_timer == bell_duration)
            sound(bell_frequency);
        sound_timer--;
    }
    else
        nosound();
}

void
whndTouchIndicator (
    bool _flag)
{
    if (_flag == touch_light_visible)
        return;

    if (visual_touch)
        if (_flag)
            set_color(COLOR_INDICATOR, color_fields + WHND_WHITE);
        else
            set_color(COLOR_INDICATOR, color_fields + WHND_BLACK);
    else if (!_flag)
        set_color(COLOR_INDICATOR, color_fields + WHND_BLACK);

    touch_light_visible = _flag;
}

int whndPaletteShift (int _red, int _green, int _blue)
{
	foreground_colors[WHND_COLOR_RED] = _red;
	foreground_colors[WHND_COLOR_GREEN] = _green;
	foreground_colors[WHND_COLOR_BLUE] = _blue;
	return(0);
}

void
whndPaletteUpdate (void)
{
    color_fields[WHND_WHITE].r = (foreground_colors[WHND_COLOR_RED] & 0x1F) << 1;
    color_fields[WHND_GRAY].r = (foreground_colors[WHND_COLOR_RED] & 0x1F);
    color_fields[WHND_BLACK].r = (background_colors[WHND_COLOR_RED] & 0x1F) << 1;
    color_fields[WHND_WHITE].g = (foreground_colors[WHND_COLOR_GREEN] & 0x1F) << 1;
    color_fields[WHND_GRAY].g = (foreground_colors[WHND_COLOR_GREEN] & 0x1F);
    color_fields[WHND_BLACK].g = (background_colors[WHND_COLOR_GREEN] & 0x1F) << 1;
    color_fields[WHND_WHITE].b = (foreground_colors[WHND_COLOR_BLUE] & 0x1F) << 1;
    color_fields[WHND_GRAY].b = (foreground_colors[WHND_COLOR_BLUE] & 0x1F);
    color_fields[WHND_BLACK].b = (background_colors[WHND_COLOR_BLUE] & 0x1F) << 1;

    set_color(COLOR_BRIGHT_STEADY, color_fields + WHND_WHITE);
    set_color(COLOR_DIM_STEADY, color_fields + WHND_GRAY);
    set_color(COLOR_INVIS, color_fields + WHND_BLACK);
    set_color(COLOR_INVIS_2, color_fields + WHND_BLACK);

    switch(pulse_counter & 0x0F)
    {
    case 0x00:
	        set_color(COLOR_CURSOR, color_fields + WHND_BLACK);
	        set_color(COLOR_BRIGHT_SLOW, color_fields + WHND_WHITE);
	        set_color(COLOR_BRIGHT_FAST, color_fields + WHND_WHITE);
	        set_color(COLOR_DIM_SLOW, color_fields + WHND_GRAY);
	        set_color(COLOR_DIM_FAST, color_fields + WHND_GRAY);
	        break;
    case 0x04:
    case 0x0C:
	        set_color(COLOR_BRIGHT_FAST, color_fields + WHND_BLACK);
	        set_color(COLOR_DIM_FAST, color_fields + WHND_BLACK);
	        break;
    case 0x08:
	        set_color(COLOR_CURSOR, color_fields + WHND_WHITE);
	        set_color(COLOR_BRIGHT_SLOW, color_fields + WHND_BLACK);
	        set_color(COLOR_BRIGHT_FAST, color_fields + WHND_WHITE);
	        set_color(COLOR_DIM_SLOW, color_fields + WHND_BLACK);
	        set_color(COLOR_DIM_FAST, color_fields + WHND_GRAY);
	        break;
    };

//	whndReadSpecialSerial();
}

void whndReadSpecialSerial (void)
{
	/*
	static int last_pulse = -1;

	if ((last_pulse / 8) != (pulse_counter / 8))
	{
//		whndTestLoop(0);
		digital_io();
		last_pulse = pulse_counter;
	}
	*/
}

//int last_mouse_x = -1;
//int last_mouse_y = -1;

int last_mouse_x = -1;
int last_mouse_y = -1;

void whndPollMouse (int *_x, int *_y, int _mode) //int _resolution, int _latch)
{
	int resolution_x, resolution_y, x, y;

	if ((LatchedHigh == _mode) || (UnlatchedHigh == _mode))
//		_resolution)
	{
		resolution_x = 63;
		resolution_y = 31;
	}
	else
	{
		resolution_x = 32;
		resolution_y = 16;
	};

	if (0 == poll_mouse())
	{
		if (mouse_b & 1)
		{
			x = mouse_x;
			y = mouse_y;
			if ((x < (base_x())) || (x > (where_x(visual_frame.x - 1, LowerRight) + base_y())) || (y < (base_y())) || (y > (base_y() + where_y(visual_frame.y - 1, LowerRight))))
			{
				(*_x) = -1;
				(*_y) = -1;
				last_mouse_x = -1;
				last_mouse_y = -1;
			}
			else
			{
				x -= base_x();
				y -= base_y();
				x /= ((visual_frame.x * scaling()) / resolution_x);
				y /= ((visual_frame.y * scaling()) / resolution_y);
				switch(_mode)
				{
				case LatchedHigh:
				case LatchedLow:
					if ((last_mouse_x == -1) && (last_mouse_y == -1))
					{
						(*_x) = x;
						(*_y) = y;
						DBG("\tTouch @ %d,%d", x, y);
//						putpixel(screen, mouse_x, mouse_y, COLOR_BRIGHT_FAST);
					}
					else
					{
//						whndPrint("[Same                  ]",0);
						(*_x) = -1;
						(*_y) = -1;
					};
					break;
				case UnlatchedHigh:
				case UnlatchedLow:
					if ((last_mouse_x != x) || (last_mouse_y != y))
					{
						DBG("\tDrag @ %d,%d", x, y);
						(*_x) = x;
						(*_y) = y;
//					putpixel(screen, mouse_x, mouse_y, COLOR_BRIGHT_FAST);
					}	
					else
					{
//						whndPrint("[Same Touch]",0);
						(*_x) = -1;
						(*_y) = -1;
					};
					break;
				default:
					(*_x) = -1;
					(*_y) = -1;
					break;
				};
				last_mouse_x = x;
				last_mouse_y = y;
			};
		}
		else
		{
			if ((last_mouse_x != -1) && (last_mouse_y != -1) && ((UnlatchedLow == _mode) || (UnlatchedHigh == _mode)))
			{
				(*_x) = 63;
				(*_y) = 63;
			}
			else
			{
				(*_x) = -1;
				(*_y) = -1;
			};


			if ((last_mouse_x != -1) || (last_mouse_y != -1))
				DBG("Unclick");	
			last_mouse_x = -1;
			last_mouse_y = -1;
		};
	}
	else
	{
		(*_x) = -1;
		(*_y) = -1;
//		last_mouse_x = -1;
//		last_mouse_x = -1;
	};
}

void whndKeyboardUpdate (void)
{
	char value;

    if (keypressed())
{
	    value = readkey() & 0xFF; //key_translate(readkey());
        clear_keybuf();
//		digital_io();
//		if (prcStepControl(STEP_RESUME))
		if (26 == value)
			prcPause();
//		whndTestLoop(-1);
		else if (25 == value)
		{
			serialSetBrightness(0);
		}
		else
			queuePush(parse_queue, value);
		/*
		if (value < 32)
		{
			sprintf(error_buffer, "<0x%02X>", value);
		}
		else
			sprintf(error_buffer, "%c", value);
		whndPrint(error_buffer, 1);
*/
    //    if (value)
      //  {
        //    if (keyboard_redirect)
          //      queuePush(parse_queue, value);
        //    else
        //        processKeypress(value);
      //  };
	};
}

void whndShowFailureMode (void)
{
	printf("%s", failure_mode);
}

void whndSetCursor (int _cursor_enable, int _cursor_type, int _x, int _y, int _size_x, int _size_y)
{
	static int x = -1;
	static int y = -1;
	static int sx = -1;
	static int sy = -1;

	if ((_cursor_enable != cursor_enable) ||
		(_cursor_type != cursor_type) ||
		(_x != x) ||
		(_y != y) ||
		(_size_x != sx) ||
		(_size_y != sy))
	{
//		DBG("SetCursor:");
		whndHoldCursor();
		cursor_enable = _cursor_enable;
		cursor_type = _cursor_type;
		cursor.x = where_x(_x, UpperLeft);
		cursor.y = where_y(_y, UpperLeft);
		cursor_size.x = _size_x * scaling();
		cursor_size.y = _size_y * scaling();
		x = _x;
		y = _y;
		sx = _size_x;
		sy = _size_y;
		whndRestoreCursor();
		update_screen = 1;
	};
}

void put_font_pixel (int _index, int _glyph, int _x, int _y, int _color)
{
	if ((-1 == font_maps[_index].size.y) || (-1 == font_maps[_index].size.x))
		return;

	if ((_y == (font_maps[_index].size.y * 9 / 10)) || _color)
	{
		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(1 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(1 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_STEADY);

		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(3 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(3 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_SLOW);
	}
	else
	{
		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(5 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(5 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_STEADY);

		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(7 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(7 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_SLOW);
	};

	if (_color)
	{
		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(0 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(0 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_STEADY);

		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(2 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(2 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_SLOW);

	}
	else
	{
		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(4 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(4 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_STEADY);

		rectfill(font_maps[_index].map,
			(_glyph * font_maps[_index].size.x + _x) * scaling(),
			(6 * font_maps[_index].size.y + _y) * scaling(),
			(_glyph * font_maps[_index].size.x + _x + 1) * scaling() - 1,
			(6 * font_maps[_index].size.y + _y + 1) * scaling() -1,
			COLOR_BRIGHT_SLOW);
	};
}

int whndBuildFont (int _index, char *_buffer)
{
	int x = -1, y = -1, glyph = -1;
	int buffer_count = 0;

	while (1)
	{
		switch(_buffer[buffer_count++])
		{
		case '=':
			if (-1 == font_maps[_index].size.x)
				font_maps[_index].size.x = x + 1;
			else if ((x + 1) != font_maps[_index].size.x)
			{
				DBG("\tInvalid width index %d glyph %d", _index, glyph);
				return(1);
			};
			put_font_pixel(_index, glyph, x, y, 0);
			x = 0;
			y++;
			break;
		case '-':
			put_font_pixel(_index, glyph, x, y, 0);
			x++;
			break;
		case '*':
			if (-1 == font_maps[_index].size.x)
				font_maps[_index].size.x = x + 1;
			else if ((x + 1)!= font_maps[_index].size.x)
			{
				DBG("\tInvalid width index %d glyph %d", _index, glyph);
				return(1);
			};
			put_font_pixel(_index, glyph, x, y, 1);
			x = 0;
			y++;
			break;
		case 'x': // full
			put_font_pixel(_index, glyph, x, y, 1);
			x++;
			break;
		case 0:
			if (1 == buffer_count)
				return(1);
		case '@':
			if (-1 == font_maps[_index].size.y)
			{
				if (0 <= y)
				{
					font_maps[_index].size.y = y;
					font_maps[_index].map = create_bitmap(font_maps[_index].size.x * 128 * scaling(), font_maps[_index].size.y * 8 * scaling());
					DBG("\tCreated font map %dx%d", font_maps[_index].size.x, font_maps[_index].size.y);
					clear_to_color(font_maps[_index].map, COLOR_INVIS);
					buffer_count = 1;
				}
				else
				{
					DBG("\tno Y value");
				};
			}
			else if ((y != font_maps[_index].size.y) && (-1 != y))
			{
				DBG("\tInvalid height index %d glyph %d %d/%d", _index, glyph, y, font_maps[_index].size.y);
				return(1);
			};

			if (_buffer[buffer_count - 1])
			{
				x = 0;
				y = 0;
				glyph = _buffer[buffer_count++];
				if (!glyph)
				{
					DBG("Invalid glyph");
					return(1);
				}
				else
					DBG("%d%c ", _index,glyph);
			}
			else
				return(0);
			break;
		};
	};
}
/*
void whndBuildAllFonts (void)
{
	int loop;

	for (loop = 0; loop < 8; loop++)
	{
		font_maps[loop].map = NULL;
		font_maps[loop].size.x = -1;
		font_maps[loop].size.y = -1;
	};

	whndBuildFont(0, font_design_7_9);
	whndBuildFont(1, font_design_7_9_rulings);
	whndBuildFont(2, font_design_5_7);
	whndBuildFont(3, font_design_5_7_rulings);
	whndBuildFont(4, "");
	whndBuildFont(5, "");
	whndBuildFont(6, font_design_10_14);
	whndBuildFont(7, font_design_10_14_rulings);
}
*/

/*
void whndBuildFonts (int _index, char *_buffer, int _size_x, int _size_y, int _start_c, int _end_c)
{
	int loop_c, loop_x, loop_y;
	int index;

	font_maps[_index].size.x = _size_x;
	font_maps[_index].size.y = _size_y;
	font_maps[_index].map = create_bitmap(_size_x * 128 * scaling(), _size_y * 8 * scaling());
	clear_to_color(font_maps[_index].map, COLOR_INVIS);
	for (loop_c = _start_c; loop_c <= _end_c; loop_c++)
	{
		for (loop_x = 0; loop_x < _size_x; loop_x++)
		{
			for (loop_y = 0; loop_y < _size_y; loop_y++)
			{
				index = ((loop_c - _start_c) * _size_x * _size_y) + (loop_y * _size_x) + loop_x;
				if (('x' == _buffer[index]) || ('*' == _buffer[index]))
				{
					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 0,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 0,
						COLOR_BRIGHT_STEADY);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 1,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 1 ,
						COLOR_BRIGHT_STEADY);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 2,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 2,
						COLOR_BRIGHT_SLOW);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 3,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 3,
						COLOR_BRIGHT_SLOW);
*/ /*
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 1), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 2), COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 3), COLOR_BRIGHT_SLOW);

					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 1), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 2), COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 3), COLOR_BRIGHT_SLOW);

					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 1) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 2) + 1, COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 3) + 1, COLOR_BRIGHT_SLOW);

					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 1) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 2) + 1, COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 3) + 1, COLOR_BRIGHT_SLOW);
			*/
			/*
				}
				else
				{
					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 4,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 4,
						COLOR_BRIGHT_STEADY);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 5,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 5,
						COLOR_BRIGHT_STEADY);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 6,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 6,
						COLOR_BRIGHT_STEADY);

					rectfill(font_maps[_index].map,
						(loop_c * _size_x + loop_x) * scaling(),
						loop_y * scaling() + _size_y * scaling() * 7,
						(loop_c * _size_x + loop_x + 1) * scaling() - 1,
						(loop_y + 1) * scaling() - 1 + _size_y * scaling() * 7,
						COLOR_BRIGHT_STEADY);
*/ /*
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 4), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 5), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 6), COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 7), COLOR_BRIGHT_SLOW);

					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 4), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 5), COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 6), COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 7), COLOR_BRIGHT_SLOW);

					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 4) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 5) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 6) + 1, COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2, loop_y * 2 + (_size_y * 2 * 7) + 1, COLOR_BRIGHT_SLOW);
				
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 4) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 5) + 1, COLOR_BRIGHT_STEADY);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 6) + 1, COLOR_BRIGHT_SLOW);
					putpixel(font_maps[_index].map, loop_c * _size_x * 2 + loop_x * 2 + 1, loop_y * 2 + (_size_y * 2 * 7) + 1, COLOR_BRIGHT_SLOW);
					*/
					/*
				};
			};
		};
	};

	line(font_maps[_index].map, 0, (1 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), (128 + 1) * _size_x * scaling() - 1, (1 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), COLOR_BRIGHT_STEADY);
	line(font_maps[_index].map, 0, (3 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), (128 + 1) * _size_x * scaling() - 1, (3 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), COLOR_BRIGHT_SLOW);
	line(font_maps[_index].map, 0, (5 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), (128 + 1) * _size_x * scaling() - 1, (6 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), COLOR_INVIS);
	line(font_maps[_index].map, 0, (7 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), (128 + 1) * _size_x * scaling() - 1, (7 * _size_y * scaling()) + (_size_y * scaling() * 9 / 10), COLOR_INVIS);
}

*/
// starts at 96
