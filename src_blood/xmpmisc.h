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
#include "db.h"

#define kSEQSig				"SEQ\x1A"
#define kQavSig				"QAV\x1A"
#define kBloodMapSig		"BLM\x1A"
#define kBakExt				".BAK"
#define kBakSignatureLength 32

#define PIX2MET(n) 				(n / (32<<4))
#define MET2PIX(n) 				(n * (32<<4))
#define IVAL2PERC(val, full) 	((val * 100) / full)
#define DELETE_AND_NULL(x) 		delete(x), x = NULL;


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
kMapStatDudes		= 0,
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

BOOL rngok(int val, int rngA, int rngB);
BOOL irngok(int val, int rngA, int rngB);
XSPRITE* GetXSpr(spritetype* pSpr);
XSECTOR* GetXSect(sectortype* pSect);
XWALL* GetXWall(walltype* pWall);


void nnExtOffsetPos(int oX, int oY, int oZ, int nAng, int* x, int* y, int* z);
class POSOFFS
{
	private:
		int bx, by, bz, ba;
	public:
		int x, y, z, a;
		//------------------------------------------------------------------------------------------
		POSOFFS(int aA, int aX, int aY, int aZ)		{ New(aA, aX, aY, aZ); }
		POSOFFS(int aA, int aX, int aY)				{ New(aA, aX, aY, 0); }
		POSOFFS() {};
		//------------------------------------------------------------------------------------------
		void New(int aA, int aX, int aY, int aZ)
		{
			x = bx = aX;
			y = by = aY;
			z = bz = aZ;
			a = ba = aA;
		}
		void Reset(char which = 0x0F)
		{
			if (which & 0x01) x = bx;
			if (which & 0x02) y = by;
			if (which & 0x04) z = bz;
			if (which & 0x08) a = ba;
		}
		void New(int aA, int aX, int aY)					{ New(aA, aX, aY, 0); }
		//------------------------------------------------------------------------------------------
		void Offset (int byX, int byY, int byZ)				{ nnExtOffsetPos(byX, byY, byZ, a, &x, &y, &z); }
		void Left(int nBy) 									{ Offset(-klabs(nBy), 0, 0); }
		void Right(int nBy)									{ Offset(klabs(nBy), 0, 0); }
		void Backward(int nBy)								{ Offset(0, -klabs(nBy), 0); }
		void Forward(int nBy)								{ Offset(0, klabs(nBy), 0); }
		void Up(int nBy)									{ Offset(0, 0, -klabs(nBy)); }
		void Down(int nBy)									{ Offset(0, 0, klabs(nBy)); }
		void Turn(int nAng)									{ a = (a + nAng) & kAngMask; }
		//------------------------------------------------------------------------------------------
		#if 0
		void Offset (int byX, int byY, int* aX, int* aY) 	{ Offset(byX, byY, 0); *aX = x; *aY = y; }
		void Left(int nBy, int* aX, int* aY)				{ Left(nBy); 		*aX = x; *aY = y; }
		void Right(int nBy, int* aX, int* aY)				{ Right(nBy);		*aX = x; *aY = y; }
		void Backward(int nBy, int* aX, int* aY)			{ Backward(nBy);	*aX = x; *aY = y; }
		void Forward(int nBy, int* aX, int* aY)				{ Forward (nBy);	*aX = x; *aY = y; }
		void Up(int nBy, int* aZ)							{ Up(nBy);			*aZ = z; }
		void Down(int nBy, int* aZ)							{ Down(nBy);		*aZ = z; }
		#endif
		//------------------------------------------------------------------------------------------
		int  Distance()										{ return exactDist(x - bx, y - by);}
		int  Angle()										{ return getangle(x - bx, y - by); }
};


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
	B2T_ENC_BREP	= 0x01,
	B2T_ENC_000F	= 0x02,
	B2T_ENC_MODS	= 0x04,
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
			uint32_t	len;
			uint8_t*	ptr;
		}
		bin;
		struct
		{
			int32_t		len;
			int32_t*	ptr;
			uint8_t		flg;
		}
		inf;
		struct
		{
			uint32_t	len;
			char*		ptr;
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

struct WALLPOINT_INFO
{
	int w, x, y;
};

struct OBJECT
{
	unsigned int type			: 8;
	unsigned int index			: 16;
};

struct LINE2D
{
	int x1, y1, x2, y2;
};


struct SCANWALL
{
	POSOFFS pos;
	short w, s;
};


struct SCANDIRENT
{
	char full[BMAX_PATH];
	char name[BMAX_PATH];
	char type[BMAX_PATH];
	unsigned int flags;
	unsigned int crc;
	signed   int extra;
	time_t mtime;
};
#pragma pack(pop)

class OBJECT_LIST
{
	private:
		OBJECT *db;
		unsigned int externalCount;
		unsigned int internalCount;
		void MarkEnd()
		{
			db[externalCount].type  = OBJ_NONE;
			db[externalCount].index = 0;
		}
		
		void Init()
		{
			db = (OBJECT*)malloc(sizeof(OBJECT));
			externalCount = 0, internalCount = 1;
			MarkEnd();
		}
		
		void Free()
		{
			if (db)
				free(db);
			
			externalCount = 0;
			internalCount = 0;
			db = NULL;
		}
	public:
		OBJECT_LIST()   		{ Init(); }
		~OBJECT_LIST()  		{ Free(); }
		OBJECT* Ptr()			{ return &db[0]; }
		OBJECT* Ptr(int nID)	{ return &db[nID]; }
		int Length()			{ return externalCount; }
		
		int Add(int nType, int nIndex, BOOL check = FALSE)
		{
			int retn = -1;
			if (check && (retn = Find(nType, nIndex)) >= 0)
				return retn;
			
			db = (OBJECT*)realloc(db, sizeof(OBJECT) * (internalCount + 1));
			dassert(db != NULL);

			db[externalCount].type  = nType;
			db[externalCount].index = nIndex;
			retn = externalCount;
			externalCount++;
			internalCount++;
			
			MarkEnd();
			return retn;
		}
		
		OBJECT* Remove(int nType, int nIndex)
		{
			int nID, t;
			if ((nID = Find(nType, nIndex)) < 0)
				return &db[externalCount];
			
			t = nID;
			while(t < externalCount) db[t] = db[t+1], t++;
			db = (OBJECT*)realloc(db, sizeof(OBJECT) * internalCount);
			dassert(db != NULL);
			externalCount--;
			internalCount--;
			return &db[nID];
		}
		
		int Find(int nType, int nIndex)
		{
			register int i;
			for (i = 0; i < externalCount; i++)
			{
				if (db[i].type == nType && db[i].index == nIndex)
					return i;
			}
			
			return -1;
		}
		
		void Clear() { Free(); Init(); }
		char Exists(int nType, int nIndex) { return (Find(nType, nIndex) >= 0); }
};

char scanWallOfSector(SCANWALL* pIn, SCANWALL* pOut);
extern NAMED_TYPE gReverseSectorErrors[3];
extern int gNextWall[kMaxWalls];


void Delay(int time);
BOOL Beep(BOOL cond);
void BeepOk( void );
void BeepFail( void );
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
BOOL objectLockShowStatus(int x, int y);
int getHighlightedObject();
char* getFilename(char* pth, char* out, BOOL addExt = FALSE);
char* getPath(char* pth, char* out, BOOL addSlash = FALSE);
void playSound(int sndId, int showMsg = 1);


void cpyObjectClear();
void doGridCorrection(int* x, int* y, int nGrid);
void doGridCorrection(int* x, int nGrid);
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
int setupSecrets();
char* array2str(NAMED_TYPE* in, int inLen, char* out, int outLen);
void removeExtension (char *str);
void pathSplit2(char *pzPath, char* buff, char** dsk, char** dir, char** fil, char** ext);
void resortFree();
void TranslateWallToSector( void );
BOOL chlRangeIsFine(int val);
short getSector();
inline BOOL slash(char ch) { return (ch == '\\' || ch == '/'); }
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

void getSectorWalls(int nSect, int* swal, int *ewal);
void avePointWall(int nWall, int* x, int* y);
BOOL isMultiTx(short nSpr);
BOOL multiTxGetRange(int nSpr, int out[4]);
BOOL multiTxPointsRx(int rx, short nSpr);
void setCstat(BOOL enable, short* pStat, int nStat);
short wallCstatAdd(int nWall, short cstat, BOOL nextWall = TRUE);
short wallCstatRem(int nWall, short cstat, BOOL nextWall = TRUE);
short wallCstatToggle(int nWall, short cstat, BOOL nextWall = TRUE);
short sectCstatAdd(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatRem(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatToggle(int nSect, short cstat, int objType = OBJ_FLOOR);
short sectCstatGet(int nSect, int objType = OBJ_FLOOR);
short sectCstatSet(int nSect, short cstat, int objType = OBJ_FLOOR);
void wallDetach(int nWall);
int perc2val(int reqPerc, int val);
void swapValues(int *nSrc, int *nDest);
void toggleResolution(int fs, int xres, int yres, int bpp = 8);
int camHitscan(short* hsc, short* hwl, short* hsp, int* hx, int* hy, int* hz, unsigned int clipMask);
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
void getWallCoords(int nWall, int* x1 = NULL, int* y1 = NULL, int* x2 = NULL, int* y2 = NULL);
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
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt = NULL, int* zb = NULL, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags = 0x07);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags = 0x07);
BOOL isSearchSector();
char removeQuotes(char* str);

void posChg(int* x, int* y, int bx, int by, char chgRel);
void posFlip(int* x, int* y, int cx, int cy, char flipX);
void posRotate(int* x, int* y, int rAng, int cx, int cy, char rPoint);

void loopGetEdgeWalls(int nFirst, int nLast, int* l, int* r, int* t, int* b);
void loopGetWalls(int nStartWall, int* swal, int *ewal);
void loopGetBox(int nFirst, int nLast, int* x1, int* y1, int* x2, int *y2);
void loopChgPos(int s, int e, int bx, int by, int flags);
void loopRotate(int s, int e, int cx, int cy, int nAng, int flags);
void loopFlip(int s, int e, int cx, int cy, int flags);
void loopDelete(int s, int e);

void midPointLoop(int nFirst, int nLast, int* ax, int *ay);
int getXYGrid(int x, int y, int min = 1, int max = 7);
void sectorDetach(int nSector);
void sectorAttach(int nSector);
int isMarkerSprite(int nSpr);
int getPrevWall(int nWall);
void avePointWall(int nWall, int* x, int* y, int* z);
double getWallLength(int nWall, int nGrid);
char sectorHasMoveRObjects(int nSect);
int revertValue(int nVal);
int reverseSectorPosition(int nSect, char flags = 0x03);
int countSpritesOfSector(int nSect);
void sectGetEdgeZ(int nSector, int* fz, int* cz);
void ceilGetEdgeZ(int nSector, int* zBot, int* zTop);
void floorGetEdgeZ(int nSector, int* zBot, int* zTop);
void sectRotate(int nSect, int cx, int cy, int nAng, int flags);
void sectFlip(int nSect, int cx, int cy, int flags, int);
int getSectorHeight(int nSector);
void flipWalls(int nStart, int nOffs);

void setFirstWall(int nSect, int nWall);
char loopInside(int nSect, POINT2D* pPoint, int nCount, char full);
int insertLoop(int nSect, POINT2D* pInfo, int nCount, walltype* pWModel = NULL, sectortype* pSModel = NULL);
void insertPoints(WALLPOINT_INFO* pInfo, int nCount);
int makeSquareSector(int ax, int ay, int area);
int redSectorCanMake(int nStartWall);
int redSectorMake(int nStartWall);
int redSectorMerge(int nThis, int nWith);

int roundAngle(int nAng, int nAngles);
BOOL testXSectorForLighting(int nXSect);


void collectUsedChannels(unsigned char used[1024]);
char isIslandSector(int nSect);
char isNextWallOf(int nSrc, int nDest);
int findNextWall(int nWall);
int findNamedID(const char* str, NAMED_TYPE* pDb, int nLen);
int words2flags(const char* str, NAMED_TYPE* pDb, int nLen);
char isDir(char* pPath);
char isFile(char* pPath);
SCANDIRENT* dirScan(char* path, char* filter, int* numfiles, int* numdirs, char flags = 0x0);
SCANDIRENT* driveScan(int* numdrives);
char hasByte(BYTE* pBytes, int l, char nByte);
char dirRemoveRecursive(char* path);
char* getCurDir(char* pth, char* out, char addSlash = 0);
char* getFiletype(char* pth, char* out, char addDot = 1);
void pathCatSlash(char* pth, int l = -1);
void pathRemSlash(char* pth, int l = -1);
char* getRelPath(char* relto, char* targt);

char insidePoints(int x, int y, POINT2D* point, int c);
char sectWallsInsidePoints(int nSect, POINT2D* point, int c);

//BOOL ss2obj(int* objType, int* objIdx, BOOL asIs = FALSE);
//BOOL dosboxRescan();
//void pathMake(char* out, char* dsk, char* dir, char* fil, char* ext);
#endif