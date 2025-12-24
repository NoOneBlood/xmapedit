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
#include "common_game.h"
#include "xmpmaped.h"
#include "preview.h"
#include "seq.h"
#include "aadjust.h"
#include "hglt.h"

PREVIEW_MODE gPreview;
static PREVIEW_MODE_KEYS gPreviewKeys[] = {

    {KEY_ESC,           FALSE, FALSE, FALSE,            3},
    {KEY_HOME,          FALSE, FALSE, FALSE,            3},
    {KEY_UP,            FALSE, FALSE, FALSE,            3},
    {KEY_LEFT,          FALSE, FALSE, FALSE,            3},
    {KEY_DOWN,          FALSE, FALSE, FALSE,            3},
    {KEY_RIGHT,         FALSE, FALSE, FALSE,            3},
    {KEY_A,             FALSE, FALSE, FALSE,            3},
    {KEY_Z,             FALSE, FALSE, FALSE,            3},
    {KEY_PADENTER,      FALSE, FALSE, FALSE,            3},
    {KEY_PADPERIOD,     FALSE, FALSE, FALSE,            3},
    {KEY_CAPSLOCK,      FALSE, FALSE, FALSE,            3},
    {KEY_TILDE,         FALSE, FALSE, FALSE,            3},
    {KEY_RSHIFT,        FALSE, FALSE, FALSE,            3},
    {KEY_LSHIFT,        FALSE, FALSE, FALSE,            3},
    {KEY_RALT,          FALSE, FALSE, FALSE,            3},
    {KEY_LALT,          FALSE, FALSE, FALSE,            3},
    {KEY_RCTRL,         FALSE, FALSE, FALSE,            3},
    {KEY_LCTRL,         FALSE, FALSE, FALSE,            3},
    {KEY_PRINTSCREEN,   FALSE, FALSE, FALSE,            3},
    {KEY_ENTER,         FALSE, FALSE, FALSE,            2},
    {KEY_F12,           FALSE, FALSE, FALSE,            3},
    {KEY_F7,            FALSE, FALSE, FALSE,            1},
    {KEY_F8,            FALSE, FALSE, FALSE,            1},
    {KEY_TAB,           FALSE, FALSE, TRUE,             2},
    {KEY_T,             FALSE, TRUE,  FALSE,            1},
    {KEY_E,             FALSE, FALSE, FALSE,            3},
    {KEY_I,             FALSE, TRUE, FALSE,             2},
    {KEY_B,             FALSE, TRUE, FALSE,             3},
    {KEY_SCROLLLOCK,    FALSE, FALSE, FALSE,            3},
    {KEY_G,             FALSE, FALSE, FALSE,            1},
    {KEY_S,             FALSE, FALSE, FALSE,            2},
    {KEY_PAD5,          FALSE, FALSE, FALSE,            2},
    {KEY_F11,           FALSE, FALSE, FALSE,            1},
    {0, 0, 0, 0, 0}
};

