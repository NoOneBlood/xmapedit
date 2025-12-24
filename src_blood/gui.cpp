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
#include "xmpmaped.h"
#include "screen.h"
#include "tile.h"

#define kBlinkTicks     100
#define kBlinkOnTicks   80
#define kRepeatDelay    32
#define kRepeatInterval 6
#define kDoubleClick    32


static int clickTime[3], downTime[3];
static BYTE oldbuttons = 0;
static int blinkClock = 0;
Container* pRoot = NULL;
Resource gGuiRes;
QFONT* pFont;

NAMED_TYPE gStdButtons[] = {

    {mrOk,      "&Ok"},
    {mrYes,     "&Yes"},
    {mrNo,      "&No"},
    {mrCancel,  "&Cancel"},
};

BOOL cmpHotKey(char keyA, char keyB) {

    return (keyA && keyAsciiSH[keyB] == keyA);

}

Widget* GetRoot(Widget* w)
{
    Widget* pW = w;
    while( 1 )
    {
        Widget* pOwn = pW->owner;
        if (pOwn == NULL)
            break;

        pW = pOwn;
    }


    return pW;
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
    gfxHLine(y0, x0, x1 - 1);
    gfxVLine(x0, y0 + 1, y1 - 1);
    gfxSetColor(color2);
    gfxHLine(y1 - 1, x0 + 1, x1 - 1);
    gfxVLine(x1 - 1, y0 + 1, y1 - 1);
}


void DrawRect( int x0, int y0, int x1, int y1, int color )
{
    gfxSetColor(color);
    gfxHLine(y0, x0, x1 - 1);
    gfxHLine(y1 - 1, x0, x1 - 1);
    gfxVLine(x0, y0 + 1, y1 - 1);
    gfxVLine(x1 - 1, y0 + 1, y1 - 1);
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
        gfxVLine(x0 + 1, y0 + 1, y1 - 1);
        DrawBevel(x0 + 2, y0 + 2, x1, y1, gStdColor[19], gStdColor[22]);
    }
    else
    {
        DrawBevel(x0, y0, x1, y1, gStdColor[16], gStdColor[24]);
        DrawBevel(x0 + 1, y0 + 1, x1 - 1, y1 - 1, gStdColor[18], gStdColor[22]);
    }
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

    if (tileLoadTile(tilenum))
        tileDrawGetSize(tilenum, scaleXArg, &width, &height);
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
    font        = pFont;
    canFocus    = FALSE;
    bgColor     = bgColorA;
    fontColor   = fontColorA;
    lines       = 0;
    flags       = flagsA;

    if ((textlen = strlen(s)) > 0)
    {
        lines++;
        text = (char*)Resource::Alloc(textlen + 1);
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
            fntcol = mostUsedByte((BYTE*)font->data, j, 255);
    }

    if (bgColor >= 0)
    {
        gfxSetColor(gStdColor[bgColor]);
        gfxFillBox(x, y, x + width, y + height);
    }

    if ((flags & kTextBlink) && !(totalclock & 64))
        return;

    if (flags & kTextUnderline) ystp++;
    if (flags & kTextOverline)  ystp++;
    if (flags & kTextBold)      bold = true;

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
    if (klabs(dir) == 2 && focus->canFocus)
        return TRUE;

    do
    {
        if (focus->isContainer)
        {
            if (((Container *)focus)->SetFocus(dir))
                return TRUE;
        }

        if (dir > 0) focus = focus->next;
        else focus = focus->prev;

        if (focus == &head)
            return FALSE;
    }
    while (!focus->canFocus);
    return TRUE;
}

void Container::ClearFocus()
{
    for ( Widget *w = head.next; w != &head; w = w->next)
    {
        focus = &head;
        if (w->isContainer)
            ((Container*)w)->ClearFocus();
    }
}

BOOL Container::SetFocusOn(Widget* pFocus)
{
    ClearFocus();

    do
    {
        if (focus == pFocus)
        {
            focus = pFocus;
            return TRUE;
        }

        if (focus->isContainer)
        {
            if (((Container *)focus)->SetFocusOn(pFocus))
                return TRUE;
        }

        focus = focus->next;
        if (focus == &head)
            return FALSE;
    }
    while ( 1 );
    return TRUE;



    /*for ( Widget *w = head.next; w != &head; w = w->next)
    {
        if (w == pFocus)
        {
            focus = pFocus;
            return TRUE;
        }

        if (w->isContainer)
            ((Container*)w)->SetFocusOn(pFocus);
    }

    return FALSE;*/
}


