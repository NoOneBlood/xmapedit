/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of nnexts.cpp adapted for xmapedit's preview mode feature.
// nnexts.cpp provides extended features for mappers
// more info at http://cruo.bloodgame.ru/xxsystem
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

#include "build.h"
#include "editor.h"
#include "xmpstub.h"
#include "gameutil.h"
#include "preview.h"
#include "screen.h"
#include "gui.h"
#include "aadjust.h"
#include "sectorfx.h"
#include "seq.h"
#include "xmpmisc.h"



BOOL gEventRedirectsUsed = FALSE;
short gProxySpritesList[];  // list of additional sprites which can be triggered by Proximity
short gProxySpritesCount;   // current count
short gSightSpritesList[];  // list of additional sprites which can be triggered by Sight
short gSightSpritesCount;   // current count

BOOL modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event) {

    dassert(xspriRangeIsFine(pSprite->extra));
	if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock) {
        
		switch (event.cmd) {
            case kCmdLock:
                pXSprite->locked = 1;
                break;
            case kCmdUnlock:
                pXSprite->locked = 0;
                break;
            case kCmdToggleLock:
                pXSprite->locked = pXSprite->locked ^ 1;
                break;
        }
		
        return TRUE;
		
    }
	
	switch (pSprite->type) {
		default: return FALSE;
		case kModernStealthRegion:
		case kModernDudeTargetChanger:
		case kModernPlayerControl:
		case kModernCondition:
		case kModernConditionFalse:
			//previewMessage("%s (%d) is not implemented.", gSpriteNames[pSprite->type], pSprite->type);
			return TRUE;
		case kMarkerDudeSpawn: {
			if (pXSprite->data1 < kDudeBase || pXSprite->data1 >= kDudeMax) break;
			if (pXSprite->target > 0) {
				if (spriRangeIsFine(pXSprite->target) && sprite[pXSprite->target].statnum == kStatDude)
					actPostSprite(pXSprite->target, kStatFree);
				
				pXSprite->target = 0;

			}
			
			pSprite->xrepeat = 0;
			spritetype* pSpawn = NULL;
			if ((pSpawn = randomSpawnDude(pSprite)) == NULL)
				pSpawn = actSpawnDude(pSprite, (short)pXSprite->data1, -1, 0);
			
			if (pSpawn)
				pXSprite->target = pSpawn->index;
		}
		return TRUE;
        case kModernRandomTX: // random Event Switch takes random data field and uses it as TX ID
        case kModernSequentialTX: // sequential Switch takes values from data fields starting from data1 and uses it as TX ID
			if (pXSprite->command == kCmdNotState) return TRUE; // work as event redirector
            switch (pSprite->type) {
                case kModernRandomTX:
                    useRandomTx(pSprite, (COMMAND_ID)pXSprite->command, TRUE);
                    break;
                case kModernSequentialTX:
                    if (!(pSprite->flags & kModernTypeFlag1)) useSequentialTx(pSprite, (COMMAND_ID)pXSprite->command, TRUE);
                    else seqTxSendCmdAll(pSprite, pSprite->index, (COMMAND_ID)pXSprite->command, FALSE);
                    break;
            }
            return TRUE;
        case kModernCustomDudeSpawn: {
			if (pXSprite->target > 0) {
				if (spriRangeIsFine(pXSprite->target) && sprite[pXSprite->target].statnum == kStatDude)
					actPostSprite(pXSprite->target, kStatFree);
					
				pXSprite->target = 0;
			}
			
			pSprite->xrepeat = 0;
			spritetype* pDude = cdSpawn(pSprite, -1);
			if (pDude)
				pXSprite->target = pDude->index;
		}
		return TRUE;
        case kMarkerWarpDest:
            if (pXSprite->txID <= 0) {
                if (pXSprite->data1 == 1 && SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    useTeleportTarget(pSprite, NULL);
                return TRUE;
            }
            // no break;
        case kModernObjPropChanger:
            if (pXSprite->txID <= 0) {
                if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    usePropertiesChanger(pSprite, -1, -1);
                return TRUE;
            }
			// no break;
        //case kModernSlopeChanger:
        case kModernObjSizeChanger:
        case kModernObjPicChanger:
        case kModernSectFXChg:
        case kModernObjDataChanger:
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return TRUE;
		
        case kModernSeqSpawner:
        case kModernEffectSpawner:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    if (pSprite->type == kModernSeqSpawner) seqSpawnerOffSameTx(pSprite);
                    // no break
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pSprite->type == kModernSeqSpawner) useSeqSpawnerGen(pSprite, 3, pSprite->index);
                    else useEffectGen(pSprite, pSprite);
            
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, ClipLow((int)((pXSprite->busyTime) + BiRandom(pXSprite->data1)) * 120 / 10, 0), kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return TRUE;
        case kModernObjDataAccum:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    // no break;
                case kCmdRepeat:
                    // force OFF after *all* TX objects reach the goal value
                    if (pSprite->flags == kModernTypeFlag0 && incDecGoalValueIsReached(pSprite)) {
                        evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    }
                    
                    modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return TRUE;
        case kModernRandom:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    // no break;
                case kCmdRepeat:
                    useRandomItemGen(pSprite, pXSprite);
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernWindGenerator:
            switch (event.cmd) {
                case kCmdOff:
                    windGenStopWindOnSectors(pSprite);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    //fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useSectorWindGen(pSprite, NULL);

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return TRUE;
        case kModernSpriteDamager:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    // no break;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pXSprite->data1 == 0 && sectRangeIsFine(pSprite->sectnum)) useSpriteDamager(pSprite, EVOBJ_SECTOR, pSprite->sectnum);
                    else if (pXSprite->data1 >= 666 && pXSprite->data1 < 669) useSpriteDamager(pSprite, -1, -1);
/*                     else {

                        PLAYER* pPlayer = getPlayerById(pXSprite->data1);
                        if (pPlayer != NULL)
                            useSpriteDamager(pXSprite, EVOBJ_SPRITE, pPlayer->pSprite->index);
                    } */

                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return TRUE;
        case kGenModernMissileUniversal:
			switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    // no break;
                case kCmdRepeat:
                    useUniMissileGen(pSprite);
                    if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return TRUE;
        case kGenModernSound:
            switch (event.cmd) {
				case kCmdOff:
					if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
					break;
				case kCmdOn:
					evKill(nSprite, 3); // queue overflow protect
					if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
					// no break;
				case kCmdRepeat:
					if (pXSprite->txID)  modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
					else useSoundGen(pXSprite, pSprite);
					
					if (pXSprite->busyTime > 0)
						evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
					break;
				default:
					if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
					else evPost(nSprite, 3, 0, kCmdOff);
					break;
            }
            return TRUE;
		
	}
	return FALSE;
	
}

BOOL modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event) {
    
    switch (pWall->type) {
        case 21:
            switch (event.cmd) {
                case kCmdOff:
                    SetWallState(nWall, pXWall, 0);
                    break;
                case kCmdOn:
                    SetWallState(nWall, pXWall, 1);
                    break;
                default:
                    SetWallState(nWall, pXWall, pXWall->restState ^ 1);
                    break;
            }
            return TRUE;
        default:
            return FALSE; // no modern type found to work with, go normal OperateWall();
    }
    
}

void modernTypeSendCommand(int nSprite, int destChannel, COMMAND_ID command) {
    switch (command) {
		case kCmdNotState:
			evSend(nSprite, 3, destChannel, kCmdModernUse); // just send command to change properties
			return;
		case kCmdUnlock:
			evSend(nSprite, 3, destChannel, command); // send normal command first
			evSend(nSprite, 3, destChannel, kCmdModernUse);  // then send command to change properties
			return;
		default:
			evSend(nSprite, 3, destChannel, kCmdModernUse); // send first command to change properties
			evSend(nSprite, 3, destChannel, command); // then send normal command
			return;
    }
}

BOOL modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState) {
    if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
        return 0;

    pXSprite->busy = nState << 16; pXSprite->state = nState;
    
    evKill(nSprite, 3);
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);

    if (pXSprite->txID != 0 && ((pXSprite->triggerOn && pXSprite->state) || (pXSprite->triggerOff && !pXSprite->state)))
        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);

	return 1;
}

spritetype* cdGetNextIncarnation(spritetype* pSprite) {
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
	for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != 3 || rxBucket[i].index == pSprite->index)
            continue;
        
        if (sprite[rxBucket[i].index].statnum == kStatInactive)
            return &sprite[rxBucket[i].index];
    }
    return NULL;
}


