/*******************************************************************************
	FILE:			EDIT2D.H

	DESCRIPTION:

	AUTHOR:			Peter M. Freese
	CREATED:		02-18-96
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/
#ifndef __EDIT2D_H
#define __EDIT2D_H
#include "xmphud.h"
#include "xmpdoorwiz.h"

extern short sectorhighlight;
extern short pointdrag;

void ProcessKeys2D();
int findUnusedPath(DIALOG_ITEM* dialog = NULL, DIALOG_ITEM *control = NULL);
int findUnusedChannel(DIALOG_ITEM* dialog = NULL);
int findUnusedStack();



void getclosestpointonwall(int x, int y, int nWall, int *nx, int *ny);
int getlinehighlight(int nTresh, int x, int y, int nZoom);
int getpointhighlight(int nTresh, int x, int y, int nZoom);

inline void lockSectorDrawing() {gSectorDrawing = 0;}
inline void unlockSectorDrawing() {gSectorDrawing = 1;}


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

extern DOOR_ROTATE* pGDoorR;
extern DOOR_SLIDEMARKED* pGDoorSM;
extern CIRCLEWALL* pGCircleW;

#endif