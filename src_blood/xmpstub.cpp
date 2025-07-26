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
#include "common_game.h"
#include "xmpstub.h"
#include "sectorfx.h"
#include "tracker.h"
#include "screen.h"
#include "tile.h"
#include "fire.h"
#include "preview.h"
#include "hglt.h"
#include "grdshd.h"
#include "aadjust.h"
#include "xmpevox.h"
#include "xmpexplo.h"
#include "xmparted.h"
#include "xmpseqed.h"
#include "xmpqaved.h"
#include "xmpview.h"
#include "xmpsky.h"
#include "mapcmt.h"

typedef void (*EDITLOOPPROCESS_FUNC)(void);

char h = 0;
BYTE key, ctrl, alt, shift;
short gHighSpr = -1, gHovSpr = -1;
short gHovWall = -1, gHovStat = -1;
int bpsx, bpsy, bpsz, bang, bhorz;
//int bsrcwall, bsrcsector, bsrcstat, bsrchit;

static int oldnumwalls = 0, oldnumsectors = 0, oldnumsprites = 0;
short temptype = 0, tempslope = 0, tempidx = -1;
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

char *gSpriteNames[1024];
char *gSpriteCaptions[1024];
char *gSpriteDataNames[1024][4];

char *gWallNames[1024];
char *gWallCaptions[1024];
char *gWallDataNames[1024][1];

char *gSectorNames[1024];
char *gSectorCaptions[1024];
char *gSectorDataNames[1024][1];

char* gCrypticArt = "CPART15.AR_";
char buffer[kBufferSize]  = "";
char buffer2[kBufferSize] = "";
char buffer3[kBufferSize] = "";
char gMapInfoStr[kBufferSize] = "";

int boardSnapshotLoad(BYTE* pData, int nLen, char dir);
int boardSnapshotMake(BYTE** pData, int* crcSiz);
SNAPSHOT_MANAGER gMapSnapshot(boardSnapshotMake, boardSnapshotLoad);
OBJECT_LIST gModernTypes;

struct COMPAT_STRING
{
	unsigned char tcol;
	unsigned char bcol;
	char* text;
};

COMPAT_STRING gCompatString[] =
{
	{kColorGrey16, kColorGreen, "VANILLA MAP"},
	{kColorLightMagenta, kColorRed, "MODERN MAP"},
};

char *gCaptionStyleNames[] =
{
	"Disabled",
	"Mapedit style",
	"BUILD style",
};

char* gShowMapNames[] =
{
	"Disabled",
	"Floors",
	"Ceilings",
};

char* gBoolNames[2] =
{
	"OFF",		// 0
	"ON",		// 1
};

char* gIsNotNames[2] =
{
	"is not",			// 0
	"is",				// 1
};

char* gYesNoNames[2] =
{
	"no",				// 0
	"yes",				// 1
};

char* gBusyNames[4] =
{
	"Sine",				// 0 kBusySine
	"Linear", 			// 1 kBusyLinear
	"SlowOff",			// 2 kBusySlowOff
	"SlowOn",			// 3 kBusySlowOn
};

char* gWaveNames[12] =
{
	"None",				// 0 kWaveNone
	"Square",			// 1 kWaveSquare
	"Saw",				// 2 kWaveSaw
	"Ramp up",			// 3 kWaveRampup
	"Ramp down",		// 4 kWaveRampdown
	"Sine",				// 5 kWaveSine
	"Flicker1",			// 6 kWaveFlicker1
	"Flicker2",			// 7 kWaveFlicker2
	"Flicker3",			// 8 kWaveFlicker3
	"Flicker4",			// 9 kWaveFlicker4
	"Strobe",			// 10 kWaveStrobe
	"Search",			// 11 kWaveSearch
};

char* gExtNames[] =
{
	"map",
	"seq",
	"art",
	"qav",
};

char* gRespawnNames[4] =
{
	"Optional",				// 0
	"Never",				// 1
	"Always",				// 2
	"Permanent",			// 3
};

char* gDepthNames[8] =
{
	"None",					// 0
	"Tread",				// 1
	"Puddle",				// 2
	"Wade",					// 3
	"Pond",					// 4
	"Bath",					// 5
	"Jungle",				// 6
	"Swim",					// 7
};

char *gKeyItemNames[] =
{
	"None",					// 0
	"Skull",				// 1
	"Eye",					// 2
	"Fire",					// 3
	"Dagger",				// 4
	"Spider",				// 5
	"Moon",					// 6
	"key 7",				// 7
};

char* gSearchStatNames[] =
{
	"Wall",					// 0, OBJ_WALL
	"Ceiling",				// 1, OBJ_CEILING
	"Floor",				// 2, OBJ_FLOOR
	"Sprite",				// 3, OBJ_SPRITE
	"Masked wall",			// 4, OBJ_MASKED
	gSearchStatNames[3],	// 5, OBJ_FLATSPRITE
	"Sector",				// 6
	"Unknown",				// 7
};

char *gZModeNames[4] =
{
	"Gravity",				// 0
	"Step",					// 1
	"Free",					// 2
	"Mouse look",			// 3
};

char* gCommandNames[256] =
{
	"OFF",					// kCmdOff
	"ON",					// kCmdOn
	"State",				// kCmdState
	"Toggle",				// kCmdToggle
	"NOT State",			// kCmdNotState
	"Link",					// kCmdLink
	"Lock",					// kCmdLock
	"Unlock",				// kCmdUnlock
	"Toggle Lock",			// kCmdToggleLock
	"Sector Stop OFF",		// kCmdStopOff
	"Sector Stop ON",		// kCmdStopOn
	"Sector Stop Next",		// kCmdStopNext
	// ...................
	// ...................
	// to be filled at init
};

NAMED_TYPE gModernCommandNames[] =
{
	{13, 				"Sector Pause"},
	{14, 				"Sector Continue"},
	{15, 				"Set dude flags"},
	{16,				"Kill events (full)"},
};

