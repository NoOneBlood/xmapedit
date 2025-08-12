/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of actor.cpp from Nblood adapted for Preview Mode
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
#include "seq.h"
#include "db.h"
#include "aadjust.h"
#include "preview.h"
#include "tile.h"

short gAffectedSectors[kMaxSectors], gAffectedXWalls[kMaxXWalls];
SPRITEHIT gSpriteHit[kMaxXSprites];
POSTPONE gPost[kMaxSprites];
int gPostCount = 0;

EXPLODE_INFO explodeInfo[] = {
    { 40, 10, 10, 75, 450, 0, 60, 80, 40 },
    { 80, 20, 10, 150, 900, 0, 60, 160, 60 },
    { 120, 40, 15, 225, 1350, 0, 60, 240, 80 },
    { 80, 5, 10, 120, 20, 10, 60, 0, 40 },
    { 120, 10, 10, 180, 40, 10, 60, 0, 80 },
    { 160, 15, 10, 240, 60, 10, 60, 0, 120 },
    { 40, 20, 10, 120, 0, 10, 30, 60, 40 },
    { 80, 20, 10, 150, 800, 5, 60, 160, 60 },
};

MissileType missileInfo[18] = {
    { 2138, 978670, 512, 40, 40, (char)-16, 16, },  // Cleaver
    { 2427, 3145728, 0, 32, 32, (char)-128, 32, },  // Regular flare
    { 3056, 2796202, 0, 32, 32, (char)-128, 32, },  // Tesla alt
    { 2427, 2446677, 0, 32, 32, (char)-128, 4, },   // Flare alt
    { 0, 1118481, 0, 24, 24, (char)-128, 16, },     // Spray flame
    { 0, 1118481, 0, 32, 32, (char)-128, 32, },     // Fireball
    { 2135, 2796202, 0, 32, 32, (char)-128, 16, },  // Tesla regular
    { 870, 699050, 0, 32, 32, (char)-24, 32, },     // EctoSkull
    { 0, 1118481, 0, 24, 24, (char)-128, 16, },     // Hellhound flame
    { 0, 838860, 0, 16, 16, (char)-16, 16, },       // Puke
    { 0, 838860, 0, 8, 8, (char)0, 16, },           // Reserved
    { 3056, 2097152, 0, 32, 32, (char)-128, 16, },  // Stone gargoyle projectile
    { 0, 2446677, 0, 30, 30, (char)-128, 24, },     // Napalm launcher
    { 0, 2446677, 0, 30, 30, (char)-128, 24, },     // Cerberus fireball
    { 0, 1398101, 0, 24, 24, (char)-128, 16, },     // Tchernobog fireball
    { 2446, 2796202, 0, 32, 32, (char)-128, 16, },  // Regular life leech
    { 3056, 2446677, 0, 16, 16, (char)-128, 16, },  // Dropped life leech (enough ammo)
    { 3056, 1747626, 0, 32, 32, (char)-128, 16, }   // Dropped life leech (no ammo)
};

THINGINFO thingInfo[]=
{
    {25, 250, 32, 11, 4096, 80, 384, 907, 0, 0, 0, 0, 256, 256, 128, 64, 0, 0, 128, },              //TNT Barrel
    {5, 5, 16, 3, 24576, 1600, 256, 3444, -16, 0, 32, 32, 256, 256, 256, 64, 0, 0, 512, },          //Armed Proxy Dynamite
    {5, 5, 16, 3, 24576, 1600, 256, 3457, -16, 0, 32, 32, 256, 256, 256, 64, 0, 0, 512, },          //Armed Remote Dynamite
    {1, 20, 32, 3, 32768, 80, 0, 739, 0, 0, 0, 0, 256, 0, 256, 128, 0, 0, 0, },                     //Vase1
    {1, 150, 32, 3, 32768, 80, 0, 642, 0, 0, 0, 0, 256, 256, 256, 128, 0, 0, 0, },                  //Vase2
    {10, 0, 0, 0, 0, 0, 0, 462, 0, 0, 0, 0, 0, 0, 0, 256, 0, 0, 0, },                               //Crate face
    {1, 0, 0, 0, 0, 0, 0, 266, 0, 0, 0, 0, 256, 0, 256, 256, 0, 0, 0, },                            //Glass window
    {1, 0, 0, 0, 0, 0, 0, 796, 0, 0, 0, 0, 256, 0, 256, 256, 0, 0, 512, },                          //Flourescent Light
    {50, 0, 0, 0, 0, 0, 0, 1127, 0, 0, 0, 0, 0, 0, 0, 256, 0, 0, 0, },                              //Wall Crack
    {8, 0, 0, 0, 0, 0, 0, 1142, 0, 0, 0, 0, 256, 0, 256, 128, 0, 0, 0, },                           //Wood Beam
    {4, 0, 0, 0, 0, 0, 0, 1069, 0, 0, 0, 0, 256, 256, 64, 256, 0, 0, 128, },                        //Spider's Web
    {40, 0, 0, 0, 0, 0, 0, 483, 0, 0, 0, 0, 64, 0, 128, 256, 0, 0, 0, },                            //Metal Grate
    {1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 256, 0, 256, 0, 0, 128, },                             //Flammable Tree
    {1000, 0, 0, 8, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 128, 256, 0, 0, 512, },                          //MachineGun Trap
    {0, 15, 8, 3, 32768, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },                              //Falling Rock
    {0, 8, 48, 3, 49152, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },                              //Kickable Pail
    {10, 2, 0, 0, 32768, 0, 0, -1, 0, 0, 0, 0, 256, 0, 256, 256, 0, 0, 128, },                      //Gib Object
    {20, 2, 0, 0, 32768, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 256, 0, 0, 128, },                          //Explode Object
    {5, 14, 16, 3, 24576, 1600, 256, 3422, -32, 0, 32, 32, 64, 256, 128, 64, 0, 0, 256, },          //Armedstick Of TNT
    {5, 14, 16, 3, 24576, 1600, 256, 3433, -32, 0, 32, 32, 64, 256, 128, 64, 0, 0, 256, },          //Armedbundle Of TNT
    {5, 14, 16, 3, 32768, 1600, 256, 3467, -128, 0, 32, 32, 64, 256, 128, 64, 0, 0, 256, },         //Armed aerosol
    {5, 6, 16, 3, 32768, 1600, 256, 1462, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, },                     //Bone (FleshGarg.)
    {8, 3, 16, 11, 32768, 1600, 256, -1, 0, 0, 0, 0, 256, 0, 256, 256, 0, 0, 0, },                  //Some alpha stuff
    {0, 1, 1, 2, 0, 0, 0, 1147, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, },                                //Water Drip
    {0, 1, 1, 2, 0, 0, 0, 1160, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, },                                 //Blood Drip
    {15, 4, 4, 3, 24576, 0, 257, -1, 0, 0, 0, 0, 128, 64, 256, 256, 0, 0, 256, },                   //Blood chucks1
    {30, 30, 8, 3, 8192, 0, 257, -1, 0, 0, 0, 0, 128, 64, 256, 256, 0, 0, 64, },                    //Blood chucks2
    {60, 5, 32, 3, 40960, 1280, 257, 3405, 0, 0, 40, 40, 128, 64, 256, 256, 0, 0, 64, },            //Axe Zombie Head
    {80, 30, 32, 3, 57344, 1600, 256, 3281, -128, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, },                //Napalm's Alt Fireexplosion
    {80, 30, 32, 3, 57344, 1600, 256, 2020, -128, 0, 32, 32, 256, 0, 256, 256, 0, 0, 0, },          //Fire Pod Explosion
    {80, 30, 32, 3, 57344, 1600, 256, 1860, -128, 0, 32, 32, 256, 0, 256, 256, 0, 0, 0, },          //Green Pod Explosion
    {150, 30, 48, 3, 32768, 1600, 257, 800, -128, 0, 48, 48, 64, 64, 112, 64, 0, 96, 96, },         //Life Leech
    {1, 30, 48, 3, 32768, 1600, 0, 2443, -128, 0, 16, 16, 0, 0, 0, 0, 0, 0, 0, },                   //Voodoo Head
    {5, 5, 16, 3, 24576, 1600, 256, 3444, -16, 7, 32, 32, 256, 256, 256, 64, 0, 0, 512, },          //433-kModernThingTNTProx
    {5, 6, 16, 3, 32768, 1600, 256, 1462, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, },                     //434-kModernThingThrowableRock
    {150, 30, 48, 3, 32768, 1600, 257, 800, -128, 0, 44, 44, 0, 1024, 512, 1024, 0, 64, 512, },     //435  - kModernThingEnemyLifeLeech
};

