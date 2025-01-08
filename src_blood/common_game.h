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
#include <direct.h>
#include <cmath>
#include "build.h"
#include "baselayer.h"
#include "cache1d.h"
#include "pragmas.h"
#include "resource.h"
#include "editor.h"
#include "crc32.h"
#include "compat.h"
#include "replace.h"
#include "inifile.h"
#include "osd.h"
#include "iob.h"
#include "xmpstr.h"
extern "C" {
#include "a.h"
}

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



#if _MSC_VER <= 1310
#define ThrowError _SetErrorLoc(__FILE__, __LINE__), _ThrowError
#else
#define ThrowError(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_ThrowError(__VA_ARGS__); \
	}
#endif

#define dassert(x) if (!(x)) __dassert(#x,__FILE__,__LINE__)

#ifndef fallthrough__
# if defined _MSC_VER
#	if _MSC_VER <= 1310
#		define fallthrough__
#	else
#  		define fallthrough__ __fallthrough
#	endif
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
#define kMaxVisibility 0x20000


extern "C" {
extern unsigned char picsiz[MAXTILES], tilefilenum[MAXTILES];
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern int vplce[4], vince[4];
extern intptr_t palookupoffse[4], bufplce[4];
extern int hitscangoalx, hitscangoaly;
extern int pow2long[32];
extern int tilefileoffs[MAXTILES];
extern int desktopxdim,desktopydim,desktopbpp,desktopfreq,desktopmodeset;

#if USE_OPENGL
	extern int glswapinterval;
	extern int set_glswapinterval(const osdfuncparm_t *parm);
	extern unsigned char nogl;
#endif
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

typedef uint8_t BITARRAY32[(32768+7)>>3];
typedef uint8_t BITARRAY16[(16384+7)>>3];
typedef uint8_t BITARRAY08[(8192+7)>>3];
typedef uint8_t BITARRAY04[(4096+7)>>3];
typedef uint8_t BITSECTOR[(kMaxSectors+7)>>3];
typedef uint8_t BITSPRITE[(kMaxSprites+7)>>3];

#define kSEQSig					"SEQ\x1A"
#define kQavSig					"QAV\x1A"
#define kBloodMapSig			"BLM\x1A"
#define kNBloodSaveSig			"NBSV"

#define DANGLE(a1, a2)  		(((a1 + kAng180 - a2) & kAngMask) - kAng180)
#define EVTIME2TICKS(x)         ((x * 120) / 10)
#define PIX2MET(n) 				(n / (32<<4))
#define MET2PIX(n) 				(n * (32<<4))
#define GRD2PIX(n)				(2048 >> n)
#define IVAL2PERC(val, full) 	((val * 100) / full)
#define DELETE_AND_NULL(x) 		delete(x), x = NULL;
#define FREE_AND_NULL(x)		free(x), x = NULL;
#define MIDPOINT(a, b) 			(a+((b-a)>>1))
#define QSETMODE(x, y)			qsetmode = ((x<<16)|(y&0xffff))
#define NUMPALOOKUPS(x)			(numpalookups-x)
#define ALIGNTO(s)				(sector[s].wallptr + sector[s].alignto)


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

#define kPatNormal		0xFFFFFFFF
#define kPatDotted		0xCCCCCCCC
#define kPatDotted2		0xBBBBBBBB
#define kPatDashed		0xFFFF

#define kTicRate 120
#define kTicsPerFrame 4
#define kTicsPerSec (kTicRate/kTicsPerFrame)

#define HITMASK			0xC000
#define HITID			0x3FFF
#define HITSECT			0x4000
#define HITWALL			0x8000
#define HITSPR			0xC000

#define CHK_HIT(a, b)			((a & HITMASK) == b)
#define GET_HITID(a)			(a & HITID)
#define CMP_HITID(a, b)			(GET_HITID(a) == b)

#define CHK_HITSECT(a)			CHK_HIT(a, HITSECT)
#define CMP_HITSECT(a, b)		(CHK_HITSECT(a) && CMP_HITID(a, b))

#define CHK_HITWALL(a)			CHK_HIT(a, HITWALL)
#define CMP_HITWALL(a, b)		(CHK_HITWALL(a) && CMP_HITID(a, b))

#define CHK_HITSPR(a)			CHK_HIT(a, HITSPR)
#define CMP_HITSPR(a, b)		(CHK_HITSPR(a) && CMP_HITID(a, b))

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
OBJ_NONE					= 255,
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
kColorGrey16				= 16,
kColorGrey17				= 17,
kColorGrey18				= 18,
kColorGrey19				= 19,
kColorGrey20				= 20,
kColorGrey21				= 21,
kColorGrey22				= 22,
kColorGrey23				= 23,
kColorGrey24				= 24,
kColorGrey25				= 25,
kColorGrey26				= 26,
kColorGrey27				= 27,
kColorGrey28				= 28,
kColorGrey29				= 29,
kColorGrey30				= 30,
kColorGrey31				= 31,
};