void Container::Insert( Widget *widget )
{
    dassert(widget != NULL);
    widget->prev = head.prev;
    widget->next = &head;
    widget->prev->next = widget;
    widget->next->prev = widget;
    widget->owner = this;
    this->drag = NULL;
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
    if (disabled)
    {
        if (event->type > 0)
            event->Clear();

        return;
    }

    if ( event->type & evMouse )
    {
        // make event relative to this container
        event->mouse.x -= left;
        event->mouse.y -= top;

        // find child owning location
        if (event->type == evMouseDown || drag == NULL)
        {

            if (event->type == evMouseDown)
                pRoot->ClearFocus();

            drag = NULL;
            // iterate in reverse order since widgets on top have priority
            for ( Widget *w = head.prev; w != &head; w = w->prev)
            {
                if (!w->disabled && w->Contains(event->mouse.x, event->mouse.y))
                {
                    drag = w;
                    if (event->type == evMouseDown && drag->canFocus)
                    {
                        while(focus != w)
                            pRoot->SetFocus(1);

                        break;
                    }
                }
            }
        }

        if (drag && (event->type == evMouseDrag || drag->Contains(event->mouse.x, event->mouse.y)))
        {
            drag->HandleEvent(event);
        }
    }
    else if ( event->type == evKeyDown )
    {
        if ( event->key.alt && keyAsciiSH[event->key.make] )
        {
            for ( Widget *w = head.prev; w != &head; w = w->prev)
            {
                if (!w->disabled && w->hotKey && keyAsciiSH[event->key.make] == w->hotKey && w->canFocus)
                {
                    while(focus != w)
                        pRoot->SetFocus(1);

                    focus->HandleEvent(event);
                    return;
                }
            }
        }

        if (!focus->disabled)
            focus->HandleEvent(event);

        // if key event not handled by focus, then broadcast to all childen with matching hot key
        if ( event->type != evNone )
        {
            for ( Widget *w = head.prev; w != &head; w = w->prev)
            {
                // set focus to the widget with the matching hot key
                if (!w->disabled && w->hotKey && keyAsciiSH[event->key.make] == w->hotKey && w->canFocus )
                {
                    while(focus != w)
                        pRoot->SetFocus(1);

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

    char fcol = gStdColor[fontColor];
    char bcol = gStdColor[borderColor];
    int len = 0, pad = 4, w1 = width, w2 = ClipLow(5*w1/100, pad), h = height;
    int fh = pFont->height;

    if ((len = gfxGetTextLen(title, pFont)) > 0)
        y+=(fh>>1);

    gfxDrawText(x+w2+pad, y-(fh>>1), fcol, title, pFont);
    gfxSetColor(bcol);

    gfxVLine(x, y, y+h);
    gfxVLine(x+w1, y, y+h);
    gfxHLine(y+h, x, x+w1);

    if (len > 0)
    {
        gfxHLine(y, x, x+w2);
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

TextButton::TextButton( int left, int top, int width, int height, char *text, CLICK_PROC clickProc ) :
    Button(left, top, width, height, clickProc), text(text)
{
    font = pFont;
    fontColor = 0;
    hotKey = GetHotKey(text);
    canFocus = TRUE;
}

void TextButton::Paint( int x, int y, BOOL hasFocus )
{
    gfxSetColor(gStdColor[0]);
    gfxHLine(y + 1, x + 2, x + width - 2);
    gfxHLine(y + height - 2, x + 2, x + width - 2);
    gfxVLine(x + 1, y + 2, y + height - 3);
    gfxVLine(x + width - 2, y + 2, y + height - 2);

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
Checkbox::Checkbox(int left, int top, BOOL value, char* l) : Widget(left, top, 0, 0) {


    width       = kCBSize;
    height      = kCBSize + (kCBLabelPad >> 1);
    checked     = value;
    result      = 0;
    canFocus    = TRUE;
    pressed     = FALSE;

    if (l)
    {
        if (strcpy(label, l) > 0)
            width+=(kCBLabelPad+gfxGetLabelLen(l, pFont));
    }
    hotKey      = GetHotKey(label);
}

Checkbox::Checkbox(int left, int top, BOOL value, char* l, int rslt) : Widget(left, top, 0, 0) {

    result      = mrOk;
    width       = kCBSize;
    height      = kCBSize + (kCBLabelPad >> 1);
    checked     = value;
    result      = rslt;
    canFocus    = TRUE;
    pressed     = FALSE;

    if (l)
    {
        if (strcpy(label, l) > 0)
            width+=(kCBLabelPad+gfxGetLabelLen(l, pFont));
    }

    hotKey      = GetHotKey(label);

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
        const int pd = 4;
        gfxLine(x + pd, y + pd, x + kCBSize - pd, y + kCBSize - pd);
        gfxLine(x + pd, y + kCBSize - pd , x + kCBSize - pd, y + pd);
    }

    gfxHLine(y + 1, x + 1, x + kCBSize - 1);
    gfxVLine(x + 1, y + 1, y + kCBSize - 1);

    gfxSetColor(gStdColor[18]);
    gfxHLine(y + kCBSize - 1, x + 1, x + kCBSize - 1);
    gfxVLine(x + kCBSize - 1, y + 1, y + kCBSize - 1);

    if (label[0])
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
    if ((event->type & evMouse) && !event->mouse.button)
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


char perc2clr(int nPerc)
{
    char nColor;
    //if (nPerc >= 90)      nColor = kColorLightGreen;
    if (nPerc >= 80)        nColor = kColorGreen;
    //else if (nPerc >= 70) nColor = kColorLightBlue;
    else if (nPerc >= 60)   nColor = kColorBlue;
    //else if (nPerc >= 50) nColor = kColorLightCyan;
    else if (nPerc >= 40)   nColor = kColorCyan;
    //else if (nPerc >= 30) nColor = kColorLightRed;
    else if (nPerc >= 20)   nColor = kColorRed;
    else if (nPerc >= 10)   nColor = 28;
    else                    nColor = kColorBlack;

    return nColor;
}

PluPick::PluPick(int left, int top, int width, int height, PLUPICK_PREFS* pArg) : Widget(left, top, width, height)
{
    this->pPrefs    = pArg;

    this->canFocus  = TRUE;
    this->nCursor   = 0;
    this->nStart    = 0;
    this->nCols     = pPrefs->nCols;
    this->nRows     = pPrefs->nRows;
    this->value     = pPrefs->nPlu;

    this->colWh = width / pPrefs->nCols;
    this->colHg = height / pPrefs->nRows;

    SetCursor(this->value);
}

void PluPick::Paint( int x, int y, BOOL hasFocus )
{
    #define kTileSize           88
    #define kPad1               4

    char nColor;
    int colWhRem,   colHgRem;
    int i, j, k, t;
    int curPlu, tx, ty;
    int tsz, twh, thg;


    int wh, hg;
    int dx  = x;
    int dy  = y;
    wh = width;
    hg = height;

    tsz = perc2val((colWh < colHg) ? colWh : colHg, kTileSize);
    tileDrawGetSize(pPrefs->nTile, tsz, &twh, &thg);

    gfxSetColor(clr2std(26));
    gfxFillBox(dx, dy, dx+wh, dy+hg);

    colHgRem = hg % nRows;
    for (i = 0, k = 0; i < nRows; i++)
    {
        dx = x;
        gfxSetColor(clr2std(kColorBlack));
        gfxHLine(dy, dx, dx+wh-1);

        colWhRem = wh % nCols;
        for (j = 0; j < nCols; j++, k++)
        {
            gfxSetColor(clr2std(kColorBlack));
            gfxVLine(dx, dy, dy + colHg);

            if ((curPlu = nStart + k) < pPrefs->pluCnt)
            {
                int cs;
                PLUINFO* pPlu = &pPrefs->pluInfo[curPlu];
                QFONT* pFont = qFonts[1];

                sprintf(buffer, "ID %d", pPlu->id);
                tx = dx + ((colWh>>1)-(twh>>1));    ty = dy + ((colHg>>1)-(thg>>1));
                tileDrawTile(tx, ty, pPrefs->nTile, tsz, pPlu->id, 0x02, pPrefs->nShade);

                tx = dx + perc2val(6, colWh);   ty = dy + perc2val(6, colHg);
                gfxPrinTextShadow(tx, ty, clr2std((pPlu->id == pPrefs->nPlu) ? kColorLightMagenta : kColorYellow), buffer, pFont);

                switch (pPlu->id) {
                    case kPlu0:
                        sprintf(buffer, "normal");
                        nColor = kColorMagenta;
                        break;
                    case kPlu1:
                        sprintf(buffer, "fog");
                        nColor = kColorLightGray;
                        break;
                    default:
                        sprintf(buffer, "%d%% alt", pPlu->efficency);
                        nColor = perc2clr(pPlu->efficency);
                        break;
                }

                pFont = qFonts[3];
                cs = pFont->charSpace;
                pFont->charSpace = 3;

                if ((t = gfxGetTextLen(strupr(buffer), pFont)) < colWh)
                {
                    gfxSetColor(clr2std(nColor));
                    int x1 = dx + ((colWh>>1)-(tsz>>1)),                    x2 = x1 + tsz;
                    int y1 = dy + colHg - pFont->height - (kPad1<<1),       y2 = y1 + pFont->height + kPad1;

                    gfxFillBox(x1, y1, x2, y2);
                    tx = x1 + ((x2-x1)>>1)-(t>>1);       ty = y1 + ((y2-y1)>>1) - (pFont->height>>1);
                    gfxPrinTextShadow(tx, ty, clr2std(kColorYellow), buffer, pFont);
                }

                pFont->charSpace = cs;

                if (curPlu == nCursor)
                    DrawRect(dx+1, dy+1, dx+colWh-1, dy+colHg-1, (hasFocus) ? fade() : clr2std(22));
            }

            dx+=colWh;
            if (--colWhRem >= 0)
                dx++;
        }

        dy+=colHg;
        if (--colHgRem >= 0)
            dy++;
    }

    DrawRect(x, y, x+wh, y+hg, clr2std(kColorBlack));
}

void PluPick::HandleEvent( GEVENT *event )
{
    Container* pCont = (Container*)owner;
    Widget* pFocus = pCont->focus;
    if (pFocus != this)
        return;


    if (event->type & evMouse)
    {
        if ((event->type == evMouseMove) || (event->type == evMouseUp && event->mouse.button == 0))
        {
            if (event->mouse.wheel > 0)
                nStart = ClipHigh(nStart + nCols, pPrefs->pluCnt-1);
            else if (event->mouse.wheel < 0)
                nStart = ClipLow(nStart - nCols, 0);

            int t = ClipLow(nCols*nRows - 1, 0);
            int nCol = ClipRange(event->mouse.x/colWh, 0, nCols-1);
            int nRow = ClipRange(event->mouse.y/colHg, 0, nRows-1);
            nCursor = ClipRange((nRow*nCols) + nStart + nCol, nStart, pPrefs->pluCnt);
            if (event->type == evMouseUp && nCursor < pPrefs->pluCnt)
            {
                value = pPrefs->pluInfo[nCursor].id + mrUser;
                EndModal(value);
            }
        }

        event->Clear();
    }
    else if (event->type == evKeyDown)
    {
        BYTE key = event->key.make;

        switch (key) {
            case KEY_UP:
                if (nCursor - nCols >= 0) nCursor -= nCols;
                event->Clear();
                break;
            case KEY_DOWN:
                if (nCursor + nCols < pPrefs->pluCnt) nCursor += nCols;
                event->Clear();
                break;
            case KEY_LEFT:
                if (nCols < 1) break;
                else if (nCursor - 1 >= 0) nCursor--;
                event->Clear();
                break;
            case KEY_RIGHT:
                if (nCols < 1) break;
                else if (nCursor + 1 < pPrefs->pluCnt) nCursor++;
                event->Clear();
                break;
        }

        ClipStart();

    }
}

void PluPick::SetCursor(int nPlu)
{
    int i = pPrefs->pluCnt;
    while(--i >= 0)
    {
        if (pPrefs->pluInfo[i].id == nPlu)
        {
            nCursor = i;
            break;
        }
    }

    ClipStart();
}

void PluPick::ClipStart()
{
    if (nCursor < 0)
        nCursor = ClipLow(nCols - abs(nCursor), 0);

    while (nCursor < nStart) nStart -= nCols;
    while (nStart + nRows * nCols <= nCursor)
        nStart += nCols;
}

EditText::EditText( int left, int top, int width, int height, char *s, int flags) : Widget(left, top, width, height)
{
    canFocus = TRUE;
    memset(string, 0, sizeof(string));
    memset(placeholder, 0, sizeof(placeholder));

    if (!isempty(s))
        strcpy(string, s);

    this->flags = flags;
    len = strlen(string), pos = len;
    maxlen = ClipHigh(width / pFont->width, sizeof(string));
}


void EditText::Paint( int x, int y, BOOL hasFocus )
{
    char bc = gStdColor[(disabled) ? 20 : (hasFocus ? 15 : 16)];
    char fc = gStdColor[(disabled) ? 26 : (hasFocus ? 0 : 30)];

    DrawBevel(x, y, x + width - 1, y + height - 1, kColorShadow, kColorHighlight);
    DrawRect(x + 1, y + 1, x + width - 2, y + height - 2, gStdColor[hasFocus ? 0 : 26]);

    gfxSetColor(bc);
    gfxFillBox(x + 2, y + 2, x + width - 3, y + height - 3);

    if (string[0]) gfxDrawText(x + 3, y + height / 2 - 4, fc, string, pFont);
    else if (placeholder[0])
        gfxDrawText(x + 3, y + height / 2 - 4, gStdColor[25], placeholder, pFont);

    if (hasFocus && IsBlinkOn())
    {
        gfxSetColor(gStdColor[0]);
        int nPos = (pos) ? gfxGetTextLen(string, pFont, pos) : 0;
        gfxVLine(x + nPos + 4, y + height / 2 - 4, y + height / 2 + 3);
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
                string[len] = '\0';
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

            case KEY_DOWN:
            case KEY_LEFT:
                if (pos > 0)
                    pos--;
                event->Clear();
                break;
            case KEY_UP:
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



EditNumber::EditNumber( int left, int top, int width, int height, int nVal, char _endChar, int nMin, int nMax) : EditText( left, top, width, height, "")
{
    value = nVal;
    endChar = _endChar;
    minValue = nMin;
    maxValue = nMax;
    ClampValue();
    pos = len;
}


void EditNumber::HandleEvent( GEVENT *event )
{
    if (event->type & evMouse)
    {
        if (event->mouse.button != 0)
            return;

        switch (event->type)
        {
            case evMouseDown:
            case evMouseDrag:
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
            case KEY_DELETE:
            case KEY_LEFT:
            case KEY_RIGHT:
            case KEY_HOME:
            case KEY_END:
                break;

            case KEY_MINUS:
                if (pos == 0 && string[0] != '-' && len < maxlen)
                {
                    if (minValue < 0)
                    {
                        memmove(&string[1], &string[0], len);
                        string[pos++] = '-';
                        string[++len] = '\0';

                        //if (!value)
                            //string[pos++] = '1', value = -1;
                    }
                }
                event->Clear();
                break;

            default:
                if (event->key.ascii != 0)
                {
                    if (event->key.ascii >= '0' && event->key.ascii <= '9' && len < maxlen)
                    {
                        if (value < maxValue)
                        {
                            if (pos <= 0 && value <= minValue && minValue != -99999999
                                && string[pos] != event->key.ascii)
                            {
                                    string[pos] = event->key.ascii;
                                    pos++;
                            }
                            else
                            {
                                memmove(&string[pos+1], &string[pos], len - pos);
                                string[pos++] = event->key.ascii;
                                string[++len] = '\0';
                            }
                        }
                    }

                    event->Clear();
                }
                break;
        }
    }

    EditText::HandleEvent(event);
    if (string[0])
    {
        value = atoi(string);
        ClampValue();
    }
    else
    {
        value = 0;
    }

    if (endChar != '\0')
        InsEndChar();
}

void EditNumber::ClampValue()
{
    value = ClipRange(value, minValue, maxValue);
    for (char* p = string; *p != '\0'; p++)
    {
        if (*p == '-' || *p == '+')
        {
            if (!isdigit(*(p+1)))
                return;

            continue;
        }

        if (!isdigit(*p))
            return;
    }

    itoa(value, string, 10); len = strlen(string);
    pos = ClipRange(pos, 0, len);
    if (endChar != '\0')
        InsEndChar();
}

void EditNumber::InsEndChar()
{
    sprintf(&string[len], "%c", endChar);
}


GEVENT_TYPE GetEvent( GEVENT *event )
{
    BYTE newbuttons;
    BYTE key;

    memset(event, 0, sizeof(GEVENT));

    if ( (key = keyGet()) != 0 )
    {
        if ( key == KEY_ESC )
            keystatus[KEY_ESC] = 0; // some of Ken's stuff still checks this!

        if (!gMouse.buttons)
        {
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

    event->mouse.dx     = gMouse.dX1;
    event->mouse.dy     = gMouse.dY1;
    event->mouse.x      = gMouse.X;
    event->mouse.y      = gMouse.Y;
    event->mouse.wheel  = gMouse.wheel;

    if (gMouse.Moved())
        event->type = evMouseMove;

    if (event->mouse.wheel)
        event->type = evMouseMove;

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

    return event->type;
}

void fontupr(ROMFONT* pFont)
{
    char r[2][2] = {{65, 97}, {192, 224}}, l, u;
    int s = pFont->size, h = pFont->hg;
    int i, n = h*25;

    if (pFont->data)
    {
        for (i = 0; i < LENGTH(r); i++)
        {
            l = r[i][1]; u = r[i][0];
            if ((l*h+n) >= s || (u*h+n) >= s)
                break;

            memcpy(&pFont->data[l*h], &pFont->data[u*h], n);
        }
    }
}

void GUIInit() {

    RESHANDLE hRes = NULL;
    int i, j, k; char temp[6];

    for (i = 0, j = 0; i < LENGTH(qFonts); i++)
    {
        if ((hRes = gGuiRes.Lookup(i, "QFN")) != NULL)
        {
            qFonts[j] = (QFONT*)gGuiRes.Lock(hRes);

            // show non-existing chars as dots
            const QFONTCHAR* pCharReplace = &qFonts[j]->info['.'];
            for (k = 33; k < 256; k++)
            {
                QFONTCHAR* pChar = &qFonts[j]->info[k];
                if (!pChar->w || !pChar->h)
                    memcpy(pChar, pCharReplace, sizeof(QFONTCHAR));
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
    
    // !!! fix baseline of 2d captions 
    if (qFonts[7] != pFont)
        qFonts[7]->baseline = 5;
    
    // load various GUI images
    for (i = 0; i < LENGTH(pBitmaps); i++)
    {
        if ((hRes = gGuiRes.Lookup(i, "QBM")) != NULL)
            pBitmaps[i] = (QBITMAP*)gGuiRes.Lock(hRes);
    }

    // load vga rom fonts
    if ((hRes = gGuiRes.Lookup(5, "INI")) != NULL)
    {
        char key[256], val[256], *pKey, *pVal;
        int nPrev = -1, nVal, i, o;

        IniFile* pIni = new IniFile((BYTE*)gGuiRes.Load(hRes), gGuiRes.Size(hRes));
        while(pIni->GetNextString(&pKey, &pVal, &nPrev, "FontInfo"))
        {
            if (!isufix(pKey) || !rngok(i = atoi(pKey), 0, LENGTH(vFonts)))
                continue;

            if ((hRes = gGuiRes.Lookup(i, "RFN")) != NULL)
            {
                o = 0;
                ROMFONT* pRFont =& vFonts[i];
                while((o = enumStr(o, pVal, val)) > 0)
                {
                    if (isufix(val) && irngok(nVal = atoi(val), 4, 32))
                    {
                        if (o == 1)         pRFont->wh = nVal;
                        else if (o == 2)    pRFont->hg = nVal;
                        else if (o == 3)    pRFont->ls = nVal;
                        else if (o == 4)    pRFont->lh = nVal;
                        else break;
                    }
                    else if (isbool(val))
                    {
                        if (o == 5) pRFont->uc = nVal;
                        else break;
                    }
                }

                pRFont->size = gGuiRes.Size(hRes);
                pRFont->data = (BYTE*)gGuiRes.Lock(hRes);
                if (pRFont->uc)
                    fontupr(pRFont);
            }
        }
    }

    // make all NULL fonts use default ROM font
    for (i = 0; i < LENGTH(vFonts); i++)
    {
        if (!vFonts[i].data)
            vFonts[i] = vFonts[0];
    }
}

int ShowModal(Container *dialog, int flags)
{
    static int getFirstEventTime = 0;

    GEVENT event;
    MOUSE mouse = gMouse;
    gMouse.ChangeCursor(kBitmapMouseCursor);
    gMouse.RangeSet(0, 0, xdim, ydim);
    gMouse.wheelDelay = 10;

    Container desktop(0, 0, xdim, ydim);
    desktop.Insert(dialog);
    pRoot = &desktop;

    memset(clickTime,   0, sizeof(clickTime));
    memset(downTime,    0, sizeof(downTime));
    oldbuttons = 0;
    blinkClock = 0;

    if (!(flags & kModalNoCenter))
    {
        // center the dialog
        dialog->left = (xdim - dialog->width) >> 1;
        dialog->top = (ydim - dialog->height) >> 1;
    }

    if (!(flags & kModalNoFocus))
    {
        if (dialog->focus != &dialog->head)
        {
            desktop.SetFocusOn(dialog->focus);
        }
        else
        {
            // find first item for focus
            while (!desktop.SetFocus(+2));
        }
    }

    int saveSize = bytesperline * ydim;
    BYTE *saveUnder = (BYTE *)Resource::Alloc(saveSize);

    if (flags & kModalFadeScreen)
        gfxFillBoxTrans(0, 0, xdim, ydim, gStdColor[0]);

    // copy save under from last displayed page
#if USE_POLYMOST
    if (getrendermode() < 3)
#endif
    {
        begindrawing();
        memcpy(saveUnder, (void*)frameplace, saveSize);
        enddrawing();
    }

    getFirstEventTime = gFrameClock + 16;
    dialog->isModal = TRUE;

    while (dialog->isModal)
    {
        updateClocks();
        UpdateBlinkClock(gFrameTicks);
        OSD_Draw();

        gMouse.Read();
        handleevents();

        if (gFrameClock > getFirstEventTime)
        {
            GetEvent(&event);

            // trap certain dialog keys
            if (event.type == evKeyDown)
            {
                switch (event.key.ascii)
                {
                    case 27:
                        if (!(flags & kModalNoTrapEsc))
                        {
                            dialog->EndModal(mrCancel);
                            continue;
                        }
                        break;
                    case 13:
                        if (!(flags & kModalNoTrapEnter))
                        {
                            dialog->EndModal(mrOk);
                            continue;
                        }
                        break;
                    case 9:
                        if (event.key.shift) while (!desktop.SetFocus(-1));
                        else while (!desktop.SetFocus(+1));
                        continue;
                }
            }

            desktop.HandleEvent(&event);
        }

        desktop.Paint(0, 0, FALSE);
        gMouse.Draw();
        showframe();


        // restore the save under after page flipping
#if USE_POLYMOST
        if (getrendermode() < 3)
#endif
        {
            if (!(flags & kModalNoRestoreFrame) || dialog->isModal)
            {
                begindrawing();
                memcpy((void*)frameplace, saveUnder, saveSize);
                enddrawing();
            }
        }
    }

    mouse.X = gMouse.X;
    mouse.Y = gMouse.Y;
    gMouse = mouse;

    Resource::Free(saveUnder);
    desktop.Remove(dialog);
    return dialog->endState;
}


int GetStringBox( char *title, char *s )
{
    if (!title) title   = "";
    if (!s)     s       = "";

    const int minWh = ClipLow(pFont->width*strlen(s), 154);
    const int wh = ClipRange(pFont->width*strlen(s), minWh, xdim-4);

    // create the dialog
    Window dialog(0, 0, wh+14, 42, title);

    // this will automatically be destroyed when the dialog is destroyed
    EditText *el = new EditText(4, 4, wh, 18, s, 0x01);
    el->maxlen += 5;
    dialog.Insert(el);

    ShowModal(&dialog);
    if ( dialog.endState != mrOk)
        return 0;       // pressed escape

    // copy the edited string
    strcpy(s, el->string);

    return strlen(s);
}


int GetNumberBox( char *title, int n, int nDefault )
{
    if (!title) title = "";
    const int wh = ClipRange(pFont->width*strlen(title), 154, xdim-4);

    // create the dialog
    Window dialog(0, 0, wh+14, 42, title);

    // this will automatically be destroyed when the dialog is destroyed
    EditNumber *en = new EditNumber(4, 4, wh, 18, n);
    dialog.Insert(en);

    ShowModal(&dialog);
    if ( dialog.endState != mrOk)
        return nDefault;        // pressed escape

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
    w1 = gfxGetTextLen(strupr(upchar), pFont)+pFont->width;
    for (i = 0; i < namesLen; i++)
    {
        sprintf(upchar, names[i].name);
        if ((k = gfxGetTextLen(strupr(upchar), pFont)+pFont->width) > w1)
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
    if (dialog.endState == mrOk && dialog.focus)    // find a button we are focusing on
    {
        Container* pCont = (Container*)dialog.focus;
        TextButton* pFocus = (TextButton*)pCont->focus;
        if (pFocus)
            return pFocus->result;
    }
    return dialog.endState;
}

int showButtons(char** names, int namesLen, char* title)
{
    VOIDLIST list(sizeof(NAMED_TYPE));
    
    for (int i = 0; i < namesLen; i++)
    {
        NAMED_TYPE t = {i, names[i]};
        list.Add(&t);
    }
    
    return showButtons((NAMED_TYPE*)list.First(), list.Length(), title);
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
        array[i].checkbox = new Checkbox(x,  y, array[i].option, array[i].label, (buttons) ? 0 : mrUser + i);
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


int showStandardWindow(char* text, char* buttons, char* caption = "")
{
    #define kDlgMinW        200
    #define kDlgMinH        46

    #define kButW           60
    #define kButH           20
    #define kPadd           4

    QFONT* pTextFont = pFont;
    BOOL newStyle = (strchr(text, '\n') || strchr(text, '\r'));
    int dialogWidth = kDlgMinW, dialogHeight = kDlgMinH;
    int i, t;
    char* pTxt;

    if (!newStyle)
        caption = text;

    Text* pText = new Text(kPadd, kPadd, 0, 0, text, kTextACenter|kTextATop, kColorBlack, pTextFont, -1);
    pTxt = pText->text; // the Text just formatted it as we need

    // get longest line
    for (i = 0; i < pText->lines; i++)
    {
        if ((t = gfxGetTextLen(pTxt, pTextFont)) > dialogWidth)
            dialogWidth = t;

        pTxt+=strlen(pTxt)+1;
    }

    pText->width  = dialogWidth;
    pText->height = (pText->lines * pTextFont->height);

    dialogWidth  += (kPadd<<2);
    if (newStyle)
        dialogHeight += (pText->height+(kPadd<<1));

    Window dialog(0, 0, dialogWidth, dialogHeight, caption); // create the dialog
    Container* pButtons = new Container(0, 0, 0, 0);

    i = 0;
    while(buttons[i] != mrNone)
    {
        t = LENGTH(gStdButtons);
        while(--t >= 0 && gStdButtons[t].id != buttons[i]);

        dassert(t >= 0);
        pButtons->Insert(new TextButton(pButtons->width, 0, kButW, kButH, gStdButtons[t].name, buttons[i]));
        pButtons->width+=(kButW+kPadd);
        i++;
    }

    pButtons->width -= kPadd;
    pButtons->height = kButH;

    if (newStyle)
    {
        pButtons->left   = pText->left + abs((pText->width>>1)-(pButtons->width>>1));
        pButtons->top    = pText->top  + pText->height + kPadd;
        dialog.Insert(pText);
    }
    else
    {
        pButtons->left   = kPadd;
        pButtons->top    = kPadd;
    }

    dialog.Insert(pButtons);

    ShowModal(&dialog);
    if (dialog.endState == mrOk && pButtons->focus) // find a button we are focusing on
    {
        TextButton* pFocus = (TextButton*)pButtons->focus;
        if (pFocus->result)
            return pFocus->result;
    }

    return dialog.endState;

}


int YesNoCancel(char *__format, ...) {

    int retn;
    char buttons[] = { mrYes, mrNo, mrCancel, mrNone };
    char buffer[1024];

    va_list argptr;
    va_start(argptr, __format);
    vsprintf(buffer, __format, argptr);
    va_end(argptr);

    if ((retn = showStandardWindow(buffer, buttons, "Select action")) == mrYes)
        retn = mrOk;

    return retn;
}

void Alert(char *__format, ...) {

    char buttons[] = { mrOk, mrNone };
    char buffer[1024];

    va_list argptr;
    va_start(argptr, __format);
    vsprintf(buffer, __format, argptr);
    va_end(argptr);

    showStandardWindow(buffer, buttons, "Info");
    return;
}

BOOL Confirm(char *__format, ...) {

    int retn;
    char buttons[] = { mrYes, mrNo, mrNone };
    char buffer[1024];

    va_list argptr;
    va_start(argptr, __format);
    vsprintf(buffer, __format, argptr);
    va_end(argptr);

    retn = showStandardWindow(buffer, buttons, "Confirm");
    return (retn == mrYes || retn == mrOk);
}

void ShowFileContents(char* pPath)
{
    char buttons[] = { mrOk, mrNone };
    unsigned char* text = NULL, *p;
    int nSiz;
    
    if ((nSiz = fileLoadHelper(pPath, &text)) > 0)
    {
        if ((p = (unsigned char*)realloc(text, nSiz+1)) != NULL)
        {
            p[nSiz] = '\0';
            showStandardWindow((char*)p, buttons, pPath);
            free(p);
            return;
        }

        free(text);
    }
    
    Alert("Unable to show contents of file '%s'!", pPath);
}

char fade(int rate) {
    return gStdColor[23 + mulscale30(8, Sin(gFrameClock * kAng360 / rate))];
}


int colorPicker(BYTE* colors, char* title, int nDefault, int flags) {

    int i, dx1, dy1, dx2, dy2, dwh, dhg;
    const int r = 16, c = 16, pad = 4;
    int scsp =(ydim >> 7);
    int sz = (8 + scsp);

    int bh = ClipLow(4 + (4*scsp), 20);
    int dw = (c*sz) + (pad * 5);
    int dh = (r*sz) + (pad * 5) + (bh + (bh>>2));

    Window dialog(0, 0, dw, dh, title);
    dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
    dialog.getSize(&dwh, &dhg);

    ColorSelect* pColor = new ColorSelect(1, 1, r, c, sz, colors);
    Panel* pColorCont   = new Panel(dx1, dy1, pColor->width+5, pColor->height+4, 1, 1, -1);
    pColorCont->Insert(pColor);

    TextButton* pOk     = new TextButton(dx1, dy2-bh, 60, bh, "&Select", mrOk);
    TextButton* pCancel = new TextButton(dx1+64, dy2-bh, 60, bh, "&Cancel", mrCancel);
    TextButton* pNone   = new TextButton(dx2-62, dy2-bh, 60, bh, "&None", 100);

    pOk->fontColor = kColorBlue;

    if (!(flags & 0x01))
    {
        pNone->disabled  = 1;
        pNone->canFocus  = 0;
        pNone->fontColor = kColorDarkGray;
    }
    else
    {
        pNone->fontColor = kColorGreen;
    }

    pCancel->fontColor = kColorRed;

    dialog.Insert(pColorCont);
    dialog.Insert(pOk);
    dialog.Insert(pCancel);
    dialog.Insert(pNone);

    switch (ShowModal(&dialog)) {
        case mrOk:
            return pColor->value;
        case 100:
            return -1;
        default:
            return nDefault;
    }
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
//  pcThumbBar = new Container(1,
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

    width       = kCBSize + (strlen(l) * 8);
    height      = kCBSize + (kCBLabelPad >> 1);
    checked     = value;
    id          = itemid;
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
