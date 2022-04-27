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
#include "common_game.h"
#include "editor.h"
#include "xmpstub.h"
#include "gfx.h"
#include "keyboard.h"
#include "misc.h"
#include "gameutil.h"
#include "gui.h"
#include "screen.h"
#include "tile.h"
#include "osd.h"
#include "enumstr.h"


#include "xmpconf.h"
#include "xmpmisc.h"

#define kBlinkTicks		100
#define kBlinkOnTicks	60

static char modalmsg[128];
static int blinkClock = 0;

Resource gGuiRes;
QFONT* pFont;

int QFNScaleTable[][5] = {
	// ydim, xscale, yscale, xstep, ystep
	{200, 0x10000,	0x10000, 0x10000, 0x10000},
	{480, 0x10000,	0x10000, 0x10000, 0x10000},
	
};

BOOL cmpHotKey(char keyA, char keyB) {
	
	return (keyA && keyAsciiSH[keyB] == keyA);
	
}

void SetBlinkOn( void )
{
	blinkClock = 0;
}


void SetBlinkOff( void )
{
	blinkClock = kBlinkOnTicks;
}


BOOL IsBlinkOn( void )
{
	return blinkClock < kBlinkOnTicks;
}


void UpdateBlinkClock( int ticks )
{
	blinkClock += ticks;
	while (blinkClock >= kBlinkTicks)
		blinkClock -= kBlinkTicks;
}

static void CenterLabel(int x, int y, char *s, int foreColor, QFONT* pFont = qFonts[0])
{
	if ( pFont != NULL ) y -= pFont->height / 2;
	else y -= 4;

	gfxDrawLabel(x - gfxGetLabelLen(s, pFont) / 2, y, foreColor, s, pFont);
}


void DrawBevel( int x0, int y0, int x1, int y1, int color1, int color2 )
{
	gfxSetColor(color1);
	gfxHLine(y0, x0, x1 - 2);
	gfxVLine(x0, y0 + 1, y1 - 2);
	gfxSetColor(color2);
	gfxHLine(y1 - 1, x0 + 1, x1 - 1);
	gfxVLine(x1 - 1, y0 + 1, y1 - 2);
}


void DrawRect( int x0, int y0, int x1, int y1, int color )
{
	gfxSetColor(color);
	gfxHLine(y0, x0, x1 - 1);
	gfxHLine(y1 - 1, x0, x1 - 1);
	gfxVLine(x0, y0 + 1, y1 - 2);
	gfxVLine(x1 - 1, y0 + 1, y1 - 2);
}


void DrawButtonFace( int x0, int y0, int x1, int y1, BOOL pressed )
{
	gfxSetColor(kColorBackground);
	gfxFillBox(x0, y0, x1, y1);
	if (pressed)
	{
		gfxSetColor(gStdColor[26]);
		gfxHLine(y0, x0, x1 - 1);
		gfxVLine(x0, y0 + 1, y1 - 1);
		gfxSetColor(gStdColor[24]);
		gfxHLine(y0 + 1, x0 + 1, x1 - 1);
		gfxVLine(x0 + 1, y0 + 2, y1 - 1);
		DrawBevel(x0 + 2, y0 + 2, x1, y1, gStdColor[19], gStdColor[22]);
	}
	else
	{
		DrawBevel(x0, y0, x1, y1, gStdColor[16], gStdColor[24]);
		DrawBevel(x0 + 1, y0 + 1, x1 - 1, y1 - 1, gStdColor[18], gStdColor[22]);
	}
}


void DrawMouseCursor( int x, int y ) {
	
	gfxDrawBitmap(pBitmaps[kBitmapMouseCursor], x, y);
	
}


char GetHotKey( char *string )
{
	for ( char *p = string; *p; p++ )
	{
		if ( *p == '&' )
			return (char)toupper(p[1]);
	}

	return '\0';
}



Shape::Shape( int left, int top, int width, int height, int bgcol) : Widget(left, top, width, height) {
	
	canFocus = FALSE;
	bgcolor = bgcol;
	
}

void Shape::Paint( int x, int y, BOOL )
{
	gfxSetColor(bgcolor);
	gfxFillBox(x, y, x + width, y + height);
}



/* Screen::Screen( int left, int top, int width, int height) : Widget(left, top, width, height) {
	
	canFocus = TRUE;
	setview(left, top, left + width,  top + height);	
}

void Screen::Paint( int x, int y, BOOL )
{

	ExtPreCheckKeys();

	drawrooms(posx,posy,posz,ang,horiz,cursectnum);
	ExtAnalyzeSprites();
	drawmasks();

	ExtCheckKeys();

	nextpage();
	updateClocks();

}

void Screen::HandleEvent(GEVENT * event) {
	
	
} */


Tile::Tile(int left, int top, int tilenumArg, int scaleXArg, int scaleYArg, int pluArg, char flagsArg) : Widget(left, top, 0, 0) {
	
	canFocus = FALSE;
	tilenum = tilenumArg;
	scaleX = scaleXArg;
	scaleY = scaleYArg;
	plu = pluArg;
	flags = flagsArg;
	
}



void Tile::Paint( int x, int y, BOOL )
{

	tileDrawTile(x, y, (short)tilenum, scaleX, (short)plu, flags);
}

Message::Message(int left, int top, char* s, QFONT* pFont, char fontColorA, short bgColorA, ushort clockArg) : Widget(left, top, width, height) {
	
	font = pFont;
	canFocus = FALSE;
	bgColor = bgColorA;
	fontColor = fontColorA;
	clock = totalclock + clockArg;
	strcpy(string, s);
	
	width = gfxGetTextLen(s, font);
	height = font->height;
	
}

void Message::Paint( int x, int y, BOOL )
{
	if (clock > totalclock)
	{
		if (bgColor >= 0)
		{
			gfxSetColor(bgColor);
			gfxFillBox(x, y, x + width, y + height);
		}
		
		gfxDrawLabel(x, y, gStdColor[fontColor], string, font);
	}
}

Label::Label(int left, int top, char* s, char fontColorA, QFONT* pFont, short bgColorA) : Widget(left, top, 0, 0)
{
	
	font = pFont;
	canFocus = FALSE;
	bgColor = bgColorA;
	fontColor = fontColorA;
	strcpy(string, s);
	
	width = gfxGetTextLen(s, font);
	height = font->height;
}


void Label::Paint( int x, int y, BOOL )
{
	if (bgColor >= 0)
	{
		gfxSetColor(bgColor);
		gfxFillBox(x, y, x + width, y + height);
	}
	
	gfxDrawLabel(x, y, gStdColor[fontColor], string, font);
}

Text::Text(int left, int top, int width, int height, char* s, int flagsA, char fontColorA, QFONT* pFont, short bgColorA) : Widget(left, top, width, height)
{
	int i, j;
	font		= pFont;
	canFocus	= FALSE;
	bgColor		= bgColorA;
	fontColor	= fontColorA;
	lines		= 0;
	flags		= flagsA;

	if ((textlen = strlen(s)) > 0)
	{
		lines++;
		text = (char*)Resource::Alloc(textlen);
		memset(text, 0, textlen);
		sprintf(text, s);
		
		for (i = 0; i < textlen; i++)
		{
			if (text[i] == '\t')
			{
				text[i] = 32;
				continue;
			}
			else if (text[i] == '\r')
			{
				if (i+1 < textlen && text[i+1] == '\n')
				{
					for (j = i; j < textlen; j++) text[j] = text[j+1];
					textlen--;
				}
			}
			else if (text[i] != '\n')
				continue;
			
			text[i] = 0;
			lines++;
		}
	}
}

Text::~Text() {
	
	if (text != NULL)
		Resource::Free(text);
}

