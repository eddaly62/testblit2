#ifndef VUEPOINT_WHND
#define VUEPOINT_WHND

#include "vuepoint.h"
#include "vuepoint_detail.h"
#include "vuepoint_queue.h"

//#define CURSOR_UNKNOWN 0
//#define CURSOR_LSHAPE 1
//#define CURSOR_CROSSHAIR 2
//#define CURSOR_UNDERSCORE 3
//#define CURSOR_BLOCK 4

#define visible_area_y1 80
#define visible_area_y2 399
#define visible_area_x1 0
#define visible_area_x2 639

void DBG(char *, ...);

enum {UpperLeft, LowerRight};
enum {MAP_DIAG_PASSED = 0, MAP_HATCHED, MAP_ALL_WHITE, MAP_NORMAL_SETS, MAP_INVIS_SETS, MAP_VISUAL, MAP_HOLD};

typedef union
{
	struct
	{
		unsigned char underline :1;
		unsigned char blink :1;
		unsigned char reverse :1;
		unsigned char overstrike :1;
		unsigned char unused :4;
	} bit;
	unsigned char word;
} attributeT;

enum {TouchDisabled, LatchedLow, LatchedHigh, UnlatchedLow, UnlatchedHigh};
extern int whndDisplayHold (int);

extern void whndSnapshot(int);
extern int whndTestLoop (int);
extern int whndBlockMove (int, int, int, int, int, int);
extern int whndBlockUpdate (void);

extern int whndQueryFontSize (int, int *, int *);
extern int whndBuildFont (int, char *);
extern int whndBlockHold (int, int, int, int);
extern int whndBlockRestore (int, int);
extern int whndBlockClear (int, int, int, int);
extern int whndDrawCircle(int, int, int, int, int, int, int, int, int);
extern void whndSetScreenSize (int, int);
extern int whndTestPattern(int);
extern int whndPaletteShift (int, int, int);

extern void whndHoldCursor(void);
extern void whndRestoreCursor(void);
//extern int whndPCGWrite (int, int, int, int);
//extern void whndBellSelect (int, int);
//extern void whndClearBlock(int, int, int, int);
//extern int whndScaling (void);
//extern int whndQueryY (int);
//extern int whndQueryX (int);
extern int whndLoadFont(int, char *, int, int);
extern int whndSetGraphicWindow (int, int, int, int);

extern int whndDrawPoint (int, int, int);

extern int whndCursorQuery (void);
//extern void whndDrawText(detailDef, int, int, bool);
//extern int whndCopyBlock(int, int, int, int);
//extern int whndWriteBlock(int, int, int);
//extern void whndPrint(char *, int);
extern void whndDrawButton (int, int, int, int );
//extern int whndClearScreen(void);
extern bool whndInit(void);
extern void whndDestroy(void);
extern bool whndStart(void);
//extern void whndError(char *);
extern void whndKeyboardUpdate (void);
extern void whndPulseCounter (void);
extern void whndBellUpdate (void);
extern void whndPaletteUpdate (void);
extern void whndBellStart (void);
extern bool whndGetClock (void);
extern void whndTouchIndicator (bool);
//extern void whndRenderText (void);
extern void perform_graphics_command(int *, unsigned char *, int, int);
extern void whndDrawTouchPoint (int, int, int);
extern void whndPollMouse(int *, int *, int);
extern void whndShowFailureMode(void);
extern void whndSetCursor (int, int, int, int, int, int);

extern int whndPrintChar (char, int, int, int, int);

#endif