// sector cstat /////////////////////////////////////////////////////
enum {
kSectParallax				= 0x01,
kSectSloped					= 0x02,
kSectSwapXY					= 0x04,
kSectExpand   				= 0x08,
kSectFlipX    				= 0x10,
kSectFlipY    				= 0x20,
kSectFlipMask				= 0x34,
kSectRelAlign 				= 0x40,
kSectMasked					= 0x80,
kSectTransluc				= 0x100,
kSectTranslucR				= 0x180,
kSectTransluc2				= 0x180,
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
kGameModeMax,
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

enum
{
ALG_LEFT			= 0x00,
ALG_TOP				= 0x00,
ALG_CENTER			= 0x01,
ALG_MIDDLE			= 0x02,
ALG_RIGHT			= 0x04,
ALG_BOTTOM			= 0x08,
};

#define kMaxObjectType 1024

extern int gFrameClock;
extern int gFrameTicks;
extern int gFrame;
extern int gFrameRate;
extern int gGamma;
extern Resource gSysRes;

extern int costable[2048];

#pragma pack(push, 1)
struct NAMED_TYPE
{
    signed int id;
    char* name;
};

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

struct LINE2D
{
	int x1, y1, x2, y2;
};

struct STATUS_BIT1
{
	uint8_t ok : 1;
};

struct RGB
{
    unsigned char r, g, b;
};
typedef RGB PALETTE[256];


struct WALLPOINT_INFO
{
	int w, x, y;
};

struct OBJECT
{
	unsigned int type			: 8;
	unsigned int index			: 16;
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

struct DONEOFTOTAL
{
	unsigned int done				: 32;
	unsigned int total				: 32;
};
#pragma pack(pop)

unsigned int qrand(void);
void ChangeExtension(char *pzFile, const char *pzExt);
bool FileLoad(char *fname, void *buffer, unsigned int size);
bool FileSave(char *fname, void *buffer, unsigned int size);
char fileExists(char* filename, RESHANDLE* rffItem = NULL);
int GetOctant(int x, int y);
void RotateVector(int *dx, int *dy, int nAngle);
void RotatePoint(int *x, int *y, int nAngle, int ox, int oy);
void trigInit(Resource &Res);
void GetSpriteExtents(spritetype* pSprite, int *top, int *bottom);
void offsetPos(int oX, int oY, int oZ, int nAng, int* x, int* y, int* z);
unsigned char mostUsedByte(unsigned char* bytes, int l, short nExcept);
void BeepOk(void);
void BeepFail(void);
char Beep(char cond);


inline char rngok(int val, int rngA, int rngB)		{ return (val >= rngA && val < rngB); }
inline char irngok(int val, int rngA, int rngB)		{ return (val >= rngA && val <= rngB); }
inline char chlRangeIsFine(int val)					{ return (val > 0 && val < 1024); }

inline char fileAttrSet(char* file, int attr)		{ return (_chmod(file, attr) == 0); }
inline char fileAttrSetRead(char* file)				{ return fileAttrSet(file, S_IREAD);}
inline char fileAttrSetWrite(char* file)			{ return fileAttrSet(file, S_IWRITE); }
inline int perc2val(int nPerc, int nVal)			{ return (nVal*nPerc) / 100; }
void swapValues(int *nSrc, int *nDest);
int revertValue(int nVal);

int fileLoadHelper(char* filepath, BYTE** out, int* loadFrom = NULL);
char fileDelete(char* file);
char isDir(char* pPath);
char isFile(char* pPath);
SCANDIRENT* dirScan(char* path, char* filter, int* numfiles, int* numdirs, char flags = 0x0);
SCANDIRENT* driveScan(int* numdrives);
char dirRemoveRecursive(char* path);
char makeBackup(char* filename);

void doGridCorrection(int* x, int* y, int nGrid);
void doGridCorrection(int* x, int nGrid);
void doWallCorrection(int nWall, int* x, int* y, int shift = 14);
void dozCorrection(int* z, int zStep = 0x3FF);

inline void updateClocks()
{	
	gFrameTicks = totalclock - gFrameClock;
	gFrameClock += gFrameTicks;	
}

inline int Sin(int ang) { return costable[(ang - 512) & 2047]; }
inline int Cos(int ang) { return costable[ang & 2047]; }
inline int scale(int a1, int a2, int a3, int a4, int a5) { return a4 + (a5-a4) * (a1-a2) / (a3-a2); }
inline int QRandom2(int a1) { return mulscale14(qrand(), a1)-a1; }

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

inline int IncNext(int n, int mod)								{ return (n + mod) & ~(mod - 1); }
inline int DecNext(int n, int mod)								{ return (n - 1) & ~(mod - 1); }
inline int ItrNext(char neg, int n, int mod)					{ return (neg) ? DecNext(n, mod) : IncNext(n, mod); }
inline int ClipLow(int a, int b)								{ return (a < b) ? b : a; }
inline int ClipHigh(int a, int b)								{ return (a > b) ? b : a; }
inline int ClipRange(int a, int b, int c)						{ return (a < b) ? b : (a > c) ? c : a;	}
inline int interpolate(int a, int b, int c)						{ return a+mulscale16(b-a,c); }
inline void SetBitString(unsigned char *pArray, int nIndex)		{ pArray[nIndex>>3] |= 1<<(nIndex&7); }
inline void ClearBitString(unsigned char *pArray, int nIndex)	{ pArray[nIndex >> 3] &= ~(1 << (nIndex & 7)); }
inline char TestBitString(unsigned char *pArray, int nIndex)	{ return pArray[nIndex>>3] & (1<<(nIndex&7)); }
inline int tgetpalookup(int a1, int a2)							{ return ClipRange((a1 >> 8) + a2, 0, numpalookups - 1); }
inline int shgetpalookup(int a1, int a2)						{ return tgetpalookup(a1, a2) << 8; }
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

inline int exactDist(int dx, int dy) { return ksqrt(dx*dx+dy*dy); }
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

inline int wrand(void)
{
	static int wrandomseed = 1;
	wrandomseed = 1103515245 * wrandomseed + 12345;
    return (wrandomseed >> 16) & 0x7FFF;
}
inline char Chance(int a1)									{ return rand() < (a1>>1); }
inline unsigned int Random(int a1)							{ return mulscale15(rand(), a1); }
inline int BiRandom(int a1)									{ return mulscale14(rand(), a1)-a1; }
inline int BiRandom2(int a1)								{ return mulscale15(wrand()+wrand(), a1) - a1; }
inline char pointBehindLine(int x, int y, int x1, int y1, int x2, int y2)
{
	return ((x-x1) * (y2-y1) > (y-y1) * (x2-x1));
}

#if USE_OPENGL
char GL_swapIntervalTest();
int GL_swapInterval2Fps(int interval);
int GL_fps2SwapInterval(int fps);
#endif

BYTE countBestColor(PALETTE in, int r, int g, int b, int wR = 1, int wG = 1, int wB = 1);

class VOIDLIST
{
	protected:
		uint8_t *db;
		uint64_t totalsiz;
		uint32_t entrysiz;
	public:
		VOIDLIST(void)
		{
			entrysiz = 0;
			totalsiz = 0;
			db = NULL;
		}
		