void Text::Paint( int x, int y, BOOL )
{
	char *txt;
	int i, j, stlen, gflen, fh = font->height;
	int dx = x, dy = y, ystp = 0;
	bool bold = false;
	char fntcol = gStdColor[fontColor]; 
	
	if (font->type != kFontTypeMono)
	{
		for (i = j = 0; i < LENGTH(font->info); i++)
		{
			QFONTCHAR* pChar =& font->info[i];
			if (pChar)
				j += (pChar->w*pChar->h);
		}
		
		if (j > 0)
			fntcol = mostUsedColor((BYTE*)font->data, j, 255);
	}

	if (bgColor >= 0)
	{
		gfxSetColor(gStdColor[bgColor]);
		gfxFillBox(x, y, x + width, y + height);
	}
	
	if ((flags & kTextBlink) && !(totalclock & 64))
		return;
	
	if (flags & kTextUnderline)	ystp++;
	if (flags & kTextOverline)	ystp++;
	if (flags & kTextBold)		bold = true;
	
	gfxSetColor(fntcol);
	if (flags & kTextABottom) dy = y+(height-(fh*lines));
	else if (flags & kTextAMiddle)
		dy = y+((height>>1)-(((fh+ystp)*lines)>>1));
	
	i = 0;
	while (i < textlen)
	{
		txt =& text[i];
		if (flags & kTextUppercase) strupr(txt);
		else if (flags & kTextLowercase)
			strlwr(txt);
				
		gflen = gfxGetTextLen(txt, font);
		if (bold)
			gflen++;
		
		if (flags & kTextARight) dx = x+(width-gflen);
		else if (flags & kTextACenter)
			dx = x+((width>>1)-(gflen>>1));		
		
		if (flags & kTextOverline)
		{
			gfxHLine(dy-1, dx, dx+gflen);
			if (bold)
				gfxHLine(dy, dx, dx+gflen);
		}
		
		gfxDrawText(dx, dy, fntcol, txt, font);
		// not the best way, but hey...
		if (bold)
			gfxDrawText(dx+1, dy, fntcol, txt, font);
				
		if (flags & kTextUnderline)
		{
			gfxHLine(dy+fh, dx, dx+gflen);
			if (bold)
				gfxHLine(dy+fh+1, dx, dx+gflen);
		}
		
		if (flags & kTextLineThrough)
		{
			gfxHLine(dy+(fh>>1), dx, dx+gflen);
			if (bold)
				gfxHLine(dy+1+(fh>>1), dx, dx+gflen);
		}

		i+=strlen(txt)+1;
		dy+=(fh+ystp);
	}
	
// word-by-word printing (WIP)
/* 
	char* pWord = (char*)Resource::Alloc(textlen);
	memset(pWord, 0, textlen);
	word = pWord;

while (i < textlen)
	{
		j = 0; memset(word, 0, textlen);
		while(i < textlen && isgraph(text[i])) word[j++] = text[i++];
		if (j > 0) // print the word
		{
			gflen = gfxGetTextLen(word, font);
			if (dx + gflen > x + width)
			{
				if ((dy += fh) > y + height) break; // overflow
				dx = x;
			}

			gfxDrawText(dx, dy, gStdColor[fontColor], word, font);
			dx+=gflen;
		}
		
		// skip anything non-printable
		j = 0; memset(word, 0, textlen);
		while(i < textlen && !isgraph(text[i]))
		{
			word[j++] = text[i];
			if (text[i] == 0) break;
			i++;
		}
		
		if (i >= textlen) break;
		else if (text[i] == 0) dx = x, dy+=fh, i++; // new line!
		else dx += gfxGetTextLen(word, font);
	}
	
	Resource::Free(pWord); */
	
}

Progress::Progress(int left, int top, int width, int height, int perc, int bdColorA, int bgColorA, int fnColorA, QFONT* pFont) : Widget(left, top, width, height)
{
	valPerc  = perc;
	canFocus = FALSE;
	bgColor = bgColorA;
	bdColor = bdColorA;
	fnColor = fnColorA;
	font = pFont;
}

void Progress::Paint(int x, int y, BOOL)
{
	char tmp[8];
	int len, dp, dx = x, dy = y, dw = width, dh = height;
	
	sprintf(tmp, "%d%%", valPerc);
	len = gfxGetTextLen(tmp, font);
	if (bgColor >= 0)
	{
		gfxSetColor(gStdColor[bgColor]);
		if (width < height)
		{
			dp = (dh*valPerc)/100; 
			gfxFillBox(dx, dy+dh-dp, dx+dw, dy+dh);
		}
		else
		{
			dp = (dw*valPerc)/100; 
			gfxFillBox(dx, dy, dx+dp, dy+dh);
		}
	}
	
	gfxDrawText(dx+((dw>>1)-(len>>1)), dy+((dh>>1)-(font->height>>1)), gStdColor[fnColor], tmp, font);

	if (bdColor >= 0)
		DrawRect(dx, dy, dx+dw, dy+dh, gStdColor[bdColor]);
	
}


Container::Container( int left, int top, int width, int height) : Widget(left, top, width, height)
{
	head.next = &head;
	head.prev = &head;
	focus = &head;
	isContainer = TRUE;
	isModal = FALSE;
	endState = mrNone;
}


Container::~Container()
{
	for (Widget *w = head.next; w != &head; w = head.next)
	{
		Remove(w);
		delete w;
	}
}


BOOL Container::SetFocus( int dir )
{
	do
	{
		if (focus->isContainer)
		{
			if (((Container *)focus)->SetFocus(dir))
				return TRUE;
		}

		if (dir > 0) focus = focus->next;
		else focus = focus->prev;
		
		//if (focus->canDefault == 0) continue;
		if (focus == &head)
			return FALSE;
	}
	while (!focus->canFocus);
	return TRUE;
}


void Container::Insert( Widget *widget )
{
	dassert(widget != NULL);
	widget->prev = head.prev;
	widget->next = &head;
	widget->prev->next = widget;
	widget->next->prev = widget;
	widget->owner = this;
}


void Container::Remove( Widget *widget )
{
	dassert(widget != NULL);
	widget->prev->next = widget->next;
	widget->next->prev = widget->prev;
}


void Container::Paint( int x, int y, BOOL /* hasFocus */ )
{
	for ( Widget *w = head.next; w != &head; w = w->next)
	{
		w->Paint(x + w->left, y + w->top, w == focus);
	}
}


void Container::HandleEvent( GEVENT *event )
{
	
	if ( event->type & evMouse )
	{
		// make event relative to this container
		event->mouse.x -= left;
		event->mouse.y -= top;
		

		// find child owning location
		if ( event->type == evMouseDown )
		{
			
			drag = NULL;
			// iterate in reverse order since widgets on top have priority
			for ( Widget *w = head.prev; w != &head; w = w->prev)
			{
				if ( w->Contains(event->mouse.x, event->mouse.y) )
				{
					drag = w;
					if (drag->canFocus)
						focus = w;
					break;
				}
			}
		}

		if (drag != NULL)
			drag->HandleEvent(event);
	}
	else if ( event->type == evKeyDown )
	{
		if ( event->key.alt && keyAsciiSH[event->key.make] )
		{
			for ( Widget *w = head.prev; w != &head; w = w->prev)
			{
				if (w->hotKey && keyAsciiSH[event->key.make] == w->hotKey && w->canFocus )
				{
					focus = w;
					focus->HandleEvent(event);
					return;
				}
			}
		}

		focus->HandleEvent(event);

		// if key event not handled by focus, then broadcast to all childen with matching hot key
		if ( event->type != evNone )
		{
			for ( Widget *w = head.prev; w != &head; w = w->prev)
			{
				// set focus to the widget with the matching hot key
				if (w->hotKey && keyAsciiSH[event->key.make] == w->hotKey && w->canFocus )
				{
					focus = w;
					focus->HandleEvent(event);
					return;
				}
			}
		}
	}
}