void PREVIEW_MODE::Start()
{
    spritetype* pSpr; XSPRITE* pXSpr;
    int i;
    
    AutoAdjustSprites();
    CleanUp();
    
    gPreviewMode = 1;
    if ((pState = new SNAPSHOT_MANAGER(boardSnapshotMake, boardSnapshotLoad)) != NULL)
        pState->Make(0);
    
    memset(scrEffects,          0, sizeof(scrEffects));
    memset(show2dsprite,        0, sizeof(show2dsprite));
    memset(show2dwall,          0, sizeof(show2dwall));
    memset(show2dsector,        0, sizeof(show2dsector));
    memset(hgltspri,            0, sizeof(hgltspri));
    memset(hgltwall,            0, sizeof(hgltwall));

    automapping = 1;
    
    memset(gSpriteHit, 0, sizeof(gSpriteHit));
    memset(xvel, 0, sizeof(xvel));
    memset(yvel, 0, sizeof(yvel));
    memset(zvel, 0, sizeof(zvel));
    
    memset(&kills, 0, sizeof(kills));
    memset(&secrets, 0, sizeof(secrets));
    levelTime = 0;
    
    memset(gScreen.msg, 0, sizeof(gScreen.msg));
    gScreen.msgShowTotal = 16;
    asksave = 0;
    
    gHighSpr = -1;
    gPostCount = 0;
    
    if (gMisc.externalModels == 2)
        usevoxels = 1;
    
    for (i = 0; i < kMaxSprites; i++)
    {
        pSpr = &sprite[i];
        if (pSpr->extra <= 0 || pSpr->statnum >= kMaxStatus)
            continue;

        pXSpr = &xsprite[pSpr->extra];
        if (pXSpr->rxID == 60 && pXSpr->txID == pXSpr->rxID && pXSpr->command == 100)
            gModernMap = TRUE;

        if ((pXSpr->lSkill & (1 << difficulty)))
        {
            DeleteSprite(pSpr->index);
            continue;
        }
        
        if (mode <= kGameModeFlags)
        {
            if ((pXSpr->lS && mode == kGameModeSingle)
                || (pXSpr->lB && mode == kGametModeDeathmatch)
                || (pXSpr->lT && mode == kGameModeFlags)
                || (pXSpr->lC && mode == kGameModeCoop)) {
                    DeleteSprite(pSpr->index);
                    continue;
            }
        }
        
        switch(pSpr->type)
        {
            case kMarkerSPStart:
                if (forceStartPos && pXSpr->data1 == 0)
                {
                    posx = pSpr->x; posy = pSpr->y; posz = pSpr->z;
                    ang = pSpr->ang; cursectnum = pSpr->sectnum;
                }
                // no break
            case kMarkerMPStart:
                DeleteSprite(pSpr->index);
                break;
            case kTrapExploder:
                pXSpr->state = 0;
                pXSpr->waitTime = ClipLow(pXSpr->waitTime, 1);
                break;
            case kTrapMachinegun:
                pXSpr->state = 0;
                // no break;
            case kTrapFlame:
                pSpr->picnum = 2178;
                break;
            case kDudeGargoyleStatueFlesh:
            case kDudeGargoyleStatueStone:
            case kThingArmedProxBomb:
                pXSpr->state = 0;
                break;
            case kGenSound:
            case kSoundSector:
            case kSoundAmbient:
                pSpr->xrepeat = 0;
                break;
            default:
                switch(pSpr->statnum)
                {
                    case kStatThing:
                    case kStatDude:
                        pXSpr->state = 1;
                        break;
                }
                break;
        }
        
        if (IsDudeSprite(pSpr))
        {
            pSpr->cstat |= kSprBlock;
            pXSpr->health = 100 << 4;

            switch (pSpr->type)
            {
                case kDudeGargoyleFlesh:
                case kDudeGargoyleStone:
                case kDudeBat:
                case kDudePhantasm:
                    break;
                case kDudePodGreen:
                case kDudePodFire:
                case kDudeTentacleGreen:
                case kDudeTentacleFire:
                    if (gModernMap && (pSpr->cstat & kSprFlipY)) break;
                    pSpr->flags = kPhysGravity|kPhysFalling|kPhysMove;
                    break;
                case kDudeSpiderBrown:
                case kDudeSpiderBlack:
                case kDudeSpiderMother:
                case kDudeSpiderRed:
                    pSpr->cstat &= ~kSprFlipY;
                    // no break
                default:
                    pSpr->flags = kPhysGravity|kPhysFalling|kPhysMove;
                    break;
            }

            kills.total++;
        }
        else if (IsThingSprite(pSpr))
        {
            int nType = pSpr->type - kThingBase;
            pXSpr->health = thingInfo[nType].startHealth << 4;
            if (!gModernMap)
                pSpr->clipdist = thingInfo[nType].clipdist;

            pSpr->flags = thingInfo[nType].flags;
            if (pSpr->flags & kPhysGravity)
                pSpr->flags |= kPhysFalling;
        }
    }
    
    if (gModernMap) nnExtInitModernStuff();
    else nnExtResetGlobals(); 
        
    if (enableSound)
    {
        sfxInit();
        ambInit();
    }
    
    warpInit();
    evInit();
    InitMirrors();
    
    if (enableMusic)
    {
        char *songName, buffer[256];
        if ((songName = pEpisode->GetKeyString(getFilename(gPaths.maps, buffer), "Song", "")) != NULL)
            sndPlaySong(songName, TRUE);
    }
    
    trInit();
    
    sectnum = cursectnum;
    if (sectnum >= 0 && isUnderwaterSector(sectnum))
        scrSetPalette(kPal1);
    
	scrSetLogMessage("Game mode: %s", gGameNames[mode].name);
	scrSetLogMessage("Difficulty: %s.", gDifficNames[difficulty].name);
	scrSetLogMessage("Preview mode enabled.");
    BeepOk();
}