// player control data names
SPECIAL_DATA_NAMES pCtrlDataNames[32] =
{
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

NAMED_TYPE gToolNames[] =
{
	{kToolMapEdit, 		"MAPedit"},
	{kToolSeqEdit, 		"SEQedit"},
	{kToolArtEdit,		"ARTedit"},
	{kToolQavEdit,		"QAVedit"},
};

NAMED_TYPE gGameObjectGroupNames[] =
{
	{ kOGrpNone, 	"None" },
	{ kOGrpModern, 	"Modern" },
	{ kOGrpDude, 	"Enemy" },
	{ kOGrpWeapon, 	"Weapon" },
	{ kOGrpAmmo, 	"Ammo" },
	{ kOGrpAmmoMix, "Mixed ammo" },
	{ kOGrpItem, 	"Item" },
	{ kOGrpHazard, 	"Hazard" },
	{ kOGrpMisc, 	"Misc" },
	{ kOGrpMarker, 	"Marker" },
};

NAMED_TYPE gDifficNames[6] =
{
	{0,						"Still Kicking"},
	{1,						"Pink On The Inside"},
	{2,						"Lighty Broiled"},
	{3,						"Well Done"},
	{4,						"Extra Crispy"},
	{5,						"All"},
};

NAMED_TYPE gGameNames[5] =
{
	{kGameModeSingle,		"Single"},
	{kGameModeCoop,			"Cooperative"},
	{kGametModeDeathmatch,	"BloodBath"},
	{kGameModeFlags,		"Teams"},
	{4,						"All"},
};

// these are for exclusive loop processing supposed to be
// set with xmpSetEditMode() function
//////////////////////////////////////////////////////////
void xmpEditLoopProcess3D(void);
void xmpEditLoopProcess2D(void);
void xmpEditLoopProcessSPLIT(void);
int  xmpEditLoop(void);
EDITLOOPPROCESS_FUNC pGEditLoopProcessFunc = xmpEditLoopProcess3D;
////////////////////////////////////////////////////////////////////

char* onOff(int var)		{ return gBoolNames[(var) ? 1 : 0]; }
char* isNot(int var)		{ return gIsNotNames[(var) ? 1 : 0]; }
char* yesNo(int var)		{ return gYesNoNames[(var) ? 1 : 0]; }
int GetXSector(int nSec)	{ return (sector[nSec].extra > 0) ? sector[nSec].extra : dbInsertXSector(nSec); }
int GetXWall(int nWall)		{ return (wall[nWall].extra > 0)  ? wall[nWall].extra  : dbInsertXWall(nWall);	}
int GetXSprite(int nSpr)	{ return (sprite[nSpr].extra > 0) ? sprite[nSpr].extra : dbInsertXSprite(nSpr); }
void initNames();

void xmpCreateToolTestMap(char withSprite);

const char* GetObjectCaption(int nType, int nID)
{
	switch(nType)
	{
		case OBJ_SECTOR:
			if (!rngok(sector[nID].type, 0, 1024))		return NULL;
			if (!gSectorCaptions[sector[nID].type])		return NULL;
			return gSectorCaptions[sector[nID].type];
		case OBJ_WALL:
			if (!rngok(wall[nID].type, 0, 1024))		return NULL;
			if (!gWallCaptions[wall[nID].type])			return NULL;
			return gWallCaptions[wall[nID].type];
		case OBJ_SPRITE:
			if (!rngok(sprite[nID].type, 0, 1024))		return NULL;
			if (!gSpriteCaptions[sprite[nID].type])		return NULL;
			return gSpriteCaptions[sprite[nID].type];
	}
	
	return NULL;
}

const char *ExtGetSectorCaption(short nSect, char captStyle)
{
	const char *pCapt; char *p = buffer;
	sectortype* pSect = &sector[nSect];
	int i;
	
	p[0] = '\0';
	if (captStyle == kCaptionStyleMapedit && pSect->extra > 0)
	{
		XSECTOR* pXSect = &xsector[pSect->extra];
		
		if (pXSect->rxID > 0)
			p += sprintf(p, "%d: ", pXSect->rxID);
		
		
		if ((pCapt = GetObjectCaption(OBJ_SECTOR, nSect)) == NULL)
			p += sprintf(p, "<<T%d>>", pSect->type);
		else
			p += sprintf(p, pCapt);
		
		
		if (pXSect->txID > 0)
			p += sprintf(p, ": %d", pXSect->txID);
		
		if (pXSect->panVel != 0)
			p += sprintf(p, " PAN(%d,%d)", pXSect->panAngle, pXSect->panVel);
		
		p += sprintf(p, " %s", gBoolNames[pXSect->state]);
		
		if (gPreviewMode)
		{
			for (i = 0; i < gBusyCount; i++)
			{
				if (gBusy[i].at0 != nSect) continue;
				p += sprintf(p, " (%d%%)", (100*klabs(pXSect->busy)) / 65536);
				break;
			}
		}
	}
	else if (pSect->type || pSect->hitag)
	{
		sprintf(p, "{%d:%d}", pSect->hitag, pSect->type);
	}
	
	return buffer;
}

const char *ExtGetWallCaption(short nWall, char captStyle)
{
	const char *pCapt; char *p = buffer;
	walltype* pWall = &wall[nWall];
	
	p[0] = '\0';
	if (captStyle == kCaptionStyleMapedit && pWall->extra > 0)
	{
		XWALL* pXWall = &xwall[pWall->extra];
		
		if (pXWall->rxID > 0)
			p += sprintf(p, "%d: ", pXWall->rxID);
		
		
		if ((pCapt = GetObjectCaption(OBJ_WALL, nWall)) == NULL)
			p += sprintf(p, "<<T%d>>", pWall->type);
		else
			p += sprintf(p, pCapt);
	
		
		if (pXWall->txID > 0)
			p += sprintf(p, ": %d", pXWall->txID);
		
		if (pXWall->panXVel != 0 || pXWall->panYVel != 0)
			p += sprintf(p, " PAN(%i,%i)", pXWall->panXVel, pXWall->panYVel);

		p += sprintf(p, " %s", gBoolNames[pXWall->state]);

	}
	else if (pWall->type || pWall->hitag)
	{
		sprintf(p, "{%i:%i}", pWall->hitag, pWall->type);
	}
	
	return buffer;
}

const char *ExtGetSpriteCaption(short nSprite, char captStyle)
{
	spritetype *pSprite = &sprite[nSprite];
	static char rx[6], name[32], midl[32], tx[6];
	short nType = pSprite->type;
	char *typeName = NULL, *to = buffer;
	int i;

	to[0] = '\0';
	
	if (captStyle == kCaptionStyleBuild)
	{
		if (nType || pSprite->hitag)
			sprintf(to, "{%i:%i}", pSprite->hitag, nType);

		return to;
	}
	
	switch (pSprite->statnum)
	{
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

void ExtPreCheckKeys(void)
{
	if (TestBitString(gotpic, 2342))
		FireProcess();	// animate dynamic fire
	
	DoSectorLighting();	// animate sector lighting
	if (gMisc.pan && totalclock >= gTimers.pan)
	{
		// animate panning sectors
		gTimers.pan = totalclock + 4;
		DoSectorPanning();
	}
	
	if (ED3D && !ED23)
	{
		gfxSetColor(clr2std(kColorGrey29));
		gfxFillBox(windowx1, windowy1, windowx2, windowy2);
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

void process2DMode()
{
	if ((!ED23 || gHudPrefs.dynamicLayoutSPLIT) && gHudPrefs.dynamicLayout2D)
	{
		if (pGLBuild)
		{
			if (gHudPrefs.layout2D == kHudLayoutFull)
			{
				gHudPrefs.layout2D = kHudLayoutCompact;
				hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
			}
		}
		else if (gHudPrefs.layout2D != kHudLayoutFull)
		{
			gHudPrefs.layout2D = kHudLayoutFull;
			hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
		}
	}
}

void processDrawRooms(int camx, int camy, int camz, int cama, int camh, int nSect)
{
	ExtPreCheckKeys();
	drawrooms(camx, camy, camz, cama, camh, nSect);
	ExtAnalyzeSprites();
	drawmasks();
	UndoSectorLighting();
}

void process3DMode()
{
	if (!ED23)
	{
		// quick hack for empty space position in 3d mode...
		if (cursectnum < 0 && numsectors > 0 && !gNoclip)
		{
			scrSetMessage("Sector lost!");
			xmpSetEditMode(0x00);
			BeepFail();
			return;
		}
	}
	
	gHovStat = searchstat;
	gHovSpr = (gHovStat == OBJ_SPRITE) ? searchwall : -1;
	if (gHovStat == OBJ_MASKED)		gHovWall = searchwall;
	else if (gHovStat == OBJ_WALL)	gHovWall = searchwall2;
	else							gHovWall = -1;
}



void xmpSplitModeCountWindowSize(Rect* pOne, Rect* pTwo)
{
	const char kPad = kSplitModeWndPad;
	int hg = gMapedHud.main.y2-gMapedHud.HeightNoLogo();
	int t  = (fullscreen) ? 0 : 1;
	int sz = gSplitMode.size;
	
	if (hg <= 0)
		hg = ydim-1;
	
	if (gSplitMode.vertical)
	{
		sz = perc2val(sz, xdim);
		
		pOne->x0 = 1;
		pOne->y0 = 1;
		pOne->x1 = sz-(kPad>>1);
		pOne->y1 = hg;
		
		pTwo->x0 = sz+(kPad>>1);
		pTwo->y0 = 0;
		pTwo->x1 = xdim-t;
		pTwo->y1 = hg;
	}
	else
	{
		sz = perc2val(sz, hg);
		
		pOne->x0 = 1;
		pOne->y0 = 1;
		pOne->x1 = xdim-t;
		pOne->y1 = sz-(kPad>>1);
		
		pTwo->x0 = 1;
		pTwo->y0 = sz+(kPad>>1);
		pTwo->x1 = xdim-t;
		pTwo->y1 = hg;
	}
}

void xmpSplitModeSet(char nMode)
{
	if (ED2D)
		scrSetGameMode(fullscreen, xdim, ydim, bpp);

	if (!irngok(gSplitMode.size, 4, 96))
	{
		gSplitMode.vertical = widescreen;
		gSplitMode.size		= 50;
	}
	
 	if (gSplitMode.swapSize && gSplitMode.size != 50)
	{
		int t;
		Rect r3D, r2D;
		xmpSplitModeCountWindowSize(&r3D, &r2D);
		t = (r2D.width() < r3D.width() || r2D.height() < r3D.height());
		if ((!nMode && t) || (nMode && !t))
			gSplitMode.size = 100 - gSplitMode.size;
	}
	
	if (gHudPrefs.dynamicLayoutSPLIT)
		hudSetLayout(&gMapedHud, (nMode)
					? gHudPrefs.layout3D : gHudPrefs.layout2D, &gMouse);

	if (nMode)
	{
		ED23 &= ~0x01;
		ED23 |=  0x02;
	}
	else
	{
		ED23 &= ~0x02;
		ED23 |=  0x01;
		
		QSETMODE(xdim, ydim);
	}
}

void xmpSplitModeProcess(Rect* r3D, Rect* r2D)
{
	const int kPad = kSplitModeWndPad;
	int hg = gMapedHud.main.y2-gMapedHud.HeightNoLogo();
	if (hg <= 0)
		hg = ydim-1;
	
	int hxd = xdim>>1, hyd = hg>>1;
	int t, x, y;
	
	MOUSE* pMouse = &gMouse;
	Rect *pRect;
	
	char mLook3D	= (gMouseLook.mode > 0);
	char clipMouse	= (pMouse->buttons || ctrl || alt || shift || mLook3D);
	char rFrameCol	= fade(384);

	if (ED23 & 0x08)
	{
		xmpSplitModeSet(1);
		xmpSplitModeCountWindowSize(r3D, r2D);
		
		searchx = pMouse->X = MIDPOINT(r3D->x0, r3D->x1);
		searchy = pMouse->Y = MIDPOINT(r3D->y0, r3D->y1);
		ED23	&= ~0x08;
	}
	else if (ED23 & 0x10)
	{
		xmpSplitModeSet(0);
		xmpSplitModeCountWindowSize(r3D, r2D);
		
		searchx = pMouse->X = MIDPOINT(r2D->x0, r2D->x1);
		searchy = pMouse->Y = MIDPOINT(r2D->y0, r2D->y1);
		ED23	&= ~0x10;
	}
	else if (ctrl)
	{
		if (keystatus[KEY_SCROLLLOCK])
		{
			gMapedHud.SetMsgImp(128, "Auto-swap size: %s", onOff(gSplitMode.swapSize^=1));
			keystatus[KEY_SCROLLLOCK] = 0;
			Beep(gSplitMode.swapSize);
			keyClear();
		}
		
		if (keystatus[KEY_DOWN]|keystatus[KEY_UP])
		{
			t = (alt) ? 4 : 8;
			if (keystatus[KEY_UP])
				t = -t;
			
			if (gSplitMode.vertical)
			{
				gSplitMode.vertical = 0;
			}
			else
			{
				gSplitMode.size = ClipRange(gSplitMode.size+t, 4, 96);
				if (rngok(gSplitMode.size, 47, 50) || rngok(gSplitMode.size, 51, 53))
					gSplitMode.size = 50;
			}
			
			xmpSplitModeCountWindowSize(r3D, r2D);
			keystatus[KEY_DOWN]		= 0;
			keystatus[KEY_UP]		= 0;
			clipMouse				= 0;
			keyClear();
		}
		else if (keystatus[KEY_LEFT]|keystatus[KEY_RIGHT])
		{
			t = (alt) ? 4 : 8;
			if (keystatus[KEY_LEFT])
				t = -t;
			
			if (!gSplitMode.vertical)
			{
				gSplitMode.vertical = 1;
			}
			else
			{
				gSplitMode.size = ClipRange(gSplitMode.size+t, 4, 96);
				if (rngok(gSplitMode.size, 47, 50) || rngok(gSplitMode.size, 51, 53))
					gSplitMode.size = 50;
			}
			
			xmpSplitModeCountWindowSize(r3D, r2D);
			keystatus[KEY_RIGHT]	= 0;
			keystatus[KEY_LEFT]		= 0;
			clipMouse				= 0;
			keyClear();
		}
	}
	else
	{
		searchx = x = pMouse->X;
		searchy = y = pMouse->Y;
	}
	
	if (clipMouse)
	{
		if (ED3D)
		{
			pMouse->RangeSet(r3D->x0+1, r3D->y0+1, r3D->x1-1, r3D->y1-1);
		}
		else
		{
			pMouse->RangeSet(r2D->x0+1, r2D->y0+1, r2D->x1-1, r2D->y1-1);
		}
	}
	else if (!mLook3D)
	{
		pMouse->RangeSet(1, 1, xdim-1, hg-1);
		
		if (ED3D)
		{
			if (rngok(x, r2D->x0, r2D->x1) && rngok(y, r2D->y0, r2D->y1))
				xmpSplitModeSet(0);
		}
		else
		{
			if (rngok(x, r3D->x0, r3D->x1) && rngok(y, r3D->y0, r3D->y1))
				xmpSplitModeSet(1);
		}
	}
	
	t = kPad+2;
	gfxSetColor(rFrameCol);
	if (gSplitMode.vertical)
	{
		gfxVLine(hxd, r3D->y1-t, r3D->y1);
		gfxVLine(hxd, r3D->y0, r3D->y0+t);
		
		pRect = new Rect(r3D->x1, r3D->y0, r2D->x0+1, r2D->y1);
		gfxFillBox(pRect);
		
		if (ED2D) pRect = new Rect(r2D->x0, hyd-1, r2D->x0+t, hyd+1);
		else pRect = new Rect(r3D->x1-t+1, hyd-1, r3D->x1, hyd+1);
	}
	else
	{
		gfxHLine(hyd, r3D->x1-t, r3D->x1);
		gfxHLine(hyd, r3D->x0, r3D->x0+t);
		
		pRect = new Rect(r3D->x0, r3D->y1, r2D->x1, r2D->y0);
		gfxFillBox(pRect);
		
		if (ED2D) pRect = new Rect(hxd-1, r2D->y0, hxd+1, r2D->y0+t);
		else pRect = new Rect(hxd-1, r3D->y1-t, hxd+1, r3D->y1);
		
	}
	
	gfxFillBox(pRect);
	
	pRect = new Rect(0, 0, xdim-1, hg);
	gfxSetColor(clr2std(kColorBlack));
	gfxRect(pRect);
}

/***********************************************************************
 * ExtCheckKeys()
 *
 * Called just before nextpage in both 2D and 3D modes.
 *
 **********************************************************************/
void ExtCheckKeys( void )
{
	int FPSy = 4, l; QFONT* pFont;
	static char hudLogoChanged = 0, compatStat = 0;
	static OBJECT object;
	char fullLayout;
	
	keyGetHelper(&key, &ctrl, &shift, &alt);
	h = (totalclock & kHClock) ? 8 : 0;
	updateClocks();
	
	if (gMapedHud.draw)
	{
		if (totalclock >= gTimers.hudObjectInfo)
		{
			gTimers.hudObjectInfo	= totalclock + 32;
			object.type				= searchstat;
			object.index			= searchindex; 
		}
		
		fullLayout = (gMapedHud.layout == kHudLayoutFull);
		gMapedHud.DrawIt();
		
		switch (object.type)
		{
			case OBJ_FLOOR:
			case OBJ_CEILING:
				ShowSectorData(object.index, !ctrl, fullLayout);
				break;
			case OBJ_MASKED:
			case OBJ_WALL:
				ShowWallData(object.index,   !ctrl, fullLayout);
				break;
			case OBJ_SPRITE:
				ShowSpriteData(object.index, !ctrl, fullLayout);
				break;
			default:
				gMapedHud.SetTile();
				ShowMapInfo(fullLayout);
				break;
		}
	}

	if (!gPreviewMode)
	{
		if (numwalls != oldnumwalls || numsectors != oldnumsectors || numsprites != oldnumsprites)
		{
			oldnumwalls = numwalls, oldnumsectors = numsectors, oldnumsprites = numsprites;
			asksave = 1;
			CleanUp();
		}
		
		if (irngok(asksave, 1, 2))
		{
			if (!gMouse.buttons)
			{
				l = LENGTH(keystatus);
				while(--l >= 0)
				{
					switch(l)
					{
						case KEY_LSHIFT:
						case KEY_RSHIFT:
						case KEY_LCTRL:
						case KEY_RCTRL:
						case KEY_LALT:
						case KEY_RALT:
							continue;
					}
					
					if (keystatus[l])
						break;
				}
				
				if (l < 0)
				{
					gMapSnapshot.Make(asksave == 1);
					asksave = 3;
				}
			}
		}
	
		
		if (gSound.ambientAlways)
			ambProcess();
		
		hgltShowStatus(4, (totalclock < gScreen.msg[0].time) ? 14 : 4);
		
		if (gAutosave.interval && totalclock > gTimers.autosave)
		{
			if (asksave)
			{
				gMapedHud.SetLogo("AUTOSAVE", NULL, gSysTiles.icoDiskette);
				gTimers.diskPic = totalclock + 128;
				hudLogoChanged = 1;
				boardSaveAuto();
				asksave = 0;
			}
			
			gTimers.autosave = totalclock + gAutosave.interval;
		}
		
		if (gCompat.indicate)
		{
			pFont = qFonts[0];
			COMPAT_STRING* pStr;
			
			if (totalclock > gTimers.compatCheck)
			{
				gTimers.compatCheck = totalclock+256;
				compatStat = (isModernMap() > 0);
			}
			
			pStr = &gCompatString[compatStat];
			l = gfxGetTextLen(pStr->text, pFont);
			gfxDrawText(xdim-l-4, 4, clr2std(pStr->tcol), pStr->text, pFont);
			FPSy+=12;
		}
	}
	else
	{
		previewProcess();
		FPSy+=20;
		
		if (previewProcessKeys() || !previewCheckKey(key))
		{
			keystatus[key] = 0;
			keyClear();
			key = 0;
		}
	}
	
	scrDisplayMessage();
	CalcFrameRate();
	processMove();
    
    getHighlightedObject();
	
	if (ED3D)
	{
		process3DMode();
		ProcessInput3D();
	}
	else
	{
		process2DMode();
		ProcessInput2D();
	}
	
    ProcesInputShared();
    
	if (hudLogoChanged)
	{
		if (totalclock >= gTimers.diskPic)
		{
			gMapedHud.SetLogo("XMAPEDIT", (char*)build_date, gSysTiles.icoXmp);
			hudLogoChanged = 0;
		}
	}
    
	pFont = qFonts[1];
	sprintf(buffer, "%4d FPS", gFrameRate); l = gfxGetTextLen(buffer, pFont);
	gfxPrinTextShadow(xdim-l-4, FPSy, clr2std((gFrameRate<30) ? kColorLightRed : kColorWhite), buffer, pFont);
}

char dlgMapSettings()
{
	const int kPad1 = 6, bw = 80, bh = 24;
	char* skyTilesNames[] = { "Auto", "01", "02", "04", "08", "16", "32" };
	char* skyTypeNames[]  = { "Static", "Stretch", "Wrap" };
	char tmp[256], *p = tmp, *skyTilesRepeat[34];
	
	int dw, dx1, dy1, dx2, dy2, dwh, dhg, i, t;
	int nSkyTilesRepeat = Sky::tileRepeatCount;
	int nSkyType = parallaxtype;
	int nSkyTiles = 0;
	
	if (Sky::customBitsFlag)
	{
		for (i = 1; i < LENGTH(skyTilesNames); i++)
		{
			if (atoi(skyTilesNames[i]) == Sky::pieces)
			{
				nSkyTiles = i;
				break;
			}
		}
	}
	
	t = LENGTH(skyTilesRepeat)-1;
	skyTilesRepeat[0] = p; p += sprintf(p, "Auto");
	for (i = 1; i < t; i++)
	{
		skyTilesRepeat[i] = ++p;
		p += sprintf(p, "%02d", i);
	}
	
	skyTilesRepeat[t] = ++p; sprintf(p, "Never");
	nSkyTilesRepeat = ClipHigh(nSkyTilesRepeat, t);
	
	Window dialog(0, 0, 320, 200, "Map properties");
	dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
	dialog.getSize(&dwh, &dhg);
		
	int enw = pFont->width  << 3;
	int enh = pFont->height << 1;
	
	FieldSet* fBSize 		= new FieldSet(dx1+2, dy1+2, dwh-kPad1, enh<<1, "2D BOARD SIZE", kColorDarkGray, kColorDarkGray);
	int cy = (fBSize->height >> 1) - (enh>>1);
	EditNumber* eW			= new EditNumber(kPad1, cy, enw, enh, PIX2MET(boardWidth), 'm', 10, 10000);
	Label* lX				= new Label(eW->left + eW->width + kPad1, cy + (pFont->height>>1), "X");
	EditNumber* eH			= new EditNumber(lX->left + lX->width + kPad1 + 1, cy, enw, enh, PIX2MET(boardHeight), 'm', 10, 10000);
	Label* lDesc			= new Label(eH->left + eW->width + kPad1, cy + (pFont->height>>1), "1m = 512 pixels", kColorDarkGray);
	
	Panel* pButtons 		= new Panel(dx1, dy2-bh-1, dwh, bh);
	TextButton* bOk			= new TextButton(0, 0, bw, bh, "Confirm", mrOk);
	bOk->fontColor 			= kColorBlue;
	TextButton* bCancel		= new TextButton(bOk->left + bOk->width + 4, 0, bw, bh, "Cancel", mrCancel);
	bCancel->fontColor		= kColorRed;
	
	dy1 += fBSize->height + 16;
	FieldSet* fVis 			= new FieldSet(dx1+2, dy1, dwh-kPad1, enh<<1, "GLOBAL VISIBILITY", kColorDarkGray, kColorDarkGray);
	EditNumber* eVis		= new EditNumber(kPad1, cy, enw, enh, visibility, '\0', 0, kMaxVisibility);
	Checkbox* cFog			= new Checkbox(eVis->left+eVis->width+kPad1, (fVis->height>>1)-(pFont->height>>1)-1, gFogMode, "Enable fog");

	
	dy1 += fVis->height + 16;
	FieldSet* fSky 			= new FieldSet(dx1+2, dy1, dwh-kPad1, enh*3, "PARALLAX SETUP", kColorDarkGray, kColorDarkGray);
	
	Label* lSkyTiles		= new Label(4, 6, "Amount of tiles:");
	TextButton* bSkyTiles	= new TextButton(lSkyTiles->left+lSkyTiles->width+8, 1, 40, 18, skyTilesNames[nSkyTiles], 1000);
	bSkyTiles->font			= qFonts[1];
	bSkyTiles->fontColor	= kColorBlue;
	
	Label* lSkyType			= new Label(4, 6, "Parallax type:");
	TextButton* bSkyType	= new TextButton(lSkyType->left+lSkyType->width+8, 1, 44, 18, skyTypeNames[nSkyType], 1001);
	bSkyType->font			= qFonts[1];
	bSkyType->fontColor		= kColorBlue;
	
	Label* lSkyRepeat		= new Label(4, 6, "Repeat amount:");
	TextButton* bSkyRepeat	= new TextButton(bSkyTiles->left, 1, 40, 18, skyTilesRepeat[nSkyTilesRepeat], 1002);
	bSkyRepeat->font		= qFonts[1];
	bSkyRepeat->fontColor	= kColorBlue;
	
	Panel* pSkyTiles		= new Panel(kPad1, cy, lSkyTiles->width+bSkyTiles->width+14, 20, 0, 0, 0);
	Panel* pSkyType			= new Panel(pSkyTiles->left+pSkyTiles->width+2, cy, lSkyType->width+bSkyType->width+14, 20, 0, 0, 0);
	Panel* pSkyRepeat		= new Panel(kPad1, 28, lSkyRepeat->width+bSkyRepeat->width, 20, 0, 0, 0);
	
	fBSize->Insert(eW);
	fBSize->Insert(lX);
	fBSize->Insert(eH);
	fBSize->Insert(lDesc);
	
	fVis->Insert(eVis);
	fVis->Insert(cFog);
	
	pSkyTiles->Insert(lSkyTiles);
	pSkyTiles->Insert(bSkyTiles);
	fSky->Insert(pSkyTiles);
	
	pSkyType->Insert(lSkyType);
	pSkyType->Insert(bSkyType);
	fSky->Insert(pSkyType);
	
	pSkyRepeat->Insert(lSkyRepeat);
	pSkyRepeat->Insert(bSkyRepeat);
	fSky->Insert(pSkyRepeat);
	
	
	pButtons->Insert(bOk);
	pButtons->Insert(bCancel);
		
	dialog.Insert(fBSize);
	dialog.Insert(fVis);
	dialog.Insert(fSky);
	
	dialog.Insert(pButtons);
	
	while( 1 )
	{
		ShowModal(&dialog);
		
		eW->ClampValue();
		eH->ClampValue();
		eVis->ClampValue();
		
		switch(dialog.endState)
		{
			case mrOk:
				Sky::customBitsFlag		= false;
				boardWidth				= MET2PIX(eW->value);
				boardHeight				= MET2PIX(eH->value);
				visibility				= eVis->value;
				gFogMode				= cFog->checked;
				
				scrLoadPLUs();
				
				if (nSkyTilesRepeat < LENGTH(skyTilesRepeat)-1)
					Sky::tileRepeatCount = nSkyTilesRepeat;
				else
					Sky::tileRepeatCount = MAXPSKYTILES-1;
				
				if (nSkyTiles > 0)
				{
					nSkyTiles = atoi(skyTilesNames[nSkyTiles]);
					for (pskybits = 0; pskybits < 6; pskybits++)
					{
						Sky::pieces = 1 << pskybits;
						if (Sky::pieces == nSkyTiles)
						{
							Sky::customBitsFlag = true;
							break;
						}
					}
				}
				
				if (!Sky::customBitsFlag) // setup from the sky tile size
				{
					Sky::pieces	= 16;
					pskybits	= 4;
				}
				
				i = numsectors;
				while(--i >= 0)
				{
					if (!isSkySector(i, OBJ_CEILING)) continue;
					else if (Sky::GetMostUsed(i, OBJ_CEILING, true, &t) && tileLoadTile(t))
					{
						Sky::SetBits(t, NULL);
						break;
					}
				}
				
				parallaxtype = nSkyType;
				asksave = 1;
				
				return true;
			case 1000:
				if ((i = showButtons(skyTilesNames, LENGTH(skyTilesNames), "Select") - mrUser) >= 0)
				{
					bSkyTiles->text = skyTilesNames[i];
					nSkyTiles = i;
				}
				continue;
			case 1001:
				if ((i = showButtons(skyTypeNames, LENGTH(skyTypeNames), "Select") - mrUser) >= 0)
				{
					bSkyType->text = skyTypeNames[i];
					nSkyType = i;
				}
				continue;
			case 1002:
				if ((i = showButtons(skyTilesRepeat, LENGTH(skyTilesRepeat), "Select") - mrUser) >= 0)
				{
					bSkyRepeat->text = skyTilesRepeat[i];
					nSkyTilesRepeat = i;
				}
				continue;
		}
		
		break;
	}
	
	return false;
}

static int OSDFUNC_ShiftSysStatnum(const osdfuncparm_t *arg)
{
	int nNew, nOld, nVal = 256, i, j;
	int nShifted = 0, nTotal = 0;
	spritetype* pSpr;
	
	if (gMapLoaded)
	{
		switch(arg->numparms)
		{
			case 1:
				nVal = Bstrtol(arg->parms[0], NULL, 10);
				break;
			default:
				return OSDCMD_SHOWHELP;
		}
	}
	else
	{
		buildprintf("No map loaded!\n");
		return OSDCMD_OK;
	}
	
	buildprintf("\n>>>> Statnum shifter utility for \"%s\"\n", gPaths.maps);
	for (i = 0; i < highlightcnt; i++)
	{
		j = highlight[i];
		if ((j & 0xC000) == 0)
			continue;

		pSpr = &sprite[j & 0x1FFF];
		nTotal++;
		
		if (pSpr->statnum == kStatFree)																	;
		else if (pSpr->statnum == kStatExplosion && !rngok(pSpr->type, 0, kExplosionMax))				;
		else if (pSpr->statnum == kStatFX && !rngok(pSpr->type, 0, kFXMax))								;
		else if (pSpr->statnum == kStatItem && !rngok(pSpr->type, kItemWeaponBase, kItemMax))			;
		else if (pSpr->statnum == kStatThing && !rngok(pSpr->type, kThingBase, kThingMax))				;
		else if (pSpr->statnum == kStatProjectile && !rngok(pSpr->type, kMissileBase, kMissileMax))		;
		else if ((pSpr->statnum == kStatDude) && !rngok(pSpr->type, kDudeBase, kDudeMax))				;
		else if (pSpr->statnum == kStatRespawn)															;
		else if (pSpr->statnum == kStatPurge)															;
		else if (pSpr->statnum == kStatTraps && !rngok(pSpr->type, kTrapBase, kTrapMax))				;
		else if (pSpr->statnum == kStatAmbience && pSpr->type != kSoundAmbient)							;
		else if (pSpr->statnum == kStatSpares)															;
		else if (pSpr->statnum == kStatFlare)															;
		else if (pSpr->statnum == kStatDebris)															;
		else if (pSpr->statnum == kStatPathMarker && pSpr->type != kMarkerPath)							;
		else if (pSpr->statnum == kStatSpares)															;
		else if (pSpr->statnum == kStatInactive)														;
		else if (rngok(pSpr->statnum, 17, 128))															;
		else continue;

		nOld = pSpr->statnum;
		nNew = ClipRange(pSpr->statnum + nVal, 0, kMaxStatus - 1);
			
		buildprintf
		(
			"Sprite #%d, Type %d - %s: Shifting statnum from %d to %d...\n",
			pSpr->index,
			pSpr->type,
			(rngok(pSpr->type, 0, LENGTH(gSpriteNames)) && !isempty(gSpriteNames[pSpr->type])) ? gSpriteNames[pSpr->type] : "<UNNAMED>",
			nOld,
			nNew
		);
		
		ChangeSpriteStat(pSpr->index, nNew);
		nShifted++;
	}
	
	Beep(nShifted > 0);
	buildprintf(">>>> %d of %d sprites shifted in total.\n", nShifted, nTotal);
	if (nShifted)
		CleanUp();
	
	if (nShifted > 0)
		asksave = 1;
	
	return OSDCMD_OK;
}


const char* helpfilename	= "xmapedit.chm";
const char helpkey			= KEY_F1;

int ExtInit(int argc, char const * const argv[])
{
	static char myBuildDate[16] = "UNK_DATE";
	char filename[BMAX_PATH] = "\0", *tmp;
	int i;
	
	BOOL iniGenerated = FALSE;
	RESHANDLE hIni = NULL;
	
	if (build_date && build_date[4] == ' ')
	{
		strcpy(myBuildDate, build_date); myBuildDate[4] = '0'; // day with leading zero
		build_date = (const char*)myBuildDate;
	}
	
	OSD_RegisterFunction
	(
		"statnumshift_sys",
		"statnumshift_sys <val>: shift system reserved statnums by given value for sprites in a highlight\n",
		OSDFUNC_ShiftSysStatnum
	);
	
	onquiteventcallback = dlgSaveAndOrQuit;
	HookReplaceFunctions();
	initcrc32table();
	
    char *appdir = Bgetappdir();

	// the OSX app bundle, or on Windows the directory where the EXE was launched
	if (appdir)
	{
		addsearchpath(appdir);
		if ((i = strlen(appdir) - 1) >= 0)
		{
			if (slash(appdir[i])) appdir[i] = 0;
			_chdir(appdir); // we need this so opening files through "Open with..." dialog works in Windows...
			sprintf(buffer, "Using \"%s\" as working directory.\n", appdir);
		}

		free(appdir);
	}
	else
	{
		ThrowError("Failed to get app directory.");
	}

	wm_setapptitle("XMAPEDIT");
	buildsetlogfile(kLogFile);
	buildprintf("XMAPEDIT BUILD %s\n", build_date);
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
			hIni = gGuiRes.Lookup((unsigned int)0, "INI");
			if (hIni)
			{
				buildprintf("Creating \"%s\" with default settings.\n", kCoreIniName);
				MapEditINI = new IniFile((BYTE*)gGuiRes.Load(hIni), hIni->size);
				MapEditINI->PutKeyInt("General", "ConfigVersion", kConfigReqVersion);
				iniGenerated = TRUE;
			}
			else
			{
				ThrowError("Failed to load default settings!\n");
			}
			
			break;
		}
		
		MapEditINI = new IniFile(kCoreIniName);	// read xmapedit settings
		if ((i = MapEditINI->GetKeyInt("General", "ConfigVersion", -1)) == kConfigReqVersion) // check config version
			break;
			
		buildprintf("--> Current config has incorrect version: Given=%d, Required=%d!\n", i, kConfigReqVersion);
		if (unlink(kCoreIniName) == 0)
		{
			delete(MapEditINI); // force creating new file
			continue;
		}
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
	initKeyMapper();
	// ---------------------------------------------------------------------------
	// setup mouse
	gMousePrefs.Init(MapEditINI, 			"Mouse");
	gMouseLook.Init(MapEditINI, 			"MouseLook");
	gMouse.Init(&gMousePrefs);
	// ---------------------------------------------------------------------------
	// read the rest of options
	buildprintf("Loading preferences...\n");
	
	gMisc.Init(MapEditINI,					"General");
	
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
	gPaths.InitResourceART(MapEditINI, "Resources.ART");
	tileInitFromIni();
	tileInitSystemTiles();
	favoriteTileInit();
	extVoxInit();
	if (gMisc.zeroTile2pal && tileLoadTile(0))
		memset((void*)waloff[0], gMisc.zeroTile2pal, tilesizx[0]*tilesizy[0]);
	
	// ---------------------------------------------------------------------------
	// initialize global 2D mode edit screen
	BYTE* pBytes = NULL;
	IniFile* pColors;
	int nSize;
	
	// load user defined colors
	if ((nSize = fileLoadHelper(kScreen2DPrefs, &pBytes)) > 0)
	{
		pColors = new IniFile(pBytes, nSize, INI_SKIPCM|INI_SKIPZR);
		gScreen2D.ColorInit(pColors);
		delete(pColors);
	}
	// load default colors
	else if ((hIni = gGuiRes.Lookup((unsigned int)3, "INI")) != NULL)
	{
		pColors = new IniFile((BYTE*)gGuiRes.Load(hIni), hIni->size);
		gScreen2D.ColorInit(pColors);
		pColors->Save(kScreen2DPrefs);
		delete(pColors);
	}
	else
	{
		ThrowError("Could not load 2D screen color settings!");
	}
	
	gScreen2D.SetView(0, 0, xdim, ydim);
	gScreen2D.prefs.showFeatures	= 1;
	gScreen2D.prefs.showSprites		= 1;
	gScreen2D.prefs.showHighlight	= 1;
	gScreen2D.prefs.showVertex		= 1;
	// ---------------------------------------------------------------------------
	// initialize hud
	gMapedHud.SetView(kHudLayoutFull, gHudPrefs.fontPack);
	gMapedHud.SetLogo("XMAPEDIT", (char*)build_date, gSysTiles.icoXmp);
	hudSetLayout(&gMapedHud, (qsetmode == 200) ? gHudPrefs.layout3D : gHudPrefs.layout2D, &gMouse);
	// ---------------------------------------------------------------------------
	// initialize sound
	buildprintf("Initialising sound system...\n");
	gSound.Init(MapEditINI, "Sound");
	// ---------------------------------------------------------------------------
	// initialize file browser settings
	gDirBroPrefs.Init(MapEditINI, "FileBrowser");
	// ---------------------------------------------------------------------------
	// misc initialization
	buildprintf("Initialising misc stuff...\n");
	defaultspritecstat 	= kSprOrigin;
	showinvisibility 	= TRUE;
	Sky::pieces			= 16;
	visibility 			= 0;
	mapversion			= 7;
	highlightsectorcnt	= -1;
	highlightcnt		= -1;
	
	trigInit(gSysRes);		FireInit();
	hgltReset();			eraseExtra();
	initNames();			dbInit();
	gObjectLock.Init();		gBeep.Init();
	AutoAdjustSpritesInit();
	userItemsInit();
	editInputInit();

	gExtApps.Init(MapEditINI, "ExternalCommands");

	memset(joinsector, -1, sizeof(joinsector));
	gSeqEd.filename 			= gPaths.seqs;
	gTool.objType 				= -1;
	gTool.objIndex 				= -1;
	// ---------------------------------------------------------------------------
	buildprintf("Init completed!\n\n\n");
	#if USE_OPENGL
	if (GL_swapIntervalTest())
		GL_fps2SwapInterval(gScreen.maxFPS);
	#endif
	xmpSetEditMode(gMisc.editMode);
	splashScreen();

	if (gMisc.forceSetup) // should we show options?
		xmpOptions();
		
	if (argc > 1)
	{
		if (!isempty(argv[1]))
			strcpy(filename, argv[1]);
	}
	else if (gMisc.autoLoadMap)
	{
		if (!isempty(gPaths.maps))
			strcpy(filename, gPaths.maps);
	}
	else if ((tmp = browseOpen(filename, getExt(kMap))) != NULL)
	{
		strcpy(filename, tmp);
	}
	
	i = kToolMapEdit;
	if (!isempty(filename))
		i = toolOpenWith(filename);
	
	switch (i)
	{
		case -1:
			Alert("Could not open \"%s\".", filename);
			switch(xmpMenuProcess())
			{
				case mrCancel:
				case mrToolArtEdit:
				case mrToolQavEdit:
				case mrToolSeqEdit:
					xmpQuit();
					break;
			}
			break;
		case kToolMapEdit:
			boardStartNew();
			strcpy(gPaths.maps,
				(!isempty(filename) && boardLoad(filename) == 0) ? filename : kDefaultMapName);
			
			ChangeExtension(gPaths.maps, getExt(kMap));
			gMapLoaded = 1;
			break;
		case kToolArtEdit:
		case kToolSeqEdit:
		case kToolQavEdit:
			xmpCreateToolTestMap(i == kToolSeqEdit);
			gTool.cantest = 1;
			switch (i)
			{
				case kToolSeqEdit:
					strcpy(gSeqEd.filename, filename);
					ChangeExtension(gPaths.images, getExt(kSeq));
					seqeditStart(gSeqEd.filename);
					break;
				case kToolArtEdit:
					strcpy(gPaths.images, filename);
					ChangeExtension(gPaths.images, getExt(kArt));
					artedStart(gPaths.images, TRUE);
					break;
				case kToolQavEdit:
					strcpy(gPaths.qavs, filename);
					ChangeExtension(gPaths.qavs, getExt(kQav));
					gQaved.Start(gPaths.qavs);
					break;
			}
			// no break
		default:
			xmpQuit();
			break;
	}
	
	// since we have our own quit functions, we can start
	// start exclusive loop without letting BUILD to do
	// anything else...
	
	return xmpEditLoop();
}

void ExtUnInit(void)
{
	sndTerm();
	favoriteTileSave();
	
	if (MapEditINI)
	{	
		gMisc.Save(MapEditINI, 			"General");
		gTileView.Save(MapEditINI,		"TileViewer");
		gPluPrefs.Save(MapEditINI,		"PalookupViewer");
		gAutosave.Save(MapEditINI, 		"AutoSave");
		gAutoGrid.Save(MapEditINI, 		"AutoGrid");
		gMousePrefs.Save(MapEditINI, 	"Mouse");
		gMouseLook.Save(MapEditINI, 	"MouseLook");
		gPreview.Save(MapEditINI, 		"PreviewMode");
		gPaths.Save(MapEditINI, 		"Visited");
		gScreen.Save(MapEditINI, 		"Screen");
		gSound.Save(MapEditINI, 		"Sound");
		gHudPrefs.Save(MapEditINI,  	"HUD");
		gCmtPrefs.Save(MapEditINI,  	"Comments");
		gImportPrefs.Save(MapEditINI, 	"ImportWizard");
		gDirBroPrefs.Save(MapEditINI, 	"FileBrowser");
		MapEditINI->Save(kCoreIniName);
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
		if (chlRangeIsFine(pXSprA->txID)) chlID = pXSprA->txID;
		else pXSprA->txID = chlID;
	}
	else if (pXSectA)
	{
		if (chlRangeIsFine(pXSectA->txID)) chlID = pXSectA->txID;
		else pXSectA->txID = chlID;
	}
	else if (pXWallA)
	{
		if (chlRangeIsFine(pXWallA->txID)) chlID = pXWallA->txID;
		else pXWallA->txID = chlID;
	}
	else
	{
		return -1;
	}

	if (pXSprB)  pXSprB->rxID  = chlID;
	if (pXSectB) pXSectB->rxID = chlID;
	if (pXWallB) pXWallB->rxID = chlID;
	
	CXTracker::Track(nTypeA, nIdxA, 1);
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

char parseAppCommandStr(char* str, char* out, char* episodeIni, char* editFile)
{
	enum
	{
		kTokCWD,
		kTokFile,
		kTokGameRes,
		kTokSoundRes,
		kTokGameIni,
	};
	
	static NAMED_TYPE tokens[] =
	{
		{kTokCWD, 			"CWD"},
		{kTokFile, 			"FILE"},
		{kTokGameRes, 		"BLOOD_RFF"},
		{kTokSoundRes, 		"SOUNDS_RFF"},
		{kTokGameIni, 		"EPISODE_INI"},
	};
	
	char tmp[BMAX_PATH]; NAMED_TYPE* pToken;
	char *p, *t, *o, *s, *f;
	int i, l, u;
	
	*out = '\0', o = out, p = str;
	while((s = strchr(p, '%')) != NULL)
	{
		strSubStr(p, NULL, s, o);
		o += (s-p);
		p = s+1;
		
		for (i = 0; i < LENGTH(tokens); i++)
		{
			pToken = &tokens[i];
			t = pToken->name;
			u = l = 0;
			s = p;

			while(*t != '\0' && *s != '\0')
			{
				/** the WHOLE word here must be written in upper XOR lower case!!! **/
				/** -------------------------------------------------------------- **/
				
				if (*t == *s && !isalpha(*t))	; // it's ok
				else if (toupper(*t) == *s)		u++;
				else if (tolower(*t) == *s)		l++;
				else break;
				
				s++, t++;
			}
			
			if ((*t != '\0') || (u && l))
				continue;
			
			f = NULL;
			switch(pToken->id)
			{
				case kTokFile:		f = editFile;						break;
				case kTokGameRes:	f = gPaths.bloodRFF;				break;
				case kTokSoundRes:	f = gPaths.soundRFF;				break;
				case kTokGameIni:	f = episodeIni;						break;
				case kTokCWD:		getcwd(tmp, sizeof(tmp)); f = tmp;	break;
				default:
					continue;
			}
			
			if (f)
			{
				if (u > 0 && irngok(pToken->id, kTokFile, kTokGameIni))
					_fullpath(tmp, f, BMAX_PATH), f = tmp;
					
				o+=sprintf(o, "%s", f);
			}
			
			p+=(t-pToken->name);
			break;
		}
	}
	
	return (o != out);
}

int boardTestExternal(EXTERNAL_APPS* pDb, int* sysErrNo = NULL)
{
	if (sysErrNo)
		*sysErrNo = 0;

	#ifdef _WIN32
		
		char* testfilename = "__test__"; IniFile *pEpisode, *pPreview;
		char *pPath, *key, *val, *p; EXTERNAL_APP* pApp; spritetype* pSpr;
		char aptext[kMaxExtApps][80], tmp[sizeof(EXTERNAL_APP)], boardpath[BMAX_PATH];
		int bpsx[2], bpsy[2], bpsz[2], bang[2], bsec[2], i = 0, nSysStat;
		EXTERNAL_APP* apps = pDb->apps; int numapps = pDb->numapps;
		STRCUTSTYLE cutStyle = {ALG_RIGHT, '.', 5};
		NAMED_TYPE apnames[kMaxExtApps];
		char episodeGenerated = 0;
		int nPrevNode = -1;

		if (system(NULL) == 0)
			return -1;

		// in case user is planning to test the board
		// in vanilla blood, we have to use 8.3 names
		// format, which means we have to create test
		// ini as well
			
		sprintf(tmp, "%s.ini", testfilename);
		if (fileExists(tmp))
			fileDelete(tmp);
		
		p = "Episode1";
		pEpisode = new IniFile(tmp);
		pEpisode->PutKeyString(p, "Title", p);
		pEpisode->PutKeyString(p, "Map1", testfilename);
		pEpisode->PutKeyString(testfilename, "Title", "Test map");
		
		if ((pPreview = gPreview.pEpisode) != NULL)
		{
			getFilename(gPaths.maps, boardpath, 0);

			while(!episodeGenerated)
			{
				sprintf(tmp, "Episode%d", ++i);
				if (!pPreview->SectionExists(tmp))
					break;
				
				while(pPreview->GetNextString(&key, &val, &nPrevNode, tmp))
				{
					if (key && val && stricmp(val, boardpath) == 0)
					{
						val = "Episode1", p = "Title";
						pEpisode->PutKeyString(val, p, pPreview->GetKeyString(tmp, p, tmp));
						pEpisode->PutKeyString(val, "Map1", testfilename);
						
						nPrevNode = -1;
						while(pPreview->GetNextString(&key, &val, &nPrevNode, boardpath))
							pEpisode->PutKeyString(testfilename, key, val);
						
						episodeGenerated = 1;
						break;
					}
				}
			}
		}
		
		pEpisode->Save();
		
		getPath(gPaths.maps, boardpath, 1);
		strcat(boardpath, testfilename);
		strcat(boardpath, ".map");
		if (fileExists(boardpath))
			fileDelete(boardpath);
		
		i = 0;
		if (numapps > 1)
		{
			while(i < numapps)
			{
				pApp = &apps[i];
				p = tmp, p+=sprintf(p, "%s ", pApp->path);
				if (*pApp->cmd == '\0' || !parseAppCommandStr(pApp->cmd, p, pEpisode->filename, boardpath))
					strcpy(p, boardpath);
				
				strCut(tmp, aptext[i], LENGTH(aptext[0]), &cutStyle);
				apnames[i].name = aptext[i];
				apnames[i].id = i;
				i++;
			}
			
			if ((i = showButtons(apnames, numapps, "Command to start...") - mrUser) < 0)
				return -3;
		}
		else if (numapps == 0)
			return -2;
		

		
		pApp = &apps[i];
		if (fileExists(pApp->path))
		{
			p = tmp, p+=sprintf(p, "%s ", "taskkill /f /im");
			getFilename(pApp->path, p, 1);
			system(tmp);
		}
		else
		{
			return -5;
		}
		
		p = tmp, p+=sprintf(p, "start %s ", pApp->path);
		if (*pApp->cmd == '\0' || !parseAppCommandStr(pApp->cmd, p, pEpisode->filename, boardpath))
			strcpy(p, boardpath);

		bpsx[0] = startposx;	bpsy[0] = startposy;
		bpsz[0] = startposz;	bang[0] = startang;
		
		if ((pSpr = findPlayerStart(0)) != NULL)
		{
			bpsx[1] = pSpr->x;			bpsy[1] = pSpr->y;
			bpsz[1] = pSpr->z;			bang[1] = pSpr->ang;
			bsec[1] = pSpr->sectnum;
		}
		
		setStartPos(posx, posy, posz, ang, 1); boardSave(boardpath, 1);
		setStartPos(bpsx[0], bpsy[0], bpsz[0], bang[0], 0);
		showframe();
		
		if (pSpr)
		{
			pSpr->x = bpsx[1];			pSpr->y   = bpsy[1];
			pSpr->z = bpsz[1];			pSpr->ang = bang[1];
			
			if (bsec[1] >= 0)
				ChangeSpriteSect(pSpr->index, bsec[1]);
		}
		
		p = tmp;
		wm_hidewindow();
		if ((nSysStat = system(p)) > 0)
			p = &tmp[6], nSysStat = system(p); // not all windows may have "start"...
		
		if (nSysStat == 0)
		{
			return 1;
		}
		else
		{
			if (sysErrNo)
				*sysErrNo = nSysStat;
		}
		
		return 0;
	#else
		return -4;
	#endif
}

BOOL ProcesInputShared()
{
	INPUTPROC* pInput = &gEditInputShared[key];
	int i, r;
    
	if (pInput->pFunc)
	{
		r = pInput->pFunc(key, ctrl, shift, alt);
		
		if (r & PROC_OK)
		{
			switch(r & PROC_UNDO_MASK)
			{
				case PROC_UNDO_CMP: asksave = 1; break;
				case PROC_UNDO_ADD: asksave = 2; break;
			}
		}
		
		if (r & PROC_BEEP)
			Beep(r & PROC_OK);

        return (r & PROC_OK) != 0;	
	}
	
	return 0;
}

void xmpOptions(void)
{	
	#define AFTERH(a, b) (a->left+a->width+b)
	#define AFTERV(a, b) (a->top+a->height+b)

	int i, j = 2, f = desktopfreq, w, h, t, numfps = 0, curFps = 0;
	char* screenModeNames[] = { "Windowed", "Borderless", "Exclusive" };
	char fpsText[32][8], resText[MAXVALIDMODES][16], gammaText[11][4];
	NAMED_TYPE fpsNames[32], resNames[MAXVALIDMODES], gammaNames[11];
	char cCrypticText[128], curRes[16], *resArt = "Resources.ART", *p;
	IniFile* pConf = MapEditINI;	

	sprintf(curRes, "%dx%d", xdim, ydim);
	
	for (i = 0; i < LENGTH(gammaText); i++)
	{
		sprintf(gammaText[i], "%02d", i);
		gammaNames[i].name	= gammaText[i];
		gammaNames[i].id	= i;
	}
	
	strcpy(fpsText[numfps], "None");
	fpsNames[numfps].name = fpsText[numfps];
	fpsNames[numfps].id = 0;
	numfps++;
	
	#if USE_OPENGL
	if (GL_swapIntervalTest())
	{
		if (f % 2)
			f++;

		curFps = GL_fps2SwapInterval(gScreen.maxFPS);
		
		strcpy(fpsText[numfps], "Freq");
		fpsNames[numfps].name	= fpsText[numfps];
		fpsNames[numfps].id		= 1;
		numfps++;
		
		while((t = f / j) > 50)
		{
			sprintf(fpsText[numfps], "%d", t);
			fpsNames[numfps].name = fpsText[numfps];
			fpsNames[numfps].id = j;
			numfps++;
			j++;
		}
		
		if (curFps >= numfps)
		{
			gScreen.maxFPS = -2;
			GL_fps2SwapInterval(gScreen.maxFPS);
			curFps = 1;
		}
	}
	#endif
	
	
	Window dialog(0, 0, 310, 400, "Options");
	
	// video
	//---------------------
	FieldSet* fVideo		= new FieldSet(8, 8, dialog.client->width-16, 96, "SCREEN", kColorRed, kColorBlack, 8);
	w = fVideo->client->width-16;
	
	FieldSet* fResolution	= new FieldSet(0, 0, perc2val(w, 38), 36, "RESOLUTION", kColorDarkGray, kColorDarkGray, 8);
	FieldSet* fScreenMode	= new FieldSet(AFTERH(fResolution, 4), 0, perc2val(w, 38), 36, "MODE", kColorDarkGray, kColorDarkGray, 8);
	FieldSet* fMaxFps		= new FieldSet(AFTERH(fScreenMode, 4), 0, perc2val(w, 24), 36, "MAX FPS", kColorDarkGray, kColorDarkGray, 8);

	TextButton* bResolution = new TextButton(0, 0, fResolution->width-16, fResolution->height-16, curRes, 100);
	TextButton* bScreenMode = new TextButton(0, 0, fScreenMode->width-16, fScreenMode->height-16, screenModeNames[fullscreen], 101);
	TextButton* bMaxFps		= new TextButton(0, 0, fMaxFps->width-16, fMaxFps->height-16, fpsText[curFps], 102);
	
	w = fResolution->width+fScreenMode->width+4;
	FieldSet* fVideoOther	= new FieldSet(0, AFTERV(fMaxFps, 8), w, 32, "MORE", kColorDarkGray, kColorDarkGray, 8);
	Checkbox* cUseTransluc	= new Checkbox(0, 4, gScreen2D.prefs.useTransluc, "Use translucency in 2D Mode");
	
	FieldSet* fVideoGamma	= new FieldSet(AFTERH(fVideoOther, 4), AFTERV(fMaxFps, 8), fMaxFps->width, 32, "GAMMA", kColorDarkGray, kColorDarkGray, 6);
	TextButton* bGamma		= new TextButton(0, 0, fVideoGamma->client->width-4, 20, gammaText[gGamma], 103);
	
	bResolution->fontColor	= kColorBlue;
	bScreenMode->fontColor	= kColorGreen;
	bMaxFps->fontColor		= kColorMagenta;
	bGamma->fontColor		= kColorCyan;
/* 	if (numfps == 1)
	{
		bMaxFps->disabled		= 1;
		bMaxFps->canFocus		= 0;
		bMaxFps->fontColor		= kColorDarkGray;
	} */
	
	fResolution->Insert(bResolution);
	fScreenMode->Insert(bScreenMode);
	fMaxFps->Insert(bMaxFps);
	
	fVideo->Insert(fResolution);
	fVideo->Insert(fScreenMode);
	fVideo->Insert(fMaxFps);
	
	fVideoOther->Insert(cUseTransluc);
	fVideo->Insert(fVideoOther);
	
	fVideoGamma->Insert(bGamma);
	fVideo->Insert(fVideoGamma);



	// sound
	//---------------------
	w = dialog.client->width-24;
	FieldSet* fAudio		= new FieldSet(8, AFTERV(fVideo,10), perc2val(w, 50), 76, "SOUND", kColorRed, kColorBlack, 8);
	FieldSet* fAsave		= new FieldSet(AFTERH(fAudio, 8), AFTERV(fVideo,10), perc2val(w, 50), 76, "AUTOSAVE", kColorRed, kColorBlack, 8);
	
	FieldSet* fAudioVol		= new FieldSet(0, 0, fAudio->client->width-8, 30, "VOLUME", kColorDarkGray, kColorDarkGray, 8);
	w = fAudioVol->client->width-8;
	
	Label* lSndVol			= new Label(0, 5, "SFX");
	EditNumber* eSndVol		= new EditNumber(AFTERH(lSndVol, 4), 0, 28, 18, IVAL2PERC(FXVolume, 255), '\0', 0, 100);
	Label* lMusVol			= new Label(AFTERH(eSndVol, 6), 5, "MUS");
	EditNumber* eMusVol		= new EditNumber(AFTERH(lMusVol, 4), 0, 28, 18, IVAL2PERC(MusicVolume, 255), '\0', 0, 100);
	Checkbox* cAmbient		= new Checkbox(0, 38, gSound.ambientAlways, "Ambient always");
	Checkbox* cBeeps		= new Checkbox(0, 50, gMisc.beep, "Beeps");
	
	fAudioVol->Insert(lSndVol);
	fAudioVol->Insert(eSndVol);
	
	fAudioVol->Insert(lMusVol);
	fAudioVol->Insert(eMusVol);
	
	fAudio->Insert(fAudioVol);
	fAudio->Insert(cAmbient);
	fAudio->Insert(cBeeps);
	
	
	
	// autosave
	//---------------------
	w = fAsave->client->width-8;
	Label* lAsaveName		= new Label(0, 6, "Name:");
	EditText* eAsaveName	= new EditText(w-(pFont->width*8), 0, pFont->width*8, 20, gAutosave.basename);
	
	Label* lInterval		= new Label(0, 27, "Interval (sec):");
	EditNumber* eInterval	= new EditNumber(w-28, 22, 28, 20, gAutosave.interval/120, '\0', 0, 999);
	
	Label* lMaxSaves		= new Label(0, 49, "Max saves:");
	EditNumber* eMaxSaves	= new EditNumber(w-28, 44, 28, 20, gAutosave.max, '\0', 1, 999);
	
	fAsave->Insert(lAsaveName);
	fAsave->Insert(eAsaveName);
	fAsave->Insert(lInterval);
	fAsave->Insert(eInterval);
	
	fAsave->Insert(lMaxSaves);
	fAsave->Insert(eMaxSaves);
	
	
	
	
	// mouse
	//---------------------
	FieldSet* fMouse		= new FieldSet(8, AFTERV(fAudio, 10), dialog.client->width-16, 58, "MOUSE", kColorRed, kColorBlack, 8);
	w = fMouse->client->width-16;
	
	FieldSet* fMSpeed		= new FieldSet(0, 0, perc2val(w, 38), 36, "SENSITIVITY", kColorDarkGray, kColorDarkGray, 8);
	FieldSet* fMLook		= new FieldSet(AFTERH(fMSpeed, 8), 0, perc2val(w, 62), 36, "MOUSE LOOK", kColorDarkGray, kColorDarkGray, 8);
	
	w = fMSpeed->width-28;
	EditNumber* eMSpeedX	= new EditNumber(0, 0, perc2val(w, 50), 24, gMousePrefs.speedX, '\0', 0, 2048);
	Label* lMSpeed			= new Label(AFTERH(eMSpeedX, 4), 8, "X");
	EditNumber* eMSpeedY	= new EditNumber(AFTERH(lMSpeed, 4), 0, perc2val(w, 50), 24, gMousePrefs.speedY, '\0', 0, 2048);
	
	Label* lMSlope			= new Label(0, 8, "Slope:");
	EditNumber* eMSlope		= new EditNumber(AFTERH(lMSlope, 4), 0, pFont->width*6, 24, gMouseLook.maxSlope, '\0', 0, 300);
	Checkbox* cMSlopeInv	= new Checkbox(AFTERH(eMSlope, 8), 0, gMouseLook.invert, "Invert");
	Checkbox* cMLookStrafe	= new Checkbox(AFTERH(eMSlope, 8), 12, gMouseLook.strafe, "Strafe");
	
	fMSpeed->Insert(eMSpeedX);
	fMSpeed->Insert(lMSpeed);
	fMSpeed->Insert(eMSpeedY);
	
	fMLook->Insert(lMSlope);
	fMLook->Insert(eMSlope);
	fMLook->Insert(cMSlopeInv);
	fMLook->Insert(cMLookStrafe);
	
	fMouse->Insert(fMSpeed);
	fMouse->Insert(fMLook);
	
	
	
	// other
	//---------------------
	FieldSet* fOther		= new FieldSet(8, AFTERV(fMouse, 10), dialog.client->width-16, 74, "OTHER", kColorRed, kColorBlack, 8);

	t = fileExists(gCrypticArt);
	sprintf(cCrypticText, "Enable Cryptic Passage ART (%s)", (t) ? "restart req" : "not found");
	p = pConf->GetKeyString(resArt, "15", NULL);
	
	Checkbox* cCrypticArt	= new Checkbox(0, 0, (t && p && Bstrcasecmp(p, gCrypticArt) == 0), cCrypticText);
	cCrypticArt->disabled	= (t == 0 || gArtEd.asksave); // may corrupt art when have unsaved changes
	cCrypticArt->canFocus	= !cCrypticArt->disabled;
	
	Checkbox* cSetEditPos	= new Checkbox(0, 12, gMisc.forceEditorPos, "Use editor start position for player 1");
	Checkbox* cSaveAsModern	= new Checkbox(0, 24, gCompat.modernMap, "Save all maps as modern compatible", 104);
	Checkbox* cAutoLoadMRU	= new Checkbox(0, 36, gMisc.autoLoadMap, "Auto load previously edited map");
	Checkbox* cAutoSecrets	= new Checkbox(0, 48, gMisc.autoSecrets, "Auto set secrets counter");
	
	
	fOther->Insert(cCrypticArt);
	fOther->Insert(cSetEditPos);
	fOther->Insert(cSaveAsModern);
	fOther->Insert(cAutoLoadMRU);
	fOther->Insert(cAutoSecrets);
	



	// bottom
	//---------------------
	t = dialog.client->height-AFTERV(fOther, 18);
	Panel* pBottom			= new Panel(8, AFTERV(fOther, 12), dialog.client->width-16, t);
	TextButton* bOk			= new TextButton(0, 0, 100, pBottom->height, "&Close", mrOk);
	Checkbox* cForceSetup	= new Checkbox(AFTERH(bOk, 8), AFTERV(bOk, 0)-(pBottom->height>>1)-5, gMisc.forceSetup, "Open at startup");
	
	w = bOk->width+cForceSetup->width+16;
	bOk->left			= dialog.client->left+((dialog.client->width>>1)-(w>>1));
	cForceSetup->left	= AFTERH(bOk, 8);
	bOk->fontColor		= kColorRed;

	pBottom->Insert(bOk);
	pBottom->Insert(cForceSetup);
	
	dialog.Insert(fVideo);
	dialog.Insert(fAudio);
	dialog.Insert(fAsave);
	dialog.Insert(fMouse);
	dialog.Insert(fOther);
	dialog.Insert(pBottom);

	while( 1 )
	{
		splashScreen();
		ShowModal(&dialog);
		
		//if (dialog.endState == mrOk)
		{
			gMisc.forceSetup			= cForceSetup->checked;
			gScreen2D.prefs.useTransluc = cUseTransluc->checked;
			gMisc.autoLoadMap			= cAutoLoadMRU->checked;
			gMisc.autoSecrets			= cAutoSecrets->checked;
			gMisc.forceEditorPos		= cSetEditPos->checked;
			
			if (!cCrypticArt->disabled && cCrypticArt->pressed)
			{
				if (cCrypticArt->checked)
				{
					// just overwrite tiles015 definition
					pConf->PutKeyString(resArt, "15", gCrypticArt);
				}
				else
				{
					// remove tiles015 key only if defined as cryptic
					if ((p = pConf->GetKeyString(resArt, "15")) != NULL && Bstrcasecmp(p, gCrypticArt) == 0)
						pConf->KeyRemove(resArt, "15");
				}
			}
			
			gMouse.velX = gMouse.velDX	= gMousePrefs.speedX = eMSpeedX->value;
			gMouse.velY = gMouse.velDY	= gMousePrefs.speedY = eMSpeedY->value;

			gMouseLook.maxSlope			= eMSlope->value;
			gMouseLook.maxSlopeF		= ClipHigh(gMouseLook.maxSlope, (widescreen) ? 100 : 200);
			gMouseLook.strafe			= cMLookStrafe->checked;
			
			if (cMSlopeInv->checked) gMouseLook.invert |= 0x01;
			else gMouseLook.invert &= ~0x01;
			
			gMisc.beep				= cBeeps->checked;
			gSound.ambientAlways	= cAmbient->checked;
			
			sndSetFXVolume(perc2val(eSndVol->value, 255));
			sndSetMusicVolume(perc2val(eMusVol->value, 255));
			sndKillAllSounds();
			ambKillAll();
			
			getFilename(eAsaveName->string, gAutosave.basename, 0);
			gAutosave.interval	= 120*eInterval->value;
			gAutosave.current 	= ClipHigh(gAutosave.current, eMaxSaves->value);
			gAutosave.max		= eMaxSaves->value;
			
			if (gMapLoaded)
				CleanUp();
		}
		
		bResolution->pressed	=	bMaxFps->pressed	= 0;
		bScreenMode->pressed	=	bOk->pressed		= 0;
		bGamma->pressed									= 0;
		
		switch(dialog.endState)
		{
			case 100:
				while( 1 )
				{
					for (i = 0; i < validmodecnt; i++)
					{
						sprintf(resText[i], "%dx%d", validmode[i].xdim, validmode[i].ydim);
						resNames[i].name = resText[i];
						resNames[i].id = i;
					}
					
					if ((i = showButtons(resNames, validmodecnt, "Select resolution") - mrUser) >= 0)
					{
						w = validmode[i].xdim;
						h = validmode[i].ydim;
						if (w == xdim && h == ydim)
							break;
						
						toggleResolution(fullscreen, w, h);
						splashScreen();
						
						if (w != xdim || h != ydim)
							Alert("Failed to set resolution: %dx%d! Using nearest mode (%dx%d)...", w, h, xdim, ydim);
						
						sprintf(curRes, "%dx%d", xdim, ydim);
						xdimgame = xdim2d = xdim;
						ydimgame = ydim2d = ydim;
					}
					break;
				}
				continue;
			case 101:
				if ((i = showButtons(screenModeNames, LENGTH(screenModeNames), "Screen mode") - mrUser) >= 0)
				{
					toggleResolution(i, xdimgame, ydimgame);
					bScreenMode->text = screenModeNames[i];
				}
				continue;
			case 102:
				if (numfps > 1)
				{
					#if USE_OPENGL
					if (!nogl && GL_swapIntervalTest())
					{
						if ((i = showButtons(fpsNames, numfps, "Max FPS") - mrUser) >= 0)
						{
							bMaxFps->text = fpsNames[i].name;
							
							switch(i)
							{
								case 0:
								case 1:
									gScreen.maxFPS = -(1+i);
									GL_fps2SwapInterval(gScreen.maxFPS);
									break;
								default:
									gScreen.maxFPS = GL_swapInterval2Fps(i);
									GL_fps2SwapInterval(gScreen.maxFPS);
									break;
							}
						}
						
						continue;
					}
					#endif
				}
				
				Alert
				(
					"FPS limit available only with OpenGL.\n"
					"Make sure \"WGL_EXT_swap_control\" extension\n"
					"supported by your device."
				);
				
				continue;
			case 103:
				if ((i = showButtons(gammaNames, LENGTH(gammaNames), "Gamma") - mrUser) >= 0)
				{
					bGamma->text = gammaNames[i].name;
					gGamma = gammaNames[i].id;
					scrSetPalette(gMisc.palette);
				}
				continue;
			case 104:
				if
				(
					cSaveAsModern->checked &&
					Confirm
					(
						"Modern compatible maps still playable in vanilla, hovewer will operate incorrectly!\n"
						"Use this option only when maps supposed to have modern features\n"
						"which is supported by such source ports as NBlood.\n\n"
						"Enable this feature now?"
					)
				)
				{
					gCompat.modernMap = 1;
					gMisc.showTypes = MO;
				}
				else
				{
					cSaveAsModern->checked = 0;
					gCompat.modernMap = 0;
					if (gMisc.showTypes == MO)
						gMisc.showTypes = VA;
				}
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

		{"The XMAPEDIT", 1, 0, 4, 0},
		{"{date}", 2, 0, 4, 0},
		{"www.cruo.bloodgame.ru/xmapedit", 1, 0, 4, 0},
		{"\0", 0, 0, 0},

		{"Created by", 1, 0, 0, 0},
		{"nuke.YKT", 1, 0, 1, 5},
		{"NoOne", 1, 0, 1, 5},
		{"\0", 0, 0, 0},

		{"Tested by", 1, 0, 0, 0},
		{"Spill,  Seizhak,  daMann,  BloodyTom", 1, 0, 1, 1},
		{"Tekedon,  Nyyss0nen", 1, 0, 1, 1},
		{"and others", 1, 0, 1, 1},
		{"\0", 0, 0, 0},
		
		{"Manual written by", 1, 0, 0, 0},
		{"Seizhak", 1, 0, 5, 5},
		{"\0", 0, 0, 0},
		
		{"Original MAPEDIT version by", 1, 0, 0, 0},
		{"Peter Freese", 1, 0, 5, 5},
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
				sprintf(buffer, "Build [%s]", build_date);
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
	const int bh = 20; const int pd = 4;
	int x = 4, y = 4, i, j;
	
	static char asavename[6][BMAX_PATH];
	static char editors[3][14];
	
	strcpy(editors[0], getExt(kQav));
	strcpy(editors[1], getExt(kSeq));
	strcpy(editors[2], getExt(kArt));
	for (i = 0; i < LENGTH(editors); i++)
		strupr(editors[i]);

	Window dialog(0, 0, 138, ydim, name);
	TextButton* pSaveBoard = new TextButton(4, y, 84,  bh, "&Save board", mrSave);
	TextButton* pSaveBoardAs = new TextButton(88, y, 40,  bh, "&As...", mrSaveAs); y+=bh;
	TextButton* pReloadBoard = new TextButton(4, y, 40,  bh, "&Re", mrReload);
	TextButton* pLoadBoard = new TextButton(44, y, 84,   bh, "&Load board", mrLoad); y+=bh;
	TextButton* pTestBoard = new TextButton(4, y, 124,   bh, "&Test board", mrTest); y+=bh;
	
	if (!fileExists(gPaths.maps, &hFile))
	{
		pReloadBoard->fontColor = kColorDarkGray;
		pReloadBoard->disabled  = TRUE;
		pReloadBoard->canFocus  = FALSE;
	}

	if (!gMapLoaded)
	{
		pSaveBoard->fontColor = pSaveBoardAs->fontColor = pReloadBoard->fontColor = pTestBoard->fontColor = kColorDarkGray;
		pSaveBoard->disabled  = pSaveBoardAs->disabled  = pReloadBoard->disabled  = pTestBoard->disabled = TRUE;
		pSaveBoard->canFocus  = pSaveBoardAs->canFocus  = pReloadBoard->canFocus  =  pTestBoard->canFocus = FALSE;
	}

	dialog.Insert(pSaveBoard);
	dialog.Insert(pSaveBoardAs);
	dialog.Insert(pReloadBoard);
	
	
	dialog.Insert(pLoadBoard);
	dialog.Insert(pTestBoard);
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&Import board", mrToolImportWizard));		y+=bh;
	TextButton* pBoardOpts = new TextButton(4, y, 124, bh, "Setup &Board", mrBoardOptions); 	y+=bh;
	if (!gMapLoaded)
	{
		pBoardOpts->fontColor 	= kColorDarkGray;
		pBoardOpts->disabled 	= TRUE;
		pBoardOpts->canFocus 	= FALSE;
	}
	dialog.Insert(pBoardOpts);
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&New board", mrNew));						y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&Quit editor", mrQuit));						y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "&Options", mrOptions));						y+=bh;
	dialog.Insert(new TextButton(4,  y, 124,  bh, "Abo&ut", mrAbout));							y+=bh;

	y+=(pd<<1);
	if (gAutosave.max > 0 && gAutosave.interval > 0)
	{
		dialog.Insert(new Label(8, y, ">AUTOSAVES", kColorBlue)); y+=(pFont->height+pd);
		
		for (i = ClipHigh(gAutosave.max, 4); i > 0; i--)
		{
			sprintf(asavename[i], "%s%d", gAutosave.basename, i);
			getPath(gPaths.maps, buffer, 1);
			
			strcat(buffer, asavename[i]);
			strcat(buffer, ".");
			strcat(buffer, gExtNames[kMap]);
			
			if (fileExists(buffer))
			{
				TextButton *pbButton = new TextButton(x, y, 124, bh, asavename[i], mrAsave + i);
				dialog.Insert(pbButton);
				y+=bh;
			}
		}

		y+=(pd<<1);
	}

	dialog.Insert(new Label(8, y, ">TOOLS", kColorBlue)); y+=(pFont->height+pd);
	TextButton* pPreviewMode   = new TextButton(x, y, 124,  bh, "&Preview mode", mrToolPreviewMode);		y+=bh;
	TextButton* pSpriteText    = new TextButton(x, y, 124,  bh, "Sprite Te&xt", mrToolSpriteText);			y+=bh;
	TextButton* pExplodeSeq    = new TextButton(x, y, 124,  bh, "&Exploder sequence", mrToolExpSeq);		y+=bh;
	TextButton* pCleanChannel  = new TextButton(x, y, 124,  bh, "&Channel cleaner", mrToolCleanChannel);	y+=bh;
	
	i = 124/3;
	TextButton* pButtonQavedit = new TextButton(x, y, i,  bh, editors[0], mrToolQavEdit); 	x+=i;
	TextButton* pButtonSeqedit = new TextButton(x, y, i,  bh, editors[1], mrToolSeqEdit); 	x+=i;
	TextButton* pButtonArtedit = new TextButton(x, y, i,  bh, editors[2], mrToolArtEdit); 	x=4;
	y+=bh;
	
	if (!gMapLoaded)
	{
		pCleanChannel->fontColor = pPreviewMode->fontColor = pExplodeSeq->fontColor = kColorDarkGray;
		pCleanChannel->disabled  = pPreviewMode->disabled  = pExplodeSeq->disabled = TRUE;
		pCleanChannel->canFocus  = pPreviewMode->canFocus  = pExplodeSeq->canFocus = FALSE;
	}
	
	if (!gMapLoaded || qsetmode != 200)
	{
		pSpriteText->fontColor 	= kColorDarkGray;
		pSpriteText->disabled 	= TRUE;
		pSpriteText->canFocus 	= FALSE;
	}
	
	dialog.height = y + (pd*6);
	dialog.Insert(pPreviewMode);
	dialog.Insert(pSpriteText);
	dialog.Insert(pExplodeSeq);
	dialog.Insert(pCleanChannel);
	dialog.Insert(pButtonQavedit);
	dialog.Insert(pButtonSeqedit);
	dialog.Insert(pButtonArtedit);

	ShowModal(&dialog);
	return dialog.endState;

}

NAMED_TYPE gTestBoardErrors[] =
{
	{-1, "Command processor not found"},
	{-2, "No external commands to process"},
	{-4, "Unavailable on this platform"},
	{-5, "Selected external app not found"},
	{-999, NULL},
};

int xmpMenuProcess() {

	char* tmp = NULL;
	int result = mrMenu, len = 0, i = 0, t;
	RESHANDLE hRes = NULL;

	while( 1 )
	{
		switch (result) {
			default:
				if (result >= mrAsave && result < mrAsaveMax)
				{
					getPath(gPaths.maps, buffer, 1); len = strlen(buffer);
					sprintf(&buffer[len],"%s%d.%s", gAutosave.basename, result - mrAsave, gExtNames[kMap]);
					strcpy(gPaths.maps, buffer);
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
			case mrTest:
				if ((i = boardTestExternal(&gExtApps, &t)) <= 0)
				{
					sprintf(buffer, "External test error");
					
					switch( i )
					{
						case 0:
							Alert("%s: Unable to process this command (errorlev %d)", buffer, t);
						case -3:
							break;
						default:
							if ((tmp = retnCodeCheck(i, gTestBoardErrors)) == NULL) break;
							Alert("%s #%d: %s", buffer, klabs(i), tmp);
							break;
					}

					result = mrMenu;
					break;
				}
				return result;
			case mrToolPreviewMode:
				if (!previewMenuProcess())
				{
					result = mrMenu;
					break;
				}
				else if (gPreviewMode) previewStop();
				previewStart();
				return result;
			case mrBoardOptions:
				if (dlgMapSettings()) return result;
				result = mrMenu;
				break;
			case mrToolSpriteText:
				if (dlgSpriteText()) return result;
				result = mrMenu;
				break;
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
						asksave = 1;
					}
					else
					{
						Alert("No unlinked channels found.");
					}
					
					return result;
				}
				result = mrMenu;
				break;
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
				if ((gMapLoaded && !Confirm("Start new board now?")) || !dlgMapSettings())
				{
					result = mrMenu;
					break;
				}
				
				boardStartNew();
				strcpy(gPaths.maps, kDefaultMapName);
				scrSetMessage("New board started.");
				return result;
			case mrSave:
			case mrSaveAs:
				if ((len = strlen(gPaths.maps)) <= 0) strcpy(gPaths.maps, kDefaultMapName), result = mrSaveAs;
				switch (result) {
					case mrSave:

						// erase user's secrets counter and set editor's.
						if (gMisc.autoSecrets) setupSecrets();
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
						if ((tmp = browseSave(gPaths.maps, ".map")) != NULL)
						{
							sprintf(gPaths.maps, tmp);
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
				if (result != mrLoad) tmp = gPaths.maps;
				else
				{
					if ((tmp = browseOpen(gPaths.maps, ".map", "Load board")) == NULL)
					{
						result = mrMenu;
						break;
					}

					sprintf(gPaths.maps, tmp);
				}

				if (!fileExists(tmp, &hRes))
				{
					Alert("The board file \"%s\" not found!", tmp);
					result = mrMenu;
					break;
				}
				
				if (boardLoad(tmp) == -2) break;
				return result;
			case mrQuit:	
				result = mrMenu;
				dlgSaveAndOrQuit();
				break;
		}
	}
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
	
	#define MOVEUP	keystatus[KEY_A]
	#define MOVEDN	keystatus[KEY_Z]
	#define MOVERT	keystatus[KEY_RIGHT]
	#define MOVELT	keystatus[KEY_LEFT]
	#define MOVEFW	keystatus[KEY_UP]
	#define MOVEBK	keystatus[KEY_DOWN]
	
	BOOL block = FALSE; BOOL in2d = ED2D;
	BOOL strafe = (!in2d && gMouseLook.strafe && gMouseLook.mode);
	int goalz, hiz = 0x80000000, loz = 0x7FFFFFFF, hihit = 0, lohit = 0, nSector;
	int i, hit, px = posx, py = posy, pz = posz, xvect = 0, yvect = 0;
	
	int nTicks = gFrameTicks;
	if (nTicks == gFrameClock)
		nTicks = 0;
	
	if (!in2d && (gMouse.hold & 2) && gMouse.wheel)
	{
		if (zmode == 0)
			zmode = 3;
		
		if (gMouse.wheel < 0)		posz = posz - (kVelStep1<<6) * nTicks;
		else if (gMouse.wheel > 0)	posz = posz + (kVelStep1<<6) * nTicks;
	}
	else if (MOVEFW)	vel = min(vel + kVelStep1 * nTicks, 127);
	else if (MOVEBK)	vel = max(vel - kVelStep1 * nTicks, -128);
	
	
	if (MOVELT)
	{
		if (strafe)
		{
			svel   = min(svel   + kVelStep3 * nTicks, 127);
			angvel = 0;
		}
		else
			angvel = max(angvel - kVelStep3 * nTicks, -128);
	}
	else if (MOVERT)
	{
		if (strafe)
		{
			svel 	= max(svel   - kVelStep3 * nTicks, -128);
			angvel  = 0;
		}
		else
			angvel = min(angvel + kVelStep3 * nTicks, 127);
	}
	
	
	if (angvel)
	{
		i = 14;
		if (shift)
			i = 10;
		
		ang = (short)((ang + (angvel * nTicks) / i) & kAngMask);
		
		if (angvel > 0)
			angvel = max(angvel - kVelStep4 * nTicks, 0);
		else
			angvel = min(angvel + kVelStep4 * nTicks, 0);
	}

	if (vel || svel)
	{
		doubvel = nTicks;
		if (shift)
			doubvel += nTicks;
		
		doubvel += perc2val(doubvel, 40);
		
		if (vel)
		{
			xvect += mulscale30(vel * doubvel >> 2, Cos(ang));
			yvect += mulscale30(vel * doubvel >> 2, Sin(ang));
			
			if (vel > 0) 
				vel = max(vel - kVelStep2 * nTicks, 0);
			else
				vel = min(vel + kVelStep2 * nTicks, 0);
		}
		
		if (svel)
		{
			xvect += mulscale30(svel * doubvel >> 2, Sin(ang));
			yvect -= mulscale30(svel * doubvel >> 2, Cos(ang));
			
			if (svel > 0) 
				svel = max(svel - kVelStep1 * nTicks, 0);
			else
				svel = min(svel + kVelStep1 * nTicks, 0);
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
				i = (hit & HITID);
				switch (hit & HITMASK)
				{
					case HITWALL:
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
							
							if (irngok(wall[i].type, kWallStack-1, kWallStack))
							{
								if (CheckLinkCamera(&posx, &posy, &posz, &i, 1))
									cursectnum = i;
							}
						}
						break;
					case HITSPR:
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
			if (CMP_HITSECT(hihit, cursectnum))
			{
				if (sector[cursectnum].ceilingstat & kSectParallax)
					hiz = -klabs(getceilzofslope(cursectnum, posx, posy) << 2);
			}
			
			if (gPreviewMode && gModernMap)
			{
				if (CHK_HITSPR(hihit))
				{
					i = GET_HITID(hihit);
					if (sprite[i].extra > 0)
					{
						XSPRITE* pXSpr = &xsprite[sprite[i].extra];
						if (pXSpr->triggerTouch)
							trTriggerSprite(i, pXSpr, kCmdSpriteTouch);
					}
				}
				
				if (CHK_HITSPR(lohit))
				{
					i = GET_HITID(hihit);
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

		if (!in2d && !alt && !((ctrl && shift) | (ctrl && !shift)))
		{
			if (MOVEUP)
			{
				goalz -= kMoveVal3;
				if (shift)
					goalz-=kMoveVal4;
			}
			
			if (MOVEDN)
			{
				goalz+=kMoveVal5;
				if (shift)
					goalz+=kMoveVal5;
			}
		}

		if (goalz != posz)
		{
			if (posz < goalz) hvel += 64 * nTicks >> 1;
			if (posz > goalz) hvel = ((goalz-posz)>>3);
				
			posz += hvel * nTicks >> 1;
			if (posz > loz-kMoveVal1) posz = loz-kMoveVal1, hvel = 0;
			if (posz < hiz+kMoveVal1) posz = hiz+kMoveVal1, hvel = 0;
		}
	}
	else
	{
		goalz = posz;
		if (!in2d && !alt && !((ctrl && shift) | (ctrl && !shift)))
		{
			if (MOVEUP)
			{
				if (zmode != 1) goalz-=kMoveVal3;
				else zlock+=kMoveVal1;
			}
			
			if (MOVEDN)
			{
				if (zmode != 1) goalz+=kMoveVal3;
				else if (zlock > 0)	 zlock-=kMoveVal1;
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
			if (posz < goalz) hvel += (128 * nTicks) >> 1;
			if (posz > goalz) hvel -= (128 * nTicks) >> 1;
			
			posz += (hvel * nTicks) >> 1;
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
			
			if ((zmode || gPreviewMode) || (!zmode && (MOVEUP|MOVEDN) && !ctrl && !shift))
			{
				if ((nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect, 0)) == 0)
				{
					legs = posz - val;
					if ((zmode == 0 && MOVEUP) || zmode != 0)
						nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect, 0);
				}
			}

			if (nLink > 0)
			{
				scrSetMessage("Moving through stack!");
				
				switch (nLink)
				{
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
	
	switch (key)
	{
		case KEY_CAPSLOCK:
			if (alt)
			{
				gNoclip = !gNoclip;
				scrSetMessage("Noclip %s", onOff(gNoclip));
				if (Beep(gNoclip))
					zmode = (gMouseLook.mode) ? 3 : 2;
			}
			else if (!in2d && !shift)
			{
				while( 1 )
				{
					switch(zmode = IncRotate(zmode, 4))
					{
						case 0:
							if (gNoclip) continue;
							break;
						case 1:
							if (!gMisc.zlockAvail) continue;
							else zlock = (loz-posz) & ~0x3FF;
							break;
						case 3:
							if (!gMouseLook.mode) continue;
							break;
					}
					break;
				}
				
				scrSetMessage("ZMode = %s", gZModeNames[zmode], onOff(gNoclip));
			}
			break;
	}
}

int getClosestSector(int x, int y)
{
	int nDist = 0x80000000, nSect = -1;
	int i, d, s, e, x1, y1;
		
	i = numsectors;
	while(--i >= 0)
	{
		getSectorWalls(i, &s, &e);
		while(s <= e)
		{
			getWallCoords(s, &x1, &y1);
			if ((d = approxDist(x1-x, y1-y)) < nDist)
				nDist = d, nSect = i;
			
			s++;
		}
	}
	
	return nSect;
}



void setStartPos(int x, int y, int z, int ang, char forceEditorPos)
{
	spritetype* pSpr; int nSect;
	updatesector(x, y, &startsectnum);
	
	if ((nSect = startsectnum) >= 0 && FindSector(x, y, z, &nSect)) startsectnum = nSect;
	else if (startsectnum < 0 && (startsectnum = getClosestSector(x, y)) >= 0)
		avePointSector(startsectnum, &x, &y);
	
	z = (startsectnum >= 0) ? getflorzofslope(startsectnum, x, y) : 0;
	
	startposx = x;
	startposy = y;
	startposz = z;
	startang  = ang;
	
	if (forceEditorPos && (pSpr = findPlayerStart(0)) != NULL)
	{
		pSpr->ang	= startang;
		pSpr->x 	= startposx;
		pSpr->y 	= startposy;
		pSpr->z 	= startposz;
		
		if (startsectnum >= 0)
			ChangeSpriteSect(pSpr->index, startsectnum);
		
		clampSprite(pSpr);
	}
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
	VOIDLIST *bspr, *bxspr, *bwal, *bxwal, *bsec, *bxsec;
	BYTE* pData = NULL; IOBuffer* pIo; CHECKMAPINFO info;
	spritetype* pLSpr, *pLMrk; XSPRITE* pLXSpr;
	sectortype* pLSect; XSECTOR* pLXSect;
	walltype* pLWall; XWALL* pLXWall;
	
	char mapname[_MAX_PATH];
	char fog = gFogMode;

	int  nSect, nSize, nXObj, nSpr;
	int i, j, s, e;

	keyClear();
	strcpy(mapname, filename); ChangeExtension(mapname, getExt(kMap));
	if ((nSize = fileLoadHelper(mapname, &pData)) <= 0)
		return -1;
	
	pIo = new IOBuffer(nSize, pData);
	dbCheckMap(pIo, &info, gSuppMapVersions);
	
	if (info.type == kMapTypeUNK)
	{
		Alert("Unknown map file format!");
		free(pData);
		return -1;
	}
	else if (info.type == kMapTypeBUILD)
	{
		IMPORT_WIZARD_MAP_ARG mapArg;
		mapArg.filepath = mapname;			mapArg.pIo = pIo;
		mapArg.version  = info.version;		mapArg.blood = 0;
		mapArg.allowSel = 0;
		
		IMPORT_WIZARD wizard;
		i = wizard.ShowDialog(&mapArg);
		free(pData);
		return i;
	}
	
	sprintf(buffer3, "Loading the %s", mapname);
	splashScreen(buffer3);
	
	
	if (highlightsectorcnt > 0)
	{
		cpyObjectClear();

		// save all the highlighted stuff before loading other map
		bspr = new VOIDLIST(sizeof(spritetype)); 	bxspr = new VOIDLIST(sizeof(XSPRITE));
		bwal = new VOIDLIST(sizeof(walltype));		bxwal = new VOIDLIST(sizeof(XWALL));
		bsec = new VOIDLIST(sizeof(sectortype));	bxsec = new VOIDLIST(sizeof(XSECTOR));
		
		for(i = 0; i < highlightsectorcnt; i++)
		{
			nSect = highlightsector[i];
			
			// sectors
			bsec->Add(&sector[nSect]);
			if (sector[nSect].extra > 0)
			{
				XSECTOR* pXSect = &xsector[sector[nSect].extra];
				
				// markers that sector own can be outside highlight so we have to
				// add it by just changing sectnum since it doesn't really
				// matter for markers
				if (pXSect->marker0 >= 0) ChangeSpriteSect(pXSect->marker0, nSect);
				if (pXSect->marker1 >= 0) ChangeSpriteSect(pXSect->marker1, nSect);

				bxsec->Add(pXSect);
			}
			
			// walls
			getSectorWalls(nSect, &s, &e);
			while(s <= e)
			{
				bwal->Add(&wall[s]);
				if (wall[s].extra > 0)
					bxwal->Add(&xwall[wall[s].extra]);
				
				s++;
			}

			// sprites of sectors
			for (s = headspritesect[nSect]; s >= 0; s = nextspritesect[s])
			{
				bspr->Add(&sprite[s]);
				if (sprite[s].extra > 0)
					bxspr->Add(&xsprite[sprite[s].extra]);
			}
		}
		
		getFilename(mapname, buffer3);
		switch (YesNoCancel("Import %d sectors with %d walls and %d sprites in \"%s\" map?", bsec->Length(), bwal->Length(), bspr->Length(), buffer3))
		{
			case mrCancel:
				return -1;
			case mrNo:
				highlightsectorcnt = -1;
				break;
			default:
				splashScreen("Importing objects...");
				break;
		}
	}

	boardReset(kHgltPoint | kHgltGradient);
	
	switch(info.type)
	{
		case kMapTypeBLOOD:
			dbLoadMap(pIo, (gCmtPrefs.enabled) ? mapname : NULL);
			break;
		case kMapTypeSAVEGAME_NBLOOD:
			dbLoadSaveGame(pIo);
			break;
	}
	
	free(pData);
	
	if (highlightsectorcnt > 0)
	{
		// restore highlight stuff after
		// loading another map
		
		pLSect  = (sectortype*)bsec->First();
		pLXSect = (XSECTOR*)bxsec->First();

		pLWall  = (walltype*)bwal->First();
		pLXWall = (XWALL*)bxwal->First();
		
		pLSpr  = (spritetype*)bspr->First();
		pLXSpr = (XSPRITE*)bxspr->First();

		// just copy all of it with same order that it was added
		for (i = 0; i < highlightsectorcnt; i++, numsectors++, pLSect++)
		{
			if ((numsectors >= kMaxSectors)
				|| (numwalls+pLSect->wallnum) >= kMaxWalls)
						break;
			
			nSect = highlightsector[i], highlightsector[i] = numsectors;
			memcpy(&sector[numsectors], pLSect, sizeof(sectortype));
			sector[numsectors].wallptr = numwalls;
			
			if (pLSect->extra > 0)
			{
				if (numxsectors < kMaxXSectors)
				{
					// copy & fix xsector
					nXObj = dbInsertXSector(numsectors);
					memcpy(&xsector[nXObj], pLXSect, sizeof(XSECTOR));
					xsector[nXObj].reference = numsectors;			
					
					if (pLXSect->marker0 >= 0 || pLXSect->marker1 >= 0)
					{
						// find & fix it's markers
						pLMrk = (spritetype*)bspr->First();
						for (j = 0; j < bspr->Length() && numsprites < kMaxSprites; j++, pLMrk++)
						{
							if (pLMrk->statnum == kStatMarker && pLMrk->owner == nSect)
							{
								nSpr = InsertSprite(numsectors, kStatMarker);
								memcpy(&sprite[nSpr], pLMrk, sizeof(spritetype));
								sprite[nSpr].sectnum = sprite[nSpr].owner = numsectors;
								sprite[nSpr].index = nSpr;
								
								if (pLXSect->marker0 == pLMrk->index)
								{
									xsector[nXObj].marker0 = nSpr;	
								}
								else
								{
									xsector[nXObj].marker1 = nSpr;
								}
							}
						}
					}
				}
				else
				{
					// erase xsector
					sector[numsectors].extra	= -1;
					sector[numsectors].type		=  0;
				}
				
				pLXSect++;
			}
			
			// copy walls
			for (j = 0; j < pLSect->wallnum; j++, numwalls++, pLWall++)
			{
				memcpy(&wall[numwalls], pLWall, sizeof(walltype));
				wall[numwalls].point2 += (numwalls-j-pLSect->wallptr);
				wall[numwalls].nextsector = wall[numwalls].nextwall = -1;
				
				if (pLWall->extra > 0)
				{
					if (numxwalls < kMaxXWalls)
					{
						// copy & fix xwall
						nXObj = dbInsertXWall(numwalls);
						memcpy(&xwall[nXObj], pLXWall, sizeof(XWALL));
						xwall[nXObj].reference = numwalls;
					}
					else
					{
						// erase xwall
						wall[numwalls].extra = -1;
						wall[numwalls].type  =  0;
					}
					
					pLXWall++;
				}
			}
			
			// copy sprites
			while(numsprites < kMaxSprites && pLSpr->sectnum == nSect)
			{
				if (pLSpr->statnum != kStatMarker)
				{
					nSpr = InsertSprite(numsectors, pLSpr->statnum);
					memcpy(&sprite[nSpr], pLSpr, sizeof(spritetype));
					sprite[nSpr].sectnum = numsectors;
					sprite[nSpr].index = nSpr;
					
					if (pLSpr->extra > 0)
					{
						if (numxsprites < kMaxXSprites)
						{
							// copy & fix xsprite
							nXObj = dbInsertXSprite(nSpr);
							memcpy(&xsprite[nXObj], pLXSpr, sizeof(XSPRITE));
							xsprite[nXObj].reference = nSpr;
						}
						else
						{
							// erase xsprite
							sprite[nSpr].extra = -1;
							sprite[nSpr].type  =  0;
						}
						
						pLXSpr++;
					}
				}
				
				pLSpr++;
			}
		}
		
		highlightsectorcnt = i;
		for (i = 0; i < highlightsectorcnt; i++)
			sectAttach(highlightsector[i]);
	}
	
	if (fog != gFogMode)
		scrLoadPLUs();
	
	if (gModernMap && gMisc.showTypes != MO)
	{
		gMisc.showTypes = MO;
		initNames();
		userItemsInit();
	}
	
	for (i = 0; i < kMaxWalls; i++)
		gNextWall[i] = -1;
	
	formatMapInfo(gMapInfoStr);
	gMapLoaded = TRUE;
	boardPreloadTiles();
	AutoAdjustSprites();
	CleanUpMisc();
	CleanUp();
	
	updatesector(posx, posy, &cursectnum);
	gMapedHud.SetMsgImp(128, gMapInfoStr);
	clampCamera();
	
    gMapSnapshot.Make(0);
    
    oldnumsectors = numsectors;
    oldnumsprites = numsprites;
    oldnumwalls = numwalls;
    
	return 0;
}

int boardSave(char* filename, BOOL autosave)
{
	int i;
	keyClear();
	UndoSectorLighting(); // fix up floor and ceiling shade values for sectors that have dynamic lighting
	worldSprCallFunc(sprFixSector);
	
	if (!autosave)
		gMapRev++;
		
	CleanUp();
	setStartPos(startposx, startposy, startposz, startang, gMisc.forceEditorPos); // this updates startsectnum as well
	if ((i = dbSaveMap(filename, (mapversion == 7 || gCompat.modernMap))) != 0)
	{
		if (!autosave)
			Alert("Failed to save %s (code: %d)", filename, i);
	}
	else
	{
		asksave = 0;
	}
	
	formatMapInfo(gMapInfoStr);
	return i;
}

int boardSaveAuto()
{
	char path[BMAX_PATH] = "\0", *p = path;
	getPath(gPaths.maps, path, 1);
	p += strlen(path);

	gAutosave.current++;
	if (gAutosave.current > gAutosave.max)
		gAutosave.current = 1;

	sprintf(p, "%s%d.%s", gAutosave.basename, gAutosave.current, gExtNames[kMap]);
	return boardSave(path, 1);
}

void boardReset(int hgltreset)
{
	gTimers.autosave = gFrameClock + gAutosave.interval;
	
	oldnumsprites = oldnumsectors = oldnumwalls = 0;
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
	mapversion = 7;
	gMapRev    = 0;
	
	memset(joinsector, -1, sizeof(joinsector));
	
	if (!hgltreset) hgltReset(); // reset using default params
	else hgltReset(hgltreset); 	// override param
	
	sectorToolDisableAll(1);
	memset(gMapInfoStr, 0, sizeof(gMapInfoStr));
	gCommentMgr.DeleteAll();
	gMapSnapshot.Remove();
	CXTracker::Clear();
	eraseExtra();
	dbInit();
}

void boardStartNew()
{
	boardReset();
	LOOPSHAPE shape(kLoopShapeSquare, -1, startposx, startposy); // create square sector
	shape.Setup(startposx + MET2PIX(8), startposy + MET2PIX(8), NULL);
	shape.Insert();
	shape.Stop();
	
	startsectnum = cursectnum = 0;
	gMapLoaded = 1;
}

struct MAPSNAPEXTRA
{
	CAMERA cam;
	uint16_t msx, msy;
	uint8_t  zmode;
};

static MAPSNAPEXTRA oldsnapextra;
int boardSnapshotMake(BYTE** pData, int* crcSiz)
{
	sectortype* pSect; XSECTOR* pXSect; walltype* pWall; XWALL* pXWall;
	MAPSNAPEXTRA extra;
	
	int totalsiz = 0, miscsiz = 0, curofs, miscofs, t, i, j;
	short numcomments = gCommentMgr.commentsCount;
	uint8_t pana[4], panb[4];
	uint8_t restore;

	// Count size of everything
	
	totalsiz += sizeof(boardWidth) * 2;
	totalsiz += sizeof(startposx)  * 3;
	totalsiz += sizeof(startsectnum);
	totalsiz += sizeof(startang);
	
	totalsiz += sizeof(pskybits);
	totalsiz += sizeof(parallaxtype);
	totalsiz += sizeof(Sky::pieces);
	totalsiz += sizeof(visibility);

	totalsiz += sizeof(numcomments);
	totalsiz += sizeof(numsectors);
	totalsiz += sizeof(numwalls);
	totalsiz += sizeof(numsprites);
	totalsiz += sizeof(miscofs);
	
	totalsiz += sizeof(pskyoff[0]) * Sky::pieces;
	totalsiz += sizeof(MAP_COMMENT) * numcomments;
	totalsiz += sizeof(walltype)    * numwalls;
	totalsiz += sizeof(sectortype)  * numsectors;
	totalsiz += sizeof(spritetype)  * numsprites;
	

	i = numsectors;
	while(--i >= 0)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
			if (sprite[j].extra > 0)
				totalsiz += sizeof(XSPRITE);
		
		pSect = &sector[i];
		if (pSect->extra > 0)
		{
			totalsiz += sizeof(XSECTOR);
			pXSect = &xsector[pSect->extra];
			if (!pXSect->panAlways && !pXSect->busy)	continue;
			if (pXSect->panFloor)						miscsiz += sizeof(pana);
			if (pXSect->panCeiling)						miscsiz += sizeof(panb);
		}
	}
	
	i = numwalls;
	while(--i >= 0)
	{
		pWall = &wall[i];
		if (pWall->extra > 0)
		{
			totalsiz += sizeof(XWALL);
			pXWall = &xwall[pWall->extra];
			if (!pXWall->panAlways && !pXWall->busy)	continue;
			if (pXWall->panXVel)						miscsiz += (sizeof(pana)>>1);
			if (pXWall->panYVel)						miscsiz += (sizeof(pana)>>1);
		}
	}
    
    if (gMisc.undoCamRestore)
        miscsiz += sizeof(extra);

	*crcSiz		= totalsiz;		// This is edge for crc
	miscofs		= totalsiz;		// This is start for writing non-crc stuff
	totalsiz 	+= miscsiz;
	
	if ((*pData = (BYTE*)malloc(totalsiz)) == NULL)
		return -1;
	
	IOBuffer Io(totalsiz, *pData);
	
	// Write header
	Io.write(&boardWidth,	sizeof(boardWidth));
	Io.write(&boardHeight,	sizeof(boardHeight));
	
	Io.write(&startposx,	sizeof(startposx));
	Io.write(&startposy,	sizeof(startposy));
	Io.write(&startposz,	sizeof(startposz));
	Io.write(&startang,		sizeof(startang));
	Io.write(&startsectnum,	sizeof(startsectnum));
	
	Io.write(&pskybits,		sizeof(pskybits));
	Io.write(&parallaxtype, sizeof(parallaxtype));
	Io.write(&Sky::pieces,	sizeof(Sky::pieces));
	Io.write(&visibility,	sizeof(visibility));
	Io.write(&numcomments,	sizeof(numcomments));
	Io.write(&numsectors,	sizeof(numsectors));
	Io.write(&numwalls,		sizeof(numwalls));
	Io.write(&numsprites,	sizeof(numsprites));
	Io.write(&miscofs,		sizeof(miscofs));
	
	Io.write(&pskyoff,		sizeof(pskyoff[0]) * Sky::pieces);
	
	// Write comments before objects
	for (i = 0; i < numcomments; i++)
		Io.write(&gCommentMgr.comments[i], sizeof(MAP_COMMENT));
	
	UndoSectorLighting();
	
	for (i = 0; i < numsectors; i++)
	{
		pSect = &sector[i];
		pXSect = NULL;
		restore = 0;

		if (pSect->extra > 0)
		{
			pXSect = &xsector[pSect->extra];
			if (pXSect->panAlways || pXSect->busy)
			{
				// This is so complicated, but has
				// to be done for proper
				// crc calculation!
				
				curofs = Io.tell();			// Save current offset
				Io.seek(miscofs, SEEK_SET);	// Go to misc
				
				if (pXSect->panFloor)
				{
					// Save object panning
					pana[0] = pSect->floorxpanning;
					pana[1] = pSect->floorypanning;
					pana[2] = pXSect->floorXPanFrac;
					pana[3] = pXSect->floorYPanFrac;
					
					// Now clear panning for object
					pXSect->floorXPanFrac = pXSect->floorYPanFrac = 0;
					pSect->floorxpanning  = pSect->floorypanning  = 0;
					
					// Write saved panning into misc
					Io.write(&pana, sizeof(pana));
					restore |= 0x01;
				}
				
				if (pXSect->panCeiling)
				{
					// Save object panning
					panb[0] = pSect->ceilingxpanning;
					panb[1] = pSect->ceilingypanning;
					panb[2] = pXSect->ceilXPanFrac;
					panb[3] = pXSect->ceilYPanFrac;
					
					// Now clear panning for object
					pXSect->ceilXPanFrac	= pXSect->ceilYPanFrac = 0;
					pSect->ceilingxpanning  = pSect->ceilingypanning  = 0;
					
					// Write saved panning into misc
					Io.write(&panb, sizeof(panb));
					restore |= 0x02;
				}
				
				miscofs = Io.tell();		// Update misc offset
				Io.seek(curofs, SEEK_SET);	// Back to current
			}
		}
		
		// Write into object offset
		Io.write(pSect, sizeof(sectortype));
		if (pXSect)
		{
			// Write into xobject offset
			Io.write(pXSect, sizeof(XSECTOR));
			
			if (restore & 0x01)
			{
				pSect->floorxpanning	= pana[0];
				pSect->floorypanning	= pana[1];
				pXSect->floorXPanFrac	= pana[2];
				pXSect->floorYPanFrac	= pana[3];
			}
			
			if (restore & 0x02)
			{
				pSect->ceilingxpanning	= panb[0];
				pSect->ceilingypanning	= panb[1];
				pXSect->ceilXPanFrac	= panb[2];
				pXSect->ceilYPanFrac	= panb[3];
			}
		}
	}
	
	for (i = 0; i < numwalls; i++)
	{
		pWall = &wall[i];
		pXWall = NULL;
		restore = 0;
		
		if (pWall->extra > 0)
		{
			pXWall = &xwall[pWall->extra];
			if (pXWall->panAlways || pXWall->busy)
			{
				// This is so complicated, but has
				// to be done for proper
				// crc calculation!
				
				curofs = Io.tell();			// Save current offset
				Io.seek(miscofs, SEEK_SET);	// Go to misc
				
				if (pXWall->panXVel)
				{
					// Save object panning
					pana[0] = pWall->xpanning;
					pana[1] = pXWall->xpanFrac;
					
					// Now clear panning for object
					pWall->xpanning  = 0;
					pXWall->xpanFrac = 0;
					
					// Write saved panning into misc
					Io.write(&pana, sizeof(pana)>>1);
					restore |= 0x01;
				}
				
				if (pXWall->panYVel)
				{
					// Save object panning
					panb[0] = pWall->ypanning;
					panb[1] = pXWall->ypanFrac;
					
					// Now clear panning for object
					pWall->ypanning  = 0;
					pXWall->ypanFrac = 0;
					
					// Write saved panning into misc
					Io.write(&panb, sizeof(panb)>>1);
					restore |= 0x02;
				}
				
				miscofs = Io.tell();		// Update misc offset
				Io.seek(curofs, SEEK_SET);	// Back to current
			}
		}
		
		// Write into current offset
		Io.write(pWall, sizeof(walltype));
		if (pXWall)
		{
			// Write into current offset
			Io.write(pXWall, sizeof(XWALL));
			
			if (restore & 0x01)
			{
				pWall->xpanning  = pana[0];
				pXWall->xpanFrac = pana[1];
			}
			
			if (restore & 0x02)
			{
				pWall->ypanning  = panb[0];
				pXWall->ypanFrac = panb[1];
			}
		}
	}
	
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			Io.write(&sprite[i], sizeof(spritetype));
			if (sprite[i].extra > 0)
				Io.write(&xsprite[sprite[i].extra], sizeof(XSPRITE));
		}	
	}
    
	if (gMisc.undoCamRestore)
    {
        // Also write other non-crc
        // stuff into misc
        // offset
        
        extra.cam.x	= posx;
        extra.cam.y = posy;
        extra.cam.z = posz;
        extra.cam.s = cursectnum;
        extra.cam.a = ang;
        extra.cam.h = horiz;
        extra.msx   = searchx;
        extra.msy   = searchy;
        extra.zmode = zmode;
        
        Io.seek(miscofs, SEEK_SET);
        Io.write(&extra, sizeof(extra));
        memcpy(&oldsnapextra, &extra, sizeof(extra));
    }
    
	return totalsiz;
}


int boardSnapshotLoad(BYTE* pData, int nLen, char isRedo)
{
	sectortype *pSect; XSECTOR *pXSect; walltype *pWall; XWALL *pXWall;
	spritetype *pSpr, tspr;  MAP_COMMENT cmt; MAPSNAPEXTRA extra;
	int NumSprites, miscofs, curofs, i, j, k;
	short numcomments;
	uint8_t pan[4];
	
	IOBuffer Io(nLen, pData);
	
	// Read header
	Io.read(&boardWidth,	sizeof(boardWidth));
	Io.read(&boardHeight,	sizeof(boardHeight));
	
	Io.read(&startposx,		sizeof(startposx));
	Io.read(&startposy,		sizeof(startposy));
	Io.read(&startposz,		sizeof(startposz));
	Io.read(&startang,		sizeof(startang));
	Io.read(&startsectnum,	sizeof(startsectnum));
	
	Io.read(&pskybits,		sizeof(pskybits));
	Io.read(&parallaxtype,	sizeof(parallaxtype));
	Io.read(&Sky::pieces,	sizeof(Sky::pieces));
	Io.read(&visibility,	sizeof(visibility));
	Io.read(&numcomments,	sizeof(numcomments));
	Io.read(&numsectors,	sizeof(numsectors));
	Io.read(&numwalls,		sizeof(numwalls));
	Io.read(&numsprites,	sizeof(numsprites));
	Io.read(&miscofs,		sizeof(miscofs));
	
	Io.read(&pskyoff, sizeof(pskyoff[0]) * Sky::pieces);
	
	gCommentMgr.DeleteAll();
	numxsprites = numxwalls = numxsectors = 0;
	NumSprites = numsprites;
	numsprites = 0;
	dbInit();
	
	for (i = 0; i < numcomments; i++)
	{
		Io.read(&cmt, sizeof(cmt));
		cmt.initRebind = 0;
		
		gCommentMgr.Add(&cmt);
	}
	
	for (i = 0; i < numsectors; i++)
	{
		pSect = &sector[i];
		Io.read(pSect, sizeof(sectortype));
		if (pSect->extra <= 0)
			continue;
		
		k = pSect->extra;
		j = dbInsertXSector(i); pXSect = &xsector[j];
		Io.read(pXSect, sizeof(XSECTOR));
		
		if (numcomments && k != j)
			gCommentMgr.RebindMatching(OBJ_SECTOR, k, OBJ_SECTOR, j, 1);
		
		if (pXSect->panAlways || pXSect->busy)
		{
			// Read the actual object panning
			
			curofs = Io.tell();				// Save current offset
			Io.seek(miscofs, SEEK_SET);		// Go to misc
			
			if (pXSect->panFloor)
			{
				Io.read(&pan, sizeof(pan));
				
				pSect->floorxpanning  = pan[0];
				pSect->floorypanning  = pan[1];
				pXSect->floorXPanFrac = pan[2];
				pXSect->floorYPanFrac = pan[3];
			}
			
			if (pXSect->panCeiling)
			{
				Io.read(&pan, sizeof(pan));
				
				pSect->ceilingxpanning = pan[0];
				pSect->ceilingypanning = pan[1];
				pXSect->ceilXPanFrac   = pan[2];
				pXSect->ceilYPanFrac   = pan[3];
			}
			
			miscofs = Io.tell();		// Update misc offset
			Io.seek(curofs, SEEK_SET);	// Go to object offset
		}
	}

	for (i = 0; i < numwalls; i++)
	{
		pWall = &wall[i];
		Io.read(pWall, sizeof(walltype));
		
		if (pWall->extra > 0)
		{
			k = pWall->extra;
			j = dbInsertXWall(i); pXWall = &xwall[j];
			Io.read(pXWall, sizeof(XWALL));
			
			if (numcomments && k != j)
				gCommentMgr.RebindMatching(OBJ_WALL, k, OBJ_WALL, j, 1);
			
			if (pXWall->panAlways || pXWall->busy)
			{
				// Read the actual object panning
				
				curofs = Io.tell();				// Save current offset
				Io.seek(miscofs, SEEK_SET);		// Go to misc
				
				if (pXWall->panXVel)
				{
					Io.read(&pan, sizeof(pan)>>1);
					
					pWall->xpanning  = pan[0];
					pXWall->xpanFrac = pan[1];
				}
				
				if (pXWall->panYVel)
				{
					Io.read(&pan, sizeof(pan)>>1);
					
					pWall->ypanning  = pan[0];
					pXWall->ypanFrac = pan[1];
				}
				
				miscofs = Io.tell();		// Update misc offset
				Io.seek(curofs, SEEK_SET);	// Go to object offset
			}
		}
	}

	for (i = 0; i < NumSprites; i++)
	{
		Io.read(&tspr, sizeof(spritetype));

		if ((j = InsertSprite(tspr.sectnum, tspr.statnum)) >= 0)
		{
			pSpr = &sprite[j];
			memcpy(pSpr, &tspr, sizeof(spritetype));
			pSpr->index = j;
			
			if (numcomments && tspr.index != j)
				gCommentMgr.RebindMatching(OBJ_SPRITE, tspr.index, OBJ_SPRITE, j, 1);
			
			if (tspr.statnum == kStatMarker)
			{
				if ((k = tspr.owner) >= 0 && sector[k].extra > 0)
				{
					pXSect = &xsector[sector[k].extra];
					if (pXSect->marker0 == tspr.index)
						pXSect->marker0 = j;
					
					if (pXSect->marker1 == tspr.index)
						pXSect->marker1 = j;
				}	
			}
		}
		
		if (tspr.extra > 0)
		{
			if (j >= 0)
			{
				k = dbInsertXSprite(j);
				Io.read(&xsprite[k], sizeof(XSPRITE));
				xsprite[k].reference = j;
			}
			else
			{
				Io.skip(sizeof(XSPRITE));
			}
		}
	}
    
	if (gMisc.undoCamRestore)
    {
        // Also read other non-crc
        // stuff from misc
        // offset
        
        Io.seek(miscofs, SEEK_SET);
        Io.read(&extra, sizeof(extra));

        if (isRedo)
        {
            memcpy(&oldsnapextra, &extra, sizeof(extra));
            
            posx		= oldsnapextra.cam.x;
            posy		= oldsnapextra.cam.y;
            posz		= oldsnapextra.cam.z;
            ang 		= oldsnapextra.cam.a;
            cursectnum	= oldsnapextra.cam.s;
            horiz		= oldsnapextra.cam.h;
            searchx		= oldsnapextra.msx;
            searchy		= oldsnapextra.msy;
            zmode		= oldsnapextra.zmode;
        }
        else
        {
            posx		= oldsnapextra.cam.x;
            posy		= oldsnapextra.cam.y;
            posz		= oldsnapextra.cam.z;
            ang 		= oldsnapextra.cam.a;
            cursectnum	= oldsnapextra.cam.s;
            horiz		= oldsnapextra.cam.h;
            searchx		= oldsnapextra.msx;
            searchy		= oldsnapextra.msy;
            zmode		= oldsnapextra.zmode;
            
            memcpy(&oldsnapextra, &extra, sizeof(extra));
        }
    }

	oldnumsectors	= numsectors;
	oldnumwalls		= numwalls;
	oldnumsprites	= numsprites;
	
	hgltReset(kHgltSector | kHgltGradient | kHgltPoint);
	sectorToolDisableAll(1);
	CleanUp();
	
    return 1;
}

static NAMED_TYPE gObjectInfoGroups[] =
{
	{OBJ_SPRITE,	"Sprites"},
	{OBJ_WALL,		"Walls"},
	{OBJ_SECTOR,	"Sectors"},
	{-1,			NULL},
};

enum
{
	kObjInfoParName,
	kObjInfoParCaption,
	kObjInfoParAvail,
};
static NAMED_TYPE gObjectInfoParams[] =
{
	{kObjInfoParName,		"Name"},
	{kObjInfoParCaption,	"Caption"},
	{kObjInfoParAvail,		"Availability"},
};

static char initNamesCmp(char* pA, char* pB)
{
	return (pA && Bstrcasecmp(pA, pB) == 0);
}

static char* initNamesHelperGetPtr(char* pName, char searchIn, int nMax)
{
	char* p; int i, j;
	if (searchIn & 0x03)
	{
		for (i = 0; i < nMax; i++)
		{
			if (searchIn & 0x01)
			{
				if (initNamesCmp(gSpriteNames[i], pName))			return gSpriteNames[i];
				if (initNamesCmp(gSpriteCaptions[i], pName))		return gSpriteCaptions[i];
				
				if (initNamesCmp(gWallNames[i], pName))				return gWallNames[i];
				if (initNamesCmp(gWallCaptions[i], pName))			return gWallCaptions[i];
				
				if (initNamesCmp(gSectorNames[i], pName))			return gSectorNames[i];
				if (initNamesCmp(gSectorCaptions[i], pName))		return gSectorCaptions[i];
			}
			
			if (searchIn & 0x02)
			{
				for (j = 0; j < 4; j++)
					if (initNamesCmp(gSpriteDataNames[i][j], pName))
						return gSpriteDataNames[i][j];
				
				if (initNamesCmp(gWallDataNames[i][0], pName))		return gWallDataNames[i][0];
				if (initNamesCmp(gSectorDataNames[i][0], pName))	return gSectorDataNames[i][0];
			}
		}
	}
	
	p = (char*)malloc(strlen(pName)+1);
	dassert(p != NULL);
	p[0] = '\0';
	return p;
}

void initNames()
{
	char key[256], val[256], *pKey, *pVal, *pStr, ignore;
	NAMED_TYPE *pGroup = gObjectInfoGroups;
	int nPar, nType, nGroup, nVal, i, t, o;
	RESHANDLE hIni; int nPrevNode;
	static int nTypeMax = 0;
	
	if ((hIni = gGuiRes.Lookup((unsigned int)4, "INI")) == NULL)
		return;
	
	IniFile* pIni = new IniFile((BYTE*)gGuiRes.Load(hIni), hIni->size);
	while(pGroup->name)
	{
		nPrevNode = -1; nGroup = pGroup->id;
		while(pIni->GetNextString(NULL, &pKey, &pVal, &nPrevNode, pGroup->name))
		{
			switch(nGroup)
			{
				case OBJ_SPRITE:
				case OBJ_SECTOR:
				case OBJ_WALL:
					// fill object type names, captions and data field names
					if (isIdKeyword(pKey, "Type", &nType) && rngok(nType, 0, 1024))
					{
						o = 0, ignore = 0;
						while((o = enumStr(o, pVal, key, val)) > 0)
						{
							// check if type must be available first
							if (findNamedID(key, gObjectInfoParams, LENGTH(gObjectInfoParams)) == kObjInfoParAvail)
							{
								nVal = 0;
								ignore = (!isufix(val) || (nVal = atoi(val)) > MO || gMisc.showTypes < nVal);
								if (nVal == MO && !gModernTypes.Exists(nGroup, nType))
									gModernTypes.Add(nGroup, nType);
								
								break;
							}
						}
						
						if (ignore) continue;
						else if (nType > nTypeMax)
							nTypeMax = nType;
						
						o = 0;
						while((o = enumStr(o, pVal, key, val)) > 0)
						{
							if (isIdKeyword(key, "Data", &nPar))
							{
								// setup data names here
								///////////////////////////////////
								
								char* p = val;
								if (p[0] == '*') // only when modern features enabled
								{
									if (gMisc.showTypes != MO)
										continue;
									
									p = &val[1];
								}
								
								switch(nGroup)
								{
									case OBJ_SPRITE:
										if (irngok(nPar, 1, 4))
										{
											pStr = initNamesHelperGetPtr(p, 0x02, nTypeMax);
											gSpriteDataNames[nType][nPar - 1] = pStr;
											if (pStr[0] == '\0')
												sprintf(pStr, "%s", p);
										}
										break;
									default:
										if (nPar == 1)
										{
											pStr = initNamesHelperGetPtr(p, 0x02, nTypeMax);
											if (nGroup == OBJ_WALL) gWallDataNames[nType][0] = pStr;
											else gSectorDataNames[nType][0] = pStr;
											if (pStr[0] == '\0')
												sprintf(pStr, "%s", p);
										}
										break;
								}
							}
							else if ((nPar = findNamedID(key, gObjectInfoParams, LENGTH(gObjectInfoParams))) >= 0)
							{
								switch(nPar)
								{
									case kObjInfoParCaption:
										if (!gMisc.useCaptions) break;
										// no break
									case kObjInfoParName:
										pStr = initNamesHelperGetPtr(val, 0x01, nTypeMax);
										if (pStr[0] == '\0')
											sprintf(pStr, "%s", val);
										
										switch(nGroup)
										{
											case OBJ_SPRITE:
												if (nPar == kObjInfoParName) gSpriteNames[nType] = pStr;
												else gSpriteCaptions[nType] = pStr;
												break;
											case OBJ_WALL:
												if (nPar == kObjInfoParName) gWallNames[nType] = pStr;
												else gWallCaptions[nType] = pStr;
												break;
											default:
												if (nPar == kObjInfoParName) gSectorNames[nType] = pStr;
												else gSectorCaptions[nType] = pStr;
												break;
										}
										break;
								}
							}
						}
					}
					break;
			}
		}
		
		pGroup++;
	}
	
	delete(pIni);
	
	for (i = 0; i < 1024; i++)
	{
		// point empty captions to a full names
		if (!gSpriteCaptions[i])	gSpriteCaptions[i]	= gSpriteNames[i];
		if (!gSectorCaptions[i])	gSectorCaptions[i]	= gSectorNames[i];
		if (!gWallCaptions[i])		gWallCaptions[i]	= gWallNames[i];
	}
	
	if (gMisc.showTypes == MO)
	{
		// fill the modern command names
		for (i = 0; i < LENGTH(gModernCommandNames); i++)
		{
			t = gModernCommandNames[i].id;
			gCommandNames[t] = gModernCommandNames[i].name;
		}
	}
	
	for (i = 64, t = 0; i < LENGTH(gCommandNames); i++)
	{
		if (!gCommandNames[i])
		{
			// fill numbers for numberic commands
			gCommandNames[i] = (char*)malloc(4);
			sprintf(gCommandNames[i], "%3d", t++);
		}
	}
}

void xmpEditLoopProcess3D(void)
{
	ExtPreCheckKeys();
	drawrooms(posx, posy, posz, ang, horiz, cursectnum);
	ExtAnalyzeSprites();
	drawmasks();
	
	UndoSectorLighting();
	ExtCheckKeys();
	
	OSD_Draw();
	showframe();
}

void xmpEditLoopProcess2D(void)
{
	gScreen2D.ScreenClear();
	gScreen2D.ScreenDraw(posx, posy, ang, grid, zoom);
	UndoSectorLighting();
	ExtCheckKeys();
	
	OSD_Draw();
	showframe();
}

void xmpEditLoopProcessSPLIT(void)
{
	Rect r3D, *b3D; Rect r2D, *b2D;
	SCREEN2D* pScr2D = &gScreen2D;
	
	xmpSplitModeCountWindowSize(&r3D, &r2D);
	b2D = new Rect(pScr2D->view.wx1, pScr2D->view.wy1, pScr2D->view.wx2, pScr2D->view.wy2);
	b3D = new Rect(windowx1, windowy1, windowx2, windowy2);
	
	setview(r3D.x0, r3D.y0, r3D.x1, r3D.y1);
	pScr2D->SetView(r2D.x0, r2D.y0, r2D.x1, r2D.y1, 1);
	
	gfxSetColor(clr2std(kColorGrey29));
	gfxFillBox(&r3D);
	
	ExtPreCheckKeys();
	drawrooms(posx, posy, posz, ang, horiz, cursectnum);
	getHighlightedObject();
	ExtAnalyzeSprites();
	drawmasks();
	
	if (gNoclip && cursectnum >= 0)
	{
		int nSect2 = cursectnum;
		if (FindSector(posx, posy, posz, &nSect2))
			cursectnum = nSect2;
	}
	
	if (cursectnum < 0)
		gfxDrawTextRect(&r3D, ALG_MIDDLE|ALG_CENTER, clr2std(kColorRed ^ h), "SECTOR LOST", qFonts[0]);
	
	pScr2D->ScreenClear();
	pScr2D->ScreenDraw(posx, posy, ang, grid, zoom);
	xmpSplitModeProcess(&r3D, &r2D);
	UndoSectorLighting();
	
	ExtCheckKeys();

	if ((ED23 & 0x80) == 0)
	{
		pScr2D->SetView(b2D->x0, b2D->y0, b2D->x1, b2D->y1, 1);
		setview(b3D->x0, b3D->y0, b3D->x1, b3D->y1);
	}
	else
	{
		ED23 &= ~0x80;
	}
	
	OSD_Draw();
	showframe();
}

int xmpEditLoop()
{
	while(!quitevent)
	{
		handleevents();
		OSD_DispatchQueued();
		pGEditLoopProcessFunc();
	}
	
	return -1;
}

void xmpSetEditMode(char nMode)
{
	char reset = ((nMode & 0x80) == 0);
	gScreen2D.SetView(0, 0, xdim, ydim);
	setview(0, 0, xdim-1, ydim-1);
	keyClear();
	
	gMouse.X = searchx = MIDPOINT(windowx1, windowx2);
	gMouse.Y = searchy = MIDPOINT(windowy1, windowy2);
	
	if (reset)
	{
		vel = svel = angvel = hvel = doubvel = 0;
		hgltReset(kHgltGradient);
		horiz = 100;
	}
	
	switch(nMode & 0x03)
	{
		default:
			hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);
			scrSetGameMode(fullscreen, xdim, ydim, bpp);
			pGEditLoopProcessFunc = xmpEditLoopProcess3D;
			ED23 = 0;
			break;
		case 0x00:
			hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
			scrSetGameMode(fullscreen, xdim, ydim, bpp);
			pGEditLoopProcessFunc = xmpEditLoopProcess2D;
			qsetmodeany(xdim, ydim);
			
			if (reset)
			{
				joinsector[0] = joinsector[1] = -1;
				sectorToolDisableAll(1);
				hgltReset(kHgltPoint);
			}
			
			ED23 = 0;
			break;
		case 0x02:
			hudSetLayout(&gMapedHud, gHudPrefs.layoutSPLIT, &gMouse);
			pGEditLoopProcessFunc = xmpEditLoopProcessSPLIT;
			xmpSplitModeSet(ED3D);
			if ((nMode & 0x80))
				ED23 |= 0x80;
			
			break;
	}
}

void xmpCreateToolTestMap(char withSprite)
{
	#define kDist 16
	
	sectortype *pSectA, *pSectB; spritetype* pSpr;
	LOOPSHAPE* pShape;
	int s, e;
	
	clipmovemask3d = BLOCK_MOVE;
	zmode = 2;

	pShape = new LOOPSHAPE(kLoopShapeCircle, -1, posx, posy);
	pShape->Setup(posx + MET2PIX(kDist), posy + MET2PIX(kDist), NULL);
	s = pShape->Insert();
	delete(pShape);
	
	pSectA = &sector[0];
	sectCstatAdd(0, kSectParallax, OBJ_CEILING);
	sectCstatAdd(0, kSectShadeFloor, OBJ_FLOOR);
	pSectA->floorpicnum = 459;
	
	pShape = new LOOPSHAPE(kLoopShapeCircle, 0, posx, posy);
	pShape->Setup(posx + MET2PIX((kDist - 1)), posy + MET2PIX((kDist - 1)), NULL);
	s = pShape->Insert();
	delete(pShape);
	
	redSectorMake(s);
	pSectB = &sector[1];
	getSectorWalls(1, &s, &e);
	while(s <= e)
	{
		wall[s].cstat |= kWallBlock;
		wall[s].picnum = 253;
		s++;
	}
	

	
	pSectA->floorz -= 4096;
	pSectA->ceilingz = pSectA->floorz;
	pSectA->floorpicnum = 253;
	
	Sky::SetPic(0, OBJ_CEILING, 2500, 1);
	
	if (withSprite)
	{
		pSpr = &sprite[InsertSprite(1, 0)];
		pSpr->z = pSectB->floorz;
		pSpr->x = posx;
		pSpr->y = posy;
		
		clampSprite(pSpr);
		posz = pSpr->z;

		searchstat		= OBJ_SPRITE;
		searchwall		= pSpr->index;
		searchindex		= pSpr->index;
		searchsector	= 1;
		
		pointhighlight	= (pSpr->index + 16384);
		
		offsetPos(0, -MET2PIX(3), 0, ang, &posx, &posy, NULL);
	}
	else
	{
		searchstat		= OBJ_FLOOR;
		searchindex		= 1;
		searchsector	= 1;
	}
	
	gMapLoaded = 1;
	
}

char IsHoverSector()
{
	return (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING || searchstat == OBJ_SECTOR);
}

char IsHoverWall()
{
	return (searchstat == OBJ_WALL || searchstat == OBJ_MASKED);
}

char* GetHoverName()
{
	const int nMax = LENGTH(gSearchStatNames);
	
	if (rngok(searchstat, 0, nMax-1))
		return gSearchStatNames[searchstat];
	
	return gSearchStatNames[nMax-1];
}