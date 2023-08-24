/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of triggers.cpp from Nblood adapted for level editor's Preview Mode
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
#include "db.h"
#include "eventq.h"
#include "sectorfx.h"
#include "gameutil.h"
#include "screen.h"
#include "xmpsnd.h"
#include "xmpstub.h"
#include "seq.h"
#include "preview.h"

#include "editor.h"
#include "xmpmisc.h"
#include "gui.h"

int basePath[kMaxSectors];
int gBusyCount = 0;
BUSY gBusy[];

void FireballTrapSeqCallback(int, int);
void MGunFireSeqCallback(int, int);
void MGunOpenSeqCallback(int, int);
void TreeToGibCallback(int, int);

int nFireballTrapClient = seqRegisterClient(FireballTrapSeqCallback);
int nMGunFireClient = seqRegisterClient(MGunFireSeqCallback);
int nMGunOpenClient = seqRegisterClient(MGunOpenSeqCallback);
int nTreeToGibClient = seqRegisterClient(TreeToGibCallback);

unsigned int GetWaveValue(unsigned int nPhase, int nType)
{
	switch (nType)
	{
	case 0:
		return 0x8000-(Cos((nPhase<<10)>>16)>>15);
	case 1:
		return nPhase;
	case 2:
		return 0x10000-(Cos((nPhase<<9)>>16)>>14);
	case 3:
		return Sin((nPhase<<9)>>16)>>14;
	}
	return nPhase;
}

char SetSpriteState(int nSprite, XSPRITE* pXSprite, int nState)
{
	if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
		return 0;
	pXSprite->busy = nState << 16;
	pXSprite->state = nState;
	evKill(nSprite, 3);

	if (pXSprite->restState != nState && pXSprite->waitTime > 0)
		evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);

	if (pXSprite->txID)
	{
		if (pXSprite->command != kCmdLink && pXSprite->triggerOn && pXSprite->state)
			evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
		if (pXSprite->command != kCmdLink && pXSprite->triggerOff && !pXSprite->state)
			evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
	}
	return 1;
}



char SetWallState(int nWall, XWALL *pXWall, int nState)
{
	if ((pXWall->busy&0xffff) == 0 && pXWall->state == nState)
		return 0;
	pXWall->busy = nState<<16;
	pXWall->state = nState;
	evKill(nWall, 0);
	if (pXWall->restState != nState && pXWall->waitTime > 0)
		evPost(nWall, 0, (pXWall->waitTime*120) / 10, pXWall->restState ? kCmdOn : kCmdOff);
	if (pXWall->txID)
	{
		if (pXWall->command != kCmdLink && pXWall->triggerOn && pXWall->state)
			evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
		if (pXWall->command != kCmdLink && pXWall->triggerOff && !pXWall->state)
			evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);
	}
	return 1;
}

char SetSectorState(int nSector, XSECTOR *pXSector, int nState)
{
	if ((pXSector->busy&0xffff) == 0 && pXSector->state == nState)
		return 0;
	pXSector->busy = nState<<16;
	pXSector->state = nState;
	evKill(nSector, 6);
	if (nState == 1)
	{
		if (pXSector->command != kCmdLink && pXSector->triggerOn && pXSector->txID)
			evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
		if (pXSector->stopOn)
		{
			pXSector->stopOn = 0;
			pXSector->stopOff = 0;
		}
		else if (pXSector->reTriggerA)
			evPost(nSector, 6, (pXSector->waitTimeA * 120) / 10, kCmdOff);
	}
	else
	{
		if (pXSector->command != kCmdLink && pXSector->triggerOff && pXSector->txID)
			evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);
		if (pXSector->stopOff)
		{
			pXSector->stopOn = 0;
			pXSector->stopOff = 0;
		}
		else if (pXSector->reTriggerB)
			evPost(nSector, 6, (pXSector->waitTimeB * 120) / 10, kCmdOn);
	}
	return 1;
}

void AddBusy(int a1, BUSYID a2, int nDelta)
{
	int i;
	for (i = 0; i < gBusyCount; i++)
	{
		if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
			break;
	}
	if (i == gBusyCount)
	{
        if (gBusyCount >= kMaxBusyCount)
        {
            previewMessage("Failed to AddBusy for #%d! Max busy reached (%d)", a1, gBusyCount);
            return;
        }
		gBusy[i].at0 = a1;
		gBusy[i].atc = a2;
		gBusy[i].at8 = nDelta > 0 ? 0 : 65536;
		gBusyCount++;
	}
	gBusy[i].at4 = nDelta;
}

void ReverseBusy(int a1, BUSYID a2)
{
	int i;
	for (i = 0; i < gBusyCount; i++)
	{
		if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
		{
			gBusy[i].at4 = -gBusy[i].at4;
			break;
		}
	}
}

unsigned int GetSourceBusy(EVENT a1)
{
	int nIndex = a1.index;
	switch (a1.type)
	{
	case 6:
	{
		int nXIndex = sector[nIndex].extra;
		return xsector[nXIndex].busy;
	}
	case 0:
	{
		int nXIndex = wall[nIndex].extra;
		return xwall[nXIndex].busy;
	}
	case 3:
	{
		int nXIndex = sprite[nIndex].extra;
		return xsprite[nXIndex].busy;
	}
	}
	return 0;
}

