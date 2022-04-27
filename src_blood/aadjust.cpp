/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
// Functions to fix and cleanup objects such as sprites, walls or sectors

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

#include "db.h"
#include "build.h"
#include "xmpstub.h"
#include "aadjust.h"
#include "xmpconf.h"
#include "xmpmisc.h"
#include "misc.h"
#include "tile.h"
#include "tilefav.h"
#include "gameutil.h"
#include "screen.h"
#include "nnexts.h"
#include "hglt.h"
#include "xmptrig.h"
#include "sectorfx.h"
#include "seq.h"
#include "gui.h"
#include "edit3d.h"
#include "mapcmt.h"

AUTODATA autoData[] = {

	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerSPStart,				-1,		2522,		56,	56,	0,	kPlu0 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerMPStart,				-1,		2522,		56,	56,	0,  kPlu5 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerDudeSpawn,			-1,		2077,		40,	40,	0,	kPluAny },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerEarthquake,			-1,		2072,		40,	40,	0,	kPlu0 },
	{	kOGrpMarker,	FALSE,	TRUE,	kGenDripBlood,				-1,		2028,		-1, -1, -1, kPlu2 },
	{	kOGrpMarker,	FALSE,	TRUE,	kGenDripWater,				-1,		2028,		-1, -1, -1, kPlu10 },
	{	kOGrpMarker,	FALSE,	TRUE,	kGenBubble,					-1,		1128,		40,	40,	-1,	kPluAny },
	{	kOGrpMarker,	FALSE,	TRUE,	kGenBubbleMulti,			-1,		1132,		40,	40,	-1,	kPluAny },
	{	kOGrpMarker,	FALSE,	TRUE,	kGenSound,					-1,		2519,		32,	24,	-1,	kPlu6 },
	{	kOGrpMarker,	FALSE,	TRUE,	kSoundSector,				-1,		2520,		32,	24,	-1,	kPlu9 },
	{	kOGrpMarker,	FALSE,	TRUE,	kSoundAmbient,				-1,		2521,		32,	24,	-1,	kPlu0 },
	{	kOGrpMarker,	FALSE,	TRUE,	kSoundPlayer,				-1,		2519,		32,	24,	-1,	kPlu5 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerUpWater,				-1,		2332,		64,	64,	0,	kPlu0 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerLowWater,			-1,		2331,		64,	64, 0,	kPlu0 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerUpStack,				-1,		2332,		64,	64, 0,	kPlu2 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerLowStack,			-1,		2331,		64,	64, 0,	kPlu2 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerUpLink,				-1,		2332,		64,	64, 0,	kPlu6 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerLowLink,				-1,		2331,		64,	64, 0,	kPlu6 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerUpGoo,				-1,		2332,		64,	64, 0,	kPlu5 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerLowGoo,				-1,		2331,		64,	64, 0,	kPlu5 },
	{   kOGrpMarker,	FALSE,	TRUE,	kMarkerPath,				-1,		2319,		144, 144, 0, kPlu0 },

	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1072,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1074,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1076,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1070,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1078,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1048,		40,	40,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchToggle,				-1,		1046,		40,	40,	-1,	kPluAny },

	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1072,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1074,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1076,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1070,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1078,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1048,		40,	40,	-1,	kPluAny },
	{   kOGrpNone,		TRUE,	TRUE,	kSwitchOneWay,				-1,		1046,		40,	40,	-1,	kPluAny },

	{   kOGrpMisc,		TRUE,	TRUE,	kSwitchCombo,				-1,		1161,		40,	40,	-1,	kPluAny },
	{	kOGrpMisc,		TRUE,	TRUE,	kThingWallCrack,			-1,		1127,		-1,	-1,	-1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingSpiderWeb,			-1,		1069,		-1,	-1,	1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingWoodBeam,				-1,		1142,		-1,	-1,	-1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingFlammableTree,		-1,		3355,		-1, -1, -1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingFlammableTree,		-1,		3363,		-1, -1, -1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingCrateFace,			-1,		462,		-1,	-1,	-1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingFluorescent,			-1,		796,		-1,	-1,	-1,	kPluAny },
	{	kOGrpMisc,		FALSE,	TRUE,	kThingMetalGrate,			-1,		483,		-1, -1, -1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationCandle,			-1,		938,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		550,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		572,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		560,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		564,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		570,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	FALSE,	kDecorationTorch,			-1,		554,		-1,	-1,	-1,	kPluAny },
	{   kOGrpMisc,		TRUE,	TRUE,   kSwitchPadlock,				37,		948,		-1,	-1,	-1,	kPluAny },

	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponFlarePistol,		-1,		524,		48,	48,	0,	kPluAny },
	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponSawedoff,		-1,		559,		48,	48,	0,	kPluAny },
	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponTommygun,		-1,		558,		48,	48,	0,	kPluAny },
	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponNapalmLauncher,	-1,		526,		48,	48,	0,	kPluAny },
	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponTeslaCannon,		-1,		539,		48,	48,	0,	kPluAny },
	{   kOGrpWeapon,	FALSE,	TRUE,	kItemWeaponLifeLeech,		-1,		800,		48,	48,	0,	kPluAny },

	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoFlares,			-1,		816,		48,	48,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoTNTBox,			-1,		809,		48,	48,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoTNTBundle,			-1,		589,		48,	48,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoProxBombBundle,	-1,		811,		40,	40,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoRemoteBombBundle,	-1,		810,		40,	40,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoSprayCan,			-1,		618,		40,	40,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoSawedoffBox,		-1,		812,		48,	48,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoSawedoffFew,		-1,		619,		48,	48,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoTommygunDrum,		-1,		817,		48,	48,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoTommygunFew,		-1,		813,		48,	48,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoGasolineCan,		-1,		801,		48,	48,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoTeslaCharge,		-1,		548,		24,	24,	0,	kPluAny },
	{	kOGrpAmmo,		FALSE,	TRUE,	kItemAmmoTrappedSoul,		-1,		820,		24,	24,	0,	kPluAny },
	{	kOGrpAmmoMix,	FALSE,	TRUE,	kItemAmmoVoodooDoll,		-1,		525,		48,	48,	0,	kPluAny },

	{	kOGrpItem,		FALSE,	TRUE,	kItemKeySkull,				-1,		2552,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemKeyEye,				-1,		2553,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemKeyFire,				-1,		2554,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemKeyDagger,				-1,		2555,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemKeySpider,				-1,		2556,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemKeyMoon,				-1,		2557,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		TRUE,	TRUE,	kItemKey7,					-1,		2558,		32,	32,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemHealthRedPotion,		-1,		517,		40,	40,	0,	kPlu2 },
	{	kOGrpItem,		FALSE,	TRUE,	kItemHealthLifeEssense,		-1,		2169,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemHealthMedPouch,		-1,		822,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemHealthLifeSeed,		-1,		2433,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemHealthDoctorBag,		-1,		519,		48,	48,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemFeatherFall,			-1,		783,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemShadowCloak,			-1,		896,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemDeathMask,				-1,		825,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemJumpBoots,				-1,		827,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemTwoGuns,				-1,		829,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemGasMask,				-1,		831,		56,	56,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemDivingSuit,			-1,		830,		80,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorAsbest,			-1,		837,		72,	72,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemReflectShots,			-1,		2428,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemBeastVision,			-1,		839,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemCrystalBall,			-1,		760,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemDoppleganger,			-1,		853,		56,	56,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemShroomDelirium,		-1,		841,		48,	48,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemShroomGrow,			-1,		842,		48,	48,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemShroomShrink,			-1,		843,		48,	48,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorBasic,			-1,		2628,		64,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorBody,				-1,		2586,      	64,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorFire,				-1,		2578,      	64,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorSpirit,			-1,		2602,		64,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemArmorSuper,			-1,		2594,		64,	64,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemTome,					-1,		518,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemBlackChest,			-1,		522,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	kItemWoodenChest,			-1,		523,		40,	40,	0,	kPluAny },
	{	kOGrpItem,		FALSE,	TRUE,	40,							-1,		832,		40,	40,	0,	kPluAny },

	{	kOGrpDude,		FALSE,	TRUE,	kDudeCultistShotgun,		11520,	2825,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistShotgunProne,	11534,	3375,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistTommy,			4102,	2820,		40,	40,	1,	kPlu3 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistTommyProne, 	4113,	3385,		40,	40,	1,	kPlu3 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistTesla,			12809,	2875,		40,	40,	1,	kPlu11 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistTNT, 			13063,	2830,		40,	40,	1,	kPlu13 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeZombieAxeNormal,		4352,	1224,		40, 40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeZombieAxeBuried,		4361,	3048,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeZombieAxeLaying,		4362,	1209,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeZombieButcher,			4608,	1370,		48,	48,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeGargoyleFlesh,			4864,	1470,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeGargoyleStatueFlesh,	11013,	1530,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudePhantasm,				5376,	3060,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeHellHound,     		5632,	1270,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeGillBeast,				7168,	1570,		48,	48,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudePodGreen,				8192,	1792,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeTentacleGreen,			8454,	1797,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudePodFire,				8709,	1793,		48,	48,	1,	kPlu2 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeTentacleFire, 			8965,	1797,		48,	48,	1,	kPlu2 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeHand,					5888,	1980,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeRat,					7936,	1745,		24,	24,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeBoneEel,				7424,	1870,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeBat,					7686,	1950,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeSpiderBrown, 			6144,	1920,		16,	16,	1,	kPlu7 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeSpiderRed,				6407,	1925,		24,	24,	1,	kPlu4 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeSpiderBlack,			6662,	1930,		32,	32,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeInnocent,				12544,	3798,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeTinyCaleb,				13568,	3870,		16,	16,	1,	kPlu12 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeGargoyleStone, 		5120,	2595,		64,	64,	1,	kPlu0 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeGargoyleStatueStone,	11269,  1530,		48,	48,	1,	kPlu0 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeSpiderMother,			6663,	1930,		40,	40,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCerberusTwoHead,		9728, 	2680,		64,	64,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCerberusOneHead, 		9984,	2749,		64,	64,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeTchernobog,			10240, 	3140,		-1,	-1,	1,	kPluAny },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeCultistBeast,			13312,	2825,		48,	48,	1,	kPlu12 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeBeast,					10752,	2960,		48,	48,	1,	kPlu12 },
	{	kOGrpDude,		FALSE,	TRUE, 	kDudeModernCustom, 			-1,		2925,		-1,	-1,	1,	kPluAny },
	{	kOGrpDude,		TRUE,	TRUE,  	225,  						-1,		3949,		-1, -1, 1, 	kPluAny },

	{	kOGrpHazard,	FALSE,	TRUE,	kThingTNTBarrel,			-1,		907,		-1,	-1,	1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE, 	kTrapMachinegun,			38,		2182,		-1,	-1,	0,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE, 	kTrapFlame,					39,		2183,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	kGenMissileFireball,		-1,		249,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	kGenMissileEctoSkull,		-1,		226,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kTrapExploder,				-1,		908,	 	4,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kTrapSawCircular,			-1,		655,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kTrapPendulum,				-1,		1080,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kTrapGuillotine,			-1,		835,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kTrapZapSwitchable,			-1,		1156,		-1,	-1,	-1,	kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	kThingArmedSpray,			-1,		3467,		32,	32,	1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kThingArmedProxBomb,		-1,		3444,		40,	40,	1,	kPluAny },
	{	kOGrpHazard,	FALSE,	TRUE,	kThingArmedRemoteBomb,		-1,		3457,		40,	40,	1,	kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	kThingArmedTNTStick,		-1,		3422,		32,	32,	1,	kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	428,						-1,		3295,		40, 40, -1, kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	430,						-1,		3633,		40, 40, -1, kPluAny },
	{	kOGrpHazard,	TRUE,	TRUE,	431,						-1,		3317,		24, 24, -1, kPluAny },

	{	kOGrpNone,		FALSE,	TRUE,	kThingFallingRock,			-1,		-1,			-1,	-1,	1,	kPluAny },
	{	kOGrpNone,		FALSE,	TRUE,	kThingKickablePail,			-1,		-1,			-1,	-1,	1,	kPluAny },
	{	kOGrpNone,		FALSE,	TRUE,	kThingObjectGib,			-1,		-1,			-1,	-1,	-1,	kPluAny },
	{	kOGrpNone,		FALSE,	TRUE,	kThingObjectExplode,		-1,		-1,			-1,	-1,	-1,	kPluAny },
	{	kOGrpNone,		FALSE,	TRUE,	kThingZombieHead,			-1,		-1,			-1, -1, 1,  kPluAny },

	{	kOGrpModern,	FALSE,	TRUE,	kModernCustomDudeSpawn,		-1,		2077,		-1, -1, -1, kPluAny },
	{   kOGrpModern,	FALSE,	TRUE,	kMarkerWarpDest,			-1,		3193,		255, 255, -1, kPlu9 },
	{   kOGrpModern,	FALSE,	TRUE,	16,							-1,		3193,		128, 128, -1, kPluAny },
	{	kOGrpModern,	FALSE,	TRUE,	kModernCondition,			-1,		4127,		160, 160, 0, kPlu10 },
	{	kOGrpModern,	FALSE,	TRUE,	kModernConditionFalse,		-1,		4127,		160, 160, 1, kPlu7 },

};