void PREVIEW_MODE::Stop()
{
    if (enableSound)
    {
        sndKillAllSounds();
        sfxKillAllSounds();
        ambKillAll();
    }
    
    if (enableMusic)
        sndStopSong();
    
    gTimers.autosave = totalclock;
    
    resortFree();
    seqKillAll();
    RestoreMirrorPic();
    nnExtResetGlobals();
    scrSetPalette(kPal0);
    
	updatesector(posx, posy, &cursectnum);
	if (gMisc.externalModels == 2)
		usevoxels = 0;
    
	memset(show2dsprite, 	0, sizeof(show2dsprite));
	memset(show2dwall, 		0, sizeof(show2dwall));
	memset(show2dsector, 	0, sizeof(show2dsector));
	automapping = 0;
    
	// free lists
	gProxySpritesList.Free();
	gSightSpritesList.Free();
	gPhysSpritesList.Free();
	
	gScreen.msgShowTotal = 1;
    
    gMapedHud.SetTile();
    
    if (pState)
    {
        pState->Load(0);
        delete(pState);
    }
    
	scrSetLogMessage("Preview mode disabled.");
    gPreviewMode = 0;
	BeepFail();
}

void PREVIEW_MODE::Process()
{
    int i;
    
    if (totalclock >= timer.time + speed)
    {
        timer.time = totalclock;
        
        if (scrEffects[kScrEffectQuake2])
            scrEffects[kScrEffectQuake2] = 0;
        
        i = kScrEffectMax;
        while(--i >= 0)
            scrEffects[i] = ClipLow(scrEffects[i] - speed, 0);

        trProcessBusy();
        evProcess(gFrameClock);
        seqProcess(speed);

        actProcessSprites();
        actPostProcess();

        ambProcess();
        sfxUpdate3DSounds();

        levelTime++;
    }

    PickupProcess(posx, posy, posz, cursectnum);
    TriggerObjectProcess();
}

