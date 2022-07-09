#include <stdio.h>
#include <stdlib.h>
#include "vuepoint.h"
#include "vuepoint_callbacks.h"
#include "vuepoint_serial.h"
#include "vuepoint_whnd.h"
#include "vuepoint_parser.h"
#include "errno.h"

#define maximum_number_of_tabs		15
#define maximum_number_of_windows	5
#define data_map_storage_limit		2125
#define rf_buffer_size				263

enum {ResetAll, ResetRead, ResetWrite, ResetTransmit, ResetReadObject, ResetWriteObject, ReadFlag, ResetClose, WriteFlag, FlagSave};

char *font_design_7_9;
char *font_design_5_7;
char *font_design_5_7_rulings;
char *font_design_7_9_rulings;
char *font_design_10_14;
char *font_design_10_14_rulings;
coordT circle_offsets[360];
coordT visual_size = {512,256};

int data_files_count = 0;
int bitmaps_count = 0;
int mode_touch = TouchDisabled;
int exit_flag = 0;
unsigned int ws;
int global_dash_param = 0xFFFF;
int processing_flag = 1;

struct screen_type
{
	coordT cursor;
	coordT offset;
	coordT maximum;
	unsigned int htab_stop[maximum_number_of_tabs];
	unsigned int vtab_stop[maximum_number_of_tabs];

	union
	{
		struct
		{
			unsigned int mode_fpl :1;
			unsigned int mode_lnm :1;
			unsigned int mode_edit :1;
			unsigned int mode_pnl :1;
			unsigned int cursor_on :1;
			unsigned int cursor_type :3;
			unsigned int font_primary :3;
			unsigned int font_secondary :3;
			unsigned int secondary_select :1;
		} bit;
		unsigned int word :32;
	} flags;

	attributeT attributes;

	char text_map[data_map_storage_limit];

	unsigned int window_active :1;
};

struct screen_type wnd[maximum_number_of_windows];



int flag_step_hold = 0;
void prcUpdate (void);

void prcPause (void);
void strip_offsets (int *, int *);
char *compose_file_name (char *, int, int);
void advance_line (int);
int prcStepControl (int);
int handle_data_file (int, int *);
int handle_incremental_plot (int, int);
int handle_monitor_mode (int);
int handle_pcg (int, int);
int handle_raster (int, int);
int handle_repeat_file(int, int *);
int handle_touch_response (int);
int prcProcess (void);
int query_column(int);
int query_font_columns (void);
int query_font_height (void);
int query_font_rows (void);
int query_font_width (void);
int query_row(int);
int query_x_coord(int, int);
int query_font (void);
int query_x (int, int);
int query_y (int, int);

int query_y_coord(int, int);
void init_window (int, int, int);

void prcCircleDraw (int *);
void prcClearFileBuffer (int *);
void prcClearTabs (int *);
void prcCombineBitMap (int *);
void prcControlBrightness (int *);
void prcCursorReposition (int *);
void prcCursor_Backspace (int *);
void prcCursor_CarriageReturn (int *);
void prcCursor_Down (int *);
void prcCursor_Formfeed (int *);
void prcCursor_Home(int *);
void prcCursor_Left (int *);
void prcCursor_Linefeed (int *);
void prcCursor_Right (int *);
void prcCursor_Tab (int *);
void prcCursor_Up (int *);
void prcCursor_VTab (int *);
void prcDashParam (int *);
void prcDebugOff (int *);
void prcDefineBitMap (int *);
void prcDefineWindow (int *);
void prcDeleteChar (int *);
void prcDeleteFile (int *);
void prcDeleteRow (int *);
void prcVectGraph (int *);
void prcDeleteWindow (int *);
void prcDetectBitMap (int *);
void prcDisableBitMap (int *);
void prcDisableCursor (int *);
void prcDisableFullPageLatch (int *);
void prcDisableNewLine (int *);
void prcDisableOverstrike (int *);
void prcDisablePseudoNewLine (int *);
void prcDisableScrollEdit (int *);
void prcDisableTouchPanel (int *);
void prcDisableXON (int *);
void prcDisplayBitMap (int *);
void prcEnableBitMap (int *);
void prcEnableBlockCursor (int *);
void prcEnableCrosshair (int *);
void prcEnableCursor (int *);
void prcEnableFullPageLatch (int *);
void prcEnableIncrementalPlot (int *);
void prcEnableMonitorMode (int *);
void prcEnableNewLine (int *);
void prcEnableOverstrike (int *);
void prcEnablePrimaryFont (int *);
void prcEnablePseudoNewLine (int *);
void prcEnableScrollEdit (int *);
void prcEnableSecondaryFont (int *);
void prcEnableTouchPanelContHigh (int *);
void prcEnableTouchPanelContinuous (int *);
void prcEnableTouchPanelLatch (int *);
void prcEnableTouchPanelLatchHigh (int *);
void prcEnableUnderscore (int *);
void prcEnableXON (int *);
void prcEnterChar (int *);
void prcExit (int *);
void prcInsertChar (int *);
void prcInsertRow (int *);
void prcLoadPCGSymbol (int *);
void prcMoveAbsolute (int *);
void prcObjTblDraw (int *);
void prcObjTblStart (int *);
void prcProcessFile (int *);
void prcRasterWrite (int *);
void prcRepeatLoop (int *);
void prcRequests(int *);
void prcSaveFile (int *);
void prcSelectBitMap (int *);
void prcSelectFonts (int *);
void prcSelectSerialPort (int *);
void prcSelectWindow (int *);
void prcSendTouchPanelSize (int *);
void prcSetBaudRate (int *);
void prcSetFontAttribute (int *);
void prcSetGraphicWindow (int *);
void prcSetSerialValues(int *);
void prcSetTab(int *);
void prcSetVertTab (int *);
void prcSnapshot (int *);
void prcSoftwareReset(int *);
void prcSplitScreen (int *);
void prcStartFile (int *);
void prcStartFileProcess (int *);
void prcStartRepeat (int *);
void prcTestPattern(int *);
void prcTransmitPage (int *);
void prcTransmitRow (int *);
void prcUpdateOff (int *);
void prcUpdateOn (int *);
void prcUpdateRange (int *);
void send_to_screen (int, char *);
void set_graphic_window (int *);
void swap (int *, int *);
void swap_window (int, int);
void vector_graphics (int *);
void reset_window (int);
void reset_cursor (int);
void reset_screen (int);
void reset_tabs (int);
void reset_data_map (int);
int pattern_match (int, int, int);

void swap (int *_value_1, int *_value_2)
/**	\author Len Blado
	\version 1.0
	\date 01-May-2008
	\invariant Invoked
	\return None

	Swaps two integer values.
*/
{
	int value;

	value = (*_value_1);
	(*_value_1) = (*_value_2);
	(*_value_2) = value;
}

void reset_data_map (int _window)
/**	\author Len Blado
	\version 1.0
	\date 01-May-2008
	\invariant Invoked
	\return None

	Resets the values stored for a given window and clears the window display.
*/
{
	int loop;
	DBG(" Data map");
	for (loop = 0; data_map_storage_limit > loop; loop++)
		wnd[_window].text_map[loop] = -1;
	DBG(" HC");
	whndHoldCursor();
	DBG(" BlkClr");
	whndBlockClear(
		wnd[_window].offset.x,
		wnd[_window].offset.y,
		wnd[_window].offset.x + wnd[_window].maximum.x - 1,
		wnd[_window].offset.y + wnd[_window].maximum.y - 1);
	DBG(" RC");
	whndRestoreCursor();
}

void reset_tabs (int _window)
/**	\author Len Blado
	\version 1.0
	\date 01-May-2008
	\invariant Invoked
	\return None

	Clears all tab stops for the window.
*/
{
	int loop;

	DBG(" Tabs");
	for (loop = 0; maximum_number_of_tabs > loop; loop++)
	{
		wnd[_window].htab_stop[loop] = -1;
		wnd[_window].vtab_stop[loop] = -1;
	};
}

void reset_screen (int _window)
{
	reset_cursor(_window);
	reset_tabs(_window);
	reset_data_map(_window);
}

void reset_cursor (int _window)
{
	DBG(" CURSOR");
	wnd[_window].cursor.x = query_x(0, UpperLeft);
	wnd[_window].cursor.y = query_y(0, UpperLeft);
}

void reset_window (int _window)
{
	DBG(" WINDOW");
	wnd[_window].offset.x = 0;
	wnd[_window].offset.y = 0;
	wnd[_window].maximum.x = visual_size.x;
	wnd[_window].maximum.y = visual_size.y;
}

void swap_window (int _window_1, int _window_2)
{
	swap(&(wnd[_window_1].offset.x), &(wnd[_window_2].offset.x));
	swap(&(wnd[_window_1].offset.y), &(wnd[_window_2].offset.y));
	swap(&(wnd[_window_1].maximum.x), &(wnd[_window_2].maximum.x));
	swap(&(wnd[_window_1].maximum.y), &(wnd[_window_2].maximum.y));
}

void init_window (int _window, int _max_x, int _max_y)
{
	int loop;

	wnd[_window].cursor.x = query_x(0, UpperLeft);
	wnd[_window].cursor.y = query_y(0, UpperLeft);
	if (_max_x < 0)
	{
		wnd[_window].offset.x = -_max_x;
		wnd[_window].maximum.x = visual_size.x - wnd[_window].offset.x;
	}
	else
	{
		wnd[_window].offset.x = 0;
		wnd[_window].maximum.x = _max_x;
	};

	if (_max_y < 0)
	{
		wnd[_window].offset.y = -_max_y;
		wnd[_window].maximum.y = visual_size.y - wnd[_window].offset.y;
	}
	else
	{
		wnd[_window].offset.y = 0;
		wnd[_window].maximum.y = _max_y;
	};

	for (loop = 0; maximum_number_of_tabs > loop; loop++)
	{
		wnd[_window].htab_stop[loop] = -1;
		wnd[_window].vtab_stop[loop] = -1;
	};

	wnd[_window].flags.word = 0;
	wnd[_window].attributes.word = 0;
	
	for (loop = 0; loop < data_map_storage_limit; loop++)
		wnd[_window].text_map[loop] = ' ';

	wnd[_window].window_active = 1;

	DBG("\tWindow #%d offset (%d,%d) max (%d,%d) created", _window, wnd[_window].offset.x, wnd[_window].offset.y, wnd[_window].maximum.x, wnd[_window].maximum.y);
	DBG("\tWindow #%d offset (%d,%d) max (%d,%d) created", _window, wnd[_window].offset.x, wnd[_window].offset.y, wnd[_window].maximum.x, wnd[_window].maximum.y);
}


int query_font_rows (void) // inaccurate values
{
	return (query_row(wnd[ws].maximum.y) - 1);
}

int query_font_columns (void) // inaccurate values
{
	return (query_column(wnd[ws].maximum.x) - 1);
}




int query_column (int _x)
{
	return(1 + (_x / query_font_width()));
}

int query_row (int _y)
{
	return(1 + (_y / query_font_height()));
}

int query_x_coord (int _column, int _flag)
{
	return(wnd[ws].offset.x + query_x(_column, _flag));
}

int query_y_coord (int _row, int _flag)
{
	return(wnd[ws].offset.y + query_y(_row, _flag));
}

int query_x (int _column, int _flag)
{
	if (UpperLeft == _flag)
		return (max(0, min(wnd[ws].maximum.x - 1, max(_column, 1) * query_font_width() - 1)));
	else
		return (max(0, min(wnd[ws].maximum.x - 1, (max(_column, 1) + 1) * query_font_width() - 2)));
}

int query_y (int _row, int _flag)
{
	if (UpperLeft == _flag)
		return (max(0, min(wnd[ws].maximum.y - 1, max(_row, 1) * query_font_height() - 1)));
	else
		return (max(0, min(wnd[ws].maximum.y - 1, (max(_row, 1) + 1) * query_font_height() - 2)));
}


int handle_repeat_file (int _flag, int *_input)
{
	static int rf_maximum = 0;
	static int rf_count = -1;
	static int rf_index = -1;
	static char rf_buffer[rf_buffer_size] = "";

	switch (_flag)
	{
	case ReadFlag:
		if (0 < rf_count)
		{
			(*_input) = rf_buffer[rf_index++];
			DBG("{%c:%d:%d:%d}", (*_input), rf_index, rf_count, rf_maximum);
			if (rf_index >= rf_maximum)
			{
				rf_count--;
				rf_index = 0;
				DBG(" RL(%d)", rf_count);
/*				if (0 < rf_count)
				{
					DBG("\tRepeat complete");
					rf_maximum = -1;
					rf_index = -1;
				}
				else
				{
					DBG("\tRestarting Repeat (%d)", rf_count);
					rf_index = 0;
				}; */
			};
			return(1);
		}
		else
			return(0);
	case WriteFlag:
		if ((rf_count < 0) && (rf_index < 0))
			if ((0 <= rf_maximum) && (rf_buffer_size >= rf_maximum))
				rf_buffer[rf_maximum++] = *_input;
		return(0);
	case ResetRead:
		DBG("\tRepeat File Read");
		for (rf_maximum--; (rf_maximum >=0) && (rf_buffer[rf_maximum] != '\033'); rf_maximum--);
		DBG(" Backed up to %d [%c]", rf_maximum, rf_buffer[rf_maximum-1]);
		rf_count = *_input;
		rf_index = 0;
		return(0);
	case ResetWrite:
		DBG("\tRepeat File Write");
		rf_maximum = 0;
		rf_index = -1;
		rf_count = -1;
		return(0);
	default:
		return(0);
	};
}
/*

int handle_rf_read (int *_input)
{
	if (-1 != rf_count)
	{
		(*_input) = rf_buffer[rf_index++];
		if (rf_index >= rf_maximum)
		{
			rf_count--;
			if (-1 == rf_count)
			{
				rf_maximum = -1;
				rf_index = -1;
			}
			else
				rf_index = 0;
		};
		return(1);
	}
	else
		return(0);
}

int handle_rf_write (int _input)
{
	if (-1 != rf_maximum)
	{
		if (256 > rf_maximum)
			rf_buffer[rf_maximum++] = _input;
		return(1);
	}
	else
		return(0);
}
*/


int pattern_match (int _pattern, int _x, int _y)
{
	if (_pattern & (1 << ((_x + _y + 256) % 16)))
		return(1);
	else
		return(0);
}

void line_draw (int _x1, int _y1, int _x2, int _y2, int _mask, int _pattern)
{
	int offset_x, offset_y;
	float slope;
	int dy;
	int dx;
	float t;                      // offset for rounding

	offset_x = wnd[ws].maximum.x * 16;
	offset_y = wnd[ws].maximum.y * 16;
	dy = _y2 - _y1;
	dx = _x2 - _x1;
	t = 0.5;
//	DBG("(%d,%d)->(%d,%d) pattern %d ", _x1, _y1, _x2, _y2, _pattern);

	if (PARSER_INVALID == _pattern)
		_pattern = 0xFFFF;

	if (pattern_match(_pattern, _x1 + wnd[ws].offset.x, _y1 + wnd[ws].offset.y))
		whndDrawPoint(
			((_x1 + offset_x) % wnd[ws].maximum.x) + wnd[ws].offset.x,
			((_y1 + offset_y) % wnd[ws].maximum.y) + wnd[ws].offset.y,
			_mask);

	if (abs(dx) > abs(dy))
	{
		slope = (float) dy / (float) dx;      // compute slope
		t += _y1;
		dx = (dx < 0) ? -1 : 1;
		slope *= dx;
		while (_x1 != _x2)
		{
			_x1 += dx;                           // step to next x value
			t += slope;                             // add slope to y value
			if (pattern_match(_pattern, _x1 + wnd[ws].offset.x, ((int) t) + wnd[ws].offset.y))
				whndDrawPoint(
					((_x1 + offset_x) % wnd[ws].maximum.x) + wnd[ws].offset.x,
					((((int) t) + offset_y) % wnd[ws].maximum.y) + wnd[ws].offset.y,
					_mask);
		}
	}
	else
	{                                    // slope >= 1
		slope = (float) dx / (float) dy;      // compute slope
		t += _x1;
		dy = (dy < 0) ? -1 : 1;
		slope *= dy;
		while (_y1 != _y2)
		{
			_y1 += dy;                           // step to next y value
			t += slope;                             // add slope to x value
			if (pattern_match(_pattern, ((int) t) + wnd[ws].offset.x, _y1 + wnd[ws].offset.y))
				whndDrawPoint(
					((((int) t) + offset_x) % wnd[ws].maximum.x) + wnd[ws].offset.x,
					((_y1 + offset_y ) % wnd[ws].maximum.y) + wnd[ws].offset.y, _mask);
        };
    };
}


int handle_raster (int _input, int _reset_count)
{
	static int raster_count = -1;
//	static int raster_active = 0;
	int color_check;
	int loop;

	if (0 <= _reset_count)
	{
		raster_count = _reset_count - 1;
//		raster_active = 1;
		DBG("\nActivated Raster mode %d counts ", raster_count);
		return(1);
	}
	else if (-1 == _input)
		return(1);
	else if (0 <= raster_count)
	{
		whndHoldCursor();
//		DBG("\tDrawing @ (%d,%d) %d:", wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, _input);
		for (loop = 0; loop < 8; loop++)
		{
			color_check = ((_input & 0x80) != 0);
//			DBG("%d", color_check);
			whndDrawPoint(wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, color_check);
			_input <<= 1;
			wnd[ws].cursor.x++;
			if (wnd[ws].cursor.x >= wnd[ws].maximum.x)
			{
				wnd[ws].cursor.x = 0;
				wnd[ws].cursor.y++;
				if (wnd[ws].cursor.y >= wnd[ws].maximum.y)
					wnd[ws].cursor.y = 0;
			};
		};
		whndRestoreCursor();
		raster_count--;
		DBG("RG(%d)", raster_count);
//		if (raster_count < 0)
//			raster_active = 0;
		return(1);
	}
	else
		return(0);
}


void send_to_screen (int _flag, char *_string)
{
	int values[2];
	int loop;

	values[1] = -1;
	for (loop = 0; _string[loop]; loop++)
	{
		if (_flag)
		{
			serialSend(_string[loop]);
		}
		else
		{
			values[0] = _string[loop];
			prcEnterChar(values);
		};
	};
}

int handle_monitor_mode (int _input)
{
	static int last_input = 0;
	static int flag_active = 0;
	static int flag_serial = 0;
	int values[2];

	if (-7 == _input)
	{
		flag_active = 1;
		flag_serial = 0;
		DBG("\tActvating Monitor Mode");
		return(0);
	}
	else if (-8 == _input)
	{
		flag_active = 1;
		flag_serial = 1;
		return(0);
	}
	else if (flag_active)
//	if (parserF_MonitorMode)
	{
		switch (_input)
		{
		case 0: send_to_screen(flag_serial, "NUL,"); break;
		case 1: send_to_screen(flag_serial, "SOH,"); break;
		case 2: send_to_screen(flag_serial, "STX,"); break;
		case 3: send_to_screen(flag_serial, "ETX,"); break;
		case 4: send_to_screen(flag_serial, "EOT,"); break;
		case 5: send_to_screen(flag_serial, "ENQ,"); break;
		case 6: send_to_screen(flag_serial, "ACK,"); break;
		case 7: send_to_screen(flag_serial, "BEL,"); break;
		case 8: send_to_screen(flag_serial, "BS,"); break;
		case 9: send_to_screen(flag_serial, "TAB,"); break;
		case 10: send_to_screen(flag_serial, "LF,"); break;
		case 11: send_to_screen(flag_serial, "VT,"); break;
		case 12: send_to_screen(flag_serial, "FF,"); break;
		case 13: send_to_screen(flag_serial, "CR,"); break;
		case 14: send_to_screen(flag_serial, "SO,"); break;
		case 15: send_to_screen(flag_serial, "SI,"); break;
		case 16: send_to_screen(flag_serial, "DLE,"); break;
		case 17: send_to_screen(flag_serial, "DC1,"); break;
		case 18: send_to_screen(flag_serial, "DC2,"); break;
		case 19: send_to_screen(flag_serial, "DC3,"); break;
		case 20: send_to_screen(flag_serial, "DC4,"); break;
		case 21: send_to_screen(flag_serial, "NAK,"); break;
		case 22: send_to_screen(flag_serial, "SYN,"); break;
		case 23: send_to_screen(flag_serial, "ETB,"); break;
		case 24: send_to_screen(flag_serial, "CAN,"); break;
		case 25: send_to_screen(flag_serial, "EM,"); break;
		case 26: send_to_screen(flag_serial, "SUB,"); break;
		case 27:
			if (last_input == 27)
			{
				DBG("\tDeactivating Monitor Mode");
				flag_active = 0;
//				parserF_MonitorMode = 0;
				last_input = 0;
				return(0);
			}
			else
				send_to_screen(flag_serial, "ESC,");
			break;
		case 28: send_to_screen(flag_serial, "FS,"); break;
		case 29: send_to_screen(flag_serial, "GS,"); break;
		case 30: send_to_screen(flag_serial, "RS,"); break;
		case 31: send_to_screen(flag_serial, "US,"); break;
		case 127: send_to_screen(flag_serial, "DEL,"); break;
		default:
			values[0] = _input;
			values[1] = -1;
			prcEnterChar(values);

//			serialSendString(txt_buffer);
			break;
		};
		last_input = _input;
		return(1);
	}
	else
		return(0);
}