BOOL actCheckProximity(int x, int y, int z, int nSector, int dist)
{
    int dx = klabs(x - posx) >> 4;
    if (dx < dist)
    {
        int dy = klabs(y - posy) >> 4;
        if (dy < dist)
        {
            int dz = klabs(z - posz) >> 8;
            if ( dz < dist && approxDist(dx, dy) < dist )
            {
                int zTop = posz - kensplayerheight, zBot = posz;
                //GetSpriteExtents(pSprite, &zTop, &zBot);

                if (cansee(posx, posy, posz, cursectnum, x, y, z, nSector) )
                    return TRUE;

                if ( cansee(posx, posy, zTop, cursectnum,
                    x, y, z, nSector) )
                    return TRUE;

                if ( cansee(posx, posy, zBot, cursectnum,
                    x, y, z, nSector) )
                    return TRUE;
            }
        }
    }
    return FALSE;
}

int MoveThing(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    if (nXSprite <= 0)
        return 0;

    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pSprite->index;
    int v8 = 0;
    THINGINFO *pThingInfo = &thingInfo[pSprite->type-kThingBase];
    int nSector = pSprite->sectnum;
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (xvel[nSprite] || yvel[nSprite])
    {
        short bakCstat = pSprite->cstat;
        pSprite->cstat &= ~257;
        v8 = gSpriteHit[nXSprite].hit = ClipMove(&pSprite->x, &pSprite->y, &pSprite->z, &nSector, xvel[nSprite]>>12, yvel[nSprite]>>12, pSprite->clipdist<<2, (pSprite->z-top)/4, (bottom-pSprite->z)/4, CLIPMASK0);
        pSprite->cstat = bakCstat;

        if (pSprite->sectnum != nSector)
        {
            if (nSector > 0)
                ChangeSpriteSect(nSprite, nSector);
        }

        if ((gSpriteHit[nXSprite].hit&0xc000) == 0x8000)
        {
            int nHitWall = gSpriteHit[nXSprite].hit&0x3fff;
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], nHitWall, pThingInfo->elastic);
            switch (pSprite->type) {
                case kThingZombieHead:
                    sfxPlay3DSound(pSprite, 607, 0, 0);
                    actDamageSprite(-1, pSprite, kDamageFall, 80);
                    break;
                case kThingKickablePail:
                    sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }
        }
    }
    else
    {
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }

    if (zvel[nSprite])
        pSprite->z += zvel[nSprite]>>8;

    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0);
    GetSpriteExtents(pSprite, &top, &bottom);
    if ((pSprite->flags & 2) && bottom < floorZ)
    {
        pSprite->z += 455;
        zvel[nSprite] += 58254;
        if (pSprite->type == kThingZombieHead)
        {
            spritetype *pFX = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z);
            if (pFX)
            {
                int v34 = ((int)gFrameClock*3)&2047;
                int v30 = ((int)gFrameClock*5)&2047;
                int vbx = ((int)gFrameClock*11)&2047;
                int v2c = 0x44444;
                int v28 = 0;
                int v24 = 0;
                RotateVector(&v2c,&v28,vbx);
                RotateVector(&v2c,&v24,v30);
                RotateVector(&v28,&v24,v34);
                xvel[pFX->index] = xvel[pSprite->index]+v2c;
                yvel[pFX->index] = yvel[pSprite->index]+v28;
                zvel[pFX->index] = zvel[pSprite->index]+v24;
            }
        }
    }

    if (CheckLink(pSprite, pSprite->sectnum, 0))
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0);

    GetSpriteExtents(pSprite, &top, &bottom);
    if (bottom >= floorZ)
    {
        actTouchFloor(pSprite, pSprite->sectnum);
        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ-bottom;
        int v20 = zvel[nSprite]-velFloor[pSprite->sectnum];
        if (v20 > 0)
        {
            pSprite->flags |= 4;
            int vax = actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v20, pSprite->sectnum, pThingInfo->elastic);
            int nDamage = mulscale30(vax, vax)-pThingInfo->dmgResist;
            if (nDamage > 0)
                actDamageSprite(nSprite, pSprite, kDamageFall, nDamage);
            zvel[nSprite] = v20;
            if (velFloor[pSprite->sectnum] == 0 && klabs(zvel[nSprite]) < 0x10000)
            {
                zvel[nSprite] = 0;
                pSprite->flags &= ~4;
            }

            switch (pSprite->type) {
                case kThingNapalmBall:
                    if (zvel[nSprite] == 0 || Chance(0xA000)) actNapalmMove(pSprite, pXSprite);
                    break;
                case kThingZombieHead:
                    if (klabs(zvel[nSprite]) > 0x80000)
                    {
                        sfxPlay3DSound(pSprite, 607, 0, 0);
                        actDamageSprite(-1, pSprite, kDamageFall, 80);
                    }
                    break;
                case kThingKickablePail:
                    if (klabs(zvel[nSprite]) > 0x80000)
                        sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }

            v8 = 0x4000|nSector;
        }
        else if (zvel[nSprite] == 0)
            pSprite->flags &= ~4;
    }
    else
    {
        gSpriteHit[nXSprite].florhit = 0;
        if (pSprite->flags&2)
            pSprite->flags |= 4;
    }
    if (top <= ceilZ)
    {
        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ-top, 0);
        if (zvel[nSprite] < 0)
        {
            xvel[nSprite] = mulscale16(xvel[nSprite], 0xc000);
            yvel[nSprite] = mulscale16(yvel[nSprite], 0xc000);
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x4000);
            switch (pSprite->type) {
                case kThingZombieHead:
                    if (klabs(zvel[nSprite]) > 0x80000) {
                        sfxPlay3DSound(pSprite, 607, 0, 0);
                        actDamageSprite(-1, pSprite, kDamageFall, 80);
                    }
                    break;
                case kThingKickablePail:
                    if (klabs(zvel[nSprite]) > 0x80000)
                        sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }
        }
    }
    else
        gSpriteHit[nXSprite].ceilhit = 0;
    if (bottom >= floorZ)
    {
        int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
        int nVelClipped = ClipHigh(nVel, 0x11111);
        if ((floorHit & 0xc000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale2(4, pSprite->x - sprite[nHitSprite].x);
                yvel[nSprite] += mulscale2(4, pSprite->y - sprite[nHitSprite].y);
                v8 = gSpriteHit[nXSprite].hit;
            }
        }
        if (nVel > 0)
        {
            int t = divscale16(nVelClipped, nVel);
            xvel[nSprite] -= mulscale16(t, xvel[nSprite]);
            yvel[nSprite] -= mulscale16(t, yvel[nSprite]);
        }
    }
    if (xvel[nSprite] || yvel[nSprite])
        pSprite->ang = getangle(xvel[nSprite], yvel[nSprite]);
    return v8;
}

