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

#include "xmpmaped.h"
#include "preview.h"
#include "aadjust.h"
#include "sectorfx.h"
#include "seq.h"
#include "nnextsif.h"
#include "tile.h"

BOOL gEventRedirectsUsed = FALSE;
IDLIST gProxySpritesList(false);	// list of additional sprites which can be triggered by Proximity
IDLIST gSightSpritesList(false);	// list of additional sprites which can be triggered by Sight
IDLIST gImpactSpritesList(false);	// list of additional sprites which can be triggered by Impact
IDLIST gPhysSpritesList(false);		// list of additional sprites which can be affected by physics
SPRITEMASS gSpriteMass[];   		// cache for getSpriteMassBySize();

EXTERNAL_FILES_LIST gExternFiles[] =
{
    { kCdudeFileNamePrefixWild, kCdudeFileExt },
};

int nnExtResAddExternalFiles(Resource* pIn, const char* pPath, EXTERNAL_FILES_LIST* pList, int nLen)
{
    char match[BMAX_PATH], tmp[BMAX_PATH];
    EXTERNAL_FILES_LIST* ptr; BDIR* pDir; Bdirent* pEntry;
    int i, nRetn = 0;
	char *name, *ext;
    char* pMatch;
	
	buildprintf("Adding external files from: \"%s\"...\n", pPath);
	
    for (i = 0; i < nLen; i++)
    {
        ptr = &pList[i];
		
        Bmemset(match, 0, sizeof(match)); pMatch = match;
        pMatch += Bsprintf(pMatch, "%s",  (ptr->name) ? ptr->name : "*");
        pMatch += Bsprintf(pMatch, ".%s", (ptr->ext) ? ptr->ext   : "*");

        if ((pDir = Bopendir(pPath)) != NULL)
        {
            while ((pEntry = Breaddir(pDir)) != NULL)
            {
				if (Bwildmatch(pEntry->name, match))
                {
					pathSplit2(pEntry->name, tmp, NULL, NULL, &name, &ext);
					if (ext[0] == '.')
						ext = &ext[1];

					buildprintf("Added: \"%s\"\n", pEntry->name);
					pIn->AddExternalResource(name, ext, 0, 0, pPath);
                    nRetn++;
                }
            }

            Bclosedir(pDir);
        }
    }

    return nRetn;
}

DICTNODE* nnExtResFileSearch(Resource* pIn, const char* pName, const char* pExt, char external)
{
    int nLenA = Bstrlen(pName), nLenB;
    int i = pIn->count;
    DICTNODE* pFile;
	
    while(--i >= 0)  // from bottom we can meet it faster
    {
        pFile = &pIn->dict[i];
        if ((external && (pFile->flags & DICT_EXTERNAL)) || !external)
        {
            if (Bstrcasecmp(pFile->type, pExt) == 0 && Bstrncasecmp(pName, pFile->name, nLenA) == 0)
            {
                nLenB = Bstrlen(pFile->name);
                if (nLenB == nLenA || (nLenB > nLenA && pFile->name[nLenA] == '_'))
					return pFile;
            }
        }
    }

    return NULL;
}

DICTNODE* nnExtResFileSearch(Resource* pIn, int nID, const char* pExt)
{
    int i = pIn->count;
    DICTNODE* pFile;
	
    while(--i >= 0)
    {
        pFile = &pIn->dict[i];
		if (pFile->id == nID && !(pFile->flags & DICT_EXTERNAL))
		{
			if (Bstrcasecmp(pFile->type, pExt) == 0)
				return pFile;
		}
    }

    return NULL;
}


