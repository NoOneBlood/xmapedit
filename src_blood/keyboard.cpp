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

#include "keyboard.h"
#include "build.h"
#include "baselayer.h"

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
	*shift = 0x0;
	if (keystatus[KEY_LSHIFT]) *shift |= 0x01;
	if (keystatus[KEY_RSHIFT]) *shift |= 0x02;
	
	*ctrl = 0x0;
	if (keystatus[KEY_LCTRL]) *ctrl |= 0x01;
	if (keystatus[KEY_RCTRL]) *ctrl |= 0x02;
	
	*alt = 0x0;
	if (keystatus[KEY_LALT]) *alt |= 0x01;
	if (keystatus[KEY_RALT]) *alt |= 0x02;
	
	if (key)
		*key = keyGet();
}