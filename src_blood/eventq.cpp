/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of eventq from Nblood adapted for Preview Mode
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

#include "pqueue.h"
#include "common_game.h"
#include "xmpstub.h"
#include "eventq.h"
#include "screen.h"
#include "callback.h"
#include "db.h"
#include "nnexts.h"
#include "preview.h"
#include "xmptrig.h"
#include "xmpconf.h"
#include "xmpmisc.h"

class EventQueue
{
public:
	PriorityQueue<EVENT>* PQueue;
	EventQueue()
	{
		PQueue = NULL;
	}
	bool IsNotEmpty(unsigned int nTime)
	{
		return PQueue->Size() > 0 && nTime >= PQueue->LowestPriority();
	}
	EVENT ERemove(void)
	{
		return PQueue->Remove();
	}
	void Kill(int, int);
	void Kill(int, int, CALLBACK_ID);
};

EventQueue eventQ;
void EventQueue::Kill(int a1, int a2)
{
	PQueue->Kill([=](EVENT nItem)->bool {return nItem.index == a1 && nItem.type == a2; });
}

void EventQueue::Kill(int a1, int a2, CALLBACK_ID a3)
{
	EVENT evn = { (unsigned int)a1, (unsigned int)a2, kCmdCallback, (unsigned int)a3 };
	PQueue->Kill([=](EVENT nItem)->bool {return !memcmp(&nItem, &evn, sizeof(EVENT)); });
}

RXBUCKET rxBucket[kChannelMax+1];

int GetBucketChannel(const RXBUCKET *pRX)
{
	switch (pRX->type) {
		case 6: {
			int nIndex = pRX->index;
			int nXIndex = sector[nIndex].extra;
			dassert(nXIndex > 0);
			return xsector[nXIndex].rxID;
		}
		case 0: {
			int nIndex = pRX->index;
			int nXIndex = wall[nIndex].extra;
			dassert(nXIndex > 0);
			return xwall[nXIndex].rxID;
		}
		case 3: {
			int nIndex = pRX->index;
			int nXIndex = sprite[nIndex].extra;
			dassert(nXIndex > 0);
			return xsprite[nXIndex].rxID;
		}
	}
	return 0;
}

static int CompareChannels(RXBUCKET *a, RXBUCKET *b)
{
	return GetBucketChannel(a) - GetBucketChannel(b);
}

static RXBUCKET *SortGetMiddle(RXBUCKET *a1, RXBUCKET *a2, RXBUCKET *a3)
{
	if (CompareChannels(a1, a2) > 0)
	{
		if (CompareChannels(a1, a3) > 0)
		{
			if (CompareChannels(a2, a3) > 0)
				return a2;
			return a3;
		}
		return a1;
	}
	else
	{
		if (CompareChannels(a1, a3) < 0)
		{
			if (CompareChannels(a2, a3) > 0)
				return a3;
			return a2;
		}
		return a1;
	}
}

static void SortSwap(RXBUCKET *a, RXBUCKET *b)
{
	RXBUCKET t = *a;
	*a = *b;
	*b = t;
}

