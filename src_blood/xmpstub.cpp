/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Additional changes by NoOne.
// Functions to handle both 3d and 2d editing modes.
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
#include <direct.h>

#include "compat.h"
#include "common_game.h"
#include "editor.h"
#include "xmpstub.h"
#include "sectorfx.h"
#include "tracker.h"
#include "edit2d.h"
#include "replace.h"
#include "screen.h"
#include "mapcmt.h"
#include "img2tile.h"
#include "tilefav.h"
#include "xmpsnd.h"
#include "fire.h"
#include "nnexts.h"
#include "preview.h"
#include "hglt.h"
#include "grdshd.h"
#include "aadjust.h"
#include "xmphud.h"
#include "xmpevox.h"
#include "xmpexplo.h"
#include "xmptools.h"
#include "xmparted.h"
#include "xmpseqed.h"
#include "xmpqaved.h"
#include "xmpconf.h"
#include "xmpmisc.h"
#include "xmpview.h"
#include "edit3d.h"

IniFile* gHints = NULL;

char h = 0;
BYTE key, ctrl, alt, shift;
short gHighSpr = -1, gHovSpr = -1;
short gHovWall = -1, gHovStat = -1;
int bpsx, bpsy, bpsz, bang, bhorz;
//int bsrcwall, bsrcsector, bsrcstat, bsrchit;

short temptype = 0, tempslope = 0, tempidx = -1, spriteNamesLength = 0;
char tempvisibility = 0;
short tempang = 0;

int cpyspritecnt = 0;
spritetype cpysprite[kMaxSprites + 1];
XSPRITE cpyxsprite[kMaxXSprites + 1];

int cpysectorcnt = 0;
sectortype cpysector[kMaxSectors + 1];
XSECTOR cpyxsector[kMaxXSectors + 1];

int cpywallcnt = 0;
walltype cpywall[kMaxWalls + 1];
XWALL cpyxwall[kMaxXWalls + 1];

char gMapInfoStr[kBufferSize];


char *gSpriteNames[1024];
char *gSpriteCaptions[1024];
char *gSpriteDataNames[1024][4];

char *gWallNames[1024];
char *gWallCaptions[1024];
char *gWallDataNames[1024][1];

char *gSectorNames[1024];
char *gSectorCaptions[1024];
char *gSectorDataNames[1024][1];

char *gCommandNames[256];
char buffer[kBufferSize]  = "";
char buffer2[kBufferSize] = "";
char buffer3[kBufferSize] = "";

char *gCaptionStyleNames[] = {
	
	"Disabled",
	"Mapedit style",
	"BUILD style",
	
};

static char* sharedTxt[] = {

	"Player", 			// 0
	"Upper ID", 		// 1
	"Lower ID", 		// 2
	"Gib1", 			// 3
	"Gib2",				// 4
	"Gib3", 			// 5
	"On Snd",			// 6
	"Off Snd",			// 7
	"Sound",			// 8
	"Rand Mul",			// 9
	"Stopping",			// 10
	"TG Swch",			// 11
	"1W Swch",			// 12
	"Toggle Switch",	// 13
	"1-Way Switch",		// 14
	"NML",				// 15
	"Normal",			// 16
	"None",				// 17
	"Sine",				// 18
	"Unnamed",			// 19
	"Path",				// 20
	"Ammo",				// 21
	"OFF",				// 22
	"ON",				// 23
	"*Velocity",		// 24
	"*Enemy",			// 25
	"Item",				// 26
	"Palette",			// 27
	"*Amount",			// 28
	"*Health",			// 29
	"*Ammo",			// 30
	"TXID",				// 31
	"TXID",				// 32
	"TXID",				// 33
	"TXID",				// 34
	"SEQ ID",			// 35
	"Where",			// 36
	"Data1",			// 37
	"Data2",			// 38
	"Data3",			// 39
	"Data4",			// 40
	"Weapon",			// 41
	"Max",				// 42
	"Condition",		// 43
	"Argument1",		// 44
	"Argument2",		// 45
	"Argument3",		// 46
	"~",				// 47
	"*Sound",			// 48

};

