/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Misc functions for xmapedit.
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


#ifndef __XMPMISC
#define __XMPMISC

#include "build.h"
#include "resource.h"
#include "gameutil.h"
#include "gfx.h"

#define kSEQSig				"SEQ\x1A"
#define kQavSig				"QAV\x1A"
#define kBloodMapSig		"BLM\x1A"
#define kBakExt				".BAK"
#define kBakSignatureLength 32

enum {
kSurfNone = 0,
kSurfStone,
kSurfMetal,
kSurfWood,
kSurfFlesh,
kSurfWater,
kSurfDirt,
kSurfClay,
kSurfSnow,
kSurfIce,
kSurfLeaves,
kSurfCloth,
kSurfPlant,
kSurfGoo,
kSurfLava,
kSurfMax,
};

#pragma pack(push, 1)
struct NAMED_TYPE;
struct FUNCT_LIST
{
	int funcType;
	void* pFunc;
};
#pragma pack(pop)

void Delay(int time);
BOOL Beep(BOOL cond);
void BeepOk( void );
void BeepFail( void );
int scanBakFile(char* file);
inline BOOL rngok(int val, int rngA, int rngB) { return (val >= rngA && val < rngB); }
BYTE fileExists(char* filename, RESHANDLE* rffItem = NULL);
int fileLoadHelper(char* filepath, BYTE** out, int* loadFrom = NULL);
void updateClocks();
void gfxPrinTextShadow(int x, int y, int col, char* text, QFONT *pFont = NULL, int shofs = 1);
void splashScreen(char* text = NULL);
void plsWait();
inline BOOL wallHoriz(int nWall) {
	
	return (wall[nWall].x == wall[wall[nWall].point2].x);
	
}

inline BOOL wallVert(int nWall) {
	
	return (wall[nWall].y == wall[wall[nWall].point2].y);
	
}

BOOL isModernMap();
void avePointSector(int nSector, int *x, int *y);
int getDataOf(BYTE idata, short objType, short objIndex);
int getClosestId(int nId, int nMaxId, char* nType, BOOL dir);
void clampSprite(spritetype* pSprite);
void clampSpriteZ(spritetype* pSprite, int z, int which);
void clampCamera();
BOOL obsoleteXObject(int otype, int oxindex);
char getPLU(int nTile, char* nPlu, BOOL dir, int minPerc = 0);
int isEffectivePLU(int nTile, BYTE* plu, int* changed = NULL, int* total = NULL);
short name2TypeId(char defName[256]);
void eraseExtra();
BOOL objectLockShowStatus(int x, int y);
int getHighlightedObject();
char* getFilename(char* pth, char* out, BOOL addExt = FALSE);
char* getPath(char* pth, char* out, BOOL addSlash = FALSE);
void playSound(int sndId, int showMsg = 1);


void cpyObjectClear();
void doGridCorrection(int* x, int* y, int nGrid);
void doWallCorrection(int nWall, int* x, int* y, int shift = 14);
void dozCorrection(int* z, int zStep = 0x3FF);
void hit2pos(int* rtx, int* rty, int* rtz = NULL, int32_t clipmask = BLOCK_MOVE | BLOCK_HITSCAN);
void hit2sector(int *rtsect, int* rtx, int* rty, int* rtz, int gridCorrection, int32_t clipmask = BLOCK_MOVE | BLOCK_HITSCAN);
void printextShadow(int xpos, int ypos, short col, char* text, int font);
void printextShadowL(int xpos, int ypos, short col, char* text);
void printextShadowS(int xpos, int ypos, short col, char* text);
void findSectorMarker(int nSect, int *nIdx1, int *nIdx2);
BYTE mostUsedColor(BYTE* pixels, int len, short noColor);
void doAutoGrid();
void setupSecrets();
char* array2str(NAMED_TYPE* in, int inLen, char* out, int outLen);
void removeExtension (char *str);
void pathSplit2(char *pzPath, char* buff, char** dsk, char** dir, char** fil, char** ext);
void resortFree();
void TranslateWallToSector( void );
BOOL chlRangeIsFine(int val);
short getSector();
BOOL slash(char ch); 
char* catslash(char* str);
BOOL markerIsNode(int idx, int goal);
void gfxDrawCaption(int x, int y, int fr, int bg, int bgpad, char* txt, QFONT* pFont);
BOOL getSeqPrefs(int nSeq, short* pic, short* xr, short* yr, short* plu);
char* retnCodeCheck(int retnCode, NAMED_TYPE* errMsg);
BOOL tmpFileMake(char* out);
int replaceByte(BYTE* buf, int len, BYTE valueA, BYTE valueB);
BOOL fileAttrSet(char* file, int attr);
BOOL fileAttrSetRead(char* file);
BOOL fileAttrSetWrite(char* file);
BOOL fileDelete(char* file);
BOOL irngok(int val, int rngA, int rngB);
void getSectorWalls(int nSect, int* swal, int *ewal);
void avePointWall(int nWall, int* x, int* y);
BOOL isMultiTx(short nSpr);
BOOL multiTxPointsRx(int rx, short nSpr);
void setCstat(BOOL enable, short* pStat, int nStat);
short wallCstatAdd(int nWall, short cstat, BOOL nextWall = TRUE);
short wallCstatRem(int nWall, short cstat, BOOL nextWall = TRUE);
short wallCstatToggle(int nWall, short cstat, BOOL nextWall = TRUE);
int perc2val(int reqPerc, int val);
void swapValues(int *nSrc, int *nDest);
void toggleResolution(int fs, int xres, int yres, int bpp = 8);
int scaleZ(int sy, int hz);
int keyneg(int step, BYTE key = 0, bool rvrs = false);
int kneg(int step, bool rvrs = false);
char* getExt(int nExt);
int DlgSaveChanges(char* text, BOOL askForArt = FALSE);
int countUniqBytes(BYTE* pBytes, int len);
void* getFuncPtr(FUNCT_LIST* db, int dbLen, int nType);
int getTypeByExt(char* str, NAMED_TYPE* db, int len);
BOOL isSkySector(int nSect, int nFor);
BOOL ceilPicMatch(int nSect, int nPic);
BOOL floorPicMatch(int nSect, int nPic);
BOOL makeBackup(char* filename);
//void getSpriteExtents2(spritetype* pSpr, int* x1, int* y1);
//BOOL ss2obj(int* objType, int* objIdx, BOOL asIs = FALSE);
//BOOL dosboxRescan();
//void pathMake(char* out, char* dsk, char* dir, char* fil, char* ext);
#endif