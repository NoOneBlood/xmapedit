/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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
#pragma once

#include "common_game.h"
#include "iob.h"

#define kMaxXSprites 			kMaxSprites
#define kMaxXWalls 				kMaxWalls
#define kMaxXSectors 			kMaxSectors

#define kXSectorDiskSize 		60
#define kXSpriteDiskSize 		56
#define kXWallDiskSize   		24

#define kMattID1    			0x4d617474  // Matt
#define kMattID2    			0x7474614d  // ttaM
#define kXMPHeadSig    			"XmP"
#define kXMPHeadVer     		1

#define kMonoCopyriteCRC 		0x77443AF8
#define kMonoCopyriteLength 	57

#pragma pack(push, 1)
struct AISTATE;

struct BLMMAGIC
{
    char sign[4];
    int16_t version;
};

struct BLMHEADER_MAIN
{
	int32_t startX;
	int32_t startY;
	int32_t startZ;
	int16_t startA;
	int16_t startS;
	int16_t skyBits;
	int32_t visibility;
	int32_t mattID;
	int8_t  skyType;
	int32_t revision;
	int16_t numsectors;
	int16_t numwalls;
	int16_t numsprites;
};

struct BLMHEADER_EXTRA
{
	char  pad[64]; 		// vanilla MAPEDIT writes a copyright here
	int32_t xsprSiz;
	int32_t xwalSiz;
	int32_t xsecSiz;
	// XMAPEDIT specific
	// ------------------
	char  	xmpsign[3];		// xmp signature
	int8_t  xmpheadver;		// xmp header version
	int8_t	xmpmapflags;	// & 0x01 = fixed sky bits number enabled so it won't be auto-adjusted
	// ------------------
	char  pad2[47];
};

struct XSPRITE {
    unsigned int unused1 : 2;           // additional dude flags in modern maps
    unsigned int unused2 : 1;           // unused
    unsigned int unused3 : 2;           // unused
    unsigned int unused4 : 6;           // unused

    signed   int reference : 15;
    unsigned int state : 1;
    unsigned int busy : 17;
    unsigned int txID : 10;
    unsigned int rxID : 10;
    unsigned int command : 8;
    unsigned int triggerOn : 1;
    unsigned int triggerOff : 1;
    unsigned int busyTime : 12;
    unsigned int waitTime : 12;
    unsigned int restState : 1;
    unsigned int interruptable : 1;

    unsigned int respawnPending : 2;    // respawnPending

    unsigned int dropItem : 8;           // Drop Item
    unsigned int decoupled : 1;         // decoupled
    unsigned int triggerOnce : 1;       // 1-shot
    unsigned int isTriggered : 1;       // works in case if triggerOnce selected

    unsigned int key : 3;               // key
    unsigned int wave : 2;              // Wave
    unsigned int triggerPush : 1;       // Push
    unsigned int triggerVector : 1;     // Vector
    unsigned int triggerImpact : 1;     // Impact
    unsigned int triggerPickup : 1;     // Pickup
    unsigned int triggerTouch : 1;      // Touch
    unsigned int triggerSight : 1;      // Sight
    unsigned int triggerProximity : 1;  // Proximity
    unsigned int lSkill : 5;            // Launch 12345
    unsigned int lS : 1;                // Single
    unsigned int lB : 1;                // Bloodbath
    unsigned int lT : 1;                // Launch Team
    unsigned int lC : 1;                // Coop
    unsigned int dudeLockout : 1;       // DudeLockout
    signed   int data1 : 16;            // Data 1
    signed   int data2 : 16;            // Data 2
    signed   int data3 : 16;            // Data 3
    unsigned int data4 : 16;            // Data 4
    unsigned int locked : 1;            // Locked
    unsigned int medium : 2;            // medium
    unsigned int respawn : 2;           // Respawn option
    unsigned int lockMsg : 8;           // Lock msg
    unsigned int health : 20;
    unsigned int dudeDeaf : 1;          // dudeDeaf
    unsigned int dudeAmbush : 1;        // dudeAmbush
    unsigned int dudeGuard : 1;         // dudeGuard
    unsigned int dudeFlag4 : 1;         // unused
    signed   int target : 16;           // target sprite
    signed   int targetX : 32;          // target x
    signed   int targetY : 32;          // target y
    signed   int targetZ : 32;          // target z
    unsigned int goalAng : 11;          // Dude goal ang
    signed   int dodgeDir : 2;          // Dude dodge direction
    unsigned int burnTime : 16;
    signed   int burnSource : 16;
    unsigned int height : 16;
    unsigned int stateTimer : 16;       // ai timer
    AISTATE* aiState;                   // ai
    signed int sysData1: 32;            // used to keep here various system data, so user can't change it in map editor
    signed int sysData2: 32;            //
    unsigned int physAttr : 32;         // currently used by additional physics sprites to keep it's attributes.
    signed int scale;                   // used for scaling SEQ size on sprites

};

