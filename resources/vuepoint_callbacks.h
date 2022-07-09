#ifndef VUEPOINT_CALLBACKS
#define VUEPOINT_CALLBACKS

#define prcVisualSizeX	512
#define prcVisualSizeY	256

enum {prcCursorTypeUnknown, prcCursorTypeLShape, prcCursorTypeCrosshair, prcCursorTypeUnderscore, prcCursorTypeBlock};
enum {STEP_RESUME, STEP_TOGGLE, STEP_HOLD};


extern int prcInit (void);
extern int prcProcess (void);
extern void whndCreateTestMaps (void);
extern void prcUpdate (void);
extern void prcPause (void);

extern int prcStepControl (int);
extern void prcDebugOff (int *);
extern void prcExit(int *);
extern void prcStepMode (int *);
extern void prcSetGraphicWindow (int *);
extern void prcSnapshot (int *);
extern void prcVectGraph (int *);
extern void prcCircleDraw (int *);
extern void prcClearFileBuffer (int *);
extern void prcClearTabs (int *);
extern void prcCombineBitMap (int *);
extern void prcControlBrightness (int *);
extern void prcCursorControl (int *);
extern void prcCursorReposition (int *);
extern void prcCursor_Backspace (int *);
extern void prcCursor_CarriageReturn (int *);
extern void prcCursor_Down (int *);
extern void prcCursor_Formfeed (int *);
extern void prcCursor_Home(int *);
extern void prcCursor_Left (int *);
extern void prcCursor_Linefeed (int *);
extern void prcCursor_Right (int *);
extern void prcCursor_Tab (int *);
extern void prcCursor_Up (int *);
extern void prcCursor_VTab (int *);
extern void prcDashParam (int *);
extern void prcDefineBitMap (int *);
extern void prcDefineWindow (int *);
extern void prcDeleteChar (int *);
extern void prcDeleteFile (int *);
extern void prcDeleteRow (int *);
extern void prcDeleteWindow (int *);
extern void prcDetectBitMap (int *);
extern void prcDisableBitMap (int *);
extern void prcDisableCursor (int *);
extern void prcDisableFullPageLatch (int *);
extern void prcDisableNewLine (int *);
extern void prcDisableOverstrike (int *);
extern void prcDisablePseudoNewLine (int *);
extern void prcDisableScrollEdit (int *);
extern void prcDisableTouchPanel (int *);
extern void prcDisableXON (int *);
extern void prcDisplayBitMap (int *);
extern void prcEnableBitMap (int *);
extern void prcEnableBlockCursor (int *);
extern void prcEnableCrosshair (int *);
extern void prcEnableCursor (int *);
extern void prcEnableFullPageLatch (int *);
extern void prcEnableIncrementalPlot (int *);
extern void prcEnableMonitorMode (int *);
extern void prcEnableNewLine (int *);
extern void prcEnableOverstrike (int *);
extern void prcEnablePrimaryFont (int *);
extern void prcEnablePseudoNewLine (int *);
extern void prcEnableScrollEdit (int *);
extern void prcEnableSecondaryFont (int *);
extern void prcEnableTouchPanelContinuous (int *);
extern void prcEnableTouchPanelLatch (int *);
extern void prcEnableUnderscore (int *);
extern void prcEnableXON (int *);
extern void prcEnterChar (int *);
extern void prcInsertChar (int *);
extern void prcInsertRow (int *);
extern void prcLoadPCGSymbol (int *);
extern void prcMoveAbsolute (int *);
extern void prcObjTblDraw (int *);
extern void prcObjTblStart (int *);
extern void prcProcessFile (int *);
extern void prcRasterWrite (int *);
extern void prcRepeatLoop (int *);
extern void prcRequests(int *);
extern void prcSaveFile (int *);
extern void prcSelectBitMap (int *);
extern void prcSelectFonts (int *);
extern void prcSelectSerialPort (int *);
extern void prcSelectWindow (int *);
extern void prcSendTouchPanelSize (int *);
extern void prcSetBaudRate (int *);
extern void prcSetFontAttribute (int *);
extern void prcSetSerialValues(int *);
extern void prcSetTab(int *);
extern void prcSetVertTab (int *);
extern void prcSoftwareReset(int *);
extern void prcSplitScreen (int *);
extern void prcStartFile (int *);
extern void prcStartFileProcess (int *);
extern void prcTransmitFile (int *);
extern void prcStartRepeat (int *);
extern void prcTestPattern(int *);
extern void prcTransmitPage (int *);
extern void prcTransmitRow (int *);
extern void prcUpdateOff (int *);
extern void prcUpdateOn (int *);
extern void prcUpdateRange (int *);

#endif