void MoveDude(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pSprite->index;

    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
    {
        previewMessage("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }

    BOOL lockout = FALSE;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int bz = (bottom-pSprite->z)/4;
    int tz = (pSprite->z-top)/4;
    // !!! int wd = pSprite->clipdist<<2;
    int wd = pSprite->clipdist;
    int nSector = pSprite->sectnum;
    int nAiStateType = -1;

    if (xvel[nSprite] || yvel[nSprite])
    {
        short bakCstat = pSprite->cstat;
        pSprite->cstat &= ~257;
        gSpriteHit[nXSprite].hit = ClipMove((int*)&pSprite->x, (int*)&pSprite->y, (int*)&pSprite->z, &nSector, xvel[nSprite]>>12, yvel[nSprite]>>12, wd, tz, bz, CLIPMASK0);
        if (nSector == -1)
        {
            nSector = pSprite->sectnum;
            if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
                actDamageSprite(pSprite->index, pSprite, kDamageFall, 1000<<4);
        }

        if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate)
        {
            short nSector2 = nSector;
            if (pushmove(&pSprite->x, &pSprite->y, &pSprite->z, &nSector2, wd, tz, bz, CLIPMASK0) == -1)
                actDamageSprite(nSprite, pSprite, kDamageFall, 1000 << 4);
            if (nSector2 != -1)
                nSector = nSector2;
        }
        pSprite->cstat = bakCstat;

        switch (gSpriteHit[nXSprite].hit&0xc000) {
        case 0xc000:
        {
            int nHitSprite = gSpriteHit[nXSprite].hit&0x3fff;
            spritetype *pHitSprite = &sprite[nHitSprite];
            XSPRITE *pHitXSprite = NULL;
            // Should be pHitSprite here
            if (pSprite->extra > 0)
                pHitXSprite = &xsprite[pHitSprite->extra];

            if (!gModernMap && pHitXSprite && pHitXSprite->triggerTouch && !pHitXSprite->state && !pHitXSprite->isTriggered)
                trTriggerSprite(nHitSprite, pHitXSprite, kCmdSpriteTouch);

            if (lockout && pHitXSprite && pHitXSprite->triggerPush && !pHitXSprite->key && !pHitXSprite->dudeLockout && !pHitXSprite->state && !pHitXSprite->busy )
                trTriggerSprite(nHitSprite, pHitXSprite, kCmdSpritePush);

            break;
        }
        case 0x8000:
        {
            int nHitWall = gSpriteHit[nXSprite].hit&0x3fff;
            walltype *pHitWall = &wall[nHitWall];
            XWALL *pHitXWall = NULL;
            if (pHitWall->extra > 0)
                pHitXWall = &xwall[pHitWall->extra];
            if (lockout && pHitXWall && pHitXWall->triggerPush && !pHitXWall->key && !pHitXWall->dudeLockout && !pHitXWall->state && !pHitXWall->busy)
                trTriggerWall(nHitWall, pHitXWall, kCmdWallPush);
            if (pHitWall->nextsector != -1)
            {
                sectortype *pHitSector = &sector[pHitWall->nextsector];
                XSECTOR *pHitXSector = NULL;
                if (pHitSector->extra > 0)
                    pHitXSector = &xsector[pHitSector->extra];

                if (lockout && pHitXSector && pHitXSector->triggerWallPush && !pHitXSector->key && !pHitXSector->dudeLockout && !pHitXSector->state && !pHitXSector->busy)
                    trTriggerSector(pHitWall->nextsector, pHitXSector, kCmdSectorPush);
            }
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], nHitWall, 0);
            break;
        }
        }
    }
    else
    {
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }

    if (pSprite->sectnum != nSector)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->triggerExit && !pXSector->dudeLockout)
            trTriggerSector(pSprite->sectnum, pXSector, kCmdSectorExit);
        ChangeSpriteSect(nSprite, nSector);

        nXSector = sector[nSector].extra;
        pXSector = (nXSector > 0) ? &xsector[nXSector] : NULL;
        if (pXSector && pXSector->triggerEnter && !pXSector->dudeLockout)
        {

            if (sector[nSector].type == kSectorTeleport)
                pXSector->data = nSprite;

            trTriggerSector(nSector, pXSector, kCmdSectorEnter);
        }

        nSector = pSprite->sectnum;
    }

    char bUnderwater = 0;
    char bDepth = 0;
    if (sector[nSector].extra > 0)
    {
        XSECTOR *pXSector = &xsector[sector[nSector].extra];
        if (pXSector->underwater) bUnderwater = 1;
        if (pXSector->Depth)      bDepth = 1;
    }
    int nUpperLink = gUpperLink[nSector];
    int nLowerLink = gLowerLink[nSector];
    if (nUpperLink >= 0 && (sprite[nUpperLink].type == kMarkerUpWater || sprite[nUpperLink].type == kMarkerUpGoo))
        bDepth = 1;
    if (nLowerLink >= 0 && (sprite[nLowerLink].type == kMarkerLowWater || sprite[nLowerLink].type == kMarkerLowGoo))
        bDepth = 1;
    if (zvel[nSprite])
        pSprite->z += zvel[nSprite]>>8;
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);
    GetSpriteExtents(pSprite, &top, &bottom);

    if (pSprite->flags & 2)
    {
        int vc = 58254;
        if (bDepth)
        {
            if (bUnderwater)
            {
                int cz = getceilzofslope(nSector, pSprite->x, pSprite->y);
                if (cz > top)
                    vc += ((bottom-cz)*-80099) / (bottom-top);
                else
                    vc = 0;
            }
            else
            {
                int fz = getflorzofslope(nSector, pSprite->x, pSprite->y);
                if (fz < bottom)
                    vc += ((bottom-fz)*-80099) / (bottom-top);
            }
        }
        else
        {
            if (bUnderwater)
                vc = 0;
            else if (bottom >= floorZ)
                vc =  0;
        }
        if (vc)
        {
            pSprite->z += ((vc*4)/2)>>8;
            zvel[nSprite] += vc;
        }
    }


    int nLink = CheckLink(pSprite, pSprite->sectnum, 0);
    if (nLink)
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);

    GetSpriteExtents(pSprite, &top, &bottom);
    if (floorZ <= bottom)
    {
        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ-bottom;
        int v30 = zvel[nSprite]-velFloor[pSprite->sectnum];
        if (v30 > 0)
        {
            int vax = actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v30, pSprite->sectnum, 0);
            int nDamage = mulscale30(vax, vax);
            nDamage -= 100<<4;
            if (nDamage > 0)
                actDamageSprite(nSprite, pSprite, kDamageFall, nDamage);
            zvel[nSprite] = v30;
            if (klabs(zvel[nSprite]) < 0x10000)
            {
                zvel[nSprite] = velFloor[pSprite->sectnum];

                pSprite->flags &= ~4;
            }
            else

                pSprite->flags |= 4;
            switch (tileGetSurfType(floorHit))
            {
            case kSurfWater:
                gFX.fxSpawn(FX_9, pSprite->sectnum, pSprite->x, pSprite->y, floorZ);
                break;
            case kSurfLava:
            {
                spritetype *pFX = gFX.fxSpawn(FX_10, pSprite->sectnum, pSprite->x, pSprite->y, floorZ);
                if (pFX)
                {
                    for (int i = 0; i < 7; i++)
                    {
                        spritetype *pFX2 = gFX.fxSpawn(FX_14, pFX->sectnum, pFX->x, pFX->y, pFX->z);
                        if (pFX2)
                        {
                            xvel[pFX2->index] = BiRandom(0x6aaaa);
                            yvel[pFX2->index] = BiRandom(0x6aaaa);
                            zvel[pFX2->index] = -Random(0xd5555);
                        }
                    }
                }
                break;
            }
            }
        }
        else if (zvel[nSprite] == 0)

            pSprite->flags &= ~4;
    }
    else
    {
        gSpriteHit[nXSprite].florhit = 0;

        if (pSprite->flags&2)
            pSprite->flags |= 4;
    }
    if (top <= ceilZ)
    {
        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ-top, 0);

        if (zvel[nSprite] <= 0 && (pSprite->flags&4))
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x2000);
    }
    else
        gSpriteHit[nXSprite].ceilhit = 0;
    GetSpriteExtents(pSprite,&top,&bottom);

    pXSprite->height = ClipLow(floorZ-bottom, 0)>>8;
    if (xvel[nSprite] || yvel[nSprite])
    {
        if ((floorHit & 0xc000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale2(4, pSprite->x - sprite[nHitSprite].x);
                yvel[nSprite] += mulscale2(4, pSprite->y - sprite[nHitSprite].y);
                return;
            }
        }
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0 && xsector[nXSector].underwater)
            return;
        if (pXSprite->height >= 0x100)
            return;

        int nDrag = 0x2a00;
        if (pXSprite->height > 0)
            nDrag -= scale(nDrag, pXSprite->height, 0x100);

        xvel[nSprite] -= mulscale16r(xvel[nSprite], nDrag);
        yvel[nSprite] -= mulscale16r(yvel[nSprite], nDrag);

        if (approxDist(xvel[nSprite], yvel[nSprite]) < 0x1000)
            xvel[nSprite] = yvel[nSprite] = 0;
    }
}