int getSpriteMassBySize(spritetype* pSprite) {
    int mass = 0; int seqId = -1; int clipDist = pSprite->clipdist; Seq* pSeq = NULL;
    if (pSprite->extra < 0) {
        ThrowError("getSpriteMassBySize: pSprite->extra < 0");

    } else if (IsDudeSprite(pSprite)) {

        switch (pSprite->type) {
        case kDudePodMother: // fake dude, no seq
            break;
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
            seqId = xsprite[pSprite->extra].data2;
            clipDist = pSprite->clipdist;
            break;
        default:
            seqId   = -1;
            break;
        }

    } else  {

        seqId = seqGetID(3, pSprite->extra);

    }

    SPRITEMASS* cached = &gSpriteMass[pSprite->extra];
    if (((seqId >= 0 && seqId == cached->seqId) || pSprite->picnum == cached->picnum) && pSprite->xrepeat == cached->xrepeat &&
        pSprite->yrepeat == cached->yrepeat && clipDist == cached->clipdist) {
        return cached->mass;
    }

    short picnum = pSprite->picnum;
    short massDiv = 30;  short addMul = 2; short subMul = 2;

    if (seqId >= 0) {
        DICTNODE* hSeq = gSysRes.Lookup(seqId, "SEQ");
        if (hSeq)
        {
            pSeq = (Seq*)gSysRes.Load(hSeq);
            picnum = seqGetTile(&pSeq->frames[0]);
        } else
            picnum = pSprite->picnum;
    }

    clipDist = ClipLow(pSprite->clipdist, 1);
    short x = tilesizx[picnum];         short y = tilesizy[picnum];
    short xrepeat = pSprite->xrepeat; 	short yrepeat = pSprite->yrepeat;

    // take surface type into account
    switch (tileGetSurfType(pSprite->index + 0xc000)) {
        case 1:  massDiv = 16; break; // stone
        case 2:  massDiv = 18; break; // metal
        case 3:  massDiv = 21; break; // wood
        case 4:  massDiv = 25; break; // flesh
        case 5:  massDiv = 28; break; // water
        case 6:  massDiv = 26; break; // dirt
        case 7:  massDiv = 27; break; // clay
        case 8:  massDiv = 35; break; // snow
        case 9:  massDiv = 22; break; // ice
        case 10: massDiv = 37; break; // leaves
        case 11: massDiv = 33; break; // cloth
        case 12: massDiv = 36; break; // plant
        case 13: massDiv = 24; break; // goo
        case 14: massDiv = 23; break; // lava
    }

    mass = ((x + y) * (clipDist / 2)) / massDiv;

    if (xrepeat > 64) mass += ((xrepeat - 64) * addMul);
    else if (xrepeat < 64 && mass > 0) {
        for (int i = 64 - xrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (yrepeat > 64) mass += ((yrepeat - 64) * addMul);
    else if (yrepeat < 64 && mass > 0) {
        for (int i = 64 - yrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (mass <= 0) cached->mass = 1 + Random(10);
    else cached->mass = ClipRange(mass, 1, 65535);

    cached->airVel = ClipRange(400 - cached->mass, 32, 400);
    cached->fraction = ClipRange(60000 - (cached->mass << 7), 8192, 60000);

    cached->xrepeat = pSprite->xrepeat;             cached->yrepeat = pSprite->yrepeat;
    cached->picnum = pSprite->picnum;               cached->seqId = seqId;
    cached->clipdist = pSprite->clipdist;

    return cached->mass;
}

void debrisConcuss(int nOwner, int nDebris, int x, int y, int z, int dmg)
{
	bool thing;
	int dx, dy, dz, size, t;
	spritetype* pSpr = &sprite[nDebris];
	if (pSpr->extra <= 0)
		return;
	
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	
	dx    = pSpr->x - x; dy = pSpr->y - y; dz = (pSpr->z - z) >> 4;
	dmg   = scale(0x40000, dmg, 0x40000 + dx * dx + dy * dy + dz * dz);
	size  = (tilesizx[pSpr->picnum] * pSpr->xrepeat * tilesizy[pSpr->picnum] * pSpr->yrepeat) >> 1;
	thing = (pSpr->type >= kThingBase && pSpr->type < kThingMax);
	
	if (pXSpr->physAttr & kPhysDebrisExplode)
	{
		if (gSpriteMass[pSpr->extra].mass > 0)
		{
			t = scale(dmg, size, gSpriteMass[pSpr->extra].mass);
			xvel[pSpr->index] += mulscale16(t, dx);
			yvel[pSpr->index] += mulscale16(t, dy);
			zvel[pSpr->index] += mulscale16(t, dz);
		}

		if (thing)
			pSpr->statnum = kStatThing; // temporary change statnum property
	}

	actDamageSprite(nOwner, pSpr, kDamageExplode, dmg);
	
	if (thing)
		pSpr->statnum = kStatDecoration; // return statnum property back
    
}

void debrisBubble(int nSprite) {
    
    spritetype* pSprite = &sprite[nSprite];
    
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    for (unsigned int i = 0; i < 1 + Random(5); i++) {
        
        int nDist = (pSprite->xrepeat * (tilesizx[pSprite->picnum] >> 1)) >> 2;
        int nAngle = Random(2048);
        int x = pSprite->x + mulscale30(nDist, Cos(nAngle));
        int y = pSprite->y + mulscale30(nDist, Sin(nAngle));
        int z = bottom - Random(bottom - top);
        spritetype* pFX = gFX.fxSpawn((FX_ID)(FX_23 + Random(3)), pSprite->sectnum, x, y, z);
        if (pFX) {
            xvel[pFX->index] = xvel[nSprite] + BiRandom(0x1aaaa);
            yvel[pFX->index] = yvel[nSprite] + BiRandom(0x1aaaa);
            zvel[pFX->index] = zvel[nSprite] + BiRandom(0x1aaaa);
        }

    }
}

void debrisMove(int nSpr) {

    spritetype* pSpr = &sprite[nSpr];
	if (pSpr->sectnum < 0 || pSpr->extra <= 0)
		return;

	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	int nXSpr = pSpr->extra;	int nSect = pSpr->sectnum;
	
    int top, bottom, i;
    GetSpriteExtents(pSpr, &top, &bottom);

    int moveHit = 0;
    int floorDist = (bottom - pSpr->z) >> 2;
    int ceilDist = (pSpr->z - top) >> 2;
    int clipDist = pSpr->clipdist << 2;
    int mass = gSpriteMass[nXSpr].mass;

    bool uwater = false;
    int tmpFraction = gSpriteMass[pSpr->extra].fraction;
    if (sector[nSect].extra >= 0 && xsector[sector[nSect].extra].underwater)
	{
        tmpFraction >>= 1;
        uwater = true;
    }

    if (xvel[nSpr] || yvel[nSpr])
	{
        short oldcstat = pSpr->cstat;
        pSpr->cstat &= ~(kSprBlock | kSprHitscan);
        moveHit = gSpriteHit[nXSpr].hit = ClipMove((int*)&pSpr->x, (int*)&pSpr->y, (int*)&pSpr->z, &nSect, xvel[nSpr] >> 12,
            yvel[nSpr] >> 12, clipDist, ceilDist, floorDist, CLIPMASK0);

        pSpr->cstat = oldcstat;
        if (pSpr->sectnum != nSect)
		{
            if (nSect < 0)
				return;
			
            ChangeSpriteSect(nSpr, nSect);
        }

        if (sector[nSect].type >= kSectorPath && sector[nSect].type <= kSectorRotate)
		{
            short nSector2 = nSect;
            if (pushmove(&pSpr->x, &pSpr->y, &pSpr->z, &nSector2, clipDist, ceilDist, floorDist, CLIPMASK0) != -1)
                nSect = nSector2;
        }

        if ((gSpriteHit[nXSpr].hit & 0xc000) == 0x8000)
		{
            i = moveHit = gSpriteHit[nXSpr].hit & 0x3fff;
            actWallBounceVector((int*)&xvel[nSpr], (int*)&yvel[nSpr], i, tmpFraction);
        }

    }
	else if (!FindSector(pSpr->x, pSpr->y, pSpr->z, &nSect))
	{
        return;
    }

    if (pSpr->sectnum != nSect)
	{
        if (nSect < 0)
			return;
		
        ChangeSpriteSect(nSpr, nSect);
        nSect = pSpr->sectnum;
    }

    if (sector[nSect].extra > 0)
        uwater = xsector[sector[nSect].extra].underwater;

    if (zvel[nSpr])
        pSpr->z += zvel[nSpr] >> 8;

    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSpr, &ceilZ, &ceilHit, &floorZ, &floorHit, clipDist, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
    GetSpriteExtents(pSpr, &top, &bottom);

    if ((pXSpr->physAttr & kPhysDebrisSwim) && uwater)
	{
        int vc = 0;
        int cz = getceilzofslope(nSect, pSpr->x, pSpr->y);
        int fz = getflorzofslope(nSect, pSpr->x, pSpr->y);
        int div = ClipLow(bottom - top, 1);

        if (gLowerLink[nSect] >= 0) cz += (cz < 0) ? 0x500 : -0x500;
        if (top > cz && (!(pXSpr->physAttr & kPhysDebrisFloat) || fz <= bottom << 2))
            zvel[nSpr] -= divscale8((bottom - ceilZ) >> 6, mass);

        if (fz < bottom)
            vc = 58254 + ((bottom - fz) * -80099) / div;

        if (vc) {
            pSpr->z += ((vc << 2) >> 1) >> 8;
            zvel[nSpr] += vc;
        }

    }
	else if ((pXSpr->physAttr & kPhysGravity) && bottom < floorZ)
	{
        pSpr->z += 455;
        zvel[nSpr] += 58254;
    }

    if ((i = CheckLink(pSpr, pSpr->sectnum, 0)) != 0)
	{
        GetZRange(pSpr, &ceilZ, &ceilHit, &floorZ, &floorHit, clipDist, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
        if (!(pSpr->cstat & kSprInvisible))
		{
            switch (i) {
                case kMarkerUpWater:
                case kMarkerUpGoo:
                    int pitch = (150000 - (gSpriteMass[pSpr->extra].mass << 9)) + BiRandom2(8192);
                    sfxPlay3DSoundCP(pSpr, 720, -1, 0, pitch, 75 - Random(40));
                    break;
            }
        }
    }

    GetSpriteExtents(pSpr, &top, &bottom);

    if (floorZ <= bottom) {

        gSpriteHit[nXSpr].florhit = floorHit;
        int v30 = zvel[nSpr] - velFloor[pSpr->sectnum];

        if (v30 > 0)
		{
            pXSpr->physAttr |= kPhysFalling;
            actFloorBounceVector((int*)&xvel[nSpr], (int*)&yvel[nSpr], (int*)&v30, pSpr->sectnum, tmpFraction);
            zvel[nSpr] = v30;

            if (klabs(zvel[nSpr]) < 0x10000)
			{
                zvel[nSpr] = velFloor[pSpr->sectnum];
                pXSpr->physAttr &= ~kPhysFalling;
            }

            moveHit = floorHit;
            spritetype* pFX = NULL; spritetype* pFX2 = NULL;
            switch (tileGetSurfType(floorHit)) {
            case kSurfLava:
                if ((pFX = gFX.fxSpawn(FX_10, pSpr->sectnum, pSpr->x, pSpr->y, floorZ)) == NULL) break;
                for (i = 0; i < 7; i++) {
                    if ((pFX2 = gFX.fxSpawn(FX_14, pFX->sectnum, pFX->x, pFX->y, pFX->z)) == NULL) continue;
                    xvel[pFX2->index] = BiRandom(0x6aaaa);
                    yvel[pFX2->index] = BiRandom(0x6aaaa);
                    zvel[pFX2->index] = -Random(0xd5555);
                }
                break;
            case kSurfWater:
                gFX.fxSpawn(FX_9, pSpr->sectnum, pSpr->x, pSpr->y, floorZ);
                break;
            }
        }
		else if (zvel[nSpr] == 0)
		{
            pXSpr->physAttr &= ~kPhysFalling;
        }

    }
	else
	{
        gSpriteHit[nXSpr].florhit = 0;
        if (pXSpr->physAttr & kPhysGravity)
            pXSpr->physAttr |= kPhysFalling;
    }

    if (top <= ceilZ)
	{
        gSpriteHit[nXSpr].ceilhit = moveHit = ceilHit;
        pSpr->z += ClipLow(ceilZ - top, 0);
        if (zvel[nSpr] <= 0 && (pXSpr->physAttr & kPhysFalling))
            zvel[nSpr] = mulscale16(-zvel[nSpr], 0x2000);
    }
	else
	{
        gSpriteHit[nXSpr].ceilhit = 0;
        GetSpriteExtents(pSpr, &top, &bottom);
    }

    if (moveHit && pXSpr->triggerImpact && !pXSpr->locked && !pXSpr->isTriggered && (pXSpr->state == pXSpr->restState || pXSpr->interruptable))
	{
        if (IsThingSprite(pSpr))
            changespritestat(nSpr, kStatThing);

        trTriggerSprite(pSpr->index, pXSpr, kCmdToggle);

    }

    if (!xvel[nSpr] && !yvel[nSpr]) return;
    else if ((floorHit & 0xc000) == 0xc000)
	{

        int nHitSprite = floorHit & 0x3fff;
        if ((sprite[nHitSprite].cstat & 0x30) == 0)
		{
            xvel[nSpr] += mulscale2(4, pSpr->x - sprite[nHitSprite].x);
            yvel[nSpr] += mulscale2(4, pSpr->y - sprite[nHitSprite].y);
            return;
        }
    }

    pXSpr->height = ClipLow(floorZ - bottom, 0) >> 8;
    if (uwater || pXSpr->height >= 0x100)
        return;

    int nDrag = 0x2a00;
    if (pXSpr->height > 0)
        nDrag -= scale(nDrag, pXSpr->height, 0x100);

    xvel[nSpr] -= mulscale16r(xvel[nSpr], nDrag);
    yvel[nSpr] -= mulscale16r(yvel[nSpr], nDrag);
    if (approxDist(xvel[nSpr], yvel[nSpr]) < 0x1000)
        xvel[nSpr] = yvel[nSpr] = 0;

}

bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event) {

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

        switch (pSprite->type) {
            case kModernCondition:
            case kModernConditionFalse:
                pXSprite->restState = 0;
                if (pXSprite->busyTime <= 0) break;
                else if (!pXSprite->locked) pXSprite->busy = 0;
                break;
            case kModernEffectSpawner:
                if (!pXSprite->locked) break;
                break;
        }
       
        return true;
    }
    else if (event.cmd == kCmdDudeFlagsSet)
    {
        if (event.type != EVOBJ_SPRITE)
        {
            previewMessage("Only sprites could use command #%d", event.cmd);
            return true;
        }
        else if (xspriRangeIsFine(sprite[event.index].extra))
        {
			return true; // not implemented yet
        }
    }

    if (pSprite->statnum == kStatDude && IsDudeSprite(pSprite))
	{
        switch (event.cmd) {
            case kCmdOff:
                if (pXSprite->state) SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdOn:
                if (!pXSprite->state) SetSpriteState(nSprite, pXSprite, 1);
                break;
            case kCmdDudeFlagsSet:
				// not implemented yet
                break;
            default:
                if (!pXSprite->state) evPost(nSprite, EVOBJ_SPRITE, 0, kCmdOn);
                else evPost(nSprite, EVOBJ_SPRITE, 0, kCmdOff);
                break;
        }

        return true;
    }

    switch (pSprite->type) {
        default:
            return false; // no modern type found to work with, go normal OperateSprite();
		case kModernThingEnemyLifeLeech:
		case kModernPlayerControl:
		case kModernThingTNTProx:
		case kModernDudeTargetChanger:
			return true;
        case kModernCondition:
        case kModernConditionFalse:
            if (!pXSprite->isTriggered) useCondition(pSprite, pXSprite, event);
			return true;
		case kThingBloodBits:
        case kThingBloodChunks:
            if (pSprite->inittype < kDudeBase || pSprite->inittype >= kDudeMax) return false;
            else if (event.cmd != kCmdToggle && event.cmd != kCmdOff && event.cmd != kCmdSpriteImpact) return true;
            return false;
        // add spawn random dude feature - works only if at least 2 data fields are not empty.
        case kMarkerDudeSpawn:
            if (!(pSprite->flags & kModernTypeFlag4)) useDudeSpawn(pXSprite, pSprite);
            else if (pXSprite->txID) evSend(nSprite, EVOBJ_SPRITE, pXSprite->txID, kCmdModernUse);
			pSprite->xrepeat = 0;
			return true;
        case kModernCustomDudeSpawn:
            if (!(pSprite->flags & kModernTypeFlag4)) useCustomDudeSpawn(pXSprite, pSprite);
            else if (pXSprite->txID) evSend(nSprite, EVOBJ_SPRITE, pXSprite->txID, kCmdModernUse);
            pSprite->xrepeat = 0;
			return true;
        case kModernRandomTX: // random Event Switch takes random data field and uses it as TX ID
        case kModernSequentialTX: // sequential Switch takes values from data fields starting from data1 and uses it as TX ID
            if (pXSprite->command == kCmdLink) return true; // work as event redirector
            switch (pSprite->type) {
                case kModernRandomTX:
                    useRandomTx(pXSprite, (COMMAND_ID)pXSprite->command, true);
                    break;
                case kModernSequentialTX:
                    if (!(pSprite->flags & kModernTypeFlag1)) useSequentialTx(pXSprite, (COMMAND_ID)pXSprite->command, true);
                    else seqTxSendCmdAll(pXSprite, pSprite->index, (COMMAND_ID)pXSprite->command, false);
                    break;
            }
            return true;
        case kModernSpriteDamager:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pXSprite->data1 == 0 && sectRangeIsFine(pSprite->sectnum)) useSpriteDamager(pXSprite, EVOBJ_SECTOR, pSprite->sectnum);
                    else if (pXSprite->data1 >= 666 && pXSprite->data1 < 669) useSpriteDamager(pXSprite, -1, -1);
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
            return true;
        case kMarkerWarpDest:
            if (pXSprite->txID <= 0) {
                if (pXSprite->data1 == 1 && SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    useTeleportTarget(pXSprite, NULL);
                return true;
            }
            fallthrough__;
        case kModernObjPropertiesChanger:
            if (pXSprite->txID <= 0)
            {
                if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    usePropertiesChanger(pXSprite, -1, -1);
                return true;
            }
            fallthrough__;
        case kModernSlopeChanger:
        case kModernObjSizeChanger:
        case kModernObjPicnumChanger:
        case kModernSectorFXChanger:
        case kModernObjDataChanger:
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return true;
        case kModernSeqSpawner:
        case kModernEffectSpawner:
            switch (event.cmd) {
            case kCmdOff:
                if (pXSprite->state == 1)
                {
                    SetSpriteState(nSprite, pXSprite, 0);
                }
                break;
            case kCmdOn:
                evKill(nSprite, 3); // queue overflow protect
                if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                if (pSprite->type == kModernSeqSpawner) seqSpawnerOffSameTx(pXSprite);
                fallthrough__;
            case kCmdRepeat:
                if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                else if (pSprite->type == kModernSeqSpawner) useSeqSpawnerGen(pXSprite, 3, pSprite->index);
                else useEffectGen(pXSprite, NULL);

                if (pXSprite->busyTime > 0)
                    evPost(nSprite, 3, ClipLow((int(pXSprite->busyTime) + BiRandom(pXSprite->data1)) * 120 / 10, 0), kCmdRepeat);
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
                    windGenStopWindOnSectors(pXSprite);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useSectorWindGen(pXSprite, NULL);

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernVelocityChanger:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useVelocityChanger(pXSprite, kCauserGame, EVOBJ_SECTOR, pSprite->sectnum);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kGenTrigger:
            if (!(pSprite->flags & kModernTypeFlag1)) return false; // work as vanilla generator
            switch (event.cmd) { // work as fast generator
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID)
                        evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernObjDataAccumulator:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    // force OFF after *all* TX objects reach the goal value
                    if (pSprite->flags == kModernTypeFlag0 && incDecGoalValueIsReached(pXSprite)) {
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
            return true;
        case kModernRandom:
        case kModernRandom2:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
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
        case kGenModernSound:
            switch (event.cmd) {
            case kCmdOff:
                if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                break;
            case kCmdOn:
                evKill(nSprite, 3); // queue overflow protect
                if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                fallthrough__;
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
            return true;
        case kGenModernMissileUniversal:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID)  modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useUniMissileGen(pXSprite, pSprite);
                    
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);

                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
    }
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
		case kCmdLink:
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

IniFile* cdDescriptLoad(int nID)
{
    DICTNODE* hFil;
	IniFile* pIni = NULL;
	
    char tmp[BMAX_PATH]; BYTE* pRawIni = NULL;
    const char* fname = kCdudeFileNamePrefix;
    const char* fext = kCdudeFileExt;
    
    if (rngok(nID, 0, 10000))
    {
        Bsprintf(tmp, "%s%d", fname, nID);
        if ((hFil = nnExtResFileSearch(&gSysRes, tmp, fext)) == NULL) // name not found
			hFil = nnExtResFileSearch(&gSysRes, nID, fext); // try by ID
		
		if (hFil && (pRawIni = (BYTE*)gSysRes.Load(hFil)) != NULL)
			pIni = new IniFile((BYTE*)pRawIni, hFil->size);
    }

    return pIni;
}

#define kParamMax 255

#pragma pack(push, 1)
struct PARAM
{
    unsigned int id             : 8;
    const char* text;
};
#pragma pack(pop)

enum enum_CDUD_POSTURE {
kCdudePosture                   = 0,
kCdudePostureL                  = kCdudePosture,
kCdudePostureC,
kCdudePostureW,
kCdudePostureMax,
};

enum enum_CDUD_STATUS {
kCdudeStatusNormal              = 0x00,
kCdudeStatusAwaked              = 0x01,
kCdudeStatusForceCrouch         = 0x02,
kCdudeStatusSleep               = 0x04,
kCdudeStatusMorph               = 0x08,
kCdudeStatusRespawn             = 0x10,
};

PARAM gParamPosture[] =
{
    {kCdudePostureL,        "Stand"  },
    {kCdudePostureC,        "Crouch" },
    {kCdudePostureW,        "Swim"   },
    {kParamMax, NULL},
};

char gCustomDudeNames[kMaxSprites][32];

int cdParseFindParam(const char* str, PARAM* pDb)
{
    PARAM* ptr = pDb;
    while (ptr->id != kParamMax)
    {
        if (Bstrcasecmp(str, ptr->text) == 0)
            return ptr->id;

        ptr++;
    }

    return -1;
}

char cdParseAnimation(const char* str, unsigned int pOut[kCdudePostureMax])
{
    char key[256], val[256];
	
	int i, nPar;
    if (isarray(str, &i))
    {
        if (i > kCdudePostureMax)
			return false;
		
        i = 0;
        while (enumStr(i, str, key, val))
        {
            nPar = cdParseFindParam(key, gParamPosture);
            if (rngok(nPar, kCdudePosture, kCdudePostureMax))
                pOut[nPar] = atoi(val);
			
            i++;
        }

        return true;
    }
    else if (!isempty(str))
    {
		pOut[kCdudePostureL] = atoi(str);
        return true;
    }

    return false;
}

char cdParseRange(const char* str, int out[2], int nBaseVal)
{
    int nLen, nVal;
    int nMin = out[0];
    int nMax = out[1];
    char chkrng = (nMin && nMax);
    int i = 0;
	
    char tmp[256];
    if (!isempty(str))
    {
        if (isarray(str, &nLen))
        {
            if (nLen > 2)
				return false;
        }

        while (i < 2 && enumStr(i, str, tmp))
        {
            nVal = atoi(tmp);
            nVal = perc2val(nVal, nBaseVal);
            out[i++] = nVal;
        }

        return i;
    }

    return 0;
}

void nnExtSprScaleSet(spritetype* pSpr, int nScale)
{
    pSpr->xrepeat = ClipRange(mulscale8(pSpr->xrepeat, nScale), 0, 255);
    pSpr->yrepeat = ClipRange(mulscale8(pSpr->yrepeat, nScale), 0, 255);
}



void cdGetSeq(spritetype* pSpr) {
	
	dassert(xspriRangeIsFine(pSpr->extra));
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	IniFile* pIni = cdDescriptLoad(pXSpr->data1);
	char respawnMarker = (pSpr->type == kModernCustomDudeSpawn);
	int nSeq = 11520, nScale = 256;
	short pic, xr, yr, pal;
	
	if (pIni && pIni->GetKeyInt("General", "Version", 0) == 2)
	{
		// cdude v2
		unsigned int anim[kCdudePostureMax];
		memset(anim, 0, sizeof(anim));
		
		char* group			= "Animation";
		char* paramIdle		= "Idle";
		char* paramSleep	= "IdleSleep";
		char* paramScale	= "Scale";
		const char* pValue	= NULL;

		if (!(pXSpr->data2 & kCdudeStatusAwaked))
			pValue = pIni->GetKeyString(group, paramSleep, NULL);
		
		if (isempty(pValue))
			pValue = pIni->GetKeyString(group, paramIdle, NULL);
		
		if (cdParseAnimation(pValue, anim))
		{
			if (isUnderwaterSector(pSpr->sectnum) && anim[kCdudePostureW])				nSeq = anim[kCdudePostureW];
			else if ((pXSpr->data2 & kCdudeStatusForceCrouch) && anim[kCdudePostureC])	nSeq = anim[kCdudePostureC];
			else if (anim[kCdudePostureL])												nSeq = anim[kCdudePostureL];
		}
		
		int range[2] = {0, 1024};
		pValue = pIni->GetKeyString(group, paramScale, NULL);
		switch(cdParseRange(pValue, range, 256))
		{
			case 1:
			case 2:
				nScale = range[0];
				break;
		}
		
		pValue = pIni->GetKeyString("General", "Name", "Custom Dude");
		sprintf(gCustomDudeNames[pSpr->index], "%0.31s", pValue);
		pXSpr->sysData4 = 2;
		delete(pIni);
	}
	else
	{
		// cdude v1
		pXSpr->sysData4 = 1;
		if (pXSpr->data2 > 0)
			nSeq = pXSpr->data2;
	}
	
	if (!respawnMarker)
	{
		if (getSeqPrefs(nSeq, &pic, &xr, &yr, &pal))
		{
			if (pic >= 0) pSpr->picnum	= pic;
			if (xr >= 0)  pSpr->xrepeat	= (BYTE)xr;
			if (yr >= 0)  pSpr->yrepeat	= (BYTE)yr;
			if (pal >= 0) pSpr->pal		= (BYTE)pal;
		}
		
		if (xr > 0 && yr > 0)
			nnExtSprScaleSet(pSpr, nScale);
		
		pSpr->cstat &= ~kSprInvisible;
	}
	else if (getSeqPrefs(nSeq, &pic, &xr, &yr, &pal))
	{
		if (pic >= 0) pXSpr->sysData1 = pic;
		if (pal >= 0) pSpr->pal	= (BYTE)pal;
	}
}

// this function allows to spawn new custom dude and inherit spawner settings,
// so custom dude can have different weapons, hp and so on...
spritetype* cdSpawn(XSPRITE* pXSource, spritetype* pSprite, int nDist) {

    spritetype* pSource = &sprite[pXSource->reference];
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
	setsprite(pDude->index, x, y, z);
	
	if (!isUnderwaterSector(pDude->sectnum))
	{
		pDude->flags |= (kPhysGravity|kPhysMove|kPhysFalling);
	}
	else
	{
		pDude->flags &= ~(kPhysGravity|kPhysFalling);
		pDude->flags |= kPhysMove;
	}
	
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
	
	// not implemented list
	switch (pSource->type) {
        // change target of dudes and make it fight
        case kModernDudeTargetChanger:
			return;
	}
	
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
                    if (pXSpr->command != kCmdLink || pXSpr->locked) break; // no redirect mode detected
                    switch (pSpr->type) {
                        case kModernRandomTX:
                            useRandomTx(pXSpr, (COMMAND_ID)pXSource->command, false); // set random TX id
                            break;
                        case kModernSequentialTX:
                            if (pSpr->flags & kModernTypeFlag1) {
                                seqTxSendCmdAll(pXSpr, pSource->index, (COMMAND_ID)pXSource->command, true);
                                return;
                            }
                            useSequentialTx(pXSpr, (COMMAND_ID)pXSource->command, false); // set next TX id
                            break;
                    }
                    if (pXSpr->txID <= 0 || pXSpr->txID >= kChannelUserMax) return;
                    modernTypeSendCommand(pSource->index, pXSpr->txID, (COMMAND_ID)pXSource->command);
                    return;
            }
            break;
        default:
            return;
    }

    switch (pSource->type) {
        // allows teleport any sprite from any location to the source destination
        case kMarkerWarpDest:
            if (destObjType != EVOBJ_SPRITE) break;
            useTeleportTarget(pXSource, &sprite[destObjIndex]);
            break;
        // changes slope of sprite or sector
        case kModernSlopeChanger:
            switch (destObjType) {
                case EVOBJ_SPRITE:
                case EVOBJ_SECTOR:
                    useSlopeChanger(pXSource, destObjType, destObjIndex);
                    break;
            }
            break;
        case kModernSpriteDamager:
        // damages xsprite via TX ID or xsprites in a sector
            switch (destObjType) {
                case EVOBJ_SPRITE:
                case EVOBJ_SECTOR:
                    useSpriteDamager(pXSource, destObjType, destObjIndex);
                    break;
            }
            break;
        // can spawn any effect passed in data2 on it's or txID sprite
        case kModernEffectSpawner:
            if (destObjType != EVOBJ_SPRITE) break;
            useEffectGen(pXSource, &sprite[destObjIndex]);
            break;
        // takes data2 as SEQ ID and spawns it on it's or TX ID object
        case kModernSeqSpawner:
            useSeqSpawnerGen(pXSource, destObjType, destObjIndex);
            break;
        // creates wind on TX ID sector
        case kModernWindGenerator:
            if (destObjType != EVOBJ_SECTOR || pXSource->data2 < 0) break;
            useSectorWindGen(pXSource, &sector[destObjIndex]);
            break;
        // size and pan changer of sprite/wall/sector via TX ID
        case kModernObjSizeChanger:
            useObjResizer(pXSource, destObjType, destObjIndex);
            break;
        // iterate data filed value of destination object
        case kModernObjDataAccumulator:
            useIncDecGen(pXSource, destObjType, destObjIndex);
            break;
        // change data field value of destination object
        case kModernObjDataChanger:
            useDataChanger(pXSource, destObjType, destObjIndex);
            break;
        // change sector lighting dynamically
        case kModernSectorFXChanger:
            if (destObjType != EVOBJ_SECTOR) break;
            useSectorLigthChanger(pXSource, &xsector[sector[destObjIndex].extra]);
            break;
        // change picture and palette of TX ID object
        case kModernObjPicnumChanger:
            usePictureChanger(pXSource, destObjType, destObjIndex);
            break;
        // change various properties
        case kModernObjPropertiesChanger:
            usePropertiesChanger(pXSource, destObjType, destObjIndex);
            break;
        // change velocity of the sprite
        case kModernVelocityChanger:
            switch (destObjType) {
                case EVOBJ_SPRITE:
                case EVOBJ_SECTOR:
                    useVelocityChanger(pXSource, kCauserGame, destObjType, destObjIndex);
                    break;
            }
            break;
        // updated vanilla sound gen that now allows to play sounds on TX ID sprites
        case kGenModernSound:
            if (destObjType != EVOBJ_SPRITE) break;
            useSoundGen(pXSource, &sprite[destObjIndex]);
            break;
        // updated ecto skull gen that allows to fire missile from TX ID sprites
        case kGenModernMissileUniversal:
            if (destObjType != EVOBJ_SPRITE) break;
            useUniMissileGen(pXSource, &sprite[destObjIndex]);
            break;
        // spawn enemies on TX ID sprites
        case kMarkerDudeSpawn:
            if (destObjType != EVOBJ_SPRITE) break;
            useDudeSpawn(pXSource, &sprite[destObjIndex]);
            break;
         // spawn custom dude on TX ID sprites
        case kModernCustomDudeSpawn:
            if (destObjType != EVOBJ_SPRITE) break;
            useCustomDudeSpawn(pXSource, &sprite[destObjIndex]);
            break;
    }
}

void nnExtResetGlobals()
{
    int i;
	
	// reset lists
	gProxySpritesList.Init();		gImpactSpritesList.Init();
	gSightSpritesList.Init();		gPhysSpritesList.Init();
	
	// free all condition trackers
	conditionsTrackingClear();
	
	// clear sprite mass cache
	memset(gSpriteMass, 0, sizeof(gSpriteMass));
}

void nnExtInitModernStuff() {
	
	int i;
	nnExtResetGlobals();

	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum == kStatFree || (sprite[i].flags & 32) || sprite[i].extra <= 0)
			continue;
		
		spritetype* pSprite = &sprite[i];
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
				for (int nSprite = headspritestat[kStatDude], found = 0; nSprite >= 0; nSprite = nextspritestat[nSprite])
				{
					XSPRITE* pXSpr = &xsprite[sprite[nSprite].extra];
					if (pXSpr->rxID != pXSprite->txID) continue;
					else if (found) ThrowError("\nCustom dude (TX ID %d):\nOnly one incarnation allowed per channel!", pXSprite->txID);
					ChangeSpriteStat(nSprite, kStatInactive);
					nSprite = headspritestat[kStatDude];
					found++;
				}
				break;
			case kModernCondition:
			case kModernConditionFalse:
			{
				if (pXSprite->busyTime > 0)
				{
					if (pXSprite->waitTime > 0)
					{
						pXSprite->busyTime += ClipHigh(((pXSprite->waitTime * 120) / 10), 4095); pXSprite->waitTime = 0;
						buildprintf("Summing busyTime and waitTime for tracking condition #%d, RX ID %d. Result = %d ticks\n", pSprite->index, pXSprite->rxID, pXSprite->busyTime);
					}
					
					pXSprite->busy = pXSprite->busyTime;
				}
				
				if (pXSprite->waitTime && pXSprite->command >= kCmdNumberic)
				{
					Alert("Delay is not available when using numberic commands (%d - %d)", kCmdNumberic, 255);
					pXSprite->isTriggered = 1;
					continue;
				}
				pXSprite->decoupled = 0; // must go through operateSprite always
				pXSprite->targetX = pXSprite->targetY = pXSprite->targetZ = pXSprite->target	 = pXSprite->sysData2   = -1;
				pXSprite->triggerSight     = pXSprite->triggerImpact  = pXSprite->triggerTouch   = pXSprite->triggerOff = false;
				pXSprite->triggerProximity = pXSprite->triggerPush    = pXSprite->triggerVector  = pXSprite->triggerOn  = false;
				pXSprite->busy = pXSprite->state = pXSprite->restState = 0;
				
				ChangeSpriteStat(pSprite->index, kStatModernCondition);
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
		if (pXSprite->triggerProximity)
		{
			switch (pSprite->statnum) {
				case kStatFX:       case kStatExplosion:            case kStatItem:
				case kStatPurge:        case kStatSpares:           case kStatFlare:
				case kStatInactive:     case kStatFree:             case kStatMarker:
				case kStatPathMarker:   case kStatThing:            case kStatDude:
				case kStatModernPlayerLinker:
					break;
				default:
					gProxySpritesList.Add(pSprite->index);
					break;
			}
		}

		// make Sight flag work not just for dudes and things...
		if (pXSprite->triggerSight || pXSprite->unused3)
		{
			switch (pSprite->statnum) {
				case kStatFX:       case kStatExplosion:            case kStatItem:
				case kStatPurge:        case kStatSpares:           case kStatFlare:
				case kStatInactive:     case kStatFree:             case kStatMarker:
				case kStatPathMarker:   case kStatModernPlayerLinker:
					break;
				default:
					gSightSpritesList.Add(pSprite->index);
					break;
			}
		}
		
        // make Impact flag work for sprites that affected by explosions...
        if (pXSprite->triggerImpact)
		{
            switch (pSprite->statnum) {
                case kStatFX:           case kStatExplosion:            case kStatItem:
                case kStatPurge:        case kStatSpares:               case kStatFlare:
                case kStatInactive:     case kStatFree:                 case kStatMarker:
                case kStatModernPlayerLinker:
                    break;
                default:
                    gImpactSpritesList.Add(pSprite->index);
                    break;
            }
        }
	}
    
	// prepare conditions for use
	if (gStatCount[kStatModernCondition])
		conditionsInit();
}


void idListProcessProxySprite(int32_t nSpr)
{
	spritetype* pSpr = &sprite[nSpr];
	if (pSpr->extra <= 0)
		return;
	
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	
	// don't process locked or triggered objects
	if (pXSpr->locked || pXSpr->isTriggered) return;
	
	// unfortunately, newly created sprite may be not with required flag
	if (!pXSpr->triggerProximity) return;
	
	// just time out
	if (!pXSpr->interruptable && pXSpr->state != pXSpr->restState)
		return;
	
	short okDist = (pSpr->statnum == kStatDude) ? 96 : ClipLow(pSpr->clipdist * 3, 32);
	
	// check if camera is close enough!
	if (actCheckProximity(pSpr->x, pSpr->y, pSpr->z, pSpr->sectnum, okDist))
	{
		trTriggerSprite(pSpr->index, pXSpr, kCmdSpriteProximity);
		return;
	}
	
	if (!pXSpr->dudeLockout)
	{
		// check if enemies is close enough!
		for (int nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected])
		{
			if (!xsprIsFine(&sprite[nAffected]) || xsprite[sprite[nAffected].extra].health <= 0) continue;
			else if (CheckProximity(&sprite[nAffected], pSpr->x, pSpr->y, pSpr->z, pSpr->sectnum, okDist))
			{
				trTriggerSprite(pSpr->index, pXSpr, kCmdSpriteProximity);
				return;
			}
		}
	}
}

void idListProcessSightSprite(int32_t nSpr)
{
	spritetype* pSpr = &sprite[nSpr];
	if (pSpr->extra <= 0)
		return;
	
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	
	// don't process locked or triggered objects
	if (pXSpr->locked || pXSpr->isTriggered) return;
	
	// just time out
	if (!pXSpr->interruptable && pXSpr->state != pXSpr->restState)
		return;
	
	// sprite is on the screen
	if ((pXSpr->unused3 & kTriggerSpriteScreen) && TestBitString(show2dsprite, pSpr->index))
	{
		trTriggerSprite(pSpr->index, pXSpr, kCmdSpriteSight);
		ClearBitString(show2dsprite, pSpr->index);
		return;
	}
		
	// check if sprite sees the camera!
	if (cansee(pSpr->x, pSpr->y, pSpr->z, pSpr->sectnum, posx, posy, posz, cursectnum))
	{
		// unfortunately, newly created sprite may be not with required flag
		if (pXSpr->triggerSight)
			trTriggerSprite(pSpr->index, pXSpr, kCmdSpriteSight);
		
		if (pXSpr->unused3 & kTriggerSpriteAim)
		{
			int x, y, z;
			short hitsect, hitwall, hitsprite;
			bool vector = (pSpr->cstat & kSprHitscan);
			if (!vector)
				pSpr->cstat |= kSprHitscan;

			camHitscan(&hitsect, &hitwall, &hitsprite, &x, &y, &z, CLIPMASK0 | CLIPMASK1);
			
			if (!vector)
				pSpr->cstat &= ~kSprHitscan;

			if (hitsprite == pSpr->index)
				trTriggerSprite(pSpr->index, pXSpr, kCmdSpriteSight);
		}
	}
}

void idListProcessPhysSprite(int32_t nSpr)
{
	spritetype* pSpr = &sprite[nSpr];
	if (pSpr->extra <= 0 || pSpr->sectnum < 0)
		return;
	
	XSPRITE* pXSpr = &xsprite[pSpr->extra];
	
	// there is no main physics attributes
	if (!(pXSpr->physAttr & kPhysMove) && !(pXSpr->physAttr & kPhysGravity))
		return;
	
	int top, bot;
	int mass	= gSpriteMass[pSpr->extra].mass;
	int airVel	= gSpriteMass[pSpr->extra].airVel;
	bool uwater	= false;
	
	GetSpriteExtents(pSpr, &top, &bot);
	sectortype* pSect = &sector[pSpr->sectnum];
	
	if (pSect->extra)
	{
		XSECTOR* pXSect = &xsector[pSect->extra];
		if ((uwater = pXSect->underwater) != 0) airVel <<= 6;
		if (pXSect->panVel != 0 && getflorzofslope(pSpr->sectnum, pSpr->x, pSpr->y) <= bot)
		{
			int angle = pXSect->panAngle; int speed = 0;
			if (pXSect->panAlways || pXSect->state || pXSect->busy)
			{
				speed = pXSect->panVel << 9;
				if (!pXSect->panAlways && pXSect->busy)
					speed = mulscale16(speed, pXSect->busy);
			}
			
			if (sector[pSpr->sectnum].floorstat & 64)
				angle = (angle + GetWallAngle(sector[pSpr->sectnum].wallptr) + 512) & 2047;
			
			int dx = mulscale30(speed, Cos(angle));
			int dy = mulscale30(speed, Sin(angle));
			xvel[nSpr] += dx;
			yvel[nSpr] += dy;

		}
	}
	
	actAirDrag(pSpr, airVel);            
	if (pXSpr->physAttr & kPhysGravity) pXSpr->physAttr |= kPhysFalling;
	if ((pXSpr->physAttr & kPhysFalling) || xvel[nSpr] || yvel[nSpr] || zvel[nSpr] || velFloor[pSpr->sectnum] || velCeil[pSpr->sectnum])
		debrisMove(nSpr);

	if (xvel[nSpr] || yvel[nSpr])
		pXSpr->goalAng = getangle(xvel[nSpr], yvel[nSpr]) & 2047;

	int ang = pSpr->ang & 2047;
	if ((uwater = isUnderwaterSector(pSpr->sectnum)) == true && Chance(0x1000 - mass))
	{
		if (zvel[nSpr] > 0x100) debrisBubble(nSpr);
		if (ang == pXSpr->goalAng)
		{
		   pXSpr->goalAng = (pSpr->ang + BiRandom2(kAng60)) & 2047;
		   debrisBubble(nSpr);
		}
	}

	int angStep = ClipLow(mulscale8(1, ((abs(xvel[nSpr]) + abs(yvel[nSpr])) >> 5)), (uwater) ? 1 : 0);
	if (ang < pXSpr->goalAng)
		pSpr->ang = ClipHigh(ang + angStep, pXSpr->goalAng);
	else if (ang > pXSpr->goalAng)
		pSpr->ang = ClipLow(ang - angStep, pXSpr->goalAng);

	int nSector = pSpr->sectnum;
	int cz = getceilzofslope(nSector, pSpr->x, pSpr->y);
	int fz = getflorzofslope(nSector, pSpr->x, pSpr->y);
	
	GetSpriteExtents(pSpr, &top, &bot);
	if (fz >= bot && gLowerLink[nSector] < 0 && !(sector[nSector].ceilingstat & 0x1))
		pSpr->z += ClipLow(cz - top, 0);
	
	if (cz <= top && gUpperLink[nSector] < 0 && !(sector[nSector].floorstat & 0x1))
		pSpr->z += ClipHigh(fz - bot, 0);
}

void nnExtProcessSuperSprites()
{
	conditionsTrackingProcess();							// process tracking conditions
	gProxySpritesList.Process(idListProcessProxySprite);	// process additional proximity sprites
	gSightSpritesList.Process(idListProcessSightSprite);	// process sight sprites (for players only)
	gPhysSpritesList.Process(idListProcessPhysSprite);		// process debris physics affected sprites
}

void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector) {
    
    int i, nExtra = sector[pXSector->reference].extra;
    spritetype* pSource = &sprite[pXSource->reference];
    bool relative = (pSource->flags & kModernTypeFlag16);

    if (valueIsBetween(pXSource->data1, -1, 32767))
    {
        if (relative)
            pXSector->shadeWave = ClipHigh(pXSector->shadeWave + pXSource->data1, 11);
        else
            pXSector->shadeWave = ClipHigh(pXSource->data1, 11);
    }

    if (valueIsBetween(pXSource->data2, -128, 128))
    {
        if (relative)
            pXSector->amplitude = ClipRange(pXSector->amplitude + pXSource->data2, -127, 127);
        else
            pXSector->amplitude = pXSource->data2;
    }

    if (valueIsBetween(pXSource->data3, -1, 32767))
    {
        if (relative)
            pXSector->shadeFreq = ClipHigh(pXSector->shadeFreq + pXSource->data3, 255);
        else
            pXSector->shadeFreq = ClipHigh(pXSource->data3, 255);
    }

    if (valueIsBetween(pXSource->data4, -1, 65535))
    {
        if (relative)
            pXSector->shadePhase = ClipHigh(pXSector->shadePhase + pXSource->data4, 255);
        else
            pXSector->shadePhase = ClipHigh(pXSource->data4, 255);
    }

    if (pSource->flags)
    {
        if (pSource->flags != kModernTypeFlag1)
        {
            pXSector->shadeAlways   = (pSource->flags & kModernTypeFlag1) ? true : false;
            pXSector->shadeFloor    = (pSource->flags & kModernTypeFlag2) ? true : false;
            pXSector->shadeCeiling  = (pSource->flags & kModernTypeFlag4) ? true : false;
            pXSector->shadeWalls    = (pSource->flags & kModernTypeFlag8) ? true : false;
            pXSector->coloredLights = (pSource->pal) ? true : false;

            short cstat = pSource->cstat;
            if ((cstat & kSprRelMask) == kSprFloor)
            {
                // !!! xsector pal bits must be extended
                if (cstat & kSprOneSided)
                {
                    if (cstat & kSprFlipY)
                        pXSector->ceilpal2 = pSource->pal;
                    else
                        pXSector->floorpal2 = pSource->pal;
                }
                else
                {
                    pXSector->floorpal2 = pSource->pal;
                    pXSector->ceilpal2  = pSource->pal;
                }
            }
        }
        else
        {
            pXSector->shadeAlways   = true;
        }
    }

    // add to shadeList
    for (i = 0; i < shadeCount; i++)
    {
        if (shadeList[i] == nExtra)
            break;
    }

    if (i >= shadeCount && i < kMaxXSectors - 1)
        shadeList[shadeCount++] = nExtra;
    
}

