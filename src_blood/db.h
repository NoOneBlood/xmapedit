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

#define kMaxXSprites kMaxSprites
#define kMaxXWalls kMaxWalls
#define kMaxXSectors kMaxSectors

extern int gSuppBuildMapVersion[];


#pragma pack(push, 1)
struct AISTATE;

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
    // #ifdef NOONE_EXTENSIONS
    // signed int sysData1: 32;            // used to keep here various system data, so user can't change it in map editor
    // signed int sysData2: 32;            //
    // unsigned int physAttr : 32;         // currently used by additional physics sprites to keep it's attributes.
    // #endif
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

struct MAPSIGNATURE {
    char signature[4];
    short version;
};

struct MAPHEADER  {
    int at0; // x
    int at4; // y
    int at8; // z
    short atc; // ang
    short ate; // sect
    short at10; // pskybits
    int at12; // visibility
    int at16; // song id, Matt
    char at1a; // parallaxtype
    int at1b; // map revision
    short at1f; // numsectors
    short at21; // numwalls
    short at23; // numsprites
};

struct MAPHEADER2 {
    char at0[64];
    int at40; // xsprite size
    int at44; // xwall size
    int at48; // xsector size
    char pad[52];
};

struct SPRITEHIT {
    int hit, ceilhit, florhit;
};

#pragma pack(pop)

extern unsigned short gStatCount[kMaxStatus + 1];

extern bool gModernMap, gCustomSkyBits;
extern bool gByte1A76C6, gByte1A76C7, gByte1A76C8;
extern short numxsprites, numxwalls, numxsectors;
extern MAPHEADER2 mapHead;

extern XSPRITE xsprite[kMaxXSprites];
extern XSECTOR xsector[kMaxXSectors];
extern XWALL xwall[kMaxXWalls];

extern SPRITEHIT gSpriteHit[kMaxXSprites];
extern int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];


extern int gMapRev, gSongId, gSkyCount;
extern const char *gItemText[];
extern const char *gAmmoText[];
extern const char *gWeaponText[];

extern unsigned short nextXSprite[kMaxXSprites];
extern unsigned short nextXWall[kMaxXWalls];
extern unsigned short nextXSector[kMaxXSectors];

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
unsigned int dbReadMapCRC(const char *pPath);
int dbLoadMap(char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector);
int dbSaveMap(char *pPath, int nX, int nY, int nZ, short nAngle, short nSector);