void Container::EndModal( int result )
{
	
	if ( isModal )
	{
		endState = result;
		isModal = FALSE;
	}
	else
		owner->EndModal(result);
	
	keyClear();
	keystatus[KEY_SPACE] = 0;
}

void FieldSet::Paint( int x, int y, BOOL hasFocus ) {
	
	char col = gStdColor[0];
	int len = 0, pad = 4, w1 = width, w2 = ClipLow(5*w1/100, pad), h = height;
	int fh = pFont->height;
	
	gfxSetColor(col);
	if ((len = gfxGetTextLen(title, pFont)) > 0)
		y+=(fh>>1);
		
	gfxVLine(x, y, y+h);
	gfxVLine(x+w1, y, y+h);
	gfxHLine(y+h, x, x+w1);
	
	if (len)
	{
		gfxHLine(y, x, x+w2);
		gfxDrawText(x+w2+pad, y-(fh>>1), col, title, pFont);
		gfxHLine(y, x+w2+len+(pad<<1), x+w1);
	}
	else
	{
		gfxHLine(y, x, x+w1);
	}
	
	Container::Paint(x, y, hasFocus);
}

void Panel::Paint( int x, int y, BOOL hasFocus )
{
	int i, n = 0;
	if (bgColor >= 0)
	{
		gfxSetColor(bgColor);
		gfxFillBox(x, y, x + width, y + height);
	}
	
	for (i = klabs(size1); i > 0; n++, i--)
	{
		if (size1 > 0)
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorHighlight, kColorShadow);
		else
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorShadow, kColorHighlight);
	}

	n += size2;

	for (i = klabs(size3); i > 0; n++, i--)
	{
		if (size3 > 0)
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorHighlight, kColorShadow);
		else
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorShadow, kColorHighlight);
	}

	Container::Paint(x, y, hasFocus);
}


TitleBar::TitleBar( int left, int top, int width, int height, char *s ) : Widget(left, top, width, height)
{
	strcpy(string, s);
	len = strlen(string);
}


void TitleBar::Paint( int x, int y, BOOL /* hasFocus */ )
{
	gfxSetColor(gStdColor[1]);
	gfxFillBox(x, y, x + width, y + height);
	DrawBevel(x, y, x + width, y + height, gStdColor[9], gStdColor[30]);
	CenterLabel(x + width / 2, y + height / 2, string, gStdColor[15]);
}


void TitleBar::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse && event->mouse.button == 0 )
	{
		switch (event->type)
		{
			case evMouseDrag:
				owner->left = ClipRange(owner->left + event->mouse.dx, 0, xdim - owner->width - 1);
				owner->top = ClipRange(owner->top + event->mouse.dy, 0, ydim - owner->height - 1);
				event->Clear();
				break;
			case evMouseUp:
				break;
		}
	}
}


Window::Window( int left, int top, int width, int height, char *title) : Panel(left, top, width, height, 1, 1, -1)
{
	titleBar = new TitleBar(3, 3, width - 6, 12, title);
	client = new Container(3, 15, width - 6, height - 18);

	Container::Insert(titleBar);
	Container::Insert(client);
	drag = NULL;
}

void Window::getEdges(int *x1, int *y1, int *x2, int *y2)
{
	*x1 = 3,  *x2 = *x1 + width - 12;
	*y1 = 3,  *y2 = *y1 + height - 23;
}

void Window::getSize(int *wh, int *hg)
{
	int x1, y1, x2, y2;
	getEdges(&x1, &y1, &x2, &y2);
	*wh = x2 - x1, *hg = y2 - y1;
}

void Button::Paint( int x, int y, BOOL /* hasFocus */ )
{
	gfxSetColor(gStdColor[0]);
	gfxHLine(y, x + 1, x + width - 2);
	gfxHLine(y + height - 1, x + 1, x + width - 2);
	gfxVLine(x, y + 1, y + height - 2);
	gfxVLine(x + width - 1, y + 1, y + height - 2);

	DrawButtonFace(x + 1, y + 1, x + width - 1, y + height - 1, pressed);
}


void Button::HandleEvent( GEVENT *event )
{
	
	if (disabled) {
		if (event->type > 0) event->Clear();
		return;
	}
		
	
	if ( event->type == evKeyDown )
	{
		if (event->key.ascii == ' ' || (hotKey && keyAsciiSH[event->key.make] == hotKey))
		{
			pressed = !pressed;
			if (clickProc != NULL)
				clickProc(this);
			if ( result )
				EndModal(result);
			event->Clear();
		}
	}
	else if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
				pressed = TRUE;
				event->Clear();
				break;

			case evMouseDrag:
				pressed = Contains(event->mouse.x, event->mouse.y);
				event->Clear();
				break;

			case evMouseUp:
				pressed = FALSE;
				if ( Contains(event->mouse.x, event->mouse.y) )
				{
					if (clickProc != NULL)
						clickProc(this);
					if ( result )
						EndModal(result);
				}
				event->Clear();
				break;
		}
	}
}


TextButton::TextButton( int left, int top, int width, int height, char *text, int result ) :
	Button(left, top, width, height, result), text(text)
{
	font = pFont;
	fontColor = 0;
	hotKey = GetHotKey(text);
	canFocus = TRUE;
}

void TextButton::Paint( int x, int y, BOOL hasFocus )
{
	gfxSetColor(gStdColor[0]);
	gfxHLine(y + 1, x + 2, x + width - 3);
	gfxHLine(y + height - 2, x + 2, x + width - 3);
	gfxVLine(x + 1, y + 2, y + height - 3);
	gfxVLine(x + width - 2, y + 2, y + height - 3);

	if (hasFocus)
	{
		gfxSetColor(gStdColor[15]);
		gfxHLine(y, x + 1, x + width - 2);
		gfxHLine(y + height - 1, x + 1, x + width - 2);
		gfxVLine(x, y + 1, y + height - 2);
		gfxVLine(x + width - 1, y + 1, y + height - 2);
		gfxPixel(x + 1, y + 1);
		gfxPixel(x + width - 2, y + 1);
		gfxPixel(x + 1, y + height - 2);
		gfxPixel(x + width - 2, y + height - 2);
	}
	DrawButtonFace(x + 2, y + 2, x + width - 2, y + height - 2, pressed);

	if ( pressed )
		CenterLabel(x + width / 2 + 1, y + height / 2 + 1, text, gStdColor[fontColor], font);
	else
		CenterLabel(x + width / 2, y + height / 2, text, gStdColor[fontColor], font);
}


void TextButton::HandleEvent(GEVENT *event)
{
	Button::HandleEvent(event);
}


#define kCBSize 11
#define kCBLabelPad 3
Checkbox::Checkbox(int left, int top, int itemid, BOOL value, char* l) : Widget(left, top, 0, 0) {
	
	
	width 		= kCBSize + (strlen(l) * 8);
	height 		= kCBSize + (kCBLabelPad >> 1);
	checked 	= value;
	id 			= itemid;
	result		= 0;
	canFocus	= TRUE;
	disabled	= FALSE;
	
	strcpy(label, l);
	hotKey		= GetHotKey(label);
}

Checkbox::Checkbox(int left, int top, int itemid, BOOL value, char* l, int rslt) : Widget(left, top, 0, 0) {
	
	result		= mrOk;
	width 		= kCBSize + (strlen(l) * 8);
	height 		= kCBSize + (kCBLabelPad >> 1);
	checked 	= value;
	id 			= itemid;
	result		= rslt;
	canFocus	= TRUE;
	disabled	= FALSE;
	
	strcpy(label, l);
	hotKey		= GetHotKey(label);

}

