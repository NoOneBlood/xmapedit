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

#include "common_game.h"
#include "gameutil.h"
#include "screen.h"
#include "db.h"



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

enum
{
kMapStatDudes       = 0,
kMapStatSpawn,
//kMapStatItems,
kMapStatWeapons,
kMapStatAmmo,
kMapStatPowerup,
kMapStatHealth,
kMapStatArmor,
kMapStatInventory,
kMapStatMax,
};

extern const char* gMapStatsNames[kMapStatMax];

XSPRITE* GetXSpr(spritetype* pSpr);
XSECTOR* GetXSect(sectortype* pSect);
XWALL* GetXWall(walltype* pWall);




class MapStats
{
    private:
        static uint16_t stats[5][5][kMapStatMax];
        static int16_t  total[5][kMapStatMax];
        static void IncItemType(int nType, XSPRITE* pXSpr);
        static char Inc(XSPRITE* pXSpr, int nWhat);
    public:
        static int Get(int nMode, int nWhat);
        static int Get(int nMode, int nSkill, int nWhat);
        static void Collect();
        static void Clear(char which = 0x03);
};

enum
{
    B2T_ENC_BREP    = 0x01,
    B2T_ENC_000F    = 0x02,
    B2T_ENC_MODS    = 0x04,
};

class BIN2TXT
{
    private:
        static const char* kB2TSign;
        inline int GetDigit(char* str, int nLen) { str[nLen] = '\0'; return strtol(str, NULL, 16); }
        int EncodeByte(int c);
        int DecodeByte(int c);
    public:
        struct
        {
            uint32_t    len;
            uint8_t*    ptr;
        }
        bin;
        struct
        {
            int32_t     len;
            int32_t*    ptr;
            uint8_t     flg;
        }
        inf;
        struct
        {
            uint32_t    len;
            char*       ptr;
        }
        txt;
        char Encode(void);
        char Decode(void);
        BIN2TXT(void);
};

#pragma pack(push, 1)
struct FUNCT_LIST
{
    int funcType;
    void* pFunc;
};

struct SCANWALL
{
    POSOFFS pos;
    short w, s;
};
#pragma pack(pop)



char scanWallOfSector(SCANWALL* pIn, SCANWALL* pOut, int snapDist = 0);
extern NAMED_TYPE gReverseSectorErrors[3];
extern int gNextWall[kMaxWalls];


void Delay(int time);


void splashScreen(char* text = NULL);
void plsWait();
inline BOOL wallHoriz(int nWall) {

    return (wall[nWall].x == wall[wall[nWall].point2].x);

}

inline BOOL wallVert(int nWall) {

    return (wall[nWall].y == wall[wall[nWall].point2].y);

}

BOOL isModernMap();

int getDataOf(int nData, int nType, int nID);
char setDataOf(int nData, int nVal, int nType, int nID);
int getClosestId(int nId, int nMaxId, char* nType, BOOL dir);
void clampSprite(spritetype* pSprite, int which = 0x03);
void clampSpriteZ(spritetype* pSprite, int z, int which);
void clampCamera();
BOOL obsoleteXObject(int otype, int oxindex);
char nextEffectivePlu(int nTile, signed char nShade, unsigned char nPlu, BOOL dir, int minPerc = 0);
int isEffectivePLU(int nTile, BYTE* pDestPlu, signed char nShade = 0, BYTE* pNormPlu = palookup[kPlu0], int* changed = NULL, int* total = NULL);
short name2TypeId(char defName[256]);
void eraseExtra();
int getHighlightedObject();

void auditSound(int nSnd, int nType, char showMsg = 1);
void inline playSound(int nSnd, char showMsg)   { auditSound(nSnd, kSoundPlayer, showMsg); }
void inline playSound(int nSnd)                 { auditSound(nSnd, kSoundPlayer, 1); }