NAMED_XOBJECT spriteNames[] = {

	VC,	kDecoration,				sharedTxt[15],		"Decoration", 					{NULL, NULL, NULL, NULL},
	VC,	kMarkerSPStart,		 		NULL,				"Player Start",					{sharedTxt[0], NULL, NULL},
	VC,	kMarkerMPStart, 			NULL,				"Bloodbath Start",				{sharedTxt[0], "*Team", NULL, NULL},
	VC,	kMarkerLowLink,				NULL,				"Lower link",					{sharedTxt[1], NULL, NULL, NULL},
	VC,	kMarkerUpLink,				NULL,				"Upper link",					{sharedTxt[2], NULL, NULL, NULL},
	VC,	kMarkerLowWater,			NULL,				"Lower water",					{sharedTxt[1], NULL, NULL, NULL},
	VC,	kMarkerUpWater,				NULL,				"Upper water",					{sharedTxt[2], sharedTxt[27], NULL, NULL},
	VC,	kMarkerLowStack,			NULL,				"Lower stack",					{sharedTxt[1], NULL, NULL, NULL},
	VC,	kMarkerUpStack,				NULL,				"Upper stack",					{sharedTxt[2], NULL, NULL, NULL},
	VC,	kMarkerLowGoo,				NULL,				"Lower goo",					{sharedTxt[1], NULL, NULL, NULL},
	VC,	kMarkerUpGoo,				NULL,				"Upper goo",					{sharedTxt[2], sharedTxt[27], NULL, NULL},
	VC,	kMarkerPath,				"P",				"Path marker",					{"Id", "Next Id", NULL, NULL},
	VC,	kMarkerDudeSpawn,       	NULL,				"Dude Spawn",					{"Enemy", sharedTxt[25], sharedTxt[25], sharedTxt[25]},
	VC,	kMarkerEarthquake,      	NULL,				"Earthquake",					{"Strength", NULL, NULL, NULL},
	VC,	kSwitchToggle,				sharedTxt[11],		sharedTxt[13],					{sharedTxt[6], sharedTxt[7], NULL, NULL},
	VC,	kSwitchOneWay,				sharedTxt[12],		sharedTxt[14],					{sharedTxt[6], sharedTxt[7], NULL, NULL},
	VC,	kSwitchCombo,				"CM Swch",			"Combination Switch",			{"Current", "Goal", sharedTxt[42], sharedTxt[8]},
	VC,	kSwitchPadlock,				NULL,				"Padlock",						{NULL, NULL, NULL, NULL},
	MO,	kMarkerWarpDest,			"Teleport",			"Teleport target", 				{sharedTxt[0], "*Angle", sharedTxt[24], sharedTxt[48]},
	MO,	kDudeModernCustom,			NULL,				"Custom Dude",					{NULL, NULL, NULL, sharedTxt[29]},
	MO, kModernCustomDudeSpawn,		NULL,				"Custom Dude Spawn",			{NULL, NULL, NULL, sharedTxt[29]},
	MO, kModernSpriteDamager,		NULL,				"Damager",						{"TargetID", "DamageType", "Damage", NULL},
	MO,	kGenMissileEctoSkull,		NULL,				"Missile Gen", 					{"Missile", sharedTxt[24], "Slope", "Burst time"},
	MO,	kModernPlayerControl,		"Player Ctrl",		"Player Control",				{sharedTxt[0],  sharedTxt[47], sharedTxt[47], sharedTxt[47]},
	MO, kModernObjPropertiesChanger,"CHG Properties",	"Properties Changer",			{sharedTxt[37], sharedTxt[38], sharedTxt[39], sharedTxt[40]},
	MO, kModernObjDataChanger,		"CHG Data",			"Data Changer",					{"New data1", "New data2", "New data3", "New data4"},
	MO,	kModernObjDataAccumulator,	NULL,				"Inc - Dec",					{"For data", "Start", "End", "Step" },
	MO, kModernObjPicnumChanger,	"CHG Picture",		"Picture Changer",				{sharedTxt[37], sharedTxt[38], sharedTxt[39], sharedTxt[40]},
	MO, kModernSectorFXChanger,		"Sector Lighting",	"Sector Lighting Changer", 		{"FX Wave", "Amplitude", "Frequency", "Phase"},
	MO, kModernEffectSpawner,		NULL,				"Effect Gen",					{sharedTxt[9], "Effect", "Range", sharedTxt[36]},
	MO,	kModernWindGenerator,		"Wind Gen",			"Sector Wind Gen",				{"Randomness",	sharedTxt[24], "Enable pan", NULL},
	MO,	kModernCondition,			NULL,				"IF",							{sharedTxt[43], sharedTxt[44], sharedTxt[45], sharedTxt[46]},
	MO,	kModernConditionFalse,		NULL,				"IF NOT",						{sharedTxt[43], sharedTxt[44], sharedTxt[45], sharedTxt[46]},
	MO, kModernRandomTX,			NULL,				"RandomTX",						{sharedTxt[31], sharedTxt[32], sharedTxt[33], sharedTxt[34]},
	MO, kModernSequentialTX,		"Seq TX",			"SequentialTX",					{sharedTxt[31], sharedTxt[32], sharedTxt[33], sharedTxt[34]},
	MO, 504,						"CHG Slope",		"Slope Changer",				{"Where", "Slope", NULL, NULL},
	MO, 506,						"CHG Velocity",		"Velocity Changer",				{"XVelocity", "YVelocity", "ZVelocity", "Range"},
	MO, 16,							"Stealth",			"Stealth region",				{"Radius", "%% To Hear",	"%% To See", NULL},
	MO, 4,							NULL,				"Marker",						{NULL, NULL, NULL, NULL},
	MO, kModernSeqSpawner,			"Spawn SEQ",		"SEQ Spawner",					{sharedTxt[9],  sharedTxt[35], sharedTxt[36], sharedTxt[8]},
	MO, kModernObjSizeChanger,  	"Resize",			"Resizer",						{sharedTxt[37], sharedTxt[38], sharedTxt[39], sharedTxt[40]},
	MO, kModernDudeTargetChanger, 	"AI Fight",			"Enemy Target Changer",			{"Trgt data1", "Fight mode", "Force trgt", "Behavior"},
	MO,	kItemShroomGrow,        	NULL,				"Grow shroom",					{NULL, NULL, NULL, NULL},
	MO,	kItemShroomShrink,      	NULL,				"Shrink shroom",				{NULL, NULL, NULL, NULL},
	MO, 40,							NULL,				"Random Item",					{sharedTxt[26], sharedTxt[26], sharedTxt[26], sharedTxt[26]},
	MO, 150,						NULL,				"Map",							{NULL, NULL, NULL, NULL},
	VC,	kDecorationTorch,			NULL,				"Torch", 						{NULL, NULL, NULL, NULL},
	VC,	kDecorationCandle,			NULL,				"Candle", 						{NULL, NULL, NULL, NULL},
	VC,	kItemWeaponFlarePistol,		NULL,				"Flare Pistol",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemWeaponSawedoff,		NULL,				"Sawed-off",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemWeaponTommygun,		NULL,				"Tommy Gun",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemWeaponNapalmLauncher,	NULL,				"Napalm Launcher",				{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemWeaponTeslaCannon,		NULL,				"Tesla Cannon",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemWeaponLifeLeech,		NULL,				"Life Leech",					{sharedTxt[30], NULL, NULL, NULL},

	VC,	kItemAmmoSprayCan,			NULL,				"Spray Can",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoTNTBundle,			"TNT Bundle",		"Bundle of TNT",				{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoTNTBox,			"TNT Case",			"Case of TNT",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoProxBombBundle,	"Prox Bomb",		"Proximity Detonator",			{sharedTxt[30], NULL, NULL, NULL},
 	VC,	kItemAmmoRemoteBombBundle,	"Remote Bomb",		"Remote Detonator",				{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoSawedoffFew,		"4 shells",			"4 shotgun shells",				{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoSawedoffBox,		"Box of shells",	"Box of shotgun shells",		{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoTommygunFew,		NULL,				"A few bullets",				{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoVoodooDoll,		NULL,				"Voodoo Doll",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoTommygunDrum,		"Drum of bullets",	"Full drum of bullets",			{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoTeslaCharge,		NULL,				"Tesla Charge",					{sharedTxt[30], NULL, NULL, NULL},
	VC,	kItemAmmoFlares,			NULL,				"Flares",						{sharedTxt[30], NULL, NULL, NULL},
 	VC,	kItemAmmoGasolineCan,		NULL,				"Gasoline Can",					{sharedTxt[30], NULL, NULL, NULL},
 	VC,	kItemAmmoTrappedSoul,		NULL,				"Trapped Soul",					{sharedTxt[30], NULL, NULL, NULL},

	VC,	kItemKeySkull,          	NULL,				"Skull key",					{NULL, NULL, NULL, NULL},
	VC,	kItemKeyEye,           		NULL,				"Eye key",						{NULL, NULL, NULL, NULL},
	VC,	kItemKeyFire,           	NULL,				"Fire key",						{NULL, NULL, NULL, NULL},
	VC,	kItemKeyDagger,         	NULL,				"Dagger key",					{NULL, NULL, NULL, NULL},
	VC,	kItemKeySpider,         	NULL,				"Spider key",					{NULL, NULL, NULL, NULL},
	VC,	kItemKeyMoon,           	NULL,				"Moon key",						{NULL, NULL, NULL, NULL},
	VC,	kItemKey7,              	NULL,				"key 7",						{NULL, NULL, NULL, NULL},
	VC,	kItemHealthDoctorBag,   	NULL,				"Doctor's Bag",					{NULL, NULL, NULL, NULL},
	VC,	kItemHealthMedPouch,    	"Pouch",			"Medicine Pouch",				{sharedTxt[28], NULL, NULL, NULL},
	VC,	kItemHealthLifeEssense, 	NULL,				"Life Essence",					{sharedTxt[28], NULL, NULL, NULL},
	VC,	kItemHealthLifeSeed,    	NULL,				"Life Seed",					{sharedTxt[28], NULL, NULL, NULL},
	VA,	kItemHealthRedPotion,   	"Potion",			"Red Potion",					{sharedTxt[28], NULL, NULL, NULL},
	VA,	kItemFeatherFall,       	NULL,				"Feather Fall",					{NULL, NULL, NULL, NULL},
	VC,	kItemShadowCloak,   		NULL,				"ShadowCloak",					{NULL, NULL, NULL, NULL},
	VC,	kItemDeathMask,   			NULL,				"Death mask",					{NULL, NULL, NULL, NULL},
	VC,	kItemJumpBoots,				NULL,				"Boots of Jumping",				{NULL, NULL, NULL, NULL},
	VC,	kItemTwoGuns,        		NULL,				"Guns Akimbo",					{NULL, NULL, NULL, NULL},
	VC,	kItemDivingSuit,        	NULL,				"Diving Suit",					{NULL, NULL, NULL, NULL},
	VA,	kItemGasMask,           	NULL,				"Gas mask",						{NULL, NULL, NULL, NULL},
	VC,	kItemCrystalBall,       	NULL,				"Crystal Ball",					{NULL, NULL, NULL, NULL},
	VA,	kItemDoppleganger,     		NULL,				"Doppleganger",					{NULL, NULL, NULL, NULL},
	VC,	kItemReflectShots,   		NULL,				"Reflective shots",				{NULL, NULL, NULL, NULL},
	VC,	kItemBeastVision,       	NULL,				"Beast Vision",					{NULL, NULL, NULL, NULL},
	VC,	kItemShroomDelirium,    	NULL,				"Delirium Shroom",				{NULL, NULL, NULL, NULL},
	VC,	kItemTome,              	NULL,				"Tome",							{NULL, NULL, NULL, NULL},
	VC,	kItemBlackChest,        	NULL,				"Black Chest",					{NULL, NULL, NULL, NULL},
	VC,	kItemWoodenChest,      		NULL,				"Wooden Chest",					{NULL, NULL, NULL, NULL},
	VC,	kItemArmorAsbest,			NULL,				"Asbestos Armor",				{NULL, NULL, NULL, NULL},
	VC,	kItemArmorBasic,			NULL,				"Basic Armor",					{NULL, NULL, NULL, NULL},
	VC,	kItemArmorBody,				NULL,				"Body Armor",					{NULL, NULL, NULL, NULL},
	VC,	kItemArmorFire,				NULL,				"Fire Armor",					{NULL, NULL, NULL, NULL},
	VC,	kItemArmorSpirit,			NULL,				"Spirit Armor",					{NULL, NULL, NULL, NULL},
	VC,	kItemArmorSuper,			NULL,				"Super Armor",					{NULL, NULL, NULL, NULL},
	VC,	kItemFlagABase,				NULL,				"Blue Team Base",				{NULL, NULL, NULL, NULL},
	VC,	kItemFlagBBase,				NULL,				"Red Team Base",				{NULL, NULL, NULL, NULL},

	VC,	kDudeCultistShotgun,		"Shotgun Cultist",	"Cultist w/Shotgun", 			{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistShotgunProne,	NULL,				"SCultist prone", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistTommy, 			"Tommy Cultist",	"Cultist w/Tommy", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistTommyProne,		NULL,				"TCultist prone", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistTesla,			"Tesla Cultist",	"Cultist w/Tesla", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistTNT,			"TNT Cultist",		"Cultist w/Dynamite", 			{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeZombieAxeNormal,   	NULL,				"Axe Zombie", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeZombieAxeBuried,  		NULL,				"Earth Zombie", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeZombieAxeLaying,  		NULL,				"Sleep Zombie", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeZombieButcher,    		"Butcher",			"Bloated Butcher",				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeGargoyleFlesh,			NULL,				"Flesh Gargoyle", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeGargoyleStatueFlesh,  	NULL,				"Flesh Statue", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudePhantasm,     			NULL,				"Phantasm", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeHellHound,        		"Hound",			"Hell Hound", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeGillBeast,    			NULL,				"GillBeast", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudePodGreen,     			NULL,				"Green Pod", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeTentacleGreen,			NULL,				"Green Tentacle", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudePodFire,      			NULL,				"Fire Pod", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeTentacleFire, 			NULL,				"Fire Tentacle", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeHand,         			NULL,				"Hand", 						{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeRat,          			NULL,				"Rat", 							{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeBoneEel,          		NULL,				"Eel", 							{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeBat,          			NULL,				"Bat", 							{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeSpiderBrown,  			NULL,				"Brown Spider", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeSpiderRed,    			NULL,				"Red Spider", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeSpiderBlack,  			NULL,				"Black Spider", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeInnocent,  			NULL,				"Innocent", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeTinyCaleb,				NULL,				"Tiny Caleb", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeGargoyleStatueStone, 	NULL,				"Stone Statue", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeGargoyleStone,			NULL,				"Stone Gargoyle", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeSpiderMother, 			NULL,				"Mother Spider", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCerberusTwoHead,   	NULL,				"Cerberus", 					{NULL, NULL, NULL, sharedTxt[29]},
	VA,	kDudeCerberusOneHead,		NULL,				"1 Head Cerberus",				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeTchernobog,   			NULL,				"Tchernobog", 					{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeCultistBeast,			NULL,				"Beast Cultist", 				{NULL, NULL, NULL, sharedTxt[29]},
	VC,	kDudeBeast,					NULL,				"Beast", 						{NULL, NULL, NULL, sharedTxt[29]},
	VA,	225,						NULL,				"Fake Dude",					{NULL, NULL, NULL, sharedTxt[29]},

	VC,	kThingTNTBarrel,			NULL,				"TNT Barrel", 					{NULL, NULL, NULL, NULL},
	VA,	kThingArmedTNTStick,		"Armed TNT S",		"Armed TNT Stick",				{NULL, NULL, NULL, NULL},
	VA,	kThingArmedSpray,			NULL,				"Armed Spray",					{NULL, NULL, NULL, NULL},
	VC,	kThingArmedProxBomb,		"Armed Prox",		"Armed Prox Bomb", 				{NULL, NULL, NULL, NULL},
	VC,	kThingArmedRemoteBomb,		NULL,				"Armed Remote", 				{NULL, NULL, NULL, NULL},
	VC,	kThingCrateFace,			NULL,				"Crate Face",					{NULL, NULL, NULL, NULL},
	VC,	kThingFluorescent,			"Flr Light",		"Fluorescent Light",			{NULL, NULL, NULL, NULL},
	VC,	kThingWallCrack,			"Crack",			"Wall Crack", 					{NULL, NULL, NULL, NULL},
	VC,	kThingWoodBeam,				"Beam",				"Wood Beam", 					{NULL, NULL, NULL, NULL},
	VC,	kThingSpiderWeb,			"Web",				"Spider's Web",					{NULL, NULL, NULL, NULL},
	VC,	kThingMetalGrate,			"Grate",			"Metal Grate", 					{NULL, NULL, NULL, NULL},
	VC,	kThingFlammableTree,		"Tree",				"Flammable Tree", 				{"Burn anim", NULL, NULL, sharedTxt[8]},
	VC,	kTrapMachinegun,			NULL,				"Machine Gun", 					{"Ammo max", sharedTxt[21], NULL, NULL},
	VC,	kThingFallingRock,			"Rock",				"Falling Rock", 				{NULL, NULL, NULL, NULL},
	VC,	kThingKickablePail,			"Pail",				"Kickable Pail", 				{NULL, NULL, NULL, NULL},
	VC,	kThingObjectGib,			"G-Obj",			"Gib Object",					{sharedTxt[3], sharedTxt[4], sharedTxt[5], sharedTxt[8]},
	VC,	kThingObjectExplode,		"E-Obj",			"Explode Object",				{sharedTxt[3], sharedTxt[4], sharedTxt[5], sharedTxt[8]},
	VA,	426,						NULL,				"Falling Gib",					{sharedTxt[3], sharedTxt[4], sharedTxt[5], sharedTxt[8]},
	VC,	kThingZombieHead,			"Head",				"Zombie Head", 					{"Kick times", NULL, NULL, sharedTxt[8]},
	VA,	428,						NULL,				"Napalm Trap",					{NULL, NULL, NULL, "Divisions"},
	VA,	430,						NULL,				"Acid Trap",					{NULL, NULL, NULL, NULL},
	VA,	431,						"Armed Leech",		"Armed Life Leech",				{NULL, NULL, sharedTxt[21], NULL},

	VC,	kTrapFlame,					"Flamer",			"Flame Trap", 					{"Flame Dist", NULL, NULL, NULL},
	VC,	kTrapSawCircular,			NULL,				"Saw Blade", 					{NULL, NULL, NULL, NULL},
	VC,	kTrapZapSwitchable,			"Zap",				"Electric Zap", 				{NULL, NULL, NULL, NULL},
	VC,	kTrapPendulum,				NULL,				"Pendulum", 					{NULL, NULL, NULL, NULL},
	VC,	kTrapGuillotine,			NULL,				"Guillotine", 					{NULL, NULL, NULL, NULL},
	VC,	kTrapExploder,				"Exploder", 		"Hidden Exploder",				{"*Explosion", "*SEQ Id", sharedTxt[48], "*Radius"},

	VC,	kGenTrigger,				"Trigger",			"Trigger Gen", 					{sharedTxt[9], NULL, NULL, NULL},
	VC,	kGenDripWater,				"Water Drip",		"WaterDrip Gen", 				{sharedTxt[9], NULL, NULL, NULL},
	VC,	kGenDripBlood,				"Blood Drip",		"BloodDrip Gen", 				{sharedTxt[9], NULL, NULL, NULL},
	VC,	kGenMissileFireball,		"Fireball",			"Fireball Gen", 				{sharedTxt[9], "Fire anim", NULL, NULL},
	VC,	kGenBubble,					"Bubble",			"Bubble Gen", 					{sharedTxt[9], NULL, NULL, NULL},
	VC,	kGenBubbleMulti,			"Bubbles",			"Multi-Bubble Gen", 			{sharedTxt[9], NULL, NULL, NULL},
	VC,	kGenSound,					"SFX",				"SFX Gen", 						{sharedTxt[9], sharedTxt[8], "*Volume", "*Pitch"},
	VC,	kSoundSector,				"SSFX",				"Sector SFX", 					{"Off->On", sharedTxt[10], "On->Off", sharedTxt[10]},
	VC,	kSoundAmbient,				NULL,				"Ambient SFX", 					{"Radius1", "Radius2", sharedTxt[8], "Volume"},
	VC,	kSoundPlayer,				NULL,				"Player SFX", 					{sharedTxt[8], NULL, NULL, NULL},

};

// player control data names
SPECIAL_DATA_NAMES pCtrlDataNames[32] = {

	64, { NULL, "Race", NULL, NULL },
	65, { NULL, "Move speed", "Jmp height", NULL },
	66, { NULL, "Effect", "Length", NULL },
	67, { NULL,	"QAV Id", "Interrupt", "Force play" },
	68, { NULL,	"QAV Id", NULL, NULL },
	69, { NULL, "Slope", NULL, NULL },
	70, { NULL, "Clear", NULL, NULL },
	71, { NULL, "Give",  "Ammo/Weapn", "Additional" },
	72, { NULL, "Pack item", "Additional", NULL },
	73, { NULL, "Angle", NULL, NULL },
	74, { NULL, "Powerup", "Duration", NULL },

};

static NAMED_XOBJECT wallNames[] = {

	VC,	0,			 	sharedTxt[15],	sharedTxt[16],		{NULL, NULL, NULL, NULL},
	VC,	kSwitchToggle,	sharedTxt[11],	sharedTxt[13],		{NULL, NULL, NULL, NULL},
	VC,	kSwitchOneWay,	sharedTxt[12],	sharedTxt[14],		{NULL, NULL, NULL, NULL},
	VC,	kWallStack,		"Stack",		"Wall stack",		{"Stack Id", NULL, NULL, NULL},
	VC,	kWallGib,		"Gib",			"Gib wall",			{"Gib Id", NULL, NULL, NULL},

};


static NAMED_XOBJECT sectorNames[] = {

	VC,	0,						sharedTxt[15],		sharedTxt[16], 			{NULL, NULL, NULL, NULL},
	VC,	kSectorZMotion,			NULL,				"Z Motion", 			{NULL, NULL, NULL, NULL},
	VC,	kSectorZMotionSprite,	"Z Motion SPR",		"Z Motion SPRITE", 		{NULL, NULL, NULL, NULL},
	VC,	kSectorTeleport,		NULL,				"Teleporter", 			{NULL, NULL, NULL, NULL},
	VC,	kSectorSlideMarked,		NULL,				"Slide Marked", 		{NULL, NULL, NULL, NULL},
	VC,	kSectorRotateMarked,	NULL,				"Rotate Marked", 		{NULL, NULL, NULL, NULL},
	VC,	kSectorSlide,			NULL,				"Slide",				{NULL, NULL, NULL, NULL},
	VC,	kSectorRotate,			NULL,				"Rotate",				{NULL, NULL, NULL, NULL},
	VC,	kSectorRotateStep,		NULL,				"Step Rotate",			{NULL, NULL, NULL, NULL},
	VC,	kSectorPath,			sharedTxt[20],		"Path Sector", 			{"Path Id", NULL, NULL, NULL},
	VC,	kSectorDamage,			"Damage",			"Damage Sector", 		{"Damage", NULL, NULL, NULL},
	VC,	kSectorCounter,			"Counter",			"Counter Sector",		{"Sprite type", NULL, NULL, NULL},

};

char* gBoolNames[2] = {

	sharedTxt[22],		// 0
	sharedTxt[23],		// 1

};

char* onOff(int var) {

	return gBoolNames[(var) ? 1 : 0];

}


char* gIsNotNames[2] = {

	"is not",			// 0
	"is",				// 1

};

char* isNot(int var) {

	return gIsNotNames[(var) ? 1 : 0];

}

char* gYesNoNames[2] = {

	"no",				// 0
	"yes",				// 1

};

char* yesNo(int var) {

	return gYesNoNames[(var) ? 1 : 0];

}


char* gBusyNames[4] = {

	sharedTxt[18],		// 0 kBusySine
	"Linear", 			// 1 kBusyLinear
	"SlowOff",			// 2 kBusySlowOff
	"SlowOn",			// 3 kBusySlowOn

};

char* gWaveNames[12] = {
	sharedTxt[17],		// 0 kWaveNone
	"Square",			// 1 kWaveSquare
	"Saw",				// 2 kWaveSaw
	"Ramp up",			// 3 kWaveRampup
	"Ramp down",		// 4 kWaveRampdown
	sharedTxt[18],		// 5 kWaveSine
	"Flicker1",			// 6 kWaveFlicker1
	"Flicker2",			// 7 kWaveFlicker2
	"Flicker3",			// 8 kWaveFlicker3
	"Flicker4",			// 9 kWaveFlicker4
	"Strobe",			// 10 kWaveStrobe
	"Search",			// 11 kWaveSearch

};

char* gExtNames[] = {

	"map",
	"seq",
	"art",
	"qav",

};

NAMED_TYPE gToolNames[] = {

	{kToolMapEdit, 		"MAPedit"},
	{kToolSeqEdit, 		"SEQedit"},
	{kToolArtEdit,		"ARTedit"},
	{kToolQavEdit,		"QAVedit"},

};

NAMED_TYPE gInitCommandNames[] = {

	{kCmdOff, 			sharedTxt[22]},
	{kCmdOn,  			sharedTxt[23]},
	{kCmdState, 		"State"},
	{kCmdToggle, 		"Toggle"},
	{kCmdNotState, 		"!State"},
	{kCmdLink, 			"Link"},
	{kCmdLock, 			"Lock"},
	{kCmdUnlock, 		"Unlock"},
	{kCmdToggleLock, 	"Toggle Lock"},
	{kCmdStopOff,		"Sector Stop OFF"},
	{kCmdStopOn, 		"Sector Stop ON"},
	{kCmdStopNext, 		"Sector Stop Next"},
	{13, 				"Sector Pause"},
	{14, 				"Sector Continue"},
	{15, 				"Set dude flags"},
	{16,				"Kill events (full)"},
	// ... numberic commands up to length?

};

NAMED_TYPE gSectorMergeMenu[] = {
	
	{0, "&Just make a new sector"},
	{1, "&Make and merge with outer sector"},
	{2, "&Cancel"},
	
};

char* gRespawnNames[4] = {

	"Optional",				// 0
	"Never",				// 1
	"Always",				// 2
	"Permanent",			// 3

};

char* gDepthNames[8] = {
	
	"None",					// 0
	"Tread",				// 1
	"Puddle",				// 2
	"Wade",					// 3
	"Pond",					// 4
	"Bath",					// 5
	"Jungle",				// 6
	"Swim",					// 7
	
};

char* gDamageNames[7] = {
	
	"None",					// 0
	"Fall",					// 1
	"Burn",					// 2
	"Vector",				// 3
	"Explode",				// 4
	"Choke",				// 5
	"Electric",				// 6
	
};


char *gKeyItemNames[] = {

	sharedTxt[17],			// 0
	"Skull",				// 1
	"Eye",					// 2
	"Fire",					// 3
	"Dagger",				// 4
	"Spider",				// 5
	"Moon",					// 6
	"key 7",				// 7

};

char* gSearchStatNames[] = {

	"Wall",					// 0, OBJ_WALL
	"Ceiling",				// 1, OBJ_CEILING
	"Floor",				// 2, OBJ_FLOOR
	"Sprite",				// 3, OBJ_SPRITE
	"Masked wall",			// 4, OBJ_MASKED
	gSearchStatNames[3],	// 5, OBJ_FLATSPRITE
	"Sector",				// 6
	"Unknown",				// 7

};

NAMED_TYPE gGameObjectGroupNames[] = {

	{ kOGrpDude, 	"Enemy" },
	{ kOGrpWeapon, 	"Weapon" },
	{ kOGrpAmmo, 	"Ammo" },
	{ kOGrpAmmoMix, "Ammo" },
	{ kOGrpItem, 	"Item" },
	{ kOGrpHazard, 	"Hazard" },
	{ kOGrpMisc, 	"Misc" },
	{ kOGrpMarker, 	"Marker" },

};

char *gZModeNames[4] = {

	"Gravity",			// 0
	"Step",				// 1
	"Free",				// 2
	"Mouse look",		// 3

};

void ExtPreLoadMap(void) { }
void ExtLoadMap(const char *mapname) { }
void ExtPreSaveMap(void) { }
void ExtSaveMap(const char *mapname) { }
void qdraw2dscreen(int x, int y, short nAngle, int nZoom, short nGrid );
void qsetvgapalette(void);
void HookReplaceFunctions(void);
void ProcessKeys3D();
void ProcessKeys2D();


void InitializeNames( void ) {

	spriteNamesLength = LENGTH(spriteNames);

	int i = 0, j = 0, k = 0;

	static char numbers[192][4];
	memset(numbers, 0, sizeof(numbers));

	// fill sprite type names, captions and data field names
	for (i = 0; i < spriteNamesLength; i++)
	{
		NAMED_XOBJECT* pObj = &spriteNames[i];
		if (gMisc.showTypes < pObj->compt)
			continue;

		gSpriteNames[pObj->id]    = pObj->name;
		gSpriteCaptions[pObj->id] = (gMisc.useCaptions && pObj->caption) ? pObj->caption : pObj->name;

		for (j = 0; j < 4; j++)
		{
			// * = modern compatibility
			if (pObj->dataOvr[j] && pObj->dataOvr[j][0] == kPrefixModernOnly)
			{
				if (gMisc.showTypes != MO) continue;
				pObj->dataOvr[j] =& pObj->dataOvr[j][1];
			}

			gSpriteDataNames[pObj->id][j] = spriteNames[i].dataOvr[j];
		}
	}

	// fill wall type names, captions and data field names
	for (i = 0; i < LENGTH(wallNames); i++)
	{
		NAMED_XOBJECT* pObj = &wallNames[i];
		gWallNames[pObj->id]    = pObj->name;
		gWallCaptions[pObj->id] = (gMisc.useCaptions && pObj->caption) ? pObj->caption : pObj->name;
		gWallDataNames[pObj->id][0] = pObj->dataOvr[0];
	}

	// fill sector type names, captions and data field names
	for (i = 0; i < LENGTH(sectorNames); i++)
	{
		NAMED_XOBJECT* pObj = &sectorNames[i];
		gSectorNames[pObj->id]      = pObj->name;
		gSectorCaptions[pObj->id] 	= (gMisc.useCaptions && pObj->caption) ? pObj->caption : pObj->name;
		gSectorDataNames[pObj->id][0] = pObj->dataOvr[0];
	}

	// fill the command names
	for (i = 0; i < LENGTH(gInitCommandNames); i++)
	{
		j = gInitCommandNames[i].id;
		if (j > 11 && gMisc.showTypes < MO)
			continue;

		gCommandNames[j] = gInitCommandNames[i].name;
	}


	// fill numbers for numberic commands
	for (i = 0; i < 192; i++)
	{
		sprintf(numbers[i], "%d", i);
		gCommandNames[64 + i] = numbers[i];
	}
}



int GetXSector( int nSector )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	if (sector[nSector].extra != -1)
		return sector[nSector].extra;

	return dbInsertXSector(nSector);
}


int GetXWall( int nWall )
{
	int nXWall;

	dassert(nWall >= 0 && nWall < kMaxWalls);

	if (wall[nWall].extra > 0)
		return wall[nWall].extra;

	nXWall = dbInsertXWall(nWall);
	return nXWall;
}

int GetXSprite( int nSprite )
{
	int nXSprite;

	dassert(nSprite >= 0 && nSprite < kMaxSprites);

	if (sprite[nSprite].extra > 0)
		return sprite[nSprite].extra;

	nXSprite = dbInsertXSprite(nSprite);
	return nXSprite;
}

char *ExtGetSectorCaption(short nSector)
{
	int i = 0, j;
	int nType = sector[nSector].type, nXSector = sector[nSector].extra;
	char* to = buffer, *typeName = NULL;
	
	to[0] = '\0';
	if (showtags == kCaptionStyleMapedit && nXSector > 0)
	{
		if (xsector[nXSector].rxID > 0) i += sprintf(&to[i], "%i: ", xsector[nXSector].rxID);
		if (nType >= 0 && nType < LENGTH(gSectorCaptions) && gSectorCaptions[nType])
			i += sprintf(&to[i], gSectorCaptions[nType]);
		else 
			i += sprintf(&to[i], "<<T%d>>", nType);

		if (xsector[nXSector].txID > 0) i += sprintf(&to[i], ": %d", xsector[nXSector].txID);
		if (xsector[nXSector].panVel != 0)
			i += sprintf(&to[i], " PAN(%d,%d)", xsector[nXSector].panAngle, xsector[nXSector].panVel);
		
		i += sprintf(&to[i], " %s", gBoolNames[xsector[nXSector].state]);

	}
	else if (nType || sector[nSector].hitag)
	{
		i += sprintf(&to[i], "{%i:%i}", sector[nSector].hitag, nType);
	}
	
	if (gPreviewMode)
	{
		for (j = 0; j < gBusyCount; j++)
		{
			if (gBusy[j].at0 != nSector) continue;
			i += sprintf(&to[i], " (%d%%)", (100*klabs(xsector[nXSector].busy)) / 65536);
			break;
		}
	}

	return to;
}

char *ExtGetWallCaption(short nWall) {

	int i = 0, nType = wall[nWall].type, nXWall = wall[nWall].extra;
	char* to = buffer;
	to[0] = '\0';

	if (showtags == kCaptionStyleMapedit && nXWall > 0)
	{
		if (xwall[nXWall].rxID > 0)
			i += sprintf(&to[i], "%d: ", xwall[nXWall].rxID);
		
		if (nType >= 0 && nType < LENGTH(gWallCaptions) && gWallCaptions[nType])
			i += sprintf(&to[i], gWallCaptions[nType]);
		else
			i += sprintf(&to[i], "<<T%d>>", nType);
		
		if (xwall[nXWall].txID > 0)
			i += sprintf(&to[i], ": %d", xwall[nXWall].txID);
		
		if (xwall[nXWall].panXVel != 0 || xwall[nXWall].panYVel != 0)
			i += sprintf(&to[i], " PAN(%i,%i)", xwall[nXWall].panXVel, xwall[nXWall].panYVel);

		i += sprintf(&to[i], " %s", gBoolNames[xwall[nXWall].state]);

	}
	else if (nType || wall[nWall].hitag)
	{
		sprintf(to, "{%i:%i}", wall[nWall].hitag, nType);
	}
	
	return to;
}

char *ExtGetSpriteCaption( short nSprite ) {

	spritetype *pSprite = &sprite[nSprite];
	static char rx[6], name[32], midl[32], tx[6];
	short nType = pSprite->type;
	char *typeName = NULL, *to = buffer;
	int i;

	to[0] = '\0';
	
	if (showtags == kCaptionStyleBuild)
	{
		if (nType || pSprite->hitag)
			sprintf(to, "{%i:%i}", pSprite->hitag, nType);

		return to;
	}
	
	switch (pSprite->statnum) {
		case kStatMarker:
		case kStatAmbience:
		case kStatFX:
			return "";
		case kStatProjectile:
			sprintf(name, "MISSILE%d", nType - kMissileBase);
			return name;
		case kStatExplosion:
			sprintf(name, "EXPLOSION%d", nType);
			return name;
	}
	

	if (!nType && pSprite->extra <= 0) return "";

	name[0] = '\0';
	if (nType < 0 || nType >= LENGTH(gSpriteCaptions) || (typeName = gSpriteCaptions[nType]) == NULL)
		sprintf(name, "<<T%i>>", nType);
	else
		sprintf(name, "%s", typeName);

	if (pSprite->extra <= 0)
		return name;

	XSPRITE *pXSprite = &xsprite[pSprite->extra];
	midl[0] = rx[0] = tx[0] = 0;

	switch (nType) {
		case kGenSound:
			if (pXSprite->data2 <= 0) break;
			else sprintf(midl, "%s [%i]", name, pXSprite->data2);
			break;
		case kDudeModernCustom:
			if (pXSprite->sysData4 >= 2)
			{
				if (gCustomDudeNames[pSprite->index][0])
					sprintf(name, gCustomDudeNames[pSprite->index]);
			}
			break;
		case kModernCondition:
		case kModernConditionFalse:
			if (pXSprite->busyTime > 0)
			{
				sprintf(name, "LOOP");
				if (nType == kModernConditionFalse)
					strcat(name, " NOT");
			}
/* 			if (pXSprite->data1 > 0) {

				for (i = 0; i < LENGTH(gCondCapt); i++) {
					if (pXSprite->data1 != gCondCapt[i].id)
						continue;

					strcat(name, " ");
					strcat(name, "[");
					strcat(name, strupr(gCondCapt[i].caption));

					if (!gCondCapt[i].isBool) {

						i = (pSprite->cstat & kSprBlock);
						if (pSprite->cstat & kSprMoveForward) sprintf(midl, "%s%d", i ? ">" : ">=", pXSprite->data2);
						else if (pSprite->cstat & kSprMoveReverse) sprintf(midl, "%s%d", i ? "<" : "<=", pXSprite->data2);
						else if (i) sprintf(midl, ">=%d & <=%d",pXSprite->data2, pXSprite->data3);
						else sprintf(midl, "=%d",pXSprite->data2);
						strcat(name, midl);
						midl[0] = 0;

					}

					strcat(name, "]");
					break;
				}

			} */
			break;
		case kModernObjDataAccumulator:
			if (pXSprite->data2 < pXSprite->data3) sprintf(name, "INC");
			else if (pXSprite->data2 > pXSprite->data3) sprintf(name, "DEC");
			break;
		case kModernSequentialTX:
			if (!(pSprite->flags & 0x01)) break;
			strcat(name, " ");
			strcat(name, "[All]");
			break;
		case kMarkerPath:
			if (pXSprite->data2 < 0) {

				sprintf(midl, "%s[%i->%s]", name, pXSprite->data1, (pXSprite->data2 == -1) ? "END" : "BCK");
				break;

			} else if (pXSprite->data1 != pXSprite->data2) {

				// check if destination marker exist
				for (i = headspritestat[kStatPathMarker]; i != -1; i = nextspritestat[i]) {
					spritetype *pSprite2 = &sprite[i];
					if (pSprite2->extra > 0 && xsprite[pSprite2->extra].data1 == pXSprite->data2) {
						sprintf(midl, "%s[%i->%i]", name, pXSprite->data1, pXSprite->data2);
						break;
					}
				}

				// destination marker is not found
				if (i == -1) sprintf(midl, "%s[%i->%i??]", name, pXSprite->data1, pXSprite->data2);
				break;

			}
			// no break
		default:
			if ((nType == 0) || (nType >= 3 && nType <= 5)) break;
			else if (nType < kMarkerEarthquake && pXSprite->data1 > 0) sprintf(midl, "%s [%i]", name, pXSprite->data1);
			break;
	}

	if (pXSprite->rxID > 0) sprintf(rx, "%i: ", pXSprite->rxID);
	if (pXSprite->txID > 0) {

		sprintf(tx, ": %i", pXSprite->txID);

		switch (pXSprite->txID) {
			case 1:
				if (pXSprite->command < kCmdNumberic) break;
				sprintf(midl, "SECRETS_COUNTER [%d]", pXSprite->command - kCmdNumberic);
				break;
			case 60:
				if (pXSprite->txID != pXSprite->rxID || pXSprite->command != 100) break;
				sprintf(midl, "ENABLE_MODERN_FEATURES");
				break;
		}

	}

	sprintf(to, "%s%s%s", rx, (!midl[0]) ? name : midl, tx);
	if (rngok(nType, kSwitchBase, kSwitchMax))
	{
		strcat(to, " ");
		strcat(to, gBoolNames[pXSprite->state]);
	}
	
	if (gPreviewMode)
	{
		if (rngok(nType, kModernCondition, kModernConditionFalse))
		{
			if (!pXSprite->state)
			{
				strcat(to, " [!]");
			}
		}
	}
	
	return to;

}

void faketimerhandler( void )
{
	sampletimer();
}

/***********************************************************************
 * ExtPreCheckKeys()
 *
 * Called before drawrooms, drawmasks and nextpage in 3D mode.
 *
 **********************************************************************/
void ExtPreCheckKeys(void)
{
	if (qsetmode == 200)	// in 3D mode
	{
		FireProcess();	// animate dynamic fire
		DoSectorLighting();	// animate sector lighting
		if (gMisc.pan && totalclock >= gTimers.pan + 4)
		{
			// animate panning sectors
			gTimers.pan = totalclock;
			DoSectorPanning();
		}
		
		clearview(gStdColor[29]);
	}
}

void ExtAnalyzeSprites(void)
{
	if (gPreviewMode)
	{
		// backup position before drawrooms
		bpsx = posx, bpsy = posy, bpsz = posz;
		bang = ang, bhorz = horiz;
	
		if (gPreview.scrEffects[kScrEffectQuake1])
			viewDoQuake(gPreview.scrEffects[kScrEffectQuake1], &posx, &posy, &posz, &ang, &horiz);

		if (gPreview.scrEffects[kScrEffectQuake2])
			viewDoQuake(gPreview.scrEffects[kScrEffectQuake2], &posx, &posy, &posz, &ang, &horiz);
		
		// makes no sense...
		// ...but this is how we can get proper searchstat
		DrawMirrors(posx, posy, posz, ang, horiz);
		drawrooms(posx, posy, posz, ang, horiz, cursectnum);
		viewProcessSprites(posx, posy, posz, ang);
		
		// restore position after drawrooms
		posx = bpsx, posy = bpsy, posz = bpsz;
		ang = bang, horiz = bhorz;
	}
	else
	{
		viewProcessSprites(posx, posy, posz, ang);
	}
}

void process2DMode() {

	if (!gPreviewMode && gCompat.indicate)
	{
		QFONT* pFont = qFonts[0];
		static char compat[16] = "WAIT";
		static int tcolor = kColorLightGray, bcolor = kColorDarkGray;
		int x, y, len, pad = 4;

		if (totalclock >= gTimers.compatCheck + 256)
		{
			if (isModernMap())
			{
				tcolor = kColorRed;
				bcolor = kColorYellow;
				sprintf(compat, "MODERN");
			}
			else
			{
				tcolor = kColorWhite;
				bcolor = kColorGreen;
				sprintf(compat, "VANILLA");
			}

			gTimers.compatCheck = totalclock;
		}

		if (totalclock >= gTimers.diskPic)
		{
			len = gfxGetTextLen(compat, pFont);

			pad <<= 1;
			x = xdim-len-pad, y = pad; pad >>= 1;
			gfxSetColor(gStdColor[bcolor]);
			gfxFillBox(x-pad, y-pad, x+len+pad, y+pFont->height+pad);
			gfxDrawText(x, y, gStdColor[tcolor], compat, pFont);
		}
	}
}

void processDrawRooms()
{
	ExtPreCheckKeys();
	drawrooms(posx,posy,posz,ang,horiz,cursectnum);
	ExtAnalyzeSprites();
	drawmasks();
}


void process3DMode() {

	// quick hack for empty space position in 3d mode...
	if (cursectnum < 0 && !gNoclip)
	{
		scrSetPalette(kPal0);
		qsetmodeany(xdim2d, ydim2d);
		if (numsectors > 0 && cursectnum < 0)
		{
			scrSetMessage("Out of sector!");
			BeepFail();
		}
		
		overheadeditor();
		keyClear();
		return;
	}
	
	gHovStat = searchstat;
	gHovSpr = (gHovStat == OBJ_SPRITE) ? searchwall : -1;
	if (gHovStat == OBJ_MASKED)		gHovWall = searchwall;
	else if (gHovStat == OBJ_WALL)	gHovWall = searchwall2;
	else							gHovWall = -1;
}

/***********************************************************************
 * ExtCheckKeys()
 *
 * Called just before nextpage in both 2D and 3D modes.
 *
 **********************************************************************/
 

void ExtCheckKeys( void )
{
	int i, x, y, len, lay = 1;
	static int oldnumwalls = 0, oldnumsectors = 0, oldnumsprites = 0;
	static int objType = -1, objIdx = -1;
	static int omode, oxdim, oydim;
	BOOL dialog = FALSE, objUpd = FALSE;
	
	if (qsetmode == 200)
	{
		UndoSectorLighting();
		lay = gHudPrefs.layout3D;
	}
	else
	{
		lay = gHudPrefs.layout2D;
	}
	
	if (omode != qsetmode || oxdim != xdim || oydim != ydim)
	{
		if (omode != qsetmode)
		{
			searchx = xdim >> 1;
			searchy = ydim >> 1;
			horiz   = 100;
		}
		
		hudSetLayout(&gMapedHud, lay, &gMouse);
		omode = qsetmode; oxdim = xdim; oydim = ydim;
	}
		
	h = (gFrameClock & kHClock) ? 8 : 0;
	updateClocks();
	
	if (gMapedHud.draw)
	{
		if (totalclock >= gTimers.hudObjectInfo + 16)
		{
			gTimers.hudObjectInfo = totalclock;
			if ((objUpd = (searchstat != objType)) == FALSE)
			{
				switch (searchstat) {
					case OBJ_FLOOR:
					case OBJ_CEILING:
						objUpd = (objIdx != searchsector);
						break;
					default:
						objUpd = (objIdx != searchwall);
						break;
				}
			}
			
			if (objUpd)
			{
				objType = searchstat;
				objIdx  = (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING) ? searchsector : searchwall;
			}
		}
		
		dialog = (lay == kHudLayoutFull);
		gMapedHud.DrawIt();
		
		switch (objType) {
			case OBJ_FLOOR:
			case OBJ_CEILING:
				ShowSectorData(objIdx, !ctrl, dialog);
				break;
			case OBJ_MASKED:
			case OBJ_WALL:
				ShowWallData(objIdx,   !ctrl, dialog);
				break;
			case OBJ_SPRITE:
				ShowSpriteData(objIdx, !ctrl, dialog);
				break;
			default:
				gMapedHud.SetMsg(gMapInfoStr);
				gMapedHud.SetComment();
				gMapedHud.SetTile();
				break;
		}
	}

	if (!gPreviewMode)
	{
		if (numwalls != oldnumwalls || numsectors != oldnumsectors || numsprites != oldnumsprites)
		{
			oldnumwalls = numwalls, oldnumsectors = numsectors, oldnumsprites = numsprites;
			CleanUp();
		}

		int dy = (totalclock < gScreen.msg[0].time) ? 14 : 4;
		if (hgltShowStatus(4, dy))
			dy+=14;

		objectLockShowStatus(4, dy);
		if (gFrameClock > gTimers.autosave + gAutosave.interval)
		{
			gTimers.autosave = gFrameClock;
			if (asksave)
			{
				asksave = FALSE; gTimers.diskPic = totalclock + 128;
				
				gAutosave.current++;
				if (gAutosave.current > gAutosave.max)
					gAutosave.current = 1;

				getPath(gPaths.maps, buffer, FALSE);
				sprintf(buffer2, "%s%s%d", buffer, gAutosave.basename, gAutosave.current);
				ChangeExtension(buffer2, gExtNames[kMap]);
				boardSave(buffer2, TRUE);
			}
		}
	}
	else
	{
		previewProcess();
	}

	keyGetHelper(&key, &ctrl, &shift, &alt);
	if ((key) && (gPreviewMode && (previewProcessKeys() || !previewCheckKey(key))) || (processKeysShared()))
	{
		keyClear();
		keystatus[key] = 0;
		key = 0;
	}
	
	CalcFrameRate();
	scrDisplayMessage();

	if (qsetmode == 200)
	{
		process3DMode();
		ProcessKeys3D();
	}
	else
	{
		process2DMode();
		ProcessKeys2D();
	}

	if (totalclock < gTimers.diskPic)
	{
		gfxDrawBitmap(kBitmapDisk1, xdim - 33, 1);

		static char saves[3];
		sprintf(saves, "%03i", gAutosave.current);
		gfxSetColor(gStdColor[kColorLightGray]);
		gfxFillBox(xdim - 29, 21, xdim - 5, 31);
		gfxDrawText(xdim - 25, 22, gStdColor[kColorBlack], saves, qFonts[1]);

	}
	else
	{
		QFONT* pFont = qFonts[1];
		sprintf(buffer, "%4d FPS", gFrameRate);
		len = gfxGetTextLen(buffer, pFont);

		y = 2;
		if (!gPreviewMode && qsetmode != 200) y+=22;
		else if (gPreviewMode)
			y+=22;

		gfxPrinTextShadow(xdim - len - 4, y, gStdColor[(gFrameRate<30) ? kColorLightRed : kColorWhite], buffer, pFont);
	}

	if (key)
	{
		keyClear();
		asksave = TRUE;
	}	
}



void qprintmessage16(char name[82]) {
	
	gMapedHud.SetMsgImp(128, name);
}

/* void textareatest() {
	
	int dw, dx1, dy1, dx2, dy2, dwh, dhg, i;
	
	Window dialog(0, 0, 320, 200, "test");
	dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
	dialog.getSize(&dwh, &dhg);	
	
	char text[] = "Test text\nLine222\nLine33\nLine4";
	
	TextArea* pArea = new TextArea(dx1, dy1, dwh, 100, text);
	TextButton* pOk = new TextButton(dx1, dy2-30, 60, 30, "&Confirm", mrOk);
	
	
	dialog.Insert(pArea);
	dialog.Insert(pOk);
	
	ShowModal(&dialog);
} */

//extern "C" char* defsfilename = "blood.def";
//extern "C" int nextvoxid = 0;
int ExtInit(int argc, char const * const argv[])
{
	int i, retn = 0;
	BOOL iniGenerated = FALSE;
	
	HookReplaceFunctions();
	setbrightness_replace 		= qsetbrightness;
	draw2dscreen_replace 		= qdraw2dscreen;
	setvgapalette_replace 		= qsetvgapalette;
	printmessage16_replace 		= qprintmessage16;
	loadtile_replace 			= qloadtile;

    char *appdir = Bgetappdir();

	// the OSX app bundle, or on Windows the directory where the EXE was launched
	if (appdir)
	{
		addsearchpath(appdir);
		if ((i = strlen(appdir) - 1) >= 0)
		{
			if (slash(appdir[i])) appdir[i] = 0;
			_chdir(appdir); // we need this so opening files through "Open with..." dialog works in Windows...
			sprintf(buffer, "Using \"%s\" as working dirctory.\n", appdir);
		}

		free(appdir);
	}
	else
	{
		ThrowError("Failed to get app directory.");
	}

	wm_setapptitle("XMAPEDIT");
	buildsetlogfile(kLogFile);
	buildprintf("XMAPEDIT BUILD %s\n", __DATE__);
	buildprintf(buffer);

	// ---------------------------------------------------------------------------
	// setup timers
	gTimers.Init(0);
	// initialize basic xmapedit paths
	// ---------------------------------------------------------------------------
	gPaths.InitBase();
	// ---------------------------------------------------------------------------
	#ifdef USE_QHEAP
		Resource::heap = new QHeap(256*1024*1024);
	#endif
	// ---------------------------------------------------------------------------
	// load core file
	buildprintf("Loading file: \"%s\".\n", kCoreRffName);
	if (fileExists(kCoreRffName)) gGuiRes.Init(kCoreRffName);
	else ThrowError("%s is not found.", kCoreRffName);
	// ---------------------------------------------------------------------------
	// load config files
	buildprintf("Loading file: \"%s\".\n", kCoreIniName);
	while( 1 )
	{
		// create INI file with default settings
		if (!fileExists(kCoreIniName))
		{
			RESHANDLE hIni = gGuiRes.Lookup((unsigned int)0, "INI");
			if (hIni)
			{
				buildprintf("Creating \"%s\" with default settings.\n", kCoreIniName);
				MapEditINI = new IniFile((BYTE*)gGuiRes.Load(hIni), hIni->size, kCoreIniName);
				MapEditINI->PutKeyInt("General", "ConfigVersion", kConfigReqVersion);
				iniGenerated = TRUE;
			}
			else
			{
				buildprintf("--> Failed to load default settings!\n");
			}
			
			break;
		}
		
		MapEditINI = new IniFile(kCoreIniName);	// read xmapedit settings
		if ((i = MapEditINI->GetKeyInt("General", "ConfigVersion", -1)) == kConfigReqVersion) // check config version
			break;
			
		buildprintf("--> Current config has incorrect version: Given=%d, Required=%d!\n", i, kConfigReqVersion);
		unlink(kCoreIniName); delete(MapEditINI); // force creating new file
		continue;
	}
	
	
	
	// ---------------------------------------------------------------------------
	// initialize game resource archive
	gPaths.InitResourceRFF(MapEditINI,	"Resources.RFF"); // paths to game resource archive
	buffer[0] = 0;
	if (!fileExists(gPaths.bloodRFF)) sprintf(buffer, gPaths.bloodRFF);
	if (!fileExists(gPaths.soundRFF)) sprintf(buffer, gPaths.soundRFF);
	if (buffer[0])
		ThrowError("%s is not found.", buffer);

	buildprintf("Loading file: \"%s\".\n", gPaths.bloodRFF);
	gSysRes.Init(gPaths.bloodRFF); // initialize BLOOD.RFF	
	// ---------------------------------------------------------------------------
	// initialize recently used files / folders
	gPaths.InitMisc(MapEditINI,	"Visited");
	// ---------------------------------------------------------------------------
	// initialize engine
	buildprintf("Initializing engine...\n");
	initengine();
	// ---------------------------------------------------------------------------
	// initialize screen
	buildprintf("Initializing screen and GUI interface...\n");
	scrInit();
	gScreen.Init(MapEditINI, "Screen"); // get screen settings
	scrCreateStdColors();
	scrSetGameMode(fullscreen, xdimgame, ydimgame, bpp);
	searchx = xdim >> 1, searchy = ydim >> 1;
	GUIInit();
	// ---------------------------------------------------------------------------
	// show splash screen
	splashScreen("LOADING THE XMAPEDIT NOW");
	// ---------------------------------------------------------------------------
	// initialize keyboard & mouse
	initinput();
	initmouse();
	// ---------------------------------------------------------------------------
	// setup mouse
	gMousePrefs.Init(MapEditINI, 			"Mouse");
	gMouseLook.Init(MapEditINI, 			"MouseLook");
	gMouse.Init(&gMousePrefs);
	// ---------------------------------------------------------------------------
	// read the rest of options
	buildprintf("Loading preferences...\n");
	
	gMisc.Init(MapEditINI,					"General");
	
	if (gMisc.editMode == 0)
		qsetmodeany(xdimgame, ydimgame);
	
	if (gMisc.forceSetup) // should we show options?
		xmpOptions();
	
	gAutoAdjust.Init(MapEditINI,			"AutoAdjusting");
	gAutosave.Init(MapEditINI,				"Autosave");
	gAutoGrid.Init(MapEditINI,				"AutoGrid");
	gCompat.Init(MapEditINI,				"Compatibility");
	gImportPrefs.Init(MapEditINI,			"ImportWizard");
	gLightBomb.Init(MapEditINI,				"LightBomb");
	gRotateOpts.Init(MapEditINI,			"Rotation");
	gTileView.Init(MapEditINI,				"TileViewer");
	gPreview.Init(MapEditINI,				"PreviewMode");
	gHudPrefs.Init(MapEditINI,				"HUD");
	gCmtPrefs.Init(MapEditINI,				"Comments");
	gPluPrefs.Init(MapEditINI,				"PalookupViewer");
	// ---------------------------------------------------------------------------
	// add more external files in resource system
	nnExtResAddExternalFiles(&gSysRes, gPaths.modNblood, gExternFiles, LENGTH(gExternFiles));
	// ---------------------------------------------------------------------------
	// initialize ART related stuff
	buildprintf("Initialising tiles...\n");
	sprintf(buffer, "Resources.ART");
	sprintf(buffer2, "15");
	sprintf(buffer3, "CPART15.AR_");
	gPaths.InitResourceART(MapEditINI, buffer);
	if (iniGenerated && fileExists(buffer3))
	{
		// user can alter defaults.ini
		if (!MapEditINI->KeyExists(buffer, buffer2) && Confirm("Enable Cryptic Passage ART?"))
			MapEditINI->PutKeyString(buffer, buffer2, buffer3);
	}

	tileInitFromIni();
	tileInitSystemTiles();
	favoriteTileInit();
	extVoxInit();
	if (gMisc.zeroTile2pal && tileLoadTile(0))
		memset((void*)waloff[0], gMisc.zeroTile2pal, tilesizx[0]*tilesizy[0]);
	
	// ---------------------------------------------------------------------------
	// initialize hud
	gMapedHud.InitColors();
	gMapedHud.SetView(0, ydim - kHudHeighNormal, xdim, ydim);
	gMapedHud.SetLogo("XMAPEDIT", pFont->charSpace * 6, gSysTiles.xmpIco);
	hudSetLayout(&gMapedHud, (qsetmode == 200) ? gHudPrefs.layout3D : gHudPrefs.layout2D, &gMouse);
	// ---------------------------------------------------------------------------
	// initialize sound
	buildprintf("Initialising sound system...\n");
	gSound.Init(MapEditINI, "Sound");
	// ---------------------------------------------------------------------------
	// misc initialization
	buildprintf("Initialising misc stuff...\n");
	defaultspritecstat 	= kSprOrigin;
	showinvisibility 	= TRUE;
	gSkyCount			= 16;
	visibility 			= 0;
	mapversion			= 7;
	
	trigInit(gSysRes);		FireInit();
	hgltReset();			eraseExtra();
	InitializeNames();		dbInit();
	gObjectLock.Init();		gBeep.Init();

	memset(joinsector, -1, sizeof(joinsector));
	gSeqEd.filename 			= gPaths.seqs;
	gTool.objType 				= -1;
	gTool.objIndex 				= -1;
	// ---------------------------------------------------------------------------
	buildprintf("Init completed!\n\n\n");
	
	splashScreen();
	//textareatest();
	
	if (argc <= 1 || argv[1] == NULL)
	{
		sprintf(buffer, kDefaultMapName);
		
		if (!gMisc.autoLoadMap)
		{
			sprintf(gPaths.maps, buffer);
			char* tmp = dirBrowse("Load board", gPaths.maps, ".map");
			if (!tmp)
			{
				while ( 1 )
				{
					switch(xmpMenuProcess()) {
						default:
							continue;
						case mrLoad:
						case mrReload:
						case mrLoadAsave:
						case mrNew:
						case mrToolImportWizard:
							return 0;
						case mrCancel:
						case mrToolArtEdit:
						case mrToolQavEdit:
						case mrToolSeqEdit:
							xmpQuit();
							break;
					}
					break;
				}
			}
			else
			{
				sprintf(buffer, tmp);
			}
		}
		else if (strlen(gPaths.maps))
		{
			sprintf(buffer, gPaths.maps);
		}
	}
	else
	{
		sprintf(buffer, "%s", argv[1]);
	}

	// create default map path
	if (sprintf(gPaths.maps, buffer) > 0)
		ChangeExtension(gPaths.maps, getExt(kMap));
	
	boardReset();
	switch (i = toolOpenWith(buffer)) {
		case kToolMapEdit:
			gMapLoaded = TRUE;
			boardLoad(buffer);
			break;
		case -1:
			Alert("Could not open file \"%s\".", buffer);
			xmpQuit(1);
			break;
		case kToolArtEdit:
		case kToolSeqEdit:
		case kToolQavEdit:
			gTool.cantest = FALSE;
			switch (i) {
				case kToolSeqEdit:
					sprintf(gSeqEd.filename, buffer);
					ChangeExtension(gPaths.images, getExt(kSeq));
					seqeditStart(gSeqEd.filename);
					break;
				case kToolArtEdit:
					sprintf(gPaths.images, buffer);
					ChangeExtension(gPaths.images, getExt(kArt));
					artedStart(gPaths.images, TRUE);
					break;
				case kToolQavEdit:
					sprintf(gPaths.qavs, buffer);
					ChangeExtension(gPaths.qavs, getExt(kQav));
					gQaved.Start(gPaths.qavs);
					break;
			}
			// no break
		default:
			xmpQuit();
			break;
	}

	return retn;
}

void ExtUnInit(void)
{
	sndTerm();
	scrUnInit(false);
	favoriteTileSave();
	if (MapEditINI)
	{	
		gMisc.Save(MapEditINI, 			"General");
		gTileView.Save(MapEditINI,		"TileViewer");
		gPluPrefs.Save(MapEditINI,		"PalookupViewer");
		gAutoGrid.Save(MapEditINI, 		"AutoGrid");
		gMouseLook.Save(MapEditINI, 	"MouseLook");
		gPreview.Save(MapEditINI, 		"PreviewMode");
		gPaths.Save(MapEditINI, 		"Visited");
		gScreen.Save(MapEditINI, 		"Screen");
		gHudPrefs.Save(MapEditINI,  	"HUD");
		gCmtPrefs.Save(MapEditINI,  	"Comments");
		gImportPrefs.Save(MapEditINI, 	"ImportWizard");
		MapEditINI->Save();
	}
}

void assignXObject(int nType, int nIdx, XSPRITE** pXSpr, XSECTOR** pXSect, XWALL** pXWall) {

	*pXSpr = NULL; *pXSect = NULL; *pXWall = NULL;

	switch (nType) {
		case OBJ_SPRITE:
			*pXSpr = &xsprite[GetXSprite(nIdx)];
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			*pXWall = &xwall[GetXWall(nIdx)];
			break;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			*pXSect = &xsector[GetXSector(nIdx)];
			break;
	}

}

int xsysConnect(int nTypeA, int nIdxA, int nTypeB, int nIdxB) {

	XWALL	*pXWallA, *pXWallB;
	XSPRITE *pXSprA, *pXSprB;
	XSECTOR *pXSectA, *pXSectB;

	assignXObject(nTypeA, nIdxA, &pXSprA, &pXSectA, &pXWallA);
	assignXObject(nTypeB, nIdxB, &pXSprB, &pXSectB, &pXWallB);

	int chlID = findUnusedChannel();
	if (pXSprA)
	{

		//if (isMultiTX(nIdxA))
		//{

		//}
		if (chlRangeIsFine(pXSprA->txID)) chlID = pXSprA->txID;
		else pXSprA->txID = chlID;

		gXTracker.TrackSprite(nIdxA, 1);
	}
	else if (pXSectA)
	{
		if (chlRangeIsFine(pXSectA->txID)) chlID = pXSectA->txID;
		else pXSectA->txID = chlID;

		gXTracker.TrackSector(nIdxA, 1);
	}
	else if (pXWallA)
	{
		if (chlRangeIsFine(pXWallA->txID)) chlID = pXWallA->txID;
		else pXWallA->txID = chlID;

		gXTracker.TrackWall(nIdxA, 1);
	}
	else
	{
		return -1;
	}

	// disconnect all objects that points on this?
	////

	if (pXSprB)  pXSprB->rxID  = chlID;
	if (pXSectB) pXSectB->rxID = chlID;
	if (pXWallB) pXWallB->rxID = chlID;




	return 0;
}

int xsysConnect2(int nTypeA, int nIdxA, int nTypeB, int nIdxB) {

	int chlID = -1;
	XWALL	*pXWallA, *pXWallB;
	XSPRITE *pXSprA, *pXSprB;
	XSECTOR *pXSectA, *pXSectB;

	assignXObject(nTypeA, nIdxA, &pXSprA, &pXSectA, &pXWallA);
	assignXObject(nTypeB, nIdxB, &pXSprB, &pXSectB, &pXWallB);

	// clipboard
	if (pXSprA) 	   chlID = pXSprA->rxID;
	else if (pXSectA)  chlID = pXSectA->rxID;
	else if (pXWallA)  chlID = pXWallA->rxID;
	if (chlID <= 0)
		return xsysConnect(nTypeB, nIdxB, nTypeA, nIdxA);

	// Q
	if (pXSprB)  		pXSprB->txID = chlID;
	else if (pXSectB) 	pXSectB->txID = chlID;
	else if (pXWallB)   pXWallB->txID = chlID;
	else return -2;

	return 0;
}


BOOL processKeysShared() {

	if (key == 0)
		return FALSE;

	int x, y, swal, ewal, sect2;
	int i = 0, j = 0, k = 0, f = 0, sect = -1, nwall = -1, type = getHighlightedObject();
	BOOL in2d = (qsetmode != 200);
	
	switch (key) {
		default: return FALSE;
/* 		case KEY_3:
		{
			if (!isSearchSector())
			{
				BeepFail();
				break;
			}
			
			sectortype* pNeigh;
			sectortype* pSect = &sector[searchindex];
			XSECTOR* pXSect = &xsector[GetXSector(searchindex)];
			pSect->type = kSectorZMotion;
			
			int nOffset = 0;
			int nDoorType = 0;
			int nVel = 10;
			int nWait = 0;
			
			switch(nDoorType) {
				case 0:
					pXSect->offFloorZ = pSect->floorz;
					pXSect->onFloorZ = pSect->floorz;
					pXSect->offCeilZ = pSect->floorz;
					pXSect->onCeilZ = pSect->ceilingz;
					pXSect->busyTimeA = nVel;
					pXSect->state = 0;
					break;
				
			}
			
			BeepOk();
			
		}
		break; */
		case KEY_F11:
			if (!alt) return FALSE;
			while( 1 )
			{
				if (in2d)
				{
					if (gHudPrefs.dynamicLayout2D) gHudPrefs.layout2D = kHudLayoutNone;
					else gHudPrefs.layout2D = IncRotate(gHudPrefs.layout2D, kHudLayoutMax);
					gHudPrefs.dynamicLayout2D = (gHudPrefs.layout2D == kHudLayoutDynamic);
					
					i = gHudPrefs.layout2D;
					if (gHudPrefs.dynamicLayout2D)
						gHudPrefs.layout2D = kHudLayoutFull;

					hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
				}
				else
				{
					gHudPrefs.layout3D = IncRotate(gHudPrefs.layout3D, kHudLayoutMax);
					if (gHudPrefs.layout3D == kHudLayoutDynamic) continue;
					else hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);
					i = gHudPrefs.layout3D;
				}
				
				gMapedHud.SetMsgImp(128, "HUD layout (%dD Mode): %s", (in2d) ? 2 : 3, gHudLayoutNames[i]);
				Beep(i > 0);
				break;
			}
			break;
		case KEY_W:
		{
			int x1, y1, x2, y2;
			if (!gCmtPrefs.enabled)
			{
				Alert("Comment system disabled!");
				return FALSE;
			}
			else if (!in2d && !alt) return FALSE;
			else if (in2d) getpoint(searchx, searchy, &x1, &y1), x2 = x1, y2 = y1;
			else x1 = posx, y1 = posy, hit2pos(&x2, &y2);
			
			if (in2d)
			{
				if (cmthglt >= 0)
				{
					// body (edit comment)
					if ((cmthglt & 0xc000) == 0) gCommentMgr.ShowDialog(x1, y1, x2, y2, cmthglt);
					else gCommentMgr.ShowBindMenu(cmthglt & 0x3FFF, x1, y1); // tail (bind, unbind, delete)
					break;
				}
			}
			else
			{
				i = searchstat; j = -1;
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						i = OBJ_WALL;
						j = wall[searchwall].extra;
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						i = OBJ_SECTOR;
						j = sector[searchsector].extra;
						break;
					case OBJ_SPRITE:
						j = searchwall;
						break;
				}
				
				if ((i = gCommentMgr.IsBind(i, j)) >= 0)
				{
					// tail (bind, unbind, delete)
					if (ctrl) gCommentMgr.ShowBindMenu(i, x2, y2);
					else gCommentMgr.ShowDialog(x1, y1, x2, y2, i); // body (edit comment)
					break;
				}
			}
		
			if (gCommentMgr.ShowDialog(x1, y1, x2, y2) >= 0)
			{
				scrSetMessage("New comment created.");
				BeepOk();
			}
		}
		break;
		case KEY_F5:
		case KEY_F6:
			i = 1;
			switch (searchstat) {
				case OBJ_SPRITE:
					hudSetLayout(&gMapedHud, kHudLayoutFull);
					if (key == KEY_F6) EditSpriteData(searchwall, !ctrl);
					else
					{
						sect = (in2d) ? sectorhighlight : sprite[searchwall].sectnum;
						if (sect >= 0)
						{
							if (shift) EditSectorLighting(sect);
							else EditSectorData(sect, !ctrl);
						}
					}
					break;
				case OBJ_WALL:
				case OBJ_MASKED:
					hudSetLayout(&gMapedHud, kHudLayoutFull);
					if (key == KEY_F6) EditWallData(searchwall, !ctrl);
					else
					{
						i = wall[searchwall].nextwall;
						if (in2d) sect = sectorhighlight;
						else sect = sectorofwall((alt || i < 0) ? searchwall : i);
						
						if (sect < 0) i = 0;
						else if (shift) EditSectorLighting(sect);
						else EditSectorData(sect, !ctrl);
					}
					break;
				case OBJ_CEILING:
				case OBJ_FLOOR:
					if (key == KEY_F5)
					{
						hudSetLayout(&gMapedHud, kHudLayoutFull);
						if (!shift) EditSectorData(searchsector, !ctrl);
						else EditSectorLighting(searchsector);
						break;
					}
					// no break
				default:
					BeepFail();
					i = 0;
					break;
			}
			if (i) hudSetLayout(&gMapedHud, (in2d) ? gHudPrefs.layout2D : gHudPrefs.layout3D);
			break;
		case KEY_Q: // connect one object with the one from clipboard via RX/TX
			if (somethingintab == 255)
			{
				scrSetMessage("Clipboard is empty!");
				BeepFail();
				break;
			}

			if (shift) i = xsysConnect2(somethingintab, tempidx, searchstat, searchindex);
			else i = xsysConnect(somethingintab, tempidx, searchstat, searchindex);
			if (!Beep(i == 0)) break;
			scrSetMessage("Objects connected.");
			CleanUpMisc();
			break;
		case KEY_J:
			i = j = -1;
			if (shift || joinsector[0] >= 0) i = getSector(); // old BUILD joining method
			else if (in2d) j = linehighlight; // one key press needs a line in 2d
			else if ((type == 100 || type == 300) && (j = searchwall) >= 0) // one key press works just like M
			{
				if (wall[j].nextwall >= 0 && (wall[wall[j].nextwall].cstat & kWallSwap))
					j = searchwall2;
			}
			
			if (j < 0) scrSetMessage("Must point near red wall.");
			else if (wall[j].nextwall < 0)
			{
				if (redSectorCanMake(j) > 0)
				{
					i = showButtons(gSectorMergeMenu, LENGTH(gSectorMergeMenu), "Select action...") - mrUser;
					if (i == 0)
					{
						scrSetMessage("New sector created.");
						redSectorMake(j);
						CleanUp();
						BeepOk();
						break;
					}
					else if (i == 1)
					{
						redSectorMake(j);
						joinsector[0] = sectorofwall(j);
						i = wall[j].nextsector;
					}
					else
					{
						i = -1;
					}
				}
			}
			else if (sectorofwall(wall[j].nextwall) != wall[j].nextsector)
			{
				scrSetMessage("Must select sibling sector.");
			}
			else
			{
				joinsector[0] = sectorofwall(j);
				i = wall[j].nextsector;
			}

			if (i < 0) BeepFail();
			else if (joinsector[0] >= 0)
			{
				if (joinsector[0] == i)
				{
					scrSetMessage("Sector merging aborted.");
					BeepFail();
				}
				else if (redSectorMerge(joinsector[0], i) == 0)
				{
					scrSetMessage("Sector #%d merged with sector #%d.", joinsector[0], i);
					BeepOk();
				}
				else
				{
					scrSetMessage("Error merging sector.");
					BeepFail();
				}

				memset(joinsector, -1, sizeof(joinsector));
				updatesector(posx, posy, &cursectnum);
				CleanUp();
			}
			else
			{
				scrSetMessage("Join sector - press J again on sector to join with.");
				joinsector[0] = i;
				BeepOk();
			}
			break;
		case KEY_ESC:
			if (keystatus[KEY_SPACE])
			{
				if (!in2d && somethingintab != 255)
				{
					scrSetMessage("Clipboard buffer cleared.");
					somethingintab = 255;
					BeepFail();
				}

				break;
			}
			if (highlightcnt > 0) hgltReset(kHgltPoint), i = 1;
			if (!in2d && highlightsectorcnt > 0) hgltReset(kHgltSector), i = 1;
			if (gHgltc > 0) grshUnhgltObjects(-1, TRUE), i = 1;
			if (i == 1)
			{
				scrSetMessage("Highlight reset.");
				BeepFail();
				break;
			}
			xmpMenuProcess();
			keyClear();
			break;
		case KEY_TAB:
			if (!alt)
			{
				if (!type)
				{
					scrSetMessage("Can't copy properties of an unknown object.");
					BeepFail();
					break;
				}

				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						i = tempidx 	= searchwall;
						temppicnum		= (searchstat == OBJ_MASKED) ? wall[i].overpicnum : wall[i].picnum;
						tempshade		= wall[i].shade;
						temppal			= wall[i].pal;
						tempxrepeat		= wall[i].xrepeat;
						tempyrepeat		= wall[i].yrepeat;
						tempxoffset		= wall[i].xpanning;
						tempyoffset		= wall[i].ypanning;
						tempcstat		= wall[i].cstat;
						tempextra		= wall[i].extra;
						temptype		= wall[i].type;
						tempslope		= 0;

						cpywall[kTabWall] = wall[i];
						if (wall[i].extra > 0)
							cpyxwall[kTabXWall] = xwall[wall[i].extra];

						break;
					case OBJ_SPRITE:
						i = tempidx 	= searchwall;
						temppicnum 		= sprite[i].picnum;
						tempshade 		= sprite[i].shade;
						temppal 		= sprite[i].pal;
						tempxrepeat 	= sprite[i].xrepeat;
						tempyrepeat 	= sprite[i].yrepeat;
						tempxoffset		= sprite[i].xoffset;
						tempyoffset		= sprite[i].yoffset;
						tempcstat 		= sprite[i].cstat;
						tempextra 		= sprite[i].extra;
						tempang 		= sprite[i].ang;
						temptype 		= sprite[i].type;
						tempslope		= spriteGetSlope(i);

						cpysprite[kTabSpr] = sprite[i];
						if (sprite[i].extra > 0)
							cpyxsprite[kTabXSpr] = xsprite[sprite[i].extra];

						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						i = tempidx 	= searchsector;
						temptype 		= sector[i].type;
						switch (searchstat) {
							case OBJ_FLOOR:
								temppicnum 		= sector[i].floorpicnum;
								tempshade 		= sector[i].floorshade;
								temppal 		= sector[i].floorpal;
								tempxrepeat 	= sector[i].floorxpanning;
								tempyrepeat 	= sector[i].floorypanning;
								tempcstat 		= sector[i].floorstat;
								tempextra 		= sector[i].extra;
								tempvisibility 	= sector[i].visibility;
								tempslope		= 0;
								break;
							default:
								temppicnum 		= sector[i].ceilingpicnum;
								tempshade 		= sector[i].ceilingshade;
								temppal 		= sector[i].ceilingpal;
								tempxrepeat 	= sector[i].ceilingxpanning;
								tempyrepeat 	= sector[i].ceilingypanning;
								tempcstat 		= sector[i].ceilingstat;
								tempextra 		= sector[i].extra;
								tempvisibility 	= sector[i].visibility;
								tempslope		= 0;
								break;
						}

						if (in2d)
							searchstat = OBJ_SECTOR; // for names

						cpysector[kTabSect] = sector[searchsector];
						if (sector[searchsector].extra > 0)
							cpyxsector[kTabXSect] = xsector[sector[searchsector].extra];

						break;
				}

				somethingintab = (char)searchstat;
				scrSetMessage("%s[%d] copied in clipboard.", gSearchStatNames[searchstat], i);
				BeepOk();
			}
			break;
		case KEY_F10:
			if (highlightsectorcnt > 0) i |= kHgltSector;
			if (highlightcnt > 0) 		i |= kHgltPoint;
			if (i > 0)
			{
				hgltIsolateRorMarkers(i);
				hgltIsolatePathMarkers(i);
				hgltIsolateChannels(i, kChlCheckOutsideR | kChlCheckInsideRS | kChlCheckOutsideS);

				scrSetMessage("Objects isolated.");
				BeepOk();
				break;
			}
			return FALSE;
		case KEY_APOSTROPHE:
		case KEY_SEMICOLON:
			i = 512;
			if (ctrl)  i >>= 1;
			if (shift) i >>= 1;
			switch (searchstat) {
				case OBJ_SPRITE:
					if (!sprInHglt(searchwall)) break;
					i = (key == KEY_APOSTROPHE) ? i : -i;
					hgltSprRotate(i);
					scrSetMessage("Rotate %d sprite(s) by %d", hgltSprCount(), i);
					BeepOk();
					break;
			}
			break;
		case KEY_DELETE:
			if (ctrl)
			{
				j = 1, i = -1;
				if (!in2d && (searchstat == OBJ_WALL || searchstat == OBJ_MASKED))
				{
					TranslateWallToSector();
					i = searchsector;
				}
				if ((i < 0) && (i = getSector()) < 0) break;
				else if (hgltCheck(OBJ_SECTOR, i) < 0) sectDelete(i);
				else j = hgltSectCallFunc(sectDelete);

				scrSetMessage("%d sector(s) deleted.", j);
				updatesector(posx, posy, &cursectnum);
				updatenumsprites();
				CleanUp();
				BeepOk();
				break;
			}
			else if (type == 200)
			{
				if (shift) sprDelete(&sprite[searchwall]);
				else i = hgltSprCallFunc(sprDelete);
				scrSetMessage("%d sprite(s) deleted.", ClipLow(i, 1));
				updatenumsprites();
				BeepOk();
				break;
			}
/* 			// point deletion
			else if (type == 100)
			{
				if (in2d)
				{
					i = (pointhighlight >= 0) ? (pointhighlight & 0x3FFF) : -1;
					if (i >= 0)
					{
						k = 0;
						for (j = 0; j < numwalls; j++)
						{
							if (wall[j].x == wall[i].x && wall[j].y == wall[i].y) k++;
						}
						
						if (linehighlight >= 0)
						{
							j = linehighlight;
							
							if (i != j && wall[j].x == wall[i].x && wall[j].y == wall[i].y)
							{
									Alert("!! %d / %d", j, i);
									if (wall[j].nextwall >= 0)
									
									dragpoint(i, wall[wall[j].nextwall].x, wall[wall[j].nextwall].y);
									else
										dragpoint(i, wall[j].x, wall[j].y);
									
									CleanUp();
									//deletepoint(i);
							}
							else 
								if (wall[wall[j].point2].x == wall[i].x && wall[wall[j].point2].y == wall[i].y)
								{
									//Alert(" ?? %d / %d", j, i);
									dragpoint(i, wall[j].x, wall[j].y);
									CleanUp();
									//deletepoint(j);
								}
							
							}
							//if (linehighlight == i) deletepoint(i);
							//else if (linehighlight == getPrevWall(i))
								//deletepoint(linehighlight);
							
							//if (linehighlight == i || linehighlight)
							//{
								
							//}
							
							//Alert("%d", k);
					}
					
				}
			} */
			BeepFail();
			break;
		case KEY_SCROLLLOCK:
			updatesector(posx, posy, &cursectnum);
			if (cursectnum >= 0)
			{
				setStartPos(posx, posy, posz, ang);
				scrSetMessage("Set start position at x=%d, y=%d, z=%d, in sector #%d", startposx, startposy, startposz, startsectnum);
				BeepOk();
			}
			else
			{
				scrSetMessage("Cannot set start position outside the sector!");
				BeepFail();
			}
			break;
		case KEY_B:
			if (ctrl)
			{
				clipmovemask2d = clipmovemask3d = (clipmovemask3d == 0) ? BLOCK_MOVE : 0;
				scrSetMessage("Block move is %s", onOff(Beep(clipmovemask3d)));
				break;
			}
			switch (searchstat) {
				case OBJ_SPRITE:
					sprite[searchwall].cstat ^= kSprBlock;
					scrSetMessage("sprite[%i] blocking flag is %s", searchwall, onOff(sprite[searchwall].cstat & kSprBlock));
					BeepOk();
					break;
				case OBJ_FLOOR:
					if (in2d) break;
					// no break
				case OBJ_WALL:
				case OBJ_MASKED:
					if (hgltCheck(OBJ_WALL, searchwall) >= 0)
					{
						k = hgltWallsCheckStat(kWallBlock, 0x02); // red walls has cstat
						f = hgltWallsCheckStat(kWallBlock, 0x06); // red walls has NO cstat
						j = k + j;
						
						for (i = 0; i < highlightcnt; i++)
						{
							if ((highlight[i] & 0xC000) != 0)
								continue;

							nwall = highlight[i];
							if (wall[nwall].nextwall < 0) continue;
							else if (f < j && f > k)
							{
								if (k < j) wallCstatRem(nwall, kWallBlock, !shift);
								else wallCstatAdd(nwall, kWallBlock, !shift);
							}
							else if (k < j) wallCstatAdd(nwall, kWallBlock, !shift);
							else wallCstatRem(nwall, kWallBlock, !shift);
						}
					}
					else if (wall[searchwall].nextwall >= 0)
					{
						wallCstatToggle(searchwall, kWallBlock, !shift);
					}
					else
					{
						BeepFail();
						break;
					}
					scrSetMessage("wall[%i] blocking flag is %s", searchwall, onOff(wall[searchwall].cstat & kWallBlock));
					BeepOk();
					break;
			}
			break;
		case KEY_E:
			if (type != 200) return FALSE;
			while ( 1 )
			{
				i = GetNumberBox("Enter statnum", sprite[searchwall].statnum, sprite[searchwall].statnum);
				if (i == sprite[searchwall].statnum)
					break;

				//if (sysStatReserved(i))
				//{
					//Alert("Statnum #%d is system reserved.", i);
					//continue;
				//}
				//else
				//{
					ChangeSpriteStat(searchwall, (short)ClipRange(i, 0, kMaxStatus - 1));
					scrSetMessage("sprite[%d].statnum = %d", searchwall, sprite[searchwall].statnum);
					BeepOk();
				//}

				break;
			}
			break;
		case KEY_HOME:
			if (ctrl)
			{
				static NAMED_TYPE searchFor[] = {
					OBJ_SPRITE,		gSearchStatNames[OBJ_SPRITE],
					OBJ_WALL,		gSearchStatNames[OBJ_WALL],
					OBJ_SECTOR,		gSearchStatNames[OBJ_SECTOR],
				};

				if ((type = showButtons(searchFor, LENGTH(searchFor), "Search for...")) != mrCancel)
				{
					j = 0;
					type -= mrUser;
					sprintf(buffer, "Locate %s #", gSearchStatNames[type]);
					if ((i = GetNumberBox(buffer, 0, -1)) >= 0)
					{
						switch (type) {
							case OBJ_SPRITE:
								for (j = 0; j < kMaxSprites; j++)
								{
									if (sprite[j].index != i || sprite[j].statnum >= kMaxStatus) continue;
									posx = sprite[i].x; posy = sprite[i].y;
									posz = sprite[i].z; ang	= sprite[i].ang;
									cursectnum = sprite[i].sectnum;
									break;
								}
								if (j >= kMaxSprites) i = -1;
								break;
							case OBJ_SECTOR:
								for (j = 0; j < numwalls; j++)
								{
									if (sectorofwall(j) != i) continue;
									avePointSector(i, &posx, &posy);
									posz = getflorzofslope(i, posx, posy);
									cursectnum = (short)i;
									break;
								}
								if (j == numwalls) i = -1;
								break;
							case OBJ_WALL:
								if (i >= 0 && i < numwalls)
								{
									j = sectorofwall(i);
									posx = wall[i].x;
									posy = wall[i].y;
									posz = getflorzofslope(j, posx, posy);
									cursectnum = (short)j;
									break;
								}
								i = -1;
								break;

						}
						
						Beep(i >= 0);
						scrSetMessage("%s %s found", gSearchStatNames[type], isNot(i >= 0));
						clampCamera();
					}
				}
			}
			else if (shift)
			{
				if (!previewMenuProcess()) break;
				else if (gPreviewMode) previewStop();
				previewStart();
			}
			else if (!gPreviewMode) previewStart();
			else previewStop();
			break;
		case KEY_F12:
			scrSetMessage("Beeps are %s", onOff(gMisc.beep^=1));
			BeepOk();
			break;
		case KEY_1:
			switch (searchstat) {
				case OBJ_SPRITE:
					sprite[searchwall].cstat ^= kSprOneSided;
					i = sprite[searchwall].cstat;
					if ((i & kSprRelMask) >= kSprFloor)
					{
						sprite[searchwall].cstat &= ~kSprFlipY;
						if (i & kSprOneSided)
						{
							if (posz > sprite[searchwall].z)
								sprite[searchwall].cstat |= kSprFlipY;
						}
					}
					scrSetMessage("Sprite #%i one-sided flag is %s", searchwall, onOff(sprite[searchwall].cstat & kSprOneSided));
					BeepOk();
					break;
				case OBJ_FLOOR:
				case OBJ_CEILING:
					// no break
				case OBJ_WALL:
				case OBJ_MASKED:
					if (Beep(searchwall > 0 && wall[searchwall].nextwall >= 0))
					{
						i = wallCstatToggle(searchwall, kWallOneWay, FALSE);
						if ((i & kWallOneWay) && !(i & kWallMasked))
								wall[searchwall].overpicnum = wall[searchwall].picnum;

						scrSetMessage("Wall #%i one-sided flag is %s", searchwall, onOff(i & kWallOneWay));
					}
					break;
			}
			break;
		case KEY_D:
			if (type == 200)
			{
				if (alt)
				{
					sprintf(buffer, "Sprite #%d clipdist", searchwall);
					i = GetNumberBox(buffer, sprite[searchwall].clipdist, sprite[searchwall].clipdist);
					sprite[searchwall].clipdist = (BYTE)ClipRange(i, 0, 255);
					scrSetMessage("sprite[%d].clipdist = %d", searchwall, sprite[searchwall].clipdist);
					BeepOk();
					return TRUE;
				}
			}
			return FALSE;
		case KEY_K:
			if (type != 100 && type != 200) BeepFail();
			
			switch (searchstat) {
				case OBJ_SPRITE:
					i = (short)(sprite[searchwall].cstat & kSprMoveMask);
					switch (i) {
						case kSprMoveNone:
							i = kSprMoveForward;
							scrSetMessage("sprite[%d] moves forward (blue).", searchwall);
							break;
						case kSprMoveForward:
							i = kSprMoveReverse;
							scrSetMessage("sprite[%d] moves reverse (green).", searchwall);
							break;
						case kSprMoveReverse:
							i = kSprMoveNone;
							scrSetMessage("sprite[%d] kinetic move disabled.", searchwall);
							break;
					}
					sprite[searchwall].cstat &= ~kSprMoveMask;
					sprite[searchwall].cstat |= (short)i;
					BeepOk();
					break;
				case OBJ_WALL:
				case OBJ_MASKED:
					i = (short)(wall[searchwall].cstat & kWallMoveMask);
					switch (i) {
						case kWallMoveNone:
							i = kWallMoveForward;
							scrSetMessage("wall[%d] moves forward (blue).", searchwall);
							break;
						case kWallMoveForward:
							i = kWallMoveReverse;
							scrSetMessage("wall[%d] moves reverse (green).", searchwall);
							break;
						case kWallMoveReverse:
						default:
							i = kWallMoveNone;
							scrSetMessage("wall[%d] kinetic move disabled.", searchwall);
							break;
					}
					wallCstatRem(searchwall, kWallMoveMask, !shift);
					wallCstatAdd(searchwall, i, !shift);
					BeepOk();
					break;
			}
			break;
		case KEY_M:
			//if (type != 100) break;
			if (searchwall < 0 || (i = wall[searchwall].nextwall) < 0)
			{
				BeepFail();
				break;
			}

			if (wallCstatToggle(searchwall, kWallMasked, !shift) & kWallMasked)
			{
				if (!shift)
				{
					// other side flip-x
					if (!(wall[searchwall].cstat & kWallFlipX)) wallCstatAdd(i, kWallFlipX, FALSE);
					else if (wall[i].cstat & kWallFlipX) // other side unflip-x
						wallCstatRem(i, kWallFlipX, FALSE);
					
					wall[searchwall].overpicnum = ClipLow(wall[searchwall].overpicnum, 0);
					wall[i].overpicnum = wall[searchwall].overpicnum;
				}
				
				// useless to have this together
				wallCstatRem(searchwall, kWallOneWay, !shift);
			}
			scrSetMessage("wall[%i] %s masked", searchwall, isNot(wall[searchwall].cstat & kWallMasked));
			BeepOk();
			break;
		case KEY_I:
			if (ctrl)
			{
				scrSetMessage("Show invisible sprites is %s", onOff(showinvisibility^=1));
				BeepOk();
			}
			else if (type == 200)
			{
				sprite[searchwall].cstat ^= kSprInvisible;
				scrSetMessage("sprite[%i] %s invisible", searchwall, isNot(sprite[searchwall].cstat & kSprInvisible));
				BeepOk();
			}
			else
			{
				BeepFail();
			}
			break;
		case KEY_R:
			if (type != 200) return FALSE;
			switch ((i = sprite[searchwall].cstat & kSprRelMask)) {
				case 0x00:
					i = 0x10;
					scrSetMessage("sprite[%i] is wall sprite", searchwall);
					break;
				case 0x10:
					i = 0x20;
					scrSetMessage("sprite[%i] is floor sprite", searchwall);
					break;
				case 0x20:
					i = 0x00;
					scrSetMessage("sprite[%i] is face sprite", searchwall);
					break;
				default:
					i = 0x00;
					scrSetMessage("sprite[%i] is face sprite", searchwall);
					break;
			}
			sprite[searchwall].cstat &= ~kSprRelMask;
			sprite[searchwall].cstat |= (ushort)i;
			BeepOk();
			break;
		case KEY_U:
			if (ctrl) return FALSE;
			else if (type == 300)
			{
				GetXSector(searchsector);
				scrSetMessage("sector[%d] is %s underwater", searchsector, isNot(xsector[sector[searchsector].extra].underwater ^= 1));
				Beep(xsector[sector[searchsector].extra].underwater);
				break;
			}
			BeepFail();
			break;
		case KEY_X:
			if (alt & 0x01)
			{
				// disable auto align for walls in a highlight
				for (i = 0; i < highlightsectorcnt; i++) sector[highlightsector[i]].alignto = 0;
				for (i = 0; i < highlightcnt; i++)
				{
					if ((highlight[i] & 0xC000) == 0x4000) continue;
					sector[sectorofwall(highlight[i])].alignto = 0;
				}
				scrSetMessage("Auto slope disabled for all highlighted walls.");
				BeepOk();
			}
			else if (type != 100 && type != 300) BeepFail();
			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED:
					if (wall[searchwall].nextwall >= 0)
					{
						sect = sectorofwall(searchwall);
						nwall = searchwall - sector[sect].wallptr;
						sector[sect].alignto = (BYTE)nwall;
						if (sector[sect].alignto)
						{
							scrSetMessage("Sector %d will align to wall %d (%d)", sect, searchwall, sector[sect].alignto);
							if (gMisc.pan) AlignSlopes();
							BeepOk();
							break;
						}
					}
					else
					{
						scrSetMessage("Must select the red wall!", sect);
						BeepFail();
						break;
					}
					// no break
				case OBJ_FLOOR:
				case OBJ_CEILING:
					if (sect < 0) sect = searchsector;
					if (Beep(sector[sect].alignto))
					{
						scrSetMessage("Sector %d auto-alignment disabled!", sect);
						sector[sect].alignto = 0;
					}
					break;
			}
			break;
	}

	return TRUE;
}

void xmpOptions(void) {
	
	while( 1 )
	{
		splashScreen();
		
		QFONT* pFont = qFonts[0];
		char text[64][32]; NAMED_TYPE modes[64];
		int i, j, k, len, x, y, w, h, half, fh = pFont->height;
		int dw = 280, dh = 130, btw = 80, bth = 20;
		int pad = 8, bw, bh;
		BOOL reallyLow = FALSE;
		half = (bth>>1)+(fh>>1);

		Window dialog(0, 0, dw, dh, "Options");
		FieldSet* pField1 = new FieldSet(pad, pad, dw-(pad*3), (bth<<1)-(pad>>1), "RESOLUTION");
		TextButton* pVideoChange = new TextButton(pad, pad, btw, bth, "Chan&ge...", 100);
		
		Checkbox* pFullscreen = new Checkbox(btw+(pad<<1), half, fullscreen, "&Fullscreen", 101);
		sprintf(buffer, "%dx%d", xdimgame, ydimgame); len = gfxGetTextLen(buffer, pFont);
		Label* pLabel = new Label(dw-(pad<<2)-len, half+2, buffer, kColorBlue);
		
		
		sprintf(buffer, "Show this &window at startup"); len = gfxGetLabelLen(buffer, pFont) + 14;
		Checkbox* pForceSetup = new Checkbox((dw>>1)-(len>>1), dh-bth-(pad*5), gMisc.forceSetup, buffer);
		TextButton* pClose = new TextButton((dw>>1)-(btw>>1), dh-bth-(pad*3), btw, bth, "&Close", mrOk);
		pClose->fontColor = kColorRed;
		
		pField1->Insert(pVideoChange);
		pField1->Insert(pFullscreen);
		pField1->Insert(pLabel);
		dialog.Insert(pClose);
		dialog.Insert(pField1);
		dialog.Insert(pForceSetup);

		ShowModal(&dialog);

		gMisc.forceSetup = pForceSetup->checked;
		
		switch (dialog.endState) {
			case 100:
				while( 1 )
				{
					memset(text, 0, sizeof(text));
					for (i = k = 0; i < validmodecnt; i++)
					{
						if ((w = validmode[i].xdim) <= 0 || (h = validmode[i].ydim) <= 0) continue;
						else if ((w < 640 || h < 480) && !reallyLow)
							continue;
						
						sprintf(buffer, "%dx%d", w, h);
						for (j = 0; j < i && stricmp(buffer, text[j]); j++); // we don't need duplicates
						if (j >= i)
						{
							sprintf(text[k], buffer);
							modes[k].name = text[k];
							modes[k].id = i;
							k++;
						}
					}
					
					if ((i = showButtons(modes, k, "Select resolution") - mrUser) < 0)
						break;
										
					w = validmode[i].xdim, h = validmode[i].ydim;
					if (w == xdim && h == ydim)
						break;
					
					bw = xdim, bh = ydim;
					toggleResolution(pFullscreen->checked, w, h); splashScreen();
					gMouseLook.Init(MapEditINI, "MouseLook");
					gMouse.Init(&gMousePrefs);
					
					xdimgame = xdim, ydimgame = ydim;
					gTileView.InitWindow();
					
					if (w != xdim || h != ydim)
						Alert("Failed to set resolution: %dx%d!", w, h);
					
					if (!Confirm("Keep %dx%d resolution?", xdim, ydim))
					{
						toggleResolution(pFullscreen->checked, bw, bh); splashScreen();
						gMouseLook.Init(MapEditINI, "MouseLook");
						gMouse.Init(&gMousePrefs);
						
						xdimgame = bw, ydimgame = bh;
						gTileView.InitWindow();
						continue;
					}
					else
					{
						xdim2d = xdim, ydim2d = ydim; // set same resolution for 2d mode?
					}
					break;
				}
				continue;
			case 101:
				if (pFullscreen->checked == fullscreen) break;
				toggleResolution(pFullscreen->checked, xdimgame, ydimgame);
				splashScreen();
				continue;
			case mrOk:
			case mrCancel:
				break;
			default:
				continue;
		}
		break;
	}
	
}



void xmpAbout(void) {

	struct GUI_TEXT {

		char* str;
		unsigned int type 	: 3;
		unsigned int attr 	: 8;
		unsigned int color	: 8;
		unsigned int font	: 8;

	};

	GUI_TEXT text[] = {

		{"The XMAPEDIT", 1, 0, 4, 11},
		{"{date}", 2, 0, 4, 11},
		{"www.cruo.bloodgame.ru/xmapedit", 1, 0, 4, 11},
		{"\0", 0, 0, 0},

		{"Created by", 1, 0, 0, 0},
		{"nuke.YKT", 1, 0, 1, 5},
		{"NoOne", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"Tested by", 1, 0, 0, 0},
		{"Spill,  Seizhak,  daMann", 1, 0, 1, 1},
		{"Tekedon,  Nyyss0nen", 1, 0, 1, 1},
		{"and others", 1, 0, 1, 1},
		{"\0", 0, 0, 0},

		{"Original MAPEDIT version by", 1, 0, 0, 0},
		{"Peter Freese", 1, 0, 4, 5},
		{"Nick Newhard", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"Original BUILD Engine by", 1, 0, 0, 0},
		{"Ken Silverman", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"jF BUILD by", 1, 0, 0, 0},
		{"Jonathon Fowler", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"Engine updates by", 1, 0, 0, 0},
		{"nuke.YKT", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"Always keep backup of your files.", 1, 0, 0, 1},
		{"You are using this software at", 1, 0, 0, 1},
		{"your own risk.", 1, 0, 0, 1},

	};
	
	int dw, i, x, y, len;
	Window dialog(0, 0, 240, ydim, "About");
	dw = dialog.width - 12;
	QFONT* font;

	x = 4, y = 14;
	for (i = 0; i < LENGTH(text); i++)
	{
		switch (text[i].type) {
			case 2:
				sprintf(buffer, "Build [%s]", __DATE__);
				break;
			default:
				sprintf(buffer, text[i].str);
				break;
		}

		if (buffer[0] != '\0')
		{
			if (text[i].attr & 16)
				strupr(buffer);

			font = qFonts[text[i].font];
			len = gfxGetTextLen(buffer, font) >> 1;
			Label* pLabel = new Label(4 + (dw >> 1) - len, y, buffer);
			pLabel->fontColor = text[i].color;
			pLabel->font = font;
			dialog.Insert(pLabel);

		}


		y+=font->height+1;
		dialog.height = y + 60;
	}
	
	len = dw>>1;
	dialog.Insert(new TextButton(4+((dw>>1)-(len>>1)),  dialog.height-48, len,  20, "&Ok", mrOk));
	ShowModal(&dialog);
	return;
}

void xmpQuit(int code)
{
	uninittimer();
	ExtUnInit();
	uninitengine();
	exit(code);
}

int xmpMenuCreate(char* name) {

	RESHANDLE hFile;
	int x = 4, y = 4, bh = 20, pd = 4, i, j;
	static char asavename[6][14], editors[3][14];
	
	sprintf(editors[0], getExt(kQav));
	sprintf(editors[1], getExt(kSeq));
	sprintf(editors[2], getExt(kArt));
	for (i = 0; i < LENGTH(editors); i++)
		strupr(editors[i]);

	Window dialog(0, 0, 138, ydim, name);
	TextButton* pSaveBoard = new TextButton(4, y, 84,  bh, "&Save board", mrSave);
	TextButton* pSaveBoardAs = new TextButton(88, y, 40,  bh, "&As...", mrSaveAs); y+=bh;
	TextButton* pReloadBoard = new TextButton(4, y, 40,  bh, "&Re", mrReload);
	if (!fileExists(gPaths.maps, &hFile))
	{
		pReloadBoard->fontColor = 8;
		pReloadBoard->disabled  = TRUE;
		pReloadBoard->canFocus  = FALSE;
	}

	if (!gMapLoaded)
	{
		pSaveBoard->fontColor = pSaveBoardAs->fontColor = pReloadBoard->fontColor = 8;
		pSaveBoard->disabled  = pSaveBoardAs->disabled  = pReloadBoard->disabled  = TRUE;
		pSaveBoard->canFocus  = pSaveBoardAs->canFocus  = pReloadBoard->canFocus  = FALSE;
	}

	dialog.Insert(pSaveBoard);
	dialog.Insert(pSaveBoardAs);
	dialog.Insert(pReloadBoard);
	dialog.Insert(new TextButton(44, y, 84,   bh, "&Load board", mrLoad));	y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&New board", mrNew));	y+=bh;	
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&Quit editor", mrQuit));	y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&Options", mrOptions));	y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "A&bout", mrAbout));		y+=bh;

	y+=(pd<<1);
	if (gMapLoaded && gAutosave.max > 0 && gAutosave.interval > 0)
	{
		dialog.Insert(new Label(8, y, ">AUTOSAVES", kColorBlue)); y+=(pFont->height+pd);
		for (i = ClipHigh(gAutosave.max, 3); i > 0; i--)
		{
			sprintf(asavename[i], "%s%d.%s", gAutosave.basename, i, gExtNames[kMap]);
			if (fileExists(asavename[i]))
			{
				j = sprintf(asavename[i], "%s%d", gAutosave.basename, i);
				TextButton *pbButton = new TextButton(x, y, 124, bh, asavename[i], mrAsave + i);
				dialog.Insert(pbButton);
				y+=bh;
			}
		}

		y+=(pd<<1);
	}

	dialog.Insert(new Label(8, y, ">TOOLS", kColorBlue)); y+=(pFont->height+pd);
	TextButton* pPreviewMode   = new TextButton(x, y, 124,  bh, "&Preview mode", mrToolPreviewMode);		y+=bh;
	TextButton* pExplodeSeq    = new TextButton(x, y, 124,  bh, "&Exploder sequence", mrToolExpSeq);		y+=bh;
	TextButton* pCleanChannel  = new TextButton(x, y, 124,  bh, "&Channel cleaner", mrToolCleanChannel);	y+=bh;
	TextButton* pImportWizard  = new TextButton(x, y, 124,  bh, "&Import wizard", mrToolImportWizard);		y+=bh;
	
	i = 124/3;
	TextButton* pButtonQavedit = new TextButton(x, y, i,  bh, editors[0], mrToolQavEdit); 	x+=i;
	TextButton* pButtonSeqedit = new TextButton(x, y, i,  bh, editors[1], mrToolSeqEdit); 	x+=i;
	TextButton* pButtonArtedit = new TextButton(x, y, i,  bh, editors[2], mrToolArtEdit); 	x=4;
	y+=bh;
	
	if (!gMapLoaded)
	{
		pCleanChannel->fontColor = pPreviewMode->fontColor = pExplodeSeq->fontColor = 8;
		pCleanChannel->disabled  = pPreviewMode->disabled  = pExplodeSeq->disabled = TRUE;
		pCleanChannel->canFocus  = pPreviewMode->canFocus  = pExplodeSeq->canFocus = FALSE;
	}
	
	dialog.height = y + (pd*6);
	dialog.Insert(pPreviewMode);
	dialog.Insert(pExplodeSeq);
	dialog.Insert(pCleanChannel);
	dialog.Insert(pImportWizard);
	dialog.Insert(pButtonQavedit);
	dialog.Insert(pButtonSeqedit);
	dialog.Insert(pButtonArtedit);

	ShowModal(&dialog);
	return dialog.endState;

}

int xmpMenuProcess() {

	char* filename = NULL;
	int result = mrMenu, len = 0, i = 0;
	RESHANDLE hRes = NULL;

	while( 1 )
	{
		switch (result) {
			default:
				if (result >= mrAsave && result < mrAsaveMax)
				{
					sprintf(gPaths.maps,"%s%d.%s", gAutosave.basename, result - mrAsave, gExtNames[kMap]);
					result = mrLoadAsave;
					break;
				}
				result = mrMenu;
				break;
			case mrNo:
			case mrCancel:
				return result;
			case mrMenu:
				memset(buffer, 0, sizeof(buffer));
				result = xmpMenuCreate("Main menu");
				break;
			case mrAbout:
				xmpAbout();
				result = mrMenu;
				break;
			case mrOptions:
				xmpOptions();
				result = mrMenu;
				break;
			case mrToolPreviewMode:
				if (!previewMenuProcess())
				{
					result = mrMenu;
					break;
				}
				else if (gPreviewMode) previewStop();
				previewStart();
				return result;
			case mrToolCleanChannel:
				if
				(
					Confirm
					(
					"This tool will search and reset unlinked RX and TX\n"
					"channels for all objects in current map. Note\n"
					"that other object properties remains\n"
					"unchanged!\n"
					"\n"
					"Do you wish to continue now?"
					)
				)
				{
					getPath(gPaths.maps, buffer, TRUE);
					strcat(buffer, kChannelCleanerLog);
					if ((i = toolChannelCleaner(buffer)) > 0)
					{
						Alert
						(
							"There is %d channels were freed in total! See\n"
							"the \"%s\" file created in the map\n"
							"directory for detailed information\n"
							"if manual editing required.", i, kChannelCleanerLog
						);
					}
					else
					{
						Alert("No unlinked channels found.");
					}
				}
				return result;
			case mrToolExpSeq:
				if (toolExploderSeq() == mrOk) return result;
				result = mrMenu;
				break;
			case mrToolImportWizard:
			{
				IMPORT_WIZARD wizard;
				wizard.ShowDialog();			
			}
			return result;
			case mrToolSeqEdit:
				gTool.cantest = gMapLoaded;
				seqeditStart(NULL);
				return result;
			case mrToolArtEdit:
				gTool.cantest = gMapLoaded;
				if (!gMapLoaded) gTileView.bglayers = 0;
				artedStart(NULL, TRUE);
				return result;
			case mrToolQavEdit:
				gQaved.Start(NULL);
				return result;
			case mrNew:
				if (gMapLoaded && !Confirm("Start new board now?"))
				{
					result = mrMenu;
					break;
				}
				boardReset();
				sprintf(gPaths.maps, kDefaultMapName);
				scrSetMessage("New board started.");
				gMapLoaded = TRUE;
				return result;
			case mrSave:
			case mrSaveAs:
				if ((len = strlen(gPaths.maps)) <= 0) sprintf(gPaths.maps, kDefaultMapName), result = mrSaveAs;
				switch (result) {
					case mrSave:

						// erase user's secrets counter and set editor's.
						if (gMisc.autoSecrets) setupSecrets();

						fixspritesectors();
						CleanUpMisc(); // obsolete xobject checkings and misc stuff
						CleanUp();

						AutoAdjustSprites(); // fix sprite type attributes

						gTimers.autosave = gFrameClock + gAutosave.interval;

						boardSave(gPaths.maps, FALSE);
						scrSetMessage("Board saved to \"%s\".", gPaths.maps);
						BeepOk();
						return result;
					default:
						result = mrMenu;
						if ((filename = dirBrowse("Save board as", gPaths.maps, ".map", kDirExpTypeSave, 0)) != NULL)
						{
							sprintf(gPaths.maps, filename);
							result = mrSave;
						}
						break;
				}
				break;
			case mrReload:
				hgltReset();
				// no break
			case mrLoad:
			case mrLoadAsave:
				if (result != mrLoad) filename = gPaths.maps;
				else
				{
					if ((filename = dirBrowse("Load board", gPaths.maps, ".map")) == NULL)
					{
						result = mrMenu;
						break;
					}

					sprintf(gPaths.maps, filename);
				}

				if (!fileExists(filename, &hRes))
				{
					Alert("The board file \"%s\" not found!", filename);
					result = mrMenu;
					break;
				}
				if (boardLoad(filename) == -2) break;
				return result;
			case mrQuit:	
				switch (result = DlgSaveChanges("Save changes?", gArtEd.asksave)) {
					case 2:
					case 1:
						boardSave(gPaths.maps, FALSE);
						if (result > 1)
						{
							extVoxUninit();
							tileUninitSystemTiles();
							artedSaveArtFiles();
							artedSaveDatFiles();
						}
						// no break
					case 0:
						xmpQuit();
						return 0;
					default:
						result = mrMenu;
						break;
				}
				break;
		}
	}
}


BOOL getDataNameOf(short otype, short idx, short didx, char* out) {

	switch (otype) {
		case OBJ_SPRITE:
			if (!irngok(sprite[idx].type, 0, LENGTH(gSpriteDataNames) - 1)) break;
			else if (gSpriteDataNames[sprite[idx].type][didx] == NULL) break;
			sprintf(out, gSpriteDataNames[sprite[idx].type][didx]);
			return TRUE;
		case OBJ_FLOOR:
		case OBJ_CEILING:
		case OBJ_SECTOR:
			if (!irngok(sector[idx].type, 0, LENGTH(gSectorDataNames) - 1)) break;
			else if (gSectorDataNames[sector[idx].type][0] == NULL) break;
			sprintf(out, gSectorDataNames[sector[idx].type][0]);
			return TRUE;
		case OBJ_WALL:
		case OBJ_MASKED:
			if (!irngok(wall[idx].type, 0, LENGTH(gWallDataNames) - 1)) break;
			else if (gWallDataNames[wall[idx].type][0] == NULL) break;
			sprintf(out, gWallDataNames[wall[idx].type][0]);
			return TRUE;
	}

	return FALSE;
}

void processMove() {
	
	#define kMoveVal1 1024 // 4<<8
	#define kMoveVal2 2048 // 8<<8
	#define kMoveVal3 4096 // 16<<8
	#define kMoveVal4 6144 // 24<<8
	#define kMoveVal5 3072 // 12<<8
	#define kVelStep1 8
	#define kVelStep2 2
	#define kVelStep3 16
	#define kVelStep4 12
	
	BOOL block = FALSE;
	BYTE ctrl, shift, alt; BOOL in2d = (qsetmode != 200);
	BOOL strafe = (!in2d && gMouseLook.strafe && gMouseLook.mode);
	int goalz, hiz = 0x80000000, loz = 0x7FFFFFFF, hihit = 0, lohit = 0, nSector;
	int i, hit, px = posx, py = posy, pz = posz, xvect = 0, yvect = 0;
	keyGetHelper(NULL, &ctrl, &shift, &alt);
	
	if (!in2d && (gMouse.hold & 2) && gMouse.wheel)
	{
		if (zmode == 0)
			zmode = 3;
		
		if (gMouse.wheel < 0)
		{
			posz = posz - (kVelStep1<<6) * gFrameTicks;
		}
		else if (gMouse.wheel > 0)
		{
			posz = posz + (kVelStep1<<6) * gFrameTicks;
			
		}
	}
	else if (keystatus[KEY_UP])
		vel = min(vel + kVelStep1 * gFrameTicks, 127);
	else if (keystatus[KEY_DOWN])
		vel = max(vel - kVelStep1 * gFrameTicks, -128);
	
	
	if (keystatus[KEY_LEFT])
	{
		if (strafe)
		{
			svel   = min(svel   + kVelStep3 * gFrameTicks, 127);
			angvel = 0;
		}
		else
			angvel = max(angvel - kVelStep3 * gFrameTicks, -128);
	}
	else if (keystatus[KEY_RIGHT])
	{
		if (strafe)
		{
			svel 	= max(svel   - kVelStep3 * gFrameTicks, -128);
			angvel  = 0;
		}
		else
			angvel = min(angvel + kVelStep3 * gFrameTicks, 127);
	}
	
	
	if (angvel)
	{
		i = 14;
		if (shift)
			i = 10;
		
		ang = (short)((ang + (angvel * gFrameTicks) / i) & kAngMask);
		
		if (angvel > 0)
			angvel = max(angvel - kVelStep4 * gFrameTicks, 0);
		else
			angvel = min(angvel + kVelStep4 * gFrameTicks, 0);
	}

	if (vel || svel)
	{
		doubvel = gFrameTicks;
		if (shift)
			doubvel += gFrameTicks;
		
		doubvel += perc2val(doubvel, 40);
		
		if (vel)
		{
			xvect += mulscale30(vel * doubvel >> 2, Cos(ang));
			yvect += mulscale30(vel * doubvel >> 2, Sin(ang));
			
			if (vel > 0) 
				vel = max(vel - kVelStep2 * gFrameTicks, 0);
			else
				vel = min(vel + kVelStep2 * gFrameTicks, 0);
		}
		
		if (svel)
		{
			xvect += mulscale30(svel * doubvel >> 2, Sin(ang));
			yvect -= mulscale30(svel * doubvel >> 2, Cos(ang));
			
			if (svel > 0) 
				svel = max(svel - kVelStep1 * gFrameTicks, 0);
			else
				svel = min(svel + kVelStep1 * gFrameTicks, 0);
		}
		

		if (gNoclip || cursectnum < 0)
		{
			block = 1;
			posx+=xvect, posy+=yvect;
			if (cursectnum < 0)
				updatesector(posx, posy, &cursectnum);
		}
		else if (gPreviewMode)
		{
			if ((hit = clipmove(&px, &py, &pz, &cursectnum, xvect<<14, yvect<<14, kPlayerRadius, kMoveVal1, kMoveVal1, BLOCK_MOVE)) != 0)
			{
				i = (hit & 0x3FFF);
				switch (hit & 0xC000) {
					case 0x8000:
						if (wall[i].nextwall < 0) block = 1;
						if (wall[i].extra > 0)
						{
							XWALL* pXWall = &xwall[wall[i].extra];
							if (pXWall->triggerTouch)
							{
								if (gModernMap)
								{
									trTriggerWall(i, pXWall, kCmdWallTouch);
									block = 1;
								}
								else
								{
									previewMessage("Wall #%d touch error: the map must be modern to make this flag work!", i);
									BeepFail();
								}
							}
						}
						break;
					case 0xC000:
						if (sprite[i].extra > 0)
						{
							XSPRITE* pXSpr = &xsprite[sprite[i].extra];
							if (pXSpr->triggerTouch)
							{
								block = 1;
								if (gModernMap || !pXSpr->state)
									trTriggerSprite(i, pXSpr, kCmdSpriteTouch);
							}
						}
						break;
				}
				
				if (block)
					posx = px, posy = py, posz = pz;
			}
		}
		
		if (!block) // must repeat
			clipmove(&posx, &posy, &posz, &cursectnum, xvect<<14, yvect<<14, kPlayerRadius, kMoveVal1, kMoveVal1, clipmovemask3d);
	}
	
	if (!gNoclip)
	{
		if (cursectnum >= 0)
		{
			getzrange(posx, posy, posz, cursectnum, &hiz, &hihit, &loz, &lohit, kPlayerRadius, BLOCK_MOVE);
			if ((hihit & 0xe000) == 0x4000 && (hihit & 0x1FFF) == cursectnum)
			{
				if (sector[cursectnum].ceilingstat & kSectParallax)
					hiz = -klabs(getceilzofslope(cursectnum, posx, posy) << 2);
			}
			
			if (gPreviewMode && gModernMap)
			{
				if ((hihit & 0xE000) == 0xC000)
				{
					i = hihit & 0x1FFF;
					if (sprite[i].extra > 0)
					{
						XSPRITE* pXSpr = &xsprite[sprite[i].extra];
						if (pXSpr->triggerTouch)
							trTriggerSprite(i, pXSpr, kCmdSpriteTouch);
					}
				}
				
				if ((lohit & 0xE000) == 0xC000)
				{
					i = lohit & 0x1FFF;
					if (sprite[i].extra > 0)
					{
						XSPRITE* pXSpr = &xsprite[sprite[i].extra];
						if (pXSpr->triggerTouch)
							trTriggerSprite(i, pXSpr, kCmdSpriteTouch);
					}
				}
			}
		}
		else
		{
			hiz = loz = posz;
		}
	}
	else if (cursectnum >= 0)
	{
		nSector = cursectnum;
		FindSector(posx, posy, posz, &nSector);
		if (nSector >= 0)
			cursectnum = nSector;
	}

	if (zmode == 0)
	{
		goalz = loz - kensplayerheight;   	// playerheight pixels above floor
		if (goalz < hiz+kMoveVal3)   		//ceiling&floor too close
			goalz = (loz+hiz) >> 1;

		if (!in2d && !alt)
		{
			if (keystatus[KEY_A])
			{
				if (ctrl && !gMouseLook.mode)
				{
					if (horiz > 0) horiz -= 4;
				}
				else
				{
					goalz -= kMoveVal3;
					if (shift)
						goalz-=kMoveVal4;
				}
			}
			
			if (keystatus[KEY_Z])                            //Z (stand low)
			{
				if (ctrl && !gMouseLook.mode)
				{
					if (horiz < 200) horiz += 4;
				}
				else
				{
					goalz+=kMoveVal5;
					if (shift)
						goalz+=kMoveVal5;
				}
			}
		}

		if (goalz != posz)
		{
			if (posz < goalz) hvel += 64 * gFrameTicks >> 1;
			if (posz > goalz) hvel = ((goalz-posz)>>3);
				
			posz += hvel * gFrameTicks >> 1;
			if (posz > loz-kMoveVal1) posz = loz-kMoveVal1, hvel = 0;
			if (posz < hiz+kMoveVal1) posz = hiz+kMoveVal1, hvel = 0;
		}
	}
	else
	{
		goalz = posz;
		if (!in2d && !alt)
		{
			if (keystatus[KEY_A])
			{
				if (ctrl && !gMouseLook.mode)
				{
					if (horiz > 0) horiz -= 4;
				}
				else if (zmode != 1) goalz-=kMoveVal3;
				else zlock+=kMoveVal1, keystatus[KEY_A] = 0; 
			}
			
			if (keystatus[KEY_Z])
			{
				if (ctrl && !gMouseLook.mode)
				{
					if (horiz < 200) horiz += 4;
				}
				else if (zmode != 1) goalz+=kMoveVal3;
				else if (zlock > 0)	 zlock-=kMoveVal1, keystatus[KEY_Z] = 0;
			}
		}
		
		if (goalz < hiz+kMoveVal1)	goalz = hiz+kMoveVal1;
		if (goalz > loz-kMoveVal1)	goalz = loz-kMoveVal1;
		
		if (zmode == 1)
			goalz = loz-zlock;

		if (goalz < hiz+kMoveVal1)   //ceiling&floor too close
			goalz = (loz + hiz)>>1;

		if (zmode == 1)
			posz = goalz;

		if (goalz != posz)
		{
			if (posz < goalz) hvel += (128 * gFrameTicks) >> 1;
			if (posz > goalz) hvel -= (128 * gFrameTicks) >> 1;
			
			posz += (hvel * gFrameTicks) >> 1;
			if (posz > loz-kMoveVal1) posz = loz-kMoveVal1, hvel = 0;
			if (posz < hiz+kMoveVal1) posz = hiz+kMoveVal1, hvel = 0;
		}
		else
			hvel = 0;
	}
	
	if (!in2d && cursectnum >= 0)
	{
		int omedium = gMisc.palette;
		
		if (gUpperLink[cursectnum] >= 0 || gLowerLink[cursectnum] >= 0)
		{
			int val = (zmode == 0) ? kensplayerheight : 2048;
			int legs = posz + val;
			int nSect = cursectnum, nLink = 0;
			
			if ((zmode || gPreviewMode) || (!zmode && (keystatus[KEY_A]|keystatus[KEY_Z])))
			{
				if ((nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect)) == 0)
				{
					legs = posz - val;
					if ((zmode == 0 && keystatus[KEY_A]) || zmode != 0)
						nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect);
				}
			}

			if (nLink > 0)
			{
				scrSetMessage("Moving through stack!");
				
				switch (nLink) {
					case kMarkerLowLink:
					case kMarkerLowWater:
					case kMarkerLowStack:
					case kMarkerLowGoo:
						cursectnum = nSect;
						if (gPreviewMode)
							gMisc.palette = kPal0;
						posz = getflorzofslope(nSect, posx, posy) - val - 1024;
						break;
					default:
						cursectnum = nSect;
						posz = getceilzofslope(nSect, posx, posy) + val + 1024;
						if (gPreviewMode && cursectnum >= 0 && isUnderwaterSector(cursectnum))
						{
							if (nLink == kMarkerUpWater) 	gMisc.palette = kPal1;
							else if (nLink == kMarkerUpGoo) gMisc.palette = kPal3;
							
							if (zmode == 0)
								zmode = (gMouseLook.mode) ? 3 : 2;
						}
						break;
				}
			}
		}
		
		if (gPreviewMode)
		{
			BOOL water = isUnderwaterSector(cursectnum);
			if (!water && gMisc.palette != kPal0) gMisc.palette = kPal0;
			else if (water && gMisc.palette == kPal0)
				gMisc.palette = kPal1;
			
			if (omedium != gMisc.palette)
				scrSetPalette(gMisc.palette);
		}
	}
	
	switch (key) {
		case KEY_CAPSLOCK:
			if (alt)
			{
				gNoclip = (gNoclip) ? 0 : 1;				
				scrSetMessage("Noclip %s", onOff(gNoclip));
				if (Beep(gNoclip))
					zmode = (gMouseLook.mode) ? 3 : 2;
			}
			else if (!in2d && !shift)
			{
				while( 1 )
				{
					switch(zmode = IncRotate(zmode, 4)) {
						case 0:
							if (gNoclip) continue;
							break;
						case 1:
							if (!gMisc.zlockAvail) continue;
							else zlock = (loz-posz) & ~0x3FF;
							break;
						case 4:
							if (gMouseLook.mode) continue;
							break;
					}
					break;
				}
				
				scrSetMessage("ZMode = %s", gZModeNames[zmode], onOff(gNoclip));
			}
			break;
	}
}

void setStartPos(int x, int y, int z, int ang)
{
	updatesector(x, y, &startsectnum);
	z = (startsectnum >= 0) ? getflorzofslope(startsectnum, x, y) : 0;
	
	startposx = x;
	startposy = y;
	startposz = z - kensplayerheight;
	startang  = ang;
}

void boardPreloadTiles()
{
	int i, j, swal, ewal;
	for (i = 0; i < numsectors; i++)
	{
		tilePreloadTile(sector[i].ceilingpicnum);
		tilePreloadTile(sector[i].floorpicnum);
		
		getSectorWalls(i, &swal, &ewal);
		for (j = swal; j <= ewal; j++)
		{
			tilePreloadTile(wall[i].picnum);
			if ((wall[i].cstat & kWallMasked) && wall[i].overpicnum >= 0)
				tilePreloadTile(wall[i].overpicnum);
		}
		
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			tilePreloadTile(sprite[i].picnum);
	}
}

int boardLoad(char *filename)
{
	BOOL bloodMap;
	BYTE* pData = NULL;
	short j, i, k, f, sect;
	short xidx, sprsect;
	short sectors = 0, walls = 0, sprites = 0;
	char mapname[_MAX_PATH];
	int nSize = 0;
	keyClear();

	sprintf(mapname, filename);	ChangeExtension(mapname, getExt(kMap));
	if ((nSize = fileLoadHelper(mapname, &pData)) <= 0)
		return -1;
	
	IOBuffer* pIo = new IOBuffer(nSize, pData);
	if ((i = dbCheckMap(pIo, gSuppMapVersions, LENGTH(gSuppMapVersions), &bloodMap)) <= 0)
	{
		Alert("Unknown map file format!");
		free(pData);
		return -1;
	}
	else if (!bloodMap)
	{
		IMPORT_WIZARD_MAP_ARG mapArg;
		mapArg.filepath = mapname;	mapArg.pIo = pIo;
		mapArg.version  = i;		mapArg.blood = bloodMap;
		mapArg.allowSel = 0;
		
		IMPORT_WIZARD wizard;
		i = wizard.ShowDialog(&mapArg);
		free(pData);
		return i;	
	}
	
	sprintf(buffer3, "Loading the %s", mapname);
	splashScreen(buffer3);
	
	// save all the highlighted stuff before loading other map
	if (highlightsectorcnt > 0)
	{
		cpyObjectClear();
		updatenumsprites();

		// check if it's enough space in arrays first
		for(i = k = j = 0; i < highlightsectorcnt; i++)
		{
			j += sector[highlightsector[i]].wallnum;
			for (f = headspritesect[highlightsector[i]]; f != -1; f = nextspritesect[f])
			{
				if (sprite[f].statnum < kMaxStatus)
					k++;
			}
		}

		buffer3[0] = 0;
		if (numsectors + i >= kMaxSectors)		sprintf(buffer3, "sectors");
		else if (numwalls + j >= kMaxWalls)		sprintf(buffer3, "walls");
		else if (numsprites + k >= kMaxSprites) sprintf(buffer3, "sprites");
		if (buffer3[0])
		{
			Alert("Failed to COPY objects: too many %s to backup!", buffer3);
			highlightsectorcnt = -1;
		}
		else
		{
			buffer3[0] = 0;
			getFilename(gPaths.maps, buffer3);
			switch (YesNoCancel("Import %d sectors with %d walls and %d sprites in \"%s\" map?", i, j, k, buffer3)) {
				case mrCancel:
					return -1;
				case mrNo:
					highlightsectorcnt = -1;
					break;
			}
		}

		//put sectors and walls to end of lists
		for(i = 0, k = kMaxWalls; i < highlightsectorcnt; i++)
			k -= sector[highlightsector[i]].wallnum;

		for(i = 0; i < highlightsectorcnt; i++) {


			sect = highlightsector[i];
			copysector(sect,(short)(kMaxSectors - highlightsectorcnt + i),(short)k, 0);
			k += sector[sect].wallnum;

			// sectors
			cpysector[cpysectorcnt] = sector[sect];
			if (sector[sect].extra > 0)
			{
				cpyxsector[cpysectorcnt] = xsector[sector[sect].extra];

				// markers that sector own can be outside highlight so
				// we have to add it in by just changing sectnum since
				// it doesn't really matter for markers
				if ((j = (short)xsector[sector[sect].extra].marker0) >= 0 && sprite[j].statnum == kStatMarker)
					ChangeSpriteSect(j, sect);
				if ((j = (short)xsector[sector[sect].extra].marker1) >= 0 && sprite[j].statnum == kStatMarker)
					ChangeSpriteSect(j, sect);
			}

			cpysectorcnt++;

			// walls of sectors
			for (j = sector[sect].wallptr; j < sector[sect].wallptr + sector[sect].wallnum; j++)
			{
				cpywall[cpywallcnt] = wall[j];
				if (wall[j].extra > 0) cpyxwall[cpywallcnt] = xwall[wall[j].extra];
				cpywallcnt++;
			}

			// sprites of sectors
			for (j = headspritesect[sect]; j != -1; j = nextspritesect[j])
			{
				if (sprite[j].statnum < kMaxStatus)
				{
					cpysprite[cpyspritecnt] = sprite[j];
					if (sprite[j].extra > 0) cpyxsprite[cpyspritecnt] = xsprite[sprite[j].extra];
					cpyspritecnt++;
				}
			}
		}
	}
	
	boardReset(kHgltPoint | kHgltGradient);
	dbLoadMap(pIo, gCmtPrefs.enabled ? mapname : NULL);
	free(pData);

	// restore highlight stuff after loading another map
	if (highlightsectorcnt > 0)
	{
		buffer3[0] = 0;
		if (numsectors + cpysectorcnt >= kMaxSectors) 		sprintf(buffer3, "sectors");
		else if (numwalls + cpywallcnt >= kMaxWalls)		sprintf(buffer3, "walls");
		else if (numsprites + cpyspritecnt >= kMaxSprites)	sprintf(buffer3, "sprites");
		if (buffer3[0])
		{
			Alert("Failed to PASTE objects: too many %s in destination map!", buffer3);
			highlightsectorcnt = -1;
		}

		//Re-attach sectors
		for(i = 0; i < highlightsectorcnt; i++)
		{
			sprsect = highlightsector[i];
			copysector((short)(kMaxSectors - highlightsectorcnt + i), numsectors, numwalls, 0);
			numwalls += sector[numsectors].wallnum;
			highlightsector[i] = sect = numsectors++;

			// copy xsector
			if (cpysector[sectors].extra > 0)
			{
				xidx = dbInsertXSector(sect);
				cpyxsector[sectors].reference = xsector[xidx].reference;
				memcpy(&xsector[xidx], &cpyxsector[sectors], sizeof(XSECTOR));

				// fix sector marker sprites
				if (xsector[xidx].marker0 >= 0 || xsector[xidx].marker1 >= 0)
				{

					for (j = 0; j < cpyspritecnt; j++)
					{
						if (cpysprite[j].statnum != kStatMarker) continue;
						if (xsector[xidx].marker0 != cpysprite[j].index && xsector[xidx].marker1 != cpysprite[j].index)
							continue;

						k = (short)InsertSprite(sect, kStatMarker);
						memcpy(&sprite[k], &cpysprite[j], sizeof(spritetype));
						sprite[k].owner = sprite[k].sectnum = sect;
						sprite[k].index = k;

						if (xsector[xidx].marker0 == cpysprite[j].index) xsector[xidx].marker0 = sprite[k].index;
						else xsector[xidx].marker1 = sprite[k].index;
					}
				}
			}

			// copy sprites
			for (j = 0; j < cpyspritecnt; j++)
			{
				if (cpysprite[j].sectnum != sprsect || cpysprite[j].statnum == kStatMarker)
					continue;

				k = (short)InsertSprite(sect, cpysprite[j].statnum);
				memcpy(&sprite[k], &cpysprite[j], sizeof(spritetype));
				sprite[k].sectnum = sect;
				sprite[k].index = k;

				// copy xsprite
				if (sprite[k].extra > 0)
				{
					xidx = dbInsertXSprite(k);
					cpyxsprite[j].reference = xsprite[xidx].reference;
					memcpy(&xsprite[xidx], &cpyxsprite[j], sizeof(XSPRITE));
				}
			}

			// copy walls
			for(j = sector[sect].wallptr; j < sector[sect].wallptr + sector[sect].wallnum; j++)
			{
				// copy xwall
				if (cpywall[walls].extra > 0)
				{
					xidx = dbInsertXWall(j);
					cpyxwall[walls].reference = xwall[xidx].reference;
					memcpy(&xwall[xidx], &cpyxwall[walls], sizeof(XWALL));
				}

				walls++;
			}

			sectors++;

		}

		for(i = 0; i < highlightsectorcnt; i++)
		{
			k = sector[highlightsector[i]].wallptr;
			f = (short) (k + sector[highlightsector[i]].wallnum);
			for(j = k; j < f; j++)
			{
				if (wall[j].nextwall >= 0)
					checksectorpointer(wall[j].nextwall, wall[j].nextsector);

				checksectorpointer((short)j, highlightsector[i]);
			}
		}
	}
	
	scrLoadPLUs();
	if (gModernMap && gMisc.showTypes != 2)
	{
		gMisc.showTypes = 2;
		InitializeNames();
	}
	
	formatMapInfo(gMapInfoStr);
	gMapLoaded = TRUE;
	boardPreloadTiles();
	AutoAdjustSprites();
	CleanUpMisc();
	CleanUp();
	
	updatesector(posx, posy, &cursectnum); clampCamera();
	gMapedHud.SetMsgImp(256, gMapInfoStr);
	return 0;
}

int boardSave(char* filename, BOOL autosave)
{
	int i;
	keyClear();
	UndoSectorLighting(); // fix up floor and ceiling shade values for sectors that have dynamic lighting
	grshUnhgltObjects(-1, FALSE); // restore shade and picnum for objects that was highlighted for gradient shading
	fixspritesectors();
	
	if (!autosave)
		gMapRev++;
	
	
	CleanUp();
	setStartPos(startposx, startposy, startposz, startang); // this updates startsectnum as well
	if ((i = dbSaveMap(filename, (mapversion == 7 || gCompat.modernMap))) != 0)
	{
		if (!autosave)
			Alert("Failed to save %s (code: %d)", filename, i);
	}
	else
	{
		asksave = 0;
	}
	
	if (!gResetHighlight) grshHgltObjects(-1); // change picnum and shade back to highlighted for highlighted objects
	formatMapInfo(gMapInfoStr);
	return i;
}

void boardReset(int hgltreset)
{
	gTimers.autosave = gFrameClock + gAutosave.interval;
	numsprites  = numsectors = numwalls		= 0;
	numxsectors = numxwalls  = numxsprites	= 0;
	gModernMap  = (BOOL)gCompat.modernMap;
	
	asksave    = 0;
	horiz	   = 100;
	startposz  = posz = 0;
	startposx  = posx = 0;
	startposy  = posy = 0;
	startang   = ang  = 1536;
	cursectnum = startsectnum = -1;
	somethingintab = 255;
	gMapRev    = 0;
	
	memset(joinsector, -1, sizeof(joinsector));
	
	if (!hgltreset) hgltReset(); // reset using default params
	else hgltReset(hgltreset); 	// override param
	
	memset(gMapInfoStr, 0, sizeof(gMapInfoStr));
	gCommentMgr.DeleteAll();
	gXTracker.TrackClear();
	eraseExtra();
	dbInit();
}