const short autoDataLength = LENGTH(autoData);

unsigned short markers[]  = { kMarkerOff, kMarkerOn, kMarkerAxis, kMarkerWarpDest };
unsigned short misc[]     = { kMarkerSPStart, kMarkerMPStart, kMarkerDudeSpawn, kMarkerEarthquake, kGenSound, kSoundSector, kSoundPlayer };
unsigned short cond[] 	  = { kModernCondition, kModernConditionFalse };
SYS_STATNUM_GROUP sysStatData[] = {

	{ 0x02,		kStatMarker,					0,							0, 					markers, LENGTH(markers) },
	{ 0x03,		kStatPathMarker,				kMarkerPath,				0,					NULL, 0 },
	{ 0x03,		kStatDude,						kDudeBase, 					kDudeMax - 1,		NULL, 0 },
	{ 0x03,		kStatThing,						kThingBase, 				kThingMax - 1,		NULL, 0 },
	{ 0x03,		kStatItem,						kItemWeaponBase, 			kItemMax - 1,		NULL, 0 },
	{ 0x03,		kStatProjectile,				kMissileBase, 				kMissileMax - 1,	NULL, 0 },
	{ 0x03,		kStatTraps,						kTrapBase, 					kTrapMax - 1,		NULL, 0 },
	{ 0x03,		kStatAmbience,					kSoundAmbient,				0,					NULL, 0 },
	{ 0x03,		kStatDecoration,				0,							0, 					misc, LENGTH(misc) },
	{ 0x03,		kStatModernStealthRegion,		kModernStealthRegion,		0,					NULL, 0 },
	{ 0x03,		kStatModernDudeTargetChanger,	kModernDudeTargetChanger,	0,					NULL, 0 },
	{ 0x03,		kStatModernCondition,			0,							0, 					cond, LENGTH(cond) },
	
};