void OperateSprite(int nSprite, XSPRITE *pXSprite, EVENT event)
{
	spritetype *pSprite = &sprite[nSprite];
	if (gModernMap && modernTypeOperateSprite(nSprite, pSprite, pXSprite, event))
	   return;

	switch (event.cmd) {
		case kCmdLock:
			pXSprite->locked = 1;
			return;
		case kCmdUnlock:
			pXSprite->locked = 0;
			return;
		case kCmdToggleLock:
			pXSprite->locked = pXSprite->locked ^ 1;
			return;
	}


	switch (pSprite->type) {
		case kDudeGargoyleStatueFlesh:
		case kDudeGargoyleStatueStone:
			previewKillDude(pSprite, pXSprite);
			break;
		case kTrapMachinegun:
			if (pXSprite->health <= 0) break;
			switch (event.cmd) {
				case kCmdOff:
					if (!SetSpriteState(nSprite, pXSprite, 0)) break;
					seqSpawn(40, 3, pSprite->extra, -1);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					seqSpawn(38, 3, pSprite->extra, nMGunOpenClient);
					if (pXSprite->data1 > 0)
						pXSprite->data2 = pXSprite->data1;
					break;
			}
			break;
		case kTrapFlame:
			switch (event.cmd) {
				case kCmdOff:
					if (!SetSpriteState(nSprite, pXSprite, 0)) break;
					seqSpawn(40, 3, pSprite->extra, -1);
					sfxKill3DSound(pSprite, 0, -1);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					seqSpawn(38, 3, pSprite->extra, nMGunOpenClient);
					sfxPlay3DSound(pSprite, 441, 0, 0);
					break;
			}
			break;
		case kMarkerDudeSpawn:
		{
			if (pXSprite->data1 < kDudeBase || pXSprite->data1 >= kDudeMax) break;
			if (pXSprite->target > 0)
			{
				if (spriRangeIsFine(pXSprite->target) && sprite[pXSprite->target].statnum == kStatDude)
					actPostSprite((short)pXSprite->target, kStatFree);

				pXSprite->target = 0;

			}

			pSprite->xrepeat = 0;
			spritetype* pDude = actSpawnDude(pSprite, (short)pXSprite->data1, -1, 0);
			if (pDude)
				pXSprite->target = pDude->index;
			break;
		}
		case kThingSpiderWeb:
		case kThingMetalGrate:
		case kThingFlammableTree:
		case kThingFluorescent:
			if (event.type == EVOBJ_SPRITE && event.index == pSprite->index)
			{
				// these cannot receive commands as they supposed to be damaged
				// which means they trigger themselves
				
				switch(pSprite->type)
				{
					case kThingFluorescent:
						seqSpawn(12, 3, pSprite->extra, -1);
						GibSprite(pSprite, (GIBTYPE)6, NULL, NULL);
						break;
					case kThingSpiderWeb:
						seqSpawn(15, 3, pSprite->extra, -1);
						break;
					case kThingMetalGrate:
						seqSpawn(21, 3, pSprite->extra, -1);
						GibSprite(pSprite, GIBTYPE_4, NULL, NULL);
						break;
					case kThingFlammableTree:
						switch (pXSprite->data1)
						{
							case -1:
								GibSprite(pSprite, GIBTYPE_14, NULL, NULL);
								sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 312);
								actPostSprite(pSprite->index, kStatFree);
								break;
							case 0:
								seqSpawn(25, 3, pSprite->extra, nTreeToGibClient);
								sfxPlay3DSound(pSprite, 351, -1, 0);
								break;
							case 1:
								seqSpawn(26, 3, pSprite->extra, nTreeToGibClient);
								sfxPlay3DSound(pSprite, 351, -1, 0);
								break;
						}
						break;
				}
			}
			break;
		case kThingObjectGib:
		case kThingObjectExplode:
		case kThingBloodBits:
		case kThingBloodChunks:
		case kThingZombieHead:
			switch (event.cmd) {
				case kCmdOff:
					if (!SetSpriteState(nSprite, pXSprite, 0)) break;
					actActivateGibObject(pSprite, pXSprite);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					actActivateGibObject(pSprite, pXSprite);
					break;
				default:
					if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)) break;
					actActivateGibObject(pSprite, pXSprite);
					break;
			}
			break;
		case kThingWallCrack:
		case kThingCrateFace:
			if (!SetSpriteState(nSprite, pXSprite, 0)) break;
			actPostSprite(nSprite, kStatFree);
			break;
		case kTrapZapSwitchable:
			switch (event.cmd) {
				case kCmdOff:
					pXSprite->state = 0;
					pSprite->cstat |= kSprInvisible;
					pSprite->cstat &= ~kSprBlock;
					break;
				case kCmdOn:
					pXSprite->state = 1;
					pSprite->cstat &= ~kSprInvisible;
					pSprite->cstat |= kSprBlock;
					break;
				case kCmdToggle:
					pXSprite->state ^= 1;
					pSprite->cstat ^= kSprInvisible;
					pSprite->cstat ^= kSprBlock;
					break;
			}
			break;
		case kSwitchPadlock:
			switch (event.cmd) {
				case kCmdOff:
					SetSpriteState(nSprite, pXSprite, 0);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					seqSpawn(37, 3, pSprite->extra, -1);
					break;
				default:
					SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
					if (pXSprite->state) seqSpawn(37, 3, pSprite->extra, -1);
					break;
			}
			break;
		case kSwitchToggle:
			switch (event.cmd) {
				case kCmdOff:
					if (!SetSpriteState(nSprite, pXSprite, 0)) break;
					sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
					break;
				default:
					if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)) break;
					if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
					else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
					break;
			}
			break;
		case kSwitchOneWay:
			switch (event.cmd) {
				case kCmdOff:
					if (!SetSpriteState(nSprite, pXSprite, 0)) break;
					sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
					break;
				case kCmdOn:
					if (!SetSpriteState(nSprite, pXSprite, 1)) break;
					sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
					break;
				default:
					if (!SetSpriteState(nSprite, pXSprite, pXSprite->restState ^ 1)) break;
					if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
					else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
					break;
			}
			break;
		case kSwitchCombo:
			switch (event.cmd) {
				case kCmdOff:
					pXSprite->data1--;
					if (pXSprite->data1 < 0)
						pXSprite->data1 += pXSprite->data3;
					break;
				default:
					pXSprite->data1++;
					if (pXSprite->data1 >= pXSprite->data3)
						pXSprite->data1 -= pXSprite->data3;
					break;
			}

			sfxPlay3DSound(pSprite, pXSprite->data4, -1, 0);

			if (pXSprite->command == kCmdLink && pXSprite->txID > 0)
				evSend(nSprite, 3, pXSprite->txID, kCmdLink);

			if (pXSprite->data1 == pXSprite->data2)
				SetSpriteState(nSprite, pXSprite, 1);
			else
				SetSpriteState(nSprite, pXSprite, 0);

			break;
	case kMarkerEarthquake:
	{
		//pXSprite->triggerOn = 0;
		//pXSprite->isTriggered = 1;
		//SetSpriteState(nSprite, pXSprite, 1);
		int dx = (pSprite->x - posx)>>4;
		int dy = (pSprite->y - posy)>>4;
		int dz = (pSprite->z - posz)>>8;
		int nDist = dx*dx+dy*dy+dz*dz+0x40000;
		gPreview.scrEffects[kScrEffectQuake1] = divscale16(pXSprite->data1, nDist);
		
	}
	break;
	case kThingTNTBarrel:
	case kThingArmedTNTStick:
	case kThingArmedTNTBundle:
	case kThingArmedSpray:
		actExplodeSprite(pSprite);
		break;
	case kTrapExploder:
		switch (event.cmd) {
			case kCmdOn:
				SetSpriteState(nSprite, pXSprite, 1);
				break;
			default:
				pSprite->cstat &= ~kSprInvisible;
				actExplodeSprite(pSprite);
				break;
		}
		break;
	case kThingArmedRemoteBomb:
		if (event.cmd != kCmdOn) actExplodeSprite(pSprite);
		else
		{
			sfxPlay3DSound(pSprite, 454, 0, 0);
			evPost(nSprite, 3, 18, kCmdOff);
		}
		break;
	case kThingArmedProxBomb:
		switch (event.cmd) {
			case kCmdSpriteProximity:
				if (pXSprite->state) break;
				sfxPlay3DSound(pSprite, 452, 0, 0);
				evPost(nSprite, 3, 30, kCmdOff);
				pXSprite->state = 1;
			case kCmdOn:
				sfxPlay3DSound(pSprite, 451, 0, 0);
				pXSprite->triggerProximity = 1;
				break;
			default:
				actExplodeSprite(pSprite);
				break;
		}
		break;
	case kGenTrigger:
	case kGenDripWater:
	case kGenDripBlood:
	case kGenMissileFireball:
	case kGenMissileEctoSkull:
	case kGenDart:
	case kGenBubble:
	case kGenBubbleMulti:
	case kGenSound:
		switch (event.cmd) {
			case kCmdOff:
				SetSpriteState(nSprite, pXSprite, 0);
				break;
			case kCmdRepeat:
				if (pSprite->type != kGenTrigger) ActivateGenerator(nSprite);
				if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
				if (pXSprite->busyTime > 0)
				{
					int nRand = BiRandom(pXSprite->data1);
					evPost(nSprite, 3, 120*(nRand+pXSprite->busyTime) / 10, kCmdRepeat);
				}
				break;
			default:
				if (!pXSprite->state)
				{
					SetSpriteState(nSprite, pXSprite, 1);
					evPost(nSprite, 3, 0, kCmdRepeat);
				}
				break;
		}
		break;
	case kSoundPlayer:
		sndStartSample(pXSprite->data1, -1, 1, 0);
		break;
	default:
		switch (event.cmd) {
			case kCmdOff:
				SetSpriteState(nSprite, pXSprite, 0);
				break;
			case kCmdOn:
				SetSpriteState(nSprite, pXSprite, 1);
				break;
			default:
				SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
				break;
		}
		break;
	}
}

void SetupGibWallState(walltype *pWall, XWALL *pXWall)
{
	walltype *pWall2 = NULL;
	if (pWall->nextwall >= 0)
		pWall2 = &wall[pWall->nextwall];
	if (pXWall->state)
	{
		pWall->cstat &= ~65;
		if (pWall2)
		{
			pWall2->cstat &= ~65;
			pWall->cstat &= ~16;
			pWall2->cstat &= ~16;
		}
		return;
	}
	char bVector = pXWall->triggerVector != 0;
	pWall->cstat |= 1;
	if (bVector)
		pWall->cstat |= 64;
	if (pWall2)
	{
		pWall2->cstat |= 1;
		if (bVector)
			pWall2->cstat |= 64;
		pWall->cstat |= 16;
		pWall2->cstat |= 16;
	}
}

void OperateWall(int nWall, XWALL *pXWall, EVENT event) {
	walltype *pWall = &wall[nWall];

	switch (event.cmd) {
		case kCmdLock:
			pXWall->locked = 1;
			return;
		case kCmdUnlock:
			pXWall->locked = 0;
			return;
		case kCmdToggleLock:
			pXWall->locked ^= 1;
			return;
	}

	if (gModernMap && modernTypeOperateWall(nWall, pWall, pXWall, event))
		return;

	switch (pWall->type) {
		case kWallGib:
		{
			char bStatus;
			switch (event.cmd) {
				case kCmdOn:
				case kCmdWallImpact:
					bStatus = SetWallState(nWall, pXWall, 1);
					break;
				case kCmdOff:
					bStatus = SetWallState(nWall, pXWall, 0);
					break;
				default:
					bStatus = SetWallState(nWall, pXWall, pXWall->state ^ 1);
					break;
			}

			if (bStatus) {
				SetupGibWallState(pWall, pXWall);
				if (pXWall->state) {
					CGibVelocity vel(100, 100, 250);
					int nType = ClipRange(pXWall->data, 0, 31);
					if (nType > 0)
						GibWall(nWall, (GIBTYPE)nType, &vel);
				}
			}
			return;
		}
		default:
			switch (event.cmd) {
				case kCmdOff:
					SetWallState(nWall, pXWall, 0);
					break;
				case kCmdOn:
					SetWallState(nWall, pXWall, 1);
					break;
				default:
					SetWallState(nWall, pXWall, pXWall->state ^ 1);
					break;
			}
			return;
	}


}