void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource) {
    // let's first search for previously dropped items and remove it
    if (pXSource->dropItem > 0)
	{
        for (short nItem = headspritestat[kStatItem]; nItem >= 0; nItem = nextspritestat[nItem])
		{
            spritetype* pItem = &sprite[nItem];
            if ((unsigned int)pItem->type == pXSource->dropItem && pItem->x == pSource->x && pItem->y == pSource->y && pItem->z == pSource->z) {
                gFX.fxSpawn((FX_ID)29, pSource->sectnum, pSource->x, pSource->y, pSource->z);
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

		// check if generator affected by physics
        if (gPhysSpritesList.Exists(pSource->index) && (pDrop->extra >= 0 || dbInsertXSprite(pDrop->index) > 0))
		{
			xsprite[pDrop->extra].physAttr |= (kPhysMove | kPhysGravity | kPhysFalling); // must fall always
			pDrop->cstat &= ~kSprBlock;
			gPhysSpritesList.AddIfNot(pDrop->index);
			getSpriteMassBySize(pDrop); // create mass cache
		}
    }

}

void useSoundGen(XSPRITE* pXSource, spritetype* pSprite) {
    
	int pitch = pXSource->data4 << 1; if (pitch < 2000) pitch = 0;
    sfxPlay3DSoundCP(pSprite, pXSource->data2, -1, 0, pitch, pXSource->data3);
	
}

void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    //spritetype* pSource = &sprite[pXSource->reference];
    
    switch (objType) {
        case EVOBJ_SECTOR:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sector[objIndex].floorpicnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].ceilingpicnum = pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sector[objIndex].floorpal = pXSource->data3;

            if (valueIsBetween(pXSource->data4, -1, 65535))
                sector[objIndex].ceilingpal = pXSource->data4;
            break;
        case EVOBJ_SPRITE:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sprite[objIndex].picnum = pXSource->data1;

            if (pXSource->data2 >= 0) sprite[objIndex].shade = (pXSource->data2 > 127) ? 127 : pXSource->data2;
            else if (pXSource->data2 < -1) sprite[objIndex].shade = (pXSource->data2 < -127) ? -127 : pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sprite[objIndex].pal = pXSource->data3;
            break;
        case EVOBJ_WALL:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                wall[objIndex].picnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                wall[objIndex].overpicnum = pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                wall[objIndex].pal = pXSource->data3;
            break;
    }
}

