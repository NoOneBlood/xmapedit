#ifndef __XMAPEDIT_H
#define __XMAPEDIT_H

#include "common_game.h"
#include "xmpdoorwiz.h"
#include "xmpshape.h"
#include "xmploop.h"
#include "xmparcwiz.h"
#include "xmpstub.h"
#include "edit2d.h"
#include "edit3d.h"
#include "xmpmisc.h"
#include "hglt.h"

enum
{
kSectToolShape			= 0	,
kSectToolDoorWiz			,
kSectToolArcWiz				,
kSectToolCurveWall			,
kSectToolUnk				,
};

extern char* const gSectToolNames[];


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
int loopIsEmpty(int nWall);


void getSectorWalls(int nSect, int* s, int *e);
void sectGetEdgeZ(int nSector, int* fz, int* cz);
void sectRotate(int nSect, int cx, int cy, int nAng, int flags);
void sectFlip(int nSect, int cx, int cy, int flags, int);
int sectSplit(int nSect, int nWallA, int nWallB, POINT2D* points, int nPoints);
int sectSplit(int nSect, POINT2D* points, int nPoints);
char sectClone(int nSrc, int nDstS, int nDstW, int flags = 0x07);
short sectCstatAdd(int nSect, short cstat, int objType);
short sectCstatRem(int nSect, short cstat, int objType);
short sectCstatToggle(int nSect, short cstat, int objType);
short sectCstatGet(int nSect, int objType);
short sectCstatGet(int nSect, short cstat, int objType);
short sectCstatSet(int nSect, short cstat, int objType);

void sectDetach(int nSect);
void sectAttach(int nSect);

char sectAutoAlignSlope(int nSect, char which = 0x03);

void wallDetach(int nWall);
void wallAttach(int nWall, int nNextS, int nNextW);
void wallAttach(int nWall, int nNextW);
double getWallLength(int nWall, int nGrid);
void getWallCoords(int nWall, int* x1 = NULL, int* y1 = NULL, int* x2 = NULL, int* y2 = NULL);
short wallCstatAdd(int nWall, short cstat, char nextWall = 1);
short wallCstatRem(int nWall, short cstat, char nextWall = 1);
short wallCstatToggle(int nWall, short cstat, char nextWall = 1);
char wallVisible(int nWall);
int wallGetSect(int nWall);

int getSectorHeight(int nSector);
void setFirstWall(int nSect, int nWall);
char setAligntoWall(int nSect, int nWall);
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
char pointOnWallLine(int x, int y, int* out = NULL);

int findWallAtPos(int x, int y);
int findWallAtPos(int x1, int y1, int x2, int y2);
char insideLoop(int x, int y, int nStartWall);


int insertPoint(int nWall, int x, int y);
void deletePoint(int nWall);

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt = NULL, int* zb = NULL, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags = 0x07);
void ceilGetEdgeZ(int nSector, int* zBot, int* zTop);
void floorGetEdgeZ(int nSector, int* zBot, int* zTop);
void setCstat(BOOL enable, short* pStat, int nStat);
char fixXSprite(int nID);
char fixXSector(int nID);
char fixXWall(int nID);

int worldSprCallFunc(HSPRITEFUNC pFunc, int nData = 0);
int collectWallsOfNode(IDLIST* pList, int nWall, char flags = 0x00);

char sectorToolDisableAll(char alsoFreeDraw);
char sectorToolEnable(int nType, int nData);
char sectorToolDlgLauncher();

inline uint32_t getlenbyrep(int32_t len, int32_t repeat)
{
	return (uint32_t)((repeat > 0) ? divscale12(len, repeat) : len<<12);
}

inline void fixxrepeat(int nWall, int lenrepquot)
{
	if (lenrepquot)
		wall[nWall].xrepeat = ClipRange(((getWallLength(nWall)<<12) + (1<<11)) / lenrepquot, 1, 255);
}

int GetWallZPeg(int nWall);
int AutoAlignWalls(int nWall0, char flags = 0x01);
char AlignWalls(int w0, int z0, int w1, int z1, char doxpan);
int AutoAlignSectors(int nStart, int nFor, IDLIST* pList);
#endif