void SectorStartSound(int nSector, int nState)
{
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
		{
			int nXSprite = pSprite->extra;
			XSPRITE *pXSprite = &xsprite[nXSprite];
			if (nState)
			{
				if (pXSprite->data3)
					sfxPlay3DSound(pSprite, pXSprite->data3, 0, 0);
			}
			else
			{
				if (pXSprite->data1)
					sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
			}
		}
	}
}

void SectorEndSound(int nSector, int nState)
{
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
		{
			int nXSprite = pSprite->extra;
			XSPRITE *pXSprite = &xsprite[nXSprite];
			if (nState)
			{
				if (pXSprite->data2)
					sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
			}
			else
			{
				if (pXSprite->data4)
					sfxPlay3DSound(pSprite, pXSprite->data4, 0, 0);
			}
		}
	}
}

void PathSound(int nSector, int nSound)
{
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
			sfxPlay3DSound(pSprite, nSound, 0, 0);
	}
}

void DragPoint(int nWall, int x, int y)
{
	//viewInterpolateWall(nWall, &wall[nWall]);
	wall[nWall].x = x;
	wall[nWall].y = y;

	int vsi = numwalls;
	int vb = nWall;
	do
	{
		if (wall[vb].nextwall >= 0)
		{
			vb = wall[wall[vb].nextwall].point2;
			//viewInterpolateWall(vb, &wall[vb]);
			wall[vb].x = x;
			wall[vb].y = y;
		}
		else
		{
			vb = nWall;
			do
			{
				if (wall[lastwall(vb)].nextwall >= 0)
				{
					vb = wall[lastwall(vb)].nextwall;
					//viewInterpolateWall(vb, &wall[vb]);
					wall[vb].x = x;
					wall[vb].y = y;
				}
				else
					break;
				vsi--;
			} while (vb != nWall && vsi > 0);
			break;
		}
		vsi--;
	} while (vb != nWall && vsi > 0);
}

BOOL isMovableSector(int nType)
{
	return (nType && nType != kSectorDamage && nType != kSectorTeleport && nType != kSectorCounter);
}

BOOL isMovableSector(sectortype* pSect)
{
	if (isMovableSector(pSect->type) && pSect->extra > 0)
	{
		XSECTOR* pXSect = &xsector[pSect->extra];
		return (pXSect->busy && !pXSect->unused1);
	}
	
	return FALSE;
}

int getSpritesNearWalls(int nSrcSect, int* spriOut, int nMax, int nDist)
{
	int i, j, c = 0, nWall, nSect, swal, ewal;
	int xi, yi, wx, wy, lx, ly, sx, sy, qx, qy, num, den;
	int skip[kMaxSprites]; spritetype* pSpr;
	
	memset(skip, 0, sizeof(skip));
	getSectorWalls(nSrcSect, &swal, &ewal);
	
	for (i = swal; i <= ewal; i++)
	{
		nSect = wall[i].nextsector;
		if (nSect < 0)
			continue;
		
		nWall = i;
		wx = wall[nWall].x;	wy = wall[nWall].y;
		lx = wall[wall[nWall].point2].x - wx;
		ly = wall[wall[nWall].point2].y - wy;
				
		for (j = headspritesect[nSect]; j >= 0; j = nextspritesect[j])
		{
			if (skip[j])
				continue;
			
			sx = sprite[j].x;	qx = sx - wx;
			sy = sprite[j].y;	qy = sy - wy;
			num = dmulscale4(qx, lx, qy, ly);
			den = dmulscale4(lx, lx, ly, ly);
			
			if (num > 0 && num < den)
			{
				xi = wx + scale(lx, num, den);
				yi = wy + scale(ly, num, den);
				if (approxDist(xi - sx, yi - sy) <= nDist)
				{
					skip[j] = 1; spriOut[c] = j;
					if (++c == nMax)
						return c;
				}
			}
		}
	}
	
	return c;
}

// SPRITES_NEAR_SECTORS
class SPRINSECT
{
	#define kMaxSprNear 256
	#define kWallDist	16
	
	private:
		struct SPRITES
		{
			unsigned int nSector;
			signed   int sprites[kMaxSprNear + 1];
		};
		SPRITES* db;
		unsigned int length;
	public:
		void Free();
		void Init(int nDist = kWallDist);
		void Save();
		void Load();
		int* GetSprPtr(int nSector);
		~SPRINSECT() { Free(); };
	
} gSprNSect;

void SPRINSECT::Free()
{
	length = 0;
	if (db)
		Bfree(db), db = NULL;
}

int* SPRINSECT::GetSprPtr(int nSector)
{
	int i;
	for (i = 0; i < length; i++)
	{
		if (db[i].nSector == nSector)
			return (int*)db[i].sprites;
	}
	return NULL;
}

void SPRINSECT::Init(int nDist)
{
	Free();

	int i, j, k, nSprites;
	int collected[kMaxSprites];
	for (i = 0; i < numsectors; i++)
	{
		sectortype* pSect = &sector[i];
		if (!isMovableSector(pSect->type))
			continue;
		
		switch (pSect->type) {
			case kSectorZMotionSprite:
			case kSectorSlideMarked:
			case kSectorRotateMarked:
				continue;
			// only allow non-marked sectors
			default:
				break;
		}

		nSprites = getSpritesNearWalls(i, collected, LENGTH(collected), nDist);
		
		// exclude sprites that is not allowed
		for (j = nSprites - 1; j >= 0; j--)
		{
			spritetype* pSpr = &sprite[collected[j]];
			if ((pSpr->cstat & kSprMoveMask) && pSpr->sectnum >= 0)
			{
				// if *next* sector is movable, exclude to avoid fighting
				if (!isMovableSector(sector[pSpr->sectnum].type))
				{
					switch (pSpr->statnum) {
						default:
							continue;
						case kStatMarker:
						case kStatPathMarker:
							if (pSpr->flags & 0x1) continue;
							// no break
						case kStatDude:
							break;
					}
				}
			}
			
			nSprites--;
			for (k = j; k < nSprites; k++)
				collected[k] = collected[k + 1];
		}
		
		if (nSprites > 0)
		{
			db = (SPRITES*)Brealloc(db, (length + 1) * sizeof(SPRITES));
			dassert(db != NULL);
			
			SPRITES* pEntry =& db[length];
			memset(pEntry->sprites, -1, sizeof(pEntry->sprites));
			memcpy(pEntry->sprites, collected, sizeof(pEntry->sprites[0])*ClipHigh(nSprites, kMaxSprNear));
			pEntry->nSector = i;
			length++;
		}
	}
}