void actHitcodeToData(int a1, HITINFO *pHitInfo, int *a3, spritetype **a4, XSPRITE **a5, int *a6, walltype **a7, XWALL **a8, int *a9, sectortype **a10, XSECTOR **a11)
{
    int nSprite = -1;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite = NULL;
    int nWall = -1;
    walltype *pWall = NULL;
    XWALL *pXWall = NULL;
    int nSector = -1;
    sectortype *pSector = NULL;
    XSECTOR *pXSector = NULL;
    switch (a1)
    {
    case 3:
    case 5:
        nSprite = pHitInfo->hitsprite;
        pSprite = &sprite[nSprite];
        if (pSprite->extra > 0)
            pXSprite = &xsprite[pSprite->extra];
        break;
    case 0:
    case 4:
        nWall = pHitInfo->hitwall;
        pWall = &wall[nWall];
        if (pWall->extra > 0)
            pXWall = &xwall[pWall->extra];
        break;
    case 1:
    case 2:
    case 6:
        nSector = pHitInfo->hitsect;
        pSector = &sector[nSector];
        if (pSector->extra > 0)
            pXSector = &xsector[pSector->extra];
        break;
    }
    if (a3)
        *a3 = nSprite;
    if (a4)
        *a4 = pSprite;
    if (a5)
        *a5 = pXSprite;
    if (a6)
        *a6 = nWall;
    if (a7)
        *a7 = pWall;
    if (a8)
        *a8 = pXWall;
    if (a9)
        *a9 = nSector;
    if (a10)
        *a10 = pSector;
    if (a11)
        *a11 = pXSector;
}