void Checkbox::Paint(int x, int y, BOOL hasFocus) {

	gfxSetColor(gStdColor[8]);
	gfxHLine(y, x, x + kCBSize - 2);
	gfxVLine(x, y, y + kCBSize - 2);
	
	if (disabled)
		gfxSetColor(gStdColor[21]);
	else
		gfxSetColor(gStdColor[hasFocus ? 15 : 17]);
	
	gfxFillBox(x + 2, y + 2, x + kCBSize - 2, y + kCBSize - 2);
	
	gfxSetColor(gStdColor[0]);
	
	if (checked)
	{
		/// !!!
		gfxFillBox(x + 4, y + 4, x + kCBSize - 4, y + kCBSize - 4);		
		//Video_Line(0, x + 3, y + 3 , x + width - 4, y + height - 4);
		//Video_Line(0, x + width - 4, y + 3 , x + 3, y + height - 4);
	}
	
	gfxHLine(y + 1, x + 1, x + kCBSize - 1);
	gfxVLine(x + 1, y + 1, y + kCBSize - 1);

	gfxSetColor(gStdColor[18]);
	gfxHLine(y + kCBSize - 1, x + 1, x + kCBSize - 1);
	gfxVLine(x + kCBSize - 1, y + 1, y + kCBSize - 1);
	
	if (strlen(label))
	{
		x = x + kCBSize + kCBLabelPad;
		gfxDrawLabel(x, y + kCBSize / 4, gStdColor[0], label, pFont);
		
		if (hasFocus)
		{
			int len = gfxGetLabelLen(label, pFont);
			gfxSetColor(gStdColor[0]);
			for (int i = x; i < x + len + 2; i+=2)
				gfxPixel(i, y + pFont->height + 2);
		}
	}
	
}

void Checkbox::HandleEvent( GEVENT *event ) {
	
	if (disabled)
		return;
	
	if (event->type == evKeyDown && (event->key.ascii == ' ' || cmpHotKey(hotKey, event->key.make))) {

		pressed = TRUE;
		checked ^= 1;
		if (result)
			EndModal(result);
		event->Clear();
	
	} else if ((event->type & evMouse) && !event->mouse.button) {

		switch (event->type) {
			case evMouseDown:
				pressed = TRUE;
				break;
			case evMouseUp:
				checked ^= 1;
				if (result)
					EndModal(result);
				event->Clear();
				break;
		}
		
	}

	return;
	
}



void colorSelectSetIndex(Widget* widget)
{
	ColorButton* pButton = (ColorButton*)widget;
	ColorSelect* pColorSelect = (ColorSelect*)widget->owner;
	pColorSelect->value = pButton->bgcolor;
	
	Container* pCont = (Container*)widget->owner;
	for (Widget *w = pCont->head.next; w != &pCont->head; w = w->next)
	{
		ColorButton* pBut = (ColorButton*)w;
		if (pBut == pButton)
			continue;

		pBut->checked = 0;
	}
}

void ColorButton::Paint( int x, int y, BOOL)
{
	DrawRect(x, y, x + width, y + height, kColorShadow);
	
	if (disabled)
	{

		DrawRect(x+1, y+1, x + width - 1, y + height - 1, gStdColor[0]);
		
		gfxSetColor(gStdColor[4]);
		/// !!!
		gfxFillBox(x+3, y+3, x + width - 3, y + height - 3);
		//Video_Line(0, x + 3, y + 3 , x + width - 4, y + height - 4);
		//Video_Line(0, x + width - 4, y + 3 , x + 3, y + height - 4);
	}
	else
	{
		if (bgcolor >= 0)
		{
			gfxSetColor((char)bgcolor);
			gfxFillBox(x+1, y+1, x + width - 1, y + height - 1);
		}
		
		
		gfxSetColor(fade());
		if (checked) DrawRect(x, y, x + width, y + height, gColor);
		if (active)  gfxPixel(x + (width >> 1), y + (height >> 1));
	}
	
}

void ColorButton::HandleEvent(GEVENT *event)
{
	if (disabled) return;
	else if ((event->type & evMouse) && !event->mouse.button)
	{
		switch (event->type) {
			case evMouseDown:
				pressed = TRUE;
				event->Clear();
				break;
			case evMouseUp:
				if (Contains(event->mouse.x, event->mouse.y))
				{
					checked = 1;
					if (clickProc) clickProc(this);
					if (result) EndModal(result);
				}
				event->Clear();
				break;
		}
	}
	return;
	
}

ColorSelect::ColorSelect(int left, int top, int rows, int cols, short colSize, BYTE* colors, char flags) :
	ColorPick(left, top, rows, cols, colSize, colors)
{
	
	// TO-DO: sort color buttons by closest RGB
	flags = 0;
	value = -1;
	int x = left, y = top;
	int i = 0, r = rows, c = cols, sz = colSize;
	while (i < 256)
	{
		ColorButton* pColor = new ColorButton(x, y, sz, sz, i, colorSelectSetIndex);
		if (colors[i] <= 0) pColor->disabled = TRUE;
		else if (colors[i] == 2)
		{
			pColor->checked = TRUE;
			value = i;
		}
		else if (colors[i] == 3)
		{
			pColor->active = TRUE;
			
		}
		
		Container::Insert(pColor);

		i++;
		if (--r) x+=sz;
		else if (--c) y+=sz, x = left, r = rows;
		else break;
	}
	
	width = (x - left) + sz;
	height = (y - top) + sz;
}

BitButton2::BitButton2( int left, int top, int width, int height, QBITMAP* pBitmap, int result ) :
	Button(left, top, width, height, result), pBitmap(pBitmap)
{
	//hotKey = 49;
	canFocus = TRUE;
	
}

void BitButton2::Paint( int x, int y, BOOL hasFocus )
{
	
	gfxSetColor(gStdColor[0]);
	gfxHLine(y, x + 1, x + width - 2);
	gfxHLine(y + height - 1, x + 1, x + width - 2);
	gfxVLine(x, y + 1, y + height - 2);
	gfxVLine(x + width - 1, y + 1, y + height - 2);

	DrawButtonFace(x + 1, y + 1, x + width - 1, y + height - 1, pressed);
	
	if (hasFocus)
	{
		gfxSetColor(gStdColor[15]);
		gfxHLine(y, x + 1, x + width - 2);
		gfxHLine(y + height - 1, x + 1, x + width - 2);
		gfxVLine(x, y + 1, y + height - 2);
		gfxVLine(x + width - 1, y + 1, y + height - 2);
		gfxPixel(x + 1, y + 1);
		gfxPixel(x + width - 2, y + 1);
		gfxPixel(x + 1, y + height - 2);
		gfxPixel(x + width - 2, y + height - 2);
	}
	
	int cx = x + width / 2;
	int cy = y + height / 2;
	
	if ( pressed )
		gfxDrawBitmap(pBitmap, cx - pBitmap->width / 2 + 1, cy - pBitmap->height / 2 + 1);
	else
		gfxDrawBitmap(pBitmap, cx - pBitmap->width / 2, cy - pBitmap->height / 2);

}

void BitButton2::HandleEvent( GEVENT *event )
{
	Button::HandleEvent(event);
}

void BitButton::Paint( int x, int y, BOOL /* hasFocus */ )
{
	gfxSetColor(gStdColor[0]);
	gfxHLine(y, x + 1, x + width - 2);
	gfxHLine(y + height - 1, x + 1, x + width - 2);
	gfxVLine(x, y + 1, y + height - 2);
	gfxVLine(x + width - 1, y + 1, y + height - 2);

	DrawButtonFace(x + 1, y + 1, x + width - 1, y + height - 1, pressed);

	int cx = x + width / 2;
	int cy = y + height / 2;

	QBITMAP *pBitmap = (QBITMAP *)gGuiRes.Load(hBitmap);
	if ( pressed )
		gfxDrawBitmap(pBitmap, cx - pBitmap->width / 2 + 1, cy - pBitmap->height / 2 + 1);
	else
		gfxDrawBitmap(pBitmap, cx - pBitmap->width / 2, cy - pBitmap->height / 2);
}