void cdTransform(spritetype* pSprite) {
    
    if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites))
        return;
    
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    
	spritetype* pIncarnation = cdGetNextIncarnation(pSprite);
    if (pIncarnation == NULL) {
        trTriggerSprite(pSprite->index, pXSprite, kCmdOff);
        return;
    }
	
    XSPRITE* pXIncarnation = &xsprite[pIncarnation->extra];
    pXSprite->key = pXSprite->dropItem = pXSprite->locked = 0;

    // save incarnation's going on and off options
    BOOL triggerOn = (BOOL)pXIncarnation->triggerOn;
	BOOL triggerOff = (BOOL)pXIncarnation->triggerOff;

    // then remove it from incarnation so it will not send the commands
    pXIncarnation->triggerOn = FALSE;
    pXIncarnation->triggerOff = FALSE;

    // trigger dude death before transform
    trTriggerSprite(pSprite->index, pXSprite, kCmdOff);

    pSprite->type = pIncarnation->type;
    pSprite->flags = pIncarnation->flags;
    pSprite->pal = pIncarnation->pal;
    pSprite->shade = pIncarnation->shade;
    pSprite->clipdist = pIncarnation->clipdist;
    pSprite->xrepeat = pIncarnation->xrepeat;
    pSprite->yrepeat = pIncarnation->yrepeat;

    pXSprite->txID = pXIncarnation->txID;
    pXSprite->command = pXIncarnation->command;
    pXSprite->triggerOn = triggerOn;
    pXSprite->triggerOff = triggerOff;
    pXSprite->busyTime = pXIncarnation->busyTime;
    pXSprite->waitTime = pXIncarnation->waitTime;

    // inherit respawn properties
    pXSprite->respawn = pXIncarnation->respawn;
    pXSprite->respawnPending = pXIncarnation->respawnPending;

    pXSprite->burnTime = 0;
    pXSprite->burnSource = -1;

    pXSprite->data1 = pXIncarnation->data1;
    pXSprite->data2 = pXIncarnation->data2;

    pXSprite->data3 = pXIncarnation->data3;  // soundBase id
    pXSprite->data4 = pXIncarnation->data4;  // start hp

    pXSprite->dudeGuard = pXIncarnation->dudeGuard;
    pXSprite->dudeDeaf = pXIncarnation->dudeDeaf;
    pXSprite->dudeAmbush = pXIncarnation->dudeAmbush;
    pXSprite->dudeFlag4 = pXIncarnation->dudeFlag4;
    pXSprite->unused1 = pXIncarnation->unused1;

    pXSprite->dropItem = pXIncarnation->dropItem;
    pXSprite->key = pXIncarnation->key;

    pXSprite->locked = pXIncarnation->locked;
    pXSprite->decoupled = pXIncarnation->decoupled;

    // clear drop items of the incarnation
    pXIncarnation->key = pXIncarnation->dropItem = 0;

    // set hp
    pXSprite->health = 100 << 4;

    pXIncarnation->triggerOn = triggerOn;
    pXIncarnation->triggerOff = triggerOff;
	
	adjSpriteByType(pSprite);
}

void cdGetSeq(spritetype* pSprite) {
	
	dassert(xspriRangeIsFine(pSprite->extra));
	XSPRITE* pXSprite = &xsprite[pSprite->extra];
	
	short pic, xr, yr, pal;
	int nSeq = (pXSprite->data2 > 0) ? pXSprite->data2 : 11520;
	if (getSeqPrefs(nSeq, &pic, &xr, &yr, &pal))
	{
		if (pic >= 0) pSprite->picnum	= pic;
		if (xr >= 0)  pSprite->xrepeat	= (BYTE)xr;
		if (yr >= 0)  pSprite->yrepeat	= (BYTE)yr;
		if (pal >= 0) pSprite->pal		= (BYTE)pal;
	}
	
	pSprite->cstat &= ~kSprInvisible;

}

// this function allows to spawn new custom dude and inherit spawner settings,
// so custom dude can have different weapons, hp and so on...
spritetype* cdSpawn(spritetype* pSprite, int nDist) {

    spritetype* pSource = pSprite; XSPRITE* pXSource = &xsprite[pSource->extra];
    spritetype* pDude = actSpawnSprite(pSprite, 6); XSPRITE* pXDude = &xsprite[pDude->extra];

    int x, y, z = pSprite->z, nAngle = pSprite->ang, nType = kDudeModernCustom;

    if (nDist > 0) {
        x = pSprite->x + mulscale30r(Cos(nAngle), nDist);
        y = pSprite->y + mulscale30r(Sin(nAngle), nDist);
    } else {
        x = pSprite->x;
        y = pSprite->y;
    }

    pDude->type = (short)nType; pDude->ang = (short)nAngle;
	//pDude->x = x;
	//pDude->y = y;
	//pDude->z = z;
    
	setsprite(pDude->index, x, y, z);
    pDude->cstat |= 0x1101; 
	
	pDude->yvel = kDeleteReally;
	
    // inherit weapon, seq and sound settings.
    pXDude->data1 = pXSource->data1;
    pXDude->data2 = pXSource->data2;
    pXDude->data3 = pXSource->data3; // move sndStartId from data3 to sysData1
    pXDude->data3 = 0;

    // inherit movement speed.
    pXDude->busyTime = pXSource->busyTime;

    // inherit clipdist?
    if (pSource->clipdist > 0) pDude->clipdist = pSource->clipdist;

    pXDude->health = 100 << 4;
	pXDude->state = 1;

    if (pSource->flags & kModernTypeFlag1) {
        switch (pSource->type) {
        case kModernCustomDudeSpawn:
            //inherit pal?
            if (pDude->pal <= 0)
				pDude->pal = pSource->pal;

            // inherit spawn sprite trigger settings, so designer can count monsters.
            pXDude->txID = pXSource->txID;
            pXDude->command = pXSource->command;
            pXDude->triggerOn = pXSource->triggerOn;
            pXDude->triggerOff = pXSource->triggerOff;

            // inherit drop items
            pXDude->dropItem = pXSource->dropItem;

            // inherit required key so it can be dropped
            pXDude->key = pXSource->key;

            // inherit dude flags
            pXDude->dudeDeaf = pXSource->dudeDeaf;
            pXDude->dudeGuard = pXSource->dudeGuard;
            pXDude->dudeAmbush = pXSource->dudeAmbush;
            pXDude->dudeFlag4 = pXSource->dudeFlag4;
            pXDude->unused1 = pXSource->unused1;
            break;
        }
    }

    // inherit sprite size (useful for seqs with zero repeats)
    if (pSource->flags & kModernTypeFlag2) {
        pDude->xrepeat = cpysprite[pSource->index].xrepeat;
        pDude->yrepeat = cpysprite[pSource->index].yrepeat;
    }
	
	adjSpriteByType(pDude);
    return pDude;
}

// this function used by various new modern types.
void modernTypeTrigger(int destObjType, int destObjIndex, EVENT event) {

    if (event.type != EVOBJ_SPRITE) return;
    spritetype* pSource = &sprite[event.index];

    if (!xspriRangeIsFine(pSource->extra)) return;
    XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (destObjType) {
        case EVOBJ_SECTOR:
            if (!xsectRangeIsFine(sector[destObjIndex].extra)) return;
            break;
        case EVOBJ_WALL:
            if (!xwallRangeIsFine(wall[destObjIndex].extra)) return;
            break;
        case EVOBJ_SPRITE:
            if (!xspriRangeIsFine(sprite[destObjIndex].extra)) return;
            else if (sprite[destObjIndex].flags & kHitagFree) return;

            // allow redirect events received from some modern types.
            // example: it allows to spawn FX effect if event was received from kModernEffectGen
            // on many TX channels instead of just one.
            switch (sprite[destObjIndex].type) {
				case kModernRandomTX:
				case kModernSequentialTX:
					spritetype* pSpr = &sprite[destObjIndex]; XSPRITE* pXSpr = &xsprite[pSpr->extra];
					if (pXSpr->command != kCmdNotState) break; // no redirect mode detected
					else if (!pXSpr->locked) {
						switch (pSpr->type) {
							case kModernRandomTX:
								useRandomTx(pSpr, (COMMAND_ID)pXSource->command, FALSE); // set random TX id
								break;
							case kModernSequentialTX:
								if (pSpr->flags & kModernTypeFlag1) {
									seqTxSendCmdAll(pSpr, pSource->index, (COMMAND_ID)pXSource->command, TRUE);
									return;
								}
								useSequentialTx(pSpr, (COMMAND_ID)pXSource->command, FALSE); // set next TX id
								break;
						}
						if (pXSpr->txID > 0 && pXSpr->txID < 1024) {
							modernTypeSendCommand(pSource->index, pXSpr->txID, (COMMAND_ID)pXSource->command);
						}
						return;
					}
					break;
            }
            break; 
        default:
            return;
    }
	
	switch (pSource->type) {
        // allows teleport any sprite from any location to the source destination
        case kMarkerWarpDest:
			if (destObjType != EVOBJ_SPRITE) break;
            useTeleportTarget(pSource, &sprite[destObjIndex]);
            break;
        // damages xsprite via TX ID or xsprites in a sector
        case kModernSpriteDamager:
            switch (destObjType) {
                case EVOBJ_SPRITE:
                case EVOBJ_SECTOR:
                    useSpriteDamager(pSource, destObjType, destObjIndex);
                    break;
            }
            break;
		// can spawn any effect passed in data2 on it's or txID sprite
        case kModernEffectSpawner:
            if (destObjType != EVOBJ_SPRITE || pXSource->data2 < 0 || pXSource->data2 >= kFXMax) break;
            useEffectGen(pSource, &sprite[destObjIndex]);
            break;
        case kModernSeqSpawner:
            useSeqSpawnerGen(pSource, destObjType, destObjIndex);
            break;
        // change various properties
        case kModernObjPropChanger:
            usePropertiesChanger(pSource, destObjType, destObjIndex);
            break;
        // size and pan changer of sprite/wall/sector via TX ID
        case kModernObjSizeChanger:
            useObjResizer(pSource, destObjType, destObjIndex);
            break;
        // iterate data filed value of destination object
        case kModernObjDataAccum:
            useIncDecGen(pSource, destObjType, destObjIndex);
            break;
        // change data field value of destination object
        case kModernObjDataChanger:
            useDataChanger(pSource, destObjType, destObjIndex);
            break;
		// change picture and palette of TX ID object
        case kModernObjPicChanger:
            usePictureChanger(pSource, destObjType, destObjIndex);
            break;
        // change sector lighting dynamically
        case kModernSectFXChg:
            if (destObjType != EVOBJ_SECTOR) break;
            useSectorLigthChanger(pSource, &xsector[sector[destObjIndex].extra]);
            break;
        // updated vanilla sound gen that now allows to play sounds on TX ID sprites
        case kGenModernSound:
            if (destObjType != EVOBJ_SPRITE) break;
            useSoundGen(pXSource, &sprite[destObjIndex]);
            break;
	}
}

void nnExtResetGlobals() {

    // reset counters
    gProxySpritesCount = gSightSpritesCount = 0;
	gEventRedirectsUsed = FALSE;
	
    // fill arrays with negative values to avoid index 0 situation
    memset(gSightSpritesList, -1, sizeof(gSightSpritesList));   memset(gProxySpritesList, -1, sizeof(gProxySpritesList));

}