int prcInit(void)
{
	whndSetScreenSize(visual_size.x, visual_size.y);
	whndBuildFont(0, font_design_7_9);
	whndBuildFont(1, font_design_7_9_rulings);
	whndBuildFont(2, font_design_5_7);
	whndBuildFont(3, font_design_5_7_rulings);
	whndBuildFont(4, "");
	whndBuildFont(5, "");
	whndBuildFont(6, font_design_10_14);
	whndBuildFont(7, font_design_10_14_rulings);

	parserInit();






	parserAddSequence(1,"%P", &prcEnterChar);
	parserAddSequence(1,"\001", &prcCursor_Home); // SOH
#ifdef DEBUGGING
	parserAddSequence(1,"\002", &prcStepMode);
	parserAddSequence(1,"\003", &prcExit);
	parserAddSequence(1,"\004", &prcDebugOff);
	parserAddSequence(1,"\005", &prcSnapshot);
#endif
	parserAddSequence(1,"\010", &prcCursor_Backspace); //BS
	parserAddSequence(1,"\011", &prcCursor_Tab); // HT
	parserAddSequence(1,"\012", &prcCursor_Linefeed); //LF
	parserAddSequence(1,"\013", &prcCursor_VTab); //VT
	parserAddSequence(1,"\014", &prcCursor_Formfeed); //FF
	parserAddSequence(1,"\015", &prcCursor_CarriageReturn); //CR
	parserAddSequence(1,"\016", &prcEnableSecondaryFont);
	parserAddSequence(1,"\017", &prcEnablePrimaryFont);
	parserAddSequence(1,"\033!", &prcSendTouchPanelSize);
	parserAddSequence(1,"\0330", &prcDisableOverstrike);
	parserAddSequence(1,"\0331", &prcEnableOverstrike);
	parserAddSequence(1,"\0332", &prcClearFileBuffer);
	parserAddSequence(1,"\0333", &prcEnableCursor);
	parserAddSequence(1,"\0334", &prcDisableCursor);
	parserAddSequence(1,"\0335", &prcEnableFullPageLatch);
	parserAddSequence(1,"\0336", &prcDisableFullPageLatch);
	parserAddSequence(1,"\0337", &prcStartRepeat);
	parserAddSequence(1,"\0338", &prcStartFileProcess);
	parserAddSequence(1,"\0339", &prcStartFile);
	parserAddSequence(1,"\033=", &prcEnableUnderscore);
	parserAddSequence(1,"\033>", &prcEnableBlockCursor);
	parserAddSequence(1,"\033?", &prcEnableCrosshair);
	parserAddSequence(1,"\033H", &prcSetTab);
	parserAddSequence(1,"\033J", &prcSetVertTab);
	parserAddSequence(1,"\033[%d'", &prcDashParam);
	parserAddSequence(1,"\033[%d;%d D", &prcSelectFonts);  // ?
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%d;%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%d;%dp", &prcSetSerialValues);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%dT", &prcDefineWindow);
	parserAddSequence(1,"\033[%d;%d;%d;%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%d;%dM", &prcCombineBitMap);
	parserAddSequence(1,"\033[%d;%d;%dl", &prcDisplayBitMap);

	parserAddSequence(1,"\033[%d;%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%dM", &prcCombineBitMap);
	parserAddSequence(1,"\033[%d;%dX", &prcUpdateRange);
	parserAddSequence(1,"\033[%d;%dc", &prcCircleDraw);
	parserAddSequence(1,"\033[%d;%dd", &prcCursorReposition);
	parserAddSequence(1,"\033[%d;%df", &prcMoveAbsolute);
	parserAddSequence(1,"\033[%d;%dl", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%d;%dt", &prcRasterWrite);

	parserAddSequence(1,"\033[%d;%d;%dr", &prcSetGraphicWindow);

	parserAddSequence(1,"\033[%s;%s;%d;%so", &prcVectGraph);
	parserAddSequence(1,"\033[%s;%s;%do", &prcVectGraph);
/*
	parserAddSequence(1,"\033[%s%d;%s%d;%d;-%do", &prcVectGraph_AA_n);
	parserAddSequence(1,"\033[%d;%d;%do", &prcVectGraph_AA_p);
	parserAddSequence(1,"\033[%d;+%d;%d;%do", &prcVectGraph_AP_p);
	parserAddSequence(1,"\033[%d;+%d;%d;-%do", &prcVectGraph_AP_n);
	parserAddSequence(1,"\033[%d;+%d;%do", &prcVectGraph_AP_p);
	parserAddSequence(1,"\033[%d;+%d;%dr", &prcSetGraphicWindow_AP);
	parserAddSequence(1,"\033[%d;-%d;%d;%do", &prcVectGraph_AN_p);
	parserAddSequence(1,"\033[%d;-%d;%d;-%do", &prcVectGraph_AN_n);
	parserAddSequence(1,"\033[%d;-%d;%do", &prcVectGraph_AN_p);
	parserAddSequence(1,"\033[+%d;%d;%d;%do", &prcVectGraph_PA_p);
	parserAddSequence(1,"\033[+%d;%d;%d;-%do", &prcVectGraph_PA_n);
	parserAddSequence(1,"\033[+%d;%d;%do", &prcVectGraph_PA_p);
	parserAddSequence(1,"\033[+%d;%d;%dr", &prcSetGraphicWindow_PA);
	parserAddSequence(1,"\033[+%d;+%d;%d;%do", &prcVectGraph_PP_p);
	parserAddSequence(1,"\033[+%d;+%d;%d;-%do", &prcVectGraph_PP_n);
	parserAddSequence(1,"\033[+%d;+%d;%do", &prcVectGraph_PP_p);
	parserAddSequence(1,"\033[+%d;+%d;%dr", &prcSetGraphicWindow_PP);
	parserAddSequence(1,"\033[+%d;-%d;%d;%do", &prcVectGraph_PN_p);
	parserAddSequence(1,"\033[+%d;-%d;%d;-%do", &prcVectGraph_PN_n);
	parserAddSequence(1,"\033[+%d;-%d;%do", &prcVectGraph_PN_p);
	parserAddSequence(1,"\033[+%d;-%d;%dr", &prcSetGraphicWindow_PN);
	parserAddSequence(1,"\033[-%d;%d;%d;%do", &prcVectGraph_NA_p);
	parserAddSequence(1,"\033[-%d;%d;%d;-%do", &prcVectGraph_NA_n);
	parserAddSequence(1,"\033[-%d;%d;%do", &prcVectGraph_NA_p);
	parserAddSequence(1,"\033[-%d;%d;%dr", &prcSetGraphicWindow_NA);
	parserAddSequence(1,"\033[-%d;+%d;%d;%do", &prcVectGraph_NP_p);
	parserAddSequence(1,"\033[-%d;+%d;%d;-%do", &prcVectGraph_NP_n);
	parserAddSequence(1,"\033[-%d;+%d;%do", &prcVectGraph_NP_p);
	parserAddSequence(1,"\033[-%d;+%d;%dr", &prcSetGraphicWindow_NP);
	parserAddSequence(1,"\033[-%d;-%d;%d;%do", &prcVectGraph_NN_p);
	parserAddSequence(1,"\033[-%d;-%d;%d;-%do", &prcVectGraph_NN_n);
	parserAddSequence(1,"\033[-%d;-%d;%do", &prcVectGraph_NN_p);
	parserAddSequence(1,"\033[-%d;-%d;%dr", &prcSetGraphicWindow_NN);
*/

	parserAddSequence(1,"\033[%d>h", &prcEnableTouchPanelLatch); // value must be 3 for high, empty for low
	parserAddSequence(1,"\033[%d?h", &prcEnableTouchPanelContinuous); // value must be 3
	parserAddSequence(1,"\033[%dA", &prcCursor_Up);
	parserAddSequence(1,"\033[%dB", &prcCursor_Down); 
	parserAddSequence(1,"\033[%dC", &prcCursor_Right);
	parserAddSequence(1,"\033[%dD", &prcCursor_Left);
	parserAddSequence(1,"\033[%dF", &prcDefineBitMap);
	parserAddSequence(1,"\033[%dG", &prcSelectWindow);
	parserAddSequence(1,"\033[%dH", &prcDetectBitMap); // if no entry, then set tab
	parserAddSequence(1,"\033[%dI", &prcDisplayBitMap);
	parserAddSequence(1,"\033[%dJ", &prcSelectBitMap);
	parserAddSequence(1,"\033[%dK", &prcDeleteWindow);
	parserAddSequence(1,"\033[%d\177", &prcDeleteFile);
	parserAddSequence(1,"\033[%d\\", &prcSelectSerialPort);
	parserAddSequence(1,"\033[%de", &prcLoadPCGSymbol);
	parserAddSequence(1,"\033[%dg", &prcClearTabs);
	parserAddSequence(1,"\033[%dh", &prcEnableMonitorMode); // value must be 3
	parserAddSequence(1,"\033[%dj", &prcTransmitFile);
	parserAddSequence(1,"\033[%dl", &prcDisableNewLine); // value must be 20
	parserAddSequence(1,"\033[%dm", &prcSetFontAttribute);
	parserAddSequence(1,"\033[%dn", &prcRequests);
	parserAddSequence(1,"\033[%do", &prcObjTblDraw);
	parserAddSequence(1,"\033[%dp", &prcSetBaudRate);
	parserAddSequence(1,"\033[%dq", &prcSelectWindow);
	parserAddSequence(1,"\033[%dr", &prcSplitScreen);
	parserAddSequence(1,"\033[%dt", &prcRasterWrite);
	parserAddSequence(5,"\033[%dw", &prcRepeatLoop);
	parserAddSequence(1,"\033[%dx", &prcTestPattern);
	parserAddSequence(1,"\033[x", &prcTestPattern);
	parserAddSequence(1,"\033[%dz", &prcControlBrightness);
	parserAddSequence(1,"\033[%d~", &prcProcessFile);
	parserAddSequence(1,"\033[<2h", &prcEnableBitMap);
	parserAddSequence(1,"\033[<2l", &prcDisableBitMap); // must be 2
	parserAddSequence(1,"\033[<h", &prcEnableScrollEdit);
	parserAddSequence(1,"\033[<l", &prcDisableScrollEdit);
	parserAddSequence(1,"\033[=h", &prcEnableXON);
	parserAddSequence(1,"\033[=l", &prcDisableXON);
	parserAddSequence(1,"\033[>l", &prcDisableTouchPanel);
	parserAddSequence(1,"\033[G", &prcSelectWindow);
	parserAddSequence(1,"\033[K", &prcDeleteWindow);
	parserAddSequence(1,"\033[f", &prcCursor_Home);
	parserAddSequence(1,"\033[s", &prcEnableIncrementalPlot);
	parserAddSequence(1,"\033[{", &prcEnablePseudoNewLine);
	parserAddSequence(1,"\033[|", &prcDisablePseudoNewLine);
	parserAddSequence(1,"\033c", &prcSoftwareReset);
	parserAddSequence(1,"\033g", &prcTransmitRow);
	parserAddSequence(1,"\033h", &prcTransmitPage);
	parserAddSequence(1,"\033l", &prcInsertRow);
	parserAddSequence(1,"\033m", &prcDeleteRow);
	parserAddSequence(1,"\033n", &prcInsertChar);
	parserAddSequence(1,"\033o", &prcDeleteChar); // Unknown value
	parserAddSequence(1,"\033s", &prcUpdateOff);
	parserAddSequence(1,"\033t", &prcUpdateOn);
	parserAddSequence(2,"\033[%d}", &prcSaveFile);
//	parserAddSequence(1,"\033[%d;%d;%do", &prcVectGraph);
//	parserAddSequence(1,"\033[%d;%df", &prcCursorControl);
//	parserAddSequence(1,"\033[%dh", &prcEnableNewLine); // value must be 20
	parserAddSequence(1,"\033[H", &prcSetTab);
	parserAddSequence(1,"\033[o", &prcObjTblStart);
	whndCreateTestMaps();
	prcSoftwareReset(NULL);
//	whndTestPattern(MAP_DIAG_PASSED);

/*
	int loop_1, loop_2;

	visual_size.x = 512;
	visual_size.y = 256;


	for (loop_1 = 0; loop_1 <= 5; loop_1++)
	{
		wnd[loop_1].cursor.x = 0;
		wnd[loop_1].cursor.y = 0;
		wnd[loop_1].offset.x = 0;
		wnd[loop_1].offset.y = 0;
		for (loop_2 = 0; maximum_number_of_tabs > loop_2 ; loop_2++)
		{
			wnd[loop_1].htab_stop[loop_2] = -1;
			wnd[loop_1].vtab_stop[loop_2] = -1;
		};
		wnd[loop_1].flags.word = 0;
		wnd[loop_1].window_active = 0;

		wnd[loop_1].maximum.x = visual_size.x; //query_font_width() * query_font_columns();
		wnd[loop_1].maximum.y = visual_size.y; //query_font_height() * query_font_rows();
	};
	wnd[0].window_active = 1;

	ws = 0; */
	return(1);
}


int handle_touch_response (int _reset_mode)
{
	static int mode_touch = TouchDisabled;
//	static int touch_flag = 0;
	int x =-1,y=-1;

	if (-1 != _reset_mode)
	{
		mode_touch = _reset_mode;
		return(0);
	}
	else
	{
		whndPollMouse(&x, &y, mode_touch); //parserF_TouchHighResolution, parserF_TouchContinuous);
		if ((x != -1) && (y != -1))
		{
			if (TouchDisabled != mode_touch)
			{
				serialSendString("\004%02d;%02d", x, y);
//				sprintf(txt_buffer, "\004%02d;%02d", x, y);
//				serialSendString(txt_buffer);
			};
		};
		return (1);
	};
}


int handle_incremental_plot (int _input, int _reset)
{
	static int active = 0;

	if (-1 != _reset)
	{
		active = 1;
		return(0);
	}
	else if (active)
	{
		DBG("IP:%d %d", _input >> 5, _input & 0x07);
		switch (_input & 0x60)
		{
		case 0x00: active = 0; return(1);
		case 0x20: whndHoldCursor(); whndDrawPoint(wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, 1); whndRestoreCursor(); break;
		case 0x40: whndHoldCursor(); whndDrawPoint(wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, 0); whndRestoreCursor(); break;
		case 0x60: break;
		default: return(1);
		};
		switch (_input & 0x07)
		{
		case 0x00: wnd[ws].cursor.x++; break;
		case 0x01: wnd[ws].cursor.x++; wnd[ws].cursor.y++; break;
		case 0x02: wnd[ws].cursor.y++; break;
		case 0x03: wnd[ws].cursor.x--; wnd[ws].cursor.y++; break;
		case 0x04: wnd[ws].cursor.x--; break;
		case 0x05: wnd[ws].cursor.x--; wnd[ws].cursor.y--; break;
		case 0x06: wnd[ws].cursor.y--; break;
		case 0x07: wnd[ws].cursor.x++; wnd[ws].cursor.y--; break;
		};
		if (wnd[ws].cursor.x < 0)
			wnd[ws].cursor.x = 0;
		if (wnd[ws].cursor.y < 0)
			wnd[ws].cursor.y = 0;
		if (wnd[ws].cursor.x >= wnd[ws].maximum.x)
			wnd[ws].cursor.x = wnd[ws].maximum.x - 1;
		if (wnd[ws].cursor.y >= wnd[ws].maximum.y)
			wnd[ws].cursor.y = wnd[ws].maximum.y - 1;
		switch (_input & 0x60)
		{
		case 0x00: active = 0; return(1);
		case 0x20: whndHoldCursor(); whndDrawPoint(wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, 1); whndRestoreCursor(); break;
		case 0x40: whndHoldCursor(); whndDrawPoint(wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, 0); whndRestoreCursor(); break;
		case 0x60: break;
		default: return(1);
		};
		return(1);
	}
	else
		return(0);
}



void set_graphic_window (int *_values)
{
	int x1, x2, y1, y2;

	if (2 != _values[2])
	{
		DBG("\nInvalid Parameter 3");
		return;
	};

	x1 = min(_values[0], wnd[ws].offset.x + wnd[ws].cursor.x);
	x2 = max(_values[0], wnd[ws].offset.x + wnd[ws].cursor.x);
	y1 = min(_values[1], wnd[ws].offset.y + wnd[ws].cursor.y);
	y2 = max(_values[1], wnd[ws].offset.y + wnd[ws].cursor.y);

	DBG("\nGraphic Window %d,%d %d,%d", x1, y1, x2, y2);

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 >= visual_size.x)
		x2 = visual_size.x - 1;
	if (y2 >= visual_size.y)
		y2 = visual_size.y - 1;

	wnd[4].offset.x = x1;
	wnd[4].offset.y = y1;
	wnd[4].cursor.x = query_x(0, UpperLeft);
	wnd[4].cursor.y = query_y(0, UpperLeft);
	wnd[4].maximum.x = x2 - x1;
	wnd[4].maximum.y = y2 - y1;
	wnd[4].flags.word = wnd[ws].flags.word;
	reset_tabs(4);
	wnd[4].attributes.word = wnd[ws].attributes.word;
	wnd[4].window_active = 1;
}

int handle_pcg (int _input, int _reset_count)
{
	static int pcg_index = -1;
	static int pcg_define = -1;
	static int pcg_read_limit = 0;
	static char pcg_buffer[80];
	int x, loop, offset,length;

	if (-1 == _reset_count)
	{
		if (pcg_read_limit)
		{
			if (-1 == pcg_index)
			{
				sprintf(pcg_buffer, "@%c-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=-------=", _input);
				pcg_define = _input;
			}
			else
			{
				x = pcg_index % 8;
				switch(pcg_index / 8)
				{
				case 0: offset = 15; length = 6; break;
				case 1: offset = 9; length = 6; break;
				case 2: offset = 3; length = 4; break;
				default: offset = -1; length = -1; DBG("\nhandle_pcg: Invalid iteration."); break;
				};
				for (loop = 0; loop < length; loop++)
				{
					if (_input & 1)
					{
						if ('=' == pcg_buffer[(offset - loop) * 8 + 2 + x])
							pcg_buffer[(offset - loop) * 8 + 2 + x] = '*';
						else
							pcg_buffer[(offset - loop) * 8 + 2 + x] = 'x';
					};
					_input >>= 1;
				};
			};
			pcg_index++;
			if (24 <= pcg_index)
			{
				DBG(pcg_buffer);
				whndBuildFont(4, pcg_buffer);
				pcg_index = -1;
				pcg_read_limit--;
				DBG("PCG:%d", pcg_read_limit);
			};
			return(1);
		}
		else
			return(0);
	}
	else
	{
		pcg_index = -1;
		pcg_read_limit = _reset_count;
		pcg_define = -1;
		return(0); 
	};
}

//FILE *df_save_file = NULL;
//FILE *df_load_file = NULL;

//int df_flag_process = 0;
//int df_flag_transmit = 0;

char *compose_file_name (char *_filename, int _flag, int _input)
{
	if (_flag)
		sprintf(_filename, "fo%04d.txt", _input);
	else
		sprintf(_filename, "fd%04d.txt", _input);
	return(_filename);
}

int handle_data_file (int _flag, int *_input)
{
	static char filename[80];
	static FILE *file_save = NULL;
	static FILE *file_load = NULL;
	static int flag_transmit = 0;
//	static int flag_process = 0;
	static int flag_object = 0;
		static int original_primary, original_secondary, original_select;

	switch(_flag)
	{
	case ResetReadObject:
		flag_object = 1;
	case ResetRead:
		if ((0 > (*_input)) || (9999 < (*_input)))
		{
			DBG("\tFile descriptor out of range");
			return(1);
		}
		if (NULL != file_load)
		{
			DBG("\tFile not found");
			fclose(file_load);
		};
//		if (flag_object)
//			sprintf(txt_buffer, "fo%04d.txt", *_input);
//		else
//			sprintf(txt_buffer, "fd%04d.txt", *_input);
		file_load = fopen(compose_file_name(filename, flag_object, *_input), "r");
		flag_transmit = 0;
		original_primary = wnd[ws].flags.bit.font_primary;
		original_secondary = wnd[ws].flags.bit.font_secondary;
		original_select = wnd[ws].flags.bit.secondary_select;
//		wnd[ws].flags.bit.font_primary = 0;
//		wnd[ws].flags.bit.font_secondary = 2;
//		wnd[ws].flags.bit.secondary_select = 0;

//		_flag;
//		DBG("txt_buffer");
		return(0);
	case ResetTransmit:
		if ((0 > (*_input)) || (9999 < (*_input)))
		{
			DBG("\tFile descriptor out of range");
			return(1);
		}
		if (NULL != file_load)
		{
			DBG("\tFile not found");
			fclose(file_load);
		};
//		if (flag_object)
//			sprintf(txt_buffer, "fo%04d.txt", *_input);
//		else
//			sprintf(txt_buffer, "fd%04d.txt", *_input);
		file_load = fopen(compose_file_name(filename, flag_object, *_input), "r");
		flag_transmit = 1;
//		_flag;
//		DBG("txt_buffer");
		return(0);
//		if ((0 > (*_input)) || (9999 < (*_input)))
//		{
//			DBG("\tFile descriptor out of range");
//			return(1);
//		}
//		if (NULL != file_load)
//		{
//			DBG("\tClosing previous file");
//			fclose(file_load);
//		};
//		if (flag_object)
//			sprintf(txt_buffer, "fo%04d.txt", *_input);
//		else
//			sprintf(txt_buffer, "fd%04d.txt", *_input);
//		file_load = fopen(compose_file_name(filename, flag_object, *_input), "r");
//		flag_transmit = 1;
//		DBG("txt_buffer");
//		return(0);
/*	case TransmitFlag:
		if (flag_transmit)
		{
			if (NULL == file_load)
			{
				DBG("\tData file does not exist.");
				flag_transmit = 0;
			}
			else if (feof(file_load))
			{
				flag_transmit = 0;
				fclose(file_load);
				file_load = NULL;
				return(0);
			}
			else
			{
				txt_buffer[0] = fgetc(file_load);
				txt_buffer[1] = 0;
				serialSendString(txt_buffer);
			};
			return(1);
		};
		return(0); */
	case ResetClose:
		if ((0 > (*_input)) || (9999 < (*_input)))
		{
			DBG("File designator out of range");
			return(1);
		};
		if (NULL == file_save)
		{
			DBG("File never initialized");
			return(1);
		};
		fclose(file_save);
//		if (flag_object)
//			sprintf(txt_buffer, "fo%04d.txt", *_input);
//		else
//			sprintf(txt_buffer, "fd%04d.txt", *_input);
//		sprintf(txt_buffer, "f%04d.txt", *_input);
		DBG("Saving file");
//		DBG(txt_buffer);
		remove(compose_file_name(filename, flag_object, *_input));
		rename("ftemp.txt", filename); //txt_buffer);
		data_files_count++;
		file_save = NULL;
		flag_object = 0;
		processing_flag = 1;
		return(0);
	case ResetWriteObject:
		flag_object = 1;
	case ResetWrite:
		if (NULL != file_save)
		{
			fclose(file_save);
			remove("ftemp.txt");
		};
		file_save = fopen("ftemp.txt", "w");
		if (NULL == file_save)
			DBG("\tNo file opened {%d}", errno);
//		flag_process = *_input;
		if (!(*_input))
			processing_flag = 2;
		return(0);
	case WriteFlag:
		if (NULL == file_save)
			return(0);
		DBG("WF:%c", *_input);
		fputc(*_input, file_save);
		return(0);
	case ReadFlag:
		if (NULL == file_load)
			return(0);
		else if (feof(file_load))
		{
			fclose(file_load);
			file_load = NULL;
			flag_object = 0;
			flag_transmit  = 0;

//		wnd[ws].flags.bit.font_primary = original_primary;
//		wnd[ws].flags.bit.font_secondary = original_secondary;
//		wnd[ws].flags.bit.secondary_select = original_select;
			return(0);
		}
		else if (flag_transmit)
		{
			serialSend(fgetc(file_load));
//			txt_buffer[0] = fgetc(file_load);
//			txt_buffer[1] = 0;
//			serialSendString(txt_buffer);
			return(0);
		}
		else
			*_input = fgetc(file_load);
		return(1);
//	case FlagSave:
//		if (NULL == file_save)
//		{
//			DBG("\tNo Save File");
//			return(1);
//		};
//		fclose(file_save);
//		if (flag_object)
//			sprintf(txt_buffer, "fo%04d.txt", *_input);
//		else
//			sprintf(txt_buffer, "fd%04d.txt", *_input);
//		DBG(txt_buffer);
//		rename("ftemp.txt", compose_file_name(filename, flag_object, *_input));
//		file_save = NULL;
//		flag_object = 0;
//		return(0);
	case ResetAll:
		if (NULL != file_save)
		{
			fclose(file_save);
			file_save = NULL;
		};
		if (NULL != file_load)
		{
			fclose(file_load);
			file_load = NULL;
		};
		flag_object = 0;
		flag_transmit = 0;
//		flag_process = 0;
		return(0);
	default:
		return(0);
	};
}

