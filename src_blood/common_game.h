/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Originally written by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// Various functions and global definitions for XMAPEDIT
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
#include "build.h"
#include "baselayer.h"
#include "cache1d.h"
#include "pragmas.h"
#include "resource.h"
#ifdef _MSC_VER
# include "msinttypes/stdint.h"
# include "msinttypes/inttypes.h"
#else
# include <stdint.h>
# include <inttypes.h>
# include <limits.h>
#endif

void _SetErrorLoc(const char* pzFile, int nLine);
void _ThrowError(const char* pzFormat, ...);
void __dassert(const char* pzExpr, const char* pzFile, int nLine);

#define ThrowError(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_ThrowError(__VA_ARGS__); \
	}

// print error to console only
#define consoleSysMsg(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_consoleSysMsg(__VA_ARGS__); \
	}

#define dassert(x) if (!(x)) __dassert(#x,__FILE__,__LINE__)

#ifndef fallthrough__
# if defined _MSC_VER
#  define fallthrough__ __fallthrough
# else
#  define fallthrough__
# endif
#endif

#define kMaxSectors MAXSECTORS
#define kMaxWalls MAXWALLS
#define kMaxSprites MAXSPRITES

#define kMaxTiles MAXTILES
#define kMaxStatus MAXSTATUS
#define kMaxViewSprites MAXSPRITESONSCREEN

#define kMaxVoxels MAXVOXELS

#define kMaxPlayers 8
#define MAXVOXMIPS 5


extern "C" {
extern unsigned char picsiz[MAXTILES], tilefilenum[MAXTILES];
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern int vplce[4], vince[4];
extern intptr_t palookupoffse[4], bufplce[4];
extern int hitscangoalx, hitscangoaly;
extern int pow2long[32];
extern int tilefileoffs[MAXTILES];
}

typedef uint8_t			BYTE;
typedef uint8_t			BOOL;

typedef int8_t	 		schar;
typedef uint16_t		ushort;


typedef signed char		int8;
typedef signed short	int16;
typedef signed int		int32;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;

#define TRUE			1
#define FALSE			0

#define kMaxPlayers 8

#define kVanillaMaxXSprites 2048
#define kVanillaMaxXWalls   512
#define kVanillaMaxXSectors 512

#define kVanillaMaxSprites 4096
#define kVanillaMaxWalls 8192
#define kVanillaMaxSectors 1024

#define LENGTH(x) 			(sizeof(x) / sizeof(x[0]))
#define BLOCK_NONE			0
#define BLOCK_MOVE			((kSprBlock << 16) | kWallBlock)
#define BLOCK_PROJECTILE	((kSprHitscan << 16) | kWallHitscan)
#define BLOCK_HITSCAN		((kSprHitscan << 16) | kWallHitscan)
#define BLOCK_PUSH			((kSprPush << 16) | kWallHitscan)

#define kAng5 28
#define kAng15 85
#define kAng30 170
#define kAng45 256
#define kAng60 341
#define kAng90 512
#define kAng120 682
#define kAng180 1024
#define kAng360 2048
#define kAngMask 2047

#define kPluAny -1
#define kPlu0 0
#define kPlu1 1
#define kPlu2 2
#define kPlu3 3
#define kPlu4 4
#define kPlu5 5
#define kPlu6 6
#define kPlu7 7
#define kPlu8 8
#define kPlu9 9
#define kPlu10 10
#define kPlu11 11
#define kPlu12 12
#define kPlu13 13
#define kPlu14 14
#define kPluMax	MAXPALOOKUPS

#define kPal0 0
#define kPal1 1
#define kPal2 2
#define kPal3 3
#define kPalMax kPal3 + 1

#define kPatNormal 0xFFFFFFFF
#define kPatDotted 0xCCCCCCCC
#define kPatDashed 0xFFFF