void nnExtInitModernStuff() {
	
	int j;
	nnExtResetGlobals();
	for (j = 0; j < kMaxSprites; j++) {
		
		if (sprite[j].statnum == kStatFree || (sprite[j].flags & 32) || sprite[j].extra <= 0)
			continue;
		
		spritetype* pSprite = &sprite[j];
		if (xsprite[pSprite->extra].reference != pSprite->index)
			continue;
		
        XSPRITE* pXSprite = &xsprite[pSprite->extra];  
		
		switch (pSprite->type) {
            case kModernRandomTX:
            case kModernSequentialTX:
                if (pXSprite->command == kCmdNotState) gEventRedirectsUsed = TRUE;
                break;
			case kDudeModernCustom:
				if (pXSprite->txID <= 0) break;
				for (int nSprite = headspritestat[kStatDude], found = 0; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
					XSPRITE* pXSpr = &xsprite[sprite[nSprite].extra];
					if (pXSpr->rxID != pXSprite->txID) continue;
					else if (found) ThrowError("\nCustom dude (TX ID %d):\nOnly one incarnation allowed per channel!", pXSprite->txID);
					ChangeSpriteStat(nSprite, kStatInactive);
					nSprite = headspritestat[kStatDude];
					found++;
				}
				break;
			// remove kStatItem status from random item generators
			case kModernRandom:
			case kModernRandom2:
				pSprite->cstat &= ~kSprBlock;
				pSprite->cstat |= kSprInvisible;
				ChangeSpriteStat(pSprite->index, kStatDecoration);
				pXSprite->target = pXSprite->command; // save the command so spawned item can inherit it
				pXSprite->command  = kCmdLink;  // generator itself can't send commands
				break;
		}
		
		// auto set going On and going Off if both are empty
		if (pXSprite->txID && !pXSprite->triggerOn && !pXSprite->triggerOff)
			pXSprite->triggerOn = pXSprite->triggerOff = 1;
		
		// check reserved statnums
		if (pSprite->statnum >= kStatModernBase && pSprite->statnum < kStatModernMax)
		{
			BOOL sysStat = TRUE;
			switch (pSprite->statnum) {
				case kStatModernStealthRegion:
					sysStat = (pSprite->type != kModernStealthRegion);
					break;
				case kStatModernDudeTargetChanger:
					sysStat = (pSprite->type != kModernDudeTargetChanger);
					break;
				case kStatModernCondition:
					sysStat = (pSprite->type != kModernCondition && pSprite->type != kModernConditionFalse);
					break;
				case kStatModernEventRedirector:
					sysStat = (pSprite->type != kModernRandomTX && pSprite->type != kModernSequentialTX);
					break;
				case kStatModernWindGen:
					sysStat = (pSprite->type != kModernWindGenerator);
					break;
				case kStatModernPlayerLinker:
				case kStatModernQavScene:
					sysStat = (pSprite->type != kModernPlayerControl);
					break;
			}

			if (sysStat)
				ThrowError("Sprite statnum %d on sprite #%d(type=%d) is in a range of reserved (%d - %d)!", pSprite->statnum, pSprite->index,  pSprite->type, kStatModernBase, kStatModernMax);
		}
		
		// the following trigger flags are senseless to have together
		if ((pXSprite->triggerTouch && (pXSprite->triggerProximity || pXSprite->triggerSight) && pXSprite->dudeLockout)
				|| (pXSprite->triggerTouch && pXSprite->triggerProximity && !pXSprite->triggerSight)) pXSprite->triggerTouch = 0;

		if (pXSprite->triggerProximity && pXSprite->triggerSight && pXSprite->dudeLockout)
			pXSprite->triggerProximity = 0;
		
		// very quick fix for floor sprites with Touch trigger flag if their Z is equals sector floorz / ceilgz
		if (pSprite->sectnum >= 0 && pXSprite->triggerTouch && (pSprite->cstat & kSprFloor))
		{
			if (pSprite->z == sector[pSprite->sectnum].floorz) pSprite->z--;
			else if (pSprite->z == sector[pSprite->sectnum].ceilingz) pSprite->z++;
		}
		
		// make Proximity flag work not just for dudes and things...
		if (pXSprite->triggerProximity && gProxySpritesCount < kMaxSuperXSprites) {
			switch (pSprite->statnum) {
				case kStatFX:       case kStatExplosion:            case kStatItem:
				case kStatPurge:        case kStatSpares:           case kStatFlare:
				case kStatInactive:     case kStatFree:             case kStatMarker:
				case kStatPathMarker:   case kStatThing:            case kStatDude:
				case 23:
					break;
				default:
					gProxySpritesList[gProxySpritesCount++] = pSprite->index;
					if (gProxySpritesCount == kMaxSuperXSprites)
						ThrowError("Max (%d) *additional* Proximity sprites reached!", kMaxSuperXSprites);
					break;
			}
		}

		// make Sight flag work not just for dudes and things...
		if (pXSprite->triggerSight && gSightSpritesCount < kMaxSuperXSprites) {
			switch (pSprite->statnum) {
				case kStatFX:       case kStatExplosion:            case kStatItem:
				case kStatPurge:        case kStatSpares:           case kStatFlare:
				case kStatInactive:     case kStatFree:             case kStatMarker:
				case kStatPathMarker:   case 23:
					break;
				default:
					gSightSpritesList[gSightSpritesCount++] = pSprite->index;
					if (gSightSpritesCount == kMaxSuperXSprites)
						ThrowError("Max (%d) Sight sprites reached!", kMaxSuperXSprites);
					break;
			}
		}
	}
	
}

void nnExtProcessSuperSprites() {
	
    // process additional proximity sprites
    if (gProxySpritesCount > 0) {
        for (int i = 0; i < gProxySpritesCount; i++) {
            if (sprite[gProxySpritesList[i]].extra < 0) continue;

            XSPRITE* pXProxSpr = &xsprite[sprite[gProxySpritesList[i]].extra];
            if (!pXProxSpr->triggerProximity || (!pXProxSpr->interruptable && pXProxSpr->state != pXProxSpr->restState) || pXProxSpr->locked == 1
                || pXProxSpr->isTriggered) continue;  // don't process locked or triggered sprites

            int x = sprite[gProxySpritesList[i]].x;	int y = sprite[gProxySpritesList[i]].y;
            int z = sprite[gProxySpritesList[i]].z;	int index = sprite[gProxySpritesList[i]].index;
            int sectnum = sprite[gProxySpritesList[i]].sectnum;
			dassert(sectnum >= 0 && sectnum < kMaxSectors);
			if (actCheckProximity(x, y, z, sectnum, 96))
				trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity);
        }
    }

    // process sight sprites (for players only)
    if (gSightSpritesCount > 0) {
        for (int i = 0; i < gSightSpritesCount; i++) {
            if (sprite[gSightSpritesList[i]].extra < 0) continue;

            XSPRITE* pXSightSpr = &xsprite[sprite[gSightSpritesList[i]].extra];
            if (!pXSightSpr->triggerSight || (!pXSightSpr->interruptable && pXSightSpr->state != pXSightSpr->restState) || pXSightSpr->locked == 1 ||
                pXSightSpr->isTriggered) continue; // don't process locked or triggered sprites

            int x = sprite[gSightSpritesList[i]].x;	int y = sprite[gSightSpritesList[i]].y;
            int z = sprite[gSightSpritesList[i]].z;	int index = sprite[gSightSpritesList[i]].index;
            int sectnum = sprite[gSightSpritesList[i]].sectnum;
			if (cansee(x, y, z, sectnum, posx, posy, posz, cursectnum))
				trTriggerSprite(index, pXSightSpr, kCmdSpriteSight);

        }
    }

}

/* void undoSectorLighting(int nSector)
{
	sectortype* pSect = &sector[nSector];
	if (!xsectRangeIsFine(pSect->extra))
		return;
	
	XSECTOR *pXSect = &xsector[pSect->extra];
	if (!pXSect->shade)
		return;
	
	int v4 = pXSect->shade;
	if (pXSect->shadeFloor)
	{
		pSect->floorshade -= v4;
		if (pXSect->coloredLights)
		{
			int nTemp = pXSect->floorpal2;
			pXSect->floorpal2 = pSect->floorpal;
			pSect->floorpal = nTemp;
		}
	}
	if (pXSect->shadeCeiling)
	{
		pSect->ceilingshade -= v4;
		if (pXSect->coloredLights)
		{
			int nTemp = pXSect->ceilpal2;
			pXSect->ceilpal2 = pSect->ceilingpal;
			pSect->ceilingpal = nTemp;
		}
	}
	if (pXSect->shadeWalls)
	{
		int nStartWall = pSect->wallptr;
		int nEndWall = nStartWall + pSect->wallnum;
		for (int j = nStartWall; j < nEndWall; j++)
		{
			wall[j].shade -= v4;
			if (pXSect->coloredLights)
			{
				wall[j].pal = pSect->floorpal;
			}
		}
	}
		
	pXSect->shade = 0;
	
}

void undoSectorLightingRX(int rx)
{
	int i;
	for (i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
	{
        if (rxBucket[i].type == 6)
			undoSectorLighting(rxBucket[i].index);
    }
} */