/*
int handle_df_transmit (int _input)
{
	if (df_flag_transmit)
	{
		sprintf(txt_buffer, "%c", _input);
		serialSendString(txt_buffer);
		return(1);
	}
	else
		return(0);
}

int handle_df_save (int _input)
{
	if (NULL == df_save_file)
		return(0);

	fputc(_input, df_save_file);

	if (df_flag_process)
		return(0);
	else
		return(1);
}

int handle_df_load (int *_input)
{
	if (NULL == df_load_file)
		return(0);

	(*_input) = fgetc(df_load_file);

	return(1);
}
*/

void prcPause (void)
{
	flag_step_hold = !flag_step_hold;
}

int prcProcess (void)
{

	int input;

//	if (whndTestLoop(-2))
//		return(0);

	if (flag_step_hold)
		return(0);

	if (!handle_touch_response(-1))
		return(0);

	
//	if (serialOutIsEmpty())
//		whndSetCursor(wnd[ws].flags.bit.cursor_on, wnd[ws].flags.bit.cursor_type, wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, query_font_width(), query_font_height());

	if (!handle_repeat_file(ReadFlag, &input))
		if (!handle_data_file(ReadFlag, &input))
		{
			if (queueIsEmpty(parse_queue )) // || flag_step_hold)
				return(0);
			else
				input = queuePop(parse_queue);
			whndDisplayHold(0);
		}
		else
			whndDisplayHold(1);
	else
		whndDisplayHold(1);

	if (handle_monitor_mode(input))												// If monitor mode is active
		return(0);																// then exit from this routine before parsing data.

//	if (handle_data_file(TransmitFlag, &input))									// 
//		return(0);																//  

	if (handle_data_file(WriteFlag, &input))
	{
		return(0);
	};

	if (handle_repeat_file(WriteFlag, &input))
	{
		return(0);
	};

	if (handle_pcg(input, -1))
	{
		return(0);
	};

	if (handle_incremental_plot(input, -1))
	{
		return(0);
	};

	if (handle_raster(input, -1))
		return(0);

	parserProcess(processing_flag, input);
	return(exit_flag);
}

void prcUpdate (void)
{
	if (serialOutIsEmpty())
		whndSetCursor(wnd[ws].flags.bit.cursor_on, wnd[ws].flags.bit.cursor_type, wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, query_font_width(), query_font_height());
//	if (queueIsEmpty(parse_queue))
		whndTestPattern(MAP_VISUAL);															// Clear the test pattern if one exists since we now have valid input to screen.
//	};
}

int query_font (void)
{
	if (wnd[ws].flags.bit.secondary_select)
		return(wnd[ws].flags.bit.font_secondary);
	else
		return(wnd[ws].flags.bit.font_primary);
}

int query_font_width (void)
{
	int x, y;

	whndQueryFontSize(query_font(), &x, &y);
	return(x);
}

int query_font_height (void)
{
	int x,y;
	whndQueryFontSize(query_font(), &x, &y);

	return(y);
}

void prcCursor_Home(int *_values)
{
	wnd[ws].cursor.x = query_x(0, UpperLeft);
	wnd[ws].cursor.y = query_y(0, UpperLeft);
}

void prcCursor_Tab (int *_values)
{
	int loop;
	DBG("\tTab");

	for (loop = 0; loop < maximum_number_of_tabs; loop++)
		if (wnd[ws].htab_stop[loop] == -1)
			break;
		else if (wnd[ws].htab_stop[loop] > wnd[ws].cursor.x)
		{
			wnd[ws].cursor.x = wnd[ws].htab_stop[loop];
			return;
		};
	wnd[ws].cursor.x += query_font_width();
	if ((query_column(wnd[ws].cursor.x) + 1) > query_font_columns())
		wnd[ws].cursor.x = query_x(0, UpperLeft);

}

void prcExit (int *_values)
{
#ifdef DEBUGGING
	exit_flag = 1;
#endif
}

void prcCursor_Backspace (int *_values)
{
	DBG("\tBackspace");

	wnd[ws].cursor.x -= query_font_width();
	if (wnd[ws].cursor.x < 0)
		wnd[ws].cursor.x = 0;
}

void prcSelectSerialPort (int *_values)
{
	DBG("\tSelect Serial Port");
	// deprecated
}

void prcEnableNewLine (int *_values)
{
	DBG("\tEnable New Line");

	wnd[ws].flags.bit.mode_lnm = 1;
}

void prcControlBrightness (int *_values)
{
	DBG("\tControl Brightness: ");
	if (_values[0] == PARSER_OFFSET_POS)
	{
		serialSetBrightness(_values[1]);
		DBG("Brightness to %d", _values[1]);
		return;
	}
	if ((0 > _values[0]) || (3 < _values[0]))
	{
		DBG("Out of range");
		return;
	};

	switch(_values[0])
	{
	case 0: serialSetBrightness(1); break;
	case 1: serialSetBrightness(45); break;
	case 2: serialSetBrightness(120); break;
	case 3: serialSetBrightness(320); break;
	};


//	serialSetBrightness(0xFFF * (_values[0] + 1) / 4);
//	whndPaletteShift(7 + _values[0] * 8, 7 + _values[0] * 8, 7 + _values[0] * 8);
}

void prcDisableNewLine (int *_values)
{
	DBG("\tDisable New Line");

	wnd[ws].flags.bit.mode_lnm = 0;
}


void advance_line (int _count)
{
	int loop, end;

	if ((_count + query_row(wnd[ws].cursor.y)) < query_font_rows())
		wnd[ws].cursor.y += query_font_height() * _count;
	else if (!wnd[ws].flags.bit.mode_fpl)
	{
		_count -= (query_font_rows() - query_row(wnd[ws].cursor.y) - 1);
		if (wnd[ws].flags.bit.mode_edit)
		{
			whndHoldCursor();

			whndBlockHold(
				query_x_coord(1, UpperLeft),
				query_y_coord(_count + 1, UpperLeft),
				query_x_coord(query_font_columns() - 1, LowerRight),
				query_y_coord(query_font_rows() - 1, LowerRight));
			whndBlockRestore(
				query_x_coord(1, UpperLeft),
				query_y_coord(1, UpperLeft));
			whndBlockClear(
				query_x_coord(1, UpperLeft),
				query_y_coord(query_font_rows() - _count, UpperLeft),
				query_x_coord(query_font_columns() - 1, LowerRight),
				query_y_coord(query_font_rows() - 1, LowerRight));
			whndRestoreCursor();
			end = query_font_columns() * (query_font_rows() - _count + 1) - 1;
			for (loop = 0; loop < end; loop++)
				wnd[ws].text_map[loop] = wnd[ws].text_map[loop + query_font_columns() * _count];
			for (loop = query_font_columns() * (query_font_rows() - 1); loop >= end; loop--)
				wnd[ws].text_map[loop] = ' ';
		};
	};
}

void prcCursor_Linefeed (int *_values)
{
//	int loop, end;
	DBG("\tLinefeed");

	advance_line(1);

	if (wnd[ws].flags.bit.mode_lnm)
		wnd[ws].cursor.x = query_x(0, UpperLeft);

}

void prcCursor_VTab (int *_values)
{
	int loop;
	DBG("\tVertical Tab");

	for (loop = 0; loop < maximum_number_of_tabs; loop++)
		if (wnd[ws].vtab_stop[loop] == -1)
			break;
		else if (wnd[ws].vtab_stop[loop] > wnd[ws].cursor.y)
		{
			wnd[ws].cursor.y = wnd[ws].vtab_stop[loop];
			return;
		};
	wnd[ws].cursor.y += query_font_height();
	if ((query_row(wnd[ws].cursor.y) + 1) > query_font_rows())
		wnd[ws].cursor.y = query_y(0, UpperLeft);
}	

void prcCursor_Formfeed (int *_value)
{
	DBG("\tFormfeed");
	reset_cursor(ws);

//	wnd[ws].cursor.x = query_x(0, UpperLeft);
//	wnd[ws].cursor.y = query_y(0, UpperLeft);
	whndHoldCursor();
	whndBlockClear(0, 0, wnd[ws].maximum.x - 1, wnd[ws].maximum.y - 1);
	whndRestoreCursor();
}

void prcCursor_CarriageReturn (int *_value)
{
//	int end,loop;

	DBG("\tCarriage Return");
	wnd[ws].cursor.x = query_x(0, UpperLeft);
	if (wnd[ws].flags.bit.mode_pnl)
	{
		advance_line(1);
	};
}

void prcEnableCursor (int *_values)
{
	DBG("\tCursor: L Shape");
	wnd[ws].flags.bit.cursor_on = 1;
	if (prcCursorTypeUnknown == wnd[ws].flags.bit.cursor_type)
		wnd[ws].flags.bit.cursor_type = prcCursorTypeLShape;
}

void prcDisableCursor (int *_values)
{
	DBG("\tCursor: Off");
	wnd[ws].flags.bit.cursor_on = 0;
}

void prcEnableCrosshair (int *_values)
{
	DBG("\tCursor: Crosshair");

	wnd[ws].flags.bit.cursor_on = 1;
	wnd[ws].flags.bit.cursor_type = prcCursorTypeCrosshair;
}

void prcEnableUnderscore (int *_value)
{
	DBG("\tCursor: Underscore");

	wnd[ws].flags.bit.cursor_on = 1;
	wnd[ws].flags.bit.cursor_type = prcCursorTypeUnderscore;
}

void prcEnableBlockCursor (int *_value)
{
	DBG("\tCursor: Block");
	wnd[ws].flags.bit.cursor_on = 1;
	wnd[ws].flags.bit.cursor_type = prcCursorTypeBlock;
}

void prcCursorReposition (int *_values)
{
	DBG("\nCursor Column/Row Reposition (%d,%d)", _values[1], _values[0]);


	wnd[ws].cursor.x = max(
			0,
			min(
				wnd[ws].maximum.x - 1,
				max(
					1,
					_values[1]) * 6 - 1
				)
			);
	wnd[ws].cursor.y = max(
			0,
			min(
				wnd[ws].maximum.y - 1,
				max(
					1,
					_values[0]) * 10 - 1
				)
			);
/*
	wnd[ws].cursor.x = max(
			0,
			min(
				wnd[ws].maximum.x - 1,
				max(
					1,
					_values[1]) * query_font_width() - 1
				)
			);
	wnd[ws].cursor.y = max(
			0,
			min(
				wnd[ws].maximum.y - 1,
				max(
					1,
					_values[0]) * query_font_height() - 1
				)
			);
*/
//	wnd[ws].cursor.y = max(0, min(wnd[ws].maximum.y - 1, (max(1, _values[0]) * query_font_height() - 1)));

//	if ((_values[1] >= 0) && (_values[1] < query_font_columns()) && (_values[0] >= 0) && (_values[0] < query_font_rows()))
//	{
//		wnd[ws].cursor.x = max(1, _values[1]) * query_font_width() - 1;
//		wnd[ws].cursor.y = max(1, _values[0]) * query_font_height() - 1;
//	}
//	else
//		DBG("\tInvalid Position");
}


void prcDebugOff (int *_values)
{
	DBG("");
}

void prcMoveAbsolute (int *_values)
{
	DBG("\tCursor Reposition (%d,%d)", _values[0], _values[1]);

	if ((_values[0] < wnd[ws].maximum.x) && (_values[0] >= 0) && (_values[1] < wnd[ws].maximum.y) && (_values[1] >= 0))
	{
		wnd[ws].cursor.x = _values[0];
		wnd[ws].cursor.y = _values[1];
	};
}

void prcSetTab(int *_values)
{
// Excess (over 15) tab stop fault handling unknown.
	int loop,loop_2;

	DBG("\tSet Tab @%d", _values);
	for (loop = 0; maximum_number_of_tabs > loop; loop++)
		if (-1 == wnd[ws].htab_stop[loop])
		{
			wnd[ws].htab_stop[loop] = wnd[ws].cursor.x;
			break;
		}
		else if (wnd[ws].cursor.x == wnd[ws].htab_stop[loop])
			break;
		else if (wnd[ws].cursor.x < wnd[ws].htab_stop[loop])
		{
			for (loop_2 = maximum_number_of_tabs; loop_2 > loop; loop_2--)
				wnd[ws].htab_stop[loop_2] = wnd[ws].htab_stop[loop_2 - 1];
			wnd[ws].htab_stop[loop] = wnd[ws].cursor.x;
			break;
		};
}

void prcSetVertTab (int *_values)
{
	int loop, loop_2;
	DBG("\tSet Vertical Tab @%d", _values);
	for (loop = 0; maximum_number_of_tabs > loop; loop++)
		if (-1 == wnd[ws].vtab_stop[loop])
		{
			wnd[ws].vtab_stop[loop] = wnd[ws].cursor.y;
			break;
		}
		else if (wnd[ws].cursor.y == wnd[ws].vtab_stop[loop])
			break;
		else if (wnd[ws].cursor.y < wnd[ws].vtab_stop[loop])
		{
			for (loop_2 = maximum_number_of_tabs; loop_2 > loop; loop_2--)
				wnd[ws].vtab_stop[loop_2] = wnd[ws].vtab_stop[loop_2 - 1];
			wnd[ws].vtab_stop[loop] = wnd[ws].cursor.y;
			break;
		};
}

void prcCursor_Up (int *_values)
{
	int max, loop, count, rows_to_shift;
	if (PARSER_INVALID == _values[0])
		count = 1;
	else if (query_font_rows() <= _values[0])
		return;
	else
		count = _values[0];
	DBG("\tCursor Up %d rows", count);
	wnd[ws].cursor.y -= (query_font_height() * count);
	if (wnd[ws].cursor.y < 0)
	{
		rows_to_shift = query_row(-wnd[ws].cursor.y);
		wnd[ws].cursor.y = query_y(0, UpperLeft);
		if (wnd[ws].flags.bit.mode_edit)
		{
			whndHoldCursor();
			whndBlockHold(
				query_x_coord(0, UpperLeft),
				query_y_coord(0, UpperLeft),
				query_x_coord(query_font_columns() - 1, LowerRight),
				query_y_coord(query_font_rows() - rows_to_shift - 1, LowerRight));
			whndBlockRestore(
				query_x_coord(0, UpperLeft),
				query_y_coord(1, UpperLeft));
			whndBlockClear(
				query_x_coord(0, UpperLeft),
				query_y_coord(0, UpperLeft),
				query_x_coord(query_font_columns() - 1, LowerRight),
				query_y_coord(rows_to_shift, LowerRight));
			whndRestoreCursor();

			max = query_font_columns() * query_font_rows() - 1;
			for (loop = max; loop >= (query_font_columns() * rows_to_shift); loop--)
				wnd[ws].text_map[loop] = wnd[ws].text_map[loop - query_font_columns() * rows_to_shift];
		};
	};
}

void prcCursor_Down (int *_values)
{
//	int loop, max;
	int count;
	if (PARSER_INVALID == _values[0])
		count = 1;
	else if (query_font_columns() <= _values[0])
		return;
	else
		count = _values[0];
	DBG("\tCursor Down");
	advance_line(count);
}

void prcCursor_Right (int *_values)
{
	DBG("\tCursor Right");
	int count;
	if (PARSER_INVALID == _values[0])
		count = 1;
	else
		count = _values[0];

	wnd[ws].cursor.x += query_font_width() * count;
	if (query_column(wnd[ws].cursor.x) >= query_font_columns())
		wnd[ws].cursor.x = query_x_coord(query_font_columns() - 1, UpperLeft);
}

void prcCursor_Left (int *_values)
{
	int count;
	DBG("\tCursor Left");
	if (PARSER_INVALID == _values[0])
		count = 1;
	else
		count = _values[0];
	wnd[ws].cursor.x -= query_font_width() * count;
	if (query_column(wnd[ws].cursor.x) < 0)
		wnd[ws].cursor.x = 0;
}

void prcSelectFonts (int *_values)
{
	DBG("\tSelect Fonts");
	if ((_values[0] >= 0) && (_values[1] >= 0) && (_values[0] < 8) && (_values[1] < 8))
	{
		if (wnd[ws].flags.bit.mode_edit)
			if (wnd[ws].flags.bit.secondary_select)
				wnd[ws].flags.bit.font_primary = _values[0];
			else
				wnd[ws].flags.bit.font_secondary = _values[1];
		else
		{
			wnd[ws].flags.bit.font_primary = _values[0];
			wnd[ws].flags.bit.font_secondary = _values[1];
		};
		wnd[ws].flags.bit.secondary_select = 0;
	};
}

void prcEnablePrimaryFont (int *_values)
{
	DBG("\tSet Primary Font");
	if (wnd[ws].flags.bit.mode_edit)
	{
		if (wnd[ws].flags.bit.font_primary == wnd[ws].flags.bit.font_secondary)
			wnd[ws].flags.bit.secondary_select = 0;
		else
			DBG("Failed");
	}
	else
		wnd[ws].flags.bit.secondary_select = 0;
}

void prcEnableSecondaryFont (int *_values)
{
	DBG("\tSet Secondary Font");
	if (wnd[ws].flags.bit.mode_edit)
	{
		if (wnd[ws].flags.bit.font_primary == wnd[ws].flags.bit.font_secondary)
			wnd[ws].flags.bit.secondary_select = 1;
		else
			DBG("Failed");
	}
	else
		wnd[ws].flags.bit.secondary_select = 1;
}

void prcLoadPCGSymbol (int *_values)
{
	DBG("\tLoad PCG %d", _values[0]);
	if ((_values[0] >= 0) && (_values[0] < 96))
		handle_pcg(0, _values[0]);
}

void prcSetFontAttribute (int *_values)
{
	switch(_values[0])
	{
	case 0: DBG("\tClear Font Attributes"); wnd[ws].attributes.bit.blink = 0; wnd[ws].attributes.bit.reverse = 0; wnd[ws].attributes.bit.underline = 0; break;
	case 4: DBG("\tSet Underlne Attribute"); wnd[ws].attributes.bit.underline = 1; break;
	case 5: DBG("\tClear Underline Attribute"); wnd[ws].attributes.bit.underline = 0; break;
	case 6: DBG("\tSet Blink Attribute"); wnd[ws].attributes.bit.blink = 1; break;
	case 7: DBG("\tSet Reverse Attribute"); wnd[ws].attributes.bit.reverse = 1; break;
	case 8: DBG("\tClear Blink Attribute"); wnd[ws].attributes.bit.blink = 0; break;
	case 9: DBG("\tClear Reverse Attribute"); wnd[ws].attributes.bit.reverse = 0; break;
	case 10: DBG("\tSelect Primary Font"); wnd[ws].flags.bit.secondary_select = 0; break;
	case 11: DBG("\tSelect Secondary Font"); wnd[ws].flags.bit.secondary_select = 1; break;
	default: DBG("\tInvalid Font Attribute: %d", _values[0]); break;
	};
}

void prcDisableOverstrike (int *_values)
{
	DBG("\tOverstrike Disable");
	wnd[ws].attributes.bit.overstrike = 0;
}

void prcEnableOverstrike (int *_values)
{
	DBG("\tOverstrike Enable");
	wnd[ws].attributes.bit.overstrike = 1;
}