void PREVIEW_MODE::TriggerObjectProcess()
{
    int nCmd = -1, nNext = -1, chk = -1;
    int t;
    
    OBJECT obj;
    walltype* pWall;
    spritetype* pSpr;
    
    sectortype* pSect;
    XSECTOR* pXSect;
     
    if ((vel || svel) && cursectnum >= 0 && sectnum != cursectnum)
    {
        sectortype* pSect = &sector[cursectnum];
        XSECTOR* pXSect = GetXSect(pSect);
        
        if (pXSect && pXSect->triggerEnter)
        {
            if (pXSect->locked)
            {
                scrSetLogMessage("The %s#%d is locked!", gSearchStatNames[OBJ_SECTOR], cursectnum);
                BeepFail();
            }
            else
            {
                trTriggerSector(cursectnum, pXSect, kCmdSectorEnter);
            }
        }

        if (sectnum >= 0 && (pXSect = GetXSect(&sector[sectnum])) != NULL && pXSect->triggerExit)
        {
            if (pXSect->locked)
            {
                scrSetLogMessage("The %s#%d is locked!", gSearchStatNames[OBJ_SECTOR], sectnum);
                BeepFail();
            }
            else
            {
                trTriggerSector(sectnum, pXSect, kCmdSectorExit);
            }
        }
        
        sectnum = cursectnum;
    }



    gMouse.ReadButtons();
    char mouse1 = ((gMouse.press & 1) != 0);
    char mouse2 = ((ED3D) ? (gMouse.press & 2) : (mouse1 && (shift & 0x02)));
    char mouse3 = ((ED3D) ? (gMouse.press & 4) : (mouse1 && (ctrl & 0x02)));
    
    if (mouse1)      nCmd = m1cmd;
    else if (mouse2) nCmd = m2cmd;
    else if (mouse3) nCmd = m3cmd;
    
    if (nCmd < 0)
        return;
    

    
    getHighlightedObject();
    switch (searchstat)
    {
        case OBJ_SPRITE:
            obj.index = searchwall;
            obj.type  = searchstat;
            if (translateObjects)
            {
                pSpr = &sprite[searchwall];
                if (pSpr->type == 709 || pSpr->statnum == kStatMarker || (pSpr->extra <= 0 && pSpr->statnum != kStatItem))
                    obj.type = OBJ_FLOOR, obj.index = pSpr->sectnum;
            }
            break;
        case OBJ_WALL:
        case OBJ_MASKED:
            pWall = &wall[searchwall];
            obj.index = searchwall;
            obj.type  = searchstat;
            if (pWall->extra <= 0)
            {
                if (!translateObjects)
                {
                    if (pWall->nextwall >= 0 && (nNext = sectorofwall(pWall->nextwall)) >= 0)
                    {
                        if ((pXSect = GetXSect(&sector[nNext])) != NULL && pXSect->triggerWallPush)
                            obj.type = OBJ_FLOOR, obj.index = nNext;
                    }
                }
                else if (ED3D)
                {
                    TranslateWallToSector();
                    if (searchsector >= 0 && sector[searchsector].extra > 0)
                        obj.type = OBJ_FLOOR, obj.index = searchsector;
                }
                else
                {
                    t = sectorofwall(obj.index);
                    if (pWall->nextwall >= 0 && (sector[t].type <= 0 || sector[t].extra <= 0))
                         obj.type = OBJ_FLOOR, obj.index = sectorofwall(pWall->nextwall);
                }
            }
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            obj.index = searchsector;
            obj.type  = searchstat;
            break;
        default:
            scrSetLogMessage("No object selected!");
            BeepFail();
            return;
    }
    
    if ((chk = TriggerObjectCheck(&obj)) < 0)
    {
        switch(obj.type)
        {
            case OBJ_FLOOR:
            case OBJ_CEILING:
                obj.type = OBJ_SECTOR; // for names
                break;
            case OBJ_SPRITE:
                if (mouse1)
                {
                    switch (sprite[obj.index].statnum)
                    {
                        case kStatItem:
                        case kStatThing:
                        case kStatDude:
                            chk = 0;
                            break;
                    }
                }
                break;
            
        }
        
        switch (chk)
        {
            case -1:
                scrSetLogMessage("The %s#%d have no trigger flags set!", gSearchStatNames[obj.type], obj.index);
                BeepFail();
                return;
            case -2:
                scrSetLogMessage("The %s#%d is common and cannot be triggered!", gSearchStatNames[obj.type], obj.index);
                BeepFail();
                return;
            case -3:
                scrSetLogMessage("The %s#%d is locked!", gSearchStatNames[obj.type], obj.index);
                BeepFail();
                return;
            case -4:
                return;
        }
    }

    switch (obj.type)
    {
        case OBJ_SPRITE:
            pSpr = &sprite[obj.index];
            if (mouse1)
            {
                switch (pSpr->statnum)
                {
                    case kStatItem:     ItemPickup(pSpr);       return;
                    case kStatDude:     DudeKill(pSpr);         return;
                    case kStatThing:    ThingKill(pSpr, nCmd);  return;
                }
            }

            switch (pSpr->type)
            {
                case kModernCondition:
                case kModernConditionFalse:
                    scrSetMessage("That must be triggered by object.");
                    BeepFail();
                    break;
                default:
                    trTriggerSprite(pSpr->index, &xsprite[pSpr->extra], nCmd);
                    break;
            }
            break;
            
        case OBJ_WALL:
        case OBJ_MASKED:
            trTriggerWall(obj.index, &xwall[wall[obj.index].extra], nCmd);
            break;
        case OBJ_FLOOR:
        case OBJ_CEILING:
            pXSect = GetXSect(&sector[obj.index]);
            if (translateObjects)
            {
                if (!triggerFlags && pXSect->rxID)
                {
                    t = numsectors;
                    while(--t >= 0)
                    {
                        if (sector[t].extra > 0 && xsector[sector[t].extra].rxID == pXSect->rxID)
                            trTriggerSector(t, &xsector[sector[t].extra], nCmd);
                    }
                    
                    break;
                }
            }
            trTriggerSector(obj.index, pXSect, nCmd);
            break;
    }
}