TextArea::TextArea( int left, int top, int width, int height, char *s ) : EditText( left, top, width, height, "")
{
	canFocus = TRUE;
	text = NULL;
	FormatText(s);
	memset(placeholder, 0, sizeof(placeholder));
}

void TextArea::FormatText(char* s, int flags)
{
	int i, j, tlen = strlen(s);
	if (!text)
	{
		text = (char*)malloc(tlen + 1);
		memset(text, 0, tlen);
	}
	
	lines = 1;
	for (i = 0; i < tlen; i++)
	{
		switch(s[i]) {
			case '\t':
				text[i] = ' ';
				continue;
			case '\r':
				if (i+1 < tlen && s[i+1] == '\n')
				{
					for (j = i; j < tlen - 1; j++) text[j] = text[j+1];
					text[j] = 0;
					continue;
				}
				// fallthrough
			case '\n':
				text[i] = 0; lines++;
				continue;
			default:
				text[i] = s[i];
				continue;
		}
	}
	
	len  = pos = tlen;
	line = lines;
}

/* void TextArea::GetCursorPos(int* x, int* y)
{
	char* txt;
	int i = 0, j = 0, k = pos, line, tx, ty;
	
	while(i < len)
	{
		txt =& text[i];
		gfxDrawText(tx, ty, gStdColor[0], txt, pFont);
		ty+=pFont->height+1; i+=strlen(txt)+1;
	}
	
} */

int TextArea::GetLineStartByPos(int pos)
{
	int i = pos;
	do{ i--; } while(i >= 0 && i < len && text[i]);
	return ClipHigh(i + 1, len - 1);
}

int TextArea::GetLineByPos(int pos)
{
	int i = pos, l = 1;
	do
	{
		i--;
		if (text[i] == 0)
			l++;
	}
	while(i >= 0);
	return l;
}

void TextArea::Paint( int x, int y, BOOL hasFocus )
{
	scrDisplayMessage(kColorWhite);
	char* txt;
	int i, lst = 0, tx = x + 3, ty = y + 3, fh = pFont->height;
	
	DrawBevel(x, y, x + width - 1, y + height - 1, kColorShadow, kColorHighlight);
	DrawRect(x + 1, y + 1, x + width - 2, y + height - 2, gStdColor[0]);
	gfxSetColor(gStdColor[15]);
	gfxFillBox(x + 2, y + 2, x + width - 3, y + height - 3);

	if (text)
	{
		i = 0;
		while(i < len)
		{
			txt =& text[i];
			gfxDrawText(tx, ty, gStdColor[0], txt, pFont);
			ty+=pFont->height+1; i+=strlen(txt)+1;
		}
		
		if (hasFocus && IsBlinkOn())
		{
			gfxSetColor(gStdColor[0]);
			
			lst = GetLineStartByPos(pos);
			tx = x + gfxGetTextLen(&text[lst], pFont, pos-lst);
			ty = y + (line*pFont->height);
			gfxVLine(tx + 3, ty - (fh>>1), ty + (fh>>1));
		}
		
		sprintf(buffer, "%d / %d", pos, pos-lst);
		gfxDrawText(x + width - gfxGetTextLen(buffer, pFont), y + height - 10, 0, buffer, pFont);
		return;
	}
	else if (placeholder[0])
		gfxDrawText(x + 3, y + 3, gStdColor[25], placeholder, pFont);
	
	
}

void TextArea::HandleEvent( GEVENT *event )
{
	int lst = 0;
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
			case evMouseDrag:
				pos = gfxFindTextPos(string, pFont, event->mouse.x - left);
				SetBlinkOn();
				event->Clear();
				break;
		}
	}
	else if ( event->type == evKeyDown )
	{
		switch (event->key.make) {
			case KEY_BACKSPACE:
				if (pos > 0)
				{
					memmove(&string[pos - 1], &string[pos], len - pos);
					pos--;
					len--;
					string[len] = '\0';
				}
				event->Clear();
				break;
			case KEY_DELETE:
				if (pos < len)
				{
					len--;
					memmove(&string[pos], &string[pos + 1], len - pos);
					string[len] = '\0';
				}
				event->Clear();
				break;
			case KEY_LEFT:
				if (pos > 0)
				{
					if (line > 1)
					{
						lst = GetLineStartByPos(--pos);
						if (pos - lst <= 0)
						{
							lst = GetLineStartByPos(pos - 1);
							pos = lst + strlen(&text[lst]);
							line = GetLineByPos(pos);
						}
					}
					else
					{
						pos--;
					}
				}
				event->Clear();
				break;
			case KEY_RIGHT:
				if (pos < len)
				{
					pos++;
					if (line < lines)
					{
						lst = GetLineStartByPos(pos);
						if (pos >= lst + strlen(&text[lst]))
						{
							pos = lst;
							line = GetLineByPos(pos+2);
						}
					}
				}
				event->Clear();
				break;
			case KEY_HOME:
				pos = 0;
				event->Clear();
				break;
			case KEY_END:
				pos = len;
				event->Clear();
				break;
			default:
				if ( event->key.ascii != 0 )
				{
					if ( len < maxlen )
					{
						memmove(&string[pos+1], &string[pos], len - pos);
						string[pos++] = event->key.ascii;
						string[++len] = '\0';
					}
					event->Clear();
				}
				break;
		}
		SetBlinkOn();
	}
}

EditText::EditText( int left, int top, int width, int height, char *s, int flags) : Widget(left, top, width, height)
{
	canFocus = TRUE;
	if (s)
		strcpy(string, s);
	
	this->flags = flags;
	len = strlen(string), pos = len;
	maxlen = ClipHigh(width / pFont->width, sizeof(string));
	memset(placeholder, 0, sizeof(placeholder));
}


void EditText::Paint( int x, int y, BOOL hasFocus )
{
	DrawBevel(x, y, x + width - 1, y + height - 1, kColorShadow, kColorHighlight);
	DrawRect(x + 1, y + 1, x + width - 2, y + height - 2, gStdColor[0]);
	gfxSetColor(gStdColor[15]);
	gfxFillBox(x + 2, y + 2, x + width - 3, y + height - 3);

	if (string[0]) gfxDrawText(x + 3, y + height / 2 - 4, gStdColor[0], string, pFont);
	else if (placeholder[0])
		gfxDrawText(x + 3, y + height / 2 - 4, gStdColor[25], placeholder, pFont);
	
	if (hasFocus && IsBlinkOn())
	{
		gfxSetColor(gStdColor[0]);
		gfxVLine(x + gfxGetTextLen(string, pFont, pos) + 3, y + height / 2 - 4, y + height / 2 + 3);
	}
}


void EditText::HandleEvent( GEVENT *event )
{
	int gfxLen;
	
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
			case evMouseDrag:
				pos = gfxFindTextPos(string, pFont, event->mouse.x - left);
				SetBlinkOn();
				event->Clear();
				break;
		}
	}
	else if ( event->type == evKeyDown )
	{
		switch ( event->key.make )
		{
			case KEY_BACKSPACE:
				if (pos > 0)
				{
					memmove(&string[pos - 1], &string[pos], len - pos);
					pos--;
					len--;
					string[len] = '\0';
				}
				event->Clear();
				break;

			case KEY_DELETE:
				if (pos < len)
				{
					len--;
					memmove(&string[pos], &string[pos + 1], len - pos);
					string[len] = '\0';
				}
				event->Clear();
				break;

			case KEY_LEFT:
				if (pos > 0)
					pos--;
				event->Clear();
				break;

			case KEY_RIGHT:
				if (pos < len)
					pos++;
				event->Clear();
				break;

			case KEY_HOME:
				pos = 0;
				event->Clear();
				break;

			case KEY_END:
				pos = len;
				event->Clear();
				break;

			default:
				if (event->key.ascii != 0)
				{
					if (flags & 0x01)
					{
						gfxLen  = gfxGetTextLen(string, pFont);
						gfxLen += pFont->info[event->key.ascii].w;
						if (gfxLen >= width-4)
						{
							event->Clear();
							break;
						}
					}
					
					if (len < maxlen)
					{
						memmove(&string[pos+1], &string[pos], len - pos);
						string[pos++] = event->key.ascii;
						string[++len] = '\0';
					}
					
					event->Clear();
				}
				break;
		}
		SetBlinkOn();
	}
}



