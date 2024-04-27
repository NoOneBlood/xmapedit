#ifndef __XMAPEDIT_H
#define __XMAPEDIT_H

#include "common_game.h"
#include "xmpdoorwiz.h"
#include "xmpshape.h"
#include "xmploop.h"
#include "xmpstub.h"
#include "edit2d.h"
#include "edit3d.h"
#include "xmpmisc.h"



void avePointLoop(int s, int e, int *x, int *y);
void midPointLoop(int nFirst, int nLast, int* ax, int *ay);
void avePointSector(int nSector, int *x, int *y);
void avePointWall(int nWall, int* x, int* y, int* z);
void avePointWall(int nWall, int* x, int* y);


void loopGetEdgeWalls(int nFirst, int nLast, int* l, int* r, int* t, int* b);
void loopGetWalls(int nStartWall, int* swal, int *ewal);
void loopGetBox(int nFirst, int nLast, int* x1, int* y1, int* x2, int *y2);
void loopChgPos(int s, int e, int bx, int by, int flags);
void loopRotate(int s, int e, int cx, int cy, int nAng, int flags);
void loopFlip(int s, int e, int cx, int cy, int flags);
void loopDelete(int s, int e);


void getSectorWalls(int nSect, int* s, int *e);
void sectGetEdgeZ(int nSector, int* fz, int* cz);
void sectRotate(int nSect, int cx, int cy, int nAng, int flags);
void sectFlip(int nSect, int cx, int cy, int flags, int);
int sectSplit(int nSect, int nWallA, int nWallB, POINT2D* points, int nPoints);
int sectSplit(int nSect, POINT2D* points, int nPoints);
char sectClone(int nSrc, int nDstS, int nDstW, int flags = 0x07);
short sectCstatAdd(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatRem(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatToggle(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatGet(int nSect, int objType = OBJ_FLOOR);
short sectCstatSet(int nSect, short cstat, int objType = OBJ_FLOOR);
void sectDetach(int nSect);
void sectAttach(int nSect);


void wallDetach(int nWall);
void wallAttach(int nWall, int nNextS, int nNextW);
double getWallLength(int nWall, int nGrid);
void getWallCoords(int nWall, int* x1 = NULL, int* y1 = NULL, int* x2 = NULL, int* y2 = NULL);
short wallCstatAdd(int nWall, short cstat, char nextWall = 1);
short wallCstatRem(int nWall, short cstat, char nextWall = 1);
short wallCstatToggle(int nWall, short cstat, char nextWall = 1);

int getSectorHeight(int nSector);
void setFirstWall(int nSect, int nWall);
char loopInside(int nSect, POINT2D* pPoint, int nCount, char full);
int insertLoop(int nSect, POINT2D* pInfo, int nCount, walltype* pWModel = NULL, sectortype* pSModel = NULL);
void insertPoints(WALLPOINT_INFO* pInfo, int nCount);
int makeSquareSector(int ax, int ay, int area);
int redSectorCanMake(int nStartWall);
int redSectorMake(int nStartWall);
int redSectorMerge(int nThis, int nWith);
char insidePoints(int x, int y, POINT2D* point, int c);
char insidePoints(int nSect, POINT2D* point, int c);
char sectWallsInsidePoints(int nSect, POINT2D* point, int c);

char pointOnLine(int x, int y, int x1, int y1, int x2, int y2);
char pointOnWallLine(int nWall, int x, int y);
char pointOnWallLine(int x, int y);

int findWallAtPos(int x, int y);
char insideLoop(int x, int y, int nStartWall);


int insertPoint(int nWall, int x, int y);

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt = NULL, int* zb = NULL, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags = 0x07);
void ceilGetEdgeZ(int nSector, int* zBot, int* zTop);
void floorGetEdgeZ(int nSector, int* zBot, int* zTop);
void setCstat(BOOL enable, short* pStat, int nStat);
char fixXSprite(int nID);
char fixXSector(int nID);
char fixXWall(int nID);
#endif