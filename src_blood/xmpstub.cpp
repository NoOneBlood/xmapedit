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

char buffer[kBufferSize]  = "";
char buffer2[kBufferSize] = "";
char buffer3[kBufferSize] = "";
char gMapInfoStr[kBufferSize] = "";

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

char* gDamageNames[7] =
{
	"None",					// 0
	"Fall",					// 1
	"Burn",					// 2
	"Vector",				// 3
	"Explode",				// 4
	"Choke",				// 5
	"Electric",				// 6
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

NAMED_TYPE gSectorMergeMenu[] =
{
	{0, "&Just make a new sector"},
	{1, "&Make and merge with outer sector"},
	{2, "&Cancel"},
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
	if (!ED23 && gHudPrefs.dynamicLayout2D)
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
	int t  = (fullscreen) ? 0 : 2;
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
		pTwo->x1 = xdim-t-1;
		pTwo->y1 = hg;
	}
	else
	{
		sz = perc2val(sz, hg);
		
		pOne->x0 = 1;
		pOne->y0 = 1;
		pOne->x1 = xdim-t-1;
		pOne->y1 = sz-(kPad>>1);
		
		pTwo->x0 = 1;
		pTwo->y0 = sz+(kPad>>1);
		pTwo->x1 = xdim-t-1;
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
	char vFrameCol	= clr2std(kColorGrey29);
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
	static int oldnumwalls = 0, oldnumsectors = 0, oldnumsprites = 0;
	static char hudLogoChanged = 0, compatStat = 0;
	static OBJECT object;
	char fullLayout;
	
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
			CleanUp();
		}
		
		hgltShowStatus(4, (totalclock < gScreen.msg[0].time) ? 14 : 4);
		
		if (totalclock > gTimers.autosave)
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
	}
	
	keyGetHelper(&key, &ctrl, &shift, &alt);
	if ((key) && (gPreviewMode && (previewProcessKeys() || !previewCheckKey(key))) || (processKeysShared()))
	{
		keystatus[key] = 0;
		keyClear();
		key = 0;
	}

	scrDisplayMessage();
	CalcFrameRate();
	processMove();
	
	if (ED3D)
	{
		process3DMode();
		ProcessKeys3D();
	}
	else
	{
		process2DMode();
		ProcessKeys2D();
	}
	
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
	
	if (key)
		asksave = 1;
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
	EditNumber* eW			= new EditNumber(kPad1, cy, enw, enh, PIX2MET(boardWidth), kValMeter, 10, 10000);
	Label* lX				= new Label(eW->left + eW->width + kPad1, cy + (pFont->height>>1), "X");
	EditNumber* eH			= new EditNumber(lX->left + lX->width + kPad1 + 1, cy, enw, enh, PIX2MET(boardHeight), kValMeter, 10, 10000);
	Label* lDesc			= new Label(eH->left + eW->width + kPad1, cy + (pFont->height>>1), "1m = 512 pixels", kColorDarkGray);
	
	Panel* pButtons 		= new Panel(dx1, dy2-bh-1, dwh, bh);
	TextButton* bOk			= new TextButton(0, 0, bw, bh, "Confirm", mrOk);
	bOk->fontColor 			= kColorBlue;
	TextButton* bCancel		= new TextButton(bOk->left + bOk->width + 4, 0, bw, bh, "Cancel", mrCancel);
	bCancel->fontColor		= kColorRed;
	
	dy1 += fBSize->height + 16;
	FieldSet* fVis 			= new FieldSet(dx1+2, dy1, dwh-kPad1, enh<<1, "GLOBAL VISIBILITY", kColorDarkGray, kColorDarkGray);
	EditNumber* eVis		= new EditNumber(kPad1, cy, enw, enh, 100 - IVAL2PERC(visibility, 4095), kValPerc, 0, 100);
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
				visibility				= 4095 - perc2val(eVis->value, 4095);
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
	
	return OSDCMD_OK;
}