BOOL sysStatReserved(int nStat) {
	
	int i; 
	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		if (nStat == sysStatData[i].statnum) return TRUE;
	}
	
	return FALSE;
}

BOOL sysStatCanChange(spritetype* pSpr) {
	
	int i, j;
	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		SYS_STATNUM_GROUP* data = &sysStatData[i];
		if (data->enumArray)
		{
			for (j = 0; j < data->enumLen; j++)
			{
				if (pSpr->type == data->enumArray[j])
					return FALSE;
			}
		}
		else if (data->typeMax <= 0 && pSpr->type == data->typeMin) return FALSE;
		else if (irngok(pSpr->type, data->typeMin, data->typeMax))  return FALSE;
	}
	
	return TRUE;
}

void AutoAdjustSprites(void) {

	if (gPreviewMode || !gAutoAdjust.enabled)
		return;

	short nSector, nSprite;
	int j = 0, k = 0, i = 0, zTop, zBot;
	spritetype* pSprite = NULL; XSPRITE* pXSprite = NULL;

	for (nSector = 0; nSector < numsectors; nSector++)
	{
		for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			pSprite = &sprite[nSprite];
			if (pSprite->statnum == kStatFree)
				continue;

			int iPicnum = -1, iType = -1, nXSprite = -1, otype = pSprite->type;
			
			CleanUpStatnum(nSprite);
	#if 0
		if ((pSprite->cstat & kSprRelMask) == kSprVoxel)
			pSprite->cstat &= ~kSprRelMask;
	#endif

			i = -1;
			iPicnum = adjIdxByTileInfo(pSprite->picnum, 0); 	// search by pic
			iType 	= adjIdxByType(pSprite->type); 				// search by type

			// this set of rules determine which row to use from the autoData table
			if (iPicnum >= 0)
			{
				// if we found by picnum, but picnum of otype == -1 in autodata, give it priority
				if ((i = adjIdxByType(otype)) >= 0)
					i = (autoData[i].picnum < 0) ? -2 : iPicnum;
			}

			if (i > -2)
			{
				if (iType >= 0) i = iType;
				if (iPicnum >= 0 && autoData[iPicnum].type == pSprite->type)
					i = iPicnum;
			}

			if (i < 0)
				continue; // if nothing found, ignore this sprite

			if (autoData[i].xsprite)
			{
				nXSprite = GetXSprite(nSprite);
				pXSprite = &xsprite[nXSprite];
			}

			short type = autoData[i].type;
			if (!autoData[i].exception) {

				pSprite->type = type;
				adjSetApperance(pSprite, i);

				if (pSprite->extra > 0)
				{
					nXSprite = pSprite->extra;
					pXSprite = &xsprite[pSprite->extra];
				}

				switch (type) {
					case 16:
						pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
						pSprite->cstat |= kSprInvisible;
						pSprite->shade = -128;
						if (pXSprite->data2 > 0 || pXSprite->data3 > 0) pSprite->pal = kPlu1;
						else pSprite->pal = kPlu10;
						break;
					case kTrapExploder:
						pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
						pSprite->cstat |= kSprInvisible;
						pSprite->shade = -128;
						break;
					case kDudeModernCustom:
						cdGetSeq(pSprite);
						break;
					case kDudeGillBeast:
						if (pSprite->sectnum >= 0 && sector[pSprite->sectnum].extra > 0 && xsector[sector[pSprite->sectnum].extra].underwater)
							pSprite->picnum = 1600;
						break;
					case kDudeCerberusOneHead:
						if (pXSprite->rxID || pXSprite->triggerProximity || pXSprite->triggerTouch || pXSprite->triggerSight) break;
						pXSprite->rxID = 7;
						break;
					case kMarkerSPStart:
					case kMarkerMPStart:
						pXSprite->data1 &= 8 - 1;
						pSprite->picnum = (short)(2522 + pXSprite->data1);
						break;
					case kMarkerUpLink:
					case kMarkerUpWater:
					case kMarkerUpGoo:
					case kMarkerUpStack:
					case kMarkerLowLink:
					case kMarkerLowWater:
					case kMarkerLowGoo:
					case kMarkerLowStack:
					case kMarkerDudeSpawn:
					case kMarkerEarthquake:
						pSprite->cstat &= ~kSprFlipY;
						if (!gModernMap) break;
						else if (type == kMarkerDudeSpawn)
						{
							for (k = 0, j = 0; k < 4; k++)
							{
								if ((j+= getDataOf((BYTE)k, OBJ_SPRITE, nSprite)) >= kDudeMax) {
									pSprite->picnum = 821;
									break;
								}
							}
						}
						break;
					case kDudeZombieAxeBuried:
						// place earth zombies to the ground
						if (pSprite->sectnum < 0) break;
						GetSpriteExtents(pSprite, &zTop, &zBot);
						pSprite->z = sector[pSprite->sectnum].floorz + getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot;
						break;
					case kThingFlammableTree:
						// try to find correct picnum
						switch (pXSprite->data1) {
							case 1:  pSprite->picnum = 547; break;
							case 0:  pSprite->picnum = 542; break;
							default:
								switch (pSprite->picnum) {
									case 3363:
										pXSprite->data1 = 1;
										pSprite->picnum = 547;
										break;
									case 3355:
										pXSprite->data1 = 0;
										pSprite->picnum = 542;
										break;
								}
								break;
						}
						break;
					case kSoundAmbient:
						if (pXSprite->data1 && pXSprite->data2 && pXSprite->data1 >= pXSprite->data2)
						{
							int tmp = pXSprite->data2;
							pXSprite->data2 = pXSprite->data1;
							pXSprite->data1 = tmp;

							if (pXSprite->data2 == pXSprite->data1)
								pXSprite->data2 = pXSprite->data1 << 1;
						}
						break;
				}
			}

			type = pSprite->type;
			if (autoData[i].group == kOGrpMarker)
			{
				pSprite->cstat &= ~kSprBlock & ~kSprHitscan;
				pSprite->cstat |= kSprInvisible;
				pSprite->shade = -128;
			}
			else if (rngok(type, kSwitchBase, kSwitchMax)
				|| rngok(type, kItemWeaponBase, kItemMax) || rngok(type, kDudeBase, kDudeMax))
					pSprite->cstat &= ~kSprBlock;

			// make sure sprites aren't in the floor or ceiling
			if (pSprite->statnum == kStatDude)
				clampSprite(pSprite);
		}
	}
}