// searchstat types /////////////////////////////////////////////////////
enum {
OBJ_PALOOKUP				= -3,	// for tilePick()
OBJ_CUSTOM					= -2,	// for tilePick()
OBJ_ALL						= -1,	// for tilePick()
OBJ_WALL					= 0,
OBJ_CEILING					= 1,
OBJ_FLOOR					= 2,
OBJ_SPRITE					= 3,
OBJ_MASKED					= 4,
OBJ_FLATSPRITE				= 5,	// for tilePick()
OBJ_SECTOR					= 6,	// for gSearchStatNames
OBJ_COMMENT					= 7,
};

// rotatesprite flags /////////////////////////////////////////////////////
enum {
kRSNone						= 0x00,
kRSTransluc					= 0x01,
kRSScale					= 0x02,
kRSYFlip					= 0x04,
kRSNoClip					= 0x08,
kRSStatus					= 0x0A,
kRSCorner					= 0x10,
kRSTranslucR				= 0x20,
kRSTransluc2				= 0x21,
kRSNoMask					= 0x40,
};

// standard 16 colors /////////////////////////////////////////////////////
enum {
kColorBlack					= 0,
kColorBlue					= 1,
kColorGreen					= 2,
kColorCyan					= 3,
kColorRed					= 4,
kColorMagenta				= 5,
kColorBrown					= 6,
kColorLightGray				= 7,
kColorDarkGray				= 8,
kColorLightBlue				= 9,
kColorLightGreen			= 10,
kColorLightCyan				= 11,
kColorLightRed				= 12,
kColorLightMagenta			= 13,
kColorYellow				= 14,
kColorWhite					= 15,
};

// sector cstat /////////////////////////////////////////////////////
enum {
kSectParallax				= 0x01,
kSectSloped					= 0x02,
kSectExpand   				= 0x08,
kSectFlipX    				= 0x10,
kSectFlipY    				= 0x20,
kSectFlipMask				= 0x34,
kSectRelAlign 				= 0x40,
kSectMasked					= 0x100,
kSectShadeFloor				= 0x8000,
};

// wall cstat /////////////////////////////////////////////////////
enum {
kWallBlock    				= 0x0001,
kWallSwap  					= 0x0002,
kWallOrgBottom   			= 0x0004,
kWallOrgOutside   			= 0x0004,
kWallFlipX       			= 0x0008,
kWallMasked      			= 0x0010,
kWallOneWay      			= 0x0020,
kWallHitscan       			= 0x0040,
kWallTransluc				= 0x0080,
kWallFlipY					= 0x0100,
kWallFlipMask				= 0x0108,
kWallTranslucR				= 0x0200,
kWallTransluc2				= 0x0280,
kWallMoveMask				= 0xC000,
kWallMoveNone				= 0x0000,
kWallMoveForward			= 0x4000,
kWallMoveReverse			= 0x8000,
};

// sprite cstat /////////////////////////////////////////////////////
enum {
kSprBlock    				= 0x0001,
kSprTransluc1 				= 0x0002,
kSprFlipX       			= 0x0004,
kSprFlipY       			= 0x0008,
kSprFace        			= 0x0000,
kSprWall        			= 0x0010,
kSprFloor       			= 0x0020,
kSprVoxel					= 0x0030,
kSprRelMask					= 0x0030,
kSprSloped					= 0x0030,
kSprOneSided    			= 0x0040,
kSprOrigin					= 0x0080,
kSprHitscan      			= 0x0100,
kSprTranslucR				= 0x0200,
kSprTransluc2				= 0x0202,
kSprPush					= 0x1000,
kSprMoveMask				= 0x6000,
kSprMoveNone				= 0x0000,
kSprMoveForward				= 0x2000,
kSprMoveReverse				= 0x4000,
kSprInvisible    			= 0x8000,
};

// sprite view types /////////////////////////////////////////////////////
enum {
kSprViewSingle 				= 0,
kSprViewFull5				= 1,
kSprViewFull8				= 2,
kSprViewBounce				= 3,
kSprViewVox					= 6,
kSprViewVoxSpin				= 7,
};

