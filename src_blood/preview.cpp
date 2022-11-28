/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A libraray of functions for Preview mode feature.
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

#include "xmpconf.h"
#include "xmpstub.h"
#include "edit2d.h"
#include "common_game.h"
#include "build.h"
#include "gfx.h"
#include "gui.h"
#include "gameutil.h"
#include "screen.h"
#include "preview.h"
#include "grdshd.h"
#include "seq.h"
#include "aadjust.h"
#include "sectorfx.h"
#include "tile.h"
#include "xmpmisc.h"
#include "editor.h"
#include "hglt.h"

PREVIEW_MODE gPreview;
static PREVIEW_MODE_KEYS gPreviewKeys[] = {
	
	{KEY_ESC,			FALSE, FALSE, FALSE,			3},
	{KEY_HOME,			FALSE, FALSE, FALSE,			3},
	{KEY_UP,			FALSE, FALSE, FALSE,			3},
	{KEY_LEFT,			FALSE, FALSE, FALSE,			3},
	{KEY_DOWN,			FALSE, FALSE, FALSE,			3},
	{KEY_RIGHT,			FALSE, FALSE, FALSE,			3},
	{KEY_A,				FALSE, FALSE, FALSE,			3},
	{KEY_Z,				FALSE, FALSE, FALSE,			3},
	{KEY_PADENTER,		FALSE, FALSE, FALSE,			3},
	{KEY_CAPSLOCK,		FALSE, FALSE, FALSE,			3},
	{KEY_TILDE,			FALSE, FALSE, FALSE,			3},
	{KEY_RSHIFT,		FALSE, FALSE, FALSE,			3},
	{KEY_LSHIFT,		FALSE, FALSE, FALSE,			3},
	{KEY_RALT,			FALSE, FALSE, FALSE,			3},
	{KEY_LALT,			FALSE, FALSE, FALSE,			3},
	{KEY_RCTRL,			FALSE, FALSE, FALSE,			3},
	{KEY_LCTRL,			FALSE, FALSE, FALSE,			3},
	{KEY_PRINTSCREEN,	FALSE, FALSE, FALSE,			3},
	{KEY_ENTER,			FALSE, FALSE, FALSE,			2},
	{KEY_F12,			FALSE, FALSE, FALSE,			3},
	{KEY_F7,			FALSE, FALSE, FALSE,			1},
	{KEY_F8,			FALSE, FALSE, FALSE,			1},
	{KEY_TAB,			FALSE, FALSE, TRUE,				2},
	{KEY_T,				FALSE, TRUE,  FALSE,			1},
	{KEY_E,				FALSE, FALSE, FALSE,			3},
	{KEY_I,				FALSE, TRUE, FALSE,				2},
	{KEY_B,				FALSE, TRUE, FALSE,				3},
	{KEY_SCROLLLOCK, 	FALSE, FALSE, FALSE,			3},
	{KEY_G,				FALSE, FALSE, FALSE,			1},
	{KEY_S,				FALSE, FALSE, FALSE,			2},
	{KEY_PAD5,			FALSE, FALSE, FALSE,			2},
	
};

NAMED_TYPE gDifficNames[6] = {

	{0,						"Still Kicking"},
	{1,						"Pink On The Inside"},
	{2,						"Lighty Broiled"},
	{3,						"Well Done"},
	{4,						"Extra Crispy"},
	{5,						"All"},
	
};

NAMED_TYPE gGameNames[5] = {

	{kGameModeSingle,		"Single"},
	{kGameModeCoop,			"Cooperative"},
	{kGametModeDeathmatch,	"BloodBath"},
	{kGameModeFlags,		"Teams"},
	{4,						"All"},
	
};

void PREVIEW_MODE::Init(IniFile* pIni, char* section)
{
	triggerFlags		= pIni->GetKeyBool(section, "TriggerFlagsRequired", 0);
	forceStartPos 		= pIni->GetKeyBool(section, "ForcePlayerStartPosition", 0);
	triggerStart		= pIni->GetKeyBool(section, "TriggerStartChannel", 1);
	translateObjects	= pIni->GetKeyBool(section, "TranslateObjects", 0);
	enableSound			= pIni->GetKeyBool(section, "EnableSound", 1);
	enableMusic			= pIni->GetKeyInt(section, "EnableMusic", 0);
	explosionType		= ClipHigh(pIni->GetKeyInt(section, "ExplosionType", 1), kExplosionMax);
	missileType			= ClipHigh(pIni->GetKeyInt(section, "MissileType", 5), kMissileMax-kMissileBase-1);
	
	speed				= ClipHigh(pIni->GetKeyInt(section, "Speed", 4), 128);
	m1cmd				= ClipHigh(pIni->GetKeyInt(section, "Mouse1Command", 2), 255);
	m2cmd				= ClipHigh(pIni->GetKeyInt(section, "Mouse2Command", 0), 255);
	m3cmd				= ClipHigh(pIni->GetKeyInt(section, "Mouse3Command", 1), 255);
	difficulty			= ClipHigh(pIni->GetKeyInt(section, "DefaultDifficulty", 5), 5);
	mode				= ClipHigh(pIni->GetKeyInt(section, "DefaultGameMode", 4), 4);
	
	sprintf(gPaths.episodeIni, pIni->GetKeyString(section, "EpisodeIni", "BLOOD.INI"));
	pEpisode			= new IniFile(gPaths.episodeIni);
}