void useSpriteDamager(XSPRITE* pXSource, int objType, int objIndex) {

    spritetype* pSource = &sprite[pXSource->reference];
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

void useDataChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    bool flag1 = (pSource->flags & kModernTypeFlag1);
    
    switch (objType) {
        case EVOBJ_SECTOR:
            if (flag1 || valueIsBetween(pXSource->data1, -1, 32767)) setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
        case EVOBJ_SPRITE:
            if (flag1 || valueIsBetween(pXSource->data1, -1, 32767)) setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            if (flag1 || valueIsBetween(pXSource->data2, -1, 32767)) setDataValueOfObject(objType, objIndex, 2, pXSource->data2);
            if (flag1 || valueIsBetween(pXSource->data3, -1, 32767)) setDataValueOfObject(objType, objIndex, 3, pXSource->data3);
            if (flag1 || valueIsBetween(pXSource->data4, -1, 65535)) setDataValueOfObject(objType, objIndex, 4, pXSource->data4);
            break;
        case EVOBJ_WALL:
            if (flag1 || valueIsBetween(pXSource->data1, -1, 32767)) setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
    }
}

void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex) {
    char buffer[5]; int data = -65535; short tmp = 0; int dataIndex = 0;
    Bsprintf(buffer, "%d", abs(pXSource->data1)); int len = Bstrlen(buffer);
    
    for (int i = 0; i < len; i++) {
        dataIndex = (buffer[i] - 52) + 4;
        if ((data = getDataFieldOfObject(objType, objIndex, dataIndex)) == -65535)
		{
            previewMessage("\nWrong index of data (%c) for IncDec Gen #%d! Only 1, 2, 3 and 4 indexes allowed!\n", buffer[i], objIndex);
            continue;
        }
        spritetype* pSource = &sprite[pXSource->reference];
        
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
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    tmp = pXSource->data3;
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
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    tmp = pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                    break;
                case kModernTypeFlag3:
                    if (data < pXSource->data3) data = pXSource->data2;
                    break;
                }
            }
        }

        pXSource->sysData1 = data;
        setDataValueOfObject(objType, objIndex, dataIndex, data);
    }

}