// sprite attributes (hitag) /////////////////////////////////////////////////////
enum {
kPhysMove 					= 0x0001, 
kPhysGravity				= 0x0002,
kPhysFalling				= 0x0004,	
kHitagAutoAim				= 0x0008,
kHitagRespawn				= 0x0010,
kHitagFree					= 0x0020,
kHitagSmoke					= 0x0100,
};

// game types /////////////////////////////////////////////////////
enum {
kGameModeSingle				= 0,
kGameModeCoop				= 1,
kGametModeDeathmatch		= 2,
kGameModeFlags				= 3,
};


// MEDIUM /////////////////////////////////////////////////////
enum {
kMediumNormal                   = 0,
kMediumWater                    = 1,
kMediumGoo                      = 2,
};

// STATNUMS /////////////////////////////////////////////////////
enum {
kStatNothing                    = -1,
kStatDecoration                 = 0,
kStatFX                         = 1,
kStatExplosion                  = 2,
kStatItem                       = 3,
kStatThing                      = 4,
kStatProjectile                 = 5,
kStatDude                       = 6,
kStatInactive                   = 7, // inactive (ambush) dudes
kStatRespawn                    = 8,
kStatPurge                      = 9,
kStatMarker                     = 10,
kStatTraps                      = 11,
kStatAmbience                   = 12,
kStatSpares                     = 13,
kStatFlare                      = 14,
kStatDebris                     = 15,
kStatPathMarker                 = 16,
kStatFree                       = 1024,
};

// POWERUPS /////////////////////////////////////////////////////
enum {
kPwUpFeatherFall        = 12,
kPwUpShadowCloak        = 13,
kPwUpDeathMask          = 14,
kPwUpJumpBoots          = 15,
kPwUpTwoGuns            = 17,
kPwUpDivingSuit         = 18,
kPwUpGasMask            = 19,
kPwUpCrystalBall        = 21,
kPwUpDoppleganger       = 23,
kPwUpReflectShots       = 24,
kPwUpBeastVision        = 25,
kPwUpShadowCloakUseless = 26,
kPwUpDeliriumShroom     = 28,
kPwUpGrowShroom         = 29,
kPwUpShrinkShroom       = 30,
kPwUpDeathMaskUseless   = 31,
kPwUpAsbestArmor        = 39,
kMaxPowerUps            = 51,
};

enum {
    kExplosionSmall = 0,
    kExplosionStandard = 1,
    kExplosionLarge = 2,
    kExplosionFireball = 3,
    kExplosionSpray = 4,
    kExplosion5 = 5,
    kExplosion6 = 6,
    kExplosionNapalm = 7,
    kExplosionMax = 8
};

// SPRITE TYPES /////////////////////////////////////////////////
enum {
    kSpriteDecoration 	= 0,
	kDecoration		  	= 0,
    // markers
    kMarkerSPStart 		= 1,
    kMarkerMPStart		= 2,
    kMarkerOff 			= 3,
    kMarkerOn 			= 4,
    kMarkerAxis			= 5,
    kMarkerLowLink		= 6,
    kMarkerUpLink		= 7,
    kMarkerWarpDest		= 8,
    kMarkerUpWater 		= 9,
    kMarkerLowWater		= 10,
    kMarkerUpStack		= 11,
    kMarkerLowStack 	= 12,
    kMarkerUpGoo 		= 13,
    kMarkerLowGoo 		= 14,
    kMarkerPath 		= 15,
    kMarkerDudeSpawn 	= 18,
    kMarkerEarthquake 	= 19,

    // switches
    kSwitchBase 		= 20,
    kSwitchToggle 		= 20,
    kSwitchOneWay 		= 21,
    kSwitchCombo 		= 22,
    kSwitchPadlock 		= 23,
    kSwitchMax 			= 24,