		VOIDLIST(int nEntrySiz, void* pEOL = NULL)
		{
			entrysiz = 0;
			totalsiz = 0;
			db = NULL;
			
			Init(nEntrySiz, pEOL);
		}
		
		~VOIDLIST(void)
		{
			Free();
		}
		
		void Init(int nEntrySiz, void* pEOL = NULL)
		{
			Free();
			entrysiz = nEntrySiz;
			db = (uint8_t*)malloc(2 * entrysiz);
			dassert(db != NULL);
			
			if (pEOL)
			{
				memcpy(&db[0], pEOL, entrysiz);			// left edge
			}
			else
			{
				memset(&db[0], 0, entrysiz);			// left edge
			}
			
			memcpy(&db[entrysiz], &db[0], entrysiz);	// right edge
			totalsiz = 0;
		}
		
		int AddUnique(void* pData)
		{
			int n;
			if ((n = Find(pData)) >= 0)
				return n;
			
			return Add(pData);
		}
		
		int Add(void* pData, void* pOther, char after)
		{
			int n;
			if ((n = Find(pOther)) >= 0)
			{
				if (after)
				{
					n+=entrysiz;
					if (n > totalsiz)
						return Add(pData);
				}
				
				totalsiz += (3 * entrysiz);
				db = (uint8_t*)realloc(db, totalsiz);
				dassert(db != NULL);
				
				totalsiz-=entrysiz;
				memcpy(&db[n + entrysiz], &db[n], totalsiz - n);
				memcpy(&db[n], pData, entrysiz);
				
				totalsiz-=entrysiz;
				return n;
			}
			
			return Add(pData);
		}
		
