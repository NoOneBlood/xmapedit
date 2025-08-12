/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// A class that builds ray of tsprites similar to laser with support of sector
// intersection. This file provides functionality for kModernLaserGen types
// which is part of nnexts.cpp. More info at http://cruo.bloodgame.ru/xxsystem
//
//
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

#include "nnexts.h"
#include "tile.h"
#include "preview.h"
#include "xmpview.h"
#include "nnextslaser.h"

#define kFlagLaserTrOnceHit         0x001
#define kFlagLaserTrOnceLast        0x002
#define kFlagLaserNoDudes           0x004
#define kFlagLaserNoPlayer          0x008
#define kFlagLaserNoThings          0x010
#define kFlagLaserNoMasks           0x040
#define kFlagLaserNoDead            0x080
#define kFlagLaserStrictCmd         0x200
#define kFlagLaserUseHitscan        0x400
#define kFlagLaserDynamicSets       0x800

#define DIST2PIX(a) (a << 8)

/**------------------------------------------
// Comment to save on tiles and use tsprite
// shade fading instead of
// tile animation.
**/
// #define LASER_SHD_USE_TILEANIM


/**-----------------------------------------
// Comment to not compensate view shade
// for tsprite (when put laser view
// processing after
// tsprites).
**/
//#ifndef LASER_SHD_USE_TILEANIM
//#define LASER_SHD_COMPENSATE
//#endif

LASER::LASER(spritetype* pLaser)
{
    pieces = NULL;
    Init(pLaser);
}

LASER::~LASER()
{
    RayDelete();
    if (pieces)
        free(pieces);
}

void LASER::Init(spritetype* pLaser)
{
    color               = 0;
    numpieces           = 0;
    piecescapacity      = 0;
    xrepeat             = 6;
    yrepeat             = 6;
    thickness           = 6;
    maxdist             = 0;
    raydone             = 0;

    oldscan.hitsprite   = -1;

    pOwn = pLaser;
    pXOwn = &xsprite[pOwn->extra];
    UpdateProperties();
    RayDelete();
}

void LASER::GenerateTiles(char nID)
{
    #define kTileW              48  // must be even for perfect positioning
    #define kTileH              16  // must be even for perfect positioning
    #define kGlobShdStep        8

    #ifdef LASER_SHD_USE_TILEANIM
    #define kFrames             32 / kGlobShdStep
    #else
    #define kFrames             1
    #endif

    unsigned char *pTileH, *pTileV, *pPlu;
    int l = kTileW*kTileH, i, f, j, k, g;
    int nColor = colors[nID][0];
    int nLineShade, nStep;
    int nTileShade = 0;
    int nTile;

    for (i = 0; i < LENGTH(tile); i++)
    {
        if (tile[i][nID] > 0)
            return;
    }

    //nTile = 1;
    if ((nTile = tileSearchFreeRange(kFrames<<1)) >= 0)
    {
        for (i = nTile; i < nTile + kFrames; i++)
        {
            if (tileLoadTile(i))
                tileFreeTile(i);

            tileAllocTile(i,  kTileH, kTileW);
            pTileV = tileLoadTile(i);
            gSysTiles.add(i);


            if (tileLoadTile(i + kFrames))
                tileFreeTile(i + kFrames);

            tileAllocTile(i + kFrames,  kTileW, kTileH);
            pTileH = tileLoadTile(i + kFrames);
            gSysTiles.add(i + kFrames);

            nLineShade  = colors[nID][2];
            nStep       = colors[nID][1];

            // first make gradient color for each line
            for (j = 0, f = 0; j < kTileH; j++, f+=kTileW)
            {
                pPlu = (unsigned char*)(palookup[0] + shgetpalookup(0, nLineShade));
                for (k = f; k < f + kTileW; k++)
                    pTileV[k] = pPlu[gStdColor[nColor]];

                if (j == (kTileH>>1) - 1)
                {
                    nStep = -nStep;
                }
                else
                {
                    nLineShade += nStep;
                }
            }

            // make the whole tile  darker
            pPlu = (unsigned char*)(palookup[0] + shgetpalookup(0, nTileShade));
            for (j = 0; j < l; j++)
                pTileV[j] = pPlu[pTileV[j]];

            // rotate and copy vertical tile
            j = k = f = g = 0;
            while (j < l && f < l)
            {
                pTileH[j] = pTileV[k];

                f = j, g = k;
                while ((g += kTileW) < l && ++f < l)
                    pTileH[f] = pTileV[g];

                j+=kTileH;
                k++;
            }

            nTileShade += kGlobShdStep;
        }

        tile[1][nID]        = nTile;
        tile[0][nID]        = nTile + kFrames;

        #if (kFrames > 1)
            panm[nTile].frames  = panm[nTile + kFrames].frames      = kFrames - 1;
            panm[nTile].speed   = panm[nTile + kFrames].speed       = 1;
            panm[nTile].type    = panm[nTile + kFrames].type        = 2;
        #endif
    }
}