void adjSetApperance(spritetype* pSprite, int idx) {

	if (!gAutoAdjust.enabled)
		return;

	int faved = tileInFaves(pSprite->picnum);
	if (faved < 0 || gFavTiles[faved].pic <= 0)
	{
		if (gAutoAdjust.setPic && autoData[idx].picnum >= 0)
			pSprite->picnum = autoData[idx].picnum;

		if (gAutoAdjust.setPlu && autoData[idx].plu >= 0)
			pSprite->pal = (BYTE)autoData[idx].plu;

		if (gAutoAdjust.setSize)
		{
			if (autoData[idx].xrepeat >= 0) pSprite->xrepeat = (BYTE)autoData[idx].xrepeat;
			if (autoData[idx].yrepeat >= 0) pSprite->yrepeat = (BYTE)autoData[idx].yrepeat;
		}

		if (pSprite->type != autoData[idx].type && autoData[idx].plu == kPluAny)
			pSprite->pal = kPlu0;
	}

	if (gAutoAdjust.setHitscan)
	{
		if (autoData[idx].hitBit == 0)     pSprite->cstat &= ~kSprHitscan;
		else if (autoData[idx].hitBit > 0) pSprite->cstat |= kSprHitscan;
	}

	switch (pSprite->type) {
		case kDudeModernCustom:
			cdGetSeq(pSprite);
			break;
		case kThingFlammableTree:
			return;
	}

}

