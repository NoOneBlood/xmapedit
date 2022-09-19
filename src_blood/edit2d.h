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

extern short sectorhighlight;

int EditDialog(DIALOG_ITEM *dialog);
void PaintDialog(DIALOG_ITEM *dialog);
void PaintDialogItem(DIALOG_ITEM *dialog, DIALOG_ITEM *control, int focus);

void ProcessKeys2D();
int findUnusedPath(DIALOG_ITEM* dialog = NULL, DIALOG_ITEM *control = NULL);
int findUnusedChannel(DIALOG_ITEM* dialog = NULL);
int findUnusedStack();

#define kVtxSize 2
void _fastcall scaleAngLine2d(int scale, int ang, int* x, int* y);
void Draw2dWallMidPoint(int nWall, char color = 15, char which = 0x01);
void Draw2dVertex(int x, int y, int color, int color2 = -1, int size = kVtxSize);
void Draw2dCross(int x, int y, int color, int size);
void Draw2dWall( int x0, int y0, int x1, int y1, char color, char thick, int pat = kPatNormal);
void DrawBuild2dFaceSprite(int x, int y, int color);
void Draw2dFaceSprite( int x, int y, int color);
void Draw2dFloorSpriteFull(int xp1, int yp1, char color, int idx);
void DrawCircle( int x, int y, int radius, int nColor, BOOL dashed = FALSE);
void drawHighlight(int x1, int y1, int x2, int y2, char color);

int redSectorCanMake(int nStartWall);
int redSectorMake(int nStartWall);
int redSectorMerge(int nThis, int nWith);
void draw2dArrowMarker(int xS, int yS, int xD, int yD, int nColor, int nZoom, int nAng = kAng30, char thick = FALSE, int pat1 = kPatNormal, int pat2 = kPatNormal);
void loopGetEdgeWalls(int nFirst, short* lw, short* rw, short* tw, short* bw);



void EditSectorData(int nSector, BOOL xFirst = TRUE);
void EditWallData(int nWall, BOOL xFirst = TRUE);
void EditSpriteData(int nSprite, BOOL xFirst = TRUE);
void EditSectorLighting(int nSector);
void ShowSectorData(int nSector, BOOL xFirst, BOOL dialog = TRUE);
void ShowWallData(int nWall, BOOL xFirst, BOOL dialog = TRUE);
void ShowSpriteData(int nSprite, BOOL xFirst, BOOL dialog = TRUE);

void controlSetReadyLabel(DIALOG_ITEM *control, char* str);
int getlinehighlight(int nTresh, int x, int y, int nZoom);
int getpointhighlight(int nTresh, int x, int y, int nZoom);
void SetControlValue(DIALOG_ITEM *dialog, int group, int value );
void loopGetWalls(int nStartWall, int* swal, int *ewal);

void FASTCALL sectorDetach(int nSector);
void FASTCALL sectorAttach(int nSector);
#endif