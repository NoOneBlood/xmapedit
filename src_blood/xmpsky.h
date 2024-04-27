/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// A class to treat parallax sectors as "local" (room) or "global" (map) sky.
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
#ifndef _XMPSKY_H_
#define _XMPSKY_H_
#include "common_game.h"

class Sky
{
	typedef BOOL (*MATCHFUNC)(int, int);
	
	private:
	static int CollectSectors(IDLIST* pList, MATCHFUNC CheckFunc, int nSect, int nFor, int globcnt);
	static char* GlobalOrLocal(BOOL global) { return (global) ? "Global" : "Local"; }
	static char* RightOrLeft(BOOL right) { return (right) ? "Right" : "Left"; }
	//------------------------------------------------------------------------------------
	public:
	static int pieces;
	static unsigned char customBitsFlag;
	static unsigned char tileRepeatCount;
	static int Setup(int nSect, int nFor, int nShade, int nPal, int nNewPic, int panX, int panY, BOOL global);
	static BOOL RotateRecursive(int dir);
	static BOOL Rotate(BOOL right);
	static int SetPan(int nSect, int nFor, int panX, int panY, BOOL global);
	static int SetPal(int nSect, int nFor, int nPal, BOOL global);
	static int SetPic(int nSect, int nFor, int nPic, BOOL global);
	static int SetShade(int nSect, int nFor, int nShade, BOOL global);
	static int ToggleFloorShade(int nSect, BOOL global);
	static int FixPan(int nSect, int nFor, BOOL global);
	static int MakeSimilar(int nSect, int nFor, BOOL global);
	static int Disable(int nSect, int nFor, BOOL global);
	//------------------------------------------------------------------------------------
	static int GetMostUsed(int nSect, int nFor, BOOL global, int* nPic = NULL, int* nPal = NULL, int* nShade = NULL, int* nPy = NULL);
	static void SetBits(int nPic, int* nUnique = NULL);
};

#endif