int ExtInit(int argc, char const * const argv[])
{
	static char myBuildDate[16] = "UNK_DATE";
	char fallThrough = 0, *tmp;
	int i, retn = 0;
	
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

	memset(joinsector, -1, sizeof(joinsector));
	gSeqEd.filename 			= gPaths.seqs;
	gTool.objType 				= -1;
	gTool.objIndex 				= -1;
	// ---------------------------------------------------------------------------
	buildprintf("Init completed!\n\n\n");
	xmpSetEditMode(gMisc.editMode);
	boardStartNew();
	splashScreen();

	if (gMisc.forceSetup) // should we show options?
		xmpOptions();
	
	buffer[0] = '\0';
	if (argc <= 1 || isempty(argv[1]))
	{
		strcpy(buffer, (isempty(gPaths.maps)) ? kDefaultMapName : gPaths.maps);
	
		if (!gMisc.autoLoadMap)
		{
			if ((tmp = browseOpen(gPaths.maps, ".map")) == NULL)
			{
				while ( 1 )
				{
					switch(xmpMenuProcess())
					{
						default:
							continue;
						case mrLoad:
						case mrReload:
						case mrLoadAsave:
						case mrNew:
						case mrToolImportWizard:
							fallThrough = 1;
							break;
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
				strcpy(buffer, tmp);
			}
		}
	}
	else
	{
		strcpy(buffer, argv[1]);
	}
	
	if (!isempty(buffer)) // create default map path
		strcpy(gPaths.maps, buffer), ChangeExtension(gPaths.maps, getExt(kMap));
		
	if (!fallThrough)
	{
		switch (i = toolOpenWith(buffer))
		{
			case -1:
			{
				Alert("Could not open \"%s\".", buffer);
				if ((tmp = browseOpen(gPaths.maps, ".map")) == NULL)
				{
					xmpQuit(1);
					break;
				}
				
				strcpy(buffer, tmp);
				// no break
			}
			case kToolMapEdit:
				gMapLoaded = (boardLoad(buffer) == 0);
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
		gAutoGrid.Save(MapEditINI, 		"AutoGrid");
		gMouseLook.Save(MapEditINI, 	"MouseLook");
		gPreview.Save(MapEditINI, 		"PreviewMode");
		gPaths.Save(MapEditINI, 		"Visited");
		gScreen.Save(MapEditINI, 		"Screen");
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

int worldSprCallFunc(HSPRITEFUNC pFunc, int nData = 0)
{
	int i, c = 0;
	for (i = 0; i < kMaxSprites; i++)
	{
		spritetype* pSpr = &sprite[i];
		if (pSpr->statnum < kMaxStatus)
		{
			pFunc(pSpr, nData);
			c++;
		}
	}
	
	return c;
}


BOOL processKeysShared() {

	if (!key)
		return FALSE;
		
	int x, y, swal, ewal, sect2;
	BOOL in2d = ED2D;
	int i = 0, j = 0, k = 0, f = 0, sect = -1, nwall = -1;
	int type;
	
	if (in2d)
	{
		if (pGLShape || pGCircleW || pGDoorR || pGDoorSM)
		{
			switch(key)
			{
				case KEY_PLUS:
				case KEY_MINUS:
				case KEY_SPACE:
				case KEY_F1:
				case KEY_F2:
				case KEY_F3:
				case KEY_F4:
				case KEY_COMMA:
				case KEY_PERIOD:
					return 0;
			}
		}
	}

	type = getHighlightedObject();
		
	switch (key)
	{
		default:
			return FALSE;
		case KEY_ENTER:
			if (ctrl && alt)
			{
				// call the advanced clipboard
				
				if (!type)
				{
					BeepFail();
					break;
				}
				
				if (somethingintab == OBJ_NONE)
				{
					scrSetMessage("There is nothing to paste.");
					BeepFail();
					break;
				}
				
				i = 1;
				if (somethingintab != searchstat)
				{
					if (somethingintab == OBJ_SECTOR)
					{
						if ((searchsector = getSector()) < 0)
						{
							scrSetMessage("Failed to get destination sector.");
							BeepFail();
							break;
						}

						searchstat = OBJ_SECTOR;
					}
					else
					{
						i = 0;
					}
				}
				
				if (i == 0)
				{
					scrSetMessage("Can't copy from %s to %s", gSearchStatNames[ClipHigh(somethingintab, 7)], gSearchStatNames[searchstat]);
					BeepFail();
					break;
				}
				
				if (Beep(tempidx >= 0))
				{
					i = -1;
					switch(searchstat)
					{
						case OBJ_WALL:
						case OBJ_MASKED:
							i = dlgCopyObjectSettings(dlgXWall, OBJ_WALL, tempidx, searchwall, wallInHglt(searchwall));
							break;
						case OBJ_SPRITE:
							i = dlgCopyObjectSettings(dlgXSprite, OBJ_SPRITE, tempidx, searchwall, sprInHglt(searchwall));
							break;
						case OBJ_SECTOR:
						case OBJ_FLOOR:
						case OBJ_CEILING:
							i = tempidx;
							DIALOG_ITEM* pDlg = (shift || (sector[i].extra > 0 && testXSectorForLighting(sector[i].extra)))
								? dlgXSectorFX : dlgXSector;
							
							i = dlgCopyObjectSettings(pDlg, OBJ_SECTOR, tempidx, searchsector, sectInHglt(searchsector));
							break;
					}
					
					if (Beep(i >= 0))
						scrSetMessage("Selected properties were copied for %d objects", i);
				}
				
				break;
			}
			return FALSE;
		case KEY_PADENTER:
			if (ED23)
			{
				if (keystatus[KEY_PADPERIOD])
				{
					xmpSetEditMode(ED3D|0x80);
				}
				else if (ED3D) ED23 |= 0x10;
				else if (ED2D) ED23 |= 0x08;
			}
			else if (!keystatus[KEY_PADPERIOD])
			{
				if (ED2D)
				{
					updatesector(posx, posy, &cursectnum);
					if (gNoclip || cursectnum >= 0)
					{
						xmpSetEditMode(0x01);
					}
					else
					{
						gMapedHud.SetMsgImp(128, "Arrow must be inside a sector before entering 3D mode.");
					}
				}
				else
				{
					xmpSetEditMode(0x00);
				}
			}
			else
			{
				xmpSetEditMode(0x02);
			}
			keystatus[KEY_PADENTER]	 = 0;
			keyClear();
			key = 0;
			return TRUE;
		case KEY_ESC:
			if (pGCircleW)
			{
				scrSetMessage("Points inserting aborted.");
				DELETE_AND_NULL(pGCircleW);
				BeepFail();
				break;
			}
			else if (pGLShape)
			{
				scrSetMessage("Loop inserting aborted.");
				DELETE_AND_NULL(pGLShape);
				BeepFail();
				break;
			}
			else if (pGDoorSM || pGDoorR)
			{
				scrSetMessage("Door inserting aborted.");
				if (pGDoorSM)	DELETE_AND_NULL(pGDoorSM);
				if (pGDoorR)	DELETE_AND_NULL(pGDoorR);
				BeepFail();
				break;
			}
			else if (keystatus[KEY_SPACE])
			{
				if (!in2d && somethingintab != 255)
				{
					scrSetMessage("Clipboard buffer cleared.");
					somethingintab = 255;
					BeepFail();
				}

				break;
			}
			else
			{
				if (highlightcnt > 0) hgltReset(kHgltPoint), i = 1;
				if (!in2d && highlightsectorcnt > 0) hgltReset(kHgltSector), i = 1;
				if (gListGrd.Length() > 0) gListGrd.Clear(), i = 1;
				if (i == 1)
				{
					scrSetMessage("Highlight reset.");
					BeepFail();
					break;
				}
			}
			xmpMenuProcess();
			keyClear();
			break;
		case KEY_COMMA:
		case KEY_PERIOD:
			if (in2d && (highlightsectorcnt > 0 || type == 300))
			{
				j = 0;
				i = (shift) ? ((ctrl) ? 128 : 256) : 512;
				if (key == KEY_COMMA)
					i = -i;
				
				if (highlightsectorcnt > 0)
				{
					j = hgltSectRotate(0x04, i);
				}
				else if (isIslandSector(searchsector))
				{
					int s, e, flags = 0;
					int cx, cy;
					
					flags |= 0x02; // rotate with outer loop of red sector
					getSectorWalls(searchsector, &s, &e); midPointLoop(s, e, &cx, &cy);
					sectRotate(searchsector, cx, cy, i, flags);
					j = 1;
				}
				else
				{
					scrSetMessage("You cannot rotate sectors with connections");
				}
				
				if (Beep(j > 0))
					scrSetMessage("Rotate %d sectors by %d.", j, i);
				
				break;
			}
			else if (type == 200)
			{
				if (sprInHglt(searchwall))
				{
					i = (shift) ? ((ctrl) ? 128 : 256) : 512;
					if (key == KEY_COMMA)
						i = -i;
					
					hgltSprRotate(i);
					scrSetMessage("Rotate %d sprite(s) by %d", hgltSprCount(), i);
					BeepOk();
					break;
				}

				i = (shift) ? 16 : 256;
				if (key == KEY_COMMA) sprite[searchwall].ang = (short)DecNext(sprite[searchwall].ang, i);
				else sprite[searchwall].ang = (short)IncNext(sprite[searchwall].ang, i);
				
				if (!isMarkerSprite(searchwall))
					sprite[searchwall].ang = sprite[searchwall].ang & kAngMask;
				
				scrSetMessage("%s #%d angle: %i", gSearchStatNames[searchstat], searchwall, sprite[searchwall].ang);
				BeepOk();
				break;
			}
			else if (!in2d && type == 100)
			{
				AutoAlignWalls(searchwall);
				BeepOk();
				break;
			}
			break;
		case KEY_PAD5:
			if (!ctrl)
			{
				if (in2d)
				{
					switch(searchstat)
					{
						case OBJ_SPRITE:
							if ((sprite[searchwall].cstat & kSprRelMask) >= kSprWall) break;
						case OBJ_WALL:
							searchsector = sectorhighlight;
							// no break
						case OBJ_FLOOR:
						case OBJ_CEILING:
							switch(gScreen2D.prefs.showMap)
							{
								case 0: return true;
								case 1: searchstat = OBJ_FLOOR;		break;
								case 2: searchstat = OBJ_CEILING;	break;
							}
							if (isSkySector(searchsector, searchstat)) return true;
							else break;
					}
				}
				
				if (searchstat < 255)
					sprintf(buffer, gSearchStatNames[searchstat]);
				
				i = -1;
				switch (searchstat)
				{
					case OBJ_SPRITE:
						i = searchwall;
						sprintf(buffer2, "repeat");
						sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
						break;
					case OBJ_WALL:
					case OBJ_MASKED:
						i = searchwall;
						sprintf(buffer2, "pan/repeat");
						wall[searchwall].xpanning = wall[searchwall].ypanning = 0;
						wall[searchwall].xrepeat  = wall[searchwall].yrepeat  = 8;
						fixrepeats(searchwall);
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						i = searchsector;
						sprintf(buffer2, "pan");
						if (searchstat == OBJ_FLOOR) sector[i].floorxpanning = sector[i].floorypanning = 0;
						else sector[i].ceilingxpanning = sector[i].ceilingypanning = 0;
						if (isSkySector(i, searchstat))
						{
							Sky::SetPan(i, searchstat, 0, 0, alt);
							i = -2;
						}
						break;
				}
				
				if (i >= 0)
					scrSetMessage("%s[%d] %s reset", buffer, i, buffer2);
				
				Beep(i >= 0 || i == -2);
			}
			break;
		case KEY_PAD7:
		case KEY_PAD9:
		case KEY_PADUP:
		case KEY_PADDOWN:
		case KEY_PADLEFT:
		case KEY_PADRIGHT:
		{
			walltype* pWall; spritetype* pSpr; sectortype* pSect;
			BOOL bCoarse, chx = 0, chy = 0;
			signed char changedir, step;
			int vlx, vly;

			changedir = (key == KEY_PADUP || key == KEY_PADRIGHT || key == KEY_PAD9) ? 1 : -1;
			bCoarse = (!shift) ? TRUE : FALSE;
			step = (bCoarse) ? 8 : 1;

			chx = (key == KEY_PADLEFT || key == KEY_PADRIGHT);
			chy = (key == KEY_PADUP   || key == KEY_PADDOWN);
			if (key == KEY_PAD7 || key == KEY_PAD9)
				chx = chy = TRUE;
			
			if (in2d)
			{
				switch(searchstat)
				{
					case OBJ_SPRITE:
						if ((sprite[searchwall].cstat & kSprRelMask) >= kSprWall) break;
					case OBJ_WALL:
						searchsector = sectorhighlight;
						// no break
					case OBJ_FLOOR:
					case OBJ_CEILING:
						switch(gScreen2D.prefs.showMap)
						{
							case 0: return true;
							case 1: searchstat = OBJ_FLOOR;		break;
							case 2: searchstat = OBJ_CEILING;	break;
						}
						if (isSkySector(searchsector, searchstat)) return true;
						else break;
				}
			}
			
			if (searchstat < 255)
				sprintf(buffer, gSearchStatNames[searchstat]);

			i = -1;
			switch (searchstat)
			{
				case OBJ_WALL:
				case OBJ_MASKED:
					pWall = (!ctrl) ? getCorrectWall(searchwall) : &wall[searchwall];
					changedir = -changedir;
					i = searchwall;
					
					if (ctrl)
					{
						sprintf(buffer2, "panning");
						if (chx)
						{
							// fix wrong panning direction when wall x-flipped and/or bottom swapped.
							if ((pWall->nextwall >= 0 && (wall[pWall->nextwall].cstat & kWallSwap)
								&& (wall[pWall->nextwall].cstat & kWallFlipX)) || (pWall->cstat & kWallFlipX))
									changedir = -changedir;
									
							pWall->xpanning = changechar(pWall->xpanning, changedir, bCoarse, 0);
						}
						
						if (chy) pWall->ypanning = changechar(pWall->ypanning, -changedir, bCoarse, 0);
						
						vlx = pWall->xpanning;
						vly = pWall->ypanning;
					}
					else
					{
						sprintf(buffer2, "repeat");
						if (changedir < 0) step = -step;
						if (chx) pWall->xrepeat = ClipRange(pWall->xrepeat + step, 0, 255);
						if (chy) pWall->yrepeat = ClipRange(pWall->yrepeat + step, 0, 255);
						
						vlx = pWall->xrepeat;
						vly = pWall->yrepeat;
					}
					break;
				case OBJ_SPRITE:
					pSpr = &sprite[searchwall];
					i = searchwall;
					if (alt)
					{
						sprintf(buffer2, "pos");
						if (chx) offsetPos((changedir == 1) ? -step : step, 0, 0, ang, &pSpr->x, &pSpr->y, NULL);
						if (chy) offsetPos(0, (changedir == 1) ? step : -step, 0, ang, &pSpr->x, &pSpr->y, NULL);
						
						vlx = pSpr->x;
						vly = pSpr->y;
					}
					else if (ctrl)
					{
						sprintf(buffer2, "offset");
						if ((pSpr->cstat & kSprRelMask) != 48)
						{
							if (chx)
							{
								j = -changedir;
								if (pSpr->cstat & kSprFlipX)
									j = -j;
								
								pSpr->xoffset = changechar(pSpr->xoffset, j, bCoarse, 0);
							}
							
							if (chy)
							{
								j = changedir;
								if ((pSpr->cstat & kSprRelMask) != kSprFace) // fix wrong offset direction when sprite flipped
								{
									if (pSpr->cstat & kSprFlipY)
										j = -j;
								}
								
								pSpr->yoffset = changechar(pSpr->yoffset, j, bCoarse, 0);
								
							}
						}
						
						vlx = pSpr->xoffset;
						vly = pSpr->yoffset;
					}
					else
					{
						sprintf(buffer2, "repeat");
						if (changedir < 0) step = -step;
						if (chx) pSpr->xrepeat = ClipRange(pSpr->xrepeat + step, 2, 255);
						if (chy) pSpr->yrepeat = ClipRange(pSpr->yrepeat + step, 2, 255);
						
						vlx = pSpr->xrepeat;
						vly = pSpr->yrepeat;
					}
					break;
				case OBJ_FLOOR:
				case OBJ_CEILING:
					pSect = &sector[searchsector];
					sprintf(buffer2, "panning");
					i = searchsector;
					if (searchstat == OBJ_FLOOR)
					{
						if (chx) pSect->floorxpanning = changechar(pSect->floorxpanning, changedir, bCoarse, 0);
						if (chy) pSect->floorypanning = changechar(pSect->floorypanning, changedir, bCoarse, 0);
						
						vlx = pSect->floorxpanning;
						vly = pSect->floorypanning;
					}
					else
					{
						if (chx) pSect->ceilingxpanning = changechar(pSect->ceilingxpanning, changedir, bCoarse, 0);
						if (chy) pSect->ceilingypanning = changechar(pSect->ceilingypanning, changedir, bCoarse, 0);
						
						vlx = pSect->ceilingxpanning;
						vly = pSect->ceilingypanning;
					}
					
					if (isSkySector(searchsector, searchstat))
					{
						i = -2; // show just Sky messages!
						if (chy)
							Sky::SetPan(searchsector, searchstat, 0, vly, alt);
						
						if (chx)
						{
							Sky::FixPan(searchsector, searchstat,  alt); 	// fix panning first
							Sky::Rotate((changedir < 0) ? 0 : 1); 			// rotate global sky
						}
					}
					break;
			}
			
			if (i >= 0) scrSetMessage("%s #%d x%s: %d  y%s: %d", strlwr(buffer), i, buffer2, vlx, buffer2, vly);
			Beep(i >= 0 || i == -2);
			break;
		}
		case KEY_F11:
			if (!alt)
			{
				if (in2d)
				{
					i = IncRotate(gScreen2D.prefs.showMap, 3);
					scrSetMessage("Show 2D map: %s", gShowMapNames[i]);
					gScreen2D.prefs.showMap = i;
					Beep(i);
					break;
				}
				
				return FALSE;
			}
			
			if (ctrl)
			{
				gHudPrefs.fontPack = IncRotate(gHudPrefs.fontPack, kHudFontPackMax);
				gMapedHud.SetMsgImp(128, "HUD font: %s", gHudFontNames[gHudPrefs.fontPack]);
				hudSetFontPack(&gMapedHud, gHudPrefs.fontPack, &gMouse);
				BeepOk();
			}
			else
			{
				while( 1 )
				{
					if (ED23)
					{
						strcpy(buffer, "SPLIT");
						gHudPrefs.layoutSPLIT = IncRotate(gHudPrefs.layoutSPLIT, kHudLayoutMax);
						if (gHudPrefs.layoutSPLIT == kHudLayoutDynamic) continue;
						else hudSetLayout(&gMapedHud, gHudPrefs.layoutSPLIT, &gMouse);
						i = gHudPrefs.layoutSPLIT;
					}
					else if (in2d)
					{
						strcpy(buffer, "2D");
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
						strcpy(buffer, "3D");
						gHudPrefs.layout3D = IncRotate(gHudPrefs.layout3D, kHudLayoutMax);
						if (gHudPrefs.layout3D == kHudLayoutDynamic) continue;
						else hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);
						i = gHudPrefs.layout3D;
					}
					
					gMapedHud.SetMsgImp(128, "HUD layout (%s Mode): %s", buffer, gHudLayoutNames[i]);
					Beep(i > 0);
					break;
				}
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
			else if (in2d) gScreen2D.GetPoint(searchx, searchy, &x1, &y1), x2 = x1, y2 = y1;
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
			switch (searchstat)
			{
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
			if (i)
			{
				if (ED23)		hudSetLayout(&gMapedHud, gHudPrefs.layoutSPLIT, &gMouse);
				else if (ED2D)	hudSetLayout(&gMapedHud, gHudPrefs.layout2D, &gMouse);
				else			hudSetLayout(&gMapedHud, gHudPrefs.layout3D, &gMouse);
			}
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
				hgltIsolateRorMarkers(i); hgltIsolatePathMarkers(i);
				hgltIsolateChannels(i);
				BeepOk();
				
				scrSetMessage("Objects isolated.");
				break;
			}
			return FALSE;
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
				else j = hgltSectDelete();

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
			if (ctrl) return FALSE;
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
		char text[MAXVALIDMODES][16]; NAMED_TYPE modes[MAXVALIDMODES];
		int i, j, k, len, x, y, w, h, half, fh = pFont->height;
		int dw = 280, dh = 130, btw = 80, bth = 20;
		int pad = 8, bw, bh;
		BOOL reallyLow = FALSE;
		half = (bth>>1)+(fh>>1);

		Window dialog(0, 0, dw, dh, "Options");
		FieldSet* pField1 = new FieldSet(pad, pad, dw-(pad*3), (bth<<1)-(pad>>1), "RESOLUTION");
		TextButton* pVideoChange = new TextButton(pad, pad, btw, bth, "Chan&ge...", 100);
		
		Checkbox* pFullscreen = new Checkbox(btw+(pad<<1), half, fullscreen, "&Fullscreen", 101);
		sprintf(buffer, "%dx%d", xdim, ydim); len = gfxGetTextLen(buffer, pFont);
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
						//for (j = 0; j < i && stricmp(buffer, text[j]); j++); // we don't need duplicates
						//if (j >= i)
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
					xdimgame = xdim, ydimgame = ydim;
					if (w != xdim || h != ydim)
						Alert("Failed to set resolution: %dx%d!", w, h);
					
					if (!Confirm("Keep %dx%d resolution?", xdim, ydim))
					{
						toggleResolution(pFullscreen->checked, bw, bh); splashScreen();
						xdimgame = bw, ydimgame = bh;
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
	if (!fileExists(gPaths.maps, &hFile))
	{
		pReloadBoard->fontColor = kColorDarkGray;
		pReloadBoard->disabled  = TRUE;
		pReloadBoard->canFocus  = FALSE;
	}

	if (!gMapLoaded)
	{
		pSaveBoard->fontColor = pSaveBoardAs->fontColor = pReloadBoard->fontColor = kColorDarkGray;
		pSaveBoard->disabled  = pSaveBoardAs->disabled  = pReloadBoard->disabled  = TRUE;
		pSaveBoard->canFocus  = pSaveBoardAs->canFocus  = pReloadBoard->canFocus  = FALSE;
	}

	dialog.Insert(pSaveBoard);
	dialog.Insert(pSaveBoardAs);
	dialog.Insert(pReloadBoard);
	dialog.Insert(new TextButton(44, y, 84,   bh, "&Load board", mrLoad));						y+=bh;
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
	TextButton* pDoorWizard    = new TextButton(x, y, 124,  bh, "&Door wizard", mrToolDoorWizard);			y+=bh;
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

	if (!gMapLoaded || qsetmode == 200)
	{
		pDoorWizard->fontColor 	= kColorDarkGray;
		pDoorWizard->disabled 	= TRUE;
		pDoorWizard->canFocus 	= FALSE;
	}
	
	if (!gMapLoaded || qsetmode != 200)
	{
		pSpriteText->fontColor 	= kColorDarkGray;
		pSpriteText->disabled 	= TRUE;
		pSpriteText->canFocus 	= FALSE;
	}
	
	dialog.height = y + (pd*6);
	dialog.Insert(pPreviewMode);
	dialog.Insert(pDoorWizard);
	dialog.Insert(pSpriteText);
	dialog.Insert(pExplodeSeq);
	dialog.Insert(pCleanChannel);
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
			case mrToolDoorWizard:
				if (dlgDoorWizard()) return result;
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
				gMapLoaded = TRUE;
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
						if ((filename = browseSave(gPaths.maps, ".map")) != NULL)
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
					if ((filename = browseOpen(gPaths.maps, ".map", "Load board")) == NULL)
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
							
							if (irngok(wall[i].type, kWallStack-1, kWallStack))
							{
								if (CheckLinkCamera(&posx, &posy, &posz, &i, 1))
									cursectnum = i;
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
				if ((nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect, 0)) == 0)
				{
					legs = posz - val;
					if ((zmode == 0 && keystatus[KEY_A]) || zmode != 0)
						nLink = CheckLinkCamera(&posx, &posy, &legs, &nSect, 0);
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
	
	switch (key)
	{
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

void setStartPos(int x, int y, int z, int ang)
{
	updatesector(x, y, &startsectnum);
	z = (startsectnum >= 0) ? getflorzofslope(startsectnum, x, y) : 0;
	
	startposx = x;
	startposy = y;
	startposz = z - kensplayerheight;
	startang  = ang;
	
	if (gMisc.forceEditorPos)
	{
		spritetype* pSpr; XSPRITE* pXSpr;
		int i;
		
		for (i = headspritestat[0]; i >= 0; i = nextspritestat[i])
		{
			pSpr = &sprite[i];
			if (pSpr->type == kMarkerSPStart && pSpr->extra > 0)
			{
				pXSpr = &xsprite[pSpr->extra];
				if (pXSpr->data1 == 0)
				{
					pSpr->ang	= startang;
					pSpr->x 	= startposx;
					pSpr->y 	= startposy;
					pSpr->z 	= startposz;
					
					if (startsectnum >= 0)
						ChangeSpriteSect(pSpr->index, startsectnum);
					
					clampSprite(pSpr);
					break;
				}
			}
		}
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
			sectClone(sect, kMaxSectors - highlightsectorcnt + i, k, 0);
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
	dbLoadMap(pIo, (gCmtPrefs.enabled) ? mapname : NULL);
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
			sectClone(kMaxSectors-highlightsectorcnt+i, numsectors, numwalls, 0);
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
	if (gModernMap && gMisc.showTypes != MO)
	{
		gMisc.showTypes = MO;
		initNames();
	}
	
	for (i = 0; i < kMaxWalls; i++)
		gNextWall[i] = -1;
	
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
	worldSprCallFunc(sprFixSector);
	
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
	
	if (pGDoorSM)	DELETE_AND_NULL(pGDoorSM);
	if (pGDoorR)	DELETE_AND_NULL(pGDoorR);
	if (pGCircleW)  DELETE_AND_NULL(pGCircleW);
	if (pGLShape)  	DELETE_AND_NULL(pGLShape);
	if (pGLBuild)  	DELETE_AND_NULL(pGLBuild);
	
	memset(gMapInfoStr, 0, sizeof(gMapInfoStr));
	gCommentMgr.DeleteAll();
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
				if (pGDoorSM)	DELETE_AND_NULL(pGDoorSM);
				if (pGDoorR)	DELETE_AND_NULL(pGDoorR);
				if (pGCircleW)  DELETE_AND_NULL(pGCircleW);
				if (pGLShape)  	DELETE_AND_NULL(pGLShape);
				if (pGLBuild)  	DELETE_AND_NULL(pGLBuild);
				
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