int PREVIEW_MODE::TriggerObjectCheck(OBJECT* pObj)
{
    int r = 0;
    switch(pObj->type)
    {
        case OBJ_FLOOR:
        case OBJ_CEILING:
        {
            sectortype* pSect = &sector[pObj->index];
            XSECTOR* pXObj    = GetXSect(pSect);
            
            if (pXObj == NULL)              return -2;
            else if (pXObj->locked)         return -3;
            else if (pXObj->isTriggered)    return -4;

            if (!triggerFlags)                                              r = 0;
            else if (pSect->type == kSectorCounter)                         r = kCmdCallback;
            else if (pXObj->triggerPush || pXObj->triggerWallPush)          r = kCmdSectorPush;
            else if (pXObj->triggerEnter)                                   r = kCmdSectorEnter;
            else if (pXObj->triggerExit)                                    r = kCmdSectorExit;
            else                                                            r = -1;
        }
        break;
        case OBJ_SPRITE:
        {
            spritetype* pSpr = &sprite[pObj->index];
            XSPRITE* pXObj    = GetXSpr(pSpr);
            
            if (pXObj == NULL)              return -2;
            else if (pXObj->locked)         return -3;
            else if (pXObj->isTriggered)    return -4;

            if (!triggerFlags)                                              r = 0;
            else if (pXObj->triggerPush)                                    r = kCmdSpritePush;
            else if (pXObj->triggerVector)                                  r = kCmdSpriteImpact;
            else if (pXObj->triggerImpact)                                  r = kCmdSpriteExplode;
            else if (pXObj->triggerPickup && pSpr->statnum == kStatItem)    r = kCmdSpritePickup;
            else if (pXObj->triggerTouch)                                   r = kCmdSpriteTouch;
            else if (pXObj->triggerProximity)                               r = kCmdSpriteProximity;
            else if (pXObj->triggerSight)                                   r = kCmdSpriteSight;
            else                                                            r = -1;
        }
        break;
        case OBJ_WALL:
        case OBJ_MASKED:
        {
            walltype* pWall = &wall[pObj->index];
            XWALL* pXObj    = GetXWall(pWall);
            
            if (pXObj == NULL)              return -2;
            else if (pXObj->locked)         return -3;
            else if (pXObj->isTriggered)    return -4;

            if (!triggerFlags)                                              r = 0;
            else if (pXObj->triggerPush)                                    r = kCmdWallPush;
            else if (pXObj->triggerVector)                                  r = kCmdWallImpact;
            else if (pXObj->triggerTouch)                                   r = kCmdWallPush;
            else                                                            r = -1;
        }
        break;
    }

    return r;
}