void LASER::GenerateTables()
{
    int x0, y0, x1, y1, zb, zt;
    int i, j, k;

    spritetype model;
    model.cstat |= (kSprWall | kSprOrigin);
    model.x = model.y = model.z = 0;
    model.sectnum = 0;
    model.ang = 1536;

    for (i = 0; i < 2; i++)
    {
        model.picnum = 0;
        for (j = 0; j < LENGTH(tile[0]) && !model.picnum; j++)
            model.picnum = tile[i][j];

        for (k = 1; k < 256; k++)
        {
            model.xrepeat = k;
            model.yrepeat = k;

            GetSpriteExtents(&model, &x0, &y0, &x1, &y1, &zt, &zb);

            size[i][0][k] = klabs(x1-x0);
            size[i][2][k] = klabs(zb-zt);
            size[i][1][k] = size[i][2][k] >> 4;
        }
    }
}

spritetype* LASER::Put(RAYPIECE* p)
{
    spritetype* pSpr;
    if ((pSpr = viewInsertTSprite(p->s, 32767)) != NULL)
    {
        pSpr->xrepeat   = p->w;
        pSpr->yrepeat   = p->h;
        pSpr->picnum    = GetTile();
        pSpr->ang       = (pOwn->ang + kAng90) & kAngMask;
        pSpr->pal       = pOwn->pal;

        if ((pOwn->cstat & kSprTransluc2) == kSprTransluc1)
            pSpr->cstat |= kSprTransluc1;
        else
            pSpr->cstat |= kSprTransluc2;

        #ifndef LASER_SHD_USE_TILEANIM
            pSpr->shade = Fade(0, -16, 8);
            #ifdef LASER_SHD_COMPENSATE
                pSpr->shade -= viewSpriteShade(0, pSpr->picnum, pSpr->sectnum);
            #endif
        #else
            pSpr->shade = -128;
        #endif

        pSpr->owner = headspritestat[kStatFree];
        pSpr->x     = p->x;
        pSpr->y     = p->y;
        pSpr->z     = p->z;
    }

    return pSpr;
}

char LASER::PiecePutH(RAYPIECE* p, int nDAng1, int nDAng2)
{
    int hxs, hys, hzs; spritetype* pSpr;

    if (spritesortcnt + 2 >= MAXSPRITESONSCREEN)
        return 0;

    hxs = size[0][0][p->w] >> 1;
    hys = size[0][1][p->h] >> 1;
    hzs = size[0][2][p->h] >> 1;

    if (nDAng1 < 0)
    {
        // left
        pSpr = Put(p);
        pSpr->cstat |= (kSprWall | kSprOneSided);
        offsetPos(-(hys-1), 0, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);
    }
    else
    {
        // right
        pSpr = Put(p);
        pSpr->cstat |= (kSprWall | kSprOneSided);
        pSpr->ang = (pSpr->ang + kAng180) & kAngMask;
        offsetPos(+(hys-1), 0, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);
    }

    if (cam.z >= p->z + hzs)
    {
        // bot
        pSpr = Put(p);
        pSpr->cstat |= (kSprFloor | kSprOneSided | kSprFlipY);
        offsetPos(0, 0, (hzs-4), pOwn->ang, NULL, NULL, &pSpr->z);
    }
    else
    {
        // top
        pSpr = Put(p);
        pSpr->cstat |= (kSprFloor | kSprOneSided);
        offsetPos(0, 0, -(hzs-4), pOwn->ang, NULL, NULL, &pSpr->z);
    }

    return 1;
}

