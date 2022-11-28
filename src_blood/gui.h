/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
//
// This file is part of XMAPEDIT.
//
// XMAPEDIT is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License version 2
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
////////////////////////////////////////////////////////////////////////////////////
***********************************************************************************/
#ifndef __GUI_H
#define __GUI_H

#include "build.h"
#include "baselayer.h"
#include "resource.h"
#include "gfx.h"
#include "screen.h"


#define kColorBackground	gStdColor[20]
#define kColorHighlight		gStdColor[16]
#define kColorShadow		gStdColor[24]

struct NAMED_TYPE;
extern QFONT* pFont;

// modal result codes
enum {
	mrNone		= 0x00,
	mrOk		= 0x01,
	mrCancel	= 0x02,
	mrAbort		= 0x03,
	mrRetry		= 0x04,
	mrIgnore	= 0x05,
	mrNo		= 0x06,
	mrYes		= 0x07,
	mrUser		= 0x08,
};

enum GEVENT_TYPE {
	evNone			= 0x00,
	evMouseDown		= 0x01,
	evMouseUp		= 0x02,
	evMouseRepeat	= 0x04,
	evMouseDrag		= 0x08,
	evMouseMove		= 0x10,
	evMouseWheel	= 0x20,
	evMouse			= 0xFF,

	evKeyDown		= 0x100,
	evKeyUp			= 0x200,
	evKey			= 0xF00,

	evCommand		= 0x1000,
	evBroadcast		= 0x2000,
	evMessage		= 0xF000,
};


struct MOUSE_GEVENT
{
	int time;
	int button;
	int x, y;
	int dx, dy;
	signed int wheel	: 3;
	BOOL doubleClick;
};

struct KEY_GEVENT
{
	BYTE ascii;
	BYTE make;
	union
	{
		int shiftFlags;
		struct
		{
			unsigned shift		: 1;
			unsigned control	: 1;
			unsigned alt		: 1;
			unsigned lshift		: 1;
			unsigned rshift		: 1;
			unsigned lcontrol	: 1;
			unsigned rcontrol	: 1;
			unsigned lalt		: 1;
			unsigned ralt		: 1;
		};
	};
};

struct MESSAGE
{
	int command;
};

struct GEVENT
{
	GEVENT_TYPE type;
	int time;
	union
	{
		MOUSE_GEVENT mouse;
		KEY_GEVENT key;
		MESSAGE message;
	};
	void Clear( void ) { type = evNone; }
};


extern Resource gGuiRes;

void SetBlinkOn( void );
void SetBlinkOff( void );
BOOL IsBlinkOn( void );
void UpdateBlinkClock( int ticks );
void CenterString(int x, int y, char *s, int foreColor);
void DrawBevel( int x0, int y0, int x1, int y1, int color1, int color2 );
void DrawRect( int x0, int y0, int x1, int y1, int color );
int GetStringBox( char *title, char *s );
int GetNumberBox( char *title, int n, int nDefault );


class Widget
{
	public:
		int type, left, top, width, height;
		BOOL canFocus, canDefault, isContainer;
		BOOL Contains(int x, int y)
			{ return x > left && y > top && x < left + width && y < top + height; }
		Widget *prev, *next, *owner; char hotKey;
		Widget( int left, int top, int width, int height ) :
			left(left), top(top), width(width), height(height), canFocus(FALSE), canDefault(FALSE),
			hotKey(0), isContainer(FALSE) {};
		virtual ~Widget() {};
		virtual void Paint( int x, int y, BOOL hasFocus ) = 0;
		virtual void HandleEvent( GEVENT *event ) = 0;
		virtual void EndModal( int result ) { owner->EndModal(result); }
};

typedef void (*CLICK_PROC)( Widget * );


class HeadWidget : public Widget
{
	public:
		HeadWidget() : Widget(0, 0, 0, 0) {};
		virtual void Paint( int, int, BOOL ) {};
		virtual void HandleEvent( GEVENT * ) {};
};

class Tile : public Widget
{
	public:
		int tilenum;
		int scaleX;
		int scaleY;
		int plu;
		char flags;
		Tile( int left, int top, int tilenumArg, int scaleXArg = 0, int scaleYArt = 0, int pluArg = 0, char flags = 0x0001);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};

class Message : public Widget
{
	public:
		int clock;
		short bgColor;
		char fontColor;
		char string[256];
		QFONT* font;
		Message(int left, int top, char* s, QFONT* pFont = pFont, char fontColor = 0, short bgColor = -1, ushort clockArg = 300);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};

class Shape : public Widget
{
	public:
		int bgcolor;
		Shape( int left, int top, int width, int height, int bgcol);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};