void PREVIEW_MODE::PickupProcess(int x, int y, int z, int nSect)
{
    int i, dx, dy, vb, zTop, zBot;
    spritetype *pItem;
    
    for (i = headspritestat[kStatItem]; i >= 0; i = nextspritestat[i])
    {
        pItem = &sprite[i];
        
        if (pItem->flags & kHitagFree)              continue;
        if ((dx = klabs(x-pItem->x)>>4) > 48)       continue;
        if ((dy = klabs(y-pItem->y)>>4) > 48)       continue;
        if (approxDist(dx, dy) > 48)                continue;
        
        zTop = z;
        zBot = z + kensplayerheight;
        
        if (pItem->z < zTop)            vb = (zTop-pItem->z)>>8;
        else if (pItem->z > zBot)       vb = (pItem->z-zBot)>>8;
        else                            vb = 0;
        
        if (vb < 32)
        {
            GetSpriteExtents(pItem, &zTop, &zBot);
            
            if
            (
                cansee(x, y, z, nSect, pItem->x, pItem->y, pItem->z, pItem->sectnum)    ||
                cansee(x, y, z, nSect, pItem->x, pItem->y, zTop, pItem->sectnum)        ||
                cansee(x, y, z, nSect, pItem->x, pItem->y, zBot, pItem->sectnum)
            )
            {
                ItemPickup(pItem);
            }
        }
    }
}

void PREVIEW_MODE::ItemPickup(spritetype* pSpr)
{
    char name[32] = "\0";
    XSPRITE* pXSpr;
    
    switch(pSpr->type)
    {
        case kItemFlagABase:
        case kItemFlagBBase:
            return;
    }
    
    if (gSpriteNames[pSpr->type])
    {
        strcpy(name, gSpriteNames[pSpr->type]);
    }
    else
    {
        sprintf(name, "ITEM #%d", pSpr->type);
    }

    scrSetLogMessage("Picked up %s", gSpriteNames[pSpr->type]);
    if ((pXSpr = GetXSpr(pSpr)) != NULL && pXSpr->triggerPickup)
        trTriggerSprite(pSpr->index, pXSpr, kCmdSpritePickup);

    actPostSprite(pSpr->index, kStatFree);
    BeepOk();
}

void PREVIEW_MODE::DudeKill(spritetype* pSpr)
{
    XSPRITE* pXSpr = GetXSpr(pSpr);
    
    if (pXSpr && pXSpr->locked)
    {
        scrSetLogMessage("Cannot kill locked %s", gSpriteNames[pSpr->type]);
        return;
    }

    switch (pSpr->type)
    {
        case kDudeZombieAxeLaying:
        case kDudeZombieAxeBuried:
            break;
        default:
            if (DudeMorph(pSpr)) return;
            break;
    }

    if (pXSpr)
    {
        if (pXSpr->dropItem) actDropObject(pSpr, pXSpr->dropItem);
        if (pXSpr->key) actDropObject(pSpr, pXSpr->key + kItemBase - 1);
        trTriggerSprite(pSpr->index, pXSpr, kCmdOff);
    }
    
    kills.done++;
    scrSetLogMessage("%s is killed.", gSpriteNames[pSpr->type]);
    actPostSprite(pSpr->index, kStatFree);
    BeepFail();
}

char PREVIEW_MODE::DudeMorph(spritetype* pSpr)
{
    int nOType = pSpr->type, nNType = -1;
    spritetype* pMorph;
    
    switch (pSpr->type)
    {
        case kDudeZombieAxeLaying:
        case kDudeZombieAxeBuried:
            nNType = kDudeZombieAxeNormal;
            break;
        case kDudeGargoyleStatueFlesh:
            nNType = kDudeGargoyleFlesh;
            break;
        case kDudeGargoyleStatueStone:
            nNType = kDudeGargoyleStone;
            break;
        case kDudeCerberusTwoHead:
            nNType = kDudeCerberusOneHead;
            break;
        case kDudeCultistBeast:
            nNType = kDudeBeast;
            break;
        case kDudeModernCustom:
            if ((pMorph = cdGetNextIncarnation(pSpr)) != NULL)
            {
                nNType = pMorph->type;
                cdTransform(pSpr);
            }
            break;
    }

    if (nNType >= 0)
    {
        pSpr->type = nNType;
        adjSpriteByType(pSpr);
        scrSetLogMessage("%s is morphed to %s.", gSpriteNames[nOType], gSpriteNames[nNType]);
        pSpr->ang = getangle(posx - pSpr->x, posy - pSpr->y) & kAngMask;
        BeepOk();
        return 1;
    }

    return 0;
}