struct XSECTOR {
    signed int reference : 14;
    unsigned int state : 1;             // State
    unsigned int busy : 17;
    unsigned int data : 16;             // Data
    unsigned int txID : 10;             // TX ID
    unsigned int rxID : 10;             // RX ID
    unsigned int busyWaveA : 3;         // OFF->ON wave
    unsigned int busyWaveB : 3;         // ON->OFF wave

    unsigned int command : 8;           // Cmd
    unsigned int triggerOn : 1;         // Send at ON
    unsigned int triggerOff : 1;        // Send at OFF
    unsigned int busyTimeA : 12;        // OFF->ON busyTime
    unsigned int waitTimeA : 12;        // OFF->ON waitTime
    unsigned int restState : 1;
    unsigned int interruptable : 1;     // Interruptable

    unsigned int reTriggerA : 1;        // OFF->ON wait
    unsigned int reTriggerB : 1;        // ON->OFF wait
    signed int amplitude : 8;           // Lighting amplitude
    unsigned int shadeFreq : 8;              // Lighting freq
    unsigned int shadePhase : 8;             // Lighting phase
    unsigned int shadeWave : 4;              // Lighting wave
    unsigned int shadeAlways : 1;       // Lighting shadeAlways
    unsigned int shadeFloor : 1;        // Lighting floor
    unsigned int shadeCeiling : 1;      // Lighting ceiling
    unsigned int shadeWalls : 1;        // Lighting walls
    signed int shade : 8;               // Lighting value
    unsigned int panAlways : 1;         // Pan always
    unsigned int panFloor : 1;          // Pan floor
    unsigned int panCeiling : 1;        // Pan ceiling
    unsigned int drag : 1;              // Pan drag
    unsigned int panVel : 8;            // Motion speed
    unsigned int panAngle : 11;         // Motion angle
    unsigned int underwater : 1;        // underwater
    unsigned int Depth : 3;             // Depth
    unsigned int unused1 : 1;
    unsigned int decoupled : 1;         // decoupled
    unsigned int triggerOnce : 1;       // 1-shot
    unsigned int isTriggered : 1;
    unsigned int key : 3;               // key
    unsigned int triggerPush : 1;              // Push
    unsigned int triggerVector : 1;            // Vector
    unsigned int triggerReserved : 1;          // Reserved
    unsigned int triggerEnter : 1;      // Enter
    unsigned int triggerExit : 1;       // Exit
    unsigned int triggerWallPush : 1;          // WallPush
    unsigned int coloredLights : 1;             // Color Lights
    unsigned int unused2 : 1;
    unsigned int busyTimeB : 12;        // ON->OFF busyTime
    unsigned int waitTimeB : 12;        // ON->OFF waitTime
    unsigned int stopOn : 1;
    unsigned int stopOff : 1;
    unsigned int ceilpal2 : 4;           // Ceil pal2
    signed int offCeilZ : 32;
    signed int onCeilZ : 32;
    signed int offFloorZ : 32;
    signed int onFloorZ : 32;
    signed int marker0 : 16;
    signed int marker1 : 16;
    unsigned int crush : 1;             // Crush
    unsigned int ceilXPanFrac : 8;      // Ceiling x panning frac
    unsigned int ceilYPanFrac : 8;      // Ceiling y panning frac
    unsigned int floorXPanFrac : 8;     // Floor x panning frac
    unsigned int damageType : 3;        // DamageType
    unsigned int floorpal2 : 4;          // Floor pal2
    unsigned int floorYPanFrac : 8;     // Floor y panning frac
    unsigned int locked : 1;            // Locked
    unsigned int windVel : 32;          // Wind vel (changed from 10 bit to use higher velocity values)
    unsigned int windAng : 11;          // Wind ang
    unsigned int windAlways : 1;        // Wind always
    unsigned int dudeLockout : 1;
    unsigned int bobTheta : 11;         // Motion Theta
    unsigned int bobZRange : 5;         // Motion Z range
    signed int bobSpeed : 12;           // Motion speed
    unsigned int bobAlways : 1;         // Motion always
    unsigned int bobFloor : 1;          // Motion bob floor
    unsigned int bobCeiling : 1;        // Motion bob ceiling
    unsigned int bobRotate : 1;         // Motion bobRotate
};

struct XWALL {
    signed int reference : 15;
    unsigned int state : 1;             // State
    unsigned int busy : 17;
    signed int data : 16;               // Data
    unsigned int txID : 10;             // TX ID
    unsigned int unused1 : 6;           // unused
    unsigned int rxID : 10;             // RX ID
    unsigned int command : 8;           // Cmd
    unsigned int triggerOn : 1;         // going ON
    unsigned int triggerOff : 1;        // going OFF
    unsigned int busyTime : 12;         // busyTime
    unsigned int waitTime : 12;         // waitTime
    unsigned int restState : 1;         // restState
    unsigned int interruptable : 1;     // Interruptable
    unsigned int panAlways : 1;         // panAlways
    signed   int panXVel : 8;           // panX
    signed   int panYVel : 8;           // panY
    unsigned int decoupled : 1;         // decoupled
    unsigned int triggerOnce : 1;       // 1-shot
    unsigned int isTriggered : 1;
    unsigned int key : 3;               // key 
    unsigned int triggerPush : 1;       // Push
    unsigned int triggerVector : 1;     // Vector
    unsigned int triggerTouch : 1;      // by NoOne: renamed from Reserved to Touch as it works with Touch now.
    unsigned int unused2 : 2;           // unused
    unsigned int xpanFrac : 8;          // x panning frac
    unsigned int ypanFrac : 8;          // y panning frac
    unsigned int locked : 1;            // Locked
    unsigned int dudeLockout : 1;       // DudeLockout
    unsigned int unused3 : 4;           // unused;
    unsigned int unused4 : 32;          // unused
};