class Label : public Widget
{
	public:
		short bgColor;
		char fontColor;
		char string[256];
		QFONT* font;
		Label(int left, int top, char* s, char fontColor = 0, QFONT* pFont = pFont, short bgColor = -1);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};

enum {
kTextALeft			= 0x00,
kTextATop			= 0x00,
kTextACenter		= 0x01,
kTextAMiddle		= 0x02,
kTextARight			= 0x04,
kTextABottom		= 0x08,
kTextUnderline		= 0x10,
kTextOverline		= 0x20,
kTextLineThrough	= 0x40,
kTextBlink			= 0x80,
kTextUppercase		= 0x100,
kTextLowercase		= 0x200,
kTextBold			= 0x400,
};

class Text : public Widget
{
	public:
		short bgColor;
		char fontColor;
		char* text;
		int textlen, lines, flags;
		QFONT* font;
		Text(int left, int top, int width, int height, char* s, int flags = 0, char fontColor = 0, QFONT* pFont = pFont, short bgColor = -1);
		~Text();
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};



class Progress : public Widget
{
	public:
		int valPerc;
		short bgColor, bdColor, fnColor;
		QFONT* font;
		Progress(int left, int top, int width, int height, int perc = 0, int bdColorA = 0, int bgColorA = 5, int fnColorA = 15, QFONT* pFont = qFonts[0]);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * ) {};
};

/* class Screen : public Widget
{
	public:
		Screen( int left, int top, int width, int height);
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT * );
}; */

class Container : public Widget
{
	public:
		BOOL isModal;
		int endState;
		Widget *focus;
		Widget *drag;	// receives drag and up events
		HeadWidget head;
		void Insert( Widget *widget );
		void Remove( Widget *widget );
		Container( int left, int top, int width, int height);
		virtual ~Container();
		virtual BOOL SetFocus( int dir );
		virtual BOOL SetFocusOn(Widget* pFocus);
		virtual void ClearFocus();
		virtual void Paint( int x, int y, BOOL hasFocus );
		virtual void HandleEvent( GEVENT *event );
		virtual void EndModal( int result );
};


class Panel : public Container
{
public:
	int size1, size2, size3, bgColor;
	Panel(int left, int top, int width, int height, int size1 = 0, int size2 = 0, int size3 = 0, int bgColor = kColorBackground) :
		Container(left, top, width, height), size1(size1), size2(size2), size3(size3), bgColor(bgColor)
		{ canFocus = FALSE; };
	virtual void Paint( int x, int y, BOOL hasFocus );
};


class TitleBar : public Widget
{
public:
	char string[256];
	int len;
	TitleBar( int left, int top, int width, int height, char *s );
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT * );
};


class Window : public Panel
{
	public:
		Container *client;
		TitleBar *titleBar;
		void Insert( Widget *widget ) { client->Insert(widget); }
		Window( int left, int top, int width, int height, char *title);
		void getEdges(int *x1, int *y1, int *x2, int *y2);
		void getSize(int *width, int *height);
};