void PREVIEW_MODE::ThingKill(spritetype* pSpr, int nCmd)
{
    XSPRITE* pXSpr = GetXSpr(pSpr);
    
    if (pXSpr)
        trTriggerSprite(pSpr->index, pXSpr, nCmd);
}

void PREVIEW_MODE::MapMessage(int nCmd)
{
    char section[16], key[16], def[32];
    int nMsg;
    
    if (nCmd >= kCmdNumberic)
    {
        getFilename(gPaths.maps, section); nMsg = (nCmd - kCmdNumberic) + 1;
        sprintf(key, "message%d", nMsg); sprintf(def, "Trigger INI %s", key);
        scrSetLogMessage(pEpisode->GetKeyString(section, key, def));
        BeepOk();
        return;
    }
    
    scrSetLogMessage("Wrong command for triggering INI message!");
    BeepFail();
    return;
}

char PREVIEW_MODE::ShowMenu(void)
{
    int nGame = mode, nSkill = difficulty;
    
    while( 1 )
    {
        nGame = showButtons(gGameNames, LENGTH(gGameNames), "Game Mode") - mrUser;
        if (nGame < 0) return 0;
        
        nSkill = showButtons(gDifficNames, LENGTH(gDifficNames), "Difficulty") - mrUser;
        if (nSkill >= 0)
            break;
    }
   
    difficulty = nSkill;
    mode = nGame;
    return 1;
}

void PREVIEW_MODE::ShowStatus(void)
{
    static QFONT* pFont = qFonts[0];
    static int l;
    
    strcpy(buffer, "<< PREVIEW MODE >>");
    gfxSetColor(clr2std(kColorRed ^ h));
    
    l = gfxGetTextLen(buffer, pFont) + 4;
    gfxFillBox(xdim - l - 4, 2, xdim - 2, 14);
    gfxDrawText(xdim - l, 4, clr2std(kColorWhite), buffer, pFont);

    l = sprintf(buffer, "%s / %s", gGameNames[mode].name, gDifficNames[difficulty].name) << 2;
    printextShadowS(xdim - l - 2, 14, clr2std(kColorLightGray ^ h), strupr(buffer));
}

char PREVIEW_MODE::DoExplosion()
{
    int nSect = -1, x, y, z, i;
    spritetype* pSpr;
    
    hit2sector(&nSect, &x, &y, &z, 0);
    if (nSect >= 0 && (i = InsertSprite(nSect, kStatTraps)) >= 0)
    {
        GetXSprite(i);
        
        pSpr = &sprite[i];
        pSpr->type = kTrapExploder;
        pSpr->x = x, pSpr->y = y;
        pSpr->z = z;

        xsprite[pSpr->extra].data1 = explosionType;
        i = gModernMap, gModernMap = 1; // force gModernMap to make custom explosion work
        actExplodeSprite(pSpr);
        clampSprite(pSpr);
        gModernMap = i;
        return 1;
    }

    return 0;
}

char PREVIEW_MODE::FireMissile(void)
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

    actFireMissile(pSpr, 0, 0,  Cos(pSpr->ang)>>16, Sin(pSpr->ang)>>16, nSlope, kMissileBase+missileType);
    DeleteSprite(nSpr); // delete temporary sprite
    return TRUE;
}

char PREVIEW_MODE::SpawnSeq()
{
    char str[256] = "\0";
    if (!GetStringBox("Spawn SEQ", str))
        return 0;

    RESHANDLE hSeq;
    ChangeExtension(str, getExt(kSeq));
    if (!fileExists(str, &hSeq) || hSeq == NULL || (hSeq->flags & 0x02))
    {
        Alert("%s is not exists in %s", str, gPaths.bloodRFF);
        return 0;
    }

    getHighlightedObject();
    switch(searchstat)
    {
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
        default:
            return 0;
    }
    
    return 1;
}