void prcRasterWrite (int *_values)
{
	DBG("\nRasterWrite: %d bytes", _values[0]);

	if (_values[0] > 0)
	{
		handle_raster(0, _values[0]);
		wnd[ws].cursor.x &= 0xFFF8;
		whndHoldCursor();
	};
}
/*
void prcCircleDraw (int *_values)
{
	int color;

	DBG("\tCircle Draw");
	if ((0 > _values[1]) || (7 < _values[1]) || (0 > _values[0]) || (511 < _values[0]))
		return;

//	whndHoldCursor();

	if (_values[1] & 1)
		color = 0;
	else
		color = 1;

	switch(_values[1])
	{
	case 0:
	case 1:
	case 4:
	case 5:
		whndDrawCircle(wnd[ws].offset.x + wnd[ws].cursor.x, wnd[ws].offset.y + wnd[ws].cursor.y, _values[0], color, 0, wnd[ws].offset.x, wnd[ws].offset.y, wnd[ws].maximum.x + wnd[ws].offset.x, wnd[ws].maximum.y + wnd[ws].offset.y);
		break;
	case 2:
	case 3:
	case 6:
	case 7:
		whndDrawCircle(wnd[ws].offset.x + wnd[ws].cursor.x, wnd[ws].offset.y + wnd[ws].cursor.y, _values[0], color, 1, wnd[ws].offset.x, wnd[ws].offset.y, wnd[ws].maximum.x + wnd[ws].offset.x, wnd[ws].maximum.y + wnd[ws].offset.y);
		break;
	};
}
*/

void prcObjTblStart (int *_values)
{
	DBG("\tObject Table Start");
	_values[0] = 0;
	handle_data_file(ResetWriteObject, _values);
}