		int Add(void* pData)
		{
			totalsiz += (3 * entrysiz);
			db = (uint8_t*)realloc(db, totalsiz);
			dassert(db != NULL);
			
			totalsiz-=entrysiz;
			memcpy(&db[totalsiz], &db[0], entrysiz);
			
			totalsiz-=entrysiz;
			memcpy(&db[totalsiz], pData, entrysiz);
			return totalsiz;
		}

		int Remove(void* pData)
		{
			int s, n, r = -1;
			
			while((n = Find(pData)) >= 0)
			{
				s = totalsiz + (2 * entrysiz);
				memcpy(&db[n], &db[n + entrysiz], s - n);
				db = (uint8_t*)realloc(db, s - entrysiz);
				totalsiz-=entrysiz;
				r = n;
			}
			
			return r;
		}
		
		int Find(void* pData)
		{
			for (int i = entrysiz; i <= totalsiz; i+=entrysiz)
			{
				if (memcmp(&db[i], pData, entrysiz) == 0)
					return i;
			}
			
			return -1;
		}
		
		void* Ptr(int nIndex)
		{
			int n = ClipRange((nIndex + 1) * entrysiz, entrysiz, totalsiz);
			return (void*)&db[n];
		}
		
		void Clear(void)
		{
			db = (uint8_t*)realloc(db, 2 * entrysiz);
			memcpy(&db[entrysiz], &db[0], entrysiz);	// right edge
			totalsiz = 0;	
		}
		
		void Free(void)
		{
			entrysiz = 0;
			totalsiz = 0;
			
			if (db)
				FREE_AND_NULL(db);
		}
		
		void Sort(void* pFunc)
		{
			if (Length() > 1)
				qsort(First(), Length(), entrysiz, (int(*)(const void*,const void*))pFunc);
		}
		
		inline char Exists(void* pData)		{ return (Find(pData) > 0);								}
		inline void* First(void)			{ return &db[entrysiz];									}
		inline void* Last(void)				{ return &db[ClipLow(totalsiz, entrysiz)];				}
		inline int Length(void)				{ return totalsiz / entrysiz;							}
		inline char Empty(void)				{ return (Length() == 0);								}
		inline char Valid(void* p)			{ return (p >= First() && p <= Last());					}
};

class IDLIST : public VOIDLIST
{
	public:
		IDLIST(int32_t dummy, int32_t eol = -1)		: VOIDLIST(sizeof(eol), &eol) { }
 		inline int Add(int nID)						{ return VOIDLIST::Add(&nID);							}
		inline int Remove(int nID)					{ return VOIDLIST::Remove(&nID);						}
		inline char Exists(int nID)					{ return VOIDLIST::Exists(&nID);						}
		inline int32_t* First(void)					{ return (int32_t*)VOIDLIST::First();					}
		inline int32_t* Last(void)					{ return (int32_t*)VOIDLIST::Last();					}
		inline int32_t* Ptr(int nID)				{ return (int32_t*)VOIDLIST::Ptr(nID);					}
		inline int32_t* Ptr(void)					{ return First();										}
		