void actImpactMissile(spritetype *pMissile, int hitCode)
{

    int nXMissile = pMissile->extra;
    XSPRITE *pXMissile = &xsprite[pMissile->extra];

    int nSpriteHit = -1; int nWallHit = -1; int nSectorHit = -1;
    spritetype *pSpriteHit = NULL; XSPRITE *pXSpriteHit = NULL;
    walltype *pWallHit = NULL; XWALL *pXWallHit = NULL;
    sectortype *pSectorHit = NULL; XSECTOR *pXSectorHit = NULL;

    actHitcodeToData(hitCode, &gHitInfo, &nSpriteHit, &pSpriteHit, &pXSpriteHit, &nWallHit, &pWallHit, &pXWallHit, &nSectorHit, &pSectorHit, &pXSectorHit);
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileTeslaAlt:
            //sub_51340(pMissile, hitCode);
            switch (hitCode) {
                case 0:
                case 4:
                    if (pWallHit) {
                        spritetype* pFX = gFX.fxSpawn(FX_52, pMissile->sectnum, pMissile->x, pMissile->y, pMissile->z, 0);
                        if (pFX) pFX->ang = (short) ((GetWallAngle(nWallHit) + 512) & 2047);
                    }
                    break;
            }
            GibSprite(pMissile, GIBTYPE_24, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissilePukeGreen:
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileArcGargoyle:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 306, 0, 0);
            GibSprite(pMissile, GIBTYPE_6, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileLifeLeechAltNormal:
        case kMissileLifeLeechAltSmall:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 306, 0, 0);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFireball:
        case kMissileFireballNapalm:
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareAlt:
            sfxKill3DSound(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareRegular:
            sfxKill3DSound(pMissile);
            GibSprite(pMissile, GIBTYPE_17, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFlameSpray:
        case kMissileFlameHound:
            break;
        case kMissileFireballCerberus:
            actExplodeSprite(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileFireballTchernobog:
            actExplodeSprite(pMissile);
            actExplodeSprite(pMissile);
            break;
        case kMissileEctoSkull:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 522, 0, 0);
            actPostSprite(pMissile->index, kStatDebris);
            seqSpawn(20, 3, pMissile->extra, -1);
            break;
        case kMissileButcherKnife:
            actPostSprite(pMissile->index, kStatDebris);
            pMissile->cstat &= ~16;
            pMissile->type = 0;
            seqSpawn(20, 3, pMissile->extra, -1);
            break;
        case kMissileTeslaRegular:
            sfxKill3DSound(pMissile);
            sfxPlay3DSound(pMissile->x, 518, 0, 0);
            GibSprite(pMissile, (hitCode == 2) ? GIBTYPE_23 : GIBTYPE_22, NULL, NULL);
            evKill(pMissile->index, 3);
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
        default:
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            break;
    }

    //if (gModernMap && pXSpriteHit && pXSpriteHit->state != pXSpriteHit->restState && pXSpriteHit->Impact)
        //trTriggerSprite(nSpriteHit, pXSpriteHit, kCmdSpriteImpact);
    pMissile->cstat &= ~257;
}

void actBuildMissile(spritetype* pMissile, int nXSprite, int nSprite) {
    int nMissile = pMissile->index;
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            //evPost(nMissile, 3, 0, kCallbackFXFlameLick);
            break;
        case kMissileTeslaAlt:
            //evPost(nMissile, 3, 0, kCallbackFXTeslaAlt);
            //pMissile->cstat |= kSprFloor;
            break;
        case kMissilePukeGreen:
            seqSpawn(29, 3, nXSprite, -1);
            break;
        case kMissileButcherKnife:
            pMissile->cstat |= 16;
            break;
        case kMissileTeslaRegular:
            sfxPlay3DSound(pMissile, 251, 0, 0);
            break;
        case kMissileEctoSkull:
            seqSpawn(2, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 493, 0, 0);
            break;
        case kMissileFireballNapalm:
            seqSpawn(61, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireball:
            seqSpawn(22, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFlameHound:
            seqSpawn(27, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] / 2 + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + BiRandom(0x11111);
            break;
        case kMissileFireballCerberus:
            seqSpawn(61, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireballTchernobog:
            seqSpawn(23, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] / 2 + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + BiRandom(0x11111);
            break;
        case kMissileFlameSpray:
            if (Chance(0x8000))
                seqSpawn(0, 3, nXSprite, -1);
            else
                seqSpawn(1, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] + BiRandom(0x11111);
            yvel[nMissile] += yvel[nSprite] + BiRandom(0x11111);
            zvel[nMissile] += zvel[nSprite] + BiRandom(0x11111);
            break;
        case kMissileFlareAlt:
            evPost(nMissile, 3, 30, kCallbackFXFlareBurst);
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileFlareRegular:
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileLifeLeechAltSmall:
            break;
        case kMissileArcGargoyle:
            sfxPlay3DSound(pMissile, 252, 0, 0);
            break;
    }
}

spritetype* actFireMissile(spritetype *pSprite, int a2, int a3, int a4, int a5, int a6, int nType)
{
    char v4 = 0;
    int nSprite = pSprite->index;
    MissileType *pMissileInfo = &missileInfo[nType-kMissileBase];
    int x = pSprite->x+mulscale30(a2, Cos(pSprite->ang+512));
    int y = pSprite->y+mulscale30(a2, Sin(pSprite->ang+512));
    int z = pSprite->z+a3;
    int clipdist = pMissileInfo->clipDist+pSprite->clipdist;
    x += mulscale28(clipdist, Cos(pSprite->ang));
    y += mulscale28(clipdist, Sin(pSprite->ang));
    int hit = HitScan(pSprite, z, x-pSprite->x, y-pSprite->y, 0, BLOCK_MOVE, clipdist);
    if (hit != -1)
    {
        if (hit == 3 || hit == 0)
        {
            v4 = 1;
            x = gHitInfo.hitx-mulscale30(Cos(pSprite->ang), 16);
            y = gHitInfo.hity-mulscale30(Sin(pSprite->ang), 16);
        }
        else
        {
            x = gHitInfo.hitx-mulscale28(pMissileInfo->clipDist<<1, Cos(pSprite->ang));
            y = gHitInfo.hity-mulscale28(pMissileInfo->clipDist<<1, Sin(pSprite->ang));
        }
    }
    //return NULL;
    spritetype *pMissile = actSpawnSprite(pSprite->sectnum, x, y, z, 5, 1);

    if (!pMissile)
        return NULL;

    int nMissile = pMissile->index;
    SetBitString(show2dsprite, nMissile);
    pMissile->type = (short) nType;
    pMissile->shade = pMissileInfo->shade;
    pMissile->pal = 0;
    pMissile->clipdist = pMissileInfo->clipDist;
    pMissile->flags = 1;
    pMissile->xrepeat = pMissileInfo->xrepeat;
    pMissile->yrepeat = pMissileInfo->yrepeat;
    pMissile->picnum = pMissileInfo->picnum;
    pMissile->ang = (short) ((pSprite->ang+pMissileInfo->angleOfs)&2047);

    pMissile->yvel = kDeleteReally;

    xvel[nMissile] = mulscale(pMissileInfo->velocity, a4, 14);
    yvel[nMissile] = mulscale(pMissileInfo->velocity, a5, 14);
    zvel[nMissile] = mulscale(pMissileInfo->velocity, a6, 14);
    //actPropagateSpriteOwner(pMissile, pSprite);
    pMissile->cstat |= 1;
    int nXSprite = pMissile->extra;
    xsprite[nXSprite].target = -1;
    evPost(nMissile, 3, 600, kCallbackRemoveSpecial);

    actBuildMissile(pMissile, nXSprite, nSprite);

    if (v4)
    {
        actImpactMissile(pMissile, hit);
        pMissile = NULL;
    }
    return pMissile;
}

int MoveMissile(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int vdi = -1;

    gHitInfo.hitsect = -1;
    gHitInfo.hitwall = -1;
    gHitInfo.hitsprite = -1;
    if (pSprite->type == kMissileFlameSpray)
        actAirDrag(pSprite, 0x1000);

    int nSprite = pSprite->index;
    int vx = xvel[nSprite]>>12;
    int vy = yvel[nSprite]>>12;
    int vz = zvel[nSprite]>>8;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);


    int i = 1;
    while ( 1 ) {

        int nSector2 = pSprite->sectnum;
        int x = pSprite->x, y = pSprite->y, z = pSprite->z;
        clipmoveboxtracenum = 1;
        int vdx = ClipMove(&x, &y, &z, &nSector2, vx, vy, pSprite->clipdist<<2, (z-top)/4, (bottom-z)/4, BLOCK_MOVE);
        clipmoveboxtracenum = 3;
        int ceilZ, floorZ, ceilHit, floorHit;
        short nSector = (short)nSector2;

        if (nSector2 < 0)
        {
            vdi = -1;
            break;
        }

        if (vdx)
        {
            int nHitSprite = vdx & 0x3fff;
            if ((vdx&0xc000) == 0xc000)
            {
                gHitInfo.hitsprite = nHitSprite;
                vdi = 3;
            }
            else if ((vdx & 0xc000) == 0x8000)
            {
                gHitInfo.hitwall = nHitSprite;
                if (wall[nHitSprite].nextsector == -1)
                    vdi = 0;
                else
                {
                    int fz, cz;
                    getzsofslope(wall[nHitSprite].nextsector, x, y, &cz, &fz);
                    if (z <= cz || z >= fz)
                        vdi = 0;
                    else
                        vdi = 4;
                }
            }
        }

        if (vdi == 0 || vdi == 4)
        {
            int t = gHitInfo.hitwall;
            if (CheckLink(pSprite, t, 1))
            {
                nSector = pSprite->sectnum;
                x = pSprite->x;
                y = pSprite->y;
                z = pSprite->z;
                vdi = -1;
            }
        }

        if (vdi == 4)
        {
            walltype *pWall = &wall[gHitInfo.hitwall];
            if (pWall->extra > 0 && xwall[pWall->extra].triggerVector) {
                XWALL *pXWall = &xwall[pWall->extra];
                trTriggerWall(gHitInfo.hitwall, &xwall[pWall->extra], kCmdWallImpact);
                if (!(pWall->cstat&64)) {
                    vdi = -1;
                    if (i-- > 0) continue;
                    vdi = 0;
                    break;
                }

            }
        }
        if (vdi >= 0 && vdi != 3)
        {
            int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
            x -= mulscale30(Cos(nAngle), 16);
            y -= mulscale30(Sin(nAngle), 16);
            int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
            vz -= scale(0x100, zvel[nSprite], nVel);
            updatesector(x, y, &nSector);
            nSector2 = nSector;
        }

        GetZRangeAtXYZ(x, y, z, nSector2, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, BLOCK_MOVE);
        GetSpriteExtents(pSprite, &top, &bottom);
        top += vz;
        bottom += vz;
        if (bottom >= floorZ)
        {
            vz += floorZ-bottom;
            vdi = 2;
        }
        if (top <= ceilZ)
        {
            vz += ClipLow(ceilZ-top, 0);
            vdi = 1;
        }

        if (vdi < 0)
        {
            pSprite->x = x;
            pSprite->y = y;
            pSprite->z = z+vz;
            updatesector(x, y, &nSector);

            if (nSector >= 0 && nSector != pSprite->sectnum)
                ChangeSpriteSect(nSprite, nSector);

            CheckLink(pSprite, pSprite->sectnum, 0);
        }


        gHitInfo.hitsect = pSprite->sectnum;
        gHitInfo.hitx = pSprite->x;
        gHitInfo.hity = pSprite->y;
        gHitInfo.hitz = pSprite->z;
        break;
    }

    return vdi;
}

void actProcessSprites() {

    nnExtProcessSuperSprites();
    short nSprite, nDude, nNextDude;
    for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pDude = &sprite[nSprite];
        if ((pDude->flags & 32) || pDude->extra <= 0)
            continue;

        XSECTOR *pXSector = NULL;
        int nSector = pDude->sectnum;
        if (sector[nSector].extra > 0)
        {
            pXSector = &xsector[sector[nSector].extra];

            int top, bottom;
            GetSpriteExtents(pDude, &top, &bottom);
            if (getflorzofslope(nSector, pDude->x, pDude->y) <= bottom)
            {
                int angle = pXSector->panAngle;
                int speed = 0;
                if (pXSector->panAlways || pXSector->state || pXSector->busy)
                {
                    speed = pXSector->panVel << 9;
                    if (!pXSector->panAlways && pXSector->busy)
                        speed = mulscale16(speed, pXSector->busy);
                }
                if (sector[nSector].floorstat&64)
                    angle = (angle+GetWallAngle(sector[nSector].wallptr)+512)&2047;
                int dx = mulscale30(speed, Cos(angle));
                int dy = mulscale30(speed, Sin(angle));
                xvel[nSprite] += dx;
                yvel[nSprite] += dy;
            }
        }

        if (pXSector && pXSector->underwater)
            actAirDrag(pDude, 5376);
        else
            actAirDrag(pDude, 128);

        if ((pDude->flags&4) || xvel[nSprite] || yvel[nSprite] || zvel[nSprite] ||
            velFloor[pDude->sectnum] || velCeil[pDude->sectnum])
            MoveDude(pDude);


        if (CheckProximity(pDude, posx, posy, posz, cursectnum, 220))
        {
            XSPRITE *pXDude = &xsprite[pDude->extra];
            switch (pDude->type) {
                case kDudeZombieAxeLaying:
                case kDudeZombieAxeBuried:
                    previewMorphDude(pDude, pXDude);
                    break;
            }
        }
    }

    // process things for effects
    for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        spritetype *pSprite = &sprite[nSprite];
        if ((pSprite->flags & kHitagFree) || pSprite->extra <= 0)
            continue;

        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSprite->triggerProximity) {
            for (nDude = headspritestat[kStatDude]; nDude >= 0; nDude = nNextDude) {
                nNextDude = nextspritestat[nDude];
                spritetype *pDude = &sprite[nDude];
                if (pDude->flags & kHitagFree) continue;
                else if (CheckProximity(pDude, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 96))
                    trTriggerSprite(nSprite, pXSprite, kCmdSpriteProximity);

            }

            if (actCheckProximity(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 96))
                trTriggerSprite(pSprite->index, pXSprite, kCmdSpriteProximity);
        }
    }

    for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->extra <= 0)
            continue;

        int nSector = pSprite->sectnum;
        XSECTOR *pXSector = (sector[nSector].extra > 0) ? &xsector[sector[nSector].extra] : NULL;
        if (pXSector && pXSector->panVel && (pXSector->panAlways || pXSector->state || pXSector->busy))
        {
            int nType = pSprite->type - kThingBase;
            THINGINFO *pThingInfo = &thingInfo[nType];
            if (pThingInfo->flags & kPhysMove)
                pSprite->flags |= kPhysMove;

            if (pThingInfo->flags & kPhysGravity)
                pSprite->flags |= kPhysFalling;
        }

        if (pSprite->flags & (kPhysMove|kPhysGravity))
        {
            if (pXSector && pXSector->panVel)
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
                {
                    int angle = pXSector->panAngle;
                    int speed = 0;
                    if (pXSector->panAlways || pXSector->state || pXSector->busy)
                    {
                        speed = pXSector->panVel << 9;
                        if (!pXSector->panAlways && pXSector->busy)
                            speed = mulscale16(speed, pXSector->busy);
                    }
                    if (sector[nSector].floorstat&64)
                        angle = (angle+GetWallAngle(sector[nSector].wallptr)+512)&2047;
                    int dx = mulscale30(speed, Cos(angle));
                    int dy = mulscale30(speed, Sin(angle));
                    xvel[nSprite] += dx;
                    yvel[nSprite] += dy;
                }
            }
            actAirDrag(pSprite, 128);

            if (((pSprite->index>>8)&15) == (gFrame&15) && (pSprite->flags&2))
                pSprite->flags |= kPhysFalling;
            if ((pSprite->flags & kPhysFalling) || xvel[nSprite] || yvel[nSprite] || zvel[nSprite] ||
                velFloor[pSprite->sectnum] || velCeil[pSprite->sectnum])
            {
                int hit = MoveThing(pSprite);
            }
        }
    }

    // process missile sprites
    for (nSprite = headspritestat[kStatProjectile]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        spritetype *pSprite = &sprite[nSprite];
        if ( pSprite->flags & kHitagFree )
            continue;

        int hitCode = MoveMissile(pSprite);

        // process impacts
        if ( hitCode >= 0 )
            actImpactMissile(pSprite, hitCode);
    }

    // process explosions
    for (nSprite = headspritestat[kStatExplosion]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->flags & kHitagFree)
            continue;

        BYTE sectmap[(kMaxSectors+7)>>3];
        gAffectedSectors[0] = -1;
        gAffectedXWalls[0] = -1;

        int nType = pSprite->type;
        EXPLODE_INFO *pExp = &explodeInfo[nType];

        int nXSprite = pSprite->extra;
        XSPRITE *pXSprite = &xsprite[nXSprite];

        int radius = pExp->radius;
        if (gModernMap && pXSprite->data4 > 0)
            radius = pXSprite->data4;


        int nAffected;
        int x = pSprite->x, y = pSprite->y, z = pSprite->z, nSector = pSprite->sectnum;

        int dx = (x - posx)>>4;
        int dy = (y - posy)>>4;
        int dz = (z - posz)>>8;
        int nDist = dx*dx+dy*dy+dz*dz+0x40000;
        int t = divscale16(pXSprite->data2, nDist);
        gPreview.scrEffects[kScrEffectQuake2] += t;

        GetClosestSpriteSectors(nSector, x, y, radius, gAffectedSectors, sectmap, gAffectedXWalls);

        for (int i = 0; i < kMaxXWalls; i++)
        {
            int nWall = gAffectedXWalls[i];
            if (nWall == -1)
                break;
            XWALL *pXWall = &xwall[wall[nWall].extra];
            trTriggerWall(nWall, pXWall, kCmdWallImpact);
        }


        for (nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected])
        {
            spritetype *pDude = &sprite[nAffected];
            if (pXSprite->data1 && CheckProximity(pDude, x, y, z, nSector, radius))
            {
                if (pDude->extra < 0) continue;
                XSPRITE *pXDude = &xsprite[pDude->extra];
                previewKillDude(pDude, pXDude);
            }
        }

        for (nAffected = headspritestat[kStatThing]; nAffected >= 0; nAffected = nextspritestat[nAffected])
        {
            spritetype *pThing = &sprite[nAffected];
            if (pXSprite->data1 && CheckProximity(pThing, x, y, z, nSector, radius))
            {
                if (pThing->extra < 0) continue;
                XSPRITE *pXThing = &xsprite[pThing->extra];
                trTriggerSprite(pThing->index, pXThing, kCmdOff);
            }
        }

        if (pXSprite->data1 != 0)
        {
            int32_t* ptr;
            if (explodeInfo->dmgType != 0)
            {
                ptr = gPhysSpritesList.GetPtr();
                while(*ptr >= 0) // add impulse for sprites from physics list
                {
                    spritetype* pPhys = &sprite[*ptr++];
                    if (pPhys->sectnum < 0 || pPhys->extra <= 0) continue;
                    else if (!TestBitString(sectmap, pPhys->sectnum) || !CheckProximity(pPhys, x, y, z, nSector, radius))
                        continue;

                    debrisConcuss(nSprite, pPhys->index, x, y, z, explodeInfo->dmgType);
                }
            }


            ptr = gImpactSpritesList.GetPtr();
            while(*ptr >= 0) // trigger sprites from impact list
            {
                spritetype* pImp = &sprite[*ptr++];
                if (pImp->sectnum < 0 || pImp->extra <= 0) continue;
                else if (!TestBitString(sectmap, pImp->sectnum) || !CheckProximity(pImp, x, y, z, nSector, radius))
                    continue;

                trTriggerSprite(pImp->index, &xsprite[pImp->extra], kCmdSpriteImpact);
            }
        }

        int ticks = 4 << 1;
        pXSprite->data1 = ClipLow(pXSprite->data1 - ticks, 0);
        pXSprite->data2 = ClipLow(pXSprite->data2 - ticks, 0);
        pXSprite->data3 = ClipLow(pXSprite->data3 - ticks, 0);
        if (pXSprite->data1 == 0 && pXSprite->data2 == 0 && pXSprite->data3 == 0 && seqGetStatus(OBJ_SPRITE, nXSprite) < 0) {
            actPostSprite(nSprite, kStatFree);
        }
    }

    gFX.fxProcess();

}