    // decorations
    kDecorationTorch 	= 30,
    kDecorationCandle 	= 32,

    // (weapons)
    kItemWeaponBase 	= 40,
    kItemWeaponRandom 	= kItemWeaponBase,
    kItemWeaponSawedoff = 41,
    kItemWeaponTommygun = 42,
    kItemWeaponFlarePistol = 43,
    kItemWeaponVoodooDoll = 44,
    kItemWeaponTeslaCannon = 45,
    kItemWeaponNapalmLauncher = 46,
    kItemWeaponPitchfork = 47,
    kItemWeaponSprayCan = 48,
    kItemWeaponTNT = 49,
    kItemWeaponLifeLeech = 50,
    kItemWeaponMax = 51,

    // items (ammos)
    kItemAmmoBase = 60,
    kItemAmmoSprayCan = kItemAmmoBase,
    kItemAmmoTNTBundle = 62,
    kItemAmmoTNTBox = 63,
    kItemAmmoProxBombBundle = 64,
    kItemAmmoRemoteBombBundle = 65,
    kItemAmmoTrappedSoul = 66,
    kItemAmmoSawedoffFew = 67,
    kItemAmmoSawedoffBox = 68,
    kItemAmmoTommygunFew = 69,
    kItemAmmoVoodooDoll = 70,
    kItemAmmoTommygunDrum = 72,
    kItemAmmoTeslaCharge = 73,
    kItemAmmoFlares = 76,
    kItemAmmoGasolineCan = 79,
    kItemAmmoMax = 81,

    kItemBase = 100,
    
    // items (keys)
    kItemKeyBase = kItemBase,
    kItemKeySkull = kItemKeyBase,
    kItemKeyEye = 101,
    kItemKeyFire = 102,
    kItemKeyDagger = 103,
    kItemKeySpider = 104,
    kItemKeyMoon = 105,
    kItemKey7 = 106,
    kItemKeyMax = 107,

    // items (health)
    kItemHealthDoctorBag = 107,
    kItemHealthMedPouch = 108,
    kItemHealthLifeEssense = 109,
    kItemHealthLifeSeed = 110,
    kItemHealthRedPotion = 111,

    // items (misc)
    kItemFeatherFall = 112,
    kItemShadowCloak = 113, // ltdInvisibility
    kItemDeathMask = 114, // invulnerability
    kItemJumpBoots = 115,
    kItemTwoGuns = 117,
    kItemDivingSuit = 118,
    kItemGasMask = 119,
    kItemCrystalBall = 121,
	kItemDoppleganger	= 123,
    kItemReflectShots = 124,
    kItemBeastVision = 125,
    kItemShroomRage		= 127,
	kItemShroomDelirium = 128,
	kItemShroomGrow		= 129,
	kItemShroomShrink	= 130,
	kItemTome			= 136,
	kItemBlackChest		= 137,
	kItemWoodenChest	= 138,
    kItemArmorAsbest = 139,
    kItemArmorBasic = 140,
    kItemArmorBody = 141,
    kItemArmorFire = 142,
    kItemArmorSpirit = 143,
    kItemArmorSuper = 144,

    kItemFlagABase = 145,
    kItemFlagBBase = 146,
    kItemFlagA = 147,
    kItemFlagB = 148,
    kItemMax = 151,