void useSectorLigthChanger(spritetype* pSource, XSECTOR* pXSector) {
    
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
    if (valueIsBetween(pXSource->data1, -1, 32767))
        pXSector->shadeWave = ClipHigh(pXSource->data1, 11);

    int oldAmplitude = pXSector->amplitude;
    if (valueIsBetween(pXSource->data2, -128, 128))
        pXSector->amplitude = pXSource->data2;

    if (valueIsBetween(pXSource->data3, -1, 32767))
        pXSector->shadeFreq = ClipHigh(pXSource->data3, 255);

    if (valueIsBetween(pXSource->data4, -1, 65535))
        pXSector->shadePhase = ClipHigh(pXSource->data4, 255);

    if (pSource->flags) {
        if (pSource->flags != kModernTypeFlag1) {
            
            pXSector->shadeAlways   = (pSource->flags & 0x0001) ? TRUE : FALSE;
            pXSector->shadeFloor    = (pSource->flags & 0x0002) ? TRUE : FALSE;
            pXSector->shadeCeiling  = (pSource->flags & 0x0004) ? TRUE : FALSE;
            pXSector->shadeWalls    = (pSource->flags & 0x0008) ? TRUE : FALSE;

        } else {

            pXSector->shadeAlways   = TRUE;

        }
    }
	
	//undoSectorLighting(pXSector->reference);
	
    // add to shadeList if amplitude was set to 0 previously
    if (oldAmplitude != pXSector->amplitude && shadeCount < kMaxXSectors) {

        BOOL found = FALSE;
        for (int i = 0; i < shadeCount; i++) {
            if (shadeList[i] != sector[pXSector->reference].extra) continue;
            found = TRUE;
            break;
        }

        if (!found)
            shadeList[shadeCount++] = sector[pXSector->reference].extra;
    }
}

void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource) {
    
	// let's first search for previously dropped items and remove it
    if (pXSource->dropItem > 0)
	{
        for (short nItem = headspritestat[kStatItem]; nItem >= 0; nItem = nextspritestat[nItem]) {
            spritetype* pItem = &sprite[nItem];
            if ((unsigned int)pItem->type == pXSource->dropItem && pItem->x == pSource->x && pItem->y == pSource->y && pItem->z == pSource->z) {
                gFX.fxSpawn((FX_ID)29, pSource->sectnum, pSource->x, pSource->y, pSource->z, 0);
                pItem->type = kSpriteDecoration;
                actPostSprite(nItem, kStatFree);
                break;
            }
        }
    }

    // then drop item
    spritetype* pDrop = randomDropPickupObject(pSource, pXSource->dropItem);
    if (pDrop != NULL)
	{
        clampSprite(pDrop);
    }
}

void useSoundGen(XSPRITE* pXSource, spritetype* pSprite) {
    
	int pitch = pXSource->data4 << 1; if (pitch < 2000) pitch = 0;
    sfxPlay3DSoundCP(pSprite, pXSource->data2, -1, 0, pitch, pXSource->data3);
	
}

void usePictureChanger(spritetype* pSource, int objType, int objIndex) {
    
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
	
    switch (objType) {
        case EVOBJ_SECTOR:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sector[objIndex].floorpicnum = (short)pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].ceilingpicnum = (short)pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sector[objIndex].floorpal = (char)pXSource->data3;

            if (valueIsBetween(pXSource->data4, -1, 65535))
                sector[objIndex].ceilingpal = (char)pXSource->data4;
            break;
        case EVOBJ_SPRITE:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sprite[objIndex].picnum = (short)pXSource->data1;

            if (pXSource->data2 >= 0) sprite[objIndex].shade = (char)((pXSource->data2 > 127) ? 127 : pXSource->data2);
            else if (pXSource->data2 < -1)
				sprite[objIndex].shade = (char)((pXSource->data2 < -127) ? -127 : pXSource->data2);

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sprite[objIndex].pal = (char)pXSource->data3;
            break;
        case EVOBJ_WALL:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                wall[objIndex].picnum = (short)pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                wall[objIndex].overpicnum = (short)pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                wall[objIndex].pal = (char)pXSource->data3;
            break;
    }
}

void useSpriteDamager(spritetype* pSource, int objType, int objIndex) {

	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
	
    sectortype* pSector = &sector[pSource->sectnum];

    int i;

    switch (objType) {
        case EVOBJ_SPRITE:
            damageSprites(pSource, &sprite[objIndex]);
            break;
        case EVOBJ_SECTOR:
            for (i = headspritesect[objIndex]; i != -1; i = nextspritesect[i]) {
                if (!IsDudeSprite(&sprite[i]) || !xspriRangeIsFine(sprite[i].extra))
                    continue;
				damageSprites(pSource, &sprite[i]);
            }
			
			if (cursectnum == objIndex)
				damageSprites(pSource, NULL);
           
		    break;
        case -1:
            for (i = headspritestat[kStatDude]; i != -1; i = nextspritestat[i]) {
                if (sprite[i].statnum != kStatDude) continue;
                switch (pXSource->data1) {
                    case 667:
                        if (IsPlayerSprite(&sprite[i])) continue;
                        damageSprites(pSource, &sprite[i]);
                        break;
                    case 668:
                        if (!IsPlayerSprite(&sprite[i])) continue;
                        damageSprites(pSource, NULL);
                        break;
                    default:
                        damageSprites(pSource, &sprite[i]);
                        break;
                }
            }
			
			switch (pXSource->data1) {
				case 667:
					break;
				default:
					damageSprites(pSource, NULL);
					break;
			}
			
            break;
    }
}

void damageSprites(spritetype* pSource, spritetype* pSprite) {
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
	
	if (pSprite != NULL) {
		dassert(xspriRangeIsFine(pSprite->extra));
		previewKillDude(pSprite, &xsprite[pSprite->extra]);
    }
	return;
}

void useDataChanger(spritetype* pSource, int objType, int objIndex) {
    
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (objType) {
        case EVOBJ_SECTOR:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
        case EVOBJ_SPRITE:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data2 != -1 && pXSource->data2 != 32767))
                setDataValueOfObject(objType, objIndex, 2, pXSource->data2);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data3 != -1 && pXSource->data3 != 32767))
                setDataValueOfObject(objType, objIndex, 3, pXSource->data3);

            if ((pSource->flags & kModernTypeFlag1) || pXSource->data4 != 65535)
                setDataValueOfObject(objType, objIndex, 4, pXSource->data4);
            break;
        case EVOBJ_WALL:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
    }
}

void useIncDecGen(spritetype* pSource, int objType, int objIndex) {
    
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];

    char buffer[5]; int data = -65535; short tmp = 0; int dataIndex = 0;
    sprintf(buffer, "%d", abs(pXSource->data1)); int len = strlen(buffer);

    for (int i = 0; i < len; i++) {
        dataIndex = (buffer[i] - 52) + 4;
        if ((data = getDataFieldOfObject(objType, objIndex, dataIndex)) == -65535) {
            previewMessage("\nWrong index of data (%c) for IncDec Gen #%d! Only 1, 2, 3 and 4 indexes allowed!\n", buffer[i], objIndex);
            continue;
        }

        if (pXSource->data2 < pXSource->data3) {

            data = ClipRange(data, pXSource->data2, pXSource->data3);
            if ((data += pXSource->data4) >= pXSource->data3) {
                switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data > pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2:
                    if (data > pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pSource)) break;
                    tmp = (short)pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                    break;
                case kModernTypeFlag3:
                    if (data > pXSource->data3) data = pXSource->data2;
                    break;
                }
            }

        } else if (pXSource->data2 > pXSource->data3) {

            data = ClipRange(data, pXSource->data3, pXSource->data2);
            if ((data -= pXSource->data4) <= pXSource->data3) {
                switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data < pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2:
                    if (data < pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pSource)) break;
                    tmp = (short)pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                    break;
                case kModernTypeFlag3:
                    if (data < pXSource->data3) data = pXSource->data2;
                    break;
                }
            }
        }

        pXSource->target = data;
        setDataValueOfObject(objType, objIndex, dataIndex, data);
    }

}

void useObjResizer(spritetype* pSource, int objType, int objIndex) {
    
	XSPRITE* pXSource = &xsprite[pSource->extra];
	switch (objType) {
		// for sectors
		case EVOBJ_SECTOR:
			if (valueIsBetween(pXSource->data1, -1, 32767))
				sector[objIndex].floorxpanning = (char)ClipRange(pXSource->data1, 0, 255);

			if (valueIsBetween(pXSource->data2, -1, 32767))
				sector[objIndex].floorypanning = (char)ClipRange(pXSource->data2, 0, 255);

			if (valueIsBetween(pXSource->data3, -1, 32767))
				sector[objIndex].ceilingxpanning = (char)ClipRange(pXSource->data3, 0, 255);

			if (valueIsBetween(pXSource->data4, -1, 65535))
				sector[objIndex].ceilingypanning = (char)ClipRange(pXSource->data4, 0, 255);
			break;
		case EVOBJ_SPRITE: {

			BOOL fit = FALSE;
			
			// resize by seq scaling
			if (pSource->flags & kModernTypeFlag1) {
					return;
					
			// resize by repeats
			} else {

				if (valueIsBetween(pXSource->data1, -1, 32767)) {
					sprite[objIndex].xrepeat = (char)ClipRange(pXSource->data1, 0, 255);
					fit = TRUE;
				}
				
				if (valueIsBetween(pXSource->data2, -1, 32767)) {
					sprite[objIndex].yrepeat = (char)ClipRange(pXSource->data2, 0, 255);
					fit = TRUE;
				}

			}

			if (valueIsBetween(pXSource->data3, -1, 32767))
				sprite[objIndex].xoffset = (char)ClipRange(pXSource->data3, 0, 255);

			if (valueIsBetween(pXSource->data4, -1, 65535))
				sprite[objIndex].yoffset = (char)ClipRange(pXSource->data4, 0, 255);
			break;
		}
		case EVOBJ_WALL:
			if (valueIsBetween(pXSource->data1, -1, 32767))
				wall[objIndex].xrepeat = (char)ClipRange(pXSource->data1, 0, 255);

			if (valueIsBetween(pXSource->data2, -1, 32767))
				wall[objIndex].yrepeat = (char)ClipRange(pXSource->data2, 0, 255);

			if (valueIsBetween(pXSource->data3, -1, 32767))
				wall[objIndex].xpanning = (char)ClipRange(pXSource->data3, 0, 255);

			if (valueIsBetween(pXSource->data4, -1, 65535))
				wall[objIndex].ypanning = (char)ClipRange(pXSource->data4, 0, 255);
			break;
    }

}