EditNumber::EditNumber( int left, int top, int width, int height, int n ) : EditText( left, top, width, height, "")
{
	value = n;
	itoa(n, string, 10);
	len = strlen(string);
	pos = len;
}


void EditNumber::HandleEvent( GEVENT *event )
{
	if ( event->type == evKeyDown )
	{
		switch ( event->key.make )
		{
			case KEY_BACKSPACE:
			case KEY_DELETE:
			case KEY_LEFT:
			case KEY_RIGHT:
			case KEY_HOME:
			case KEY_END:
				break;

			case KEY_MINUS:
				if (pos == 0 && string[0] != '-' && len < maxlen)
				{
					memmove(&string[1], &string[0], len);
					string[pos++] = '-';
					string[++len] = '\0';
				}
				event->Clear();
				break;

			default:
				if ( event->key.ascii != 0 )
				{
					if (event->key.ascii >= '0' && event->key.ascii <= '9' && len < maxlen)
					{
						memmove(&string[pos+1], &string[pos], len - pos);
						string[pos++] = event->key.ascii;
						string[++len] = '\0';
					}
					event->Clear();
				}
				break;
		}
	}
	EditText::HandleEvent( event );
	value = atoi(string);
}

#define kRepeatDelay	60
#define kRepeatInterval	6
#define kDoubleClick	60

GEVENT_TYPE GetEvent( GEVENT *event )
{
	static int clickTime[3], downTime[3];
	static BYTE oldbuttons = 0;
	BYTE newbuttons;
	BYTE key;

	memset(event, 0, sizeof(GEVENT));

	if ( (key = keyGet()) != 0 )
	{
		
		if ( key == KEY_ESC )
			keystatus[KEY_ESC] = 0;	// some of Ken's stuff still checks this!
		
		if (!gMouse.buttons) {

			if ( keystatus[KEY_LSHIFT] )
				event->key.lshift = 1;
			if ( keystatus[KEY_RSHIFT] )
				event->key.rshift = 1;
			event->key.shift = event->key.lshift | event->key.rshift;

			if ( keystatus[KEY_LCTRL] )
				event->key.lcontrol = 1;
			if ( keystatus[KEY_RCTRL] )
				event->key.rcontrol = 1;
			event->key.control = event->key.lcontrol | event->key.rcontrol;

			if ( keystatus[KEY_LALT] )
				event->key.lalt = 1;
			if ( keystatus[KEY_RALT] )
				event->key.ralt = 1;
			event->key.alt = event->key.lalt | event->key.ralt;

			if ( event->key.alt )
				event->key.ascii = 0;
			else if ( event->key.control )
				event->key.ascii = 0;
			else if ( event->key.shift )
				event->key.ascii = keyAsciiSH[key];
			else
				event->key.ascii = keyAscii[key];

			event->key.make = key;
			event->type = evKeyDown;
			return event->type;
			
		}
	}

	event->mouse.dx = gMouse.dX;
	event->mouse.dy = gMouse.dY;
	event->mouse.x = gMouse.X;
	event->mouse.y = gMouse.Y;

	// which buttons just got pressed?
	newbuttons = (BYTE)(~oldbuttons & gMouse.buttons);

	BYTE buttonMask = 1;
	for (int nButton = 0; nButton < 3; nButton++, buttonMask <<= 1)
	{
		event->mouse.button = nButton;

		if (newbuttons & buttonMask)
		{
			oldbuttons |= buttonMask;
			event->type = evMouseDown;

			// create double click event if in time interval
			if ( gFrameClock < clickTime[nButton] + kDoubleClick )
				event->mouse.doubleClick = TRUE;

			clickTime[nButton] = gFrameClock;
			downTime[nButton] = 0;

			return event->type;
		}
		else if (oldbuttons & buttonMask)
		{
			if (gMouse.buttons & buttonMask)
			{
				downTime[nButton] += gFrameTicks;

				if ( event->mouse.dx || event->mouse.dy )
				{
					event->type = evMouseDrag;
					return event->type;
				}

				if ( downTime[nButton] > kRepeatDelay )
				{
					downTime[nButton] -= kRepeatInterval;
					event->type = evMouseRepeat;
					return event->type;
				}
			}
			else
			{
				oldbuttons &= ~buttonMask;
				event->type = evMouseUp;
				return event->type;
			}
		}
	}
	return evNone;
}

void gfxDrawBitmap(int id, int x, int y) {
	
	gfxDrawBitmap(pBitmaps[id],x ,y);
	
}

char* fixFonts[] = {
	
	"ARIAL12",
	"CALIB16",
	
};

void LoadFontSymbols(ROMFONT* pFont)
{
	BYTE* data = pFont->data;
	int h = pFont->hg;
	int w = pFont->wh;
	
	unsigned char CheckBox[2][8] = {
		{ 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF },
		{ 0xFF, 0xC3, 0xA5, 0x99, 0x99, 0xA5, 0xC3, 0xFF }
	};
	unsigned char RadioButton[2][8] = {
		{ 0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C },
		{ 0x3C, 0x42, 0x99, 0xBD, 0xBD, 0x99, 0x42, 0x3C }
	};
	
	memset(&data[2*h], 0, (w*h)*4);
	memcpy(&data[2*h], CheckBox[0], 16);
	memcpy(&data[4*h], RadioButton[0], 16);
}

extern "C" unsigned char textfont[];
extern "C" unsigned char smalltextfont[];

void GUIInit() {

	RESHANDLE hRes = NULL;
	int i, j, k; char temp[6];
	
	for (i = 0, j = 0; i < LENGTH(qFonts); i++)
	{
		if ((hRes = gGuiRes.Lookup(i, "QFN")) != NULL)
		{
			qFonts[j] = (QFONT*)gGuiRes.Lock(hRes);
			for (k = 0; k < LENGTH(fixFonts); k++)
			{
				if (stricmp(hRes->name, fixFonts[k]) != 0) continue;
				qFonts[j]->baseline-=2;
				break;
			}
			
			j++;
		}
	}
	
	if (j == 0)
		ThrowError("Failed to load fonts!");
	
	pFont = qFonts[0]; // default GUI font
	
	// make all NULL fonts use default GUI font
	for (i = 0; i < LENGTH(qFonts); i++)
		if (!qFonts[i]) qFonts[i] = pFont;
	
	// we need this table for VCOLOR fonts
	for (i = LENGTH(QFNScaleTable) - 1; i >=0; i--)
	{
		if (ydim < QFNScaleTable[i][0]) continue;
		xscale = QFNScaleTable[i][1];
		yscale = QFNScaleTable[i][2];
		xstep  = QFNScaleTable[i][3];
		ystep  = QFNScaleTable[i][4];
		break;
	}
	
	// load various GUI images
	for (i = 0; i < LENGTH(pBitmaps); i++)
	{
		if ((hRes = gGuiRes.Lookup(i, "QBM")) != NULL)
			pBitmaps[i] = (QBITMAP*)gGuiRes.Lock(hRes);
	}
	
	// load vga rom fonts
	for (i = 0; i < LENGTH(vFonts); i++)
	{
		ROMFONT* pRFont =& vFonts[i];
		if (i <= 1)
		{
			pRFont->wh = 8;
			pRFont->hg = 8;
			pRFont->ls = (i == 0) ? 8 : 4;
			pRFont->data = (BYTE*)((i == 0) ? &textfont : &smalltextfont);
			LoadFontSymbols(pRFont);
		}
		else if ((hRes = gGuiRes.Lookup(i, "RFN")) != NULL)
		{
			strncpy(temp, hRes->name, 5);
			pRFont->wh = enumStrGetInt(0, temp, 'X'); // get width from name
			pRFont->hg = enumStrGetInt(1, NULL, 'X'); // get height from name
			if (pRFont->wh && pRFont->hg)
			{
				pRFont->data = (BYTE*)gGuiRes.Lock(hRes);
				LoadFontSymbols(pRFont);
				pRFont->ls = pRFont->wh;
			}
		}
	}
}