int adjIdxByType(int nType) {

	int i;
	for (i = 0; i < LENGTH(autoData); i++)
	{
		if (nType != autoData[i].type) continue;
		return i;
	}

	return -1;
}

BOOL adjSpriteByType(spritetype* pSprite) {

	int i;
	if ((i = adjIdxByType(pSprite->type)) < 0)
		return FALSE;

	adjSetApperance(pSprite, i);
	clampSprite(pSprite);
	return TRUE;
}

// how many of *same* pictures should be skipped before we meet required?
int adjCountSkips(int pic) {

	int skip = 0;
	for (int i = 0; i < tileIndexCount; i++)
	{
		if (i == tileIndexCursor) break;
		else if (tileIndex[i] == pic)
			skip++;
	}

	return skip;

}

// returns required index of AUTODATA array
int adjIdxByTileInfo(int pic, int skip) {

	int i = 0, j = 0, k = 0;
	for (i = 0; i < autoDataLength; i++)
	{
		if (autoData[i].picnum == pic && k++ == skip)
			return i;
	}

	return -1;

}

int adjFillTilesArray(int objectGroup) {

	int i = 0, j = 0;
	for (j = 0, tileIndexCount = 0; j < autoDataLength; j++)
	{
		if (autoData[j].picnum < 0 || !(autoData[j].group & objectGroup))
			continue;
		
		for (i = 0; i < spriteNamesLength; i++)
		{
			if (spriteNames[i].id != autoData[j].type) continue;
			else if (gMisc.showTypes < spriteNames[i].compt)
				break;
			
			tileIndex[tileIndexCount] 	 = (short)autoData[j].picnum;
			tilePluIndex[tileIndexCount] = (char)ClipRange(autoData[j].plu, 0, kPluMax);
			tileIndexCount++;
		}
	}

	return tileIndexCount;
}