void TranslateSector(int nSector, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12)
{
	int x, y;
	int nXSector = sector[nSector].extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int v20 = interpolate(a6, a9, a2);
	int vc = interpolate(a6, a9, a3);
	int v28 = vc - v20;
	int v24 = interpolate(a7, a10, a2);
	int v8 = interpolate(a7, a10, a3);
	int v2c = v8 - v24;
	int v44 = interpolate(a8, a11, a2);
	int vbp = interpolate(a8, a11, a3);
	int v14 = vbp - v44;
	int nWall = sector[nSector].wallptr;
	int i, nSprite;
	int fixY = (gModernMap) ? a5 : a4;
	int* ptr;

	if (a12) // non-marked sectors?
	{		
		for (i = 0; i < sector[nSector].wallnum; nWall++, i++)
		{
			x = baseWall[nWall].x;
			y = baseWall[nWall].y;
			if (vbp)
				RotatePoint(&x, &y, vbp, a4, a5);
			DragPoint(nWall, x+vc-a4, y+v8-a5);
		}
	}
	else
	{
		for (i = 0; i < sector[nSector].wallnum; nWall++, i++)
		{
			int v10 = wall[nWall].point2;
			x = baseWall[nWall].x;
			y = baseWall[nWall].y;
			if (wall[nWall].cstat&16384)
			{
				if (vbp)
					RotatePoint(&x, &y, vbp, a4, a5);
				DragPoint(nWall, x+vc-a4, y+v8-a5);
				if ((wall[v10].cstat&49152) == 0)
				{
					x = baseWall[v10].x;
					y = baseWall[v10].y;
					if (vbp)
						RotatePoint(&x, &y, vbp, a4, a5);
					DragPoint(v10, x+vc-a4, y+v8-a5);
				}
				continue;
			}
			if (wall[nWall].cstat&32768)
			{
				if (vbp)
					RotatePoint(&x, &y, -vbp, a4, a5);
				DragPoint(nWall, x-(vc-a4), y-(v8-a5));
				if ((wall[v10].cstat&49152) == 0)
				{
					x = baseWall[v10].x;
					y = baseWall[v10].y;
					if (vbp)
						RotatePoint(&x, &y, -vbp, a4, a5);
					DragPoint(v10, x-(vc-a4), y-(v8-a5));
				}
				continue;
			}
		}
	}
	for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		switch (pSprite->statnum) {
			case kStatMarker:
			case kStatPathMarker:
				if (!gModernMap || !(pSprite->flags & 0x1)) continue;
				break;
		}

		x = baseSprite[nSprite].x;
		y = baseSprite[nSprite].y;
		if (sprite[nSprite].cstat&8192)
		{
			if (vbp)
				RotatePoint(&x, &y, vbp, a4, a5);
			//viewBackupSpriteLoc(nSprite, pSprite);
			pSprite->ang = (pSprite->ang+v14)&2047;
			pSprite->x = x+vc-a4;
			pSprite->y = y+v8-a5;
		}
		else if (sprite[nSprite].cstat&16384)
		{
			if (vbp)
				RotatePoint(&x, &y, -vbp, a4, fixY);
			//viewBackupSpriteLoc(nSprite, pSprite);
			pSprite->ang = (pSprite->ang-v14)&2047;
			pSprite->x = x-(vc-a4);
			pSprite->y = y-(v8-a5);
		}
		else if (pXSector->drag)
		{
			int top, bottom;
			GetSpriteExtents(pSprite, &top, &bottom);
			int floorZ = getflorzofslope(nSector, pSprite->x, pSprite->y);
			if (!(pSprite->cstat&48) && floorZ <= bottom)
			{
				if (v14)
					RotatePoint(&pSprite->x, &pSprite->y, v14, v20, v24);
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->ang = (pSprite->ang+v14)&2047;
				pSprite->x += v28;
				pSprite->y += v2c;
			}
		}
	}
	
	// translate sprites near outside walls
	////////////////////////////////////////////////////////////
	if (gModernMap && (ptr = gSprNSect.GetSprPtr(nSector)) != NULL)
	{
		while(*ptr >= 0)
		{
			spritetype *pSprite = &sprite[*ptr++];
			if (pSprite->statnum >= kMaxStatus)
				continue;
			
			nSprite = pSprite->index;
			x = baseSprite[nSprite].x;
			y = baseSprite[nSprite].y;
			if (pSprite->cstat & 8192)
			{
				if (vbp)
					RotatePoint(&x, &y, vbp, a4, a5);
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->ang = (pSprite->ang+v14)&2047;
				pSprite->x = x+vc-a4;
				pSprite->y = y+v8-a5;
			}
			else if (pSprite->cstat & 16384)
			{
				if (vbp)
					RotatePoint(&x, &y, -vbp, a4, fixY);
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->ang = (pSprite->ang-v14)&2047;
				pSprite->x = x-(vc-a4);
				pSprite->y = y-(v8-a5);
			}
		}
	}
	/////////////////////
	
	// drag camera
	if (pXSector->drag)
	{
		updatesector(posx, posy, &cursectnum);
		if (cursectnum == nSector && (zmode == 0 || qsetmode != 200))
		{
			if (v14)
				RotatePoint(&posx, &posy, v14, v20, v24);
			
			ang = (short)((ang + v14) & kAngMask);
			posx += v28;
			posy += v2c;
		}
	}
}

void ZTranslateSector(int nSector, XSECTOR *pXSector, int a3, int a4)
{
	sectortype *pSector = &sector[nSector];
	//viewInterpolateSector(nSector, pSector);
	
	int i, nSprite;
	int dfz, dcz, *ptr1 = NULL, *ptr2;
	
	dfz = pXSector->onFloorZ-pXSector->offFloorZ;
	dcz = pXSector->onCeilZ-pXSector->offCeilZ;
	
	// get pointer to sprites near outside walls before translation
	///////////////////////////////////////////////////////////////
	if (gModernMap && (dfz || dcz))
		ptr1 = gSprNSect.GetSprPtr(nSector);
	
	if (dfz != 0)
	{
		int oldZ = pSector->floorz;
		baseFloor[nSector] = pSector->floorz = pXSector->offFloorZ + mulscale16(dfz, GetWaveValue(a3, a4));
		velFloor[nSector] += (pSector->floorz-oldZ)<<8;
		for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			spritetype *pSprite = &sprite[nSprite];
			if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
				continue;
			int top, bottom;
			GetSpriteExtents(pSprite, &top, &bottom);
			if (pSprite->cstat&8192)
			{
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->z += pSector->floorz-oldZ;
			}
			else if (pSprite->flags&2)
				pSprite->flags |= 4;
			else if (oldZ <= bottom && !(pSprite->cstat&48))
			{
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->z += pSector->floorz-oldZ;
			}
		}
		
		// translate sprites near outside walls (floor)
		////////////////////////////////////////////////////////////
		if (ptr1)
		{
			ptr2 = ptr1;
			while(*ptr2 >= 0)
			{
				spritetype *pSprite = &sprite[*ptr2++];
				if (pSprite->cstat & 8192)
					pSprite->z += pSector->floorz-oldZ;
			}
		}
		/////////////////////
	}

	if (dcz != 0)
	{
		int oldZ = pSector->ceilingz;
		baseCeil[nSector] = pSector->ceilingz = pXSector->offCeilZ + mulscale16(dcz, GetWaveValue(a3, a4));
		velCeil[nSector] += (pSector->ceilingz-oldZ)<<8;
		for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			spritetype *pSprite = &sprite[nSprite];
			if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
				continue;
			if (pSprite->cstat&16384)
			{
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->z += pSector->ceilingz-oldZ;
			}
		}
		
		// translate sprites near outside walls (ceil)
		////////////////////////////////////////////////////////////
		if (ptr1)
		{
			ptr2 = ptr1;
			while(*ptr2 >= 0)
			{
				spritetype *pSprite = &sprite[*ptr2++];
				if (pSprite->cstat & 16384)
					pSprite->z += pSector->ceilingz-oldZ;
			}
		}
		/////////////////////
		
	}
}

int GetHighestSprite(int nSector, int nStatus, int *a3)
{
	*a3 = sector[nSector].floorz;
	int v8 = -1;
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		if (sprite[nSprite].statnum == nStatus || nStatus == kStatFree)
		{
			spritetype *pSprite = &sprite[nSprite];
			int top, bottom;
			GetSpriteExtents(pSprite, &top, &bottom);
			if (top-pSprite->z > *a3)
			{
				*a3 = top-pSprite->z;
				v8 = nSprite;
			}
		}
	}
	return v8;
}

int GetCrushedSpriteExtents(unsigned int nSector, int *pzTop, int *pzBot)
{
	int vc = -1;
	sectortype *pSector = &sector[nSector];
	int vbp = pSector->ceilingz;
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
		{
			int top, bottom;
			GetSpriteExtents(pSprite, &top, &bottom);
			if (vbp > top)
			{
				vbp = top;
				*pzTop = top;
				*pzBot = bottom;
				vc = nSprite;
			}
		}
	}
	return vc;
}