int ShowModal(Container *dialog, int flags)
{
	GEVENT event;
	gMouse.ResetSpeedNow();
	gMouse.PushRange();
	gMouse.SetRange(0, 0, xdim, ydim);
	
	Container desktop(0, 0, xdim, ydim);
	desktop.Insert(dialog);
	
	if (!(flags & kModalNoCenter))
	{
		// center the dialog
		dialog->left = (xdim - dialog->width) >> 1;
		dialog->top = (ydim - dialog->height) >> 1;
	}
	
	// find first item for focus
	while (!desktop.SetFocus(+1));

	int saveSize = bytesperline * ydim;
	BYTE *saveUnder = (BYTE *)Resource::Alloc(saveSize);


	// copy save under from last displayed page
#if USE_POLYMOST
	if (getrendermode() < 3)
#endif
	{
		begindrawing();
		memcpy(saveUnder, (void*)frameplace, saveSize);
		enddrawing();
	}

	dialog->isModal = TRUE;
	while (dialog->isModal)
	{
		updateClocks();
		UpdateBlinkClock(gFrameTicks);
		if (flags & kModalFadeScreen)
			drawHighlight(0, 0, xdim, ydim, gStdColor[0]);
		
		OSD_Draw();
		
		gMouse.Read(gFrameClock);
		handleevents();
		GetEvent(&event);

		// trap certain dialog keys
		if ( event.type == evKeyDown )
		{
			switch (event.key.ascii) {
				case 27:
					dialog->EndModal(mrCancel);
					continue;
				case 13:
					dialog->EndModal(mrOk);
					continue;
				case 9:
					if ( event.key.shift ) while (!desktop.SetFocus(-1));
					else while (!desktop.SetFocus(+1));
					continue;
			}
		}

		desktop.HandleEvent(&event);
		desktop.Paint(0, 0, FALSE);
		DrawMouseCursor(gMouse.X, gMouse.Y);
		showframe(); // this instead of scrNextPage() makes GUI to show in 2d mode

		// restore the save under after page flipping
#if USE_POLYMOST
		if (getrendermode() < 3)
#endif
		{
			begindrawing();
			memcpy((void*)frameplace, saveUnder, saveSize);
			enddrawing();
		}
	}
	
	gMouse.PopRange();
	Resource::Free(saveUnder);
	desktop.Remove(dialog);
	return dialog->endState;
}


int GetStringBox( char *title, char *s )
{
	// create the dialog
	Window dialog(0, 0, 168, 40, title);

	// this will automatically be destroyed when the dialog is destroyed
	EditText *el = new EditText(4, 4, 154, 16, s);
	el->maxlen += 5;
	dialog.Insert(el);

	ShowModal(&dialog);
	if ( dialog.endState != mrOk)
		return 0;		// pressed escape

	// copy the edited string
	strcpy(s, el->string);

	return strlen(s);
}


int GetNumberBox( char *title, int n, int nDefault )
{
	// create the dialog
	Window dialog(0, 0, 168, 40, title);

	// this will automatically be destroyed when the dialog is destroyed
	EditNumber *en = new EditNumber(4, 4, 154, 16, n);
	dialog.Insert(en);

	ShowModal(&dialog);
	if ( dialog.endState != mrOk)
		return nDefault;		// pressed escape

	return en->value;
}

#define kButtonWidth 60
#define kButtonHeight 20
#define kButtonPadding 8
#define kVertSpace 2
#define kDialogPad 4

int showButtons(NAMED_TYPE* names, int namesLen, char* title) {
	
	char upchar[128];
	int i = 0, k = 0, w1 = 0, result = -1, x = kDialogPad, y = kVertSpace;
	int rows = 1, width = 0, height = 0, maxHeight = ydim - 100, maxWidth = xdim - 100;
	
	
	if (namesLen <= 0)
		return mrCancel;

	int buth = kButtonHeight + kVertSpace;
	int butsPerRow = maxHeight / buth;
	
	rows = ClipLow(namesLen / butsPerRow, 1);
	while ((namesLen * buth) / rows > maxHeight)
		rows++;
	
	// determine height of the dialog window
	height = ((namesLen * buth) / rows) + buth;

	// get longest string to determine width of buttons and the dialog window
	sprintf(upchar, title);
	w1 = gfxGetTextLen(strupr(upchar), pFont);
	for (i = 0; i < namesLen; i++)
	{
		sprintf(upchar, names[i].name);
		if ((k = gfxGetTextLen(strupr(upchar), pFont)) > w1)
			w1 = k;
	}
	
	width = ((w1 * rows)) + kButtonPadding + kDialogPad + (kDialogPad >> 1);
	
	//Alert("%d / %d / %d / %d / %d", width, rows, butsPerRow, height, maxHeight);
	
	// create the dialog
	Window dialog(0, 0, width, height, title);
	
	// insert buttons
	for (i = 0; i < namesLen; i++)
	{
		
		if (y + buth >= height)
		{
			y = kVertSpace;
			x += w1;
		}
		
		if (x >= maxWidth)
			break;
			
		result                 = mrUser + names[i].id;
		TextButton *pbButton   = new TextButton(x, y, w1, kButtonHeight, names[i].name, result);
		if ((pbButton->hotKey = GetHotKey(names[i].name)) == '\0')
			pbButton->hotKey = (char)ClipHigh(49 + i, 57);
		
		
		dialog.Insert(pbButton);
		y += buth;
	}

	ShowModal(&dialog);
	if (dialog.endState == mrOk && dialog.focus)	// find a button we are focusing on
	{
		Container* pCont = (Container*)dialog.focus;
		TextButton* pFocus = (TextButton*)pCont->focus;
		if (pFocus)
			return pFocus->result;
	}
	return dialog.endState;
}

int showButtons(char** names, int namesLen, char* title) {
	
	int i;
	NAMED_TYPE* array = (NAMED_TYPE*)Resource::Alloc(sizeof(NAMED_TYPE)*namesLen); 
	for (i = 0; i < namesLen; i++)
	{
		array[i].name = names[i];
		array[i].id = i;
	}
	i = showButtons(array, namesLen, title);
	Resource::Free(array);
	return i;
}

#define kDialogPad 4
int createCheckboxList(CHECKBOX_LIST* array, int len, char* title, BOOL buttons) {
	
	//QFONT* pFont = qFonts[0];
	int dialogWidth = (buttons) ? 128 : 0, dialogHeight = 0;
	int i, j, x = kDialogPad, y = kDialogPad;
	
	if ((j = strlen(title)*pFont->width) > dialogWidth)
		dialogWidth = j;
	
	for (i = 0; i < len; i++)
	{
		if ((j = strlen(array[i].label)*pFont->width) > dialogWidth)
			dialogWidth = j;
	}
	
	dialogWidth +=(kCBSize + kCBLabelPad + kDialogPad);
	dialogHeight = ((kCBSize + kVertSpace)*len) + kDialogPad + 16;
	if (buttons)
		dialogHeight+=(kButtonHeight+(kDialogPad<<1));
	
	Window dialog(0, 0, dialogWidth, dialogHeight, title);
	
	if (buttons)
		dialog.Insert(new TextButton(x,  dialogHeight - (kButtonHeight<<1) - 2, 60, kButtonHeight, "&Ok", mrOk));
	
	// insert checkboxes
	for (i = 0; i < len; i++)
	{
		array[i].checkbox = new Checkbox(x,  y, 0, array[i].option, array[i].label, (buttons) ? 0 : mrUser + i);
		dialog.Insert(array[i].checkbox);
		y+=(kCBSize + kVertSpace);
	}

	if (ShowModal(&dialog) != mrCancel)
	{
		for (i = 0, j = 0; i < len; i++)
			array[i].option = array[i].checkbox->checked;
	}
	
	return dialog.endState;
	
}

