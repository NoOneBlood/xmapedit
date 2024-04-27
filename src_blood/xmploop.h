/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// New wall loop or sector creation class
// Adapted from overheadeditor() function in build.c
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
#ifndef __XMPLOOP_H
#define __XMPLOOP_H

#include "common_game.h"
#include "xmp2dscr.h"

extern NAMED_TYPE gLoopBuildErrors[];

class LOOPBUILD
{
	private:
		POINT2D *point, *first, *last;
		walltype* pModelW; sectortype* pModelS;
		unsigned int numPoints			: 32;
		signed   int destSect			: 32;
		signed   int status				: 16;
		signed   int splitA				: 32;
		signed   int splitB				: 32;
		int Process(int x, int y);
	public:
		LOOPBUILD()		{ point = NULL;  Start(); }
		~LOOPBUILD()	{ Stop(); }
		void Start();
		void Stop();
		int  Make();
		char Setup(int x, int y);
		//void PrintCaption(SCREEN2D* pScr, int x1, int y1, int x2, int y2, char fc, char bc);
		void Draw(SCREEN2D* pScr);
		void AddPoint(int x, int y, int nPoint = -1);
		void RemPoint(int nPoint = -1);
		inline void UpdPoint(int x, int y, int nPoint)	{ point[nPoint].x = x, point[nPoint].y = y; }
		inline int StatusGet()							{ return status; };
		inline int NumPoints()							{ return numPoints; };
};
#endif