void useObjResizer(XSPRITE* pXSource, short objType, int objIndex) {
    switch (objType) {
        // for sectors
    case EVOBJ_SECTOR:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            sector[objIndex].floorxpanning = ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            sector[objIndex].floorypanning = ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sector[objIndex].ceilingxpanning = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sector[objIndex].ceilingypanning = ClipRange(pXSource->data4, 0, 255);
        break;
        // for sprites
    case EVOBJ_SPRITE: {

        // resize by seq scaling
        if (sprite[pXSource->reference].flags & kModernTypeFlag1)
		{
            if (valueIsBetween(pXSource->data1, -255, 32767))
			{
                int mulDiv = (valueIsBetween(pXSource->data2, 0, 257)) ? pXSource->data2 : 256;
                if (pXSource->data1 > 0) xsprite[sprite[objIndex].extra].scale = mulDiv * ClipHigh(pXSource->data1, 25);
                else if (pXSource->data1 < 0) xsprite[sprite[objIndex].extra].scale = mulDiv / ClipHigh(abs(pXSource->data1), 25);
                else xsprite[sprite[objIndex].extra].scale = 0;
            }

        // resize by repeats
        }
		else
		{
            if (valueIsBetween(pXSource->data1, -1, 32767))
			{
                sprite[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);
            }
            
            if (valueIsBetween(pXSource->data2, -1, 32767))
			{
                sprite[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);
            }
        }

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sprite[objIndex].xoffset = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sprite[objIndex].yoffset = ClipRange(pXSource->data4, 0, 255);
        break;
    }
    case EVOBJ_WALL:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            wall[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            wall[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            wall[objIndex].xpanning = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            wall[objIndex].ypanning = ClipRange(pXSource->data4, 0, 255);
        break;
    }

}

void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex) {

    spritetype* pSource = &sprite[pXSource->reference];
    bool flag1 = (pSource->flags & kModernTypeFlag1);
    bool flag2 = (pSource->flags & kModernTypeFlag2);
    bool flag4 = (pSource->flags & kModernTypeFlag4);

    short data3 = (short)pXSource->data3;
    short data4 = (short)pXSource->data4;
    int nSector;

    switch (objType) {
        case EVOBJ_WALL:
        {
            walltype* pWall = &wall[objIndex]; int old = -1;

            // data1 = set this wall as first wall of sector or as sector.alignto wall
            if (valueIsBetween(pXSource->data1, -1, 32767))
            {
                if ((nSector = sectorofwall(objIndex)) >= 0)
                {
                    switch (pXSource->data1) {
                        case 1:
                            setfirstwall(nSector, objIndex);
                            /// correct?
                            if (xwallRangeIsFine(wall[objIndex].extra)) break;
                            dbInsertXWall(objIndex);
                            break;
                        case 2:
                            sector[nSector].alignto = ClipLow(objIndex - sector[nSector].wallptr, 0);
                            break;
                    }
                }
            }
            
            // data3 = set wall hitag
            if (valueIsBetween(data3, -1, 32767))
            {
                // relative
                if (flag1)
                {
                    if (flag4) pWall->hitag &= ~data3;
                    else pWall->hitag |= data3;
                }
                else pWall->hitag = data3; // absolute
            }

            // data4 = set wall cstat
            if (valueIsBetween(pXSource->data4, -1, 65535))
            {
                old = pWall->cstat;

                // relative
                if (flag1)
                {
                    if (flag4) pWall->cstat &= ~data4;
                    else pWall->cstat |= data4;
                }
                // absolute
                else
                {
                    pWall->cstat = data4;
                    if (!flag2)
                    {
                        // check for exceptions
                        if ((old & 0x2) && !(pWall->cstat & 0x2)) pWall->cstat |= 0x2; // kWallBottomSwap
                        if ((old & 0x4) && !(pWall->cstat & 0x4)) pWall->cstat |= 0x4; // kWallBottomOrg, kWallOutsideOrg
                        if ((old & 0x20) && !(pWall->cstat & 0x20)) pWall->cstat |= 0x20; // kWallOneWay

                        if ((old & 0x4000) && !(pWall->cstat & 0x4000)) pWall->cstat |= 0x4000; // kWallMoveForward
                        else if ((old & 0x8000) && !(pWall->cstat & 0x8000)) pWall->cstat |= 0x8000; // kWallMoveBackward
                    }
                }
            }
        }
        break;
        case EVOBJ_SPRITE:
        {
            spritetype* pSprite = &sprite[objIndex]; bool thing2debris = false;
            XSPRITE* pXSprite = &xsprite[pSprite->extra]; int old = -1;

            // data3 = set sprite hitag
            if (valueIsBetween(data3, -1, 32767))
            {
                old = pSprite->flags;
                
                // relative
                if (flag1)
                {
                    if (flag4) pSprite->flags &= ~data3; 
                    else pSprite->flags |= data3;
                }
                // absolute
                else
                {
                    pSprite->flags = data3;
                }
                
                // handle exceptions (check always because they are system)
                if ((old & kHitagFree) && !(pSprite->flags & kHitagFree)) pSprite->flags |= kHitagFree;
                if ((old & kHitagRespawn) && !(pSprite->flags & kHitagRespawn)) pSprite->flags |= kHitagRespawn;

                // prepare things for different (debris) physics.
                thing2debris = (pSprite->statnum == kStatThing);
            }

            // data2 = sprite physics settings
            if (valueIsBetween(pXSource->data2, -1, 32767) || thing2debris) {
                switch (pSprite->statnum) {
                case kStatDude: // dudes already treating in game
                case kStatFree:
                case kStatMarker:
                case kStatPathMarker:
                    break;
                default:
                    // store physics attributes in xsprite to avoid setting hitag for modern types!
                    int flags = (pXSprite->physAttr != 0) ? pXSprite->physAttr : 0;
                    int oldFlags = flags;

                    if (thing2debris)
                    {
                        // converting thing to debris
                        if ((pSprite->flags & kPhysMove) != 0) flags |= kPhysMove;
                        else flags &= ~kPhysMove;

                        if ((pSprite->flags & kPhysGravity) != 0) flags |= (kPhysGravity | kPhysFalling);
                        else flags &= ~(kPhysGravity | kPhysFalling);

                        pSprite->flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
                        xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;
                        pXSprite->restState = pXSprite->state;
                    }
                    else
                    {
                        static char digits[6];
                        memset(digits, 0, sizeof(digits));
                        Bsprintf(digits, "%d", pXSource->data2);
                        for (unsigned int i = 0; i < sizeof(digits); i++)
                            digits[i] = (digits[i] >= 48 && digits[i] <= 57) ? (digits[i] - 57) + 9 : 0;

                        // first digit of data2: set main physics attributes
                        switch (digits[0]) {
                            case 0:
                                flags &= ~kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;
                            case 1:
                                flags |= kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;
                            case 2:
                                flags &= ~kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                            case 3:
                                flags |= kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                        }

                        // second digit of data2: touch physics flags
                        switch (digits[1]) {
                            case 0:
                                flags &= ~kPhysDebrisTouch;
                                break;
                            case 1:
                                flags |= kPhysDebrisTouch;
                                break;
                        }

                        // third digit of data2: weapon physics flags
                        switch (digits[2]) {
                            case 0:
                                flags &= ~kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;
                            case 1:
                                flags |= kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;
                            case 2:
                                flags &= ~kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                            case 3:
                                flags |= kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                        }

                        // fourth digit of data2: swimming / flying physics flags
                        switch (digits[3]) {
                            case 0:
                                flags &= ~kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 1:
                                flags |= kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 2:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFloat;
                                flags &= ~kPhysDebrisFly;
                                break;
                            case 3:
                                flags |= kPhysDebrisFly;
                                flags &= ~kPhysDebrisSwim;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 4:
                                flags |= kPhysDebrisFly;
                                flags |= kPhysDebrisFloat;
                                flags &= ~kPhysDebrisSwim;
                                break;
                            case 5:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFly;
                                flags &= ~kPhysDebrisFloat;
                                break;
                            case 6:
                                flags |= kPhysDebrisSwim;
                                flags |= kPhysDebrisFly;
                                flags |= kPhysDebrisFloat;
                                break;
                        }

                    }

					// check if there is no sprite in list
                    BOOL exists = gPhysSpritesList.Exists(objIndex);

                    // adding sprite in physics list
                    if ((flags & kPhysGravity) != 0 || (flags & kPhysMove) != 0)
                    {
                        if (oldFlags == 0)
                            xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;

                        pXSprite->physAttr = flags; // update physics attributes
						
						if (!exists)
						{
                            // allow things to became debris, so they use different physics...
                            if (pSprite->statnum == kStatThing)
								ChangeSpriteStat(objIndex, 0);

                            // set random goal ang for swimming so they start turning
                            if ((flags & kPhysDebrisSwim) && !xvel[objIndex] && !yvel[objIndex] && !zvel[objIndex])
                                pXSprite->goalAng = (pSprite->ang + BiRandom2(kAng45)) & 2047;
                            
                            if (pXSprite->physAttr & kPhysDebrisVector)
                                pSprite->cstat |= kSprHitscan;
							
							gPhysSpritesList.Add(objIndex);
                            getSpriteMassBySize(pSprite); // create physics cache
						}
                    }
                    // removing sprite from physics list
                    else if (exists)
                    {
                        pXSprite->physAttr = flags;
                        xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;
                        if (pSprite->lotag >= kThingBase && pSprite->lotag < kThingMax)
                            changespritestat(objIndex, kStatThing);  // if it was a thing - restore statnum
						
						gPhysSpritesList.Remove(objIndex);
                    }
                    break;
                }
            }

            // data4 = sprite cstat
            if (valueIsBetween(pXSource->data4, -1, 65535))
            {
                old = pSprite->cstat;
                
                // relative
                if (flag1)
                {
                    if (flag4) pSprite->cstat &= ~data4;
                    else pSprite->cstat |= data4;
                }
                // absolute
                else
                {
                    pSprite->cstat = data4;
                    if (!flag2)
                    {
                        // check exceptions
                        if ((old & 0x1000) && !(pSprite->cstat & 0x1000)) pSprite->cstat |= 0x1000; //kSpritePushable
                        if ((old & 0x80) && !(pSprite->cstat & 0x80)) pSprite->cstat |= 0x80; // kSpriteOriginAlign

                        if ((old & 0x2000) && !(pSprite->cstat & 0x2000)) pSprite->cstat |= 0x2000; // kSpriteMoveForward, kSpriteMoveFloor
                        else if ((old & 0x4000) && !(pSprite->cstat & 0x4000)) pSprite->cstat |= 0x4000; // kSpriteMoveReverse, kSpriteMoveCeiling
                    }
                }
            }
        }
        break;
        case EVOBJ_SECTOR:
        {
            sectortype* pSector = &sector[objIndex];
            XSECTOR* pXSector = &xsector[sector[objIndex].extra];

            // data1 = sector underwater status and depth level
            if (pXSource->data1 >= 0 && pXSource->data1 < 2)
            {
                pXSector->underwater = (pXSource->data1) ? true : false;

                spritetype* pLower = (gLowerLink[objIndex] >= 0) ? &sprite[gLowerLink[objIndex]] : NULL;
                XSPRITE* pXLower = NULL; spritetype* pUpper = NULL; XSPRITE* pXUpper = NULL;
                
                if (pLower) {
                    
                    pXLower = &xsprite[pLower->extra];

                    // must be sure we found exact same upper link
                    for (int i = 0; i < kMaxSectors; i++) {
                        if (gUpperLink[i] < 0 || xsprite[sprite[gUpperLink[i]].extra].data1 != pXLower->data1) continue;
                        pUpper = &sprite[gUpperLink[i]]; pXUpper = &xsprite[pUpper->extra];
                        break;
                    }
                }
                
                // treat sectors that have links, so warp can detect underwater status properly
                if (pLower) {
                    if (pXSector->underwater) {
                        switch (pLower->type) {
                            case kMarkerLowStack:
                            case kMarkerLowLink:
                                pXLower->sysData1 = pLower->type;
                                pLower->type = kMarkerLowWater;
                                break;
                            default:
                                if (pSector->ceilingpicnum < 4080 || pSector->ceilingpicnum > 4095) pXLower->sysData1 = kMarkerLowLink;
                                else pXLower->sysData1 = kMarkerLowStack;
                                break;
                        }
                    }
                    else if (pXLower->sysData1 > 0) pLower->type = pXLower->sysData1;
                    else if (pSector->ceilingpicnum < 4080 || pSector->ceilingpicnum > 4095) pLower->type = kMarkerLowLink;
                    else pLower->type = kMarkerLowStack;
                }

                if (pUpper) {
                    if (pXSector->underwater) {
                        switch (pUpper->type) {
                            case kMarkerUpStack:
                            case kMarkerUpLink:
                                pXUpper->sysData1 = pUpper->type;
                                pUpper->type = kMarkerUpWater;
                                break;
                            default:
                                if (pSector->floorpicnum < 4080 || pSector->floorpicnum > 4095) pXUpper->sysData1 = kMarkerUpLink;
                                else pXUpper->sysData1 = kMarkerUpStack;
                                break;
                        }
                    }
                    else if (pXUpper->sysData1 > 0) pUpper->type = pXUpper->sysData1;
                    else if (pSector->floorpicnum < 4080 || pSector->floorpicnum > 4095) pUpper->type = kMarkerUpLink;
                    else pUpper->type = kMarkerUpStack;
                }

                // search for dudes in this sector and change their underwater status
                for (int nSprite = headspritesect[objIndex]; nSprite >= 0; nSprite = nextspritesect[nSprite]) {

                    spritetype* pSpr = &sprite[nSprite];
                    if (pSpr->statnum != kStatDude || !IsDudeSprite(pSpr) || !xspriRangeIsFine(pSpr->extra))
                        continue;

                    //PLAYER* pPlayer = getPlayerById(pSpr->type);
                    if (pXSector->underwater) {
                        if (pLower)
                            xsprite[pSpr->extra].medium = (pLower->type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

/*                         if (pPlayer) {
                            int waterPal = kMediumWater;
                            if (pLower) {
                                if (pXLower->data2 > 0) waterPal = pXLower->data2;
                                else if (pLower->type == kMarkerUpGoo) waterPal = kMediumGoo;
                            }

                            pPlayer->nWaterPal = waterPal;
                            pPlayer->posture = kPostureSwim;
                            pPlayer->pXSprite->burnTime = 0;
                        } */

                    } else {

                        xsprite[pSpr->extra].medium = kMediumNormal;
 /*                        if (pPlayer) {
                            pPlayer->posture = (!pPlayer->input.buttonFlags.crouch) ? kPostureStand : kPostureCrouch;
                            pPlayer->nWaterPal = 0;
                        } */

                    }
                }
            }
            else if (pXSource->data1 > 9) pXSector->Depth = 7;
            else if (pXSource->data1 > 1) pXSector->Depth = pXSource->data1 - 2;


            // data2 = sector visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                pSector->visibility = ClipRange(pXSource->data2, 0 , 234);

            // data3 = sector ceil cstat
            if (valueIsBetween(pXSource->data3, -1, 32767))
            {
                // relative
                if (flag1)
                {
                    if (flag4) pSector->ceilingstat &= ~data3;
                    else pSector->ceilingstat |= data3;
                }
                // absolute
                else pSector->ceilingstat = data3;
            }

            // data4 = sector floor cstat
            if (valueIsBetween(pXSource->data4, -1, 65535))
            {
                // relative
                if (flag1)
                {
                    if (flag4) pSector->floorstat &= ~data4;
                    else pSector->floorstat |= data4;
                }
                // absolute
                else pSector->floorstat = data4;
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

void sprite2sectorSlope(spritetype* pSprite, sectortype* pSector, char rel, bool forcez) {
    
    int slope = 0, z = 0;
    switch (rel) {
        default:
            z = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
			if ((pSprite->cstat & kSprRelMask) == kSprFloor && pSprite->extra > 0 && xsprite[pSprite->extra].triggerTouch) z--;
            slope = pSector->floorheinum;
            break;
        case 1:
            z = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
            if ((pSprite->cstat & kSprRelMask) == kSprFloor && pSprite->extra > 0 && xsprite[pSprite->extra].triggerTouch) z++;
            slope = pSector->ceilingheinum;
            break;
    }

    spriteSetSlope(pSprite->index, slope);
    if (forcez) pSprite->z = z;
}

void useSlopeChanger(XSPRITE* pXSource, int objType, int objIndex) {

    int slope, oslope, i;
    spritetype* pSource = &sprite[pXSource->reference];
    bool flag2 = (pSource->flags & kModernTypeFlag2);

    if (pSource->flags & kModernTypeFlag1) slope = ClipRange(pXSource->data2, -32767, 32767);
    else slope = (32767 / kPercFull) * ClipRange(pXSource->data2, -kPercFull, kPercFull);

    if (objType == EVOBJ_SECTOR) {
        
        sectortype* pSect = &sector[objIndex];

        switch (pXSource->data1) {
        case 2:
        case 0:
            if (slope == 0) pSect->floorstat &= ~0x0002;
            else if (!(pSect->floorstat & 0x0002))
                pSect->floorstat |= 0x0002;

            // just set floor slopew
            if (flag2) {

                pSect->floorheinum = slope;

            } else {

                // force closest floor aligned sprites to inherit slope of the sector's floor
                for (i = headspritesect[objIndex], oslope = pSect->floorheinum; i != -1; i = nextspritesect[i])
				{
                    if ((sprite[i].cstat & kSprRelMask) != kSprSloped) continue;
                    else if (getflorzofslope(objIndex, sprite[i].x, sprite[i].y) - kSlopeDist <= sprite[i].z) {

                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 0, true);

                        // set new slope of floor
                        pSect->floorheinum = slope;

                        // force sloped sprites to be on floor slope z
                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 0, true);

                        // restore old slope for next sprite
                        pSect->floorheinum = oslope;

                    }
                }

                // finally set new slope of floor
                pSect->floorheinum = slope;

            }

            if (pXSource->data1 == 0) break;
            fallthrough__;
        case 1:
            if (slope == 0) pSect->ceilingstat &= ~0x0002;
            else if (!(pSect->ceilingstat & 0x0002))
                pSect->ceilingstat |= 0x0002;

            // just set ceiling slope
            if (flag2) {

                pSect->ceilingheinum = slope;

            } else {

                // force closest floor aligned sprites to inherit slope of the sector's ceiling
                for (i = headspritesect[objIndex], oslope = pSect->ceilingheinum; i != -1; i = nextspritesect[i]) {
                    if ((sprite[i].cstat & kSprRelMask) != kSprSloped) continue;
                    else if (getceilzofslope(objIndex, sprite[i].x, sprite[i].y) + kSlopeDist >= sprite[i].z) {

                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 1, true);

                        // set new slope of ceiling
                        pSect->ceilingheinum = slope;

                        // force sloped sprites to be on ceiling slope z
                        sprite2sectorSlope(&sprite[i], &sector[objIndex], 1, true);

                        // restore old slope for next sprite
                        pSect->ceilingheinum = oslope;

                    }
                }

                // finally set new slope of ceiling
                pSect->ceilingheinum = slope;

            }
            break;
        }

        // let's give a little impulse to the physics sprites...
        for (i = headspritesect[objIndex]; i != -1; i = nextspritesect[i]) {

            if (sprite[i].extra > 0 && xsprite[sprite[i].extra].physAttr > 0) {
                xsprite[sprite[i].extra].physAttr |= kPhysFalling;
                zvel[i]++;
                
            } else if ((sprite[i].statnum == kStatThing || sprite[i].statnum == kStatDude) && (sprite[i].flags & kPhysGravity)) {
                sprite[i].flags |= kPhysFalling;
                zvel[i]++;
            }

        }

    } else if (objType == EVOBJ_SPRITE) {
        
        spritetype* pSpr = &sprite[objIndex];
		if ((pSpr->cstat & kSprRelMask) != kSprFloor) pSpr->cstat |= kSprFloor;
        if ((pSpr->cstat & kSprRelMask) != kSprSloped)
            pSpr->cstat |= kSprSloped;

        switch (pXSource->data4) {
            case 1:
            case 2:
            case 3:
                if (!sectRangeIsFine(pSpr->sectnum)) break;
                switch (pXSource->data4) {
                    case 1: sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 0, flag2); break;
                    case 2: sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 1, flag2); break;
                    case 3:
                        if (getflorzofslope(pSpr->sectnum, pSpr->x, pSpr->y) - kSlopeDist <= pSpr->z) sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 0, flag2);
                        if (getceilzofslope(pSpr->sectnum, pSpr->x, pSpr->y) + kSlopeDist >= pSpr->z) sprite2sectorSlope(pSpr, &sector[pSpr->sectnum], 1, flag2);
                        break;
                }
                break;
            default:
                spriteSetSlope(objIndex, slope);
                break;
        }
    }
}