static void SortRXBucket(int nCount)
{
	RXBUCKET *v144[32];
	int vc4[32];
	int v14 = 0;
	RXBUCKET *pArray = rxBucket;
	while (true)
	{
		while (nCount > 1)
		{
			if (nCount < 16)
			{
				for (int nDist = 3; nDist > 0; nDist -= 2)
				{
					for (RXBUCKET *pI = pArray+nDist; pI < pArray+nCount; pI += nDist)
					{
						for (RXBUCKET *pJ = pI; pJ > pArray && CompareChannels(pJ-nDist, pJ) > 0; pJ -= nDist)
						{
							SortSwap(pJ, pJ-nDist);
						}
					}
				}
				break;
			}
			RXBUCKET *v30, *vdi, *vsi;
			vdi = pArray + nCount / 2;
			if (nCount > 29)
			{
				v30 = pArray;
				vsi = pArray + nCount-1;
				if (nCount > 42)
				{
					int v20 = nCount / 8;
					v30 = SortGetMiddle(v30, v30+v20, v30+v20*2);
					vdi = SortGetMiddle(vdi-v20, vdi, vdi+v20);
					vsi = SortGetMiddle(vsi-v20*2, vsi-v20, vsi);
				}
				vdi = SortGetMiddle(v30, vdi, vsi);
			}
			RXBUCKET v44 = *vdi;
			RXBUCKET *vc = pArray;
			RXBUCKET *v8 = pArray+nCount-1;
			RXBUCKET *vbx = vc;
			RXBUCKET *v4 = v8;
			while (true)
			{
				while (vbx <= v4)
				{
					int nCmp = CompareChannels(vbx, &v44);
					if (nCmp > 0)
						break;
					if (nCmp == 0)
					{
						SortSwap(vbx, vc);
						vc++;
					}
					vbx++;
				}
				while (vbx <= v4)
				{
					int nCmp = CompareChannels(v4, &v44);
					if (nCmp < 0)
						break;
					if (nCmp == 0)
					{
						SortSwap(v4, v8);
						v8--;
					}
					v4--;
				}
				if (vbx > v4)
					break;
				SortSwap(vbx, v4);
				v4--;
				vbx++;
			}
			RXBUCKET *v2c = pArray+nCount;
			int vt = ClipHigh(vbx-vc, vc-pArray);
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&vbx[i-vt], &pArray[i]);
			}
			vt = ClipHigh(v8-v4, v2c-v8-1);
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&v2c[i-vt], &vbx[i]);
			}
			int vvsi = v8-v4;
			int vvdi = vbx-vc;
			if (vvsi >= vvdi)
			{
				vc4[v14] = vvsi;
				v144[v14] = v2c-vvsi;
				nCount = vvdi;
				v14++;
			}
			else
			{
				vc4[v14] = vvdi;
				v144[v14] = pArray;
				nCount = vvsi;
				pArray = v2c - vvsi;
				v14++;
			}
		}
		if (v14 == 0)
			return;
		v14--;
		pArray = v144[v14];
		nCount = vc4[v14];
	}
}

unsigned short bucketHead[1024+1];

void evInit(void)
{
	if (eventQ.PQueue)
		delete eventQ.PQueue;

	eventQ.PQueue = new StdPriorityQueue<EVENT>();
	eventQ.PQueue->Clear();
	int nCount = 0;
	for (int i = 0; i < numsectors; i++)
	{
		int nXSector = sector[i].extra;
		if (nXSector >= kMaxXSectors)
			ThrowError("Invalid xsector reference in sector %d", i);
		if (nXSector > 0 && xsector[nXSector].rxID > 0)
		{
			dassert(nCount < kChannelMax);
			rxBucket[nCount].type = 6;
			rxBucket[nCount].index = i;
			nCount++;
		}
	}
	for (int i = 0; i < numwalls; i++)
	{
		int nXWall = wall[i].extra;
		if (nXWall >= kMaxXWalls)
			ThrowError("Invalid xwall reference in wall %d", i);
		if (nXWall > 0 && xwall[nXWall].rxID > 0)
		{
			dassert(nCount < kChannelMax);
			rxBucket[nCount].type = 0;
			rxBucket[nCount].index = i;
			nCount++;
		}
	}
	for (int i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			int nXSprite = sprite[i].extra;
			if (nXSprite >= kMaxXSprites)
				ThrowError("Invalid xsprite reference in sprite %d", i);
			if (nXSprite > 0 && xsprite[nXSprite].rxID > 0)
			{
				dassert(nCount < kChannelMax);
				rxBucket[nCount].type = 3;
				rxBucket[nCount].index = i;
				nCount++;
			}
		}
	}
	SortRXBucket(nCount);
	int i, j = 0;
	for (i = 0; i < 1024; i++)
	{
		bucketHead[i] = j;
		while(j < nCount && GetBucketChannel(&rxBucket[j]) == i)
			j++;
	}
	bucketHead[i] = j;
}

char evGetSourceState(int nType, int nIndex)
{
	switch (nType)
	{
	case 6:
	{
		int nXIndex = sector[nIndex].extra;
		dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
		return xsector[nXIndex].state;
	}
	case 0:
	{
		int nXIndex = wall[nIndex].extra;
		dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
		return xwall[nXIndex].state;
	}
	case 3:
	{
		int nXIndex = sprite[nIndex].extra;
		dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
		return xsprite[nXIndex].state;
	}
	}
	return 0;
}