int VCrushBusy(unsigned int nSector, unsigned int a2)
{
	int nXSector = sector[nSector].extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	int dz1 = pXSector->onCeilZ - pXSector->offCeilZ;
	int vc = pXSector->offCeilZ;
	if (dz1 != 0)
		vc += mulscale16(dz1, GetWaveValue(a2, nWave));
	int dz2 = pXSector->onFloorZ - pXSector->offFloorZ;
	int v10 = pXSector->offFloorZ;
	if (dz2 != 0)
		v10 += mulscale16(dz2, GetWaveValue(a2, nWave));
	int v18;
	if (GetHighestSprite(nSector, 6, &v18) >= 0 && vc >= v18)
		return 1;
	//viewInterpolateSector(nSector, &sector[nSector]);
	if (dz1 != 0)
		sector[nSector].ceilingz = vc;
	if (dz2 != 0)
		sector[nSector].floorz = v10;
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int VSpriteBusy(unsigned int nSector, unsigned int a2)
{
	int nXSector = sector[nSector].extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	int dz1 = pXSector->onFloorZ - pXSector->offFloorZ;
	if (dz1 != 0)
	{
		for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			spritetype *pSprite = &sprite[nSprite];
			if (pSprite->cstat&8192)
			{
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->z = baseSprite[nSprite].z+mulscale16(dz1, GetWaveValue(a2, nWave));
			}
		}
	}
	int dz2 = pXSector->onCeilZ - pXSector->offCeilZ;
	if (dz2 != 0)
	{
		for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			spritetype *pSprite = &sprite[nSprite];
			if (pSprite->cstat&16384)
			{
				//viewBackupSpriteLoc(nSprite, pSprite);
				pSprite->z = baseSprite[nSprite].z+mulscale16(dz2, GetWaveValue(a2, nWave));
			}
		}
	}
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int VDoorBusy(unsigned int nSector, unsigned int a2)
{
	int nXSector = sector[nSector].extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int vbp;
	if (pXSector->state)
		vbp = 65536/ClipLow((120*pXSector->busyTimeA)/10, 1);
	else
		vbp = -65536/ClipLow((120*pXSector->busyTimeB)/10, 1);
	int top, bottom;
	int nSprite = GetCrushedSpriteExtents(nSector,&top,&bottom);
	if (nSprite >= 0 && a2 > pXSector->busy)
	{
		spritetype *pSprite = &sprite[nSprite];
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		if (pXSector->onCeilZ > pXSector->offCeilZ || pXSector->onFloorZ < pXSector->offFloorZ)
		{
			if ((pXSector->interruptable) || (pXSector->crush && pXSprite->health > 0))
			{
				if (!pXSector->crush);
				else if (IsDudeSprite(pSprite)) previewKillDude(pSprite, pXSprite);
				else if (IsThingSprite(pSprite))
					previewDestroyThing(pSprite, pXSprite, kCmdSpriteImpact);
				
				a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
			}
		}
	}
	else if (nSprite >= 0 && a2 < pXSector->busy)
	{
		spritetype *pSprite = &sprite[nSprite];
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		if (pXSector->offCeilZ > pXSector->onCeilZ || pXSector->offFloorZ < pXSector->onFloorZ)
		{
			if ((pXSector->interruptable) || (pXSector->crush && pXSprite->health > 0))
			{
				if (!pXSector->crush);
				else if (IsDudeSprite(pSprite)) previewKillDude(pSprite, pXSprite);
				else if (IsThingSprite(pSprite))
					previewDestroyThing(pSprite, pXSprite, kCmdSpriteImpact);
				
				a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
			}
		}
	}
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	ZTranslateSector(nSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int HDoorBusy(unsigned int nSector, unsigned int a2)
{
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	spritetype *pSprite1 = &sprite[pXSector->marker0];
	spritetype *pSprite2 = &sprite[pXSector->marker1];
	TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->type == kSectorSlide);
	ZTranslateSector(nSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int RDoorBusy(unsigned int nSector, unsigned int a2)
{
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	XSECTOR *pXSector = &xsector[nXSector];
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	spritetype *pSprite = &sprite[pXSector->marker0];
	TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, 0, pSprite->x, pSprite->y, pSprite->ang, pSector->type == kSectorRotate);
	ZTranslateSector(nSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int StepRotateBusy(unsigned int nSector, unsigned int a2)
{
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	XSECTOR *pXSector = &xsector[nXSector];
	spritetype *pSprite = &sprite[pXSector->marker0];
	int vbp;
	if (pXSector->busy < a2)
	{
		vbp = pXSector->data+pSprite->ang;
		int nWave = pXSector->busyWaveA;
		TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, pXSector->data, pSprite->x, pSprite->y, vbp, 1);
	}
	else
	{
		vbp = pXSector->data-pSprite->ang;
		int nWave = pXSector->busyWaveB;
		TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, vbp, pSprite->x, pSprite->y, pXSector->data, 1);
	}
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		pXSector->data = vbp&2047;
		return 3;
	}
	return 0;
}

int GenSectorBusy(unsigned int nSector, unsigned int a2)
{
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	XSECTOR *pXSector = &xsector[nXSector];
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSend(nSector, 6, pXSector->txID, kCmdLink);
	if ((a2&0xffff) == 0)
	{
		SetSectorState(nSector, pXSector, a2>>16);
		SectorEndSound(nSector, a2>>16);
		return 3;
	}
	return 0;
}

int PathBusy(unsigned int nSector, unsigned int a2)
{
	sectortype *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	XSECTOR *pXSector = &xsector[nXSector];
	spritetype *pSprite = &sprite[basePath[nSector]];
	spritetype *pSprite1 = &sprite[pXSector->marker0];
	XSPRITE *pXSprite1 = &xsprite[pSprite1->extra];
	spritetype *pSprite2 = &sprite[pXSector->marker1];
	XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
	int nWave = pXSprite1->wave;
	TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, 1);
	ZTranslateSector(nSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if ((a2&0xffff) == 0)
	{
		evPost(nSector, 6, (120*pXSprite2->waitTime)/10, kCmdOn);
		pXSector->state = 0;
		pXSector->busy = 0;
		if (pXSprite1->data4)
			PathSound(nSector, pXSprite1->data4);
		pXSector->marker0 = pXSector->marker1;
		pXSector->data = pXSprite2->data1;
		return 3;
	}
	return 0;
}

void OperateDoor(unsigned int nSector, XSECTOR *pXSector, EVENT event, BUSYID busyWave)
{
	switch (event.cmd) {
		case kCmdOff:
			if (!pXSector->busy) break;
			AddBusy(nSector, busyWave, -65536/ClipLow((pXSector->busyTimeB*120)/10, 1));
			SectorStartSound(nSector, 1);
			break;
		case kCmdOn:
			if (pXSector->busy == 0x10000) break;
			AddBusy(nSector, busyWave, 65536/ClipLow((pXSector->busyTimeA*120)/10, 1));
			SectorStartSound(nSector, 0);
			break;
		default:
			if (pXSector->busy & 0xffff)  {
				if (pXSector->interruptable) {
					ReverseBusy(nSector, busyWave);
					pXSector->state = !pXSector->state;
				}
			} else {
				char t = !pXSector->state; int nDelta;

				if (t) nDelta = 65536/ClipLow((pXSector->busyTimeA*120)/10, 1);
				else nDelta = -65536/ClipLow((pXSector->busyTimeB*120)/10, 1);

				AddBusy(nSector, busyWave, nDelta);
				SectorStartSound(nSector, pXSector->state);
			}
			break;
	}
}

char SectorContainsDudes(int nSector)
{
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		if (sprite[nSprite].statnum == kStatDude)
			return 1;
	}
	return 0;
}

int TeleFrag(int nKiller, int nSector)
{
	int cnt = 0;
	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		XSPRITE* pXSprite = pSprite->extra > 0 ? &xsprite[pSprite->extra] : NULL; 
		
		if (!pXSprite) continue;
		else if (pSprite->statnum == kStatThing) previewDestroyThing(pSprite, pXSprite, kCmdSpriteImpact); 
		else if (pSprite->statnum == kStatDude && pXSprite->health > 0)
		{
			previewKillDude(pSprite, pXSprite);
			cnt++;
		}
	}

	return cnt;
}

void OperateTeleport(unsigned int nSector, XSECTOR *pXSector)
{
	int nDest = pXSector->marker0;
	spritetype *pDest = &sprite[nDest];

	for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		spritetype *pSprite = &sprite[nSprite];
		if (pSprite->statnum == kStatDude)
		{
			if (SectorContainsDudes(pDest->sectnum))
			{
				previewMessage("Dude teleport failed: Destination sector contains dudes!");
				continue;
			}

			pSprite->x = pDest->x;
			pSprite->y = pDest->y;
			pSprite->z += sector[pDest->sectnum].floorz-sector[nSector].floorz;
			pSprite->ang = pDest->ang;
			ChangeSpriteSect(nSprite, pDest->sectnum);
			sfxPlay3DSound(pDest, 201, -1, 0);
			
			xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
			
		}
	}
	
	int marker = xsector[sector[cursectnum].extra].marker0;
	if (cursectnum == nSector && marker >= 0 && sprite[marker].sectnum >= 0 && sprite[marker].sectnum != cursectnum)
	{
		int cnt = TeleFrag(0, sprite[marker].sectnum);
		int i = sprintf(buffer, "Teleport from sector %d to %d", cursectnum, sprite[marker].sectnum);
		if (cnt)
			sprintf(&buffer[i], " (telefragged %d dudes!)", cnt);
		
		previewMessage(buffer);
		posx = sprite[marker].x; posy = sprite[marker].y; posz = sprite[marker].z;
		
		i = nSector;
		FindSector(posx, posy, posz, &i);
		cursectnum = i;
		
		//updatesector(posx, posy, &cursectnum);
		gPreview.sectnum = cursectnum;
		ang = sprite[marker].ang; horiz = 100;
		vel = svel = angvel = 0;
		
		sfxPlay3DSound(pDest, 201, -1, 0);
	}
}