    // dudes
    kDudeBase = 200,
    kDudeCultistTommy = 201,
    kDudeCultistShotgun = 202,
    kDudeZombieAxeNormal = 203,
    kDudeZombieButcher = 204,
    kDudeZombieAxeBuried = 205,
    kDudeGargoyleFlesh = 206,
    kDudeGargoyleStone = 207,
    kDudeGargoyleStatueFlesh = 208,
    kDudeGargoyleStatueStone = 209,
    kDudePhantasm = 210,
    kDudeHellHound = 211,
    kDudeHand = 212,
    kDudeSpiderBrown = 213,
    kDudeSpiderRed = 214,
    kDudeSpiderBlack = 215,
    kDudeSpiderMother = 216,
    kDudeGillBeast = 217,
    kDudeBoneEel = 218,
    kDudeBat = 219,
    kDudeRat = 220,
    kDudePodGreen = 221,
    kDudeTentacleGreen = 222,
    kDudePodFire = 223,
    kDudeTentacleFire = 224,
    kDudePodMother = 225,
    kDudeTentacleMother = 226,
    kDudeCerberusTwoHead = 227,
    kDudeCerberusOneHead = 228,
    kDudeTchernobog = 229,
    kDudeCultistTommyProne = 230,
    kDudePlayer1 = 231,
    kDudePlayer2 = 232,
    kDudePlayer3 = 233,
    kDudePlayer4 = 234,
    kDudePlayer5 = 235,
    kDudePlayer6 = 236,
    kDudePlayer7 = 237,
    kDudePlayer8 = 238,
    kDudeBurningInnocent = 239,
    kDudeBurningCultist = 240,
    kDudeBurningZombieAxe = 241,
    kDudeBurningZombieButcher = 242,
    kDudeCultistReserved = 243, // unused
    kDudeZombieAxeLaying = 244,
    kDudeInnocent = 245,
    kDudeCultistShotgunProne = 246,
    kDudeCultistTesla = 247,
    kDudeCultistTNT = 248,
    kDudeCultistBeast = 249,
    kDudeTinyCaleb = 250,
    kDudeBeast = 251,
    kDudeBurningTinyCaleb = 252,
    kDudeBurningBeast = 253,
    kDudeVanillaMax = 254,
    kDudeMax = 256,
    
    kMissileBase = 300,
    kMissileButcherKnife = kMissileBase,
    kMissileFlareRegular = 301,
    kMissileTeslaAlt = 302,
    kMissileFlareAlt = 303,
    kMissileFlameSpray = 304,
    kMissileFireball = 305,
    kMissileTeslaRegular = 306,
    kMissileEctoSkull = 307,
    kMissileFlameHound = 308,
    kMissilePukeGreen = 309,
    kMissileUnused = 310,
    kMissileArcGargoyle = 311,
    kMissileFireballNapalm = 312,
    kMissileFireballCerberus = 313,
    kMissileFireballTchernobog = 314,
    kMissileLifeLeechRegular = 315,
    kMissileLifeLeechAltNormal = 316,
    kMissileLifeLeechAltSmall = 317,
    kMissileMax = 318,

    // things
    kThingBase = 400,
    kThingTNTBarrel = 400,
    kThingArmedProxBomb = 401,
    kThingArmedRemoteBomb = 402,
    kThingCrateFace = 405,
    kThingGlassWindow = 406,
    kThingFluorescent = 407,
    kThingWallCrack = 408,
	kThingWoodBeam	= 409,
    kThingSpiderWeb = 410,
    kThingMetalGrate = 411,
    kThingFlammableTree = 412,
    kTrapMachinegun = 413, // not really a thing, should be in traps instead
    kThingFallingRock = 414,
    kThingKickablePail = 415,
    kThingObjectGib = 416,
    kThingObjectExplode = 417,
    kThingArmedTNTStick = 418,
    kThingArmedTNTBundle = 419,
    kThingArmedSpray = 420,
    kThingBone = 421,
    kThingDripWater = 423,
    kThingDripBlood = 424,
    kThingBloodBits = 425,
    kThingBloodChunks = 426,
    kThingZombieHead = 427,
    kThingNapalmBall = 428,
    kThingPodFireBall = 429,
    kThingPodGreenBall = 430,
    kThingDroppedLifeLeech = 431,
    kThingVoodooHead = 432, // unused
    kThingMax = 436,

    // traps
    kTrapBase			= 450,
	kTrapFlame 			= 452,
    kTrapSawCircular 	= 454,
    kTrapZapSwitchable 	= 456,
	kTrapPendulum		= 457,
	kTrapGuillotine		= 458,
    kTrapExploder 		= 459,
	kTrapMax				 ,