void prcVectGraph (int *_values)
{
	int y_loop;
	int y_low;
	int y_high;
	int x2,y2, pattern;
	int bases[3];

	bases[0] = wnd[ws].cursor.x;
	bases[1] = wnd[ws].cursor.y;
	bases[2] = 0x10000;

	strip_offsets(bases, _values);

	DBG("VG: (%d,%d) (%d,%d) Mode %d Pattern %04X\t", wnd[ws].cursor.x, wnd[ws].cursor.y, _values[0], _values[1], _values[2], _values[3]);

	if (_values[0] < 0)
		x2 = 0;
	else if (_values[0] >= wnd[ws].maximum.x)
		x2 = wnd[ws].maximum.x - 1;
	else
		x2 = _values[0];

	if (_values[1] < 0)
		y2 = 0;
	else if (_values[1] >= wnd[ws].maximum.y)
		y2 = wnd[ws].maximum.y - 1;
	else
		y2 = _values[1];

	if (PARSER_INVALID == _values[3])
		pattern = global_dash_param;
	else
		pattern = _values[3] % 65536;


	whndHoldCursor();
	switch(_values[2])
	{
	case 0:
		line_draw(min(wnd[ws].cursor.x, x2), min(wnd[ws].cursor.y, y2), max(wnd[ws].cursor.x, x2), max(wnd[ws].cursor.y, y2), -1, pattern);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 1:
		line_draw(min(wnd[ws].cursor.x, x2), min(wnd[ws].cursor.y, y2), max(wnd[ws].cursor.x, x2), max(wnd[ws].cursor.y, y2), 1, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, x2, y2, 1, pattern);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 2:
		line_draw(min(wnd[ws].cursor.x, x2), min(wnd[ws].cursor.y, y2), max(wnd[ws].cursor.x, x2), max(wnd[ws].cursor.y, y2), 0, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, x2, y2, 0, pattern);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 3:
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 4:
		line_draw(x2, y2, x2, y2, -1, 0xFFFF);
//		whndDrawPoint(x2, y2, -1);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 5:
		line_draw(x2, y2, x2, y2, 1, 0xFFFF);
//		whndDrawPoint(x2, y2, 1);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 6:
		line_draw(x2, y2, x2, y2, 0, 0xFFFF);
//		whndDrawPoint(x2, y2, 0);
		wnd[ws].cursor.x = x2;
		wnd[ws].cursor.y = y2;
		break;
	case 7:
		line_draw(min(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, max(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, -1, pattern);
		line_draw(wnd[ws].cursor.x, min(wnd[ws].cursor.y, y2), wnd[ws].cursor.x, max(wnd[ws].cursor.y, y2), -1, pattern);
		line_draw(x2, min(wnd[ws].cursor.y, y2), x2, max(wnd[ws].cursor.y, y2), -1, pattern);
		line_draw(min(wnd[ws].cursor.x, x2), y2, max(wnd[ws].cursor.x, x2), y2, -1, pattern);
		break;
	case 8:
		line_draw(min(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, max(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, 1, pattern);
		line_draw(wnd[ws].cursor.x, min(wnd[ws].cursor.y, y2), wnd[ws].cursor.x, max(wnd[ws].cursor.y, y2), 1, pattern);
		line_draw(x2, min(wnd[ws].cursor.y, y2), x2, max(wnd[ws].cursor.y, y2), 1, pattern);
		line_draw(min(wnd[ws].cursor.x, x2), y2, max(wnd[ws].cursor.x, x2), y2, 1, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, x2, wnd[ws].cursor.y, 1, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, wnd[ws].cursor.x, y2, 1, pattern);
//		line_draw(x2, wnd[ws].cursor.y, x2, y2, 1, pattern);
//		line_draw(wnd[ws].cursor.x, y2, x2, y2, 1, pattern);
		break;
	case 9:
		line_draw(min(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, max(wnd[ws].cursor.x, x2), wnd[ws].cursor.y, 0, pattern);
		line_draw(wnd[ws].cursor.x, min(wnd[ws].cursor.y, y2), wnd[ws].cursor.x, max(wnd[ws].cursor.y, y2), 0, pattern);
		line_draw(x2, min(wnd[ws].cursor.y, y2), x2, max(wnd[ws].cursor.y, y2), 0, pattern);
		line_draw(min(wnd[ws].cursor.x, x2), y2, max(wnd[ws].cursor.x, x2), y2, 0, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, x2, wnd[ws].cursor.y, 0, pattern);
//		line_draw(wnd[ws].cursor.x, wnd[ws].cursor.y, wnd[ws].cursor.x, y2, 0, pattern);
//		line_draw(x2, wnd[ws].cursor.y, x2, y2, 0, pattern);
//		line_draw(wnd[ws].cursor.x, y2, x2, y2, 0, pattern);
		break;
	case 10:
		y_low = min(y2, wnd[ws].cursor.y);
		y_high = max(y2, wnd[ws].cursor.y);
		DBG("\tSolid Rect (%d,%d) -> (%d,%d)", wnd[ws].cursor.x, y_low, x2, y_high);
		for (y_loop = y_low; y_loop <= y_high; y_loop++)
			line_draw(min(wnd[ws].cursor.x, x2), y_loop, max(wnd[ws].cursor.x, x2), y_loop, 1, pattern);
		break;
	case 11:
		y_low = min(y2, wnd[ws].cursor.y);
		y_high = max(y2, wnd[ws].cursor.y);
		DBG("\tInvis Rect (%d,%d) -> (%d,%d)", wnd[ws].cursor.x, y_low, x2, y_high);
		for (y_loop = y_low; y_loop <= y_high; y_loop++)
			line_draw(min(wnd[ws].cursor.x, x2), y_loop, max(wnd[ws].cursor.x, x2), y_loop, 0, pattern);
//			line_draw(wnd[ws].cursor.x, y_loop, x2, y_loop, 0, pattern);
		break;
	case 12:
		y_low = min(y2, wnd[ws].cursor.y);
		y_high = max(y2, wnd[ws].cursor.y);
		DBG("\tComplement Rect (%d,%d) -> (%d,%d)", wnd[ws].cursor.x, y_low, x2, y_high);
		for (y_loop = y_low; y_loop <= y_high; y_loop++)
			line_draw(min(wnd[ws].cursor.x, x2), y_loop, max(wnd[ws].cursor.x, x2), y_loop, -1, pattern);
//			line_draw(wnd[ws].cursor.x, y_loop, x2, y_loop, -1, pattern);
		break;
	};
//	line_draw(0, 0, 0, 0, 0, -1);
	whndRestoreCursor();
}



void prcClearTabs (int *_values)
{
	int loop, loop_2;

	DBG("\tClear Tabs");
	switch(_values[0])
	{
	case 0:
		for (loop = 0; maximum_number_of_tabs > loop; loop++)
		{
			if (wnd[ws].htab_stop[loop] == wnd[ws].cursor.x)
			{
				for (loop_2 = loop + 1; maximum_number_of_tabs > loop_2; loop_2++)
					wnd[ws].htab_stop[loop_2 - 1] = wnd[ws].htab_stop[loop_2];
				wnd[ws].htab_stop[maximum_number_of_tabs - 1] = -1;
			};
		};
		break;
	case 1:
		for (loop = 0; maximum_number_of_tabs > loop; loop++)
		{
			if (wnd[ws].vtab_stop[loop] == wnd[ws].cursor.y)
			{
				for (loop_2 = loop + 1; maximum_number_of_tabs > loop_2; loop_2++)
					wnd[ws].vtab_stop[loop_2 - 1] = wnd[ws].vtab_stop[loop_2];
				wnd[ws].vtab_stop[maximum_number_of_tabs - 1] = -1;
			};
		};
		break;
	case 4:
		reset_tabs(ws);
		break;
	};
}

int prcStepControl (int _mode)
{
//	static int flag = 0;

	DBG("StC");
	/*
	switch(_mode)
	{
	case STEP_TOGGLE: flag = !flag; return(0);
	case STEP_RESUME:
		if (flag)
		{
			flag_step_hold = 0;
			return(0);
		}
		else
			return(1);
	case STEP_HOLD:
		if (flag)
			flag_step_hold = 1;
		return(0);
	default:
		DBG("\tStep Control: Invalid option");
		return(0);
	}; */
	return(0);
}

void prcStepMode (int *_values)
{
	DBG("StT");
#ifdef DEBUGGING
//	prcStepControl(STEP_TOGGLE);
#endif
}

void prcDisableScrollEdit (int *_values)
{
	DBG("\tDisable S/E");
	wnd[ws].flags.bit.mode_edit = 0;
}

void prcEnableScrollEdit (int *_values)
{
	int loop;

	DBG("\tEnable S/E");
	wnd[ws].flags.bit.mode_edit = 1;
	whndHoldCursor();
	DBG("%d %d %d %d", query_x_coord(0, UpperLeft), query_y_coord(0, UpperLeft), query_x_coord(query_font_columns() - 1, LowerRight), query_y_coord(query_font_rows() - 1, LowerRight));

	whndBlockClear(
		query_x_coord(0, UpperLeft),
		query_y_coord(0, UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_font_rows() - 1, LowerRight));
	whndRestoreCursor();
	for (loop = query_font_columns() * query_font_rows() - 1; loop >= 0; loop--)
		wnd[ws].text_map[loop] = ' ';
	wnd[ws].cursor.x = query_x(0, UpperLeft);
	wnd[ws].cursor.y = query_y(0, UpperLeft);
}

void prcInsertRow (int *_values)
{
	int loop, start;
	DBG("\tInsert Row (S/E)");

	if (!wnd[ws].flags.bit.mode_edit)
		return;

	whndHoldCursor();
	whndBlockHold(
		query_x_coord(0, UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_font_rows() - 2, LowerRight));
	whndBlockRestore(
		query_x_coord(0, UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft));
	whndBlockClear(
		query_x_coord(0, UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_row(wnd[ws].cursor.y), LowerRight));
	whndRestoreCursor();

	start = query_font_columns() * query_row(wnd[ws].cursor.y);
	for (loop = query_font_columns() * query_font_rows() - 1; loop > start; loop--)
		wnd[ws].text_map[loop] = wnd[ws].text_map[loop - query_font_columns()];
	for (loop = 0; loop < query_font_columns(); loop++)
		wnd[ws].text_map[loop + query_row(wnd[ws].cursor.y) * query_font_columns()] = ' ';
	wnd[ws].cursor.x = query_x(0, UpperLeft);
}

void prcSnapshot (int *_values)
{
	static int count = 0;
	DBG("\nSnapshot");
	whndSnapshot(count++);
}

void prcDeleteRow (int *_values)
{
	int loop, end;
	DBG("\tDelete Row (S/E)");

	if (!wnd[ws].flags.bit.mode_edit)
		return;

	whndHoldCursor();
	whndBlockHold(
		query_x_coord(query_column(0), UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_font_rows() - 1, LowerRight));
	whndBlockRestore(
		query_x_coord(query_column(0), UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft));
	whndBlockClear(
		query_x_coord(query_column(0), UpperLeft),
		query_y_coord(query_font_rows() - 1, UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_font_rows() - 1, LowerRight));
	whndRestoreCursor();

	end = query_font_columns() * (query_font_rows() - 1);
	for (loop = query_font_columns() * query_row(wnd[ws].cursor.y); loop < end; loop++)
		wnd[ws].text_map[loop] = wnd[ws].text_map[loop + query_font_columns()];
	for (loop = 0; loop < query_font_columns(); loop++)
		wnd[ws].text_map[loop + (query_font_rows() - 1) * query_font_columns()] = ' ';

}

void prcInsertChar (int *_values)
{
	int max, start, loop;
	DBG("\tInsect Char (S/E):");
	if (!wnd[ws].flags.bit.mode_edit)
	{
		DBG("Not in S/E mode");
		return;
	};

	whndHoldCursor();
//	if (!query_last_column())
	if (wnd[ws].flags.bit.mode_fpl)
	{
		whndBlockHold(
			query_x_coord(query_font_columns() - 1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
			query_x_coord(query_font_columns() - 1, LowerRight),
			query_y_coord(query_font_rows() - 2, LowerRight));

		whndBlockHold(
			query_x_coord(1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft),
			query_x_coord(query_font_columns() - 2, LowerRight),
			query_y_coord(query_font_rows() - 1, LowerRight));

		whndBlockRestore(
			query_x_coord(2, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft));

		whndBlockRestore(
			query_x_coord(1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft));
		max = query_font_rows() * query_font_columns() - 1;
	}
	else
		max = query_font_columns() + query_row(wnd[ws].cursor.y) * query_font_columns() - 1;


	whndBlockHold(
		query_x_coord(query_column(wnd[ws].cursor.x), UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
		query_x_coord(query_font_columns() - 2, LowerRight),
		query_y_coord(query_row(wnd[ws].cursor.y), LowerRight));
	whndBlockRestore(
		query_x_coord(query_column(wnd[ws].cursor.x) + 1, UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft));

	whndBlockClear(
		query_x_coord(query_column(wnd[ws].cursor.x), UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
		query_x_coord(query_column(wnd[ws].cursor.x), LowerRight),
		query_y_coord(query_row(wnd[ws].cursor.y), LowerRight));

	start = query_column(wnd[ws].cursor.x) + query_row(wnd[ws].cursor.y) * query_font_columns();
	
	for (loop = max; loop > start; loop--)
		wnd[ws].text_map[loop] = wnd[ws].text_map[loop - 1];

	wnd[ws].text_map[start] = ' ';

	whndRestoreCursor();
}

void prcDeleteChar (int *_values)
{
	int loop, max, y;

	DBG("\tDelete Character (S/E):");

	if (!wnd[ws].flags.bit.mode_edit)
	{
		DBG("Not in S/E mode");
		return;
	};

	whndHoldCursor();

	whndBlockHold(
		query_x_coord(query_column(wnd[ws].cursor.x) + 1, UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(query_row(wnd[ws].cursor.y), LowerRight));
	whndBlockRestore(
		query_x_coord(query_column(wnd[ws].cursor.x), UpperLeft),
		query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft));

	if (wnd[ws].flags.bit.mode_fpl)
	{
		whndBlockHold(
			query_x_coord(1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft),
			query_x_coord(1, LowerRight),
			query_y_coord(query_font_rows() - 1, LowerRight));
		
		whndBlockHold(
			query_x_coord(2, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft),
			query_x_coord(query_font_columns() - 1, LowerRight),
			query_y_coord(query_font_rows() - 1, LowerRight));
		
		whndBlockRestore(
			query_x_coord(1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y) + 1, UpperLeft));

		whndBlockRestore(
			query_x_coord(query_font_columns() - 1, UpperLeft),
			query_y_coord(query_row(wnd[ws].cursor.y), UpperLeft));
		
		y = query_font_rows() - 1;
	}
	else
		y = query_row(wnd[ws].cursor.y);

	whndBlockClear(
		query_x_coord(query_font_columns() - 1, UpperLeft),
		query_y_coord(y, UpperLeft),
		query_x_coord(query_font_columns() - 1, LowerRight),
		query_y_coord(y, LowerRight));

	max = query_font_columns() * (y + 1) - 1;

	for (loop = query_column(wnd[ws].cursor.x) + query_row(wnd[ws].cursor.y) * query_font_rows(); loop < max; loop++)
		wnd[ws].text_map[loop] = wnd[ws].text_map[loop + 1];

	wnd[ws].text_map[max] = ' ';

	whndRestoreCursor();
}

void prcEnableXON (int *_values)
{
	DBG("Enable XON");
	_values[1] = _values[0];
	_values[0] = -1;
	_values[2] = -1;
	_values[3] = -1;
	_values[4] = -1;

	prcSetSerialValues(_values);
}

void prcDisableXON (int *_values)
{
	DBG("\tDisable XON");
	_values[1] = _values[0];
	_values[0] = -1;
	_values[2] = -1;
	_values[3] = -1;
	_values[4] = -1;

	prcSetSerialValues(_values);
}

void prcTransmitRow (int *_values)
{
	int loop;

	DBG("\tTransmit Row");

	serialSend(13);
//	txt_buffer[0] = 13;
//	txt_buffer[1] = 0;
//	serialSendString("\015");

	for (loop = 0; loop < query_font_columns(); loop++)
		serialSend(wnd[ws].text_map[query_font_columns() * query_row(wnd[ws].cursor.y) + loop]);
//		txt_buffer[loop + 1] = wnd[ws].text_map[query_font_columns() * query_row(wnd[ws].cursor.y) + loop];

//	loop++;
	serialSendString("\015\012\031");
//	txt_buffer[loop++] = 13;
//	txt_buffer[loop++] = 10;
//	txt_buffer[loop++] = 25;
//	txt_buffer[loop] = 0;
//	serialSendString(txt_buffer);

	whndSetCursor(0, 0, wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, query_font_width(), query_font_height());
}


void prcEnableMonitorMode (int *_values)
{
	if (20 == _values[0])
		prcEnableNewLine(_values);
	else if (3 == _values[0])
	{
		DBG("Monitor Mode");
		handle_monitor_mode(-7);
//		parserF_MonitorMode = 1;
	}
	else if (4 == _values[0])
	{
		DBG("Monitor Mode (Serial)");
#ifdef DEBUGGING
		handle_monitor_mode(-8);
#endif
	};
}

void prcTransmitPage (int *_values)
{
	int loop; //, buffer_count = 0;
	DBG("\tTransmit Page");

//	serialSendString("\001");
//	txt_buffer[1] = 0;
	serialSend(1);
//	txt_buffer[0] = 1;
//	txt_buffer[buffer_count++] = 1;

	for (loop = 0; loop < (query_font_rows() * query_font_columns()); loop++)
	{
		serialSend(wnd[ws].text_map[loop]);
//		txt_buffer[buffer_count++] = wnd[ws].text_map[loop];
		if ((loop % query_font_columns()) == (query_font_columns() - 1))
		{
			DBG("<-Row%d ", loop / query_font_columns());
			serialSendString("\015\012");
//			txt_buffer[buffer_count++] = 13;
//			txt_buffer[buffer_count++] = 10;
		};
	};

	serialSend('\031');
//	txt_buffer[buffer_count++] = 25;
//	txt_buffer[buffer_count++] = 0;
//	serialSendString(txt_buffer);

	wnd[ws].cursor.x = query_x(0, UpperLeft);
	wnd[ws].cursor.y = query_y(0, UpperLeft);
	whndSetCursor(wnd[ws].flags.bit.cursor_on, 0, wnd[ws].cursor.x + wnd[ws].offset.x, wnd[ws].cursor.y + wnd[ws].offset.y, query_font_width(), query_font_height());
}

void prcSetSerialValues(int *_values)
{
	DBG("\tSet Serial Information");
	_values[1] = -1;
	serialSettingsTranslate(_values);
}

void prcEnableFullPageLatch (int *_values)
{
	DBG("\tEnable Full Page Latch");
	wnd[ws].flags.bit.mode_fpl = 1;
}

void prcDisableFullPageLatch (int *_values)
{
	DBG("\tDisable Full Page Latch");
	wnd[ws].flags.bit.mode_fpl = 0;
}

void prcEnablePseudoNewLine (int *_values)
{
	DBG("Enable Pseudo New Line");
	wnd[ws].flags.bit.mode_pnl = 1;
}

void prcDisablePseudoNewLine (int *_values)
{
	DBG("\tDisable Pseudo New Line");
	wnd[ws].flags.bit.mode_pnl = 0;
}


//void prcCursorControl (int *_values) { };

void prcEnableIncrementalPlot (int *_values)
{
	DBG("\tEnable Incremental Plot");
	handle_incremental_plot(0, 0);
}

void prcDashParam (int *_values)
{
	int bases[1];
	bases[0] = 65536;
	strip_offsets(bases, _values);
	_values[0] = _values[0] % 65536;
	DBG("\tSet Dash Param %d", _values[0]);
	global_dash_param = _values[0];
}

void prcObjTblDraw (int *_values) {
	if (PARSER_INVALID == _values[0])
		prcObjTblStart(_values);
	else
	{
		DBG("\tObject Table Draw");
		handle_data_file(ResetReadObject, _values);
	};
};

void prcEnterChar (int *_values)
{
	int loop, end;
//	DBG("\t|>%c", _values[0]);

//	prcInsertChar(_values);
	whndHoldCursor();
	if ((' ' <= _values[0]) && (127 > _values[0]))
	{
//		if (wnd[ws].flags.bit.mode_fpl)
			if ((query_column(wnd[ws].cursor.x) + 1) > query_font_columns())
				return;

//		if ((query_column(wnd[ws].cursor.x) + 1) <= query_font_columns())
//		{
//		if ((query_column(wnd[ws].cursor.x) + 1) <= query_font_columns())
//		{
//		}
//		if (wnd[ws].cursor.x < visual_size.x)
		{
			whndPrintChar(_values[0], wnd[ws].offset.x + wnd[ws].cursor.x, wnd[ws].offset.y + wnd[ws].cursor.y, query_font(), wnd[ws].attributes.word);
			if ((wnd[ws].cursor.x + query_font_width()) < wnd[ws].maximum.x)
				line_draw(
					wnd[ws].cursor.x + query_font_width(),
					wnd[ws].cursor.y,
					wnd[ws].cursor.x + query_font_width(),
					wnd[ws].cursor.y + query_font_height() - 1,
					wnd[ws].attributes.bit.reverse ? 1 : 0,
					0xFFFF);

			if ((wnd[ws].cursor.x + query_font_width() + 1) < wnd[ws].maximum.x)
				line_draw(
					wnd[ws].cursor.x + query_font_width() + 1,
					wnd[ws].cursor.y,
					wnd[ws].cursor.x + query_font_width() + 1,
					wnd[ws].cursor.y + query_font_height() - 1,
					wnd[ws].attributes.bit.reverse ? 1 : 0,
					0xFFFF);


			if (wnd[ws].flags.bit.mode_edit)
				wnd[ws].text_map[query_font_columns() * query_row(wnd[ws].cursor.y) + query_column(wnd[ws].cursor.x)] = _values[0];
			wnd[ws].cursor.x += query_font_width();

			if (wnd[ws].flags.bit.mode_edit)
			{
				if ((query_column(wnd[ws].cursor.x) + 1) > query_font_columns())
				{
					if ((query_row(wnd[ws].cursor.y) + 1) < query_font_rows())
					{
						wnd[ws].cursor.y += query_font_height();
						wnd[ws].cursor.x = query_x(0, UpperLeft);
					}
					else if (!wnd[ws].flags.bit.mode_fpl)
					{
						whndBlockHold(
							query_x_coord(1, UpperLeft),
							query_y_coord(2, UpperLeft),
							query_x_coord(query_font_columns() - 1, LowerRight),
							query_y_coord(query_font_rows() - 1, LowerRight));
						whndBlockRestore(
							query_x_coord(1, UpperLeft),
							query_y_coord(1, UpperLeft));
						whndBlockClear(
							query_x_coord(1, UpperLeft),
							query_y_coord(query_font_rows() - 1, UpperLeft),
							query_x_coord(query_font_columns() - 1, LowerRight),
							query_y_coord(query_font_rows() - 1, LowerRight));
						end = query_font_columns() * query_font_rows() - 1;
						for (loop = 0; loop < end; loop++)		
							wnd[ws].text_map[loop] = wnd[ws].text_map[loop + query_font_columns()];
						for (loop = query_font_columns() * (query_font_rows() - 1); loop < end; loop++)
							wnd[ws].text_map[loop] = ' ';
						wnd[ws].cursor.x = query_x(1, UpperLeft);
					};
				};
			};
		};
	};
	whndRestoreCursor();
}

void prcEnableBitMap (int *_values)
{ 
	DBG("\tEnable Bit Map");
}

void prcDisableBitMap (int *_values)
{
	DBG("\tDisable Bit Map");
}

void prcDefineBitMap (int *_values)
{
	DBG("\tDefine Bit Map");
}

void prcSelectBitMap (int *_values)
{
	DBG("\tSelect Bit Map");
}

void prcDisplayBitMap (int *_values)
{
	DBG("Display Bit Map");
}

void prcDetectBitMap (int *_values)
{
	if (PARSER_INVALID == _values[0])
		prcSetTab(_values);
	else
	{
		DBG("\tDetect Bit Map");
	};
}

void prcUpdateOn (int *_values)
{
	DBG("Update On Bit Map");
}

void prcUpdateOff (int *_values)
{
	DBG("\tUpdate Off Bit Map");
}

void prcUpdateRange (int *_values)
{
	DBG("\tUpdate Range Bit Map");
}

void prcCombineBitMap (int *_values)
{
	DBG("\tCombine Bit Map");
}

void prcDefineWindow (int *_values)
{
	int _x1, _y1, _x2, _y2;

	DBG("\tDefine Bit Map:");
	_x1 = min(_values[0], _values[2]);
	_x2 = max(_values[0], _values[2]);
	_y1 = min(_values[1], _values[3]);
	_y2 = max(_values[1], _values[3]);
	if ((0 > _x1) || (0 > _y1) || (visual_size.x <= _x2) || (visual_size.y <= _y2))
	{
		 DBG("\tInvalid Coordinates");
		 return;
	};

}

void prcStartRepeat (int *_values)
{
	processing_flag = 4;

	handle_repeat_file(ResetWrite, _values);
}


void prcSoftwareReset(int *_values)
{
	int list[1];
	int loop;

//	whndTestPattern(0);

	for (loop = 0; loop < maximum_number_of_windows; loop++)
	{
		DBG("\nResetting window #%d", loop);
		reset_window(loop);
//		DBG(" cursor");
//		reset_cursor(loop);
//		DBG(" data map");
//		reset_data_map(loop);
//		DBG(" tabs");
//		reset_tabs(loop);
		DBG(" screen");
		reset_screen(loop);

		DBG(" flags");
		wnd[loop].flags.word = 0;
		wnd[loop].flags.bit.font_secondary = 2;
		wnd[loop].window_active = 0;
	};
	wnd[0].window_active = 1;
	system("del fd*.txt");
	system("del fo*.txt");
	ws = 0;
	list[0] = 3;
	prcControlBrightness(list);
}

void prcRequests(int *_values)
{
	switch(_values[0])
	{
	case 5: serialSendString("\005000;000"); break;
	case 6: serialSendString("\005%03d;%03d", wnd[ws].cursor.x, wnd[ws].cursor.y);
//		sprintf(txt_buffer, "\005%03d;%03d", wnd[ws].cursor.x, wnd[ws].cursor.y);
//		serialSendString(txt_buffer);
		break;
	case 4:
		serialSendString("952-005-19v1.3"); break; // serial number 952-005-19 v1.0
	case 3:
		serialSendString("\0057800"); break;
	case 8:
		serialSendString("%04d;%08d;%04d;%08d;%08d", data_files_count, data_files_count * 20000, bitmaps_count, bitmaps_count * 20000, 400000000 - (bitmaps_count + data_files_count) * 20000);
//		sprintf(txt_buffer, "%04d;%08d;%04d;%08d;%08d", data_files_count, data_files_count * 20000, bitmaps_count, bitmaps_count * 20000, 400000000 - (bitmaps_count + data_files_count) * 20000);
//		serialSendString(txt_buffer); break;
	default:
		DBG("\tInvalid Request (%d)", _values[0]);
		break;
	};
}

void prcTestPattern(int *_values)
{
	if (1 == _values[0])
		whndTestPattern(MAP_HATCHED);
	else
		whndTestPattern(MAP_DIAG_PASSED);
	serialSendString("\005000;000");
}


void prcSelectWindow (int *_values)
{
	int loop;

	DBG("\nSelect Window %d", _values[0]);
	if (_values[0] == 0)
	{
		for (loop = 1; 4 > loop; loop++)
			wnd[loop].window_active = 0;
		reset_window(0);
		reset_screen(0);
		wnd[0].flags.word = wnd[ws].flags.word;
		wnd[0].attributes.word = wnd[ws].attributes.word;
		ws = 0;
	}
	else if ((_values[0] > 0) && (_values[0] <= 4))
	{
		if (wnd[_values[0] - 1].window_active)
		{
			ws = _values[0] - 1;
			DBG("\tWindow %d @ (%d,%d) offset with (%d,%d) maximum.", ws, wnd[ws].offset.x, wnd[ws].offset.y, wnd[ws].maximum.x, wnd[ws].maximum.y);
		}
		else
			DBG("\tWindow not active");
	}
	else
		DBG("Invalid Window");
}

void prcDeleteWindow (int *_values)
{
	DBG("\nDelete Window", _values[0]);
}

void strip_offsets (int *_bases, int *_values)
{
	int loop_1 = 0, loop_2 = 0, loop_3 = 0;

	while (1)
	{
		switch (_values[loop_1++])
		{
		case PARSER_OFFSET_NEG:
//			DBG("{PON:%d %d}", _bases[loop_3], _values[loop_1]);
			_values[loop_2++] = _bases[loop_3++] - _values[loop_1++];
			break;
		case PARSER_OFFSET_POS:
//			DBG("{POP:%d %d}", _bases[loop_3], _values[loop_1]);
			_values[loop_2++] = _bases[loop_3++] + _values[loop_1++];
			break;
		case PARSER_INVALID:
			_values[loop_2] = PARSER_INVALID;
			return;
		default:
//			DBG("{Pno:%d %d}", _values[loop_1 - 1]);
			_values[loop_2++] = _values[loop_1 - 1];
			break;
		};
	};
}

void prcSetGraphicWindow (int *_values)
{
	int bases[2];
	int x1, x2, y1, y2;

	bases[0] = wnd[ws].cursor.x;
	bases[1] = wnd[ws].cursor.y;

	strip_offsets(bases, _values);

	if (2 != _values[2])
	{
		DBG("\nInvalid Parameter 3");
		return;
	};

	x1 = wnd[ws].offset.x + min(_values[0], wnd[ws].cursor.x);
	x2 = wnd[ws].offset.x + max(_values[0], wnd[ws].cursor.x);
	y1 = wnd[ws].offset.y + min(_values[1], wnd[ws].cursor.y);
	y2 = wnd[ws].offset.y + max(_values[1], wnd[ws].cursor.y);

	DBG("\nGraphic Window %d,%d %d,%d", x1, y1, x2, y2);

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 >= visual_size.x)
		x2 = visual_size.x - 1;
	if (y2 >= visual_size.y)
		y2 = visual_size.y - 1;

	wnd[4].offset.x = x1;
	wnd[4].offset.y = y1;
	wnd[4].cursor.x = query_x(0, UpperLeft);
	wnd[4].cursor.y = query_y(0, UpperLeft);
	wnd[4].maximum.x = x2 - x1;
	wnd[4].maximum.y = y2 - y1;
	wnd[4].flags.word = wnd[ws].flags.word;
	reset_tabs(4);
	wnd[4].attributes.word = wnd[ws].attributes.word;
	wnd[4].window_active = 1;
}

void prcSplitScreen (int *_values)
{
	static int last_window = -1;
	int split_new = 0;
	int loop;
	unsigned int original_flags;

	whndHoldCursor();
	DBG("\nSplit Screen: %d", _values[0]);

	if (3 == _values[0])
	{
		if (wnd[4].window_active)
		{
			if (4 == ws)
			{
				DBG("\nDeactivating Graphic Window");
				whndSetGraphicWindow(-1, -1, -1, -1);
				ws = last_window;
			}
			else
			{
				DBG("\nActivating Graphic Window");
				whndSetGraphicWindow(wnd[4].offset.x, wnd[4].offset.y, wnd[4].maximum.x, wnd[4].maximum.y);
				last_window = ws;
				ws = 4;
			};
		}
		else
			DBG("\nNo graphic window");
		whndRestoreCursor();
		return;
	};

	if ((ws < 0) || (ws > 1))
		return;

	original_flags = wnd[ws].flags.word;

	switch (_values[0])
	{
	case 0: // perform a horizontal split
		if ( (wnd[ws].cursor.y < query_font_height()) || ((wnd[ws].maximum.y - wnd[ws].cursor.y) < query_font_height()))
		{
			DBG("\nInvalid row for split");
		}
		else if (ws == 0) // on screen #1
		{
			if (!wnd[1].window_active) // if screen #2 isn't present, then create it
			{
				split_new = 1;
				DBG("\nHoriz Splitting Window #1 into 1 & 2");
			}
			else if ((wnd[1].offset.x > 0) && (!wnd[3].window_active)) // if screen #2 is present, it is divided vertically and screen #4 isn't present, then create screen #4
			{
				DBG("\nHoriz Splitting Window #1 into 1 & 4");
				split_new = 3;
			}
			else
				DBG("\nInvalid Split");
			
			if (split_new)
			{
				init_window(split_new, wnd[0].maximum.x, -wnd[0].cursor.y);
				init_window(0, wnd[0].maximum.x, wnd[0].cursor.y);
				ws = split_new; // set active screen to new screen
			};
		}
		else // on screen #2
		{
			if ((wnd[1].offset.x > 0) && (!wnd[2].window_active)) // if screen #2 is vertically divided and screen #3 isn't present, then create screen #3
			{
				DBG("\nHoriz Splitting Window #2 into 2 & 3");
				split_new = 2;
			}
			else
				DBG("\nInvalid Split");

			if (split_new)
			{
				init_window(split_new, -wnd[1].offset.x, -wnd[1].cursor.y);
				init_window(1, -wnd[1].offset.x, wnd[1].cursor.y);
				swap_window(1,split_new);
				// retain #2 as active screen
			};
		};
		break;
	case 1: // perform a vertical split
		if ((wnd[ws].cursor.x < query_font_width()) || ((wnd[ws].maximum.x - wnd[ws].cursor.x) < query_font_width()))
		{
			DBG("\nInvalid column for split");
		}
		else if (ws == 0) // on screen #1
		{

			if (!wnd[1].window_active)  // if screen #2 doesn't exist, create it
			{
				split_new = 1;
				DBG("\nVert Splitting Window #1 into 1 & 2");
			}
			else if ((wnd[1].offset.x == 0) && (!wnd[2].window_active)) // if screen #2 exists, it's split horizontally from #1, and screen #3 doesn't exist, create #3
			{
				DBG("\nVert Splitting Window #1 into 1 & 3");
				split_new = 2;
			}
			else
				DBG("\nInvalid Split");

			if (split_new)
			{
				init_window(split_new, -wnd[0].cursor.x, wnd[0].maximum.y);
				init_window(0, wnd[0].cursor.x, wnd[0].maximum.y);
				ws = split_new; // set new screen as active.
			};
		}
		else // on screen #2
		{
			if ((wnd[1].offset.x == 0) && (!wnd[3].window_active)) // screen #2 exists (we're on it), it's split horizontally from #1, and screen #4 doesn't exist, so create #4
			{
				DBG("\nVert Splitting Window #2 into 2 & 4");
				split_new = 3;
			}
			else
				DBG("\nInvalid Split");

			if (split_new)
			{
				init_window(split_new, -wnd[1].cursor.x, -wnd[1].offset.y);
				init_window(1, wnd[1].cursor.x, -wnd[1].offset.y);
				swap_window(1,split_new);
				// keep #2 as active screen
			};
		};
		break;
	};
	
	if (split_new)
	{
		for (loop = 0; loop < 4; loop++)
			if (wnd[loop].window_active)
			{
//				wnd[loop].cursor.x = 0;
//				wnd[loop].cursor.y = 0;
				wnd[loop].flags.word = original_flags;
				DBG("\nWindow #%d (%d,%d) (%d,%d)", loop, wnd[loop].offset.x, wnd[loop].offset.y, wnd[loop].offset.x + wnd[loop].maximum.x - 1, wnd[loop].offset.y + wnd[loop].maximum.y - 1);

				whndBlockClear(wnd[loop].offset.x, wnd[loop].offset.y, wnd[loop].offset.x + wnd[loop].maximum.x - 1, wnd[loop].offset.y + wnd[loop].maximum.y - 1);
			};
	};
	DBG("XXXX");
	whndRestoreCursor();
}



void prcSendTouchPanelSize (int *_values)
{
	switch(mode_touch)
	{
	case UnlatchedLow:
	case LatchedLow:
		serialSendString("\00431;15");
		break;
	default:
		serialSendString("\04462;31");
		break;
	};
}


void prcSetBaudRate (int *_values)
{
	_values[1] = -1;
	_values[2] = -1;
	_values[3] = -1;
	_values[4] = -1;

	prcSetSerialValues(_values);
}

void prcDisableTouchPanel (int *_value)
{
	DBG("\tSet Touch Disabled");
	handle_touch_response(TouchDisabled);

//	parserF_TouchEnable = 0;
}

void prcEnableTouchPanelContinuous (int *_value)
{
	DBG("\tSet Touch Continuous");
	if (3 == _value[0])
		handle_touch_response(UnlatchedHigh);
	else
		handle_touch_response(UnlatchedLow);
//	parserF_TouchEnable = 1;
//	parserF_TouchContinuous = 1;
//	parserF_TouchHighResolution = 0;
}

void prcEnableTouchPanelLatch (int *_value)
{
	DBG("\tSet Touch Latched");
	if (3 == _value[0])
		handle_touch_response(LatchedHigh);
	else
		handle_touch_response(LatchedLow);
//	handle_touch_response(LatchedLow);
//	parserF_TouchEnable = 1;
//	parserF_TouchContinuous = 0;
//	parserF_TouchHighResolution = 0;
}

void prcEnableTouchPanelContHigh (int *_value)
{
	DBG("\tSet Touch Continuous High Resolution.");
	handle_touch_response(UnlatchedHigh);
//	parserF_TouchEnable = 1;
//	parserF_TouchContinuous = 1;
//	parserF_TouchHighResolution = 1;
}

void prcEnableTouchPanelLatchHigh (int *_value)
{
	DBG("\tSet Touch Latch High Resolution.");
	handle_touch_response(LatchedHigh);
//	parserF_TouchEnable = 1;
//	parserF_TouchContinuous = 0;
//	parserF_TouchHighResolution = 1;
}


void prcStartFileProcess (int *_values)
{
	int flag = 1;
	handle_data_file(ResetWrite, &flag);
//	if (NULL != df_save_file)
//	{
//		fclose(df_save_file);
//		remove("ftemp.txt");
//	}
//	df_save_file = fopen("ftemp.txt", "w");
//	df_flag_process = 1;
}

void prcStartFile (int *_values)
{
	int flag = 0;
	handle_data_file(ResetWrite, &flag);
//	if (NULL != df_save_file)
//	{
//		fclose(df_save_file);
//		remove("ftemp.txt");
//	}
//	df_save_file = fopen("ftemp.txt", "w");
//	df_flag_process = 0;
}

void prcRepeatLoop (int *_values)
{
	processing_flag = 1;
	handle_repeat_file(ResetRead, _values);
//	rf_count = _values[0];
//	rf_index = 0;
}

void prcSaveFile (int *_values)
{
	handle_data_file(ResetClose, _values);
//	if ((_values[0] < 0) || (_values[0] > 9999))
//		return;

//	fclose(df_save_file);
//	sprintf(txt_buffer, "f%04d.txt", _values[0]);
//	rename("tfile.txt", txt_buffer);
//	df_save_file = NULL;
}

void prcClearFileBuffer (int *_values)
{
	handle_data_file(ResetAll, _values);
//	if (NULL != df_save_file)
//	{
//		fclose(df_save_file);
//		df_save_file = NULL;
//	};

//	if (NULL != df_load_file)
//	{
//		fclose(df_load_file);
//		df_load_file = NULL;
//	}

}

void prcProcessFile (int *_values)
{
	handle_data_file(ResetRead, _values);
//	if ((_values[0] < 0) || (_values[0] > 9999))
//		return;
//	sprintf(txt_buffer, "f%04d.txt", _values[0]);
//	df_load_file = fopen(txt_buffer, "r");
}

void prcDeleteFile (int *_values)
{
	char filename[80];
	if ((_values[0] < 0) || (_values[0] > 9999))
		return;
//	sprintf(txt_buffer, "f%04d.txt", _values[0]);
	remove(compose_file_name(filename, 0, _values[0]));//txt_buffer);
}

void prcTransmitFile (int *_values)
{
	handle_data_file(ResetTransmit, _values);
//	if ((_values[0] < 0) || (_values[0] > 9999))
//		return;
//	sprintf(txt_buffer, "f%04d.txt", _values[0]);
//	df_load_file = fopen(txt_buffer, "r");
//	df_flag_transmit = 1;
}

void prcCircleDraw (int *_values)
{
	int loop, loop_y, x, y, last_x, last_y, color_select, hatch_select, last_right_x, last_left_x, right_x, left_x;
	float slope_left, slope_right;

	DBG("\tDraw Circle:");

	if ((_values[0] < 0) || (_values[0] >= visual_size.x))
	{
		DBG("Invalid radius %d", _values[0]);
		return;
	};

	whndHoldCursor();
	color_select = ((_values[1] & 1) == 0);
	if (_values[1] > 3)
		hatch_select = 0xF0F0;
	else
		hatch_select = 0xFFFF;

	switch(_values[1])
	{
	case 0:
	case 1:
	case 4:
	case 5:
		last_x = wnd[ws].cursor.x + (256 + circle_offsets[359].x * _values[0]) / 512;
		last_y = wnd[ws].cursor.y + (256 + circle_offsets[359].y * _values[0]) / 512;
		for (loop = 0; loop < 360; loop++)
		{
			x = wnd[ws].cursor.x + (256 + circle_offsets[loop].x * _values[0]) / 512;
			y = wnd[ws].cursor.y + (256 + circle_offsets[loop].y * _values[0]) / 512;
			if ((last_x != x) || (last_y != y))
			{
				line_draw(last_x, last_y, x, y, color_select, hatch_select);
				last_x = x;
				last_y = y;
			};
		};
		break;
	case 2:
	case 3:
	case 6:
	case 7:

		DBG("(%d,%d)", wnd[ws].cursor.x, wnd[ws].cursor.y);
		last_y = wnd[ws].cursor.y + (circle_offsets[0].y * _values[0] + 256) / 512;
		last_right_x = wnd[ws].cursor.x + (circle_offsets[0].x * _values[0] + 256) / 512;
		last_left_x = wnd[ws].cursor.x + (circle_offsets[0].x * _values[0] + 256) / 512;

		for (loop = 1; 180 > loop; loop++)
		{
			y = wnd[ws].cursor.y + (circle_offsets[loop].y * _values[0] + 256) / 512;
			if (last_y != y)
			{
				right_x = wnd[ws].cursor.x + (circle_offsets[loop].x * _values[0] + 256) / 512;
				left_x = wnd[ws].cursor.x + (circle_offsets[360 - loop].x * _values[0] + 256) / 512;

				slope_right = ((float) (right_x - last_right_x)) / ((float) (y - last_y));
				slope_left = ((float) (left_x - last_left_x)) / ((float) (y - last_y));

				for (loop_y = last_y; loop_y < y; loop_y++)
					line_draw(
						left_x + ((int) (slope_left * (loop_y - last_y))), loop_y,
						right_x + ((int) (slope_right * (loop_y - last_y))), loop_y, color_select, hatch_select);
				last_y = y;
				last_right_x = right_x;
				last_left_x = left_x;
			};
		};
		break;
	default:
		DBG("Invalid circle selection");
	};
	whndRestoreCursor();

}

coordT circle_offsets[360] =
{
	{0, -512},
	{9, -512},
	{18, -512},
	{27, -511},
	{36, -511},
	{45, -510},
	{54, -509},
	{62, -508},
	{71, -507},
	{80, -506},
	{89, -504},
	{98, -503},
	{106, -501},
	{115, -499},
	{124, -497},
	{133, -495},
	{141, -492},
	{150, -490},
	{158, -487},
	{167, -484},
	{175, -481},
	{183, -478},
	{192, -475},
	{200, -471},
	{208, -468},
	{216, -464},
	{224, -460},
	{232, -456},
	{240, -452},
	{248, -448},
	{256, -443},
	{264, -439},
	{271, -434},
	{279, -429},
	{286, -424},
	{294, -419},
	{301, -414},
	{308, -409},
	{315, -403},
	{322, -398},
	{329, -392},
	{336, -386},
	{343, -380},
	{349, -374},
	{356, -368},
	{362, -362},
	{368, -356},
	{374, -349},
	{380, -343},
	{386, -336},
	{392, -329},
	{398, -322},
	{403, -315},
	{409, -308},
	{414, -301},
	{419, -294},
	{424, -286},
	{429, -279},
	{434, -271},
	{439, -264},
	{443, -256},
	{448, -248},
	{452, -240},
	{456, -232},
	{460, -224},
	{464, -216},
	{468, -208},
	{471, -200},
	{475, -192},
	{478, -183},
	{481, -175},
	{484, -167},
	{487, -158},
	{490, -150},
	{492, -141},
	{495, -133},
	{497, -124},
	{499, -115},
	{501, -106},
	{503, -98},
	{504, -89},
	{506, -80},
	{507, -71},
	{508, -62},
	{509, -54},
	{510, -45},
	{511, -36},
	{511, -27},
	{512, -18},
	{512, -9},
	{512, 0},
	{512, 9},
	{512, 18},
	{511, 27},
	{511, 36},
	{510, 45},
	{509, 54},
	{508, 62},
	{507, 71},
	{506, 80},
	{504, 89},
	{503, 98},
	{501, 106},
	{499, 115},
	{497, 124},
	{495, 133},
	{492, 141},
	{490, 150},
	{487, 158},
	{484, 167},
	{481, 175},
	{478, 183},
	{475, 192},
	{471, 200},
	{468, 208},
	{464, 216},
	{460, 224},
	{456, 232},
	{452, 240},
	{448, 248},
	{443, 256},
	{439, 264},
	{434, 271},
	{429, 279},
	{424, 286},
	{419, 294},
	{414, 301},
	{409, 308},
	{403, 315},
	{398, 322},
	{392, 329},
	{386, 336},
	{380, 343},
	{374, 349},
	{368, 356},
	{362, 362},
	{356, 368},
	{349, 374},
	{343, 380},
	{336, 386},
	{329, 392},
	{322, 398},
	{315, 403},
	{308, 409},
	{301, 414},
	{294, 419},
	{286, 424},
	{279, 429},
	{271, 434},
	{264, 439},
	{256, 443},
	{248, 448},
	{240, 452},
	{232, 456},
	{224, 460},
	{216, 464},
	{208, 468},
	{200, 471},
	{192, 475},
	{183, 478},
	{175, 481},
	{167, 484},
	{158, 487},
	{150, 490},
	{141, 492},
	{133, 495},
	{124, 497},
	{115, 499},
	{106, 501},
	{98, 503},
	{89, 504},
	{80, 506},
	{71, 507},
	{62, 508},
	{54, 509},
	{45, 510},
	{36, 511},
	{27, 511},
	{18, 512},
	{9, 512},
	{0, 512},
	{-9, 512},
	{-18, 512},
	{-27, 511},
	{-36, 511},
	{-45, 510},
	{-54, 509},
	{-62, 508},
	{-71, 507},
	{-80, 506},
	{-89, 504},
	{-98, 503},
	{-106, 501},
	{-115, 499},
	{-124, 497},
	{-133, 495},
	{-141, 492},
	{-150, 490},
	{-158, 487},
	{-167, 484},
	{-175, 481},
	{-183, 478},
	{-192, 475},
	{-200, 471},
	{-208, 468},
	{-216, 464},
	{-224, 460},
	{-232, 456},
	{-240, 452},
	{-248, 448},
	{-256, 443},
	{-264, 439},
	{-271, 434},
	{-279, 429},
	{-286, 424},
	{-294, 419},
	{-301, 414},
	{-308, 409},
	{-315, 403},
	{-322, 398},
	{-329, 392},
	{-336, 386},
	{-343, 380},
	{-349, 374},
	{-356, 368},
	{-362, 362},
	{-368, 356},
	{-374, 349},
	{-380, 343},
	{-386, 336},
	{-392, 329},
	{-398, 322},
	{-403, 315},
	{-409, 308},
	{-414, 301},
	{-419, 294},
	{-424, 286},
	{-429, 279},
	{-434, 271},
	{-439, 264},
	{-443, 256},
	{-448, 248},
	{-452, 240},
	{-456, 232},
	{-460, 224},
	{-464, 216},
	{-468, 208},
	{-471, 200},
	{-475, 192},
	{-478, 183},
	{-481, 175},
	{-484, 167},
	{-487, 158},
	{-490, 150},
	{-492, 141},
	{-495, 133},
	{-497, 124},
	{-499, 115},
	{-501, 106},
	{-503, 98},
	{-504, 89},
	{-506, 80},
	{-507, 71},
	{-508, 62},
	{-509, 54},
	{-510, 45},
	{-511, 36},
	{-511, 27},
	{-512, 18},
	{-512, 9},
	{-512, 0},
	{-512, -9},
	{-512, -18},
	{-511, -27},
	{-511, -36},
	{-510, -45},
	{-509, -54},
	{-508, -62},
	{-507, -71},
	{-506, -80},
	{-504, -89},
	{-503, -98},
	{-501, -106},
	{-499, -115},
	{-497, -124},
	{-495, -133},
	{-492, -141},
	{-490, -150},
	{-487, -158},
	{-484, -167},
	{-481, -175},
	{-478, -183},
	{-475, -192},
	{-471, -200},
	{-468, -208},
	{-464, -216},
	{-460, -224},
	{-456, -232},
	{-452, -240},
	{-448, -248},
	{-443, -256},
	{-439, -264},
	{-434, -271},
	{-429, -279},
	{-424, -286},
	{-419, -294},
	{-414, -301},
	{-409, -308},
	{-403, -315},
	{-398, -322},
	{-392, -329},
	{-386, -336},
	{-380, -343},
	{-374, -349},
	{-368, -356},
	{-362, -362},
	{-356, -368},
	{-349, -374},
	{-343, -380},
	{-336, -386},
	{-329, -392},
	{-322, -398},
	{-315, -403},
	{-308, -409},
	{-301, -414},
	{-294, -419},
	{-286, -424},
	{-279, -429},
	{-271, -434},
	{-264, -439},
	{-256, -443},
	{-248, -448},
	{-240, -452},
	{-232, -456},
	{-224, -460},
	{-216, -464},
	{-208, -468},
	{-200, -471},
	{-192, -475},
	{-183, -478},
	{-175, -481},
	{-167, -484},
	{-158, -487},
	{-150, -490},
	{-141, -492},
	{-133, -495},
	{-124, -497},
	{-115, -499},
	{-106, -501},
	{-98, -503},
	{-89, -504},
	{-80, -506},
	{-71, -507},
	{-62, -508},
	{-54, -509},
	{-45, -510},
	{-36, -511},
	{-27, -511},
	{-18, -512},
	{-9, -512}
};

char *font_design_7_9_rulings =
"@`"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="

"@a"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"xxxxxxx*"
"-------="
"-------="

"@b"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"----xxx*"
"----x--="
"----x--="

"@c"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"xxxxx--="
"----x--="
"----x--="

"@d"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----xxx*"
"-------="
"-------="

"@e"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"xxxxx--="
"-------="
"-------="

"@f"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"xxxxxxx*"
"----x--="
"----x--="

"@g"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"xxxxxxx*"
"-------="
"-------="


"@h"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----xxx*"
"----x--="
"----x--="

"@i"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"xxxxx--="
"----x--="
"----x--="

"@j"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"xxxxxxx*"
"----x--="
"----x--="

"@k"
"-------="
"-----xx*"
"----x--="
"---x---="
"--x----="
"-x---xx="
"-x-----="
"-x-----="
"-x-x---="
"-x--x--="
"-x---x-="
"-x----x*"
"--x----="
"---x---="
"----xxx*"
"-------="

"@l"
"-------="
"xxx----="
"---x---="
"----x--="
"-----x-="
"-xx---x="
"------x="
"------x="
"----x-x="
"---x--x="
"--x---x="
"xx----x="
"-----x-="
"----x--="
"xxxx---="
"-------="

"@m"
"-------="
"-------="
"-------*"
"------x="
"-xxxxxx*"
"----x--="
"-xxxxxx*"
"---x---="
"--x----="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@n"
"-------="
"-------="
"-------="
"---xx--*"
"--x--xx="
"-------="
"---xx--*"
"--x--xx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@o"
"-------="
"-------="
"----x--="
"----x--="
"--xxxxx="
"----x--="
"----x--="
"-------="
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@p"
"-------="
"-xxxxxx*"
"--x----="
"---x---="
"----x--="
"---x---="
"--x----="
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@q"
"-------="
"-------="
"-------="
"-------*"
"--xxxxx="
"-x-x-x-="
"---x-x-="
"---x-x-="
"---x-x-="
"---x-x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@r"
"-------="
"--xx---="
"-x--x--="
"-x--x--="
"--xx---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@s"
"-------="
"----x--="
"---xxx-="
"--xxxxx="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@t"
"-------="
"-------="
"-------="
"-----x-="
"-----xx="
"-xxxxxx*"
"-----xx="
"-----x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@u"
"-------="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"--xxxxx="
"---xxx-="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@v"
"-------="
"-------="
"-------="
"---x---="
"--xx---="
"-xxxxxx*"
"--xx---="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@w"
"-------="
"-------="
"-------="
"--xxxx-="
"--x--x-="
"--x--x-="
"--xxxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@x"
"-------="
"-------="
"-------="
"--xxxx-="
"--xxxx-="
"--xxxx-="
"--xxxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@y"
"-------="
"-------="
"-xxxxxx*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@z"
"-------="
"-------="
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@{"
"-------="
"-------="
"---xxx-="
"--x---x="
"-x-----*"
"-x-----*"
"-x-----*"
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@|"
"-------="
"-------="
"---xxx-="
"--xxxxx="
"-xxxxxx*"
"-xxxxxx*"
"-xxxxxx*"
"--xxxxx="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@}"
"-------="
"-------="
"-------="
"----x--="
"---x-x-="
"--x---x="
"-x-----*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@~"
"-------="
"-------="
"-------="
"----x--="
"---xxx-="
"--xxxxx="
"-xxxxxx*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@\177"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*"
"x-x-x-x="
"-x-x-x-*";



char *font_design_5_7_rulings =
"@`"
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="

"@a"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"xxxxx*"

"@b"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"---xx*"

"@c"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"xxx--="

"@d"
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---xx*"

"@e"
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"xxxx-="

"@f"
"-----="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"-xxxx*"
"--xxx="
"-xxxx*"
"---x-="

"@g"
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"xxxxx*"

"@h"
"-----="
"---xx*"
"--x--="
"-x--x="
"-x---="
"-x-x-="
"-x--x*"
"--x--="
"---xx*"
"-----="

"@i"
"-----="
"xxx--="
"---x-="
"-x--x="
"----x="
"--x-x="
"xx--x="
"---x-="
"xxx--="
"-----="

"@j"
"-----="
"---x-="
"---x-="
"--xxx="
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-----="

"@k"
"-----="
"---xx*"
"--x--="
"-x--x="
"-x---="
"-x--x*"
"-x-x-="
"--x--="
"---xx*"
"-----="

"@l"
"-----="
"xxx--="
"---x-="
"-x--x="
"----x="
"xx--x="
"--x-x="
"---x-="
"xxx--="
"-----="

"@m"
"-----="
"-----*"
"----x="
"-xxxx*"
"---x-="
"-xxxx*"
"--x--="
"-x---="
"-----="
"-----="

"@n"
"-----="
"--xx-*"
"-x-xx="
"-----="
"--xx-*"
"-x-xx="
"-----="
"-----="
"-----="
"-----="

"@o"
"-----="
"---x-="
"---x-="
"-xxxx*"
"---x-="
"---x-="
"-----="
"-xxxx*"
"-----="
"-----="

"@p"
"-----="
"-xxxx*"
"--x--="
"---x-="
"----x="
"---x-="
"--x--="
"-xxxx*"
"-----="
"-----="

"@q"
"-----="
"-----="
"-----*"
"--xxx*"
"-xx-x="
"--x-x="
"--x-x="
"--x-x="
"-----="
"-----="

"@r"
"-----="
"--xx-="
"-x--x="
"-x--x="
"--xx-="
"-----="
"-----="
"-----="
"-----="
"-----="

"@s"
"-----="
"---x-="
"--xxx="
"-x-x-*"
"---x-="
"---x-="
"---x-="
"---x-="
"-----="
"-----="

"@t"
"-----="
"-----="
"---x-="
"----x="
"-xxxx*"
"----x="
"---x-="
"-----="
"-----="
"-----="

"@u"
"-----="
"---x-="
"---x-="
"---x-="
"---x-="
"-x-x-*"
"--xxx="
"---x-="
"-----="
"-----="

"@v"
"-----="
"-----="
"---x-="
"--x--="
"-xxxx*"
"--x--="
"---x-="
"-----="
"-----="
"-----="

"@w"
"-----="
"-----="
"-----="
"--xxx="
"--x-x="
"--xxx="
"-----="
"-----="
"-----="
"-----="

"@x"
"-----="
"-----="
"-----="
"--xxx="
"--xxx="
"--xxx="
"-----="
"-----="
"-----="
"-----="

"@y"
"-----="
"-----="
"-xxxx*"
"-x---*"
"-x---*"
"-x---*"
"-xxxx*"
"-----="
"-----="
"-----="

"@z"
"-----="
"-----="
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-xxxx*"
"-----="
"-----="
"-----="

"@{"
"-----="
"---x-="
"--x-x="
"-x---*"
"-x---*"
"--x-x="
"---x-="
"-----="
"-----="
"-----="

"@|"
"-----="
"---x-="
"--xxx="
"-xxxx*"
"-xxxx*"
"--xxx="
"---x-="
"-----="
"-----="
"-----="

"@}"
"-----="
"---x-="
"--x-x="
"-x---*"
"-xxxx*"
"-----="
"-----="
"-----="
"-----="
"-----="

"@~"
"-----="
"---x-="
"--xxx="
"-xxxx*"
"-xxxx*"
"-----="
"-----="
"-----="
"-----="
"-----="

"@\177"
"x-x-x="
"-x-x-*"
"x-x-x="
"-x-x-*"
"x-x-x="
"-x-x-*"
"x-x-x="
"-x-x-*"
"x-x-x="
"-x-x-*";


char *font_design_5_7 = 
"@ "
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@!"
"-----="
"---x-="
"---x-="
"---x-="
"---x-="
"-----="
"---x-="
"-----="
"-----="
"-----="

"@\042"
"-----="
"--x-x="
"--x-x="
"--x-x="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@#"
"-----="
"--x-x="
"--x-x="
"-xxxx*"
"--x-x="
"-xxxx*"
"--x-x="
"--x-x="
"-----="
"-----="

"@$"
"-----="
"---x-="
"--xxx*"
"-x-x-="
"--xxx="
"---x-*"
"-xxxx="
"---x-="
"-----="
"-----="

"@%"
"-----="
"-xx--="
"-xx--*"
"----x="
"---x-="
"--x--="
"-x--x*"
"----x*"
"-----="
"-----="

"@&"
"-----="
"--x--="
"-x-x-="
"-x-x-="
"--x--="
"-x-x-="
"-x--x="
"--xx-*"
"-----="
"-----="

"@'"
"-----="
"---x-="
"---x-="
"---x-="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@("
"-----="
"---x-="
"--x--="
"-x---="
"-x---="
"-x---="
"--x--="
"---x-="
"-----="
"-----="

"@)"
"-----="
"---x-="
"----x="
"-----*"
"-----*"
"-----*"
"----x="
"---x-="
"-----="
"-----="

"@*"
"-----="
"---x-="
"-x-x-*"
"--xxx="
"---x-="
"--xxx="
"-x-x-*"
"---x-="
"-----="
"-----="

"@+"
"-----="
"-----="
"---x-="
"---x-="
"-xxxx*"
"---x-="
"---x-="
"-----="
"-----="
"-----="

"@,"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"---x-="
"---x-="
"--x--="

"@-"
"-----="
"-----="
"-----="
"-----="
"-xxxx*"
"-----="
"-----="
"-----="
"-----="
"-----="

"@."
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"---x-="
"-----="
"-----="

"@/"
"-----="
"-----="
"-----*"
"----x="
"---x-="
"--x--="
"-x---="
"-----="
"-----="
"-----="

"@0"
"-----="
"--xxx="
"-x---*"
"-x--x*"
"-x-x-*"
"-xx--*"
"-x---*"
"--xxx="
"-----="
"-----="

"@1"
"-----="
"---x-="
"--xx-="
"---x-="
"---x-="
"---x-="
"---x-="
"--xxx="
"-----="
"-----="

"@2"
"-----="
"--xxx="
"-x---*"
"-----*"
"--xxx="
"-x---="
"-x---="
"-xxxx*"
"-----="
"-----="

"@3"
"-----="
"-xxxx*"
"-----*"
"----x="
"---xx="
"-----*"
"-x---*"
"--xxx="
"-----="
"-----="

"@4"
"-----="
"----x="
"---xx="
"--x-x="
"-x--x="
"-xxxx*"
"----x="
"----x="
"-----="
"-----="

"@5"
"-----="
"-xxxx*"
"-x---="
"-xxxx="
"-----*"
"-----*"
"-x---*"
"--xxx="
"-----="
"-----="

"@6"
"-----="
"---xx*"
"--x--="
"-x---="
"-xxxx="
"-x---*"
"-x---*"
"--xxx="
"-----="
"-----="

"@7"
"-----="
"-xxxx*"
"-----*"
"-----*"
"----x="
"---x-="
"--x--="
"-x---="
"-----="
"-----="

"@8"
"-----="
"--xxx="
"-x---*"
"-x---*"
"--xxx="
"-x---*"
"-x---*"
"--xxx="
"-----="
"-----="

"@9"
"-----="
"--xxx="
"-x---*"
"-x---*"
"--xxx*"
"-----*"
"----x="
"-xxx-="
"-----="
"-----="

"@:"
"-----="
"-----="
"-----="
"---x-="
"-----="
"---x-="
"-----="
"-----="
"-----="
"-----="

"@;"
"-----="
"-----="
"-----="
"---x-="
"-----="
"---x-="
"---x-="
"--x--="
"-----="
"-----="

"@<"
"-----="
"----x="
"---x-="
"--x--="
"-x---="
"--x--="
"---x-="
"----x="
"-----="
"-----="

"@="
"-----="
"-----="
"-----="
"-xxxx*"
"-----="
"-xxxx*"
"-----="
"-----="
"-----="
"-----="

"@>"
"-----="
"-x---="
"--x--="
"---x-="
"----x="
"---x-="
"--x--="
"-x---="
"-----="
"-----="

"@?"
"-----="
"--xxx="
"-x---*"
"-----*"
"---xx="
"---x-="
"-----="
"---x-="
"-----="
"-----="

"@@"
"-----="
"--xxx="
"-x---*"
"-x-x-*"
"-x-xx*"
"-x-xx="
"-x---="
"--xxx*"
"-----="
"-----="

"@A"
"-----="
"---x-="
"--x-x="
"-x---*"
"-x---*"
"-xxxx*"
"-x---*"
"-x---*"
"-----="
"-----="

"@B"
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-xxxx="
"-x---*"
"-x---*"
"-xxxx="
"-----="
"-----="

"@C"
"-----="
"--xxx="
"-x---*"
"-x---="
"-x---="
"-x---="
"-x---*"
"--xxx="
"-----="
"-----="

"@D"
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-xxxx="
"-----="
"-----="

"@E"
"-----="
"-xxxx*"
"-x---="
"-x---="
"-xxxx="
"-x---="
"-x---="
"-xxxx*"
"-----="
"-----="

"@F"
"-----="
"-xxxx*"
"-x---="
"-x---="
"-xxxx="
"-x---="
"-x---="
"-x---="
"-----="
"-----="

"@G"
"-----="
"--xxx="
"-x---*"
"-x---="
"-x---="
"-x--x*"
"-x---*"
"--xxx*"
"-----="
"-----="

"@H"
"-----="
"-x---*"
"-x---*"
"-x---*"
"-xxxx*"
"-x---*"
"-x---*"
"-x---*"
"-----="
"-----="

"@I"
"-----="
"--xxx="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"--xxx="
"-----="
"-----="

"@J"
"-----="
"-----*"
"-----*"
"-----*"
"-----*"
"-----*"
"-x---*"
"--xxx="
"-----="
"-----="

"@K"
"-----="
"-x---*"
"-x--x="
"-x-x-="
"-xx--="
"-x-x-="
"-x--x="
"-x---*"
"-----="
"-----="

"@L"
"-----="
"-x---="
"-x---="
"-x---="
"-x---="
"-x---="
"-x---="
"-xxxx*"
"-----="
"-----="

"@M"
"-----="
"-x---*"
"-xx-x*"
"-x-x-*"
"-x-x-*"
"-x---*"
"-x---*"
"-x---*"
"-----="
"-----="

"@N"
"-----="
"-x---*"
"-x---*"
"-xx--*"
"-x-x-*"
"-x--x*"
"-x---*"
"-x---*"
"-----="
"-----="

"@O"
"-----="
"--xxx="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"--xxx="
"-----="
"-----="

"@P"
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-xxxx="
"-x---="
"-x---="
"-x---="
"-----="
"-----="

"@Q"
"-----="
"--xxx="
"-x---*"
"-x---*"
"-x---*"
"-x-x-*"
"-x--x="
"--xx-*"
"-----="
"-----="

"@R"
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-xxxx="
"-x-x-="
"-x--x="
"-x---*"
"-----="
"-----="

"@S"
"-----="
"--xxx="
"-x---*"
"-x---="
"--xxx="
"-----*"
"-x---*"
"--xxx="
"-----="
"-----="

"@T"
"-----="
"-xxxx*"
"-x-x-*"
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"-----="
"-----="

"@U"
"-----="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"--xxx="
"-----="
"-----="

"@V"
"-----="
"-x---*"
"-x---*"
"-x---*"
"--x-x="
"--x-x="
"---x-="
"---x-="
"-----="
"-----="

"@W"
"-----="
"-x---*"
"-x---*"
"-x---*"
"-x-x-*"
"-x-x-*"
"-x-x-*"
"--x-x="
"-----="
"-----="

"@X"
"-----="
"-x---*"
"-x---*"
"--x-x="
"---x-="
"--x-x="
"-x---*"
"-x---*"
"-----="
"-----="

"@Y"
"-----="
"-x---*"
"-x---*"
"--x-x="
"---x-="
"---x-="
"---x-="
"---x-="
"-----="
"-----="

"@Z"
"-----="
"-xxxx*"
"-----*"
"----x="
"---x-="
"--x--="
"-x---="
"-xxxx*"
"-----="
"-----="

"@["
"-----="
"-xxxx*"
"-xx--="
"-xx--="
"-xx--="
"-xx--="
"-xx--="
"-xxxx*"
"-----="
"-----="

"@\\"
"-----="
"-----="
"-x---="
"--x--="
"---x-="
"----x="
"-----*"
"-----="
"-----="
"-----="

"@]"
"-----="
"-xxxx*"
"----x*"
"----x*"
"----x*"
"----x*"
"----x*"
"-xxxx*"
"-----="
"-----="

"@^"
"-----="
"---x-="
"--x-x="
"-x---*"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@_"
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="
"-xxxx*"

"@`"
"-----="
"--x--="
"---x-="
"----x="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@a"
"-----="
"-----="
"-----="
"--xxx="
"-----*"
"--xxx*"
"-x---*"
"--xxx*"
"-----="
"-----="

"@b"
"-----="
"-x---="
"-x---="
"-xxxx="
"-x---*"
"-x---*"
"-x---*"
"-xxxx="
"-----="
"-----="

"@c"
"-----="
"-----="
"-----="
"--xxx*"
"-x---="
"-x---="
"-x---="
"--xxx*"
"-----="
"-----="

"@d"
"-----="
"-----*"
"-----*"
"--xxx*"
"-x---*"
"-x---*"
"-x---*"
"--xxx*"
"-----="
"-----="

"@e"
"-----="
"-----="
"-----="
"--xxx="
"-x---*"
"-xxxx*"
"-x---="
"--xxx="
"-----="
"-----="

"@f"
"-----="
"----x="
"---x-="
"---x-="
"--xxx="
"---x-="
"---x-="
"---x-="
"-----="
"-----="

"@g"
"-----="
"-----="
"-----="
"--xxx*"
"-x---*"
"-x---*"
"-x---*"
"--xxx*"
"-----*"
"---xx="

"@h"
"-----="
"-x---="
"-x---="
"-x---="
"-xxxx="
"-x---*"
"-x---*"
"-x---*"
"-----="
"-----="

"@i"
"-----="
"---x-="
"-----="
"--xx-="
"---x-="
"---x-="
"---x-="
"--xxx="
"-----="
"-----="

"@j"
"-----="
"-----="
"-----="
"---x-="
"-----="
"---x-="
"---x-="
"---x-="
"-x-x-="
"--x--="

"@k"
"-----="
"--x--="
"--x--="
"--x--*"
"--x-x="
"--xx-="
"--x-x="
"--x--*"
"-----="
"-----="

"@l"
"-----="
"--xx-="
"---x-="
"---x-="
"---x-="
"---x-="
"---x-="
"--xxx="
"-----="
"-----="

"@m"
"-----="
"-----="
"-----="
"-xx-x="
"-x-x-*"
"-x-x-*"
"-x-x-*"
"-x-x-*"
"-----="
"-----="

"@n"
"-----="
"-----="
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-----="
"-----="

"@o"
"-----="
"-----="
"-----="
"-----="
"--xxx="
"-x---*"
"-x---*"
"-x---*"
"--xxx="
"-----="

"@p"
"-----="
"-----="
"-----="
"-xxxx="
"-x---*"
"-x---*"
"-x---*"
"-xxxx="
"-x---="
"-x---="

"@q"
"-----="
"-----="
"-----="
"--xxx*"
"-x---*"
"-x---*"
"-x---*"
"--xxx*"
"-----*"
"-----*"

"@r"
"-----="
"-----="
"-----="
"--x-x*"
"--xx-="
"--x--="
"--x--="
"--x--="
"-----="
"-----="

"@s"
"-----="
"-----="
"-----="
"--xxx*"
"-x---="
"--xxx="
"-----*"
"-xxxx="
"-----="
"-----="

"@t"
"-----="
"-----="
"---x-="
"--xxx="
"---x-="
"---x-="
"---x-="
"----x="
"-----="
"-----="

"@u"
"-----="
"-----="
"-----="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"--xxx*"
"-----="
"-----="

"@v"
"-----="
"-----="
"-----="
"-x---*"
"-x---*"
"--x-x="
"--x-x="
"---x-="
"-----="
"-----="

"@w"
"-----="
"-----="
"-----="
"-x---*"
"-x---*"
"-x---*"
"-x-x-*"
"--x-x="
"-----="
"-----="

"@x"
"-----="
"-----="
"-----="
"-x---*"
"--x-x="
"---x-="
"--x-x="
"-x---*"
"-----="
"-----="

"@y"
"-----="
"-----="
"-----="
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"--xxx*"
"-----*"
"---xx="

"@z"
"-----="
"-----="
"-----="
"-xxxx*"
"----x="
"---x-="
"--x--="
"-xxxx*"
"-----="
"-----="

"@{"
"-----="
"----x="
"---x-="
"---x-="
"--x--="
"---x-="
"---x-="
"----x="
"-----="
"-----="

"@|"
"-----="
"---x-="
"---x-="
"---x-="
"-----="
"---x-="
"---x-="
"---x-="
"-----="
"-----="

"@}"
"-----="
"-----="
"--x--="
"---x-="
"---x-="
"----x="
"---x-="
"---x-="
"--x--="
"-----="

"@~"
"-----="
"-----*"
"--xxx="
"-x---="
"-----="
"-----="
"-----="
"-----="
"-----="
"-----="

"@\177"
"-----="
"-----="
"-xxxx*"
"-x---*"
"-x---*"
"-x---*"
"-x---*"
"-xxxx*"
"-----="
"-----=";

char *font_design_7_9 =
"@ "
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@!"
"-------="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"-------="
"-------="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@\042"
"-------="
"---x-x-="
"---x-x-="
"---x-x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@#"
"-------="
"---x-x-="
"---x-x-="
"-xxxxxx*"
"---x-x-="
"-xxxxxx*"
"---x-x-="
"---x-x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@$"
"-------="
"----x--="
"--xxxxx="
"-x--x--*"
"-x--x--="
"--xxxxx="
"----x--*"
"-x--x--*"
"--xxxxx="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@%"
"-------="
"--x----="
"-x-x---*"
"-x-x--x="
"--x--x-="
"----x--="
"---x--x="
"--x--x-*"
"-x---x-*"
"------x="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@&"
"-------="
"---x---="
"--x-x--="
"--x-x--="
"---x---="
"--xx---*"
"-x--x--*"
"-x---xx="
"-x---xx="
"--xxx--*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@'"
"-------="
"-----x-="
"----x--="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@("
"-------="
"-------*"
"------x="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"------x="
"-------*"
"-------="
"-------="
"-------="
"-------="
"-------="

"@)"
"-------="
"-x-----="
"--x----="
"---x---="
"---x---="
"---x---="
"---x---="
"---x---="
"---x---="
"--x----="
"-x-----="
"-------="
"-------="
"-------="
"-------="
"-------="

"@*"
"-------="
"-------="
"-------="
"----x--="
"--x-x-x="
"---x-x-="
"---x-x-="
"--x-x-x="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@+"
"-------="
"-------="
"-------="
"-------="
"----x--="
"----x--="
"--xxxxx="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@,"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"---xx--="
"---xx--="
"----x--="
"---x---="
"-------="
"-------="
"-------="
"-------="

"@-"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@."
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"---xx--="
"---xx--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@/"
"-------="
"-------="
"-------="
"-------*"
"------x="
"-----x-="
"----x--="
"---x---="
"--x----="
"-x-----="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@0"
"-------="
"-------="
"---xxx-*"
"--x---x="
"--x--xx="
"--x-x-x="
"--x-x-x="
"--xx--x="
"--x---x="
"-x-xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@1"
"-------="
"-------="
"----x--="
"---xx--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@2"
"-------="
"-------="
"---xxx-="
"--x---x="
"------x="
"-----x-="
"----x--="
"---x---="
"--x----="
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@3"
"-------="
"-------="
"---xxx-="
"--x---x="
"------x="
"----xx-="
"------x="
"------x="
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@4"
"-------="
"-------="
"------x="
"-----xx="
"----x-x="
"---x--x="
"--xxxxx*"
"------x="
"------x="
"------x="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@5"
"-------="
"-------="
"--xxxxx="
"--x----="
"--x----="
"--xxxx-="
"------x="
"------x="
"-----x-="
"--xxx--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@6"
"-------="
"-------="
"----xx-="
"---x---="
"--x----="
"--xxxx-="
"--x---x="
"--x---x="
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@7"
"-------="
"-------="
"--xxxxx="
"------x="
"-----x-="
"-----x-="
"----x--="
"----x--="
"---x---="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@8"
"-------="
"-------="
"---xxx-="
"--x---x="
"--x---x="
"---xxx-="
"--x---x="
"--x---x="
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@9"
"-------="
"-------="
"---xxx-="
"--x---x="
"--x---x="
"--x---x="
"---xxxx="
"------x="
"-----x-="
"---xx--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@:"
"-------="
"-------="
"-------="
"-------="
"---xx--="
"---xx--="
"-------="
"---xx--="
"---xx--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@;"
"-------="
"-------="
"-------="
"-------="
"---xx--="
"---xx--="
"-------="
"---xx--="
"---xx--="
"----x--="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="

"@<"
"-------="
"-------="
"-----x-="
"----x--="
"---x---="
"--x----="
"---x---="
"----x--="
"-----x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@="
"-------="
"-------="
"-------="
"-------="
"-------="
"--xxxxx="
"-------="
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@>"
"-------="
"-------="
"---x---="
"----x--="
"-----x-="
"------x="
"-----x-="
"----x--="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@?"
"-------="
"---xxx-="
"--x---x="
"--x---x="
"------x="
"-----x-="
"----x--="
"----x--="
"-------="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@@"
"-------="
"-------="
"---xxx-="
"--x---x="
"-x--xx-*"
"-x-x-x-*"
"-x-x-x-*"
"-x--xxx*"
"--x----="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@A"
"-------="
"---xxx-="
"--x---x="
"-x-----*"
"-x-----*"
"-xxxxxx*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@B"
"-------="
"-xxxxx-="
"-x----x="
"-x----x="
"-xxxxxx="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@C"
"-------="
"---xxxx="
"--x----*"
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"--x----*"
"---xxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@D"
"-------="
"-xxxxx-="
"-x----x="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x----x="
"-xxxxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@E"
"-------="
"-xxxxxx*"
"-x-----="
"-x-----="
"-x-----="
"-xxxx--="
"-x-----="
"-x-----="
"-x-----="
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@F"
"-------="
"-xxxxxx*"
"-x-----="
"-x-----="
"-x-----="
"-xxxx--="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@G"
"-------="
"---xxxx="
"--x----*"
"-x-----="
"-x-----="
"-x-----="
"-x---xx*"
"-x-----*"
"--x----*"
"---xxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@H"
"-------="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@I"
"-------="
"--xxxxx="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@J"
"-------="
"---xxxx*"
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-x---x-="
"--xxx--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@K"
"-------="
"-x-----*"
"-x----x="
"-x---x-="
"-x--x--="
"-x-x---="
"-xx-x--="
"-x---x-="
"-x----x="
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@L"
"-------="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@M"
"-------="
"-x-----*"
"-xx---x*"
"-x-x-x-*"
"-x--x--*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@N"
"-------="
"-x-----*"
"-xx----*"
"-x-x---*"
"-x-x---*"
"-x--x--*"
"-x---x-*"
"-x---x-*"
"-x----x*"
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@O"
"-------="
"---xxx-="
"--x---x="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@P"
"-------="
"-xxxxxx="
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx="
"-x-----="
"-x-----="
"-x-----="
"-x-----="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@Q"
"-------="
"---xxx-="
"--x---x="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"--x---x="
"---xxx-="
"----x--="
"-----xx="
"-------="
"-------="
"-------="
"-------="

"@R"
"-------="
"-xxxxxx="
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx="
"-x--x--="
"-x---x-="
"-x----x="
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@S"
"-------="
"--xxxxx="
"-x-----*"
"-x-----="
"-x-----="
"--xxxxx="
"-------*"
"-------*"
"-x-----*"
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@T"
"-------="
"-xxxxxx*"
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@U"
"-------="
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@V"
"-------="
"-x-----*"
"-x-----*"
"--x---x="
"--x---x="
"--x---x="
"---x-x-="
"---x-x-="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@W"
"-------="
"-x-----*"
"-x-----*"
"-x-----*"
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-x-x-x-*"
"--x---x="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@X"
"-------="
"-x-----*"
"-x-----*"
"--x---x="
"---x-x-="
"----x--="
"---x-x-="
"--x---x="
"-x-----*"
"-x-----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@Y"
"-------="
"-x-----*"
"-x-----*"
"--x---x="
"---x-x-="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@Z"
"-------="
"-xxxxxx*"
"-x-----*"
"------x="
"-----x-="
"----x--="
"---x---="
"--x----="
"-x-----*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@["
"-------="
"-----xx*"
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"-----xx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@\\"
"-------="
"-------="
"-------="
"-x-----="
"--x----="
"---x---="
"----x--="
"-----x-="
"------x="
"-------*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@]"
"-------="
"-xxx---="
"---x---="
"---x---="
"---x---="
"---x---="
"---x---="
"---x---="
"---x---="
"-xxx---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@^"
"-------="
"---x---="
"--x-x--="
"-x---x-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@_"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="

"@`"
"-------="
"-x-----="
"--x----="
"---x---="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@a"
"-------="
"-------="
"-------="
"-------="
"--xxxx-="
"------x="
"--xxxxx="
"-x----x="
"-x----x="
"--xxxx-*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@b"
"-------="
"-x-----="
"-x-----="
"-x-----="
"-x-xxx-="
"-xx---x="
"-x-----*"
"-x-----*"
"-xx---x="
"-x-xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@c"
"-------="
"-------="
"-------="
"-------="
"---xxxx="
"--x----*"
"-x-----="
"-x-----="
"--x----*"
"---xxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@d"
"-------="
"-------*"
"-------*"
"-------*"
"---xxx-*"
"--x---x*"
"-x-----*"
"-x-----*"
"--x---x*"
"---xxx-*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@e"
"-------="
"-------="
"-------="
"-------="
"---xxxx="
"--x----*"
"-xxxxxx*"
"-x-----="
"--x----="
"---xxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@f"
"-------="
"-----xx="
"----x--*"
"----x--="
"---xxx-="
"----x--="
"----x--="
"----x--="
"----x--="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@g"
"-------="
"-------="
"-------="
"-------="
"--xxxx-*"
"-x----x="
"-x----x="
"--xxxx-="
"-x-----="
"--xxxxx="
"-x-----*"
"--xxxxx="
"-------="
"-------="
"-------="
"-------="

"@h"
"-------="
"-xx----="
"--x----="
"--x----="
"--x-xxx="
"--xx---*"
"--x----*"
"--x----*"
"--x----*"
"--x----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@i"
"-------="
"----x--="
"-------="
"-------="
"---xx--="
"----x--="
"----x--="
"----x--="
"----x--="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@j"
"-------="
"----x--="
"-------="
"-------="
"---xx--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"--xx---="
"-------="
"-------="
"-------="

"@k"
"-------="
"-xx----="
"--x----="
"--x----="
"--x--xx="
"--x-x--="
"--xx---="
"--x-x--="
"--x--x-="
"-xx---x*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@l"
"-------="
"---xx--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@m"
"-------="
"-------="
"-------="
"-------="
"-xxx-xx="
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@n"
"-------="
"-------="
"-------="
"-------="
"-xx-xxx="
"--xx---*"
"--x----*"
"--x----*"
"--x----*"
"--x----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@o"
"-------="
"-------="
"-------="
"-------="
"---xxx-="
"--x---x="
"-x-----*"
"-x-----*"
"--x---x="
"---xxx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@p"
"-------="
"-------="
"-------="
"-------="
"-x-xxx-="
"-xx---x="
"-x-----*"
"-x-----*"
"-xx---x="
"-x-xxx-="
"-x-----="
"-x-----="
"-x-----="
"-------="
"-------="
"-------="

"@q"
"-------="
"-------="
"-------="
"-------="
"---xxx-*"
"--x---x*"
"-x-----*"
"-x-----*"
"--x---x*"
"---xxx-*"
"-------*"
"-------*"
"-------*"
"-------="
"-------="
"-------="

"@r"
"-------="
"-------="
"-------="
"-------="
"-xx-xxx="
"--xx---*"
"--x----="
"--x----="
"--x----="
"--x----="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@s"
"-------="
"-------="
"-------="
"-------="
"--xxxxx="
"-x-----="
"--xxxxx="
"-------*"
"-x-----*"
"--xxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@t"
"-------="
"---x---="
"---x---="
"---x---="
"-xxxxxx="
"---x---="
"---x---="
"---x---="
"---x---="
"----xxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@u"
"-------="
"-------="
"-------="
"-------="
"-x----x="
"-x----x="
"-x----x="
"-x----x="
"-x----x="
"--xxxx-*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@v"
"-------="
"-------="
"-------="
"-------="
"-xx---x*"
"--x---x="
"---x-x-="
"---x-x-="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@w"
"-------="
"-------="
"-------="
"-------="
"-x-----*"
"-x--x--*"
"-x--x--*"
"-x--x--*"
"-x-x-x-*"
"--x---x="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@x"
"-------="
"-------="
"-------="
"-------="
"--x----*"
"---x--x="
"----xx-="
"----xx-="
"---x--x="
"--x----*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@y"
"-------="
"-------="
"-------="
"-------="
"-x-----*"
"-x-----*"
"-x-----*"
"--x---x="
"---x-x-="
"----x--="
"---x---="
"--x----="
"-x-----="
"-------="
"-------="
"-------="


"@z"
"-------="
"-------="
"-------="
"-------="
"-xxxxxx="
"-x---x-="
"----x--="
"---x---="
"--x---x="
"-xxxxxx="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@{"
"-------="
"------x*"
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"----x--="
"-----x-="
"-----x-="
"-----x-="
"-----x-="
"------x*"
"-------="
"-------="
"-------="
"-------="

"@|"
"-------="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"----x--="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="


"@}"
"-------="
"-xx----="
"---x---="
"---x---="
"---x---="
"---x---="
"----x--="
"---x---="
"---x---="
"---x---="
"---x---="
"-xx----="
"-------="
"-------="
"-------="
"-------="


"@~"
"-------="
"--xx--x="
"-x--xx-="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="
"-------="

"@\177"
"-------="
"-xxxxxx*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-x-----*"
"-xxxxxx*"
"-------="
"-------="
"-------="
"-------="
"-------="
"-------=";

char *font_design_10_14 =
"@ "
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@0"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx-----xx*"
"--xx----xxx*"
"--xx---xx-x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx-xx---x*"
"--xxxx----x*"
"--xxx-----x*"
"--xx------x*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@@"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx------x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xxxxx*"
"--xx---xxxx*"
"--xx----xx-="
"--xx-------="
"---xx------="
"----xxxxxxx*"
"-----xxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@P"
"-----------="
"-----------="
"--xxxxxxx--="
"--xxxxxxxx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"--xx-----xx="
"--xxxxxxxx-="
"--xxxxxxx--="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="

"@`"
"-----------="
"-----------="
"---xx------="
"----xx-----="
"-----xx----="
"------xx---="
"-------xx--="
"--------xx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@p"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx-xxxx--="
"--xxxxxxxx-="
"---xxx---xx="
"---xx-----x*"
"---xx-----x*"
"---xx-----x*"
"---xx-----x*"
"---xxx---xx="
"---xxxxxxx-="
"---xx-xxx--="
"---xx------="
"---xx------="
"--xxxx-----="
"--xxxx-----="

"@!"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@1"
"-----------="
"-----------="
"------xx---="
"-----xxx---="
"----xxxx---="
"----xxxx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"----xxxxxx-="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@A"
"-----------="
"-----------="
"------xx---="
"-----xxxx--="
"----xx--xx-="
"----xx--xx-="
"---xx----xx="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@Q"
"-----------="
"-----------="
"-----xxxx--="
"----xxxxxx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx--xx--x*"
"--xx---xx-x*"
"--xx----xxx*"
"---xx---xxx="
"----xxxxxxx="
"-----xxxx-x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@a"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"----xxxxxx-="
"----xxxxxxx="
"----------x*"
"----------x*"
"----xxxxxxx*"
"---xxxxxxxx*"
"--xx------x*"
"--xx------x*"
"---xxxxxxxx="
"----xxxxx-x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@q"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----xxx-xx*"
"----xxxxxxx*"
"---xx---xxx="
"--xx-----xx="
"--xx-----xx="
"--xx-----xx="
"--xx-----xx="
"---xx---xxx="
"----xxxxxxx="
"-----xxx-xx="
"---------xx="
"---------xx="
"--------xxx*"
"--------xxx*"

"@\042"
"-----------="
"-----------="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@2"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xxx----xx*"
"--xxx----xx*"
"---------xx*"
"--------xxx="
"-------xxx-="
"-----xxx---="
"---xxx-----="
"--xxx------="
"--xxx------="
"--xxx-----x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@B"
"-----------="
"-----------="
"--xxxxxxx--="
"--xxxxxxxx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"--xx-----xx="
"--xxxxxxxx-="
"--xxxxxxxx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"--xx-----xx="
"--xxxxxxxx-="
"--xxxxxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@R"
"-----------="
"-----------="
"--xxxxxxx--="
"--xxxxxxxx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"--xx-----xx="
"--xxxxxxxx-="
"--xxxxxxx--="
"--xx--xx---="
"--xx---xx--="
"--xx----xx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@b"
"-----------="
"-----------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-xxxxx-="
"--xxxxxxxxx="
"--xxx-----x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx----xx="
"--xxxxxxxx-="
"--xx-xxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@r"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx--xxxx-="
"--xxxxxxxxx="
"--xxxx---xx*"
"--xxx------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="

"@#"
"-----------="
"-----------="
"------xx--x*"
"------xx--x*"
"-----xx--xx="
"-----xx--xx="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"----xx--xx-="
"----xx--xx-="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"---xx--xx--="
"---xx--xx--="
"--xx--xx---="
"--xx--xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@3"
"-----------="
"-----------="
"---xxxxxxxx="
"--xxxxxxxxx*"
"--xxx----xx*"
"----------x*"
"---------xx*"
"------xxxxx="
"------xxxxx="
"---------xx*"
"----------x*"
"----------x*"
"--xxx-----x*"
"--xxx----xx*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@C"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx------x*"
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx------x*"
"--xx------x*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@S"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xxx----xx*"
"--xx------x*"
"--xx-------="
"---xxx-----="
"----xxxxx--="
"-----xxxxxx="
"---------xx*"
"----------x*"
"--xx------x*"
"--xxx----xx*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@c"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----xxxxxx="
"----xxxxxxx*"
"---xx-----x*"
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"---xx-----x*"
"----xxxxxxx*"
"-----xxxxxx="
"-----------="
"-----------="
"-----------="
"-----------="

"@s"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"----xxxxxxx="
"---xxxxxxxx*"
"--xx-------="
"--xx-------="
"---xxxxxxx-="
"----xxxxxxx="
"----------x*"
"----------x*"
"--xxxxxxxxx="
"---xxxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@$"
"-----------="
"-----------="
"------xx---="
"------xx---="
"----xxxxxxx="
"---xxxxxxxx*"
"--xx--xx---="
"--xx--xx---="
"---xxxxxxx-="
"----xxxxxxx="
"------xx--x*"
"------xx--x*"
"--xxxxxxxxx="
"---xxxxxxx-="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@4"
"-----------="
"-----------="
"--------xx-="
"-------xxx-="
"------xxxx-="
"-----xx-xx-="
"----xx--xx-="
"---xx---xx-="
"--xx----xx-="
"--xx----xx-="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--------xx-="
"--------xx-="
"--------xx-="
"--------xx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@D"
"-----------="
"-----------="
"--xxxxxxx--="
"--xxxxxxxx-="
"--xx-----xx="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx-----xx="
"--xxxxxxxx-="
"--xxxxxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@T"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx--xx--x*"
"--xx--xx--x*"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@d"
"-----------="
"-----------="
"----------x*"
"----------x*"
"----------x*"
"----------x*"
"-----xxxx-x*"
"----xxxxxxx*"
"---xxx---xx*"
"--xxx-----x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx----xx*"
"---xxxxxxxx*"
"----xxxxx-x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@t"
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"----xxxxxx-="
"----xxxxxx-="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-------xx--="
"--------xx-="
"---------xx="
"---------x-="
"-----------="
"-----------="
"-----------="

"@%"
"-----------="
"-----------="
"--xxxx-----="
"--x--x----x*"
"--x--x----x*"
"--xxxx----x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx---xxx*"
"--xx----x--*"
"--xx----x--*"
"--------xxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@5"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx-------="
"--xx-------="
"--xxxxxxx--="
"---xxxxxxx-="
"---------xx="
"----------x*"
"----------x*"
"----------x*"
"--xx------x*"
"--xx-----xx="
"---xxxxxxx-="
"----xxxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@E"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xxxxxxxx-="
"--xxxxxxxx-="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@U"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@e"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx="
"--xx-------="
"--xx-------="
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@u"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx-----x*"
"---xxx---xx*"
"----xxxxxxx*"
"-----xxxx-x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@&"
"-----------="
"-----------="
"----xx-----="
"---xxxx----="
"--xx--xx---="
"--xx--xx---="
"--xx--xx---="
"--xx--xx---="
"----xx-----="
"----xxx----="
"---xx-xx---="
"--xx---xx--="
"--xx----xx-="
"--xxx---xx-="
"---xxxxx-xx="
"----xxxx--x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@6"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xxx----xx="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-xxxxx-="
"--xxxxxxxxx="
"--xxx----xx*"
"--xx------x*"
"--xx------x*"
"--xxx----xx*"
"---xxxxxxxx="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@F"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xxxxxxxx-="
"--xxxxxxxx-="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="

"@V"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx="
"---xx----xx="
"---xx----xx="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"-----xxxx--="
"-----xxxx--="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@f"
"-----------="
"-----------="
"------xxxxx="
"-----xxxxxx*"
"-----xx---x*"
"-----xx----="
"-----xx----="
"-----xx----="
"---xxxxxx--="
"---xxxxxx--="
"-----xx----="
"-----xx----="
"-----xx----="
"-----xx----="
"-----xx----="
"-----xx----="
"-----------="
"-----------="
"-----------="
"-----------="

"@v"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx----xx*"
"---xxx--xxx="
"----xxxxxx-="
"-----xxxx--="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@'"
"-----------="
"-----------="
"-------xx--="
"-------xx--="
"------xx---="
"------xx---="
"-----xx----="
"-----xx----="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@7"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"----------x*"
"----------x*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="

"@G"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx------x*"
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx----xxx*"
"--xx----xxx*"
"--xx------x*"
"--xx------x*"
"---xxxxxxxx*"
"----xxxxxxx="
"-----------="
"-----------="
"-----------="
"-----------="

"@W"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx-xxxx-x*"
"--xxxx--xxx*"
"---xx----xx="
"---xx----xx="
"-----------="
"-----------="
"-----------="
"-----------="

"@g"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----xxxxxx="
"----xxxxxxx*"
"---xxx---xx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx*"
"----xxxxxxx*"
"-----xxxx-x*"
"----------x*"
"---xx----xx*"
"---xxxxxxxx="
"----xxxxxx-="

"@w"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx--xx--x*"
"--xx--xx--x*"
"---xxxxxxxx="
"---xxx--xxx="
"---xx----xx="
"-----------="
"-----------="
"-----------="
"-----------="

"@("
"-----------="
"-----------="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"---xx------="
"----xx-----="
"-----xx----="
"------xx---="
"-------xx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@8"
"-----------="
"-----------="
"-----xxxx--="
"---xxxxxxxx="
"--xxx----xx*"
"--xx------x*"
"--xxx----xx*"
"---xxx--xxx="
"----xxxxxx-="
"----xxxxxx-="
"---xxx--xxx="
"--xxx----xx*"
"--xx------x*"
"--xxx----xx*"
"---xxxxxxxx="
"-----xxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@H"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@X"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"---xx----xx="
"----xx--xx-="
"----xx--xx-="
"-----xxxx--="
"------xx---="
"------xx---="
"-----xxxx--="
"----xx--xx-="
"----xx--xx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@h"
"-----------="
"-----------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-xxxxx-="
"--xxxxxxxxx="
"--xxx-----x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@x"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"---xx----xx="
"----xx--xx-="
"-----xxxx--="
"-----xxxx--="
"----xx--xx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@)"
"-----------="
"-----------="
"-----xx----="
"------xx---="
"-------xx--="
"--------xx-="
"---------xx="
"----------x*"
"----------x*"
"----------x*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"-----------="
"-----------="
"-----------="
"-----------="

"@9"
"-----------="
"-----------="
"-----xxxx--="
"----xxxxxx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"---xx-----x*"
"----xxxxxxx*"
"-----xxxx-x*"
"----------x*"
"----------x*"
"---------xx="
"--------xx-="
"----xxxxx--="
"----xxxx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@I"
"-----------="
"-----------="
"----xxxxxx-="
"----xxxxxx-="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"----xxxxxx-="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@Y"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx="
"---xx----xx="
"----xx--xx-="
"----xx--xx-="
"-----xxxx--="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@i"
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"-----xxx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----xxxx--="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@y"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx----xx*"
"---xxxxxxxx*"
"----xxxxx-x*"
"----------x*"
"----------x*"
"---xx-----x*"
"----xxxxxxx="
"-----xxxxx-="

"@*"
"-----------="
"-----------="
"------xx---="
"------xx---="
"--xx--xx--x*"
"--xx--xx--x*"
"---xx-xx-xx="
"-----xxxx--="
"------xx---="
"------xx---="
"-----xxxx--="
"---xx-xx-xx="
"--xx--xx--x*"
"--xx--xx--x*"
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@:"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@J"
"-----------="
"-----------="
"--------xxx*"
"--------xxx*"
"---------xx="
"---------xx="
"---------xx="
"---------xx="
"---------xx="
"---------xx="
"---------xx="
"---------xx="
"--xx-----xx="
"--xx-----xx="
"---xxxxxxx-="
"----xxxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@Z"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"--xx-------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@j"
"-----------="
"-----------="
"-----------="
"-------xx--="
"-------xx--="
"-----------="
"-----------="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"-------xx--="
"--xx---xx--="
"--xx---xx--="
"---xxxxx---="
"----xxx----="

"@z"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@+"
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@;"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-------x---="
"------x----="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@K"
"-----------="
"-----------="
"--xx------x*"
"--xx-----xx="
"--xx----xx-="
"--xx---xx--="
"--xx--xx---="
"--xx-xx----="
"--xxxx-----="
"--xxxx-----="
"--xx-xx----="
"--xx--xx---="
"--xx---xx--="
"--xx----xx-="
"--xx-----xx="
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@["
"-----------="
"-----------="
"---xxxxxxxx*"
"---xxxxxxxx*"
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxx-----="
"---xxxxxxxx*"
"---xxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@k"
"-----------="
"-----------="
"----xx-----="
"----xx-----="
"----xx-----="
"----xx-----="
"----xx----x*"
"----xx---xx="
"----xx--xx-="
"----xx-xx--="
"----xxxx---="
"----xxxx---="
"----xx-xx--="
"----xx--xx-="
"----xx---xx="
"----xx----x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@{"
"-----------="
"-----------="
"--------xxx="
"-------xx--="
"------xx---="
"------xx---="
"------xx---="
"-----xx----="
"----xx-----="
"----xx-----="
"-----xx----="
"------xx---="
"------xx---="
"------xx---="
"-------xx--="
"--------xxx="
"-----------="
"-----------="
"-----------="
"-----------="

"@,"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----xxx---="
"-----xxx---="
"------xx---="
"------xx---="
"-----xx----="
"----xx-----="

"@<"
"-----------="
"-----------="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"--xx-------="
"---xx------="
"----xx-----="
"-----xx----="
"------xx---="
"-------xx--="
"--------xx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@L"
"-----------="
"-----------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xx-------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@\\"
"-----------="
"-----------="
"--xx-------="
"--xx-------="
"---xx------="
"----xx-----="
"----xx-----="
"-----xx----="
"------xx---="
"------xx---="
"-------xx--="
"--------xx-="
"--------xx-="
"---------xx="
"----------x*"
"----------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@l"
"-----------="
"-----------="
"----xxxx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="

"@|"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@-"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@M"
"-----------="
"-----------="
"--x--------*"
"--xx------x*"
"--xxx----xx*"
"--xxx----xx*"
"--xxxx--xxx*"
"--xxxx--xxx*"
"--xx-xxxx-x*"
"--xx-xxxx-x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@]"
"-----------="
"-----------="
"---xxxxxxxx="
"---xxxxxxxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"--------xxx="
"---xxxxxxxx="
"---xxxxxxxx="
"-----------="
"-----------="
"-----------="
"-----------="

"@m"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xxx---xx-="
"--xxxx-xxxx="
"--xx-xxx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx--xx--x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@}"
"-----------="
"-----------="
"---xxx-----="
"-----xx----="
"------xx---="
"------xx---="
"------xx---="
"-------xx--="
"--------xx-="
"--------xx-="
"-------xx--="
"------xx---="
"------xx---="
"------xx---="
"-----xx----="
"---xxx-----="
"-----------="
"-----------="
"-----------="
"-----------="

"@."
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@>"
"-----------="
"-----------="
"----xx-----="
"-----xx----="
"------xx---="
"-------xx--="
"--------xx-="
"---------xx="
"----------x*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"-----------="
"-----------="
"-----------="
"-----------="

"@N"
"-----------="
"-----------="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxx-----x*"
"--xxxx----x*"
"--xx-xx---x*"
"--xx--xx--x*"
"--xx--xx--x*"
"--xx---xx-x*"
"--xx----xxx*"
"--xx-----xx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@^"
"-----------="
"-----------="
"------xx---="
"-----xxxx--="
"----xx--xx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@n"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xx-xxxx--="
"--xxxxxxxx-="
"--xx----xxx="
"--xx-----xx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"-----------="
"-----------="
"-----------="
"-----------="

"@~"
"-----------="
"-----------="
"----xx-----="
"---xx-xx--x*"
"--xx----xxx="
"---------xx="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@/"
"-----------="
"-----------="
"-----------="
"-----------="
"----------x*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@?"
"-----------="
"-----------="
"----xxxxxx-="
"---xxxxxxxx="
"--xx------x*"
"--xx------x*"
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"------xx---="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@O"
"-----------="
"-----------="
"-----xxxx--="
"----xxxxxx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx="
"----xxxxxx-="
"-----xxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@_"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"

"@o"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----xxxx--="
"----xxxxxx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx="
"----xxxxxx-="
"-----xxxx--="
"-----------="
"-----------="
"-----------="
"-----------="

"@\177"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------=";

char *font_design_10_14_rulings =
"@`"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="

"@p"
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"---xx------="
"----xx-----="
"-----xx----="
"------xx---="
"-------xx--="
"-------xx--="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="

"@d"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xxxxx*"
"------xxxxx*"

"@t"
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"-------xx--="
"--------xx-="
"---------xx="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"---------xx="
"--------xx-="
"-------xx--="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@a"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"xxxxxxxxxxx*"
"xxxxxxxxxxx*"

"@q"
"-----------="
"-----------="
"-----------="
"-----------="
"----------x*"
"---------xx*"
"-----xxxxxx*"
"----xxxxxxx="
"---xxx--xx-="
"--xxxx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"----xx--xx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@e"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"xxxxxxxx---="
"xxxxxxxx---="

"@u"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"--xx--xx--x*"
"--xx--xx--x*"
"---xx-xx-xx="
"----xxxxxx-="
"-----xxxx--="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@b"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"------xxxxx*"
"------xxxxx*"

"@r"
"-----------="
"-----------="
"----xxxx---="
"---xxxxxx--="
"--xx----xx-="
"--xx----xx-="
"--xx----xx-="
"--xx----xx-="
"---xxxxxx--="
"----xxxx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@f"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="

"@u"
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"-----xx----="
"----xx-----="
"---xx------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"---xx------="
"----xx-----="
"-----xx----="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@c"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"xxxxxxxx---="
"xxxxxxxx---="

"@s"
"-----------="
"-----------="
"------xx---="
"-----xxxx--="
"----xxxxxx-="
"---xx-xx-xx="
"--xx--xx--x*"
"--xx--xx--x*"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="

"@g"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"xxxxxxxxxxx*"
"xxxxxxxxxxx*"

"@w"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"----xxxxxx-="
"----xxxxxx-="
"----xx--xx-="
"----xx--xx-="
"----xxxxxx-="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@h"
"-----------="
"-----------="
"------xxxxx*"
"-----xxxxxx*"
"----xx-----="
"---xx------="
"--xx----xx-="
"--xx----xx-="
"--xx-------="
"--xx-------="
"--xx--xx---="
"--xx---xx--="
"--xx----xxx*"
"--xx----xxx*"
"---xx------="
"----xx-----="
"-----xxxxxx*"
"------xxxxx*"
"-----------="
"-----------="

"@*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"----xxxxxx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@l"
"-----------="
"-----------="
"xxxxxx-----="
"xxxxxxx----="
"------xx---="
"-------xx--="
"--xx----xx-="
"--xx----xx-="
"--------xx-="
"--------xx-="
"xx------xx-="
"xxx-----xx-="
"--xx----xx-="
"---xx---xx-="
"-------xx--="
"------xx---="
"xxxxxxx----="
"xxxxxx-----="
"-----------="
"-----------="

"@|"
"-----------="
"-----------="
"------xx---="
"-----xxxx--="
"----xxxxxx-="
"---xxxxxxxx="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"---xxxxxxxx="
"----xxxxxx-="
"-----xxxx--="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@i"
"-----------="
"-----------="
"xxxxxx-----="
"xxxxxxx----="
"------xx---="
"-------xx--="
"--xx----xx-="
"--xx----xx-="
"--------xx-="
"--------xx-="
"----xx--xx-="
"---xx---xx-="
"xxxx----xx-="
"xxx-----xx-="
"-------xx--="
"------xx---="
"xxxxxxx----="
"xxxxxx-----="
"-----------="
"-----------="

"@y"
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@m"
"-----------="
"-----------="
"----------x*"
"---------xx="
"--------xx-="
"-------xx--="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"------xx---="
"------xx---="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----xx----="
"----xx-----="
"---xx------="
"--xx-------="
"-----------="
"-----------="
"-----------="
"-----------="

"@}"
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----xxxx--="
"----xx--xx-="
"---xx----xx="
"--xx------x*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@j"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"-----xxxx--="
"----xxxxxx-="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"---xxxxxxxx="
"-----------="
"-----------="

"@z"
"-----------="
"-----------="
"-----------="
"-----------="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@n"
"-----------="
"-----------="
"----xx----x*"
"---xxxx---x*"
"--xx--xxxxx="
"--xx---xxx-="
"-----------="
"-----------="
"----xx----x*"
"---xxxx---x*"
"--xx--xxxxx="
"--xx---xxx-="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@~"
"-----------="
"-----------="
"-----------="
"-----------="
"------xx---="
"------xx---="
"-----xxxx--="
"-----xxxx--="
"----xxxxxx-="
"---xxxxxxxx="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@k"
"-----------="
"-----------="
"------xxxxx*"
"-----xxxxxx*"
"----xx-----="
"---xx------="
"--xx----xx-="
"--xx----xx-="
"--xx-------="
"--xx-------="
"--xx------x*"
"--xx-----xx*"
"--xx----xx-="
"---xx--xx--="
"----xx-----="
"-----xx----="
"------xxxxx*"
"-------xxxx*"
"-----------="
"-----------="

"@{"
"-----------="
"-----------="
"------xx---="
"-----xxxx--="
"----xx--xx-="
"---xx----xx="
"--xx------x*"
"--xx------x*"
"--xx------x*"
"--xx------x*"
"---xx----xx="
"----xx--xx-="
"-----xxxx--="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@o"
"-----------="
"-----------="
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"--xxxxxxxxx*"
"--xxxxxxxxx*"
"------xx---="
"------xx---="
"------xx---="
"------xx---="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="
"-----------="

"@\177"
"xx--xx--xx-="
"xx--xx--xx-="
"--xx--xx--x*"
"--xx--xx--x*"
"xx--xx--xx-="
"xx--xx--xx-="
"--xx--xx--x*"
"--xx--xx--x*"
"xx--xx--xx-="
"xx--xx--xx-="
"--xx--xx--x*"
"--xx--xx--x*"
"xx--xx--xx-="
"xx--xx--xx-="
"--xx--xx--x*"
"--xx--xx--x*"
"xx--xx--xx-="
"xx--xx--xx-="
"--xx--xx--x*"
"--xx--xx--x*";