char PREVIEW_MODE::KeyCheck(char key, char ctrl, char alt, char shift)
{
    PREVIEW_MODE_KEYS *p = gPreviewKeys;
    while(p->key)
    {
		if (p->key == key)
        {
            if (p->reqShift && !shift)      return 0;
            else if (p->reqCtrl && !ctrl)   return 0;
            else if (p->reqAlt && !alt)     return 0;
            else if (p->mode >= 3)          return 1;
            else if (ED3D)                  return (p->mode & 0x02) != 0;
            else if (p->mode & 0x01)        return 1;
        }
        
        p++;
    }
    
    return 0;
}

char PREVIEW_MODE::KeyProcess(char key, char ctrl, char alt, char shift)
{
	switch (key)
    {
		case KEY_ESC:
		case KEY_HOME:
			Stop();
			break;
		case KEY_ENTER:
			keystatus[key] = 0;
			return 0;
		case KEY_S:
			SpawnSeq();
			break;
		case KEY_E:
			if (shift & 0x02)
			{
				sprintf(buffer, "Set explosion type (%d-%d)", 0, kExplosionMax);
				explosionType = GetNumberBox(buffer, explosionType, explosionType);
				explosionType = ClipRange(explosionType, 0, kExplosionMax);
				BeepOk();
				break;
			}
			if (DoExplosion()) break;
			BeepFail();
			break;
		case KEY_F:
			if (shift & 0x02)
			{
				sprintf(buffer, "Set missile type (%d-%d)", 0, kMissileMax-kMissileBase-1);
				missileType = GetNumberBox(buffer, missileType, missileType);
				missileType = ClipRange(missileType, 0, kMissileMax-kMissileBase-1);
				BeepOk();
				break;
			}
			if (FireMissile()) break;
			BeepFail();
			break;
		default:
			return 0;
	}
	
	return 1;
}

void PREVIEW_MODE::Init(IniFile* pIni, char* section)
{
    triggerFlags        = pIni->GetKeyBool(section, "TriggerFlagsRequired", 0);
    forceStartPos       = pIni->GetKeyBool(section, "ForcePlayerStartPosition", 0);
    triggerStart        = pIni->GetKeyBool(section, "TriggerStartChannel", 1);
    translateObjects    = pIni->GetKeyBool(section, "TranslateObjects", 0);
    enableSound         = pIni->GetKeyBool(section, "EnableSound", 1);
    enableMusic         = pIni->GetKeyInt(section, "EnableMusic", 0);
    explosionType       = ClipHigh(pIni->GetKeyInt(section, "ExplosionType", 1), kExplosionMax);
    missileType         = ClipHigh(pIni->GetKeyInt(section, "MissileType", 5), kMissileMax-kMissileBase-1);

    speed               = ClipHigh(pIni->GetKeyInt(section, "Speed", 4), 128);
    m1cmd               = ClipHigh(pIni->GetKeyInt(section, "Mouse1Command", 2), 255);
    m2cmd               = ClipHigh(pIni->GetKeyInt(section, "Mouse2Command", 0), 255);
    m3cmd               = ClipHigh(pIni->GetKeyInt(section, "Mouse3Command", 1), 255);
    difficulty          = ClipHigh(pIni->GetKeyInt(section, "DefaultDifficulty", 5), 5);
    mode                = ClipHigh(pIni->GetKeyInt(section, "DefaultGameMode", 4), 4);

    strcpy(gPaths.episodeIni, pIni->GetKeyString(section, "EpisodeIni", "BLOOD.INI"));
    pEpisode            = new IniFile(gPaths.episodeIni);
}

void PREVIEW_MODE::Save(IniFile* pIni, char* section)
{
    pIni->PutKeyInt(section, "DefaultDifficulty", difficulty);
    pIni->PutKeyInt(section, "DefaultGameMode", mode);
    pIni->PutKeyInt(section, "ExplosionType", explosionType);
    pIni->PutKeyInt(section, "MissileType", missileType);

    pIni->PutKeyString(section, "EpisodeIni", gPaths.episodeIni);
}