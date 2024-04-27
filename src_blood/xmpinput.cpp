/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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
#include "screen.h"
#include "xmpinput.h"


unsigned char const keyAscii[128] =
{
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

unsigned char const keyAsciiSH[128] =
{
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

unsigned char keyGet(void)
{
	if (keyfifoplc == keyfifoend)
		return 0;

	int key = keyfifo[keyfifoplc];
	keyfifoplc = (keyfifoplc + 2) & (KEYFIFOSIZ - 1);
	return (unsigned char)key;
}

void keyClear(void)
{
	keyfifoplc = keyfifoend;
}

void keyGetHelper(unsigned char* key, unsigned char* ctrl, unsigned char* shift, unsigned char* alt)
{
	if (shift)
	{
		*shift = 0x0;
		if (keystatus[KEY_LSHIFT]) *shift |= 0x01;
		if (keystatus[KEY_RSHIFT]) *shift |= 0x02;
	}
	
	if (ctrl)
	{
		*ctrl = 0x0;
		if (keystatus[KEY_LCTRL]) *ctrl |= 0x01;
		if (keystatus[KEY_RCTRL]) *ctrl |= 0x02;
	}
	
	if (alt)
	{
		*alt = 0x0;
		if (keystatus[KEY_LALT]) *alt |= 0x01;
		if (keystatus[KEY_RALT]) *alt |= 0x02;
	}
	
	if (key)
		*key = keyGet();
}


MOUSE gMouse;
MOUSE::MOUSE()
{
	Init(&gMousePrefs);
}

void MOUSE::Init(MOUSE_PREFS* pPrefs)
{
	velX = velDX		= pPrefs->speedX;
	velY = velDY		= pPrefs->speedY;
	velRst				= 0;
	
	buttons				= 0;
	press				= 0;
	hold				= 0;
	release				= 0;
	
	wheel				= 0;
	wheelDelay			= 0;
	wheelTimer			= 0;
		
	ChangeCursor(0);
	RangeSet(0, 0, xdim, ydim);
	
	X = (right  - left) >> 1;
	Y = (bottom - top)  >> 1;
	
	initialized++;
	return;
}

void MOUSE::Read()
{
	ReadButtons();
	ReadCoords();
	return;
}

void MOUSE::ReadCoords()
{
	VelocityProcess();
	
	int mx, my, dummy;
	getmousevalues(&mx, &my, &dummy);

	mx *= velX;
	my *= velY;
	
	dfX += mx * 640;
	dfY += my * 400;
	
	dX2 = dfX >> 16;
	dY2 = dfY >> 16;

	dfX &= 0xFFFF;
	dfY &= 0xFFFF;

	dX1 = X;
	dY1 = Y;

	X = ClipRange(X + dX2, left, right  - 1);
	Y = ClipRange(Y + dY2, top,  bottom - 1);

	dX1 = X - dX1;
	dY1 = Y - dY1;
	
	return;
}

void MOUSE::ReadButtons()
{
	VelocityProcess();
	readmousebstatus(&buttons);
	if ((buttons & 16) || (buttons & 32))
	{
		if (wheelDelay && totalclock <= wheelTimer)
		{
			buttons &= ~48;
			wheel = 0;
		}
		else
		{
			wheel = (buttons & 16) ? -1 : 1;
			wheelTimer = totalclock + wheelDelay;
		}
	}
	else
	{
		wheel = 0;
	}
	
	
	release = 0;
	if ((hold & 1) && !(buttons & 1)) release |= 1;
	if ((hold & 2) && !(buttons & 2)) release |= 2;
	if ((hold & 4) && !(buttons & 4)) release |= 4;
	
	press	= (~hold & buttons);
	hold	= buttons;
	
	return;
}

void MOUSE::RangeSet(MOUSE* pMouse)
{
	RangeSet(pMouse->left, pMouse->top, pMouse->right, pMouse->bottom);
}

void MOUSE::RangeSet(int x1, int y1, int x2, int y2)
{
	left 	= x1;	right 	= x2;
	top 	= y1;	bottom 	= y2;

	dX1 = dX2 = dfX = 0;
	dY1 = dY2 = dfY = 0;
	
	X = ClipRange(X, left + 1, right  - 1);
	Y = ClipRange(Y, top  + 1, bottom - 1);
	
	return;
}

void MOUSE::VelocitySet(int vX, int vY, int vR)
{
	if (vX >= 0)	velX 	= vX;
	if (vY >= 0)	velY 	= vY;
	if (vR >= 0)	velRst  = (vR > 0) ? 1 : 0;
}

void MOUSE::VelocityReset()
{
	velX = velDX; velY = velDY;
	velRst = false;
}

void MOUSE::VelocityProcess()
{
	#define kTicks 4
	#define kStep  2
	
	// smooth mouse speed reset
	if (velRst && velTimer <= totalclock)
	{
		if (velX < velDX) velX = ClipHigh(velX + kStep, velDX);
		else if (velX > velDX)
			velX = ClipLow(velX - kStep, velDX);
		
		if (velY < velDY) velY = ClipHigh(velY + kStep, velDY);
		else if (velY > velDY)
			velY = ClipLow(velY - kStep, velDY);
		
		
		velRst = (velX != velDX || velY != velDY);
		if (velRst)
			 velTimer = totalclock + kTicks;
	}
}

void MOUSE::ChangeCursor(int nId)
{
	if (nId > 0 && pBitmaps && pBitmaps[nId])
	{
		cursor.type 	= nId;
		cursor.width	= pBitmaps[nId]->width;
		cursor.height	= pBitmaps[nId]->height;
		cursor.pad		= 0;
	}
	else
	{
		cursor.type 	= 0;
		cursor.width	= mulscale16(10, 0x10000);
		cursor.height	= mulscale16(10, 0x10000);
		cursor.pad		= mulscale16(4,  0x10000);
	}
}

void MOUSE::Draw() {
	
	int i;
	int nCur 	= cursor.type;
	int wh 		= cursor.width;
	int hg 		= cursor.height;
	int pd 		= cursor.pad;

	switch (nCur) {
		case 0:
			if (qsetmode == 200)
			{
				gfxSetColor(fade());
				gfxHLine(Y, X-wh, X-pd);
				gfxHLine(Y, X+pd, X+wh);
				gfxVLine(X, Y-hg, Y-pd);
				gfxVLine(X, Y+pd, Y+hg);
				gfxPixel(X, Y);
			}
			else
			{
				pd>>=1;
				gfxSetColor(gStdColor[gridlock ? kColorLightRed : kColorWhite]);
				for (i = 0; i < 2; i++)
				{
					gfxHLine(Y+i, X-wh, X-pd);
					gfxHLine(Y+i, X+pd, X+wh);
					gfxVLine(X+i, Y-hg, Y-pd);
					gfxVLine(X+i, Y+pd, Y+hg);
				}
			}
			break;
		default:
			gfxDrawBitmap(pBitmaps[nCur], X, Y);
			break;
	}
	
	return;
}

BOOL  MOUSE::Moved(char which)
{
	if (which & 0x01) return dX2;
	if (which & 0x02) return dY2;
	return FALSE;
}