char LASER::PiecePutV(RAYPIECE* p, int nDAng1, int nDAng2)
{
    int hxs; spritetype* pSpr;

    if (spritesortcnt + 2 >= MAXSPRITESONSCREEN)
        return 0;

    hxs = size[1][0][p->w] >> 1;

    if (nDAng1 < 0)
    {
        // left
        pSpr = Put(p);
        pSpr->cstat |= (kSprWall | kSprOneSided), pSpr->cstat &= ~kSprOrigin;
        offsetPos(-hxs, 0, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);
    }
    else
    {
        // right
        pSpr = Put(p);
        pSpr->ang = ((pOwn->ang + kAng90) + kAng180) & kAngMask;
        pSpr->cstat |= (kSprWall | kSprOneSided), pSpr->cstat &= ~kSprOrigin;
        offsetPos(hxs, 0, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);

    }

    if (nDAng2 < 0)
    {
        // forward
        pSpr = Put(p);
        pSpr->ang = (pOwn->ang) & kAngMask;
        pSpr->cstat |= (kSprWall | kSprOneSided), pSpr->cstat &= ~kSprOrigin;
        offsetPos(0, hxs, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);
    }
    else
    {
        // backward
        pSpr = Put(p);
        pSpr->ang = ((pOwn->ang) + kAng180) & kAngMask;
        pSpr->cstat |= (kSprWall | kSprOneSided), pSpr->cstat &= ~kSprOrigin;
        offsetPos(0, -hxs, 0, pOwn->ang, &pSpr->x, &pSpr->y, NULL);
    }

    return 1;
}

void LASER::PieceListAdd(RAYPIECE* pPiece)
{
    if (numpieces >= piecescapacity)
        PieceListGrow();

    SetBitString(piecesectmap, pPiece->s);
    pieces[numpieces++] = *pPiece;
}

void LASER::PieceListGrow(int nBy)
{
    piecescapacity = numpieces + nBy;
    pieces = (RAYPIECE*)realloc(pieces, sizeof(RAYPIECE) * piecescapacity);
    dassert(pieces != NULL);
}

void LASER::RayCollectPiecesH(int nSect, int nFullDist, POSOFFS* pos)
{
    SCANWALL src, dst; spritetype* pSpr;
    int nDist = 0, sz, t;

    src.pos = *pos;
    src.s = nSect;
    src.w = -1;

    RAYPIECE piece;
    piece.z = pos->z;
    piece.h = yrepeat;

    while(src.s >= 0 && nFullDist > 8)
    {
        src.pos.Forward(4);
        if (!scanWallOfSector(&src, &dst))
            return; // there is no space to shoot the ray

        nDist = Distance(pos->x - dst.pos.x, pos->y - dst.pos.y);
        nDist = ClipHigh(nDist, nFullDist);

        do
        {
            xrepeat = LookupRepeat(&size[0][0][0], nDist);
            sz = size[0][0][xrepeat];
            nFullDist -= sz;
            nDist -= sz;
            t = sz>>1;

            pos->Forward(t);

            piece.w = xrepeat;
            piece.x = pos->x;
            piece.y = pos->y;
            piece.s = src.s;

            PieceListAdd(&piece);

            pos->Forward(t);
        }
        while(nDist > 32);

        pos->Backward(1); // for less visable spaces

        src.pos = dst.pos, src.pos.a = pos->a;
        src.s   = wall[dst.w].nextsector;
        src.w   = wall[dst.w].nextwall;
    }
}