void OperatePath(unsigned int nSector, XSECTOR *pXSector, EVENT event)
{
	int nSprite;
	spritetype *pSprite = NULL;
	XSPRITE *pXSprite;
	spritetype *pSprite2 = &sprite[pXSector->marker0];
	XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
	int nId = pXSprite2->data2;
	for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		pSprite = &sprite[nSprite];
		if (pSprite->type == kMarkerPath)
		{
			pXSprite = &xsprite[pSprite->extra];
			if (pXSprite->data1 == nId)
				break;
		}
	}

	// trigger marker after it gets reached
	if (gModernMap && pXSprite2->state != 1)
		trTriggerSprite(pSprite2->index, pXSprite2, kCmdOn);

	if (nSprite < 0) {
		previewMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
		pXSector->state = 0;
		pXSector->busy = 0;
		return;
	}

	pXSector->marker1 = nSprite;
	pXSector->offFloorZ = pSprite2->z;
	pXSector->onFloorZ = pSprite->z;
	switch (event.cmd) {
		case kCmdOn:
			pXSector->state = 0;
			pXSector->busy = 0;
			AddBusy(nSector, BUSYID_7, 65536/ClipLow((120*pXSprite2->busyTime)/10,1));
			if (pXSprite2->data3) PathSound(nSector, pXSprite2->data3);
			break;
	}
}

void OperateSector(unsigned int nSector, XSECTOR *pXSector, EVENT event)
{
	sectortype *pSector = &sector[nSector];

	if (gModernMap && modernTypeOperateSector(nSector, pSector, pXSector, event))
		return;

	switch (event.cmd) {
		case kCmdLock:
			pXSector->locked = 1;
			break;
		case kCmdUnlock:
			pXSector->locked = 0;
			break;
		case kCmdToggleLock:
			pXSector->locked ^= 1;
			break;
		case kCmdStopOff:
			pXSector->stopOn = 0;
			pXSector->stopOff = 1;
			break;
		case kCmdStopOn:
			pXSector->stopOn = 1;
			pXSector->stopOff = 0;
			break;
		case kCmdStopNext:
			pXSector->stopOn = 1;
			pXSector->stopOff = 1;
			break;
		default:
			if (gModernMap && pXSector->unused1) break;
			switch (pSector->type) {
				case kSectorZMotionSprite:
					OperateDoor(nSector, pXSector, event, BUSYID_1);
					break;
				case kSectorZMotion:
					OperateDoor(nSector, pXSector, event, BUSYID_2);
					break;
				case kSectorSlideMarked:
				case kSectorSlide:
					OperateDoor(nSector, pXSector, event, BUSYID_3);
					break;
				case kSectorRotateMarked:
				case kSectorRotate:
					OperateDoor(nSector, pXSector, event, BUSYID_4);
					break;
				case kSectorRotateStep:
					switch (event.cmd) {
						case kCmdOn:
							pXSector->state = 0;
							pXSector->busy = 0;
							AddBusy(nSector, BUSYID_5, 65536/ClipLow((120*pXSector->busyTimeA)/10, 1));
							SectorStartSound(nSector, 0);
							break;
						case kCmdOff:
							pXSector->state = 1;
							pXSector->busy = 65536;
							AddBusy(nSector, BUSYID_5, -65536/ClipLow((120*pXSector->busyTimeB)/10, 1));
							SectorStartSound(nSector, 1);
							break;
					}
					break;
				case kSectorTeleport:
					OperateTeleport(nSector, pXSector);
					break;
				case kSectorPath:
					OperatePath(nSector, pXSector, event);
					break;
				default:
					if (!pXSector->busyTimeA && !pXSector->busyTimeB) {

						switch (event.cmd) {
							case kCmdOff:
								SetSectorState(nSector, pXSector, 0);
								break;
							case kCmdOn:
								SetSectorState(nSector, pXSector, 1);
								break;
							default:
								SetSectorState(nSector, pXSector, pXSector->state ^ 1);
								break;
						}

					} else {

						OperateDoor(nSector, pXSector, event, BUSYID_6);

					}

					break;
			}
			break;
	}
}

void InitPath(unsigned int nSector, XSECTOR *pXSector)
{
	int nSprite;
	spritetype *pSprite;
	XSPRITE *pXSprite;
	
	int nId = pXSector->data;
	for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		pSprite = &sprite[nSprite];
		if (pSprite->type == kMarkerPath)
		{
			pXSprite = &xsprite[pSprite->extra];
			if (pXSprite->data1 == nId)
				break;
		}
	}

	if (nSprite < 0) {
		//ThrowError("Unable to find path marker with id #%d", nId);
		previewMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
		return;

	}

	pXSector->marker0 = nSprite;
	basePath[nSector] = nSprite;
	if (pXSector->state)
		evPost(nSector, 6, 0, kCmdOn);
}

void LinkSector(int nSector, XSECTOR *pXSector, EVENT event)
{
	sectortype *pSector = &sector[nSector];
	int nBusy = GetSourceBusy(event);
	switch (pSector->type) {
		case kSectorZMotionSprite:
			VSpriteBusy(nSector, nBusy);
			break;
		case kSectorZMotion:
			VDoorBusy(nSector, nBusy);
			break;
		case kSectorSlideMarked:
		case kSectorSlide:
			HDoorBusy(nSector, nBusy);
			break;
		case kSectorRotateMarked:
		case kSectorRotate:
			RDoorBusy(nSector, nBusy);
			break;
		default:
			pXSector->busy = nBusy;
			if ((pXSector->busy&0xffff) == 0)
				SetSectorState(nSector, pXSector, nBusy>>16);
			break;
	}
}

void LinkSprite(int nSprite, XSPRITE *pXSprite, EVENT event) {
	spritetype *pSprite = &sprite[nSprite];
	int nBusy = GetSourceBusy(event);

	switch (pSprite->type)  {
		case kSwitchCombo:
		{
			if (event.type == 3)
			{
				int nSprite2 = event.index;
				int nXSprite2 = sprite[nSprite2].extra;
				pXSprite->data1 = xsprite[nXSprite2].data1;
				if (pXSprite->data1 == pXSprite->data2)
					SetSpriteState(nSprite, pXSprite, 1);
				else
					SetSpriteState(nSprite, pXSprite, 0);
			}
		}
		break;
		default:
		{
			pXSprite->busy = nBusy;
			if ((pXSprite->busy & 0xffff) == 0)
				SetSpriteState(nSprite, pXSprite, nBusy >> 16);
		}
		break;
	}
}

void LinkWall(int nWall, XWALL *pXWall, EVENT event)
{
	int nBusy = GetSourceBusy(event);
	pXWall->busy = nBusy;
	if ((pXWall->busy & 0xffff) == 0)
		SetWallState(nWall, pXWall, nBusy>>16);
}

void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int command)
{
	if (!pXSector->locked && !pXSector->isTriggered)
	{

		if (pXSector->triggerOnce)
			pXSector->isTriggered = 1;

		if (pXSector->decoupled && pXSector->txID > 0)
			evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command);

		else {
			EVENT event;
			event.cmd = command;
			OperateSector(nSector, pXSector, event);
		}

	}
}

void trTriggerWall(unsigned int nWall, XWALL *pXWall, int command)
{
	if (!pXWall->locked && !pXWall->isTriggered)
	{

		if (pXWall->triggerOnce)
			pXWall->isTriggered = 1;

		if (pXWall->decoupled && pXWall->txID > 0)
			evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command);

		else {
			EVENT event;
			event.cmd = command;
			OperateWall(nWall, pXWall, event);
		}

	}
}

void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int command) {
	if (!pXSprite->locked && !pXSprite->isTriggered) {

		if (pXSprite->triggerOnce)
			pXSprite->isTriggered = 1;

		if (pXSprite->decoupled && pXSprite->txID > 0)
		   evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);

		else {
			EVENT event;
			event.cmd = command;
			event.type = EVOBJ_SPRITE;
			event.index = nSprite;
			OperateSprite(nSprite, pXSprite, event);
		}

	}
}


void trMessageSector(unsigned int nSector, EVENT event)
{
	XSECTOR *pXSector = &xsector[sector[nSector].extra];
	if (!pXSector->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
		switch (event.cmd) {
			case kCmdLink:
				LinkSector(nSector, pXSector, event);
				break;
			case kCmdModernUse:
				modernTypeTrigger(6, nSector, event);
				break;
			default:
				OperateSector(nSector, pXSector, event);
				break;
		}
	}
}