void usePropertiesChanger(spritetype* pSource, int objType, int objIndex) {
	
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (objType) {
        case EVOBJ_WALL: {
            walltype* pWall = &wall[objIndex]; int old = -1;

            // data3 = set wall hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) pWall->hitag = pWall->hitag |= (short)pXSource->data3;
                else pWall->hitag = (short)pXSource->data3;
            }

            // data4 = set wall cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                old = pWall->cstat;

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pWall->cstat = pWall->cstat |= (short)pXSource->data4; // relative
                else pWall->cstat = (short)pXSource->data4; // absolute

                // and hanlde exceptions
                if ((old & 0x2) && !(pWall->cstat & 0x2)) pWall->cstat |= 0x2; // kWallSwap
                if ((old & 0x4) && !(pWall->cstat & 0x4)) pWall->cstat |= 0x4; // kWallOrgBottom, kWallOrgOutside
                if ((old & 0x20) && !(pWall->cstat & 0x20)) pWall->cstat |= 0x20; // kWallOneWay

                if (old & 0xc000) {

                    if (!(pWall->cstat & 0xc000))
                        pWall->cstat |= 0xc000; // kWallMoveMask

                    if ((old & 0x0) && !(pWall->cstat & 0x0)) pWall->cstat |= 0x0; // kWallMoveNone
                    else if ((old & 0x4000) && !(pWall->cstat & 0x4000)) pWall->cstat |= 0x4000; // kWallMoveForward
                    else if ((old & 0x8000) && !(pWall->cstat & 0x8000)) pWall->cstat |= 0x8000; // kWallMoveReverse

                }
            }
        }
        break;
        case EVOBJ_SPRITE: {
            spritetype* pSprite = &sprite[objIndex]; BOOL thing2debris = FALSE;
            XSPRITE* pXSprite = &xsprite[pSprite->extra]; int old = -1;

            // data4 = sprite cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {

                old = pSprite->cstat;

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pSprite->cstat |= (short)pXSource->data4; // relative
                else pSprite->cstat = (short)pXSource->data4; // absolute

                // and handle exceptions
                if ((old & 0x1000) && !(pSprite->cstat & 0x1000)) pSprite->cstat |= 0x1000; //kSpritePushable
                if ((old & 0x80) && !(pSprite->cstat & 0x80)) pSprite->cstat |= 0x80; // kSprOrigin

                if (old & 0x6000) {

                    if (!(pSprite->cstat & 0x6000))
                        pSprite->cstat |= 0x6000; // kSprMoveMask

                    if ((old & 0x0) && !(pSprite->cstat & 0x0)) pSprite->cstat |= 0x0; // kSprMoveNone
                    else if ((old & 0x2000) && !(pSprite->cstat & 0x2000)) pSprite->cstat |= 0x2000; // kSprMoveForward, kSpriteMoveFloor
                    else if ((old & 0x4000) && !(pSprite->cstat & 0x4000)) pSprite->cstat |= 0x4000; // kSprMoveReverse, kSpriteMoveCeiling

                }

            }
        }
        break;
        case EVOBJ_SECTOR: {

            sectortype* pSector = &sector[objIndex];
            XSECTOR* pXSector = &xsector[sector[objIndex].extra];

            // data2 = sector visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].visibility = (BYTE)ClipRange(pXSource->data2, 0, 234);

            // data3 = sector ceil cstat
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].ceilingstat |= (short)pXSource->data3;
                else sector[objIndex].ceilingstat = (short)pXSource->data3;
            }

            // data4 = sector floor cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].floorstat |= (short)pXSource->data4;
                else sector[objIndex].floorstat = (short)pXSource->data4;
            }
        }
        break;
        // no TX id
        case -1:
            // data2 = global visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                visibility = ClipRange(pXSource->data2, 0, 4096);
        break;
    }

}

void useSectorWindGen(spritetype* pSource, sectortype* pSector) {

    dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
    XSECTOR* pXSector = NULL; int nXSector = 0;
    
    if (pSector != NULL) {
        
		pXSector = &xsector[pSector->extra];
        nXSector = sector[pXSector->reference].extra;
    } else if (xsectRangeIsFine(sector[pSource->sectnum].extra)) {
        pXSector = &xsector[sector[pSource->sectnum].extra];
        nXSector = sector[pXSector->reference].extra;
    } else {
        nXSector = dbInsertXSector(pSource->sectnum);
        pXSector = &xsector[nXSector]; pXSector->windAlways = 1;
    }
	
	dassert(xsectRangeIsFine(nXSector));
	
    if ((pSource->flags & kModernTypeFlag1))
        pXSector->panAlways = pXSector->windAlways = 1;

    int windVel = ClipRange(pXSource->data2, 0, 32767);
    switch (pXSource->data1) {
        default:
            pXSector->windVel = windVel;
            break;
        case 1:
        case 3:
			pXSector->windVel = nnExtRandom(0, windVel);
            break;
    }
    
    int ang = pSource->ang;
    if (pXSource->data4 <= 0) {
        switch (pXSource->data1) {
        case 2:
        case 3:
            while (pSource->ang == ang)
                pSource->ang = (short)(nnExtRandom(-kAng360, kAng360) & 2047);
            break;
        }
    }
    else if (pSource->cstat & 0x2000) pSource->ang += pXSource->data4;
    else if (pSource->cstat & 0x4000) pSource->ang -= pXSource->data4;
    else if (pXSource->target == 0) {
        if ((ang += pXSource->data4) >= kAng180) pXSource->target = 1;
        pSource->ang = (short)ClipHigh(ang, kAng180);
    } else {
        if ((ang -= pXSource->data4) <= -kAng180) pXSource->target = 0;
        pSource->ang = (short)ClipLow(ang, -kAng180);
    }

    pXSector->windAng = pSource->ang;

    if (pXSource->data3 > 0 && pXSource->data3 < 4) {
        switch (pXSource->data3) {
        case 1:
            pXSector->panFloor = TRUE;
            pXSector->panCeiling = FALSE;
            break;
        case 2:
            pXSector->panFloor = FALSE;
            pXSector->panCeiling = TRUE;
            break;
        case 3:
            pXSector->panFloor = TRUE;
            pXSector->panCeiling = TRUE;
            break;
        }

        int oldPan = pXSector->panVel;
        pXSector->panAngle = pXSector->windAng;
        pXSector->panVel = pXSector->windVel;

        // add to panList if panVel was set to 0 previously
        if (oldPan == 0 && pXSector->panVel != 0 && panCount < kMaxXSectors) {

            int i;
            for (i = 0; i < panCount; i++) {
                if (panList[i] != nXSector) continue;
                break;
            }

            if (i == panCount)
                panList[panCount++] = (short)nXSector;
        }

    }
}

void useSeqSpawnerGen(spritetype* pSource, int objType, int index) {

    dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
	if (pXSource->data2 > 0 && !gSysRes.Lookup(pXSource->data2, "SEQ")) {
        previewMessage("Missing sequence #%d", pXSource->data2);
        return;
    }

    
    switch (objType) {
        case EVOBJ_SECTOR:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(2, sector[index].extra);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqKill(1, sector[index].extra);
            } else {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 2, sector[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqSpawn(pXSource->data2, 1, sector[index].extra, -1);
            }
            return;
        case EVOBJ_WALL:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(0, wall[index].extra);
                if ((pXSource->data3 == 3 || pXSource->data3 == 2) && (wall[index].cstat & kWallMasked))
                    seqKill(4, wall[index].extra);
            } else {

                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 0, wall[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2) {

                    if (wall[index].nextwall < 0) {
                        if (pXSource->data3 == 3)
                            seqSpawn(pXSource->data2, 0, wall[index].extra, -1);

                    } else {
                        if (!(wall[index].cstat & kWallMasked))
                            wall[index].cstat |= kWallMasked;

                        seqSpawn(pXSource->data2, 4, wall[index].extra, -1);
                    }
                }
            }
            return;
        case EVOBJ_SPRITE:
            if (pXSource->data2 <= 0) seqKill(3, sprite[index].extra);
            else if (sectRangeIsFine(sprite[index].sectnum)) {
                    if (pXSource->data3 > 0) {
                        int nSprite = InsertSprite(sprite[index].sectnum, 0);
                        int top, bottom; GetSpriteExtents(&sprite[index], &top, &bottom);
                        sprite[nSprite].x = sprite[index].x;
                        sprite[nSprite].y = sprite[index].y;
						
						sprite[nSprite].yvel = kDeleteReally;
                        
						switch (pXSource->data3) {
                            default:
                                sprite[nSprite].z = sprite[index].z;
                                break;
                            case 2:
                                sprite[nSprite].z = bottom;
                                break;
                            case 3:
                                sprite[nSprite].z = top;
                                break;
                            case 4:
                                sprite[nSprite].z = sprite[index].z + ((tilesizy[sprite[index].picnum] / 2) + panm[sprite[index].picnum].ycenter);
                                break;
                            case 5:
                            case 6:
                                if (!sectRangeIsFine(sprite[index].sectnum)) sprite[nSprite].z = top;
                                else sprite[nSprite].z = (pXSource->data3 == 5) ? sector[sprite[nSprite].sectnum].floorz : sector[sprite[nSprite].sectnum].ceilingz;
                                break;
                        }
                        
                        if (nSprite >= 0) {
                            
                            int nXSprite = dbInsertXSprite(nSprite);
                            seqSpawn(pXSource->data2, 3, nXSprite, -1);
                            if (pSource->flags & kModernTypeFlag1) {

                                sprite[nSprite].pal = pSource->pal;
                                sprite[nSprite].shade = pSource->shade;
                                sprite[nSprite].xrepeat = pSource->xrepeat;
                                sprite[nSprite].yrepeat = pSource->yrepeat;
                                sprite[nSprite].xoffset = pSource->xoffset;
                                sprite[nSprite].yoffset = pSource->yoffset;

                            }

                            if (pSource->flags & kModernTypeFlag2) {
                                
                                sprite[nSprite].cstat |= pSource->cstat;

                            }

                            // should be: the more is seqs, the shorter is timer
                            evPost(nSprite, EVOBJ_SPRITE, 1000, kCallbackRemoveSpecial);
                        }
                    } else {

                        seqSpawn(pXSource->data2, 3, sprite[index].extra, -1);

                    }
                    if (pXSource->data4 > 0)
                        sfxPlay3DSound(&sprite[index], pXSource->data4, -1, 0);
            }
            return;
    }
}

