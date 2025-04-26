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
#ifndef __EDIT2D_H
#define __EDIT2D_H
#include "xmphud.h"
#include "xmpmaped.h"

extern short sectorhighlight;
extern short pointdrag;

void ProcessInput2D();
int findUnusedPath(DIALOG_ITEM* dialog = NULL, DIALOG_ITEM *control = NULL);
int findUnusedChannel(DIALOG_ITEM* dialog = NULL);
int findUnusedStack();



void getclosestpointonwall(int x, int y, int nWall, int *nx, int *ny);
int getlinehighlight(int nTresh, int x, int y, int nZoom);
int getpointhighlight(int nTresh, int x, int y, int nZoom);

#define kMaxCirclePoints 64
class CIRCLEWALL
{
	public:
		POINT2D coords[kMaxCirclePoints + 2];
		unsigned int count 				: 8;
		signed   int line;
		signed   int cenx;
		signed   int ceny;
		//-----------------------------------------------------------
		void Start(int nWall, int x, int y);
		void Stop();
		void Setup(int x, int y);
		void Insert();
		void Draw(SCREEN2D* pScr);
		inline void Reset()					{ line = -1, count = 0; }
		inline void SetPoints(char add)
		{
			if (add) count++; else count--;
			count = ClipRange(count, 2, kMaxCirclePoints);
		}
		//-----------------------------------------------------------
		CIRCLEWALL(int nWall, int x, int y)		{ Start(nWall, x, y); }
		~CIRCLEWALL() 							{ Stop(); }
};

extern SCREEN2D gScreen2D;
extern DOOR_ROTATE* pGDoorR;
extern DOOR_SLIDEMARKED* pGDoorSM;
extern CIRCLEWALL* pGCircleW;
extern LOOPSHAPE* pGLShape;
extern LOOPBUILD* pGLBuild;
extern LOOPSPLIT* pGLoopSplit;
extern SECTAUTOARC* pGArc;
#endif