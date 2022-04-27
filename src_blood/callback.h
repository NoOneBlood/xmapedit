/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Written by NoOne.
// A lite version of callback.cpp from Nblood adapted for level editor's Preview Mode
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

#ifndef __CALLBACK_H
#define __CALLBACK_H
enum CALLBACK_ID {
	kCallbackNone = -1,
	kCallbackRemove = 0,
	kCallbackRemoveSpecial = 1,
	kCallbackMissileBurst,
	kCallbackMissileSpriteBlock,
	kCallbackFXFlareBurst,
	kCallbackCounterCheck,
	kCallbackMax,
};

typedef void (* CALLBACK_FUNC)( int nIndex );
extern CALLBACK_FUNC gCallback[kCallbackMax];

#endif