    // generators
    kGenTrigger = 700,
    kGenDripWater = 701,
    kGenDripBlood = 702,
    kGenMissileFireball = 703,
    kGenMissileEctoSkull = 704,
    kGenDart = 705,
    kGenBubble = 706,
    kGenBubbleMulti = 707,
    
    // sound sprites
    kGenSound = 708,
    kSoundSector = 709,
	kSoundAmbient = 710,
    kSoundPlayer = 711,
};

// WALL TYPES /////////////////////////////////////////////////
enum {
    kWallBase = 500,
    kWallStack = 501,
    kWallGib = 511,
    kWallMax = 512,
};


// SECTOR TYPES /////////////////////////////////////////////////
enum {
    kSectorBase = 600,
    kSectorZMotion = 600,
    kSectorZMotionSprite = 602,
    kSectorTeleport = 604,
    kSectorPath = 612,
    kSectorRotateStep = 613,
    kSectorSlideMarked = 614,
    kSectorRotateMarked = 615,
    kSectorSlide = 616,
    kSectorRotate = 617,
    kSectorDamage = 618,
    kSectorCounter = 619,
    kSectorMax = 620,
};

extern int gFrameClock;
extern int gFrameTicks;
extern int gFrame;
extern int gFrameRate;
extern int gGamma;
extern Resource gSysRes;

struct PICANM {
	unsigned frames 	: 5;
	unsigned update 	: 1;
	unsigned type 		: 2;
	signed xcenter 		: 8;
	signed ycenter 		: 8;
  	unsigned speed 		: 4;
	unsigned view 		: 3;
	unsigned filler 	: 1;
};

extern PICANM* panm;

struct VECTOR2D {
    int dx, dy;
};

struct POINT2D {
    int x, y;
};

struct POINT3D {
    int x, y, z;
};

inline int scale(int a1, int a2, int a3, int a4, int a5)
{
    return a4 + (a5-a4) * (a1-a2) / (a3-a2);
}