struct SECTOR4 {
	uint16_t wallptr, wallnum;
	int8_t   ceilingstat, ceilingxpanning, ceilingypanning;
	int8_t   ceilingshade;
	int32_t  ceilingz;
	int16_t  ceilingpicnum, ceilingheinum;
	int8_t   floorstat, floorxpanning, floorypanning;
	int8_t   floorshade;
	int32_t  floorz;
	int16_t  floorpicnum, floorheinum;
	int32_t  tag;
};

struct WALL4 {
	int32_t x, y;
	int16_t point2;
	int8_t cstat;
	int8_t shade;
	uint8_t xrepeat, yrepeat, xpanning, ypanning;
	int16_t picnum, overpicnum;
	int16_t nextsector1, nextwall1;
	int16_t nextsector2, nextwall2;
	int32_t tag;
};

struct SPRITE4 {
	int32_t x, y, z;
	int8_t cstat, shade;
	uint8_t xrepeat, yrepeat;
	int16_t picnum, ang, xvel, yvel, zvel, owner;
	int16_t sectnum, statnum;
	int32_t tag;
	int8_t *extra;
};

struct SECTOR6 {
	uint16_t wallptr, wallnum;
	int16_t ceilingpicnum, floorpicnum;
	int16_t ceilingheinum, floorheinum;
	int32_t ceilingz, floorz;
	int8_t ceilingshade, floorshade;
	uint8_t ceilingxpanning, floorxpanning;
	uint8_t ceilingypanning, floorypanning;
	uint8_t ceilingstat, floorstat;
	uint8_t ceilingpal, floorpal;
	uint8_t visibility;
	int16_t lotag, hitag, extra;
};

struct WALL6 {
	int32_t x, y;
	int16_t point2, nextsector, nextwall;
	int16_t picnum, overpicnum;
	int8_t  shade;
	int8_t  pal;
	int16_t cstat;
	int8_t xrepeat, yrepeat, xpanning, ypanning;
	int16_t lotag, hitag, extra;
};

struct SPRITE6 {
	int32_t x, y, z;
	int16_t cstat;
	int8_t  shade;
	uint8_t pal, clipdist;
	uint8_t xrepeat, yrepeat;
	int8_t  xoffset, yoffset;
	int16_t picnum, ang, xvel, yvel, zvel, owner;
	int16_t sectnum, statnum;
	int16_t lotag, hitag, extra;
};
#pragma pack(pop)

extern int gMapRev, gSkyCount;
extern int gSuppMapVersions[5];
extern bool gModernMap, gCustomSkyBits;
extern unsigned short gStatCount[kMaxStatus + 1];
extern short numxsprites, numxwalls, numxsectors;

extern XSPRITE xsprite[kMaxXSprites];
extern XSECTOR xsector[kMaxXSectors];
extern XWALL xwall[kMaxXWalls];

extern int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];


void InsertSpriteSect(int nSprite, int nSector);
void RemoveSpriteSect(int nSprite);
void InsertSpriteStat(int nSprite, int nStat);
void RemoveSpriteStat(int nSprite);
void qinitspritelists(void);
int InsertSprite(int nSector, int nStat);
int qinsertsprite(short nSector, short nStat);
int DeleteSprite(int nSprite);
int qdeletesprite(short nSprite);
int ChangeSpriteSect(int nSprite, int nSector);
int qchangespritesect(short nSprite, short nSector);
int ChangeSpriteStat(int nSprite, int nStatus);
int qchangespritestat(short nSprite, short nStatus);
void InitFreeList(unsigned short *pList, int nCount);
void InsertFree(unsigned short *pList, int nIndex);
unsigned short dbInsertXSprite(int nSprite);
void dbDeleteXSprite(int nXSprite);
unsigned short dbInsertXWall(int nWall);
void dbDeleteXWall(int nXWall);
unsigned short dbInsertXSector(int nSector);
void dbDeleteXSector(int nXSector);
void dbXSpriteClean(void);
void dbXWallClean(void);
void dbXSectorClean(void);
void dbInit(void);
void PropagateMarkerReferences(void);

int dbCheckMap(IOBuffer* pIo, int* pSuppVerDB, int DBLen, BOOL* bloodMap);
int dbLoadMap(IOBuffer* pIo, char* cmtpath);
int dbLoadBuildMap(IOBuffer* pIo);
int dbSaveMap(char *filename, BOOL ver7);