void CleanUp() {

	int nSprite, nSector, i, j, x, y, sect, sect2;
	
	if (gPreviewMode)
		return;

	for(i = numwalls - 1; i >= 0; i--)     //delete points
	{
		x = wall[i].x, y = wall[i].y;
		if (x != wall[wall[i].point2].x || y != wall[wall[i].point2].y)
			continue;
		
		sect = sectorofwall(i);
		if (sector[sect].wallnum <= 1)
		{
			sectDelete(sect); // delete the sector
		}
		else
		{
			deletepoint((short)i);
			for (j = 0; j < numsectors; j++)
				redRestore(j);
		}
	}
	
	if (gMisc.pan)
		AlignSlopes();

	dbXSectorClean();
	dbXWallClean();
	dbXSpriteClean();
	InitSectorFX();
	gCommentMgr.Cleanup();

	for (nSector = 0; nSector < numsectors; nSector++)
	{
		int nXSector = sector[nSector].extra;
		if (nXSector <= 0)
			continue;
		
		switch (sector[nSector].type) {
			case kSectorTeleport:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerWarpDest)
						sprite[nSprite].owner = (short)nSector;
					else
						xsector[nXSector].marker0 = -1;
				}
				break;
			case kSectorSlide:
			case kSectorSlideMarked:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerOff)
						sprite[nSprite].owner = (short)nSector;
					else
						xsector[nXSector].marker0 = -1;
				}

				if (xsector[nXSector].marker1 >= 0)
				{
					int nSprite = xsector[nXSector].marker1;
					if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerOn)
						sprite[nSprite].owner = (short)nSector;
					else
						xsector[nXSector].marker1 = -1;
				}
				break;
			case kSectorRotate:
			case kSectorRotateMarked:
			case kSectorRotateStep:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerAxis)
						sprite[nSprite].owner = (short)nSector;
					else
						xsector[nXSector].marker0 = -1;
				}
				break;
		}

	}

	for (nSector = 0; nSector < numsectors; nSector++)
	{
		int nXSector = sector[nSector].extra;
		if (nXSector <= 0)
			continue;
		
		switch (sector[nSector].type) {
			case kSectorTeleport:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (sprite[nSprite].owner != (short)nSector)
					{
						int nSprite2 = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
						sprite[nSprite2] = sprite[nSprite];
						sprite[nSprite2].owner = (short)nSector;
						xsector[nXSector].marker0 = nSprite2;
					}
				}

				if (xsector[nXSector].marker0 < 0)
				{
					int nSprite = InsertSprite(nSector, kStatMarker);
					sprite[nSprite].x = wall[sector[nSector].wallptr].x;
					sprite[nSprite].y = wall[sector[nSector].wallptr].y;
					sprite[nSprite].owner = (short)nSector;
					sprite[nSprite].type = kMarkerWarpDest;
					xsector[nXSector].marker0 = nSprite;
				}
				break;

			case kSectorSlide:
			case kSectorSlideMarked:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (sprite[nSprite].owner != (short)nSector)
					{
						int nSprite2 = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
						sprite[nSprite2] = sprite[nSprite];
						sprite[nSprite2].owner = (short)nSector;
						xsector[nXSector].marker0 = nSprite2;
					}
				}

				if (xsector[nXSector].marker0 < 0)
				{
					int nSprite = InsertSprite(nSector, kStatMarker);
					sprite[nSprite].x = wall[sector[nSector].wallptr].x;
					sprite[nSprite].y = wall[sector[nSector].wallptr].y;
					sprite[nSprite].owner = (short)nSector;
					sprite[nSprite].type = kMarkerOff;
					xsector[nXSector].marker0 = nSprite;
				}

				if (xsector[nXSector].marker1 >= 0)
				{
					int nSprite = xsector[nXSector].marker1;
					if (sprite[nSprite].owner != (short)nSector)
					{
						int nSprite2 = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
						sprite[nSprite2] = sprite[nSprite];
						sprite[nSprite2].owner = (short)nSector;
						xsector[nXSector].marker1 = (short)nSprite2;
					}
				}

				if (xsector[nXSector].marker1 < 0)
				{
					int nSprite = InsertSprite(nSector, kStatMarker);
					sprite[nSprite].x = wall[sector[nSector].wallptr].x;
					sprite[nSprite].y = wall[sector[nSector].wallptr].y;
					sprite[nSprite].owner = (short)nSector;
					sprite[nSprite].type = kMarkerOn;
					xsector[nXSector].marker1 = nSprite;
				}
				break;
			case kSectorRotate:
			case kSectorRotateMarked:
			case kSectorRotateStep:
				if (xsector[nXSector].marker0 >= 0)
				{
					int nSprite = xsector[nXSector].marker0;
					if (sprite[nSprite].owner != (short)nSector)
					{
						int nSprite2 = InsertSprite(sprite[nSprite].sectnum, kStatMarker);
						sprite[nSprite2] = sprite[nSprite];
						sprite[nSprite2].owner = (short)nSector;
						xsector[nXSector].marker0 = nSprite2;
					}
				}

				if (xsector[nXSector].marker0 < 0)
				{
					int nSprite = InsertSprite(nSector, kStatMarker);
					sprite[nSprite].x = wall[sector[nSector].wallptr].x;
					sprite[nSprite].y = wall[sector[nSector].wallptr].y;
					sprite[nSprite].owner = (short)nSector;
					sprite[nSprite].type = kMarkerAxis;
					xsector[nXSector].marker0 = nSprite;
				}
				break;
			default:
				xsector[nXSector].marker0 = -1;
				xsector[nXSector].marker1 = -1;
				break;
		}
	}

	for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = j)
	{
		j = nextspritestat[nSprite];
		sprite[nSprite].extra = -1;
		sprite[nSprite].cstat |= kSprInvisible;
		sprite[nSprite].cstat &= ~kSprBlock & ~kSprHitscan;

		int nSector = sprite[nSprite].owner;
		int nXSector = sector[nSector].extra;

		if (nSector >= 0 && nSector < numsectors && nXSector > 0 && nXSector < kMaxXSectors)
		{
			switch (sprite[nSprite].type)
			{
				case kMarkerOff:
					sprite[nSprite].picnum = 3997;
					if (xsector[nXSector].marker0 == nSprite)
						continue;
					break;

				case kMarkerOn:
					sprite[nSprite].picnum = 3997;
					if (xsector[nXSector].marker1 == nSprite)
						continue;
					break;

				case kMarkerAxis:
					sprite[nSprite].picnum = 3997;
					if (xsector[nXSector].marker0 == nSprite)
						continue;
					break;

				case kMarkerWarpDest:
					if (xsector[nXSector].marker0 == nSprite)
						continue;
					break;
			}
		}

		DeleteSprite(nSprite);
	}


}