void PREVIEW_MODE::Save(IniFile* pIni, char* section)
{
	pIni->PutKeyInt(section, "DefaultDifficulty", difficulty);
	pIni->PutKeyInt(section, "DefaultGameMode", mode);
	pIni->PutKeyInt(section, "ExplosionType", explosionType);
	pIni->PutKeyInt(section, "MissileType", missileType);
	
	pIni->PutKeyString(section, "EpisodeIni", gPaths.episodeIni);
}

int previewMenuDifficulty() {
	
	char cskill[LENGTH(gDifficNames)][32]; char* skill[LENGTH(gDifficNames)];
	
	memset(cskill, 0, sizeof(cskill));
	for (int i = 0; i < LENGTH(gDifficNames); i++)
	{
		if (gDifficNames[i].id == gPreview.difficulty) sprintf(cskill[i], "*%s*", gDifficNames[i].name);
		else sprintf(cskill[i], gDifficNames[i].name);
		skill[i] = cskill[i];
	}
	
	return showButtons(skill, LENGTH(gDifficNames), "Difficulty");
}

int previewMenuGameMode() {
	
	char cmode[LENGTH(gGameNames)][32]; char* mode[LENGTH(gGameNames)];
	
	memset(cmode, 0, sizeof(cmode));
	for (int i = 0; i < LENGTH(gGameNames); i++)
	{
		if (gGameNames[i].id == gPreview.mode) sprintf(cmode[i], "*%s*", gGameNames[i].name);
		else sprintf(cmode[i], gGameNames[i].name);
		mode[i] = cmode[i];
	}
	
	return showButtons(mode, LENGTH(gGameNames), "Game mode");
}

BOOL previewMenuProcess() {
	
	int diff = gPreview.difficulty, mode = gPreview.mode;
	
	while ( 1 )
	{
		if ((diff = previewMenuDifficulty()) < mrUser) break;
		else if ((mode = previewMenuGameMode()) >= mrUser)
		{
			diff -= mrUser; mode -= mrUser;
			if (diff != gPreview.difficulty || mode != gPreview.mode)
			{
				gPreview.difficulty = diff; gPreview.mode = mode;
				if (gPreviewMode)
					previewStop();
			}
			
			return TRUE;
		}
	}
	
	return FALSE; // pressed ESC
}

BOOL previewCheckKey(int key) {
	for (int i = 0; i < LENGTH(gPreviewKeys); i++) {
		if (gPreviewKeys[i].key != key) continue;
		else if (gPreviewKeys[i].reqShift && !shift) return FALSE;
		else if (gPreviewKeys[i].reqCtrl  && !ctrl)	 return FALSE;
		else if (gPreviewKeys[i].reqAlt	  && !alt) return FALSE;
		else if (gPreviewKeys[i].mode >= 3) return TRUE;
		else if (qsetmode == 200) return (BOOL)(gPreviewKeys[i].mode & 0x02);
		else if (gPreviewKeys[i].mode & 0x01)
			return TRUE;
	}
	
	return FALSE;
}