int createCheckboxList(CHECKBOX_LIST_P* array, int len, char* title, BOOL buttons) {
	
	int i;
	CHECKBOX_LIST* list = (CHECKBOX_LIST*)Resource::Alloc(sizeof(CHECKBOX_LIST)*len);
	for (i = 0; i < len; i++)
	{
		list[i].option = *array[i].option;
		sprintf(list[i].label, array[i].label);
	}
	
	i = createCheckboxList(list, len, title, buttons);
	for (i = 0; i < len; i++)
		*array[i].option = list[i].option;

	Resource::Free(list);
	list = NULL;
	return i;
}



int YesNoCancel(char *__format, ...) {
	
	
	va_list argptr;
	va_start(argptr, __format);
	vsprintf(modalmsg, __format, argptr);
	va_end(argptr);
	
	// create the dialog
	int len = ClipRange(gfxGetTextLen(modalmsg, pFont) + 10, 202, xdim - 5);
	Window dialog(59, 80, len, 46, modalmsg);

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrOk));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
 	dialog.Insert(new TextButton(132, 4, 60, 20, "&Cancel", mrCancel));
	
	ShowModal(&dialog);
	if (dialog.endState == mrOk && dialog.focus)	// find a button we are focusing on
	{
		Container* pCont = (Container*)dialog.focus;
		TextButton* pFocus = (TextButton*)pCont->focus;
		if (pFocus)
			return pFocus->result;
	}
	return dialog.endState;

}

void Alert(char *__format, ...) {
	
	va_list argptr;
	va_start(argptr, __format);
	vsprintf(modalmsg, __format, argptr);
	va_end(argptr);
	
	// create the dialog
	int len = ClipRange(gfxGetTextLen(modalmsg, pFont) + 10, 202, xdim - 5);
	Window dialog(59, 80, len, 46, modalmsg);

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Ok", mrOk));
	ShowModal(&dialog);
	return;

}

BOOL Confirm(char *__format, ...) {
	
	va_list argptr;
	va_start(argptr, __format);
	vsprintf(modalmsg, __format, argptr);
	va_end(argptr);
	
	// create the dialog
	int len = ClipRange(gfxGetTextLen(modalmsg, pFont) + 10, 202, xdim - 5);
	Window dialog(59, 80, len, 46, modalmsg);

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrOk));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
	
	ShowModal(&dialog);
	if (dialog.endState == mrOk && dialog.focus)	// find a button we are focusing on
	{
		Container* pCont = (Container*)dialog.focus;
		TextButton* pFocus = (TextButton*)pCont->focus;
		if (pFocus)
			return (pFocus->result == mrOk);
	}
	
	return (dialog.endState == mrOk);
}

char fade(int rate) {
	return gStdColor[23 + mulscale30(8, Sin(gFrameClock * kAng360 / rate))];
}

/*

void ThumbButton::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDrag:
				top = ClipRange(event->mouse.y - height / 2, kSBHeight, owner->height - kSBHeight - height);
				break;

			case evMouseDown:
				pressed = TRUE;
				break;

			case evMouseUp:
				pressed = FALSE;
				break;
		}
	}
}


void ScrollButton::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
				pressed = TRUE;
				if (clickProc != NULL)
					clickProc(this);
				break;

			case evMouseDrag:
				pressed = Contains(event->mouse.x, event->mouse.y);
				break;

			case evMouseRepeat:
				if ( pressed && clickProc != NULL )
					clickProc(this);
				break;

			case evMouseUp:
				pressed = FALSE;
				break;
		}
	}
}


void ScrollLineUp( Widget *widget )
{
	ScrollBar *pScrollBar = (ScrollBar *)widget->owner;
	pScrollBar->ScrollRelative(-1);
}


void ScrollLineDown( Widget *widget )
{
	ScrollBar *pScrollBar = (ScrollBar *)widget->owner;
	pScrollBar->ScrollRelative(+1);
}


ScrollBar::ScrollBar( int left, int top, int height, int min, int max, int value ) :
	Container(left, top, kSBWidth + 2, height), min(min), max(max), value(value)
{
	pbUp = new ScrollButton(1, 1, gGuiRes.Lookup(kBitmapScrollUp, "QBM"), ScrollLineUp);
	pbDown = new ScrollButton(1, height - kSBHeight - 1, gGuiRes.Lookup(kBitmapScrollDn, "QBM"), ScrollLineDown);
//	pcThumbBar = new Container(1,
	pbThumb = new ThumbButton(1, 10, 20);
	Insert(pbUp);
	Insert(pbDown);
	Insert(pbThumb);
	size = 0;
}

void ScrollBar::ScrollRelative( int offset )
{
	pbThumb->top = ClipRange(pbThumb->top + offset, kSBHeight, height - kSBHeight - pbThumb->height);
}



void ScrollBar::Paint( int x, int y, BOOL hasFocus )
{
	DrawBevel(x, y, x + width, y + height, kColorShadow, kColorHighlight);
	DrawRect(x + 1, y + 1, x + width - 1, y + height - 1, gStdColor[0]);
	gfxSetColor(kColorShadow);
	gfxFillBox(x + 2, y + 2, x + width - 2, y + height - 2);
	Container::Paint(x, y, hasFocus);
}



Radio::Radio(int left, int top, int group, int itemid, BOOL value, char* l) : Widget(left, top, 0, 0) {
	
	width 		= kCBSize + (strlen(l) * 8);
	height 		= kCBSize + (kCBLabelPad >> 1);
	checked 	= value;
	id 			= itemid;
	strcpy(label, l);

}

void Radio::Paint(int x, int y, BOOL) {

	gfxSetColor(gStdColor[8]);
	gfxHLine(y, x, x + kCBSize - 2);
	gfxVLine(x, y, y + kCBSize - 2);
	
	gfxSetColor(gStdColor[15]);
	gfxFillBox(x + 2, y + 2, x + kCBSize - 2, y + kCBSize - 2);
	
	if (checked) {
		gfxSetColor(gStdColor[0]);
		gfxFillBox(x + 4, y + 4, x + kCBSize - 4, y + kCBSize - 4);
	}
	
	gfxSetColor(gStdColor[0]);
	gfxHLine(y + 1, x + 1, x + kCBSize - 1);
	gfxVLine(x + 1, y + 1, y + kCBSize - 1);

	gfxSetColor(gStdColor[18]);
	gfxHLine(y + kCBSize - 1, x + 1, x + kCBSize - 1);
	gfxVLine(x + kCBSize - 1, y + 1, y + kCBSize - 1);
	
	if (height)
	gfxDrawLabel(x + kCBSize + kCBLabelPad, y + kCBSize / 4, gStdColor[0], label, pFont);

}

void Radio::HandleEvent( GEVENT *event ) {
	
	if (event->type & evMouse) {
		if (event->mouse.button != 0)
			return;
		
		switch (event->type) {
			case evMouseDown:
				pressed = TRUE;
				checked ^= 1;
 				Widget* pNext = NULL;
				while ( 1 ) {
					
					Checkbox* widget = (Checkbox *)next;
					pNext = widget->next;
					if (pNext == NULL) break;
					if (widget->checked) {
						widget->checked = FALSE;
					}
					
					
					
				} 
				event->Clear();
				break;
		}
		
	}
	return;
	
} */