void actAirDrag(int* vx, int* vy, int* vz, int nSect, int nDrag)
{
    int windX = 0, windY = 0;
    sectortype *pSector = &sector[nSect];

    if (pSector->extra > 0)
    {
        XSECTOR *pXSector = &xsector[pSector->extra];
        if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
        {
            int windVel = pXSector->windVel << 12;

            if ( !pXSector->windAlways && pXSector->busy )
                windVel = mulscale16(windVel, pXSector->busy);

            windX = mulscale30(windVel, Cos(pXSector->windAng));
            windY = mulscale30(windVel, Sin(pXSector->windAng));
        }
    }

    if (vx) *vx += mulscale16(windX - *vx, nDrag);
    if (vy) *vy += mulscale16(windY - *vy, nDrag);
    if (vz) *vz -= mulscale16(*vz, nDrag);

}

void actAirDrag( spritetype *pSpr, int nDrag ) {

    int nSpr = pSpr->index;
    actAirDrag(&xvel[nSpr], &yvel[nSpr], &zvel[nSpr], pSpr->sectnum, nDrag);
}

void actExplodeSprite(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    if (nXSprite <= 0 || nXSprite >= kMaxXSprites || pSprite->statnum == kStatExplosion)
        return;

    sfxKill3DSound(pSprite);
    evKill(pSprite->index, 3);
    short nType = kExplosionStandard;

    switch (pSprite->type) {
        case kMissileFireballNapalm:
            nType = kExplosionNapalm;
            seqSpawn(4, 3, nXSprite, -1);
            if (Chance(0x8000)) pSprite->cstat |= 4;
            sfxPlay3DSound(pSprite, 303, -1, 0);
            break;
        case kMissileFlareAlt:
            nType = kExplosionFireball;
            seqSpawn(9, 3, nXSprite, -1);
            if (Chance(0x8000)) pSprite->cstat |= 4;
            sfxPlay3DSound(pSprite, 306, 24+(pSprite->index&3), 1);
            break;
        case kMissileFireballCerberus:
        case kMissileFireballTchernobog:
            nType = kExplosionFireball;
            seqSpawn(5, 3, nXSprite, -1);
            sfxPlay3DSound(pSprite, 304, -1, 0);
            break;
        case kThingArmedTNTStick:
            nType = kExplosionSmall;
            seqSpawn(3,3,nXSprite,-1);
            sfxPlay3DSound(pSprite, 303, -1, 0);
            break;
        case kThingArmedProxBomb:
        case kThingArmedRemoteBomb:
        case kThingArmedTNTBundle:
            nType = 1;
            seqSpawn(3, OBJ_SPRITE, nXSprite);
            sfxPlay3DSound(pSprite, 303, -1, 0);
            break;
        case kThingArmedSpray:
            nType = kExplosionSpray;
            seqSpawn(5, 3, nXSprite, -1);
            sfxPlay3DSound(pSprite, 307, -1, 0);
            break;
        case kThingTNTBarrel:
            nType = kExplosionLarge;
            seqSpawn(4, 3, nXSprite, -1);
            sfxPlay3DSound(pSprite, 305, -1, 0);
            break;
        case kTrapExploder: {
            // Defaults for exploder
            nType = 1; int nSnd = 304; int nSeq = 4;

            // allow to customize hidden exploder trap
            if (gModernMap) {
                // Temp variables for override via data fields
                int tSnd = 0; int tSeq = 0;


                XSPRITE* pXSPrite = &xsprite[nXSprite];
                nType = (short)pXSPrite->data1;  // Explosion type
                tSeq = pXSPrite->data2; // SEQ id
                tSnd = pXSPrite->data3; // Sound Id

                if (nType <= 1 || nType > kExplosionMax) { nType = 1; nSeq = 4; nSnd = 304; }
                else if (nType == 2) { nSeq = 4; nSnd = 305; }
                else if (nType == 3) { nSeq = 9; nSnd = 307; }
                else if (nType == 4) { nSeq = 5; nSnd = 307; }
                else if (nType <= 6) { nSeq = 4; nSnd = 303; }
                else if (nType == 7) { nSeq = 4; nSnd = 303; }
                else if (nType == 8) { nType = 0; nSeq = 3; nSnd = 303; }

                // Override previous sound and seq assigns
                if (tSeq > 0) nSeq = tSeq;
                if (tSnd > 0) nSnd = tSnd;
            }


            if (gSysRes.Lookup(nSeq, "SEQ"))
                seqSpawn(nSeq, 3, nXSprite, -1);

            sfxPlay3DSound(pSprite, nSnd, -1, 0);
        }
            break;
        case kThingPodFireBall:
            nType = kExplosionFireball;
            seqSpawn(9, 3, nXSprite, -1);
            sfxPlay3DSound(pSprite, 307, -1, 0);
            break;
        default:
            nType = kExplosionStandard;
            seqSpawn(4, 3, nXSprite, -1);
            if (Chance(0x8000))
                pSprite->cstat |= 4;
            sfxPlay3DSound(pSprite, 303, -1, 0);
            break;
    }

    int nSprite = pSprite->index;
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
    actPostSprite(nSprite, kStatExplosion);

    EXPLODE_INFO *pExplodeInfo = &explodeInfo[nType];
    pSprite->type = nType;
    pSprite->flags &= ~3;

    pSprite->xrepeat = pSprite->yrepeat = pExplodeInfo->repeat;
    xsprite[nXSprite].target = 0;
    xsprite[nXSprite].data1 = pExplodeInfo->ticks;
    xsprite[nXSprite].data2 = pExplodeInfo->quakeEffect;
    xsprite[nXSprite].data3 = pExplodeInfo->flashEffect;

}