void CleanUpStatnum(int nSpr) {

	if (gPreviewMode)
		return;
	
	int i, j;
	spritetype* pSpr =& sprite[nSpr];

	for (i = 0; i < LENGTH(sysStatData); i++)
	{
		int chgTo = -1;
		SYS_STATNUM_GROUP* data = &sysStatData[i];
		
		// set proper statnum for types in a range
		if ((data->check & 0x01) && pSpr->statnum != data->statnum)
		{
			if (data->enumArray)
			{
				for (j = 0; j < data->enumLen; j++)
				{
					if (pSpr->type != data->enumArray[j]) continue;
					chgTo = data->statnum;
					break;
				}
			}
			else if (data->typeMax <= 0 && pSpr->type == data->typeMin) chgTo = data->statnum;
			else if (data->typeMax  > 0 && irngok(pSpr->type, data->typeMin, data->typeMax))
				chgTo = data->statnum;
		}

		// erase statnum for types that are NOT in a range
		if ((data->check & 0x02) && pSpr->statnum == data->statnum)
		{
			if (data->enumArray)
			{
				for (j = 0; j < data->enumLen; j++)
				{
					if (pSpr->type == data->enumArray[j])
						break;
				}
				
				if (j >= data->enumLen)
					chgTo = 0;
			}
			else if (data->typeMax <= 0 && pSpr->type != data->typeMin) chgTo = 0;
			else if (data->typeMax  > 0 && !irngok(pSpr->type, data->typeMin, data->typeMax))
				chgTo = 0;
		}
		
		if (chgTo >= 0)
		{
			if (data->check & 0x04) pSpr->statnum = chgTo;
			else ChangeSpriteStat(nSpr, chgTo);
		}
	}
}

