/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Intends for various wall loop shape creation
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

#ifndef __XMPSHAPE_H
#define __XMPSHAPE_H

#include "common_game.h"
#include "xmp2dscr.h"

enum
{
	kLoopShapeRect			= 0,
	kLoopShapeCircle		= 1,
	kLoopShapeSquare		= 2,
	kLoopShapeTriangle		= 3,
	kLoopShapeMax
};


extern NAMED_TYPE gLoopShapeErrors[];
extern NAMED_TYPE gLoopShapeTypes[kLoopShapeMax];


class LOOPSHAPE
{
	private:
		OBJECT model;
		POINT2D point[255];
		unsigned int active			: 1;
		unsigned int rotateAng		: 16;
		unsigned int shapeType		: 8;
		unsigned int minPoints		: 8;
		unsigned int maxPoints		: 8;
		unsigned int numPoints		: 8;
		unsigned int minWidth		: 32;
		unsigned int minHeight		: 32;
		unsigned int width			: 32;
		unsigned int height			: 32;
		unsigned int size			: 32;
		signed   int destSect		: 32;
		signed   int status			: 16;
		signed	 int sx, sy;
		signed	 int cx, cy;
		signed	 int ex, ey;
	public:
		void Start(int nType, int nSect, int x, int y);
		void Stop();
		char Setup(int x2, int y2, OBJECT* pModel);
		void SetupRectangle(int x2, int y2);
		void SetupSquare(int x2, int y2);
		void SetupTriangle(int x2, int y2);
		void SetupCircle(int x2, int y2);
		void Draw(SCREEN2D* pScr);
		int Insert();
		
		void UpdateShape(int x, int y);
		char ChgPoints(int nNum);
		void ChgAngle(int nAng);
		LOOPSHAPE(int nType, int nSect, int x, int y)			{ active = 0; Start(nType, nSect, x, y);	}
		~LOOPSHAPE()											{ Stop();	active = 0;						}
		inline void AddAngle(int nAng = 32)						{ ChgAngle(klabs(nAng)); 					}
		inline void RemAngle(int nAng = 32)						{ ChgAngle(-nAng); 							}
		inline char AddPoints(int nNum = 1)						{ return ChgPoints(klabs(nNum)); 			}
		inline char RemPoints(int nNum = 1)						{ return ChgPoints(-nNum); 					}
		inline void Start(int nSect, int x, int y)				{ Start(shapeType, nSect, x, y);			}
		inline int Width()										{ return width;								}
		inline int Height()										{ return height;							}
		inline int StatusGet()									{ return status;							}
		inline int SectorGet()									{ return destSect;							}
		inline int NumPoints()									{ return numPoints;							}
		inline void StatusSet(int nStat)						{ status = nStat;							}
};
#endif