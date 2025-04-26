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

#ifndef _XMPINPUT
#define _XMPINPUT
#include "xmpconf.h"

#define KEY_ESC			0x01
#define KEY_1			0x02
#define KEY_2			0x03
#define KEY_3			0x04
#define KEY_4			0x05
#define KEY_5			0x06
#define KEY_6			0x07
#define KEY_7			0x08
#define KEY_8			0x09
#define KEY_9			0x0A
#define KEY_0			0x0B
#define KEY_MINUS		0x0C
#define KEY_EQUALS		0x0D
#define KEY_PLUS		0x0D
#define KEY_BACKSPACE	0x0E
#define KEY_TAB			0x0F
#define KEY_Q			0x10
#define KEY_W			0x11
#define KEY_E			0x12
#define KEY_R			0x13
#define KEY_T			0x14
#define KEY_Y			0x15
#define KEY_U			0x16
#define KEY_I			0x17
#define KEY_O			0x18
#define KEY_P			0x19
#define KEY_LBRACE		0x1A
#define KEY_RBRACE		0x1B
#define KEY_ENTER		0x1C
#define KEY_LCTRL		0x1D
#define KEY_A			0x1E
#define KEY_S			0x1F
#define KEY_D			0x20
#define KEY_F 			0x21
#define KEY_G			0x22
#define KEY_H			0x23
#define KEY_J			0x24
#define KEY_K			0x25
#define KEY_L			0x26
#define KEY_SEMICOLON	0x27
#define KEY_APOSTROPHE	0x28
#define KEY_TILDE		0x29
#define KEY_LSHIFT 		0x2A
#define KEY_BACKSLASH	0x2B
#define KEY_Z			0x2C
#define KEY_X			0x2D
#define KEY_C			0x2E
#define KEY_V			0x2F
#define KEY_B			0x30
#define KEY_N			0x31
#define KEY_M			0x32
#define KEY_COMMA		0x33
#define KEY_PERIOD		0x34
#define KEY_SLASH		0x35
#define KEY_RSHIFT 		0x36
#define KEY_PADSTAR		0x37
#define KEY_LALT 		0x38
#define KEY_SPACE 		0x39
#define KEY_CAPSLOCK	0x3A
#define KEY_F1			0x3B
#define KEY_F2			0x3C
#define KEY_F3			0x3D
#define KEY_F4			0x3E
#define KEY_F5			0x3F
#define KEY_F6			0x40
#define KEY_F7			0x41
#define KEY_F8			0x42
#define KEY_F9			0x43
#define KEY_F10			0x44
#define KEY_NUMLOCK		0x45
#define KEY_SCROLLLOCK	0x46
#define KEY_PAD7		0x47
#define KEY_PADHOME		0x47
#define KEY_PAD8		0x48
#define KEY_PADUP		0x48
#define KEY_PAD9	 	0x49
#define KEY_PADPAGEUP 	0x49
#define KEY_PADMINUS  	0x4A
#define KEY_PAD4	 	0x4B
#define KEY_PADLEFT		0x4B
#define KEY_PAD5	 	0x4C
#define KEY_PAD6	 	0x4D
#define KEY_PADRIGHT 	0x4D
#define KEY_PADPLUS  	0x4E
#define KEY_PAD1	 	0x4F
#define KEY_PADEND	 	0x4F
#define KEY_PAD2	 	0x50
#define KEY_PADDOWN 	0x50
#define KEY_PAD3	 	0x51
#define KEY_PADPAGEDN 	0x51
#define KEY_PAD0	 	0x52
#define KEY_PADINSERT 	0x52
#define KEY_PADPERIOD	0x53
#define KEY_PADDELETE	0x53
#define KEY_SYSREQ		0x54

#define KEY_F11			0x57
#define KEY_F12			0x58

#define KEY_PADENTER	0x9C
#define KEY_RCTRL		0x9D
#define KEY_PADSLASH	0xB5
#define KEY_PRINTSCREEN	0xB7
#define KEY_RALT		0xB8
#define KEY_PAUSE		0xC5
#define KEY_BREAK		0xC6
#define KEY_HOME		0xC7
#define KEY_UP			0xC8
#define KEY_PAGEUP 		0xC9
#define KEY_LEFT		0xCB
#define KEY_RIGHT 		0xCD
#define KEY_END	 		0xCF
#define KEY_DOWN 		0xD0
#define KEY_PAGEDN 		0xD1
#define KEY_INSERT 		0xD2
#define KEY_DELETE		0xD3

extern unsigned char const keyAscii[128];
extern unsigned char const keyAsciiSH[128];

void keyClear(void);
unsigned char keyGet(void);
unsigned char keyGetHit(void);
void keyGetHelper(unsigned char* key, unsigned char* ctrl, unsigned char* shift, unsigned char* alt);

class MOUSE
{
	private:
	#pragma pack(push, 1)
	struct CURSOR
	{
		unsigned int type			: 7;
		unsigned int width			: 7;
		unsigned int height			: 7;
		unsigned int pad			: 7;
	};
	#pragma pack(pop)
	unsigned int initialized	: 16;
	//---------------------------------
	public:
	CURSOR cursor;
	//---------------------------------
	unsigned int top			: 16;
	unsigned int left			: 16;
	unsigned int right			: 16;
	unsigned int bottom			: 16;
	//---------------------------------
	unsigned int velX, velDX	: 12;
	unsigned int velY, velDY	: 12;
	unsigned int velRst			: 1;
	unsigned int velTimer		: 32;
	//---------------------------------
	signed int dX1, dX2			: 16;
	signed int dY1, dY2			: 16;
	signed int dfX, dfY			: 32;
	signed int X, Y				: 16;
	//---------------------------------
	signed   int buttons;
	unsigned int press, first	: 10;
	unsigned int hold, release	: 10;
	//---------------------------------
	signed 	 int wheel			: 2;
	unsigned int wheelDelay		: 8;
	unsigned int wheelTimer		: 32;
	//---------------------------------
	
	MOUSE();
	void Init(MOUSE_PREFS* pPrefs);
	int  Initialized() { return initialized; }
	void Read();
	void ReadCoords();
	void ReadButtons();
	void RangeSet(MOUSE* pMouse);
	void RangeSet(int x1, int y1, int x2, int y2);
	void VelocitySet(int vX = -1, int vY = -1, int vR = -1);
	void VelocityReset();
	void VelocityProcess();
	void ChangeCursor(int nId);
	BOOL Moved(char which = 0x03);
	void Draw();
};

extern MOUSE gMouse;
#endif