void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector) {

    spritetype* pSource = &sprite[pXSource->reference];
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

    int windVel = ClipRange(pXSource->data2, 0, 32767);
    if ((pXSource->data1 & 0x0001))
        windVel = nnExtRandom(0, windVel);
    
    // process vertical wind in nnExtProcessSuperSprites();
    if ((pSource->cstat & kSprRelMask) == kSprFloor)
	{
        pXSource->sysData2 = windVel << 1;
        return;
    }

    pXSector->windVel = windVel;
    if ((pSource->flags & kModernTypeFlag1))
        pXSector->panAlways = pXSector->windAlways = 1;

    int ang = pSource->ang;
    if (pXSource->data4 <= 0) {
        if ((pXSource->data1 & 0x0002)) {
            while (pSource->ang == ang)
                pSource->ang = nnExtRandom(-kAng360, kAng360) & 2047;
        }
    }
    else if (pSource->cstat & 0x2000) pSource->ang += pXSource->data4;
    else if (pSource->cstat & 0x4000) pSource->ang -= pXSource->data4;
    else if (pXSource->sysData1 == 0) {
        if ((ang += pXSource->data4) >= kAng180) pXSource->sysData1 = 1;
        pSource->ang = ClipHigh(ang, kAng180);
    } else {
        if ((ang -= pXSource->data4) <= -kAng180) pXSource->sysData1 = 0;
        pSource->ang = ClipLow(ang, -kAng180);
    }

    pXSector->windAng = pSource->ang;

    if (pXSource->data3 > 0 && pXSource->data3 < 4) {
        switch (pXSource->data3) {
        case 1:
            pXSector->panFloor = true;
            pXSector->panCeiling = false;
            break;
        case 2:
            pXSector->panFloor = false;
            pXSector->panCeiling = true;
            break;
        case 3:
            pXSector->panFloor = true;
            pXSector->panCeiling = true;
            break;
        }

        short oldPan = pXSector->panVel;
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
                panList[panCount++] = nXSector;
        }

    }
}

void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index) {

    if (pXSource->data2 > 0 && !gSysRes.Lookup(pXSource->data2, "SEQ")) {
        previewMessage("Missing sequence #%d", pXSource->data2);
        return;
    }

    spritetype* pSource = &sprite[pXSource->reference];
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

                if (pXSource->data4 > 0) {

                    int cx, cy, cz;
                    cx = (wall[index].x + wall[wall[index].point2].x) >> 1;
                    cy = (wall[index].y + wall[wall[index].point2].y) >> 1;
                    int nSector = sectorofwall(index);
                    int32_t ceilZ, floorZ;
                    getzsofslope(nSector, cx, cy, &ceilZ, &floorZ);
                    int32_t ceilZ2, floorZ2;
                    getzsofslope(wall[index].nextsector, cx, cy, &ceilZ2, &floorZ2);
                    ceilZ = ClipLow(ceilZ, ceilZ2);
                    floorZ = ClipHigh(floorZ, floorZ2);
                    cz = (ceilZ + floorZ) >> 1;

                    // !!!
					//sfxPlay3DSound(cx, cy, cz, pXSource->data4, nSector);

                }

            }
            return;
        case EVOBJ_SPRITE:
            
            if (pXSource->data2 <= 0) seqKill(3, sprite[index].extra);
            else if (sectRangeIsFine(sprite[index].sectnum))
            {
                if (pXSource->data3 > 0)
                {
                    int nSprite = InsertSprite(sprite[index].sectnum, kStatDecoration);
                    int top, bottom; GetSpriteExtents(&sprite[index], &top, &bottom);
                    sprite[nSprite].x = sprite[index].x;
                    sprite[nSprite].y = sprite[index].y;
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
                            sprite[nSprite].z = sprite[index].z + (tilesizy[sprite[index].picnum] / 2 + panm[sprite[index].picnum].ycenter);
                            break;
                        case 5:
                        case 6:
                            if (!sectRangeIsFine(sprite[index].sectnum)) sprite[nSprite].z = top;
                            else if (pXSource->data3 == 5) sprite[nSprite].z = getflorzofslope(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y);
                            else sprite[nSprite].z = getceilzofslope(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y);
                            break;
                    }
                        
                    if (nSprite >= 0)
                    {
                        int nXSprite = dbInsertXSprite(nSprite);
                        seqSpawn(pXSource->data2, 3, nXSprite, -1);
                        if (pSource->flags & kModernTypeFlag1)
                        {
                            sprite[nSprite].pal = pSource->pal;
                            sprite[nSprite].shade = pSource->shade;
                            sprite[nSprite].xrepeat = pSource->xrepeat;
                            sprite[nSprite].yrepeat = pSource->yrepeat;
                            sprite[nSprite].xoffset = pSource->xoffset;
                            sprite[nSprite].yoffset = pSource->yoffset;
                        }

                        if (pSource->flags & kModernTypeFlag2)
                        {
                            sprite[nSprite].cstat |= pSource->cstat;
                        }

                        if (pSource->flags & kModernTypeFlag4)
                        {
                            sprite[nSprite].ang = pSource->ang;
                        }

                        // should be: the more is seqs, the shorter is timer
                        evPost(nSprite, EVOBJ_SPRITE, 1000, kCallbackRemove);
                    }
                } else
                {
                    seqSpawn(pXSource->data2, 3, sprite[index].extra, -1);
                }
                    
                if (pXSource->data4 > 0)
                    sfxPlay3DSound(&sprite[index], pXSource->data4, -1, 0);
            }
            return;
    }
}