		// compat
		typedef void (*IDLIST_PROCESS_FUNC)(int32_t nId);
		
		inline void Init(int32_t eol = -1)			{ VOIDLIST::Init(sizeof(eol), &eol);					}
		inline int AddIfNot(int nID)				{ return VOIDLIST::AddUnique(&nID);						}
		inline int GetLength(void)					{ return Length();										}
		inline int32_t* GetPtr(void)				{ return First();										}
		
		void Process(IDLIST_PROCESS_FUNC pFunc)
		{
			for (int32_t* p = First(); *p >= 0; p++)
				pFunc(*p);
		}
		//
};

class OBJECT_LIST : public VOIDLIST
{
	public:
  		OBJECT_LIST() : VOIDLIST()
		{
			OBJECT obj = {OBJ_NONE, 0};
			VOIDLIST::Init(sizeof(obj), &obj);			
		}
		
		inline int Add(unsigned int nType, unsigned int nIndex)
		{
			OBJECT o = { nType, nIndex };
			return VOIDLIST::Add(&o);
		}
		
		inline int Find(unsigned int nType, unsigned int nIndex)
		{
			OBJECT o = { nType, nIndex };
			return VOIDLIST::Find(&o);
		}
		
		inline int Remove(unsigned int nType, unsigned int nIndex)
		{
			OBJECT o = { nType, nIndex };
			return VOIDLIST::Remove(&o);
		}
		
		inline char Exists(unsigned int nType, unsigned int nIndex)	
		{
			return (Find(nType, nIndex) >= 0);
		}
		
		inline OBJECT* First(void)						{ return (OBJECT*)VOIDLIST::First();	}
		inline OBJECT* Last(void)						{ return (OBJECT*)VOIDLIST::Last();		}
		inline OBJECT* Ptr(int nID)						{ return (OBJECT*)VOIDLIST::Ptr(nID);	}
		inline OBJECT* Ptr(void)						{ return First();						}
};


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
		void Offset (int byX, int byY, int byZ)				{ offsetPos(byX, byY, byZ, a, &x, &y, &z); }
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

class BITSTRING
{
	private:
		uint8_t* db;
		uint32_t nLargest, nAdd;
	public:
		BITSTRING(int nAdd = 16384)
		{
			this->db		= NULL;
			this->nAdd		= nAdd;
			this->nLargest	= 0;
		}
		~BITSTRING()
		{
			if (db)
				FREE_AND_NULL(db);
		}
		void Add(uint32_t nID)
		{
			if (nID >= nLargest)
			{
				uint32_t c = nLargest>>3;
				uint32_t l;
				
				nLargest = l = nID+nAdd+8, l >>=3;
				db = (uint8_t*)realloc(db, sizeof(uint8_t)*l);
				dassert(db != NULL);
				
				memset(&db[c], 0, l-c);
			}
			
			db[nID>>3] |= 1<<(nID&7);
		}
		
		void Remove(uint32_t nID)
		{
			if (nID < nLargest)
				db[nID>>3] &= ~(1<<(nID&7));
		}
		
		inline char Exists(uint32_t nID)
		{
			return (nID < nLargest && (db[nID>>3] & (1<<(nID&7))));
		}
};

class Rect {
public:
    int x0, y0, x1, y1;
    Rect()
	{
		x0 = 0;
		y0 = 0;
		x1 = 0;
		y1 = 0;
	}
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
	
	void scale(int byX, int byY)
	{
		x0 -= byX;
		y0 -= byY;
		x1 += byX;
		y1 += byY;
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