BOOL previewProcessKeys() {
	
	switch (key) {
		case KEY_ESC:
		case KEY_HOME:
			previewStop();
			break;
		case KEY_ENTER:
			keystatus[key] = 0;
			key = KEY_CAPSLOCK;
			return FALSE;
		case KEY_S:
			previewSpawnSeq();
			break;
		case KEY_E:
			if (shift & 0x02)
			{
				sprintf(buffer, "Set explosion type (%d-%d)", 0, kExplosionMax);
				gPreview.explosionType = GetNumberBox(buffer, gPreview.explosionType, gPreview.explosionType);
				gPreview.explosionType = ClipRange(gPreview.explosionType, 0, kExplosionMax);
				BeepOk();
				break;
			}
			if (previewDoExplode(gPreview.explosionType)) break;
			BeepFail();
			break;
		case KEY_F:
			if (shift & 0x02)
			{
				sprintf(buffer, "Set missile type (%d-%d)", 0, kMissileMax-kMissileBase-1);
				gPreview.missileType = GetNumberBox(buffer, gPreview.missileType, gPreview.missileType);
				gPreview.missileType = ClipRange(gPreview.missileType, 0, kMissileMax-kMissileBase-1);
				BeepOk();
				break;
			}
			if (previewFireMissile(kMissileBase+gPreview.missileType)) break;
			BeepFail();
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}

void previewSaveState()
{
	AutoAdjustSprites();
	CleanUp();
	
	memcpy(cpysector, sector, sizeof(sector));	memcpy(cpyxsector, xsector, sizeof(xsector));
	memcpy(cpywall,   wall,   sizeof(wall));	memcpy(cpyxwall,   xwall,   sizeof(xwall));
	memcpy(cpysprite, sprite, sizeof(sprite));	memcpy(cpyxsprite, xsprite, sizeof(xsprite));
	
	gPreview.oVisibility = visibility;
	gPreview.modernMap = gModernMap;
}

void previewLoadState() {

	int i;
	memcpy(sector, cpysector, sizeof(sector));	memcpy(xsector, cpyxsector, sizeof(xsector));
	memcpy(wall,   cpywall,   sizeof(wall));	memcpy(xwall,   cpyxwall,   sizeof(xwall));
	
	for (i = 0; i < kMaxSprites; i++)
	{
		int nStat  = sprite[i].statnum;
		int nStat2 = cpysprite[i].statnum;
		int nSect  = sprite[i].sectnum;
		int nSect2 = cpysprite[i].sectnum;
		
		if (nStat != kStatFree)
		{
			if (nStat2 == kStatFree && nStat != kStatFree)
			{ // delete created sprites
				DeleteSprite(i);
				continue;
			}

			if (nStat2 != nStat) ChangeSpriteStat(i, nStat2);
			if (nSect2 != nSect) ChangeSpriteSect(i, nSect2);
			
			sprite[i] = cpysprite[i];
			if (sprite[i].extra > 0)
				xsprite[sprite[i].extra] = cpyxsprite[sprite[i].extra];
		}
	}
	
	// puts free sprites with lower indexes to tail of headspritestat array
	// generally required for FS, but also helps pqueue to not desync in some maps
	// for unknown reason
	resortFree(); 
	updatenumsprites();
	CleanUp();
	
	visibility = gPreview.oVisibility;
	gModernMap = (BOOL)gPreview.modernMap;
}

void previewInitGameLite() {
	
	int i; BOOL found = FALSE;
	
	memset(gSpriteHit, 0, sizeof(gSpriteHit));
	memset(xvel, 0, sizeof(xvel));	memset(yvel, 0, sizeof(yvel));
	memset(zvel, 0, sizeof(zvel));
	
	for (i = 0; i < kMaxSprites; i++)
	{
		spritetype *pSprite = &sprite[i];
		if (pSprite->extra <= 0 || pSprite->statnum >= kMaxStatus)
			continue;
		
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		if (pXSprite->rxID == 60 && pXSprite->txID == pXSprite->rxID && pXSprite->command == 100)
			gModernMap = TRUE;
				
		if ((pXSprite->lSkill & (1 << gPreview.difficulty)))
		{
			hideSprite(pSprite);
			continue;
		}
		else if (gPreview.mode <= kGameModeFlags)
		{
			if ((pXSprite->lS && gPreview.mode == kGameModeSingle)
				|| (pXSprite->lB && gPreview.mode == kGametModeDeathmatch)
				|| (pXSprite->lT && gPreview.mode == kGameModeFlags)
				|| (pXSprite->lC && gPreview.mode == kGameModeCoop)) {
					hideSprite(pSprite);
					continue;
			}
		}
		
		switch (pSprite->type) {
			case 1:
				if (gPreview.forceStartPos && pXSprite->data1 == 0) {
					posx = pSprite->x;
					posy = pSprite->y;
					posz = pSprite->z;
					ang = pSprite->ang;
					cursectnum = pSprite->sectnum;
					vel = svel = 0;
					found = TRUE;
				}
				// no break
			case 2:
				hideSprite(pSprite);
				break;
			case kTrapExploder:
				pXSprite->state = 0;
				pXSprite->waitTime = ClipLow(pXSprite->waitTime, 1);
				break;
			case kTrapMachinegun:
			//case kModernThingTNTProx:
				pXSprite->state = 0;
				// no break;
			case kTrapFlame:
				pSprite->picnum = 2178;
				break;
			case kDudeGargoyleStatueFlesh:
			case kDudeGargoyleStatueStone:
			case kThingArmedProxBomb:
				pXSprite->state = 0;
				break;
			case kGenSound:
			case kSoundSector:
			case kSoundAmbient:
				pSprite->xrepeat = 0;
				break;
			default:
				if (pSprite->statnum == kStatThing || pSprite->statnum == kStatDude) pXSprite->state = 1;
				break;
		}
		
		if (IsDudeSprite(pSprite))
		{
			pSprite->cstat |= kSprBlock;
			pXSprite->health = 100 << 4;
			
			switch (pSprite->type) {
				case kDudeGargoyleFlesh:
				case kDudeGargoyleStone:
				case kDudeBat:
				case kDudePhantasm:
					break;
				case kDudePodGreen:
				case kDudePodFire:
				case kDudeTentacleGreen:
				case kDudeTentacleFire:
					if (gModernMap && (pSprite->cstat & kSprFlipY)) break;
					pSprite->flags = kPhysGravity|kPhysFalling|kPhysMove;
					break;
				case kDudeSpiderBrown:
				case kDudeSpiderBlack:
				case kDudeSpiderMother:
				case kDudeSpiderRed:
					pSprite->cstat &= ~kSprFlipY;
					// no break
				default:
					pSprite->flags = kPhysGravity|kPhysFalling|kPhysMove;
					break;
			}
			
			gPreview.kills.total++;
		}
		else if (IsThingSprite(pSprite))
		{
			int nType = pSprite->type - kThingBase;
			pXSprite->health = thingInfo[nType].startHealth << 4;
			if (!gModernMap)
				pSprite->clipdist = thingInfo[nType].clipdist;
			
			pSprite->flags = thingInfo[nType].flags;
			if (pSprite->flags & kPhysGravity)
				pSprite->flags |= kPhysFalling;
		}
	}
	
	if (gModernMap)
		nnExtInitModernStuff();
	else
		nnExtResetGlobals();
	
	if (gPreview.forceStartPos && !found && startsectnum >= 0)
	{
		posx = startposx;
		posy = startposy;
		posz = startposz;
		ang = startang;
		cursectnum = startsectnum;
	}
	
	if (cursectnum >= 0 && sector[cursectnum].extra > 0)
	{
		XSECTOR* pXSect = &xsector[sector[cursectnum].extra];
		if (pXSect->underwater)
			scrSetPalette(kPal1);
	}
	
}

void previewStart() {
	
	gPreviewMode = TRUE;
	grshUnhgltObjects(-1, FALSE);
	previewSaveState();
	
	updatesector(posx, posy, &cursectnum);
	gPreview.palette = kPal0;
	gPostCount = 0;
	
	memset(gPreview.scrEffects, 0, sizeof(gPreview.scrEffects));
	memset(show2dsprite, 	0, sizeof(show2dsprite));
	memset(show2dwall, 		0, sizeof(show2dwall));
	memset(show2dsector, 	0, sizeof(show2dsector));
	automapping = true;
	
	previewInitGameLite();
	if (gPreview.enableSound)
	{
		sfxInit();
		ambInit();
	}
	
	warpInit();
	evInit();
	InitMirrors();
	
	if (gPreview.enableMusic)
	{
		char* songName;
		if ((songName = gPreview.pEpisode->GetKeyString(getFilename(gPaths.maps, buffer2), "Song", "")) != NULL)
			sndPlaySong(songName, TRUE);
	}
	
	//gFrameClock = 0;
	trInit();

	asksave = FALSE;
	gHighSpr = -1;

	gPreview.sectnum = cursectnum;
	
	gMouse.VelocityReset();
	gMapedHud.SetTile(-1, -1, -1);
	
	memset(&gPreview.kills,   0, sizeof(gPreview.kills));
	memset(&gPreview.secrets, 0, sizeof(gPreview.secrets));
	
	gPreview.levelTime = 0;
	
	memset(gScreen.msg, 0, sizeof(gScreen.msg));
	gScreen.msgShowTotal = 16;
	
	if (gMisc.externalModels == 2) usevoxels = 1;
	previewMessage("Game mode: %s", gGameNames[gPreview.mode].name);
	previewMessage("Difficulty: %s.", gDifficNames[gPreview.difficulty].name);
	previewMessage("Preview mode enabled.");
	BeepOk();
	
}

void previewProcess() {
	
	int i = 0;
	previewShowStatus();
	if (totalclock >= gPreview.ticks + gPreview.speed)
	{
		gPreview.ticks = totalclock;
		
		if (gPreview.scrEffects[kScrEffectQuake2])
			gPreview.scrEffects[kScrEffectQuake2] = 0;

		for (i = 0; i < kScrEffectMax; i++)
			gPreview.scrEffects[i] = ClipLow(gPreview.scrEffects[i] - 4, 0);
			
		trProcessBusy();
		evProcess(gFrameClock);
		seqProcess(gPreview.speed);
		
		actProcessSprites();
		actPostProcess();
		
		ambProcess();
		sfxUpdate3DSounds();
		
		gPreview.levelTime++;
	}
	
	previewCheckPickup();
	previewGetTriggerObject();
}

void previewStop() {
	
	if (gPreview.enableSound)
	{
		sndKillAllSounds();
		sfxKillAllSounds();
		ambKillAll();
	}
	
	if (gPreview.enableMusic)
		sndStopSong();
	
	seqKillAll();
	previewLoadState();
	
	gPreviewMode = FALSE;
	gTimers.autosave = gFrameClock;
	if (!gResetHighlight)
		grshHgltObjects(-1);
	
	RestoreMirrorPic();
	
	gPreview.palette = kPal0;
	scrSetPalette(kPal0);

	updatesector(posx, posy, &cursectnum);
	if (gMisc.externalModels == 2)
		usevoxels = 0;
	
	memset(show2dsprite, 	0, sizeof(show2dsprite));
	memset(show2dwall, 		0, sizeof(show2dwall));
	memset(show2dsector, 	0, sizeof(show2dsector));
	automapping = false;
	
	int i, s, e;
	for (i = 0; i < numsectors; i++)
	{
		getSectorWalls(i, &s, &e);
		while(s <= e)
		{
			// restore wall highlight effect
			if (hgltCheck(OBJ_WALL, s) >= 0)
				SetBitString(show2dwall, s);
			
			s++;
		}
		
		for (s = headspritesect[i]; s >= 0; s = nextspritesect[s])
		{
			// restore sprite highlight effect
			if (hgltCheck(OBJ_SPRITE, s) >= 0)
				SetBitString(show2dsprite, s);
		}
	}
	
	// free lists
	gProxySpritesList.Free();
	gSightSpritesList.Free();
	gPhysSpritesList.Free();
	
	gScreen.msgShowTotal = 1;
	gMapedHud.SetTile(-1, -1, -1);
	previewMessage("Preview mode disabled.");
	BeepFail();

}

void previewShowStatus() {
	
	QFONT* pFont = qFonts[0];
	int tcol = kColorWhite, bcol = kColorRed ^ h, i;
	i = sprintf(buffer,"<< PREVIEW MODE >>");
	
	gfxSetColor(gStdColor[bcol]);
	int len = gfxGetTextLen(buffer, pFont) + 4;
	gfxFillBox(xdim - len - 4, 2, xdim - 2, 14);
	gfxDrawText(xdim - len, 4, gStdColor[tcol], buffer, pFont);
		
	i = sprintf(buffer, "%s / %s", gGameNames[gPreview.mode].name, gDifficNames[gPreview.difficulty].name) << 2;
	printextShadowS(xdim - i - 2, 14, gStdColor[kColorLightGray ^ h], strupr(buffer));
	

	
}

int previewCheckTriggerFlags(int type, int idx) {
	
	int retn = 0;
	switch(type) {
		case OBJ_FLOOR:
		case OBJ_CEILING: {
			if (sector[idx].extra <= 0) return -2;
			
			XSECTOR* pXObj = &xsector[sector[idx].extra];
			if (pXObj->locked) return -3;
			else if (pXObj->isTriggered) return -4;
			
			if (!gPreview.triggerFlags)
				break;
			
			if (sector[idx].type == kSectorCounter) retn = kCmdCallback;
			else if (pXObj->triggerPush || pXObj->triggerWallPush) retn = kCmdSectorPush;
			else if (pXObj->triggerEnter) retn = kCmdSectorEnter;
			else if (pXObj->triggerExit) retn = kCmdSectorExit;
			else retn = -1;
		}
		break;
		case OBJ_SPRITE: {
			if (sprite[idx].extra <= 0) return -2;
			
			XSPRITE* pXObj = &xsprite[sprite[idx].extra];
			if (pXObj->locked) return -3;
			else if (pXObj->isTriggered) return -4;
			
			if (!gPreview.triggerFlags) break;
			else if (pXObj->triggerPush) retn = kCmdSpritePush;
			else if (pXObj->triggerVector) retn = kCmdSpriteImpact;
			else if (pXObj->triggerImpact) retn = kCmdSpriteExplode;
			else if (pXObj->triggerPickup && sprite[xsprite[sprite[idx].extra].reference].statnum == kStatItem) retn = kCmdSpritePickup;
			else if (pXObj->triggerTouch) retn = kCmdSpriteTouch;
			else if (pXObj->triggerProximity) retn = kCmdSpriteProximity;
			else if (pXObj->triggerSight) retn = kCmdSpriteSight;
			else retn = -1;
		}
		break;
		case OBJ_WALL:
		case OBJ_MASKED: {
			if (wall[idx].extra <= 0) return -2;
			
			XWALL* pXObj = &xwall[wall[idx].extra];
			if (pXObj->locked) return -3;
			else if (pXObj->isTriggered) return -4;
			
			if (!gPreview.triggerFlags) break;
			else if (pXObj->triggerPush) retn = kCmdWallPush;
			else if (pXObj->triggerVector) retn = kCmdWallImpact;
			else if (pXObj->triggerTouch) retn = kCmdWallPush;
			else retn = -1;
		}
		break;
	}
	
	return retn;
}

void previewCheckPickup()
{
    int x = posx;
    int y = posy;
    int z = posz;
    int nSector = cursectnum;
    int nNextSprite;
    for (int nSprite = headspritestat[kStatItem]; nSprite >= 0; nSprite = nNextSprite) {
        spritetype *pItem = &sprite[nSprite];
        nNextSprite = nextspritestat[nSprite];
        if (pItem->flags&32)
            continue;
        int dx = klabs(x-pItem->x)>>4;
        if (dx > 48)
            continue;
        int dy = klabs(y-pItem->y)>>4;
        if (dy > 48)
            continue;
        int top = posz, bottom = posz + kensplayerheight;
        int vb = 0;
        if (pItem->z < top)
            vb = (top-pItem->z)>>8;
        else if (pItem->z > bottom)
            vb = (pItem->z-bottom)>>8;
        if (vb > 32)
            continue;
        if (approxDist(dx,dy) > 48)
            continue;
        GetSpriteExtents(pItem, &top, &bottom);
        if (cansee(x, y, z, nSector, pItem->x, pItem->y, pItem->z, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, top, pItem->sectnum)
         || cansee(x, y, z, nSector, pItem->x, pItem->y, bottom, pItem->sectnum))
            previewPickupItem(pItem, (pItem->extra > 0) ? &xsprite[pItem->extra] : NULL);
    }
}

void previewPickupItem(spritetype* pSprite, XSPRITE* pXSprite) {
	
	int nType = pSprite->type;
	switch(nType)
	{
		case kItemFlagABase:
		case kItemFlagBBase:
			return;
		default:
			if (!rngok(nType, kItemBase, kItemMax))
			{
				if (!irngok(nType, kItemWeaponBase, kItemWeaponMax))
				{
					if (!rngok(nType, kItemAmmoBase, kItemAmmoMax))
						return;
				}
			}
			break;
	}
	
	previewMessage("Picked up %s", gSpriteNames[pSprite->type]);
	if (pXSprite && pXSprite->triggerPickup)
		trTriggerSprite(pSprite->index, pXSprite, kCmdSpritePickup);
	
	hideSprite(pSprite);
	BeepOk();
}

char previewMorphDude(spritetype* pSprite, XSPRITE* pXSprite) {
	
	int otype = pSprite->type, type = -1;
	
	switch (pSprite->type) {
		case kDudeZombieAxeLaying:
		case kDudeZombieAxeBuried:
			type = pSprite->type = kDudeZombieAxeNormal;
			break;
		case kDudeGargoyleStatueFlesh:
			type = pSprite->type = kDudeGargoyleFlesh;
			break;
		case kDudeGargoyleStatueStone:
			type = pSprite->type = kDudeGargoyleStone;
			break;
		case kDudeCerberusTwoHead:
			type = pSprite->type = kDudeCerberusOneHead;
			break;
		case kDudeCultistBeast:
			type = pSprite->type = kDudeBeast;
			break;
		case kDudeModernCustom:
			spritetype* pIncarnation = cdGetNextIncarnation(pSprite);
			if (pIncarnation == NULL) break;
			otype = pSprite->type;
			type = pIncarnation->type;
			cdTransform(pSprite);
			break;
	}
	
	if (type >= 0)
	{
		adjSpriteByType(pSprite);
		previewMessage("%s is morphed to %s.", gSpriteNames[otype], gSpriteNames[type]);
		pSprite->ang = getangle(posx - pSprite->x, posy - pSprite->y) & kAngMask;
		BeepOk();
		return 1;
	}
	
	return 0;
}

void previewKillDude(spritetype* pSprite, XSPRITE* pXSprite) {
	
	if (pXSprite->locked)
	{
		previewMessage("Cannot kill locked %s", gSpriteNames[pSprite->type]);
		return;
	}
	else
	{
		switch (pSprite->type) {
			case kDudeZombieAxeLaying:
			case kDudeZombieAxeBuried:
				break;
			default:
				if (previewMorphDude(pSprite, pXSprite)) return;
				break;
		}
		
		if (pXSprite)
		{
			if (pXSprite->dropItem) actDropObject(pSprite, pXSprite->dropItem);
			if (pXSprite->key) actDropObject(pSprite, pXSprite->key + kItemBase - 1);
			trTriggerSprite(pSprite->index, pXSprite, kCmdOff);
		}
	}
	
	gPreview.kills.done++;
	previewMessage("%s is killed.", gSpriteNames[pSprite->type]);
	if (pSprite->yvel == kDeleteReally) actPostSprite(pSprite->index, kStatFree);
	else hideSprite(pSprite);
	BeepFail();
	
}


void previewDestroyThing(spritetype* pSprite, XSPRITE* pXSprite, int cmd)
{
	if (pXSprite)
		trTriggerSprite(pSprite->index, pXSprite, cmd);
}


void previewGetTriggerObject() {

	int cmd = -1, type = -1, next = -1, idx = -1, xidx = -1, chk = -1;
	if ((vel || svel) && cursectnum >= 0 && gPreview.sectnum != cursectnum)
	{
		XSECTOR* pXSect = NULL;
		if (sector[cursectnum].extra > 0)
		{
			pXSect = &xsector[sector[cursectnum].extra];
			if (pXSect->triggerEnter)
			{
				type = OBJ_FLOOR;
				idx = cursectnum;
				cmd = kCmdSectorEnter;
			}
		}
		
		if (gPreview.sectnum >= 0 && sector[gPreview.sectnum].extra > 0)
		{
			pXSect = &xsector[sector[gPreview.sectnum].extra];
			if (pXSect->triggerExit)
			{
				type = OBJ_FLOOR;
				idx = gPreview.sectnum;
				cmd = kCmdSectorExit;
			}
		}
	}
	
	gPreview.sectnum = cursectnum;
	
	gMouse.ReadButtons();
	BOOL mouse1 = (BOOL)(gMouse.press & 1);
	BOOL mouse2 = (BOOL)((qsetmode == 200) ? (gMouse.press & 2) : (mouse1 && (shift & 0x02)));
	BOOL mouse3 = (BOOL)((qsetmode == 200) ? (gMouse.press & 4) : (mouse1 && (ctrl & 0x02)));
	if (cmd < 0)
	{
		if (mouse1)		 cmd = gPreview.m1cmd;
		else if (mouse2) cmd = gPreview.m2cmd;
		else if (mouse3) cmd = gPreview.m3cmd;
	}

	if (cmd >= 0)
	{
		if (type < 0)
		{
			getHighlightedObject(); type = searchstat;
			switch (searchstat) {
				case OBJ_SPRITE:
				case OBJ_WALL:
				case OBJ_MASKED:
					idx = searchwall;
					break;
				case OBJ_FLOOR:
				case OBJ_CEILING:
					idx = searchsector;
					break;
				default:
					type = -1;
					idx  = -1;
					break;
			}
		}
		
		if (type < 0)
		{
			previewMessage("No object selected!");
			BeepFail();
			return;
		}
		
		switch (type) {
			case OBJ_WALL:
			case OBJ_MASKED:
				if (wall[idx].extra > 0) break;
				else if (!gPreview.translateObjects)
				{
					if (wall[idx].nextwall >= 0)
					{
						next = sectorofwall(wall[idx].nextwall);
						if (sector[next].extra > 0 && xsector[sector[next].extra].triggerWallPush)
						{
							type = OBJ_FLOOR;
							idx = next;
						}
					}
				}
				else if (qsetmode == 200)
				{
					idx = sectorofwall(idx);
					TranslateWallToSector();
					if (searchsector >= 0 && sector[searchsector].extra > 0)
					{
						idx = searchsector;
						type = OBJ_FLOOR;
					}
				}
				else
				{
					next = wall[idx].nextwall;
					idx = sectorofwall(idx);
					if (next >= 0 && (sector[idx].type <= 0 || sector[idx].extra <= 0))
					{
						idx = sectorofwall(next);
						type = OBJ_FLOOR;
					}
				}
				break;
			case OBJ_SPRITE:
				if (!gPreview.translateObjects) break;
				else if (sprite[idx].type == 709 || sprite[idx].statnum == kStatMarker || (sprite[idx].extra <= 0 && sprite[idx].statnum != kStatItem))
				{
					type = OBJ_FLOOR;
					idx = sprite[idx].sectnum;
				}
				break;
		}

		if ((chk = previewCheckTriggerFlags(type, idx)) < 0)
		{
			if (type == OBJ_FLOOR || type == OBJ_CEILING) type = 6;
			else if (type == OBJ_SPRITE && mouse1)
			{
				switch (sprite[idx].statnum) {
					case kStatItem:
					case kStatThing:
					case kStatDude:
						chk = 0;
						break;
				}
			}
			
			switch (chk) {
				case -1:
					previewMessage("The %s#%d have no trigger flags set!", gSearchStatNames[type], idx);
					BeepFail();
					return;
				case -2:
					previewMessage("The %s#%d is common and cannot be triggered!", gSearchStatNames[type], idx);
					BeepFail();
					return;
				case -3:
					previewMessage("The %s#%d is locked!", gSearchStatNames[type], idx);
					BeepFail();
					return;
				case -4:
					return;
			}
		}
				
		switch (type) {
			case OBJ_SPRITE:
			{
				spritetype* pSprite = &sprite[idx];
				XSPRITE* pXSprite = (pSprite->extra > 0) ? &xsprite[pSprite->extra] : NULL;
				if (mouse1)
				{
					switch (pSprite->statnum){
						case kStatItem:
							previewPickupItem(pSprite, pXSprite);
							return;
						case kStatDude:
							previewKillDude(pSprite, pXSprite);
							return;
						case kStatThing:
							previewDestroyThing(pSprite, pXSprite, cmd);
							return;
					}
				}
				
				switch (pSprite->type)
				{
					case kModernCondition:
					case kModernConditionFalse:
						scrSetMessage("That must be triggered by object.");
						BeepFail();
						break;
					default:
						trTriggerSprite(idx, &xsprite[sprite[idx].extra], cmd);
						break;
				}
			}
			break;
			case OBJ_WALL:
			case OBJ_MASKED:
				trTriggerWall(idx, &xwall[wall[idx].extra], cmd);
				break;
			case OBJ_FLOOR:
			case OBJ_CEILING:
				int rx = xsector[sector[idx].extra].rxID;
				if (!gPreview.translateObjects)
				{
					trTriggerSector(idx, &xsector[sector[idx].extra], cmd);
				}
				else if (!gPreview.triggerFlags && rx > 0)
				{
					for (int i = 0; i < numsectors; i++)
					{
						if (sector[i].extra > 0 && xsector[sector[i].extra].rxID == rx)
							trTriggerSector(i, &xsector[sector[i].extra], cmd);
					}
				}
				else
				{
					trTriggerSector(idx, &xsector[sector[idx].extra], cmd);
				}
				break;
		}
	}
}

BOOL previewFireMissile(int nMissile)
{
	spritetype* pSpr;
	int nSpr, nSect, nSlope;
	if ((nSect = cursectnum) < 0) return FALSE;
	else if (!FindSector(posx, posy, &nSect)) return FALSE;
	else if ((nSpr = InsertSprite(nSect, 0)) < 0) // use temporary sprite to create a missile
		return FALSE;

	pSpr = &sprite[nSpr];
	pSpr->x = posx;
	pSpr->y = posy;
	pSpr->z = posz;
	pSpr->ang = ang;
	pSpr->clipdist = 0;
	
	if (horiz < 100) nSlope = (100 - horiz)<<7;
	else if (horiz > 100) nSlope = -(horiz - 100)<<7;
	else nSlope = 0;

	actFireMissile(pSpr, 0, 0,  Cos(pSpr->ang)>>16, Sin(pSpr->ang)>>16, nSlope, nMissile);
	DeleteSprite(nSpr); // delete temporary sprite
	return TRUE;
	
}

BOOL previewDoExplode(int nExplode) {
	
	int i, sect = -1, x, y, z;
	hit2sector(&sect, &x, &y, &z, 0);
	if (sect >= 0 && (i = InsertSprite(sect, kStatTraps)) >= 0)
	{
		GetXSprite(i);
		spritetype* pSprite = &sprite[i];
		pSprite->yvel = kDeleteReally;
		pSprite->type = kTrapExploder;
		pSprite->x = x, pSprite->y = y;
		pSprite->z = z;
		
		xsprite[pSprite->extra].data1 = nExplode;
		i = gModernMap, gModernMap = TRUE; // force gModernMap to make custom explosion work
		actExplodeSprite(pSprite);
		clampSprite(pSprite);
		gModernMap = i;
		return TRUE;
	}
	
	return FALSE;
}



void previewSpawnSeq() {
	
	buffer[0] = 0;
	if (!GetStringBox("Spawn SEQ", buffer))
		return;
	
	RESHANDLE hSeq;
	ChangeExtension(buffer, getExt(kSeq));
	if (!fileExists(buffer, &hSeq) || hSeq == NULL || (hSeq->flags & 0x02))
	{
		Alert("%s is not exists in %s", buffer, gPaths.bloodRFF);
		return;
	}
	
	getHighlightedObject();
	switch(searchstat) {
		case OBJ_FLOOR:
		case OBJ_CEILING:
			seqSpawn(hSeq->id, searchstat, GetXSector(searchsector));
			break;
		case OBJ_WALL:
		case OBJ_MASKED:
			seqSpawn(hSeq->id, searchstat, GetXWall(searchwall));
			break;
		case OBJ_SPRITE:
			seqSpawn(hSeq->id, searchstat, GetXSprite(searchwall));
			clampSprite(&sprite[searchwall]);
			break;
	}
	
}

void previewMessage(char *__format, ...)
{
	char temp[256];
	va_list argptr;
	va_start(argptr, __format);
	vsprintf(temp, __format, argptr);
	va_end(argptr);
	
	scrSetMessage(temp);
	buildprintf("%s\n",temp);
}

void previewDelSprite(int nSprite) {
	
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	int nXSprite = sprite[nSprite].extra;
	
	evKill(nSprite, 3);
	if (nXSprite > 0)
		seqKill(3, nXSprite);
	
	sfxKill3DSound(&sprite[nSprite]);
	switch (sprite[nSprite].statnum) {
		case kStatExplosion:
		case kStatProjectile:
		case kStatFX:
		case kStatDude:
			if (sprite[nSprite].yvel == kDeleteReally)
			{
				DeleteSprite(nSprite);
				return;
			}
			break;
		default:
			break;
	}
	
	if (sprite[nSprite].statnum < kStatFree)
		ChangeSpriteStat(nSprite, 0);
	
	hideSprite(&sprite[nSprite]); // just hide the sprites already created by level designer

}

void hideSprite(spritetype *pSprite) {
	
	pSprite->yrepeat = pSprite->xrepeat = 0;
	pSprite->x = pSprite->y = kHiddenSpriteLoc;
	pSprite->flags = 32;
	pSprite->type = 0;
	pSprite->cstat = kSprInvisible;

	if (pSprite->extra > 0)
		xsprite[pSprite->extra].txID = xsprite[pSprite->extra].rxID = xsprite[pSprite->extra].health = 0;

}

void unhideSprite(spritetype* pSprite) {
	if (pSprite->x == kHiddenSpriteLoc && pSprite->y == kHiddenSpriteLoc)
		sprite[pSprite->index] = cpysprite[pSprite->index];
}