void trMessageWall(unsigned int nWall, EVENT event)
{
	XWALL *pXWall = &xwall[wall[nWall].extra];
	if (!pXWall->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
		switch (event.cmd) {
			case kCmdLink:
				LinkWall(nWall, pXWall, event);
				break;
			case kCmdModernUse:
				modernTypeTrigger(0, nWall, event);
				break;
			default:
				OperateWall(nWall, pXWall, event);
				break;
		}
	}
}

void trMessageSprite(unsigned int nSprite, EVENT event) {
    if (sprite[nSprite].statnum == kStatFree)
        return;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->extra <= 0 || pSprite->extra >= kMaxXSprites)
        return;
    XSPRITE* pXSprite = &xsprite[sprite[nSprite].extra];
    if (!pXSprite->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkSprite(nSprite, pXSprite, event);
                break;
            case kCmdModernUse:
                modernTypeTrigger(3, nSprite, event);
                break;
            default:
                OperateSprite(nSprite, pXSprite, event);
                break;
        }
    }
}



void ProcessMotion(void)
{
	sectortype *pSector;
	int nSector;
	for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
	{
		int nXSector = pSector->extra;
		if (nXSector <= 0)
			continue;
		XSECTOR *pXSector = &xsector[nXSector];
		if (pXSector->bobSpeed != 0)
		{
			if (pXSector->bobAlways)
				pXSector->bobTheta += pXSector->bobSpeed;
			else if (pXSector->busy == 0)
				continue;
			else
				pXSector->bobTheta += mulscale16(pXSector->bobSpeed, pXSector->busy);
			int vdi = mulscale30(Sin(pXSector->bobTheta), pXSector->bobZRange<<8);
			for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
			{
				spritetype *pSprite = &sprite[nSprite];
				if (pSprite->cstat&24576)
				{
					//viewBackupSpriteLoc(nSprite, pSprite);
					pSprite->z += vdi;
				}
			}
			if (pXSector->bobFloor)
			{
				int floorZ = pSector->floorz;
				//viewInterpolateSector(nSector, pSector);
				pSector->floorz = baseFloor[nSector]+vdi;
				for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
				{
					spritetype *pSprite = &sprite[nSprite];
					if (pSprite->flags&2)
						pSprite->flags |= 4;
					else
					{
						int top, bottom;
						GetSpriteExtents(pSprite, &top, &bottom);
						if (bottom >= floorZ && (pSprite->cstat&48) == 0)
						{
							//viewBackupSpriteLoc(nSprite, pSprite);
							pSprite->z += vdi;
						}
					}
				}
			}
			if (pXSector->bobCeiling)
			{
				int ceilZ = pSector->ceilingz;
				//viewInterpolateSector(nSector, pSector);
				pSector->ceilingz = baseCeil[nSector]+vdi;
				for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
				{
					spritetype *pSprite = &sprite[nSprite];
					int top, bottom;
					GetSpriteExtents(pSprite, &top, &bottom);
					if (top <= ceilZ && (pSprite->cstat&48) == 0)
					{
						//viewBackupSpriteLoc(nSprite, pSprite);
						pSprite->z += vdi;
					}
				}
			}
		}
	}
}

void AlignSlopes(void)
{
	sectortype *pSector;
	int nSector;
	for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
	{
		if (pSector->alignto)
		{
			walltype *pWall = &wall[pSector->wallptr+pSector->alignto];
			walltype *pWall2 = &wall[pWall->point2];
			int nNextSector = pWall->nextsector;
			if (nNextSector >= 0)
			{
				int x = (pWall->x+pWall2->x)/2;
				int y = (pWall->y+pWall2->y)/2;
				//viewInterpolateSector(nSector, pSector);
				alignflorslope(nSector, x, y, getflorzofslope(nNextSector, x, y));
				alignceilslope(nSector, x, y, getceilzofslope(nNextSector, x, y));
			}
		}
	}
}

int(*gBusyProc[])(unsigned int, unsigned int) =
{
	VCrushBusy,
	VSpriteBusy,
	VDoorBusy,
	HDoorBusy,
	RDoorBusy,
	StepRotateBusy,
	GenSectorBusy,
	PathBusy
};

void trProcessBusy(void)
{
	memset(velFloor, 0, sizeof(velFloor));
	memset(velCeil, 0, sizeof(velCeil));
	for (int i = gBusyCount-1; i >= 0; i--)
	{
		int nStatus;
		int oldBusy = gBusy[i].at8;
		gBusy[i].at8 = ClipRange(oldBusy+gBusy[i].at4*4, 0, 65536);
		if (!gModernMap || !xsector[sector[gBusy[i].at0].extra].unused1) nStatus = gBusyProc[gBusy[i].atc](gBusy[i].at0, gBusy[i].at8);
		else nStatus = 3; // allow to pause/continue motion for sectors any time by sending special command
		switch (nStatus) {
			case 1:
				gBusy[i].at8 = oldBusy;
				break;
			case 2:
				gBusy[i].at8 = oldBusy;
				gBusy[i].at4 = -gBusy[i].at4;
				break;
			case 3:
				gBusy[i] = gBusy[--gBusyCount];
				break;
		}
	}
	ProcessMotion();
	AlignSlopes();
}

void InitGenerator(int);

void setBasePoint(int objType, int objIdx)
{
	switch (objType) {
		case OBJ_SPRITE:
			baseSprite[objIdx].x = sprite[objIdx].x;
			baseSprite[objIdx].y = sprite[objIdx].y;
			baseSprite[objIdx].z = sprite[objIdx].z;
			break;
		case OBJ_WALL:
			baseWall[objIdx].x = wall[objIdx].x;
			baseWall[objIdx].y = wall[objIdx].y;
			break;
		case OBJ_SECTOR:
			baseFloor[objIdx] = sector[objIdx].floorz;
			baseCeil[objIdx] = sector[objIdx].ceilingz;
			break;
	}
}

void setBaseWallSect(int nSect)
{
	int swal, ewal, i;
	getSectorWalls(nSect, &swal, &ewal);
	for (i = swal; i <= ewal; i++)
		setBasePoint(OBJ_WALL, i);
}

void setBaseSpriteSect(int nSect)
{
	int i;
	i = headspritesect[nSect];
	while(i >= 0)
	{
		setBasePoint(OBJ_SPRITE, i);
		i = nextspritesect[i];
	}
}

void trBasePointInit()
{
	int i;
	for (i = 0; i < numwalls; i++)		setBasePoint(OBJ_WALL, i);
	for (i = 0; i < kMaxSprites; i++)	setBasePoint(OBJ_SPRITE, i);
	for (i = 0; i < numsectors; i++)	setBasePoint(OBJ_SECTOR, i);
}