void useEffectGen(spritetype* pSource, spritetype* pSprite) {
    
	XSPRITE* pXSource = &xsprite[pSource->extra];
	if (pSprite == NULL) pSprite = pSource;
	int fxId = (pXSource->data3 <= 0) ? pXSource->data2 : pXSource->data2 + Random(pXSource->data3 + 1);
    if (xspriRangeIsFine(pSprite->extra) && valueIsBetween(fxId, 0, kFXMax)) {
        int pos, top, bottom; GetSpriteExtents(pSprite, &top, &bottom);
        spritetype* pEffect = NULL;

        // select where exactly effect should be spawned
        switch (pXSource->data4) {
            case 1:
                pos = bottom;
                break;
            case 2: // middle
                pos = pSprite->z + ((tilesizy[pSprite->picnum] / 2) + panm[pSprite->picnum].ycenter);
                break;
            case 3:
            case 4:
                if (!sectRangeIsFine(pSprite->sectnum)) pos = top;
                else pos = (pXSource->data4 == 3) ? sector[pSprite->sectnum].floorz : sector[pSprite->sectnum].ceilingz;
                break;
            default:
                pos = top;
                break;
        }

        if ((pEffect = gFX.fxSpawn((FX_ID)fxId, pSprite->sectnum, pSprite->x, pSprite->y, pos, 0)) != NULL) {

            pEffect->owner = pSource->index;

            if (pSource->flags & kModernTypeFlag1) {
                pEffect->pal = pSource->pal;
                pEffect->xoffset = pSource->xoffset;
                pEffect->yoffset = pSource->yoffset;
                pEffect->xrepeat = pSource->xrepeat;
                pEffect->yrepeat = pSource->yrepeat;
                pEffect->shade = pSource->shade;
            }

            if (pSource->flags & kModernTypeFlag2) {
                pEffect->cstat = pSource->cstat;
                if (pEffect->cstat & kSprInvisible)
                    pEffect->cstat &= ~kSprInvisible;
            }

            if (pEffect->cstat & kSprOneSided)
                pEffect->cstat &= ~kSprOneSided;

        }
    }
}

BOOL txIsRanged(XSPRITE* pXSource) {
    if (pXSource->data1 > 0 && pXSource->data2 <= 0 && pXSource->data3 <= 0 && pXSource->data4 > 0) {
        if (pXSource->data1 > pXSource->data4) {
            // data1 must be less than data4
            int tmp = pXSource->data1; pXSource->data1 = pXSource->data4;
            pXSource->data4 = tmp;
        }
        return TRUE;
    }
    return FALSE;
}

void seqTxSendCmdAll(spritetype* pSource, int nIndex, COMMAND_ID cmd, BOOL modernSend) {
    
    XSPRITE* pXSource = &xsprite[pSource->extra];
	BOOL ranged = txIsRanged(pXSource);
    if (ranged) {
        for (pXSource->txID = pXSource->data1; pXSource->txID <= pXSource->data4; pXSource->txID++) {
            if (pXSource->txID <= 0 || pXSource->txID >= 1024) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    } else {
        for (int i = 0; i <= 3; i++) {
            pXSource->txID = GetDataVal(pSource, i);
            if (pXSource->txID <= 0 || pXSource->txID >= 1024) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    }
    
    pXSource->txID = pXSource->target = 0;
    return;
}


void useSequentialTx(spritetype* pSource, COMMAND_ID cmd, BOOL setState) {
    
    dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
    BOOL range = txIsRanged(pXSource); int cnt = 3; int tx = 0;

    if (range) {
        
        // make sure sysData is correct as we store current index of TX ID here.
        if (pXSource->target < pXSource->data1) pXSource->target = pXSource->data1;
        else if (pXSource->target > pXSource->data4) pXSource->target = pXSource->data4;

    } else {
        
        // make sure sysData is correct as we store current index of data field here.
        if (pXSource->target > 3) pXSource->target = 0;
        else if (pXSource->target < 0) pXSource->target = 3;

    }

    switch (cmd) {
        case kCmdOff:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->target-- < 0) pXSource->target = 3;
                    if ((tx = GetDataVal(pSource, pXSource->target)) <= 0) continue;
                    else break;
                }
            } else {
                if (--pXSource->target < pXSource->data1) pXSource->target = pXSource->data4;
                tx = pXSource->target;
            }
            break;
        default:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->target > 3) pXSource->target = 0;
                    if ((tx = GetDataVal(pSource, pXSource->target++)) <= 0) continue;
                    else break;
                }
            } else {
                tx = pXSource->target;
                if (pXSource->target >= pXSource->data4) {
                    pXSource->target = pXSource->data1;
                    break;
                }
                pXSource->target++;
            }
            break;
    }

    pXSource->txID = (tx > 0 && tx < 1024) ? tx : 0;
    if (setState)
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
        //evSend(pSource->index, EVOBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);
}

void useRandomTx(spritetype* pSource, COMMAND_ID cmd, BOOL setState) {
	
	cmd = (COMMAND_ID)cmd;
	XSPRITE* pXSource = &xsprite[pSource->extra];
    int tx = 0; int maxRetries = kMaxRandomizeRetries;
    
    if (txIsRanged(pXSource)) {
        while (maxRetries-- >= 0) {
            if ((tx = nnExtRandom(pXSource->data1, pXSource->data4)) != pXSource->txID)
                break;
        }
    } else {
        while (maxRetries-- >= 0) {
            if ((tx = randomGetDataValue(pXSource, kRandomizeTX)) > 0 && tx != pXSource->txID)
                break;
        }
    }

    pXSource->txID = (tx > 0 && tx < 1024) ? tx : 0;
    if (setState) {
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
	}
}

void useTeleportTarget(spritetype* pSource, spritetype* pSprite) {
    
	if (!sectRangeIsFine(pSource->sectnum))
		return;
	
	dassert(xspriRangeIsFine(pSource->extra));
	XSPRITE* pXSource = &xsprite[pSource->extra];
    XSECTOR* pXSector = (sector[pSource->sectnum].extra > 0) ? &xsector[sector[pSource->sectnum].extra] : NULL;
	BOOL isDude = FALSE;

	if (pSprite == NULL) {
		
		isDude = TRUE;
		posx = pSource->x; posy = pSource->y; posz = pSource->z;
		if (cursectnum != pSource->sectnum)
			updatesector(posx, posy, &cursectnum);
		
		if (cursectnum >= 0) {
			int height = kensplayerheight;
			if (posz > sector[cursectnum].floorz - height) posz = sector[cursectnum].floorz - height;
			if (posz < sector[cursectnum].ceilingz) posz = sector[cursectnum].ceilingz;
		}
		
		if (pXSource->data2 == 1)
			ang = pSource->ang;
		
		if (pXSource->data3 == 1)
			vel = svel = 0;
		
	} else {
		
		isDude = IsDudeSprite(pSprite);
		pSprite->x = pSource->x; pSprite->y = pSource->y; pSprite->z = pSource->z;

		if (pSprite->sectnum != pSource->sectnum)
			changespritesect(pSprite->index, pSource->sectnum);
		
		// make sure sprites aren't in the floor or ceiling
		clampSprite(pSprite);
		
		if (pXSource->data2 == 1)
			pSprite->ang = pSource->ang;

		if (pXSource->data3 == 1)
			xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
		
	}

    if (pXSector) {
        if (pXSector->triggerEnter && isDude && !pXSector->dudeLockout)
            trTriggerSector(pSource->sectnum, pXSector, kCmdSectorEnter);
    }

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSource, pXSource->data4, -1, 0);

}

void useUniMissileGen(spritetype* pSprite) {

    dassert(xspriRangeIsFine(pSprite->extra));
	XSPRITE* pXSprite = &xsprite[pSprite->extra]; int dx = 0, dy = 0, dz = 0;
    if (pXSprite->data1 < kMissileBase || pXSprite->data1 >= kMissileMax)
        return;

    if (pSprite->cstat & 32) {
        if (pSprite->cstat & 8) dz = 0x4000;
        else dz = -0x4000;
    } else {
        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = pXSprite->data3 << 6; // add slope controlling
        if (dz > 0x10000) dz = 0x10000;
        else if (dz < -0x10000) dz = -0x10000;
    }

    spritetype* pMissile = NULL;
    pMissile = actFireMissile(pSprite, 0, 0, dx, dy, dz, pXSprite->data1);
    if (pMissile != NULL) {

        // inherit some properties of the generator
        if (pSprite->flags & kModernTypeFlag1) {

            pMissile->xrepeat = pSprite->xrepeat;
            pMissile->yrepeat = pSprite->yrepeat;

            pMissile->pal = pSprite->pal;
            pMissile->shade = pSprite->shade;

        }

        // add velocity controlling
        if (pXSprite->data2 > 0) {

            int velocity = pXSprite->data2 << 12;
            xvel[pMissile->index] = mulscale(velocity, dx, 14);
            yvel[pMissile->index] = mulscale(velocity, dy, 14);
            zvel[pMissile->index] = mulscale(velocity, dz, 14);

        }

        // add bursting for missiles
        if (pMissile->type != kMissileFlareAlt && pXSprite->data4 > 0)
            evPost(pMissile->index, 3, ClipHigh(pXSprite->data4, 500), kCallbackMissileBurst);

    }

}