void LASER::RayCollectPiecesV(int nSect, int nDist, POSOFFS* pos)
{
    int sz;
    RAYPIECE piece;

    piece.x = pos->x;
    piece.y = pos->y;
    piece.w = xrepeat;
    piece.s = nSect;

    while(nDist > 8)
    {
        yrepeat = LookupRepeat(&size[1][2][0], nDist);
        sz = size[1][2][yrepeat];
        piece.h = yrepeat;
        nDist -= sz;

        if (direction)
        {
            piece.z = pos->z;
            pos->Up(sz);
        }
        else
        {
            pos->Down(sz);
            piece.z = pos->z;
        }

        PieceListAdd(&piece);
    }
}


void LASER::SetState(char nState)
{
    LASERSCAN* o = &oldscan;
    int nCmd;

    if (nState)
    {
        pXOwn->state = 1;
        o->hitsprite = -1;
        raydone = 0;
        Process();
    }
    else
    {
        if (pXOwn->triggerOff && o->hitsprite >= 0)
        {
            nCmd = (strictcmd)
                    ? kCmdOff : pXOwn->command;

            if (pXOwn->txID)
            {
                 evSend(pOwn->index, EVOBJ_SPRITE, pXOwn->txID, (COMMAND_ID)nCmd, o->hitsprite);
            }
            else
            {
                nnExtTriggerObject(EVOBJ_SPRITE, o->hitsprite, nCmd);
            }
        }

        pXOwn->state = raydone = 0;
        RayDelete();
    }
}

char LASER::UpdateProperties()
{
    char r = 0;
    char t;

    troncehit   = CheckFlags(kFlagLaserTrOnceHit);
    troncelast  = CheckFlags(kFlagLaserTrOnceLast);
    strictcmd   = CheckFlags(kFlagLaserStrictCmd);

    if (pXOwn->data1 != color && rngok(pXOwn->data1, 0, kLaserMaxColors))
    {
        LASER::GenerateTiles(pXOwn->data1);
        color = pXOwn->data1;
    }

    /** The following changes will require ray rebuilding **/
    /*******************************************************/

    if ((t = CheckFlags(kFlagLaserNoDudes)) != nodudes)         nodudes     = t,    r = 1;
    if ((t = CheckFlags(kFlagLaserNoPlayer)) != noplayers)      noplayers   = t,    r = 1;
    if ((t = CheckFlags(kFlagLaserNoThings)) != nothings)       nothings    = t,    r = 1;
    if ((t = CheckFlags(kFlagLaserNoMasks)) != nomasks)         nomasks     = t,    r = 1;
    if ((t = CheckFlags(kFlagLaserNoDead)) != nodead)           nodead      = t,    r = 1;
    if ((t = CheckFlags(kFlagLaserUseHitscan)) != usehitscan)   usehitscan  = t,    r = 1;
    hasignoreflags = (nodudes | noplayers | nothings | nomasks | nodead);

    t = ((pOwn->cstat & kSprRelMask) == kSprFloor);
    if (t != vertical)
        vertical = t, r = 1;

    if (vertical)
    {
        t = ((pOwn->cstat & kSprOneSided) != 0 && (pOwn->cstat & kSprFlipY) == 0);
        if (t != direction)
            direction = t, r = 1;
    }

    if (pXOwn->data2 != thickness && irngok(pXOwn->data2, 1, 255))
    {
        (vertical) ? xrepeat = pXOwn->data2 : yrepeat = pXOwn->data2;
        thickness = pXOwn->data2;
        r = 1;
    }

    if (pXOwn->data3 != maxdist)
        maxdist = pXOwn->data3, r = 1;

    return r;
}