inline int IncBy(int a, int b)
{
    a += b;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline int DecBy(int a, int b)
{
    a--;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline int IncRotate(int n, int mod)
{
	if (++n >= mod)
		n = 0;
	return n;
}

inline int DecRotate(int n, int mod)
{
	if (--n < 0)
		n += mod;
	return n;
}

inline int IncNext(int n, int mod)
{
	return (n + mod) & ~(mod - 1);
}

inline int DecNext(int n, int mod)
{
	return (n - 1) & ~(mod - 1);
}

inline int ClipLow(int a, int b)
{
    if (a < b)
        return b;
    return a;
}

inline int ClipHigh(int a, int b)
{
    if (a >= b)
        return b;
    return a;
}

inline int ClipRange(int a, int b, int c)
{
    if (a < b)
        return b;
    if (a > c)
        return c;
    return a;
}

inline int interpolate(int a, int b, int c)
{
    return a+mulscale16(b-a,c);
}

inline void SetBitString(unsigned char *pArray, int nIndex)
{
    pArray[nIndex>>3] |= 1<<(nIndex&7);
}

inline void ClearBitString(unsigned char *pArray, int nIndex)
{
    pArray[nIndex >> 3] &= ~(1 << (nIndex & 7));
}

inline char TestBitString(unsigned char *pArray, int nIndex)
{
    return pArray[nIndex>>3] & (1<<(nIndex&7));
}

inline int dmulscale30r(int a, int b, int c, int d)
{
    int64_t acc = 1<<(30-1);
    acc += ((int64_t)a) * b;
    acc += ((int64_t)c) * d;
    return (int)(acc>>30);
}

inline int mulscale16r(int a, int b)
{
    int64_t acc = 1<<(16-1);
    acc += ((int64_t)a) * b;
    return (int)(acc>>16);
}

inline int mulscale30r(int a, int b)
{
    int64_t acc = 1<<(30-1);
    acc += ((int64_t)a) * b;
    return (int)(acc>>30);
}

inline int approxDist(int dx, int dy)
{
    dx = klabs(dx);
    dy = klabs(dy);
    if (dx > dy)
        dy = (3*dy)>>3;
    else
        dx = (3*dx)>>3;
    return dx+dy;
}

inline char Chance(int a1)
{
    return rand() < (a1>>1);
}

inline unsigned int Random(int a1)
{
    return mulscale15(rand(), a1);
}

inline int BiRandom(int a1)
{
    return mulscale14(rand(), a1)-a1;
}

template<typename T> void GetSpriteExtents(T const * const pSprite, int *top, int *bottom)
{
    *top = *bottom = pSprite->z;
	if ((pSprite->cstat & kSprRelMask) == kSprFloor)
		return;
	

	int nTile = pSprite->picnum;
	if (pSprite->cstat & kSprOrigin)
	{
		int height = tilesizy[nTile];
        int center = height / 2 + panm[nTile].ycenter;
        *top -= (pSprite->yrepeat << 2)*center;
        *bottom += (pSprite->yrepeat << 2)*(height - center);
	}
	else
	{
		*top -= (panm[nTile].ycenter + tilesizy[nTile]) * (pSprite->yrepeat << 2);
	}

}

struct Point
{
	int x, y;
	Point(int x, int y) : x(x), y(y) {};
	Point& operator+=( const Point& p ) { x += p.x; y += p.y; return *this; }
	Point& operator-=( const Point& p ) { x -= p.x; y -= p.y; return *this; }
	bool operator==( const Point& p ) const { return x == p.x && y == p.y; }
};

class Rect {
public:
    int x0, y0, x1, y1;
    Rect(int _x0, int _y0, int _x1, int _y1)
    {
        x0 = _x0; y0 = _y0; x1 = _x1; y1 = _y1;
    }
    bool isValid(void) const
    {
        return x0 < x1 && y0 < y1;
    }
    char isEmpty(void) const
    {
        return !isValid();
    }
    bool operator!(void) const
    {
        return isEmpty();
    }

    Rect & operator&=(Rect &pOther)
    {
        x0 = ClipLow(x0, pOther.x0);
        y0 = ClipLow(y0, pOther.y0);
        x1 = ClipHigh(x1, pOther.x1);
        y1 = ClipHigh(y1, pOther.y1);
        return *this;
    }

    void offset(int dx, int dy)
    {
        x0 += dx;
        y0 += dy;
        x1 += dx;
        y1 += dy;
    }

    int height()
    {
        return y1 - y0;
    }

    int width()
    {
        return x1 - x0;
    }

    bool inside(Rect& other)
    {
        return (x0 <= other.x0 && x1 >= other.x1 && y0 <= other.y0 && y1 >= other.y1);
    }

    bool inside(int x, int y)
    {
        return (x0 <= x && x1 > x && y0 <= y && y1 > y);
    }
};

class BitReader {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitReader(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; nSize -= nBitPos>>3; }
    BitReader(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; }
    int readBit()
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        int bit = ((*pBuffer)>>nBitPos)&1;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
        return bit;
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    unsigned int readUnsigned(int nBits)
    {
        unsigned int n = 0;
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            n += readBit()<<i;
        return n;
    }
    int readSigned(int nBits)
    {
        dassert(nBits <= 32);
        int n = (int)readUnsigned(nBits);
        n <<= 32-nBits;
        n >>= 32-nBits;
        return n;
    }
};

class BitWriter {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitWriter(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; memset(pBuffer, 0, nSize); nSize -= nBitPos>>3; }
    BitWriter(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; memset(pBuffer, 0, nSize); }
    void writeBit(int bit)
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        *pBuffer |= bit<<nBitPos;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    void write(int nValue, int nBits)
    {
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            writeBit((nValue>>i)&1);
    }
};