void callbackUniMissileBurst(int nSprite)
{
   
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    spritetype* pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;

    for (int i = 0; i < 8; i++)
    {
        
		spritetype* pBurst = actSpawnSprite(pSprite, 5);
		if (!pBurst)
			ThrowError("Failed missile bursting");
		
        pBurst->type = pSprite->type;
        pBurst->shade = pSprite->shade;
        pBurst->picnum = pSprite->picnum;

        pBurst->cstat = pSprite->cstat;
        if ((pBurst->cstat & kSprBlock)) {
            pBurst->cstat &= ~kSprBlock; // we don't want missiles impact each other
            evPost(pBurst->index, 3, 100, kCallbackMissileSpriteBlock); // so set blocking flag a bit later
        }

        pBurst->pal = pSprite->pal;
        pBurst->clipdist = (char)(pSprite->clipdist >> 2);
        pBurst->flags = pSprite->flags;
        pBurst->xrepeat = (char)(pSprite->xrepeat >> 1);
        pBurst->yrepeat = (char)(pSprite->yrepeat >> 1);
        pBurst->ang = (short)((pSprite->ang + missileInfo[pSprite->type - kMissileBase].angleOfs) & 2047);
        pBurst->owner = pSprite->owner;
		
		pBurst->yvel = kDeleteReally;
        
		actBuildMissile(pBurst, pBurst->extra, pSprite->index);

        int nAngle2 = (i << 11) / 8;
        int dx = 0;
        int dy = mulscale30r(nRadius, Sin(nAngle2));
        int dz = mulscale30r(nRadius, -Cos(nAngle2));
        if (i & 1)
        {
            dy >>= 1;
            dz >>= 1;
        }
        RotateVector(&dx, &dy, nAngle);
        xvel[pBurst->index] += dx;
        yvel[pBurst->index] += dy;
        zvel[pBurst->index] += dz;
        evPost(pBurst->index, 3, 960, kCallbackRemoveSpecial);
    }
    
	evPost(nSprite, 3, 0, kCallbackRemoveSpecial);
}

void callbackMakeMissileBlocking(int nSprite) // 23
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    sprite[nSprite].cstat |= kSprBlock;
}

int GetDataVal(spritetype* pSprite, int data) {
    dassert(xspriRangeIsFine(pSprite->extra));
    
    switch (data) {
        case 0: return xsprite[pSprite->extra].data1;
        case 1: return xsprite[pSprite->extra].data2;
        case 2: return xsprite[pSprite->extra].data3;
        case 3: return xsprite[pSprite->extra].data4;
    }

    return -1;
}

int nnExtRandom(int a, int b) {
    //if (!gAllowTrueRandom)
		return Random(((b + 1) - a)) + a;
    // used for better randomness in single player
    //std::uniform_int_distribution<int> dist_a_b(a, b);
   // return dist_a_b(gStdRandom);
}

// tries to get random data field of sprite
int randomGetDataValue(XSPRITE* pXSprite, int randType) {
    if (pXSprite == NULL) return -1;
    int random = 0; int bad = 0; int maxRetries = kMaxRandomizeRetries;

    int rData[4];
    rData[0] = pXSprite->data1; rData[2] = pXSprite->data3;
    rData[1] = pXSprite->data2; rData[3] = pXSprite->data4;
    // randomize only in case if at least 2 data fields fits.
    for (int i = 0; i < 4; i++) {
        switch (randType) {
        case kRandomizeItem:
            if (rData[i] >= kItemWeaponBase && rData[i] < kItemMax) break;
            else bad++;
            break;
        case kRandomizeDude:
            if (rData[i] >= kDudeBase && rData[i] < kDudeMax) break;
            else bad++;
            break;
        case kRandomizeTX:
            if (rData[i] > 0 && rData[i] < 1024) break;
            else bad++;
            break;
        default:
            bad++;
            break;
        }
    }

    if (bad < 3) {
        // try randomize few times
        while (maxRetries > 0) {
            random = nnExtRandom(0, 3);
            if (rData[random] > 0) return rData[random];
            else maxRetries--;
        }
    }

    return -1;
}

int listTx(spritetype* pRedir, int tx) {
    dassert(xspriRangeIsFine(pRedir->extra));
	XSPRITE* pXRedir = &xsprite[pRedir->extra];

	if (txIsRanged(pXRedir)) {
        if (tx == -1) tx = pXRedir->data1;
        else if (tx < pXRedir->data4) tx++;
        else tx = -1;
    } else {
        if (tx == -1) {
            for (int i = 0; i <= 3; i++) {
                if ((tx = GetDataVal(pRedir, i)) <= 0) continue;
                else return tx;
            }
        } else {
            int saved = tx; BOOL savedFound = FALSE;
            for (int i = 0; i <= 3; i++) {
                tx = GetDataVal(pRedir, i);
                if (savedFound && tx > 0) return tx;
                else if (tx != saved) continue;
                else savedFound = TRUE;
            }
        }
        
        tx = -1;
    }

    return tx;
}

spritetype* evrIsRedirector(int nSprite) {
    if (spriRangeIsFine(nSprite)) {
        switch (sprite[nSprite].type) {
        case kModernRandomTX:
        case kModernSequentialTX:
            if (xspriRangeIsFine(sprite[nSprite].extra) && xsprite[sprite[nSprite].extra].command == kCmdNotState
                && !xsprite[sprite[nSprite].extra].locked) return &sprite[nSprite];
            break;
        }
    }

    return NULL;
}

spritetype* evrListRedirectors(int objType, int objXIndex, spritetype* pRedir, int* tx) {
	
	if (pRedir != NULL)
		dassert(xspriRangeIsFine(pRedir->extra));
	//XSPRITE* pXRedir = &xsprite[pRedir->extra];
	
	if (!gEventRedirectsUsed) return NULL;
    else if (pRedir && (*tx = listTx(pRedir, *tx)) != -1)
        return pRedir;

    int id = 0;
    switch (objType) {
        case EVOBJ_SECTOR:
            if (!xsectRangeIsFine(objXIndex)) return NULL;
            id = xsector[objXIndex].txID;
            break;
        case EVOBJ_SPRITE:
            if (!xspriRangeIsFine(objXIndex)) return NULL;
            id = xsprite[objXIndex].txID;
            break;
        case EVOBJ_WALL:
            if (!xwallRangeIsFine(objXIndex)) return NULL;
            id = xwall[objXIndex].txID;
            break;
        default:
            return NULL;
    }

    int nIndex = (pRedir) ? pRedir->index : -1; BOOL prevFound = FALSE;
    for (int i = bucketHead[id]; i < bucketHead[id + 1]; i++) {
        if (rxBucket[i].type != EVOBJ_SPRITE) continue;
        spritetype* pSpr = evrIsRedirector(rxBucket[i].index);
        if (!pSpr) continue;
        
		XSPRITE* pXSpr = &xsprite[pSpr->extra];
		if (prevFound || nIndex == -1) { *tx = listTx(pSpr, *tx); return pSpr; }
        else if (nIndex != pSpr->index) continue;
        else prevFound = TRUE;
    }

    *tx = -1;
    return NULL;
}

int getDataFieldOfObject(int objType, int objIndex, int dataIndex) {
    int data = -65535;
    switch (objType) {
        case EVOBJ_SPRITE:
            switch (dataIndex) {
                case 1:  return xsprite[sprite[objIndex].extra].data1;
                case 2:  return xsprite[sprite[objIndex].extra].data2;
                case 3:  return xsprite[sprite[objIndex].extra].data3;
                case 4:  return xsprite[sprite[objIndex].extra].data4;
                default: return data;
            }
        case EVOBJ_SECTOR: return xsector[sector[objIndex].extra].data;
        case EVOBJ_WALL: return xwall[wall[objIndex].extra].data;
        default: return data;
    }
}

BOOL setDataValueOfObject(int objType, int objIndex, int dataIndex, int value) {
    switch (objType) {
        case EVOBJ_SPRITE: {
            XSPRITE* pXSprite = &xsprite[sprite[objIndex].extra];

            // exceptions
            if (IsDudeSprite(&sprite[objIndex]) && pXSprite->health <= 0) return TRUE;
            switch (sprite[objIndex].type) {
                case 425:
                case 426:
                case 427:
                    return TRUE;
            }

            switch (dataIndex) {
                case 1:
                    xsprite[sprite[objIndex].extra].data1 = value;
                    switch (sprite[objIndex].type) {
                        case 22:
                            if (value == xsprite[sprite[objIndex].extra].data2) SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 1);
                            else SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 0);
                            break;
                    }
                    return TRUE;
                case 2:
                    xsprite[sprite[objIndex].extra].data2 = value;
                    switch (sprite[objIndex].type) {
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
							cdGetSeq(&sprite[objIndex]);
                            break;
                    }
                    return TRUE;
                case 3:
                    xsprite[sprite[objIndex].extra].data3 = value;
                    return TRUE;
                case 4:
                    xsprite[sprite[objIndex].extra].data4 = value;
                    return TRUE;
                default:
                    return FALSE;
            }
        }
        case EVOBJ_SECTOR:
            xsector[sector[objIndex].extra].data = value;
            return TRUE;
        case EVOBJ_WALL:
            xwall[wall[objIndex].extra].data = value;
            return TRUE;
        default:
            return FALSE;
    }
}

void seqSpawnerOffSameTx(spritetype* pSource) {
	
	XSPRITE* pXSource = &xsprite[pSource->extra];
	
    for (int i = 0; i < kMaxXSprites; i++) {
        if (xsprite[i].reference != i || sprite[xsprite[i].reference].statnum >= kStatFree)
			continue;
		
        XSPRITE* pXSprite = &xsprite[i];
		if (xsprite[i].reference != pSource->index && spriRangeIsFine(xsprite[i].reference)) {
            if (pSource->type != kModernSeqSpawner) continue;
            else if (pXSprite->txID == pXSource->txID && pXSprite->state == 1) {
                evKill(xsprite[i].reference, EVOBJ_SPRITE);
                pXSprite->state = 0;
            }
        }

    }

}