void LASER::ScanV(LASERSCAN* l)
{
    HITINFO* h = &gHitInfo;
    int nDist = DIST2PIX(maxdist)<<4, nDist2 = 0;
    int t, cz, fz, ch, fh;
    int z = l->srcz;

    // Use getzrange because hitscan stuff
    // doesn't really hit sprites when
    // slope argument passed as
    // vertical

    while( 1 )
    {
        getzrange(l->srcx, l->srcy, z, l->srcs, &cz, &ch, &fz, &fh, 4, BLOCK_HITSCAN);
        l->hitz = (direction) ? cz : fz;
        h->hitsprite = -1;

        if (maxdist)
        {
            nDist2 += klabs(l->srcz-l->hitz);

            if (nDist2 > nDist)
            {
                l->hitz = l->srcz + ((direction) ? -nDist : nDist);
                break;
            }
        }

        if (direction)
        {
            if (ch >= 0 && (ch & 0xC000) == 0xC000)
            {
                t = ch & 0x3FFF;
                GetSpriteExtents(&sprite[t], &cz, &fz);
                if (l->hitz >= cz)
                    h->hitsprite = t;
            }
        }
        else if (fh >= 0 && (fh & 0xC000) == 0xC000)
        {
            t = fh & 0x3FFF;
            GetSpriteExtents(&sprite[t], &cz, &fz);
            if (l->hitz <= fz)
                h->hitsprite = t;
        }

        if (h->hitsprite >= 0
            && hasignoreflags && !SpriteAllowed(&sprite[h->hitsprite]))
            {
                t = klabs(fz-cz);
                if (direction)
                    t = -t;

                z += t;
                continue;
            }

        break;
    }

    l->hitsprite    = h->hitsprite;
    l->hitx         = l->srcx;
    l->hity         = l->srcy;
}

void LASER::ScanH(LASERSCAN* l)
{
    HITINFO* h = &gHitInfo;
    int s, bs, a, ba, x, bx, y, by;
    int nDist, nCode;
    spritetype* pSpr;

    if (hasignoreflags)
    {
        nDist = DIST2PIX(maxdist)>>4;
        pSpr = pOwn;

        s = pSpr->sectnum;
        a = pOwn->ang;
        x = l->srcx;
        y = l->srcy;

        while(s >= 0)
        {
            bs = pSpr->sectnum, pSpr->sectnum   = s;
            ba = pSpr->ang,     pSpr->ang       = a;
            bx = pSpr->x,       pSpr->x         = x;
            by = pSpr->y,       pSpr->y         = y;

            offsetPos(0, 4, 0, l->srca, &pSpr->x, &pSpr->y, NULL);

            if (usehitscan)
            {
                nCode = HitScan(pSpr, pSpr->z, Cos(l->srca) >> 16, Sin(l->srca) >> 16, 0, CLIPMASK1, 0);
            }
            else
            {
                nCode = VectorScan(pSpr, 0, 0, Cos(l->srca) >> 16, Sin(l->srca) >> 16, 0, 0, 0x01);
            }

            pSpr->sectnum   = bs;
            pSpr->ang       = ba;
            pSpr->x         = bx;
            pSpr->y         = by;

            if (maxdist
                && (nDist -= (Distance(h->hitx - x, h->hity - y)>>4)))
                {
                    // Somehow, i think there is something wrong
                    // with masked walls and max distance
                    // argument, so we clamp it
                    // manually

                    h->hitx = l->srcx; h->hity = l->srcy;
                    offsetPos(0, DIST2PIX(maxdist), 0, l->srca, &h->hitx, &h->hity, NULL);
                    h->hitsprite = -1;
                    break;
                }

            x = h->hitx;
            y = h->hity;

            if (nCode == OBJ_MASKED && nomasks)
            {
                s = wall[h->hitwall].nextsector;
                offsetPos(0, 2, 0, l->srca, &x, &y, NULL);
                continue;
            }

            if (h->hitsprite >= 0)
            {
                pSpr = &sprite[h->hitsprite];
                if (SpriteAllowed(pSpr))
                    break;

                h->hitsprite = -1;
                s = pSpr->sectnum;
                offsetPos(0, 2, 0, l->srca, &x, &y, NULL);
                continue;
            }

            break;
        }
    }
    else
    {
        offsetPos(0, 4, 0, l->srca, &pOwn->x, &pOwn->y, NULL);

        if (usehitscan)
        {
            HitScan(pOwn, pOwn->z, Cos(l->srca) >> 16, Sin(l->srca) >> 16, 0, CLIPMASK1, DIST2PIX(maxdist) >> 4);
        }
        else
        {
            VectorScan(pOwn, 0, 0, Cos(l->srca) >> 16, Sin(l->srca) >> 16, 0, DIST2PIX(maxdist) >> 4, 0x01);
        }

        pOwn->x = l->srcx; pOwn->y = l->srcy;
    }

    l->hitsprite    = h->hitsprite;
    l->hitx         = h->hitx;
    l->hity         = h->hity;
    l->hitz         = h->hitz;
}