void useEffectGen(XSPRITE* pXSource, spritetype* pSprite) {
    
    int fxId = (pXSource->data3 <= 0) ? pXSource->data2 : pXSource->data2 + Random(pXSource->data3 + 1);
    spritetype* pSource = &sprite[pXSource->reference];
    if (pSprite == NULL)
        pSprite = pSource;


    if (!xspriRangeIsFine(pSprite->extra)) return;
/*     else if (fxId >= kEffectGenCallbackBase) {
        
        int length = sizeof(gEffectGenCallbacks) / sizeof(gEffectGenCallbacks[0]);
        if (fxId < kEffectGenCallbackBase + length) {
            
            fxId = gEffectGenCallbacks[fxId - kEffectGenCallbackBase];
            evKill(pSprite->index, EVOBJ_SPRITE, (CALLBACK_ID)fxId);
            evPost(pSprite->index, EVOBJ_SPRITE, 1, (CALLBACK_ID)fxId);

        }
        
    } */
	else if (valueIsBetween(fxId, 0, kFXMax)) {

        int pos, top, bottom; GetSpriteExtents(pSprite, &top, &bottom);
        spritetype* pEffect = NULL;

        // select where exactly effect should be spawned
        switch (pXSource->data4) {
            case 1:
                pos = bottom;
                break;
            case 2: // middle
                pos = pSprite->z + (tilesizy[pSprite->picnum] / 2 + panm[pSprite->picnum].ycenter);
                break;
            case 3:
            case 4:
                if (sectRangeIsFine(pSprite->sectnum))
                {
                    if (pXSource->data4 == 3)
                        pos = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                    else
                        pos = getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                    
                    break;
                }
                fallthrough__;
            default:
                pos = top;
                break;
        }

        if ((pEffect = gFX.fxSpawn((FX_ID)fxId, pSprite->sectnum, pSprite->x, pSprite->y, pos)) != NULL) {

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

            if (pSource->flags & kModernTypeFlag4)
            {
                pEffect->ang = pSource->ang;
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

void seqTxSendCmdAll(XSPRITE* pXSource, int nIndex, COMMAND_ID cmd, bool modernSend) {
    
    bool ranged = txIsRanged(pXSource);
    if (ranged) {
        for (pXSource->txID = pXSource->data1; pXSource->txID <= pXSource->data4; pXSource->txID++) {
            if (pXSource->txID <= 0 || pXSource->txID >= kChannelUserMax) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    } else {
        for (int i = 0; i <= 3; i++) {
            pXSource->txID = GetDataVal(&sprite[pXSource->reference], i);
            if (pXSource->txID <= 0 || pXSource->txID >= kChannelUserMax) continue;
            else if (!modernSend) evSend(nIndex, 3, pXSource->txID, cmd);
            else modernTypeSendCommand(nIndex, pXSource->txID, cmd);
        }
    }
    
    pXSource->txID = pXSource->sysData1 = 0;
    return;
}


void useRandomTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState) {
    
    spritetype* pSource = &sprite[pXSource->reference];
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

    pXSource->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
    if (setState)
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
        //evSend(pSource->index, EVOBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);
}

void useSequentialTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    bool range = txIsRanged(pXSource); int cnt = 3; int tx = 0;

    if (range) {
        
        // make sure sysData is correct as we store current index of TX ID here.
        if (pXSource->sysData1 < pXSource->data1) pXSource->sysData1 = pXSource->data1;
        else if (pXSource->sysData1 > pXSource->data4) pXSource->sysData1 = pXSource->data4;

    } else {
        
        // make sure sysData is correct as we store current index of data field here.
        if (pXSource->sysData1 > 3) pXSource->sysData1 = 0;
        else if (pXSource->sysData1 < 0) pXSource->sysData1 = 3;

    }

    switch (cmd) {
        case kCmdOff:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->sysData1-- < 0) pXSource->sysData1 = 3;
                    if ((tx = GetDataVal(pSource, pXSource->sysData1)) <= 0) continue;
                    else break;
                }
            } else {
                if (--pXSource->sysData1 < pXSource->data1) pXSource->sysData1 = pXSource->data4;
                tx = pXSource->sysData1;
            }
            break;
        default:
            if (!range) {
                while (cnt-- >= 0) { // skip empty data fields
                    if (pXSource->sysData1 > 3) pXSource->sysData1 = 0;
                    if ((tx = GetDataVal(pSource, pXSource->sysData1++)) <= 0) continue;
                    else break;
                }
            } else {
                tx = pXSource->sysData1;
                if (pXSource->sysData1 >= pXSource->data4) {
                    pXSource->sysData1 = pXSource->data1;
                    break;
                }
                pXSource->sysData1++;
            }
            break;
    }

    pXSource->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
    if (setState)
        SetSpriteState(pSource->index, pXSource, pXSource->state ^ 1);
        //evSend(pSource->index, EVOBJ_SPRITE, pXSource->txID, (COMMAND_ID)pXSource->command);

}

void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = (sector[pSource->sectnum].extra >= 0) ? &xsector[sector[pSource->sectnum].extra] : NULL;
	BYTE isDude = 0;
	
	if (pSprite == NULL)
	{
		isDude = 1;
		posx = pSource->x; posy = pSource->y; posz = pSource->z;
		if (cursectnum != pSource->sectnum)
			updatesector(posx, posy, &cursectnum);
		
		if (cursectnum >= 0)
		{
			int height = kensplayerheight;
			if (posz > sector[cursectnum].floorz - height) posz = sector[cursectnum].floorz - height;
			if (posz < sector[cursectnum].ceilingz) posz = sector[cursectnum].ceilingz;
		}
		
		if (pXSource->data2 == 1)
			ang = pSource->ang;
		
		if (pXSource->data3 == 1)
			vel = svel = 0;
	}
	else
	{
		isDude = (IsDudeSprite(pSprite)) ? 2 : 0;
		if (pSprite->sectnum != pSource->sectnum)
			ChangeSpriteSect(pSprite->index, pSource->sectnum);
		
		pSprite->x = pSource->x; pSprite->y = pSource->y;
		int zTop, zBot; GetSpriteExtents(pSource, &zTop, &zBot);
		pSprite->z = zBot;
		
		clampSprite(pSprite, 0x01);

		if (pSource->flags & kModernTypeFlag1) // force telefrag
			TeleFrag(pSprite->index, pSource->sectnum);

		if (pSprite->flags & kPhysGravity)
			pSprite->flags |= kPhysFalling;


		if (pXSource->data2 == 1)
			changeSpriteAngle(pSprite, pSource->ang);
		
		if (pXSource->data3 == 1)
		{
			// reset velocity
			xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
		}
		else if (pXSource->data3 > 0)
		{
			// change movement direction according source angle
			if (pXSource->data3 & kModernTypeFlag2)
			{
				int vAng = getVelocityAngle(pSprite);
				RotatePoint(&xvel[pSprite->index], &yvel[pSprite->index], (pSource->ang - vAng) & kAngMask, pSprite->x, pSprite->y);
			}

			if (pXSource->data3 & kModernTypeFlag4)
				zvel[pSprite->index] = 0;
		}
	}

    if (pXSector)
	{
        if (pXSector->triggerEnter && (isDude == 1 || (isDude == 2 && !pXSector->dudeLockout)))
            trTriggerSector(pSource->sectnum, pXSector, kCmdSectorEnter);
    }

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSource, pXSource->data4, -1, 0);
	
}

bool xsprIsFine(spritetype* pSpr)
{
    if (pSpr && xspriRangeIsFine(pSpr->extra) && !(pSpr->flags & kHitagFree))
    {
        if (!(pSpr->flags & kHitagRespawn) || (pSpr->statnum != kStatThing && pSpr->statnum != kStatDude))
            return true;
    }

    return false;
}

int getVelocityAngle(spritetype* pSpr)
{
    return getangle(xvel[pSpr->index] >> 12, yvel[pSpr->index] >> 12);
}

void changeSpriteAngle(spritetype* pSpr, int nAng)
{
    if (!IsDudeSprite(pSpr))
        pSpr->ang = nAng;
    else
    {
		pSpr->ang = nAng;
		if (xsprIsFine(pSpr))
			xsprite[pSpr->extra].goalAng = pSpr->ang;
    }
}

void useVelocityChanger(XSPRITE* pXSource, int causerID, short objType, int objIndex)
{
    #define kVelShift       8
    #define kScaleVal       0x10000
    
    int t, r = 0, nAng, vAng;
    int xv = 0, yv = 0, zv = 0;

    spritetype* pCauser = NULL;
    spritetype* pSource = &sprite[pXSource->reference];
    bool relative   = (pSource->flags & kModernTypeFlag1);
    bool toDstAng   = (pSource->flags & kModernTypeFlag2);
    bool toSrcAng   = (pSource->flags & kModernTypeFlag4);
    bool toRndAng   = (pSource->flags & kModernTypeFlag8);
    bool chgDstAng  = !(pSource->flags & kModernTypeFlag16);
	bool toEvnAng = false;
    //bool toEvnAng   = (toDstAng && toSrcAng && (pCauser = getCauser(causerID)) != NULL);
    bool toAng      = (toDstAng || toSrcAng || toEvnAng || toRndAng);
    bool toAng180   = (toRndAng && (toDstAng || toSrcAng || toEvnAng));

    if (objType == EVOBJ_SPRITE)
    {
        spritetype* pSpr = &sprite[objIndex];
        if ((r = mulscale14(pXSource->data4 << kVelShift, kScaleVal)) != 0)
            r = nnExtRandom(-r, r);

        if (valueIsBetween(pXSource->data3, -32767, 32767))
        {
            if ((zv = mulscale14(pXSource->data3 << kVelShift, kScaleVal)) != 0)
                zv += r;
        }

        if (!toAng)
        {
            if (valueIsBetween(pXSource->data1, -32767, 32767))
            {
                if ((xv = mulscale14(pXSource->data1 << kVelShift, kScaleVal)) != 0)
                    xv += r;
            }

            if (valueIsBetween(pXSource->data2, -32767, 32767))
            {
                if ((yv = mulscale14(pXSource->data2 << kVelShift, kScaleVal)) != 0)
                    yv += r;
            }
        }
        else
        {
            if (toEvnAng)       nAng = pCauser->ang;
            else if (toSrcAng)  nAng = pSource->ang;
            else                nAng = pSpr->ang;

            nAng = nAng & kAngMask;

            if (!toAng180 && toRndAng)
            {
                t = nAng;
                while (t == nAng)
                    nAng = nnExtRandom(0, kAng360);
            }

            if (chgDstAng)
                changeSpriteAngle(pSpr, nAng);

            if ((t = (pXSource->data1 << kVelShift)) != 0)
                t += r;

            xv = mulscale14(t, Cos(nAng) >> 16);
            yv = mulscale14(t, Sin(nAng) >> 16);
        }

        if (pXSource->physAttr)
        {
            t = 1;
            switch (pSpr->statnum) {
                case kStatThing:
                    break;
                case kStatFX:
                    t = 0;
                    fallthrough__;
                case kStatDude:
                case kStatProjectile:
                    if (pXSource->physAttr & kPhysMove)     pSpr->flags |= kPhysMove;     else pSpr->flags  &= ~kPhysMove;
                    if (pXSource->physAttr & kPhysGravity)  pSpr->flags |= kPhysGravity;  else pSpr->flags  &= ~kPhysGravity;
                    if (pXSource->physAttr & kPhysFalling)  pSpr->flags |= kPhysFalling;  else pSpr->flags  &= ~kPhysFalling;
                    break;
            }

            // debris physics for sprites that is allowed
			if (t)
            {
                if (pSpr->extra <= 0)
                    dbInsertXSprite(pSpr->index);

                if (pSpr->statnum == kStatThing)
                {
                    pSpr->flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
                    ChangeSpriteStat(pSpr->index, 0);
                }

                XSPRITE* pXSpr = &xsprite[pSpr->extra];
                pXSpr->physAttr = pXSource->physAttr;
                if (!gPhysSpritesList.Exists(pSpr->index))
				{
					getSpriteMassBySize(pSpr);
					gPhysSpritesList.Add(pSpr->index);
				}
            }
        }

        if (relative)
        {
            xvel[pSpr->index] += xv;
            yvel[pSpr->index] += yv;
            zvel[pSpr->index] += zv;
        }
        else
        {
            xvel[pSpr->index] = xv;
            yvel[pSpr->index] = yv;
            zvel[pSpr->index] = zv;
        }

        vAng = getVelocityAngle(pSpr);

        if (toAng)
        {
            if (toAng180)
                RotatePoint(&xvel[pSpr->index], &yvel[pSpr->index], kAng180, pSpr->x, pSpr->y);
            else
                RotatePoint(&xvel[pSpr->index], &yvel[pSpr->index], (nAng - vAng) & kAngMask, pSpr->x, pSpr->y);


            vAng = getVelocityAngle(pSpr);
        }

        if (chgDstAng)
            changeSpriteAngle(pSpr, vAng);

/*         if (pSpr->owner >= 0)
        {
            // hack to make player projectiles damage it's owner
            if (pSpr->statnum == kStatProjectile && (t = actOwnerIdToSpriteId(pSpr->owner)) >= 0 && IsPlayerSprite(&sprite[t]))
                actPropagateSpriteOwner(pSpr, pSource);
        }

        viewCorrectPrediction(); */

        //if (pXSource->rxID == 157)
        //viewSetSystemMessage("%d: %d  /  %d  /  %d, C: %d", pSpr->sectnum, xvel[pSpr->index], yvel[pSpr->index], zvel[pSpr->index], sprite[causerID].type);
    }
    else if (objType == EVOBJ_SECTOR)
    {
        for (t = headspritesect[objIndex]; t >= 0; t = nextspritesect[t])
        {
            useVelocityChanger(pXSource, causerID, EVOBJ_SPRITE, t);
        }
    }
}