class Button : public Widget
{
public:
	int result;
	BOOL pressed;
	BOOL disabled;
	CLICK_PROC clickProc;
	Button( int left, int top, int width, int height, int result ) :
		Widget(left, top, width, height), result(result), pressed(FALSE), disabled(FALSE), clickProc(NULL)
		{}
	Button( int left, int top, int width, int height, CLICK_PROC clickProc ) :
		Widget(left, top, width, height), result(mrNone), pressed(FALSE), disabled(FALSE), clickProc(clickProc)
		{}
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class BitButton2 : public Button
{
	QBITMAP* pBitmap;
public:
	BitButton2( int left, int top, int width, int height, QBITMAP* pBitmap, int result );
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class TextButton : public Button
{
public:
	char *text;
	QFONT* font;
	ushort fontColor;
	TextButton( int left, int top, int width, int height, char *text, int result );
	TextButton( int left, int top, int width, int height, char *text, CLICK_PROC clickProc);
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class Checkbox : public Widget {
public:
	int  id;
	BOOL pressed;
	BOOL checked;
	BOOL disabled;
	char label[64];
	int result;
	Checkbox(int left, int top, BOOL value, char* l);
	Checkbox(int left, int top, BOOL value, char* l, int rslt);
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class ColorButton : public Button
{
public:
	short bgcolor;
	BOOL checked;
	BOOL active;
	ColorButton( int left, int top, int width, int height, short bgcolor, CLICK_PROC clickProc) :
		Button( left, top, width, height, clickProc), bgcolor(bgcolor)
		{
			canFocus = FALSE;
			checked = FALSE;
			pressed = FALSE;
			active = FALSE;
		};
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class ColorPick : public Container
{
public:
	int value;
	int rows;
	int cols;
	short colSize;
	BYTE* colors;
	//char order[256];
	ColorPick* connect;
	ColorPick(int left, int top, int rows, int cols, short colSize, BYTE* colors) :
		Container(left, top, width, height), rows(rows), cols(cols), colSize(colSize), colors(colors)
		{ canFocus = FALSE; };
};

class ColorSelect : public ColorPick
{
	Container *client;
public:
	void Insert( Widget *widget ) { client->Insert(widget); }
	ColorSelect(int left, int top, int rows, int cols, short colSize, BYTE* colors, char flags = 0);
};


class FieldSet : public Container
{
public:
	char  title[128];
	char  fontColor, borderColor;
	virtual void Paint( int x, int y, BOOL hasFocus );
	FieldSet(int left, int top, int width, int height, char* s = NULL, char fontCol = 0, short brCol = 0) : Container(left, top, width, height)
	{
		canFocus	= FALSE;
		fontColor	= fontCol;
		borderColor	= brCol;
		memset(title, 0, sizeof(title));
		if (s)
			strcpy(title, s);
	};
};


class BitButton : public Button
{
	RESHANDLE hBitmap;
public:
	BitButton(int left, int top, int width, int height, RESHANDLE hBitmap, CLICK_PROC clickProc) :
	Button(left, top, width, height, clickProc), hBitmap(hBitmap) {};
	virtual void Paint( int x, int y, BOOL hasFocus );
};

struct PLUINFO
{
	unsigned int id				: 8;
	unsigned int efficency		: 7;
	unsigned int pixelsAltered	: 16;
	unsigned int pixelsTotal	: 16;
};

struct PLUPICK_PREFS
{
	unsigned int nTile			: 16;
	signed   int nShade			: 10;
	unsigned int nPlu			: 8;
	PLUINFO* pluInfo;
	unsigned int pluCnt			: 10;
	unsigned int nCols			: 10;
	unsigned int nRows			: 10;
};

class PluPick : public Widget
{
public:
	unsigned int nRows			: 10;
	unsigned int colHg			: 16;
	unsigned int nCols			: 10;
	unsigned int colWh			: 16;
	signed   int nStart			: 10;
	signed   int nCursor		: 10;
	unsigned int value			: 10;
	PLUPICK_PREFS* pPrefs;
	PluPick(int left, int top, int width, int height, PLUPICK_PREFS* pPrefs);
	virtual void Paint(int x, int y, BOOL hasFocus);
	virtual void HandleEvent(GEVENT *event);
	void SetCursor(int nValue);
	void ClipStart();
};

class EditText : public Widget
{
public:
	char string[256];
	char placeholder[256];
	int len;
	int pos;
	int maxlen;
	int flags;
	EditText( int left, int top, int width, int height, char *s, int flags = 0x0);
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class TextArea : public EditText
{
	
private:
	struct LINE_RANGE
	{
		int p1, p2;
	};
public:
	char* text;
	int lines, line;
	LINE_RANGE *linerng;
	TextArea( int left, int top, int width, int height, char *s );
	virtual void HandleEvent(GEVENT *event);
	virtual void Paint(int x, int y, BOOL hasFocus);
	void FormatText(char* text, int flags = 0x0);
	void GetCursorPos(int* x = NULL, int* y = NULL);
	int  GetLineStartByPos(int pos);
	int  GetLineByPos(int pos);
};


class EditNumber : public EditText
{
public:
	int value;
	EditNumber( int left, int top, int width, int height, int n );
	virtual void HandleEvent( GEVENT *event );
};

enum {
kModalNoCenter 			= 0x01,
kModalFadeScreen		= 0x02,
kModalNoFocus			= 0x04,
};

int ShowModal(Container *dialog, int flags = 0x0);
int showButtons(NAMED_TYPE* names, int namesLen, char* title);
int showButtons(char** names, int namesLen, char* title);

struct CHECKBOX_LIST_P {
	
	BOOL* option;
	char label[64];
	Checkbox* checkbox;
	
};

struct CHECKBOX_LIST {
	
	BOOL option;
	char label[64];
	Checkbox* checkbox;
	
};

int createCheckboxList(CHECKBOX_LIST_P* array, int len, char* title, BOOL buttons = TRUE);
int createCheckboxList(CHECKBOX_LIST* array, int len, char* title, BOOL buttons = TRUE);

char fade(int rate = 120);
void GUIInit();
int YesNoCancel(char *__format, ...);
void Alert(char *__format, ...);
BOOL Confirm(char *__format, ...);
#endif