void LASER::Process()
{
    static POSOFFS pos;
    LASERSCAN *o = &oldscan, *n = &newscan;
    int nOSpr = o->hitsprite;
    int nHSpr, nCmd;

    n->srcs = pOwn->sectnum;
    n->srca = pOwn->ang;
    n->srcx = pOwn->x;
    n->srcy = pOwn->y;
    n->srcz = pOwn->z;
    n->srch = 0;

    if (CheckFlags(kFlagLaserDynamicSets) && UpdateProperties())
        raydone = 0;

    if (vertical)
    {
        ScanV(n);
    }
    else
    {
        ScanH(n);
    }

    nHSpr = n->hitsprite;

    if (!pXOwn->locked && !pXOwn->isTriggered)
    {
        if (pXOwn->triggerOn)
        {
            // trigger at hit
            if (nHSpr >= 0 && (!troncehit || nHSpr != nOSpr))
            {
                nCmd = (strictcmd)
                        ? kCmdOn : pXOwn->command;

                if (pXOwn->txID)
                {
                    evSend(pOwn->index, EVOBJ_SPRITE, pXOwn->txID, (COMMAND_ID)nCmd, nHSpr);
                }
                else
                {
                    nnExtTriggerObject(EVOBJ_SPRITE, nHSpr, nCmd);
                }
            }
        }

        // trigger at miss
        if (troncelast && nOSpr >= 0 && nHSpr != nOSpr)
        {
            nCmd = (strictcmd)
                        ? kCmdOff : pXOwn->command;

            if (pXOwn->txID)
            {
                evSend(pOwn->index, EVOBJ_SPRITE, pXOwn->txID, (COMMAND_ID)nCmd, nOSpr);
            }
            else
            {
                nnExtTriggerObject(EVOBJ_SPRITE, nOSpr, nCmd);
            }
        }
    }

    o->hitsprite = nHSpr;

    if (!raydone || AreScansDifferent())
    {
        pos.New(n->srca, n->srcx, n->srcy, n->srcz);
        memcpy(o, n, sizeof(LASERSCAN));
        RayDelete();
        raydone = 1;

        if (vertical)
        {
            RayCollectPiecesV(n->srcs, klabs(n->hitz - n->srcz), &pos);
        }
        else
        {
            RayCollectPiecesH(n->srcs, Distance(n->srcx - n->hitx, n->srcy - n->hity), &pos);
        }
    }
}

char LASER::RayShow(SCREEN2D* pScr)
{
    if (!numpieces)
        return 0;

    char c1 = pScr->ColorGet(kColorYellow);
    char c2 = pScr->ColorGet(kColorWhite);
    char c3 = pScr->ColorGet(kColorGrey28);

    int x1, y1, x2, y2, sz;
    int i = numpieces;
    RAYPIECE* pPiece;

    x1 = pOwn->x;
    y1 = pOwn->y;

    if (!vertical)
    {
        for (i = 0; i < numpieces; i++)
        {
            pPiece = &pieces[i];
            sz = size[0][0][pPiece->w]>>1;
            x2 = pPiece->x, y2 = pPiece->y;

            offsetPos(0, sz, 0, pOwn->ang, &x2, &y2, NULL);

            pScr->ScalePoints(&x1, &y1, &x2, &y2);
            pScr->DrawLine(x1, y1, x2, y2, c3, 1, (pScr->HOVER && h) ? kPatNormal : kPatDotted);


            //pScr->DrawIconCross(x2, y2, c2, 2);

            x1 = x2 = pPiece->x;
            y1 = y2 = pPiece->y;

            pScr->ScalePoints(&x2, &y2);
            pScr->DrawVertex(x2, y2, c1, c1, 4);
        }
    }
    else
    {
        pScr->ScalePoints(&x1, &y1);
        pScr->DrawVertex(x1, y1, c1, c1, 4);
    }


    return 1;
}