void useUniMissileGen(XSPRITE* pXSource, spritetype* pSprite) {

    int dx = 0, dy = 0, dz = 0;
    spritetype* pSource = &sprite[pXSource->reference];
    if (pSprite == NULL)
        pSprite = pSource;

    if (pXSource->data1 < kMissileBase || pXSource->data1 >= kMissileMax)
        return;

    if (pSprite->cstat & 32) {
        if (pSprite->cstat & 8) dz = 0x4000;
        else dz = -0x4000;
    } else {
        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = pXSource->data3 << 6; // add slope controlling
        if (dz > 0x10000) dz = 0x10000;
        else if (dz < -0x10000) dz = -0x10000;
    }

    spritetype* pMissile = NULL;
    if ((pMissile = actFireMissile(pSprite, 0, 0, dx, dy, dz, pXSource->data1)) != NULL) {

        int from; // inherit some properties of the generator
        if ((from = (pSource->flags & kModernTypeFlag3)) > 0) {

            
            int canInherit = 0xF;
            if (xspriRangeIsFine(pMissile->extra) && seqGetStatus(EVOBJ_SPRITE, pMissile->extra) >= 0) {
                
                canInherit &= ~0x8;
               
                SEQINST* pInst = GetInstance(EVOBJ_SPRITE, pMissile->extra); Seq* pSeq = pInst->pSequence;
                for (int i = 0; i < pSeq->nFrames; i++) {
                    if ((canInherit & 0x4) && pSeq->frames[i].pal != 0) canInherit &= ~0x4;
                    if ((canInherit & 0x2) && pSeq->frames[i].xrepeat != 0) canInherit &= ~0x2;
                    if ((canInherit & 0x1) && pSeq->frames[i].yrepeat != 0) canInherit &= ~0x1;
                }


        }

            if (canInherit != 0) {
                
                if (canInherit & 0x2)
                    pMissile->xrepeat = (from == kModernTypeFlag1) ? pSource->xrepeat : pSprite->xrepeat;
                
                if (canInherit & 0x1)
                    pMissile->yrepeat = (from == kModernTypeFlag1) ? pSource->yrepeat : pSprite->yrepeat;

                if (canInherit & 0x4)
                    pMissile->pal = (from == kModernTypeFlag1) ? pSource->pal : pSprite->pal;
                
                if (canInherit & 0x8)
                    pMissile->shade = (from == kModernTypeFlag1) ? pSource->shade : pSprite->shade;

            }

        }

        // add velocity controlling
        if (pXSource->data2 > 0) {

            int velocity = pXSource->data2 << 12;
            xvel[pMissile->index] = mulscale14(velocity, dx);
            yvel[pMissile->index] = mulscale14(velocity, dy);
            zvel[pMissile->index] = mulscale14(velocity, dz);

        }

        // add bursting for missiles
        if (pMissile->type != kMissileFlareAlt && pXSource->data4 > 0)
            evPost(pMissile->index, 3, ClipHigh(pXSource->data4, 500), kCallbackMissileBurst);
    }
}




void nnExtTriggerObject(int objType, int objIndex, int command)
{
    switch (objType)
	{
        case EVOBJ_SECTOR:
            if (!xsectRangeIsFine(sector[objIndex].extra)) break;
            trTriggerSector(objIndex, &xsector[sector[objIndex].extra], command);
            break;
        case EVOBJ_WALL:
            if (!xwallRangeIsFine(wall[objIndex].extra)) break;
            trTriggerWall(objIndex, &xwall[wall[objIndex].extra], command);
            break;
        case EVOBJ_SPRITE:
            if (!xspriRangeIsFine(sprite[objIndex].extra)) break;
            trTriggerSprite(objIndex, &xsprite[sprite[objIndex].extra], command);
            break;
    }

    return;
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

int listTx(XSPRITE* pXRedir, int tx) {
    if (txIsRanged(pXRedir)) {
        if (tx == -1) tx = pXRedir->data1;
        else if (tx < pXRedir->data4) tx++;
        else tx = -1;
    } else {
        if (tx == -1) {
            for (int i = 0; i <= 3; i++) {
                if ((tx = GetDataVal(&sprite[pXRedir->reference], i)) <= 0) continue;
                else return tx;
            }
        } else {
            int saved = tx; bool savedFound = false;
            for (int i = 0; i <= 3; i++) {
                tx = GetDataVal(&sprite[pXRedir->reference], i);
                if (savedFound && tx > 0) return tx;
                else if (tx != saved) continue;
                else savedFound = true;
            }
        }
        
        tx = -1;
    }

    return tx;
}

XSPRITE* evrIsRedirector(int nSprite) {
    if (spriRangeIsFine(nSprite)) {
        switch (sprite[nSprite].type) {
        case kModernRandomTX:
        case kModernSequentialTX:
            if (xspriRangeIsFine(sprite[nSprite].extra) && xsprite[sprite[nSprite].extra].command == kCmdLink
                && !xsprite[sprite[nSprite].extra].locked) return &xsprite[sprite[nSprite].extra];
            break;
        }
    }

    return NULL;
}

XSPRITE* evrListRedirectors(int objType, int objXIndex, XSPRITE* pXRedir, int* tx) {
    if (!gEventRedirectsUsed) return NULL;
    else if (pXRedir && (*tx = listTx(pXRedir, *tx)) != -1)
        return pXRedir;

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

    int nIndex = (pXRedir) ? pXRedir->reference : -1; bool prevFound = false;
    for (int i = bucketHead[id]; i < bucketHead[id + 1]; i++) {
        if (rxBucket[i].type != EVOBJ_SPRITE) continue;
        XSPRITE* pXSpr = evrIsRedirector(rxBucket[i].index);
        if (!pXSpr) continue;
        else if (prevFound || nIndex == -1) { *tx = listTx(pXSpr, *tx); return pXSpr; }
        else if (nIndex != pXSpr->reference) continue;
        else prevFound = true;
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
            spritetype* pSpr 	= &sprite[objIndex];
			XSPRITE* pXSpr 		= &xsprite[sprite[objIndex].extra];
			
            // exceptions
            if (IsDudeSprite(pSpr) && pXSpr->health <= 0)
				return TRUE;
            switch (pSpr->type)
			{
                case 425:
                case 426:
                case 427:
                    return TRUE;
            }

            switch (dataIndex)
			{
                case 1:
                    pXSpr->data1 = value;
                    switch (pSpr->type)
					{
                        case 22:
                            if (value == pXSpr->data2) SetSpriteState(objIndex, pXSpr, 1);
                            else SetSpriteState(objIndex, pXSpr, 0);
                            break;
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
							cdGetSeq(pSpr);
                            break;
                    }
                    return TRUE;
                case 2:
                    pXSpr->data2 = value;
                    switch (pSpr->type)
					{
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
							cdGetSeq(pSpr);
                            break;
                    }
                    return TRUE;
                case 3:
                    pXSpr->data3 = value;
                    return TRUE;
                case 4:
                    pXSpr->data4 = value;
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

void seqSpawnerOffSameTx(XSPRITE* pXSource) {

    int i, j;
    XSPRITE* pXSpr; spritetype* pSpr;

    for (i = 0; i < numsectors; i++)
    {
        for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
        {
            pSpr = &sprite[j];
            if (pSpr->type != kModernSeqSpawner)
                continue;

            pXSpr = (xspriRangeIsFine(pSpr->extra)) ? &xsprite[pSpr->extra] : NULL;
            if (!pXSpr || pXSpr->reference == pXSource->reference || !pXSpr->state)
                continue;

            evKill(j, EVOBJ_SPRITE);
            pXSpr->state = 0;
        }
    }
}

// this function spawns random dude using dudeSpawn
spritetype* randomSpawnDude(XSPRITE* pXSource, spritetype* pSprite, int a3, int a4) {
    
    spritetype* pSprite2 = NULL; int selected = -1;
    spritetype* pSource = &sprite[pXSource->reference];
    
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if ((selected = randomGetDataValue(pXSource, kRandomizeDude)) > 0)
            pSprite2 = nnExtSpawnDude(pXSource, pSprite, selected, a3, 0);
    }

    return pSprite2;
}

spritetype* nnExtSpawnDude(XSPRITE* pXSource, spritetype* pSprite, short nType, int a3, int a4)
{

    spritetype* pDude = NULL;
    spritetype* pSource = &sprite[pXSource->reference];
    if (nType < kDudeBase || nType >= kDudeMax || (pDude = actSpawnSprite(pSprite, kStatDude)) == NULL)
        return NULL;
	
    XSPRITE* pXDude = &xsprite[pDude->extra];

    int angle = pSprite->ang;
    int x, y, z = a4 + pSprite->z;
    if (a3 < 0)
    {
        x = pSprite->x;
        y = pSprite->y;
    }
	else
    {
        x = pSprite->x + mulscale30r(Cos(angle), a3);
        y = pSprite->y + mulscale30r(Sin(angle), a3);
    }

	pDude->x = x;
	pDude->y = y;
	pDude->z = z;

	ChangeSpriteSect(pDude->index, pSource->sectnum);

    pDude->type 		= nType;
    pDude->ang 			= angle;
	pDude->yvel 		= kDeleteReally;
    pDude->cstat 		|= 0x1101;
    pXDude->respawn 	= 1;
    pXDude->health  	= 100 << 4;

	if (!isUnderwaterSector(pDude->sectnum))
	{
		pDude->flags |= (kPhysGravity|kPhysMove|kPhysFalling);
	}
	else
	{
		pDude->flags &= ~(kPhysGravity|kPhysFalling);
		pDude->flags |= kPhysMove;
	}
	
    // add a way to inherit some values of spawner by dude.
    if (pSource->flags & kModernTypeFlag1)
	{
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

        // inherit dude flags
        pXDude->dudeDeaf = pXSource->dudeDeaf;
        pXDude->dudeGuard = pXSource->dudeGuard;
        pXDude->dudeAmbush = pXSource->dudeAmbush;
        pXDude->dudeFlag4 = pXSource->dudeFlag4;
        pXDude->unused1 = pXSource->unused1;

    }
	
	adjSpriteByType(pDude);
	if (pDude->sectnum >= 0)
		clampSprite(pDude);
	
    return pDude;
}

// this function stops wind on all TX sectors affected by WindGen after it goes off state.
void windGenStopWindOnSectors(XSPRITE* pXSource) {
    spritetype* pSource = &sprite[pXSource->reference];
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
    int rx = -1; XSPRITE* pXRedir = NULL;
    while ((pXRedir = evrListRedirectors(EVOBJ_SPRITE, sprite[pXSource->reference].extra, pXRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            if (rxBucket[i].type != EVOBJ_SECTOR) continue;
            XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
            if ((pXSector->state == 1 && !pXSector->windAlways) || (pSource->flags & kModernTypeFlag2))
                pXSector->windVel = 0;
        }
    }
}

// this function checks if all TX objects have the same value
bool incDecGoalValueIsReached(XSPRITE* pXSprite) {
    
    if (pXSprite->data3 != pXSprite->sysData1) return false;
    char buffer[5]; Bsprintf(buffer, "%d", abs(pXSprite->data1)); int len = Bstrlen(buffer); int rx = -1;
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type == EVOBJ_SPRITE && evrIsRedirector(rxBucket[i].index)) continue;
        for (int a = 0; a < len; a++) {
            if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                return false;
        }
    }

    XSPRITE* pXRedir = NULL; // check redirected TX buckets
    while ((pXRedir = evrListRedirectors(EVOBJ_SPRITE, sprite[pXSprite->reference].extra, pXRedir, &rx)) != NULL) {
        for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
            for (int a = 0; a < len; a++) {
                if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, (buffer[a] - 52) + 4) != pXSprite->data3)
                    return false;
            }
        }
    }
    
    return true;
}

void useCustomDudeSpawn(XSPRITE* pXSource, spritetype* pSprite) {

    cdSpawn(pXSource, pSprite, pSprite->clipdist << 1);
        
}

void useDudeSpawn(XSPRITE* pXSource, spritetype* pSprite) {

    if (randomSpawnDude(pXSource, pSprite, pSprite->clipdist << 1, 0) == NULL)
        nnExtSpawnDude(pXSource, pSprite, pXSource->data1, pSprite->clipdist << 1, 0);
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

    }
	else if (event.cmd == kCmdToggle)
	{
        event.cmd = (pXSector->state) ? kCmdOn : kCmdOff;
    }

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