spritetype *actSpawnSprite( int nSector, int x, int y, int z, int nStatus, BOOL bAddXSprite) {

    if (nSector < 0)
        ThrowError("Out of sector!");

    int nSprite = InsertSprite(nSector, nStatus);
    spritetype *pSprite = &sprite[nSprite];

    setsprite(pSprite->index, x, y, z);
    pSprite->yvel = kDeleteReally;
    pSprite->type    = 0;

    if (bAddXSprite && pSprite->extra == -1)
        dbInsertXSprite(pSprite->index);

    return pSprite;

}

spritetype *actSpawnSprite( spritetype *pSource, int nStat ) {

    spritetype *pSprite = actSpawnSprite(pSource->sectnum, pSource->x, pSource->y, pSource->z, nStat, TRUE);
    if (pSprite) {
        xvel[pSprite->index] = xvel[pSource->index];
        yvel[pSprite->index] = yvel[pSource->index];
        zvel[pSprite->index] = zvel[pSource->index];
        pSprite->flags = 0;
    }

    return pSprite;
}

spritetype* actDropObject( spritetype *pActor, int nObject ) {

    // create a sprite for the dropped ammo
    int x = pActor->x;
    int y = pActor->y;
    if (pActor->sectnum < 0 || pActor->sectnum >= kMaxSectors)
    {
        previewMessage("%s (%d) have no sector!", gSpriteNames[pActor->type], pActor->index);
        return NULL;
    }

    int z = getflorzofslope(pActor->sectnum, x, y);
    spritetype *pSprite = actSpawnSprite(pActor->sectnum, x, y, z, kStatItem, FALSE);
    pSprite->type = (short)nObject;
    adjSpriteByType(pSprite);
    pSprite->shade = -127;

    clampSprite(pSprite);
    return pSprite;

}

spritetype *actSpawnDude(spritetype *pSource, short nType, int a3, int a4) {
    if (pSource->sectnum < 0) return NULL;
    XSPRITE* pXSource = &xsprite[pSource->extra];
    a3 = 0;
    int x, y, z;
    z = a4 + pSource->z;
    x = pSource->x;
    y = pSource->y;

    spritetype *pSprite2 = actSpawnSprite(pSource->sectnum, x, y, z, kStatDude, TRUE);
    if (!pSprite2) return NULL;
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    pXSprite2->state = 1;
    pXSprite2->health = 100 << 4;
    pSprite2->type = nType;
    pSprite2->ang = pSource->ang;
    //setsprite(pSprite2->index, x, y, z);


    pSprite2->x = x;
    pSprite2->y = y;
    pSprite2->z = z;

    ChangeSpriteSect(pSprite2->index, pSource->sectnum);

    pSprite2->cstat |= 0x1101;
    pSprite2->clipdist = 32;

    if (!isUnderwaterSector(pSprite2->sectnum))
        pSprite2->flags =(kPhysGravity|kPhysMove|kPhysFalling);

    adjSpriteByType(pSprite2);

    if (pSprite2->sectnum >= 0)
        clampSprite(pSprite2);

    return pSprite2;
}

void actActivateGibObject(spritetype *pSprite, XSPRITE *pXSprite) {

    if (pSprite->flags & 32)
        return;

    int vdx = ClipRange(pXSprite->data1, 0, 31);
    int vc = ClipRange(pXSprite->data2, 0, 31);
    int v4 = ClipRange(pXSprite->data3, 0, 31);

    if (vdx > 0) GibSprite(pSprite, (GIBTYPE)(vdx-1), NULL, NULL);
    if (vc > 0) GibSprite(pSprite, (GIBTYPE)(vc-1), NULL, NULL);
    if (v4 > 0 && pXSprite->burnTime > 0) GibSprite(pSprite, (GIBTYPE)(v4-1), NULL, NULL);

    if (pXSprite->data4)
        sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, pXSprite->data4);

    if (pXSprite->dropItem)
        actDropObject(pSprite, pXSprite->dropItem);

    //if (pXSprite->key)
        //actDropObject(pSprite, pXSprite->key + kItemBase - 1);

    if (!(pSprite->cstat & kSprInvisible))
        hideSprite(pSprite);
        //actPostSprite(pSprite->index, kStatFree);
}

void actPostSprite(int nSprite, int nStatus) {
    int n;
    dassert(gPostCount < kMaxSprites);
    dassert(nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus);
    dassert(nStatus >= 0 && nStatus <= kStatFree);
    if (sprite[nSprite].flags & 32)
    {
        for (n = 0; n < gPostCount; n++)
            if (gPost[n].at0 == nSprite)
                break;
        dassert(n < gPostCount);
    }
    else
    {
        n = gPostCount;
        sprite[nSprite].flags |= 32;
        gPostCount++;
    }
    gPost[n].at0 = (short)nSprite;
    gPost[n].at2 = (short)nStatus;
}