void CleanUpMisc() {

	if (gPreviewMode)
		return;
	
	int i, j;
	for (i = 0; i < numwalls; i++)
	{
		// useless x-object check
		if (wall[i].type == 0 && wall[i].extra > 0 && obsoleteXObject(OBJ_WALL, wall[i].extra))
			dbDeleteXWall(wall[i].extra);

		if (wall[i].nextwall >= 0)
			continue;

		// clear useless cstat for white walls...
		if (wall[i].cstat & kWallMasked) wall[i].cstat &= ~kWallMasked;
		if (wall[i].cstat & kWallBlock) wall[i].cstat &= ~kWallBlock;
		if (wall[i].cstat & kWallHitscan) wall[i].cstat &= ~kWallHitscan;
		if (wall[i].cstat & kWallOneWay) wall[i].cstat &= ~kWallOneWay;
		if (wall[i].cstat & kWallSwap) wall[i].cstat &= ~kWallSwap;
		if (wall[i].cstat & kWallTransluc) wall[i].cstat &= ~kWallTransluc;
		if (wall[i].cstat & kWallTranslucR) wall[i].cstat &= ~kWallTranslucR;
	}

	for (i = 0; i < numsectors; i++)
	{
		sectortype* pSector = &sector[i];

		// useless x-object check
		if (pSector->type == 0 && pSector->extra > 0 && obsoleteXObject(OBJ_FLOOR, pSector->extra))
			dbDeleteXSector(pSector->extra);

		// fix ceiling slopes
		if (pSector->ceilingslope != 0)
		{
			if (!(pSector->ceilingstat & kSectSloped))
				pSector->ceilingslope = 0;
		}
		else if (pSector->ceilingstat & kSectSloped)
		{
			pSector->ceilingstat &= ~kSectSloped;
		}

		// fix floor slopes
		if (pSector->floorslope != 0)
		{
			if (!(pSector->floorstat & kSectSloped))
				pSector->floorslope = 0;
		}
		else if (pSector->floorstat & kSectSloped)
		{
			pSector->floorstat &= ~kSectSloped;
		}

		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			CleanUpStatnum(j);
			spritetype* pSprite =& sprite[j];

			// useless x-sprites check
			if (pSprite->statnum == kMaxStatus) continue;
			else if (!pSprite->type && pSprite->extra > 0 && obsoleteXObject(OBJ_SPRITE, pSprite->extra))
				dbDeleteXSprite(pSprite->extra);

			if (pSprite->extra > 0)
			{
				XSPRITE* pXSpr =&xsprite[sprite[j].extra];

				// no pickup for non-item
				if (pSprite->statnum != kStatItem && pXSpr->triggerPickup)
					pXSpr->triggerPickup = FALSE;
				
				// item out of a range
				if (pXSpr->dropItem >= kItemMax)
					pXSpr->dropItem = 0;
				
				if (pSprite->statnum == kStatDude)
				{
					// use KEY instead of DROP field for key drop
					if (pXSpr->dropItem >= kItemKeySkull && pXSpr->dropItem <= kItemKey7)
						pXSpr->key = 1 + pXSpr->dropItem - kItemKeySkull, pXSpr->dropItem = 0;
				}
			}
		}
	}
}