char LASER::RayShow(int nSect)
{
    enum
    {
        kRetnSTOP   = 0,
        kRetnSKIP   = 1,
        kRetnOKAY   = 2,
    };

    if (!TestBitString(piecesectmap, nSect))
        return kRetnSKIP;

    int nAng, xAng, yAng;
    int i = numpieces;
    RAYPIECE* pPiece;

    while(--i >= 0)
    {
        pPiece = &pieces[i];
        if (pPiece->s == nSect)
        {
            nAng = getangle(pPiece->x-cam.x, pPiece->y-cam.y);
            yAng = DANGLE(nAng, pOwn->ang + kAng90);
            xAng = DANGLE(nAng, pOwn->ang);

            if (vertical)
            {
                if (!PiecePutV(pPiece, xAng, yAng))
                    return kRetnSTOP;
            }
            else
            {
                if (!PiecePutH(pPiece, xAng, yAng))
                    return kRetnSTOP;
            }

        }
    }

    return kRetnOKAY;
}

char LASER::SpriteAllowed(spritetype* pSpr)
{
    const int n = pSpr->inittype;

    if (nodead)
    {
        if ((pSpr->extra <= 0 || !xsprite[pSpr->extra].health)
            || (rngok(n, kDudeBase, kDudeMax) && pSpr->statnum == kStatThing))
                return 0; // could be dude turned in gib
    }

    if (nodudes && rngok(n, kDudeBase, kDudeMax) && !irngok(n, kDudePlayer1, kDudePlayer8))     return 0;
    else if (noplayers && irngok(n, kDudePlayer1, kDudePlayer8))                                return 0;
    else if (nothings && pSpr->statnum == kStatThing)                                           return 0;
    else                                                                                        return 1;
}

int LASER::LookupRepeat(unsigned int* pSiz, int nDist, unsigned char nRep)
{
    while(nRep > 1 && pSiz[nRep--] > nDist);
    return nRep;
}

#if 0
char LASER::PieceBehindCam(RAYPIECE* pPiece)
{
    #define kPeriphery kAng90

    int sz = size[0][0][pPiece->w]>>1;
    int x, y;

    x = pPiece->x; y = pPiece->y;
    offsetPos(0, +sz, 0, pOwn->ang, &x, &y, NULL);
    if (klabs(DANGLE(getangle(x-cam.x, y-cam.y), cam.a)) < kPeriphery)
        return 0;

    x = pPiece->x; y = pPiece->y;
    offsetPos(0, -sz, 0, pOwn->ang, &x, &y, NULL);
    if (klabs(DANGLE(getangle(x-cam.x, y-cam.y), cam.a)) <= kPeriphery)
        return 0;

    return 1;
}
#endif

signed char LASER::colors[kLaserMaxColors][3] =
{
    { kColorLightRed,       -8,     48 },       // 0
    { kColorLightGreen,     -8,     48 },       // 1
    { kColorLightBlue,      -8,     48 },       // 2
    { kColorYellow,         -8,     48 },       // 3
    { kColorLightGray,      -8,     48 },       // 4
    { kColorWhite,          -8,     48 },       // 5
    { kColorRed,            -8,     48 },       // 6
    { kColorGreen,          -8,     48 },       // 7
    { kColorBrown,          -8,     48 },       // 8
    { kColorDarkGray,       -8,     48 },       // 9
};

LASER::CAMERA LASER::cam;
unsigned int LASER::tile[2][kLaserMaxColors];
unsigned int LASER::size[2][3][256];