void actPostProcess(void) {

    for (int i = 0; i < gPostCount; i++) {
        POSTPONE *pPost = &gPost[i];
        int nSprite = pPost->at0;
        spritetype *pSprite = &sprite[nSprite];
        pSprite->flags &= ~32;
        int nStatus = pPost->at2;
        if (nStatus == kStatFree) previewDelSprite(nSprite);
        else ChangeSpriteStat(nSprite, nStatus);
    }

    gPostCount = 0;
}

int actWallBounceVector(int *x, int *y, int nWall, int a4)
{
    int wx, wy;
    GetWallNormal(nWall, &wx, &wy);
    int t = dmulscale16(*x, wx, *y, wy);
    int t2 = mulscale16r(t, a4+0x10000);
    *x -= mulscale16(wx, t2);
    *y -= mulscale16(wy, t2);
    return mulscale16r(t, 0x10000-a4);
}

int actFloorBounceVector(int *x, int *y, int *z, int nSector, int a5)
{
    int t = 0x10000-a5;
    if (sector[nSector].floorheinum == 0)
    {
        int t2 = mulscale16(*z, t);
        *z = -(*z-t2);
        return t2;
    }
    walltype *pWall = &wall[sector[nSector].wallptr];
    walltype *pWall2 = &wall[pWall->point2];
    int angle = getangle(pWall2->x-pWall->x, pWall2->y-pWall->y)+512;
    int t2 = sector[nSector].floorheinum<<4;
    int t3 = approxDist(-0x10000, t2);
    int t4 = divscale16(-0x10000, t3);
    int t5 = divscale16(t2, t3);
    int t6 = mulscale30(t5, Cos(angle));
    int t7 = mulscale30(t5, Sin(angle));
    int t8 = tmulscale16(*x, t6, *y, t7, *z, t4);
    int t9 = mulscale16(t8, 0x10000+a5);
    *x -= mulscale16(t6, t9);
    *y -= mulscale16(t7, t9);
    *z -= mulscale16(t4, t9);
    return mulscale16r(t8, t);
}

void actTouchFloor(spritetype *pSprite, int nSector)
{
    sectortype * pSector = &sector[nSector];
    XSECTOR * pXSector = NULL;
    if (pSector->extra > 0)
        pXSector = &xsector[pSector->extra];

    bool doDamage = (pXSector && (pSector->type == kSectorDamage || pXSector->damageType > 0));
    // don't allow damage for damage sectors if they are not enabled
    #ifdef NOONE_EXTENSIONS
    if (gModernMap && doDamage && pSector->type == kSectorDamage && !pXSector->state)
        doDamage = false;
    #endif

    if (doDamage) {
        DAMAGE_TYPE nDamageType;

        if (pSector->type == kSectorDamage)
            nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType, kDamageFall, kDamageTesla);
        else
            nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType - 1, kDamageFall, kDamageTesla);
        int nDamage;
        if (pXSector->data)
            nDamage = ClipRange(pXSector->data, 0, 1000);
        else
            nDamage = 1000;
        actDamageSprite(pSprite->index, pSprite, nDamageType, scale(4, nDamage, 120) << 4);
    }
    if (tileGetSurfType(nSector + 0x4000) == kSurfLava)
    {
        actDamageSprite(pSprite->index, pSprite, kDamageBurn, 16);
        sfxPlay3DSound(pSprite, 352, 5, 2);
    }
}

spritetype * actSpawnThing(int nSector, int x, int y, int z, int nThingType)
{
    spritetype *pSprite = actSpawnSprite(nSector, x, y, z, 4, 1);
    if (pSprite == NULL)
        return NULL;

    pSprite->type = nThingType;
    XSPRITE *pXThing = &xsprite[GetXSprite(pSprite->extra)];
    THINGINFO *pThingInfo = &thingInfo[nThingType - kThingBase];

    pXThing->health = pThingInfo->startHealth<<4;
    pSprite->clipdist = pThingInfo->clipdist;
    pSprite->flags = pThingInfo->flags;

    if (pSprite->flags & 2)
        pSprite->flags |= 4;

    pSprite->cstat     |= pThingInfo->cstat;
    pSprite->picnum     = pThingInfo->picnum;
    pSprite->shade      = pThingInfo->shade;
    pSprite->pal        = pThingInfo->pal;

    if (pThingInfo->xrepeat)
        pSprite->xrepeat = pThingInfo->xrepeat;

    if (pThingInfo->yrepeat)
        pSprite->yrepeat = pThingInfo->yrepeat;

    SetBitString(show2dsprite, pSprite->index);
    switch (nThingType) {
        case kThingArmedTNTStick:
        case kThingArmedTNTBundle:
            sfxPlay3DSound(pSprite, 450, 0, 0);
            // no break
        case kThingArmedSpray:
            //evPost(pSprite->index, 3, 0, kCallbackFXDynPuff);
            break;
    }
    return pSprite;
}

spritetype * actFireThing(spritetype *pSprite, int a2, int a3, int a4, int thingType, int a6)
{
    int x = pSprite->x+mulscale30(a2, Cos(pSprite->ang+512));
    int y = pSprite->y+mulscale30(a2, Sin(pSprite->ang+512));
    int z = pSprite->z+a3;
    x += mulscale28(pSprite->clipdist, Cos(pSprite->ang));
    y += mulscale28(pSprite->clipdist, Sin(pSprite->ang));
    if (HitScan(pSprite, z, x-pSprite->x, y-pSprite->y, 0, CLIPMASK0, pSprite->clipdist) != -1)
    {
        x = gHitInfo.hitx-mulscale28(pSprite->clipdist<<1, Cos(pSprite->ang));
        y = gHitInfo.hity-mulscale28(pSprite->clipdist<<1, Sin(pSprite->ang));
    }
    spritetype *pThing = actSpawnThing(pSprite->sectnum, x, y, z, thingType);
    //actPropagateSpriteOwner(pThing, pSprite);
    pThing->ang = pSprite->ang;
    xvel[pThing->index] = mulscale30(a6, Cos(pThing->ang));
    yvel[pThing->index] = mulscale30(a6, Sin(pThing->ang));
    zvel[pThing->index] = mulscale14(a6, a4);
    xvel[pThing->index] += xvel[pSprite->index]/2;
    yvel[pThing->index] += yvel[pSprite->index]/2;
    zvel[pThing->index] += zvel[pSprite->index]/2;
    return pThing;
}

void actNapalmMove(spritetype *pSprite, XSPRITE *pXSprite)
{
    //int nSprite = actOwnerIdToSpriteId(pSprite->owner);
    actPostSprite(pSprite->index, kStatDecoration);
    seqSpawn(9, 3, pSprite->extra);
    if (Chance(0x8000))
        pSprite->cstat |= 4;

    sfxPlay3DSound(pSprite, 303, 24+(pSprite->flags&3), 1);
    //actRadiusDamage(nSprite, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 128, 0, 60, kDamageExplode, 15, 120);
    if (pXSprite->data4 > 1)
    {
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        int v14[2];
        v14[0] = pXSprite->data4>>1;
        v14[1] = pXSprite->data4-v14[0];
        int v4 = pSprite->ang;
        xvel[pSprite->index] = 0;
        yvel[pSprite->index] = 0;
        zvel[pSprite->index] = 0;
        for (int i = 0; i < 2; i++)
        {
            int t1 = Random(0x33333)+0x33333;
            int t2 = BiRandom(0x71);
            pSprite->ang = (t2+v4+2048)&2047;
            spritetype *pSprite2 = actFireThing(pSprite, 0, 0, -0x93d0, kThingNapalmBall, t1);
            XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
            pSprite2->owner = pSprite->owner;
            seqSpawn(61, 3, pSprite2->extra);
            pXSprite2->data4 = v14[i];
        }
    }
}

int actDamageSprite(int nSource, spritetype *pSprite, DAMAGE_TYPE damageType, int damage)
{
    return 0;
}