void cpyObjectClear();
void hit2pos(int* rtx, int* rty, int* rtz = NULL, int32_t clipmask = BLOCK_MOVE | BLOCK_HITSCAN);
void hit2sector(int *rtsect, int* rtx, int* rty, int* rtz, int gridCorrection, int32_t clipmask = BLOCK_MOVE | BLOCK_HITSCAN);
void findSectorMarker(int nSect, int *nIdx1, int *nIdx2);
void doAutoGrid();
int setupSecrets();
char* array2str(NAMED_TYPE* in, int inLen, char* out, int outLen);

void resortFree();
void TranslateWallToSector( void );
short getSector();

BOOL markerIsNode(int idx, int goal);

BOOL getSeqPrefs(int nSeq, short* pic, short* xr, short* yr, short* plu);
char* retnCodeCheck(int retnCode, NAMED_TYPE* errMsg);
BOOL tmpFileMake(char* out);
int replaceByte(BYTE* buf, int len, BYTE valueA, BYTE valueB);

BOOL isMultiTx(short nSpr);
BOOL multiTxGetRange(int nSpr, int out[4]);
BOOL multiTxPointsRx(int rx, short nSpr);


void toggleResolution(int fs, int xres, int yres, int bpp = 8);
int camHitscan(short* hsc, short* hwl, short* hsp, int* hx, int* hy, int* hz, unsigned int clipMask);
int keyneg(int step, BYTE key = 0, bool rvrs = false);
int kneg(int step, bool rvrs = false);
char* getExt(int nExt);
int DlgSaveChanges(char* text, BOOL askForArt = FALSE);
char dlgSaveAndOrQuit(void);
int countUniqBytes(BYTE* pBytes, int len);
void* getFuncPtr(FUNCT_LIST* db, int dbLen, int nType);
int getTypeByExt(char* str, NAMED_TYPE* db, int len);
BOOL isSkySector(int nSect, int nFor);
BOOL ceilPicMatch(int nSect, int nPic);
BOOL floorPicMatch(int nSect, int nPic);


BOOL isUnderwaterSector(XSECTOR* pXSect);
BOOL isUnderwaterSector(sectortype* pSect);
BOOL isUnderwaterSector(int nSector);
int formatMapInfo(char* str);

int collectObjectsByChannel(int nChannel, BOOL rx, OBJECT_LIST* pOut = NULL, char flags = 0x0);
int pluPick(int nTile, int nShade, int nPlu, char* titleArg);
int pluPickAdvanced(int nTile, int nShade, int nPlu, char* titleArg);
int pluPickClassic(int nTile, int nShade, int nPlu, char* titleArg);

void setPluOf(int nPlu, int oType, int oIdx);
int getPluOf(int oType, int oIdx);
int getPicOf(int oType, int oIdx);
int getShadeOf(int oType, int oIdx);
void setShadeOf(int nShade, int oType, int oIdx);
void iterShadeOf(int nStep, int oType, int oIdx);

void posChg(int* x, int* y, int bx, int by, char chgRel);
void posFlip(int* x, int* y, int cx, int cy, char flipX);
void posRotate(int* x, int* y, int rAng, int cx, int cy, char rPoint);




int isMarkerSprite(int nSpr);

int reverseSectorPosition(int nSect, char flags = 0x03);
int countSpritesOfSector(int nSect);



int roundAngle(int nAng, int nAngles);
BOOL testXSectorForLighting(int nXSect);


void collectUsedChannels(unsigned char used[1024]);
char isIslandSector(int nSect);
char isNextWallOf(int nSrc, int nDest);
int findNextWall(int nWall, char forceSearch = 0);
int findNamedID(const char* str, NAMED_TYPE* pDb, int nLen);
spritetype* findPlayerStart(int nPlayer);
int words2flags(const char* str, NAMED_TYPE* pDb, int nLen);

char hasByte(BYTE* pBytes, int l, char nByte);
#endif