void evSend(int nIndex, int nType, int rxId, COMMAND_ID command)
{
	int i;
	switch (command) {
		case kCmdState:
			command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
			break;
		case kCmdNotState:
			command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
			break;
		default:
			break;
	}
	
	EVENT event;
	event.index = nIndex;
	event.type = nType;
	event.cmd = command;

	switch (rxId) {
		case kChannelLevelStart:
		case kChannelLevelStartCoop:
		case kChannelLevelStartMatch:
			if (!gPreview.triggerStart) return;
			break;
		case kChannelLevelExitNormal:
			previewMessage("Trigger normal level end.");
			BeepOk();
			return;
		case kChannelLevelExitSecret:
			previewMessage("Trigger secret level end.");
			BeepOk();
			return;
		case kChannelSecretFound:
			switch (command - kCmdNumberic) {
				case 0:
					previewMessage("A secret is revealed.");
					gPreview.score+=kPreviewScoreSecret;
					BeepOk();
					break;
				case 1:
					previewMessage("You found a SUPER secret.");
					gPreview.score+=kPreviewScoreSuperSecret;
					BeepOk();
					break;
				default:
					previewMessage("Wrong command for triggering secret!");
					BeepFail();
					break;
			}
			return;
		case kChannelTextOver:
			if (nType != OBJ_SPRITE) return; // only sprites can send messages
			else if (command >= kCmdNumberic)
			{
				if ((i = 1 + (command - kCmdNumberic)) <= 32) {
					sprintf(buffer, "message%d", i); sprintf(buffer3, "Trigger INI %s", buffer);
					previewMessage(gPreview.pEpisode->GetKeyString(getFilename(gPaths.maps, buffer2), buffer, buffer3));
					BeepOk();
					return;
				}
			}
			previewMessage("Wrong command for triggering INI message!");
			BeepFail();
			return;
		case kChannelRemoteBomb0:
		case kChannelRemoteBomb1:
		case kChannelRemoteBomb2:
		case kChannelRemoteBomb3:
		case kChannelRemoteBomb4:
		case kChannelRemoteBomb5:
		case kChannelRemoteBomb6:
		case kChannelRemoteBomb7:
			return;
	}

	for (int i = bucketHead[rxId]; i < bucketHead[rxId+1]; i++) {
		if (event.type != rxBucket[i].type || event.index != rxBucket[i].index) {
			switch (rxBucket[i].type) {
				case 6:
					trMessageSector(rxBucket[i].index, event);
					break;
				case 0:
					trMessageWall(rxBucket[i].index, event);
					break;
				case 3:
				{
					int nSprite = rxBucket[i].index;
					spritetype *pSprite = &sprite[nSprite];
					if (pSprite->flags&32)
						continue;
					int nXSprite = pSprite->extra;
					if (nXSprite > 0)
					{
						XSPRITE *pXSprite = &xsprite[nXSprite];
						if (pXSprite->rxID > 0)
							trMessageSprite(nSprite, event);
					}
					break;
				}
			}
		}
	}
}

void evPost(int nIndex, int nType, unsigned int nDelta, COMMAND_ID command) {
	dassert(command != kCmdCallback);
	if (command == kCmdState) command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
	else if (command == kCmdNotState) command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
	EVENT evn = {};
	evn.index = nIndex;
	evn.type = nType;
	evn.cmd = command;
	eventQ.PQueue->Insert((int)gFrameClock+nDelta, evn);
}

void evPost(int nIndex, int nType, unsigned int nDelta, CALLBACK_ID callback) {
	EVENT evn = {};
	evn.index = nIndex;
	evn.type = nType;
	evn.cmd = kCmdCallback;
	evn.funcID = callback;
	eventQ.PQueue->Insert((int)gFrameClock+nDelta, evn);
}

void evProcess(unsigned int nTime)
{
#if 0
	while (1)
	{
		// Inlined?
		char bDone;
		if (eventQ.fNodeCount > 0 && nTime >= eventQ.queueItems[1])
			bDone = 1;
		else
			bDone = 0;
		if (!bDone)
			break;
#endif
	while(eventQ.IsNotEmpty(nTime))
	{
		EVENT event = eventQ.ERemove();
		if (event.cmd == kCmdCallback)
		{
			dassert(event.funcID < kCallbackMax);
			dassert(gCallback[event.funcID] != NULL);
			gCallback[event.funcID](event.index);
		}
		else
		{
			switch (event.type)
			{
			case 6:
				trMessageSector(event.index, event);
				break;
			case 0:
				trMessageWall(event.index, event);
				break;
			case 3:
				trMessageSprite(event.index, event);
				break;
			}
		}
	}
}

void evKill(int a1, int a2)
{
	eventQ.Kill(a1, a2);
}

void evKill(int a1, int a2, CALLBACK_ID a3)
{
	eventQ.Kill(a1, a2, a3);
}