// this function spawns random dude using dudeSpawn
spritetype* randomSpawnDude(spritetype* pSource) {
    spritetype* pSprite2 = NULL; short selected = -1;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if ((selected = (short)randomGetDataValue(pXSource, kRandomizeDude)) > 0)
            pSprite2 = actSpawnDude(pSource, selected, -1, 0);
    }
    return pSprite2;
}

// this function stops wind on all TX sectors affected by WindGen after it goes off state.
void windGenStopWindOnSectors(spritetype* pSource) {
   
    XSPRITE* pXSource = &xsprite[pSource->extra];
    if (pXSource->txID <= 0 && xsectRangeIsFine(sector[pSource->sectnum].extra)) {
        xsector[sector[pSource->sectnum].extra].windVel = 0;
        return;
    }

    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type != EVOBJ_SECTOR) continue;
        XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
        if ((pXSector->state == 1 && !pXSector->windAlways)
            || ((pSource->flags & kModernTypeFlag1) && !(pSource->flags & kModernTypeFlag2))) {
                pXSector->windVel = 0;
        }
    }
    
    // check redirected TX buckets
    int rx = -1; spritetype* pRedir = NULL; //XSPRITE* pXRedir = NULL;
    while ((pRedir = evrListRedirectors(EVOBJ_SPRITE, pSource->extra, pRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            if (rxBucket[i].type != EVOBJ_SECTOR) continue;
            XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
            if ((pXSector->state == 1 && !pXSector->windAlways) || (pSource->flags & kModernTypeFlag2))
                pXSector->windVel = 0;
        }
    }
}

// this function checks if all TX objects have the same value
BOOL incDecGoalValueIsReached(spritetype* pSprite) {
    
	XSPRITE* pXSprite = &xsprite[pSprite->extra];
    if (pXSprite->data3 != pXSprite->target) return FALSE;
    char buffer[5]; sprintf(buffer, "%d", abs(pXSprite->data1)); int len = strlen(buffer); int rx = -1;
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type == EVOBJ_SPRITE && evrIsRedirector(rxBucket[i].index)) continue;
        for (int a = 0; a < len; a++) {
            if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                return FALSE;
        }
    }

    spritetype* pRedir = NULL; //XSPRITE* pXRedir = NULL; // check redirected TX buckets
    while ((pRedir = evrListRedirectors(EVOBJ_SPRITE, pSprite->extra, pRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            for (int a = 0; a < len; a++) {
                if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                    return FALSE;
            }
        }
    }
    
    return TRUE;
}


int sectorInMotion(int nSector) {
 
    for (int i = 0; i < kMaxBusyCount; i++) {
        if (gBusy->at0 == nSector) return i;
    }

    return -1;
}

void sectorKillSounds(int nSector) {
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite]) {
        if (sprite[nSprite].type != kSoundSector) continue;
        sfxKill3DSound(&sprite[nSprite]);
    }
}

void sectorPauseMotion(int nSector) {

    if (!xsectRangeIsFine(sector[nSector].extra)) return;
    XSECTOR* pXSector = &xsector[sector[nSector].extra];
    pXSector->unused1 = 1;
    
    evKill(nSector, EVOBJ_SECTOR);

    sectorKillSounds(nSector);
    if ((pXSector->busy == 0 && !pXSector->state) || (pXSector->busy == 65536 && pXSector->state))
        SectorEndSound(nSector, xsector[sector[nSector].extra].state);
    
    return;
}

void sectorContinueMotion(int nSector, EVENT event) {
    
    if (!xsectRangeIsFine(sector[nSector].extra)) return;
    else if (gBusyCount >= kMaxBusyCount)
	{
        previewMessage("Failed to continue motion for sector #%d. Max (%d) busy objects count reached!", nSector, kMaxBusyCount);
        return;
    }

    XSECTOR* pXSector = &xsector[sector[nSector].extra];
    pXSector->unused1 = 0;
    
    int busyTimeA = pXSector->busyTimeA;    int waitTimeA = pXSector->waitTimeA;
    int busyTimeB = pXSector->busyTimeB;    int waitTimeB = pXSector->waitTimeB;
    if (sector[nSector].type == kSectorPath) {
        if (!spriRangeIsFine(pXSector->marker0)) return;
        busyTimeA = busyTimeB = xsprite[sprite[pXSector->marker0].extra].busyTime;
        waitTimeA = waitTimeB = xsprite[sprite[pXSector->marker0].extra].waitTime;
    }
    
    if (!pXSector->interruptable && event.cmd != kCmdSectorMotionContinue
        && ((!pXSector->state && pXSector->busy) || (pXSector->state && pXSector->busy != 65536))) {
            
            event.cmd = kCmdSectorMotionContinue;

    } else if (event.cmd == kCmdToggle) {
        
        event.cmd = (pXSector->state) ? kCmdOn : kCmdOff;

    }

    //viewSetSystemMessage("%d / %d", pXSector->busy, pXSector->state);

    int nDelta = 1;
    switch (event.cmd) {
        case kCmdOff:
            if (pXSector->busy == 0) {
                if (pXSector->reTriggerB && waitTimeB) evPost(nSector, EVOBJ_SECTOR, (waitTimeB * 120) / 10, kCmdOff);
                return;
            }
            pXSector->state = 1;
            nDelta = 65536 / ClipLow((busyTimeB * 120) / 10, 1);
            break;
        case kCmdOn:
            if (pXSector->busy == 65536) {
                if (pXSector->reTriggerA && waitTimeA) evPost(nSector, EVOBJ_SECTOR, (waitTimeA * 120) / 10, kCmdOn);
                return;
            }
            pXSector->state = 0;
            nDelta = 65536 / ClipLow((busyTimeA * 120) / 10, 1);
            break;
        case kCmdSectorMotionContinue:
            nDelta = 65536 / ClipLow((((pXSector->state) ? busyTimeB : busyTimeA) * 120) / 10, 1);
            break;
    }

    //bool crush = pXSector->Crush;
    int busyFunc = BUSYID_0;
    switch (sector[nSector].type) {
        case kSectorZMotion:
            busyFunc = BUSYID_2;
            break;
        case kSectorZMotionSprite:
            busyFunc = BUSYID_1;
            break;
        case kSectorSlideMarked:
        case kSectorSlide:
            busyFunc = BUSYID_3;
            break;
        case kSectorRotateMarked:
        case kSectorRotate:
            busyFunc = BUSYID_4;
            break;
        case kSectorRotateStep:
            busyFunc = BUSYID_5;
            break;
        case kSectorPath:
            busyFunc = BUSYID_7;
            break;
        default:
            ThrowError("Unsupported sector type %d", sector[nSector].type);
            break;
    }

    SectorStartSound(nSector, pXSector->state);
    nDelta = (pXSector->state) ? -nDelta : nDelta;
    gBusy[gBusyCount].at0 = nSector;
    gBusy[gBusyCount].at4 = nDelta;
    gBusy[gBusyCount].at8 = pXSector->busy;
    gBusy[gBusyCount].atc = (BUSYID)busyFunc;
    gBusyCount++;
    return;

}

spritetype* randomDropPickupObject(spritetype* pSource, short prevItem) {

    spritetype* pSprite2 = NULL; int selected = -1; int maxRetries = 9;
    
	if (xspriRangeIsFine(pSource->extra))
	{
        XSPRITE* pXSource = &xsprite[pSource->extra];
        while ((selected = randomGetDataValue(pXSource, kRandomizeItem)) == prevItem) if (maxRetries-- <= 0) break;
        if (selected > 0)
		{
            pSprite2 = actDropObject(pSource, selected);
            if (pSprite2 != NULL)
			{
                pXSource->dropItem = pSprite2->type; // store dropped item type in dropItem
                pSprite2->x = pSource->x;
                pSprite2->y = pSource->y;
                pSprite2->z = pSource->z;

                if ((pSource->flags & kModernTypeFlag1) && (pXSource->txID > 0 || (pXSource->txID != 3 && pXSource->lockMsg > 0)) &&
                    dbInsertXSprite(pSprite2->index) > 0) {

                    XSPRITE* pXSprite2 = &xsprite[pSprite2->extra];

                    // inherit spawn sprite trigger settings, so designer can send command when item picked up.
                    pXSprite2->txID = pXSource->txID;
                    pXSprite2->command = pXSource->target;
                    pXSprite2->triggerOn = pXSource->triggerOn;
                    pXSprite2->triggerOff = pXSource->triggerOff;

                    pXSprite2->triggerPickup = true;

                }
            }
        }
    }
	
    return pSprite2;
}

bool modernTypeOperateSector(int nSector, sectortype* pSector, XSECTOR* pXSector, EVENT event) {

    if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock) {
        
        switch (event.cmd) {
            case kCmdLock:
                pXSector->locked = 1;
                break;
            case kCmdUnlock:
                pXSector->locked = 0;
                break;
            case kCmdToggleLock:
                pXSector->locked = pXSector->locked ^ 1;
                break;
        }

        switch (pSector->type) {
            case kSectorCounter:
                if (pXSector->locked != 1) break;
                SetSectorState(nSector, pXSector, 0);
				evPost(nSector, 6, 0, kCallbackCounterCheck);
                break;
        }

        return true;
    
    // continue motion of the paused sector
    } else if (pXSector->unused1) {
        
        switch (event.cmd) {
            case kCmdOff:
            case kCmdOn:
            case kCmdToggle:
            case kCmdSectorMotionContinue:
                sectorContinueMotion(nSector, event);
                return true;
        }
    
    // pause motion of the sector
    } else if (event.cmd == kCmdSectorMotionPause) {
        
        sectorPauseMotion(nSector);
        return true;

    }

    return false;

}