void trInit(void)
{
	gBusyCount = 0;
	spritetype *pMark1, *pMark2;
	int i, *ptr;
	
	dassert((numsectors >= 0) && (numsectors < kMaxSectors));
	
	gSprNSect.Init();
	
	for (i = 0; i < numwalls; i++)
	{
		setBasePoint(OBJ_WALL, i);
		if (wall[i].extra <= 0)
			continue;
		
		XWALL *pXWall = &xwall[wall[i].extra];
		if (pXWall->state)
			pXWall->busy = 65536;
	}
	
	for (i = 0; i < kMaxSprites; i++)
	{
		sprite[i].inittype = -1;
		if (sprite[i].statnum >= kStatFree)
			continue;
		
		setBasePoint(OBJ_SPRITE, i);
		sprite[i].inittype = sprite[i].type;
	}

	
	for (i = 0; i < numsectors; i++)
	{
		pMark2 = NULL;
		sectortype *pSector = &sector[i];
		setBasePoint(OBJ_SECTOR, i);
		int nXSector = pSector->extra;
		if (nXSector > 0)
		{
			XSECTOR *pXSector = &xsector[nXSector];
			if (pXSector->state)
				pXSector->busy = 65536;
			switch (pSector->type) {
				case kSectorCounter:
					if (gModernMap) pXSector->triggerOff = false;
					else pXSector->triggerOnce = 1;
					evPost(i, 6, 0, kCallbackCounterCheck);
					break;
				case kSectorSlideMarked:
				case kSectorSlide:
					pMark2 =& sprite[pXSector->marker1];
					// no break
				case kSectorRotateMarked:
				case kSectorRotate:
					pMark1 =& sprite[pXSector->marker0];
					if (pMark2) TranslateSector(i, 0, -65536, pMark1->x, pMark1->y, pMark1->x, pMark1->y, pMark1->ang, pMark2->x, pMark2->y, pMark2->ang, pSector->type == kSectorSlide);
					else TranslateSector(i, 0, -65536, pMark1->x, pMark1->y, pMark1->x, pMark1->y, 0, pMark1->x, pMark1->y, pMark1->ang, pSector->type == kSectorRotate);
					
					if (gModernMap && (ptr = gSprNSect.GetSprPtr(i)) != NULL)
					{
						while(*ptr >= 0)
							setBasePoint(OBJ_SPRITE, *ptr++);
					}
					
					setBaseWallSect(i); setBaseSpriteSect(i);
					
					if (pMark2) TranslateSector(i, 0, pXSector->busy, pMark1->x, pMark1->y, pMark1->x, pMark1->y, pMark1->ang, pMark2->x, pMark2->y, pMark2->ang, pSector->type == kSectorSlide);
					else TranslateSector(i, 0, pXSector->busy, pMark1->x, pMark1->y, pMark1->x, pMark1->y, 0, pMark1->x, pMark1->y, pMark1->ang, pSector->type == kSectorRotate);
					// no break
				case kSectorZMotionSprite:
				case kSectorZMotion:
					ZTranslateSector(i, pXSector, pXSector->busy, 1);
					break;
				case kSectorPath:
					InitPath(i, pXSector);
					break;
				default:
					break;
			}
		}
	}
	for (int i = 0; i < kMaxSprites; i++)
	{
		int nXSprite = sprite[i].extra;
		if (sprite[i].statnum < kStatFree && nXSprite > 0)
		{
			XSPRITE *pXSprite = &xsprite[nXSprite];
			if (pXSprite->state)
				pXSprite->busy = 65536;
			switch (sprite[i].type) {
			case kSwitchPadlock:
				pXSprite->triggerOnce = 1;
				break;
            case kModernRandom:
            case kModernRandom2:
				if (!gModernMap || pXSprite->state == pXSprite->restState) break;
				evPost(i, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
				if (pXSprite->waitTime > 0)
					evPost(i, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);
				break;
            case kModernSeqSpawner:
            case kModernObjDataAccumulator:
            case kModernDudeTargetChanger:
            case kModernEffectSpawner:
            case kModernWindGenerator:
				if (pXSprite->state == pXSprite->restState) break;
				evPost(i, 3, 0, kCmdRepeat);
				if (pXSprite->waitTime > 0)
					evPost(i, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);
				break;
            case kGenTrigger:
            case kGenDripWater:
            case kGenDripBlood:
            case kGenMissileFireball:
            case kGenDart:
            case kGenBubble:
            case kGenBubbleMulti:
            case kGenMissileEctoSkull:
            case kGenSound:
				InitGenerator(i);
				break;
			case kThingArmedProxBomb:
				pXSprite->triggerProximity = 1;
				break;
			case kThingFallingRock:
				if (pXSprite->state) sprite[i].flags |= 7;
				else sprite[i].flags &= ~7;
				break;
			}
			if (pXSprite->triggerVector) sprite[i].cstat |= kSprBlock;
			if (pXSprite->triggerPush) sprite[i].cstat |= 4096;
		}
	}

	evSend(0, 0, kChannelLevelStart, kCmdOn);
	switch (gPreview.mode) {
		case kGameModeCoop:
			evSend(0, 0, kChannelLevelStartCoop, kCmdOn);
			break;
		case kGametModeDeathmatch:
			evSend(0, 0, kChannelLevelStartMatch, kCmdOn);
			break;
		case kGameModeFlags:
			evSend(0, 0, kChannelLevelStartMatch, kCmdOn);
			evSend(0, 0, kChannelLevelStartTeamsOnly, kCmdOn);
			break;
	}
}

void InitGenerator(int nSprite)
{
	spritetype *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	XSPRITE *pXSprite = &xsprite[nXSprite];
	switch (sprite[nSprite].type) {
		case kGenTrigger:
			pSprite->cstat &= ~kSprBlock;
			pSprite->cstat |= kSprInvisible;
			break;
	}
	if (pXSprite->state != pXSprite->restState && pXSprite->busyTime > 0)
		evPost(nSprite, 3, (120*(pXSprite->busyTime+BiRandom(pXSprite->data1)))/10, kCmdRepeat);
}

void ActivateGenerator(int nSprite)
{
	spritetype *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	XSPRITE *pXSprite = &xsprite[nXSprite];
	
	spritetype* pEffect;
	int top, bot;

	switch (pSprite->type) {
		case kGenDripWater:
		case kGenDripBlood:
			GetSpriteExtents(pSprite, &top, &bot);
			pEffect = gFX.fxSpawn((pSprite->type == kGenDripWater) ? kFXWaterDripThingReplace : kFXBloodDripThingReplace, pSprite->sectnum, pSprite->x, pSprite->y, bot, 0);
			if (pEffect)
			{
				pEffect->xrepeat = pSprite->xrepeat;
				pEffect->yrepeat = pSprite->yrepeat;
			}
		    //actSpawnThing(pSprite->sectnum, pSprite->x, pSprite->y, bot, (pSprite->type == kGenDripWater) ? kThingDripWater : kThingDripBlood);
			break;
		case kGenSound:
			sfxPlay3DSound(pSprite, pXSprite->data2, -1, 0);
			break;
		case kGenMissileFireball:
			switch (pXSprite->data2) {
				case 0:
					FireballTrapSeqCallback(3, nXSprite);
					break;
				case 1:
					seqSpawn(35, 3, nXSprite, nFireballTrapClient);
					break;
				case 2:
					seqSpawn(36, 3, nXSprite, nFireballTrapClient);
					break;
			}
			break;
		case kGenMissileEctoSkull:
			break;
		case kGenBubble:
		case kGenBubbleMulti:
			GetSpriteExtents(pSprite, &top, &bot);
			gFX.fxSpawn((pSprite->type == kGenBubble) ? FX_23 : FX_26, pSprite->sectnum, pSprite->x, pSprite->y, top, 0);
			break;
	}
}

void TreeToGibCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    pSprite->type = kThingObjectExplode;
    pXSprite->state = 1;
    pXSprite->data1 = 15;
    pXSprite->data2 = 0;
    pXSprite->data3 = 0;
    pXSprite->health = thingInfo[17].startHealth;
    pXSprite->data4 = 312;
    pSprite->cstat |= 257;
}

void FireballTrapSeqCallback(int, int nXSprite)
{
	XSPRITE *pXSprite = &xsprite[nXSprite];
	int nSprite = pXSprite->reference;
	spritetype *pSprite = &sprite[nSprite];
	if (pSprite->cstat&32)
		actFireMissile(pSprite, 0, 0, 0, 0, (pSprite->cstat&8) ? 0x4000 : -0x4000, kMissileFireball);
	else
		actFireMissile(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, kMissileFireball);
}


void MGunFireSeqCallback(int, int nXSprite)
{
	int nSprite = xsprite[nXSprite].reference;
	spritetype *pSprite = &sprite[nSprite];
	XSPRITE *pXSprite = &xsprite[nXSprite];
	if (pSprite->type == kTrapMachinegun)
	{
	
		if (pXSprite->data2 > 0 || pXSprite->data1 == 0)
		{
			if (pXSprite->data2 > 0)
			{
				pXSprite->data2--;
				if (pXSprite->data2 == 0)
					evPost(nSprite, 3, 1, kCmdOff);
			}
			
	/* 		int dx = (Cos(pSprite->ang)>>16)+BiRandom(1000);
			int dy = (Sin(pSprite->ang)>>16)+BiRandom(1000);
			int dz = BiRandom(1000);
			actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorBullet); */
			
			sfxPlay3DSound(pSprite, 359, -1, 0);
		}
	}
	else
	{
		int x = pSprite->x;
		int y = pSprite->y;
		int z = pSprite->z;
		int t = (pXSprite->data1<<23)/120;
		int dx = mulscale30(t, Cos(pSprite->ang));
		int dy = mulscale30(t, Sin(pSprite->ang));
		for (int i = 0; i < 2; i++)
		{
			spritetype *pFX = gFX.fxSpawn(FX_32, pSprite->sectnum, x, y, z, 0);
			if (pFX)
			{
				xvel[pFX->index] = dx + BiRandom(0x8888);
				yvel[pFX->index] = dy + BiRandom(0x8888);
				zvel[pFX->index] = BiRandom(0x8888);
			}
		}
	}
}

void MGunOpenSeqCallback(int, int nXSprite)
{
	seqSpawn(39, 3, nXSprite, nMGunFireClient);
}
