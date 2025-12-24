#include "xmpmaped.h"
#include "aadjust.h"
#include "preview.h"
#include "xmparted.h"
#include "tile.h"

char* const gSectToolNames[] =
{
    "Loop shape",
    "Loop split",
    "Door wizard",
    "Arc wizard",
    "Curve wall",
    "Unknown",
};

void avePointLoop(int s, int e, int *x, int *y)
{
    int dax = 0, day = 0, i;
    for(i = s; i <= e; i++)
    {
        dax += wall[i].x;
        day += wall[i].y;
    }

    if (e > s)
    {
        dax /= (e-s+1);
        day /= (e-s+1);
    }

    *x = dax;
    *y = day;
}

void midPointLoop(int nFirst, int nLast, int* ax, int *ay)
{
    int x1, y1, x2, y2;
    loopGetBox(nFirst, nLast, &x1, &y1, &x2, &y2);

    *ax = x1 + ((x2 - x1)>>1);
    *ay = y1 + ((y2 - y1)>>1);
}

void avePointSector(int nSector, int *x, int *y)
{
    int s, e;
    getSectorWalls(nSector, &s, &e);
    avePointLoop(s, e, x, y);
}

void avePointWall(int nWall, int* x, int* y, int* z)
{
    int nSect = sectorofwall(nWall);
    avePointWall(nWall, x, y);

    *z = 0;
    if (nSect >= 0)
    {
        int cz, fz;
        getzsofslope(nSect, *x, *y, &cz, &fz);
        *z = fz - klabs((cz-fz)>>1);
    }
}

void avePointWall(int nWall, int* x, int* y)
{
    *x = ((wall[nWall].x + wall[wall[nWall].point2].x) >> 1);
    *y = ((wall[nWall].y + wall[wall[nWall].point2].y) >> 1);
}



void loopGetWalls(int nWall, int* s, int *e)
{
    int i;
    *s = *e = nWall;
    
    while( 1 )
    {
        if ((i = wall[*e].point2) <= *e)
        {
            *s = i;
            break;
        }
        
        *e = i;
    }

}

void loopGetEdgeWalls(int nFirst, int nLast, int* l, int* r, int* t, int* b)
{
    *l = *r = *t = *b = nFirst;

    while(nFirst <= nLast)
    {
        if (wall[nFirst].x < wall[*l].x) *l = nFirst;
        if (wall[nFirst].x > wall[*r].x) *r = nFirst;
        if (wall[nFirst].y < wall[*t].y) *t = nFirst;
        if (wall[nFirst].y > wall[*b].y) *b = nFirst;
        nFirst++;
    }
}

void loopGetBox(int nFirst, int nLast, int* x1, int* y1, int* x2, int *y2)
{
    int l, r, t, b;
    loopGetEdgeWalls(nFirst, nLast, &l, &r, &t, &b);

    *x1 = wall[l].x;    *x2 = *x1 + (wall[r].x - *x1);
    *y1 = wall[t].y;    *y2 = *y1 + (wall[b].y - *y1);

    if (*x1 > *x2)
        swapValues(x1, x2);

    if (*y1 > *y2)
        swapValues(y1, y2);
}

void loopChgPos(int s, int e, int bx, int by, int flags)
{
    while(s <= e)
        posChg(&wall[s].x, &wall[s].y, bx, by, flags), s++;
}

void loopRotate(int s, int e, int cx, int cy, int nAng, int flags)
{
    while(s <= e)
        posRotate(&wall[s].x, &wall[s].y, nAng, cx, cy, flags), s++;
}

void loopFlip(int s, int e, int cx, int cy, int flags)
{
    // based on eduke32
    ////////////////////////////////////

    int i = numwalls, j, t, x1, y1, x2, y2, wA, wB;
    int nSLoop, nELoop;
    walltype buf;

    // save position of wall at start of loop
    getWallCoords(s, &x1, &y1, NULL, NULL);
    nSLoop = s; nELoop = e;

    // flip walls
    for (i = s; i <= e; i++)
    {
        getWallCoords(i, NULL, NULL, &x2, &y2);
        if (flags & 0x01)
        {
            wall[i].x = (cx - x2) + cx;
            wall[i].y = y2;
        }
        else
        {
            wall[i].x = x2;
            wall[i].y = (cy - y2) + cy;
        }

        if (wall[i].point2 == nSLoop)
        {
            nELoop = i;
            if (flags & 0x01)
            {
                wall[nELoop].x = (cx - x1) + cx;
                wall[nELoop].y = y1;
            }
            else
            {
                wall[nELoop].x = x1;
                wall[nELoop].y = (cy - y1) + cy;
            }

            t = (nELoop - nSLoop)>>1;
            for (j = 1; j <= t; j++)
            {
                wA = nSLoop + j;
                wB = nELoop - j + 1;

                memcpy(&buf, &wall[wA], sizeof(walltype));
                memcpy(&wall[wA], &wall[wB], sizeof(walltype));
                memcpy(&wall[wB], &buf, sizeof(walltype));
            }

            // make point2 point to next wall in loop
            for (j = nSLoop; j < nELoop; j++) wall[j].point2 = j + 1;
            wall[nELoop].point2 = nSLoop;
            nSLoop = nELoop + 1;

            // save position of wall at start of loop
            getWallCoords(nSLoop, &x1, &y1, NULL, NULL);
        }
    }
}

void loopDelete(int nWall)
{
    int x1, y1, x2, y2, x3, y3;
    int i;

    getWallCoords(nWall, &x3, &y3);
    i = nWall;
    do
    {
        wall[i].x = x3, wall[i].y = y3;
        i = wall[i].point2;
    }
    while(i != nWall);

    i = numwalls;
    while(--i >= 0)
    {
        getWallCoords(i, &x1, &y1, &x2, &y2);
        if ((x1 == x2 && y1 == y2) && ((x3 == x1 && y3 == y1) || (x3 == x2 && y3 == y2)))
            deletePoint(i); // this will delete sector when wallnum == 1
    }
}

char loopLocalIsEmpty(int nWall)
{
    int i, s, e, ls, le;
    int nSect;

    if (wall[nWall].nextwall >= 0)
        return 0;

    nSect = sectorofwall(nWall);
    getSectorWalls(nSect, &s, &e);
    loopGetWalls(nWall, &ls, &le);

    if (ls == s && le == e)
        return 0;

    for (i = ls; i <= le; i++)
    {
        if (wall[i].nextwall >= 0)
            return 0;
    }

    return 1;
}

int loopIsEmpty(int nWall)
{
    IDLIST* pList = new IDLIST(1);
    int ix, iy, x1, y1, x2, y2; int32_t* p;
    int n = numwalls;
    char stop = 0;

    getWallCoords(nWall, &ix, &iy);
    x1 = ix, y1 = iy;

    do
    {
        pList = new IDLIST(1);
        collectWallsOfNode(pList, nWall);
        for (p = pList->GetPtr(); *p >= 0; p++)
        {
            getWallCoords(*p, NULL, NULL, &x2, &y2);
            if (*p == nWall || x2 != x1 || y2 != y1)
                continue;

            nWall = *p;
            if (wall[nWall].nextwall < 0)
            {
                memcpy(&wall[n], &wall[nWall], sizeof(walltype));

                wall[n].point2 = ++n;
                if (wall[n].x == ix && wall[n].y == iy)
                    wall[n].point2 = numwalls, stop = 1;
            }
            else
            {
                stop = 2;
            }

            break;
        }

        delete(pList);
    }
    while(!stop && *p >=0);

    if (stop == 1
        && clockdir(numwalls) == 1)
            return n - numwalls;

    return 0;
}

int loopTransfer(int nWall, int nSect, char flags)
{
    VOIDLIST wl(sizeof(walltype));
    int s = nWall;
    int i, j;

    do
    {
        wl.Add(&wall[s]);
        s = wall[s].point2;
    }
    while(s != nWall);

    if (flags & 0x01)
        loopDelete(nWall);

    if ((i = insertLoop(nSect, (walltype*)wl.First(), wl.Length(), NULL)) >= 0)
    {
        if (flags & 0x02)
        {
            s = i;
            do
            {
                if ((j = findNextWall(s)) >= 0)
                {
                    wallAttach(s, sectorofwall(j), j);
                    wallAttach(j, nSect, s);
                }

                s = wall[s].point2;
            }
            while(s != i);
        }
    }

    return i;
}

void getSectorWalls(int nSect, int* s, int *e)
{
    *s = sector[nSect].wallptr;
    *e = *s + sector[nSect].wallnum - 1;
}

void sectGetEdgeZ(int nSector, int* fz, int* cz)
{
    int s, e, x, y, z;
    getSectorWalls(nSector, &s, &e);

    *fz = sector[nSector].floorz;
    *cz = sector[nSector].ceilingz;
    while(s <= e)
    {
        getWallCoords(s++, &x, &y);
        if (fz && (z = getflorzofslope(nSector, x, y)) > *fz)
            *fz = z;

        if (cz && (z = getceilzofslope(nSector, x, y)) < *cz)
            *cz = z;
    }
}

void sectRotate(int nSect, int cx, int cy, int nAng, int flags)
{
    char rPoint = (klabs(nAng) != kAng90);
    int i, s, e;

    // rotate walls
    getSectorWalls(nSect, &s, &e);
    loopRotate(s, e, cx, cy, nAng, rPoint); // inside loop
    if ((flags & 0x02) && wall[s].nextwall >= 0)
    {
        // outer loop
        loopGetWalls(wall[s].nextwall, &s, &e);
        loopRotate(s, e, cx, cy, nAng, rPoint);
    }

    // rotate sprites
    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        spritetype* pSpr = &sprite[i];

        posRotate(&pSpr->x, &pSpr->y, nAng, cx, cy, rPoint);
        if (pSpr->statnum != kStatMarker && pSpr->statnum != kStatPathMarker)
            pSpr->ang = (pSpr->ang + nAng) & kAngMask;
    }
}

void sectFlip(int nSect, int cx, int cy, int flags, int)
{
    char flipX = ((flags & 0x01) > 0);
    int i, t, s, e;

    // flip walls
    getSectorWalls(nSect, &s, &e);
    loopFlip(s, e, cx, cy, flags); // inside loop
    if ((flags & 0x02) && wall[s].nextwall >= 0)
    {
        // outer loop
        loopGetWalls(wall[s].nextwall, &s, &e);
        loopFlip(s, e, cx, cy, flags);
    }

    // fix nextwalls?
    for (i = s; i <= e; i++)
    {
        if (wall[i].nextwall >= 0
            && (t = findNextWall(i, 1)) >= 0)
            {
                // forces sectorofwall() to
                // actually *search* the
                // sector...

                wall[i].nextwall = -1;
                wall[t].nextwall = -1;

                wallAttach(i, sectorofwall(t), t);
                wallAttach(t, sectorofwall(i), i);
            }
    }

    // flip sprites
    for (i = headspritesect[nSect]; i >= 0; i = nextspritesect[i])
    {
        spritetype* pSpr = &sprite[i];
        posFlip(&pSpr->x, &pSpr->y, cx, cy, flipX);
        if (pSpr->statnum != kStatMarker && pSpr->statnum != kStatPathMarker)
        {
            if (flipX)
                pSpr->ang = (kAng180 + (kAng360 - pSpr->ang)) & kAngMask;
            else
                pSpr->ang = (kAng360 - pSpr->ang) & kAngMask;
        }
    }

    if (flipX)
    {
        sector[nSect].ceilingstat ^= kSectFlipX;
        sector[nSect].floorstat ^= kSectFlipX;
    }
    else
    {
        sector[nSect].ceilingstat ^= kSectFlipY;
        sector[nSect].floorstat ^= kSectFlipY;
    }
}

int sectSplit(int nSect, int nWallA, int nWallB, POINT2D* points, int nPoints)
{
    int nSectOld, nLoopNumA, nLoopNumB;
    int nWall, nWallOther, nWallNum;
    int i, j, k, f, s, e, ls, le;
    int x1, y1, x2, y2;
    int x3, y3, x4, y4;
    int nRetn = 0;

    if (nWallA < 0 || nWallB < 0)
        return -1;

    if (nWallA == nWallB)
        return -2;
    
    nSectOld = -1;
    
#if 0
    if (sectCountParts(nSect) > 1)
    {
        // This is a multipart sector, so we have to
        // find the part we are trying to split and
        // turn it into new standalone sector.
        //
        // Unlike normal loops, the "part"
        // must have clockwise walls
        // direction.
        
        avePointWall(nWallA, &x1, &y1); avePointWall(nWallB, &x2, &y2);
        offsetPos(0, 8, 0, (GetWallAngle(nWallA) + kAng90) & kAngMask, &x1, &y1, NULL);
        offsetPos(0, 8, 0, (GetWallAngle(nWallB) + kAng90) & kAngMask, &x2, &y2, NULL);
        getSectorWalls(nSect, &s, &e);
        
        while(s <= e)
        {
            loopGetWalls(s, &ls, &le);
            if (clockdir(ls) == 0 && loopInside(x1, y1, ls) && loopInside(x2, y2, ls))
            {
                if (loopnumofsector(nSect, nWallA) == loopnumofsector(nSect, nWallB))
                {
                    if (numsectors + 2 >= kMaxSectors)
                        return -4;
                    
                    sectortype model = sector[nSect];
                    VOIDLIST wls(sizeof(walltype));
                    
                    getWallCoords(nWallA, &x1, &y1, &x2, &y2);
                    getWallCoords(nWallB, &x3, &y3, &x4, &y4);
                    
                    for (i = ls; i <= le; i++) wls.Add(&wall[i]);
                    
                    // Remove the part loop and re-insert it as a
                    // new sector, then move any normal loops
                    // that was inside of an old sector
                    // to new.
                    
                    loopDelete(ls);
                    i = insertLoop(-1, (walltype*)wls.First(), wls.Length(), &model);
                    i = sectorofwall(i); sectLoopTransfer(nSect, i);
                    
                    // Refresh info and continue split.
                    
                    nSectOld    = nSect, nSect = i;
                    nWallA      = findWallAtPos(nSect, x1, y1, x2, y2);
                    nWallB      = findWallAtPos(nSect, x3, y3, x4, y4);
                }
                
                break;
            }
            
            s = ++le;
        }
        
        // If no parts found, don't allow to
        // split it to avoid walls
        // corruption.
        
        if (s > e)
            return -5;
    }
#endif

    nLoopNumA   = loopnumofsector(nSect, nWallA);
    nLoopNumB   = loopnumofsector(nSect, nWallB);
    nWall       = nWallNum = numwalls+nPoints-1; // where to add more walls

    if (nLoopNumA == nLoopNumB)
    {
        if (numwalls + (nPoints<<1) >= kMaxWalls)   return -3;
        else if (numsectors + 1 >= kMaxSectors)     return -4;
    }
    else if (numwalls + nPoints >= kMaxWalls)   return -3;
    else if (numsectors >= kMaxSectors)         return -4;


    getSectorWalls(nSect, &s, &e);

    /* first fix up the new walls
    ---------------------------------------------*/
    for (i = numwalls, j = 0; i <= nWallNum; i++, j++)
    {
        wall[i].x           = points[j].x;
        wall[i].y           = points[j].y;

        wall[i].cstat       = wall[s].cstat;
        wall[i].shade       = wall[s].shade;
        wall[i].xrepeat     = wall[s].xrepeat;
        wall[i].yrepeat     = wall[s].yrepeat;
        wall[i].picnum      = wall[s].picnum;
        wall[i].overpicnum  = wall[s].overpicnum;
        wall[i].point2      = i+1;
        wallDetach(i);
    }

    for (i = numwalls; i <= nWallNum; i++)
        fixrepeats(i);

    if (nLoopNumA == nLoopNumB)
    {
        /** NORMAL SPLIT
        //////////////////////////////////*/


        /* copy rest of loop next
        ---------------------------------------------*/
        for (i = nWallB; i != nWallA; i = wall[i].point2)
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            nWall++;
        }
        wall[nWall-1].point2 = numwalls;

        /* add other loops for 1st sector
        ---------------------------------------------*/

        for (i = nLoopNumA, j = s; j <= e; j++)
        {
            if ((k = loopnumofsector(nSect, j)) != i && k != nLoopNumA)
            {
                i = k;
                if (insideLoop(wall[j].x, wall[j].y, numwalls))
                {
                    k = nWall;
                    f = j;
                    do
                    {
                        memcpy(&wall[nWall], &wall[f], sizeof(walltype));
                        wall[nWall].point2 = nWall+1;
                        f = wall[f].point2;
                        nWall++;
                    }
                    while(f != j);
                    wall[nWall-1].point2 = k;
                }
            }
        }

        nWallOther = nWall;

        /* copy split points for other sector backwards
        ---------------------------------------------*/
        for(i = nWallNum; i > numwalls; i--)
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            wallDetach(nWall);
            nWall++;
        }

        /* copy rest of loop next
        ---------------------------------------------*/
        for (i = nWallA; i != nWallB; i = wall[i].point2)
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            nWall++;
        }
        wall[nWall-1].point2 = nWallOther;

        /* add other loops for 2nd sector
        ---------------------------------------------*/
        for(i = nLoopNumA, j = s; j <= e; j++)
        {
            if ((k = loopnumofsector(nSect, j)) != i && k != nLoopNumA)
            {
                i = k;
                if (insideLoop(wall[j].x, wall[j].y, nWallOther))
                {
                    f = j;
                    k = nWall;
                    do
                    {
                        memcpy(&wall[nWall], &wall[f], sizeof(walltype));
                        wall[nWall].point2 = nWall+1;
                        f = wall[f].point2;
                        nWall++;
                    }
                    while(f != j);
                    wall[nWall-1].point2 = k;
                }
            }
        }

        /* fix all next pointers on old sector line
        ---------------------------------------------*/
        for (i = numwalls; i < nWall; i++)
        {
            if (wall[i].nextwall >= 0)
            {
                j = (i < nWallOther);
                wall[wall[i].nextwall].nextwall = i;
                wall[wall[i].nextwall].nextsector = numsectors+j;
            }
        }

        /* set all next pointers on split
        ---------------------------------------------*/
        for(i = numwalls; i < nWallNum; i++)
        {
            j = nWallOther + (nWallNum - 1 - i);
            wall[i].nextwall = j;
            wall[i].nextsector = numsectors+1;
            wall[j].nextwall = i;
            wall[j].nextsector = numsectors;
        }

        /* copy sector attributes & fix wall pointers
        ---------------------------------------------*/
        sector[nSect].alignto = 0;

        memcpy(&sector[numsectors], &sector[nSect], sizeof(sectortype));
        sector[numsectors+0].wallnum = nWallOther - numwalls;
        sector[numsectors+0].wallptr = numwalls;

        memcpy(&sector[numsectors+1], &sector[nSect], sizeof(sectortype));
        sector[numsectors+1].wallnum = nWall - nWallOther;
        sector[numsectors+1].wallptr = nWallOther;


        /* fix sector of sprites
        ---------------------------------------------*/
        while((i = headspritesect[nSect]) >= 0)
        {
            if (insideLoop(sprite[i].x, sprite[i].y, numwalls) == 0)
            {
                ChangeSpriteSect(i, numsectors+1);
            }
            else
            {
                ChangeSpriteSect(i, numsectors);
            }
        }

        j = nWall - numwalls;
        numwalls = nWall;
        numsectors += 2;

        /* detach old walls for safe deletesector
        ---------------------------------------------*/
        for (i = s; i <= e; i++)
        {
            if (wall[i].nextwall >= 0)
                wallDetach(wall[i].nextwall);

            wallDetach(i);
        }
        deletesector(nSect);

        /* check pointers
        ---------------------------------------------*/
        for(i = numwalls - j; i < numwalls; i++)
        {
            if (wall[i].nextwall >= 0)
                checksectorpointer(wall[i].nextwall, wall[i].nextsector);

            checksectorpointer(i, sectorofwall(i));
        }

        nRetn = 1;
    }
    else
    {
        /** LOOP JOINING
        //////////////////////////////////*/


        /* copy rest of loop next
        ---------------------------------------------*/
        i = nWallB;
        do
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            i = wall[i].point2;
            nWall++;
        }
        while(i != nWallB);

        /* copy split points for other sector backwards
        ---------------------------------------------*/
        for(i = nWallNum; i > numwalls; i--)
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            wallDetach(nWall);
            nWall++;
        }

        /* copy rest of loop next
        ---------------------------------------------*/
        i = nWallA;
        do
        {
            memcpy(&wall[nWall], &wall[i], sizeof(walltype));
            wall[nWall].point2 = nWall+1;
            i = wall[i].point2;
            nWall++;
        }
        while(i != nWallA);
        wall[nWall-1].point2 = numwalls;


        /* add other loops for sector
        ---------------------------------------------*/
        for (i = nLoopNumA, j = s; j <= e; j++)
        {
            if ((k = loopnumofsector(nSect, j)) == i) continue;
            else if (k == nLoopNumA || k == nLoopNumB)
                continue;

            i = k;
            f = j; k = nWall; //copy loop
            do
            {
                memcpy(&wall[nWall], &wall[f], sizeof(walltype));
                wall[nWall].point2 = nWall+1;
                f = wall[f].point2;
                nWall++;
            }
            while(f != j);
            wall[nWall-1].point2 = k;
        }

        /* fix all next pointers on old sector line
        ---------------------------------------------*/
        for (i = numwalls; i < nWall; i++)
        {
            if (wall[i].nextwall >= 0)
            {
                wall[wall[i].nextwall].nextwall = i;
                wall[wall[i].nextwall].nextsector = numsectors;
            }
        }

        /* copy sector attributes & fix wall pointers
        ---------------------------------------------*/
        sector[numsectors] = sector[nSect];
        sector[numsectors].wallptr = numwalls;
        sector[numsectors].wallnum = nWall - numwalls;


        /* fix sector of sprites
        ---------------------------------------------*/
        while((i = headspritesect[nSect]) >= 0)
            ChangeSpriteSect(i, numsectors);

        j = nWall - numwalls;
        numwalls = nWall;
        numsectors++;

        /* detach old walls for safe deletesector
        ---------------------------------------------*/
        for (i = s; i <= e; i++)
        {
            if (wall[i].nextwall >= 0)
                wallDetach(wall[i].nextwall);

            wallDetach(i);
        }
        deletesector(nSect);

        /* check pointers
        ---------------------------------------------*/
        for(i = numwalls - j; i < numwalls; i++)
        {
            if (wall[i].nextwall >= 0)
                checksectorpointer(wall[i].nextwall, wall[i].nextsector);

            checksectorpointer(i, numsectors-1);
        }

        nRetn = 2;
    }

    if (nRetn > 0)
    {
        for(i = numwalls - j; i < numwalls; i++)
        {
            if(!wallVisible(i))
            {
                fixrepeats(i);
                if (wall[i].nextwall >= 0 && !wallVisible(wall[i].nextwall))
                    fixrepeats(wall[i].nextwall);
            }
        }
        
        //if (nSectOld >= 0)
            //redSectorMerge(numsectors-3, numsectors-2);
    }

    return nRetn;
}

int sectSplit(int nSect, POINT2D* points, int nPoints)
{
    POINT2D *first = &points[0], *last = &points[nPoints-1];
    int nWallA = -1, nWallB = -1;
    int i, s, e;

    getSectorWalls(nSect, &s, &e);
    for (i = s; i <= e; i++)
    {
        if (nWallA < 0)
        {
            if (wall[i].x == first->x && wall[i].y == first->y)
                nWallA = i;
        }

        if (nWallB < 0)
        {
            if (wall[i].x == last->x && wall[i].y == last->y)
                nWallB = i;
        }

        if (nWallA >= 0 && nWallB >= 0)
            break;
    }

    return sectSplit(nSect, nWallA, nWallB, points, nPoints);
}

int sectCloneMarker(XSECTOR* pXOwner, spritetype* pWich)
{
    int nSpr; spritetype* pSpr;
    if ((nSpr = InsertSprite(pWich->sectnum, kStatMarker)) < 0)
        return -1;

    pSpr = &sprite[nSpr];
    memcpy(pSpr, pWich, sizeof(spritetype));
    pSpr->owner = pXOwner->reference;
    pSpr->index = nSpr;
    pSpr->extra = -1;

    switch(pSpr->type)
    {
        case kMarkerWarpDest:
        case kMarkerAxis:
        case kMarkerOff:
            pXOwner->marker0 = nSpr;
            break;
        case kMarkerOn:
            pXOwner->marker1 = nSpr;
            break;
    }

    return nSpr;
}

char sectClone(int nSrc, int nDstS, int nDstW, int flags)
{
    int i, s, e, nWall = nDstW;
    XSECTOR* pXSect;

    // clone walls
    getSectorWalls(nSrc, &s, &e);
    for (i = s; i <= e; i++, nWall++)
    {
        wall[nWall] = wall[i];
        if (flags & 0x04)
            fixXWall(nWall);

        wall[nWall].point2 += nDstW-s;
        if (wall[nWall].nextwall >= 0)
        {
            wall[nWall].nextsector  += nDstS-nSrc;
            wall[nWall].nextwall    += nDstW-s;
        }
    }

    if (nWall > nDstW)
    {
        // clone sector
        sector[nDstS]           = sector[nSrc];
        sector[nDstS].wallnum   = nWall-nDstW;
        sector[nDstS].wallptr   = nDstW;

        if (sector[nDstS].extra > 0) // fix xsector
        {
            if (flags & 0x04)
                fixXSector(nDstS);

            pXSect = &xsector[sector[nDstS].extra];

            if (flags & 0x03) // fix marker(s)
            {
                if (pXSect->marker0 >= 0 && sprite[pXSect->marker0].statnum == kStatMarker)
                {
                    if ((i = sectCloneMarker(pXSect, &sprite[pXSect->marker0])) >= 0 && (flags & 0x02))
                    {
                        if (inside(sprite[pXSect->marker0].x, sprite[pXSect->marker0].y, nDstS))
                            ChangeSpriteSect(i, nDstS);
                    }
                }

                if (pXSect->marker1 >= 0 && sprite[pXSect->marker1].statnum == kStatMarker)
                {
                    if ((i = sectCloneMarker(pXSect, &sprite[pXSect->marker1])) >= 0 && (flags & 0x02))
                    {
                        if (inside(sprite[pXSect->marker1].x, sprite[pXSect->marker1].y, nDstS))
                            ChangeSpriteSect(i, nDstS);
                    }
                }
            }
        }

        if (flags & 0x01)
        {
            // clone sprites
            for (i = headspritesect[nSrc]; i >= 0; i = nextspritesect[i])
            {
                // ignore these in this loop (even if don't own it)
                if (sprite[i].statnum == kStatMarker)
                    continue;

                if ((s = InsertSprite(nDstS, sprite[i].statnum)) < 0)
                    break;

                sprite[s]           = sprite[i];
                sprite[s].sectnum   = nDstS;
                sprite[s].index     = s;

                if (flags & 0x04)
                    fixXSprite(s);
            }
        }
    }

    return 1;
}

short sectCstatAdd(int nSect, short cstat, int objType)
{
    if (objType == OBJ_FLOOR)
    {
        setCstat(TRUE, &sector[nSect].floorstat, cstat);
        return sector[nSect].floorstat;
    }
    else
    {
        setCstat(TRUE, &sector[nSect].ceilingstat, cstat);
        return sector[nSect].ceilingstat;
    }
}

short sectCstatRem(int nSect, short cstat, int objType)
{
    if (objType == OBJ_FLOOR)
    {
        setCstat(FALSE, &sector[nSect].floorstat, cstat);
        return sector[nSect].floorstat;
    }
    else
    {
        setCstat(FALSE, &sector[nSect].ceilingstat, cstat);
        return sector[nSect].ceilingstat;
    }
}

short sectCstatToggle(int nSect, short cstat, int objType) {

    if (objType == OBJ_FLOOR)
    {
        sector[nSect].floorstat ^= cstat;
        return sector[nSect].floorstat;
    }
    else
    {
        sector[nSect].ceilingstat ^= cstat;
        return sector[nSect].ceilingstat;
    }
}

short sectCstatGet(int nSect, int objType)
{
    if (objType == OBJ_FLOOR)
        return sector[nSect].floorstat;

    return sector[nSect].ceilingstat;

}

short sectCstatGet(int nSect, short cstat, int objType)
{
    if (objType == OBJ_FLOOR)
    {
        return sector[nSect].floorstat & cstat;
    }
    else
    {
        return sector[nSect].ceilingstat & cstat;
    }
}

short sectCstatSet(int nSect, short cstat, int objType)
{
    if (objType == OBJ_FLOOR)
    {
        sector[nSect].floorstat = cstat;
        return sector[nSect].floorstat;
    }
    else
    {
        sector[nSect].ceilingstat = cstat;
        return sector[nSect].ceilingstat;
    }

}

void sectDetach(int nSect)
{
    int s, e;
    getSectorWalls(nSect, &s, &e);

    while(s <= e)
    {
        if (wall[s].nextwall >= 0)
        {
            wallDetach(wall[s].nextwall);
            wallDetach(s);
        }

        s++;
    }
}

void sectAttach(int nSect)
{
    int s, e, n;
    getSectorWalls(nSect, &s, &e);

    while(s <= e)
    {
        if (wall[s].nextwall < 0 && (n = findNextWall(s)) >= 0)
        {
            wallAttach(s, sectorofwall(n), n);
            wallAttach(n, nSect, s);
        }

        s++;
    }
}

char sectAutoAlignSlope(int nSect, char which)
{
    sectortype* pSect = &sector[nSect];
    if (!pSect->alignto)
        return 0;

    int nWall = pSect->wallptr+pSect->alignto;
    int nSectNext = wall[nWall].nextsector;
    int x, y, x1, y1, x2, y2;
    if (nSectNext < 0)
        return 0;

    getWallCoords(nWall, &x1, &y1, &x2, &y2);
    x = (x1+x2)>>1;
    y = (y1+y2)>>1;

    if (which & 0x01)
        alignceilslope(nSect, x, y, getceilzofslope(nSectNext, x, y));

    if (which & 0x02)
        alignflorslope(nSect, x, y, getflorzofslope(nSectNext, x, y));

    return 1;
}

int sectCountParts(int nSect)
{
    int c = 0, s, e, ls, le;
    getSectorWalls(nSect, &s, &e);
        
    while(s <= e)
    {
        loopGetWalls(s, &ls, &le);
        if (clockdir(ls) == 0)
            c++;

        s = ++le;
    }
    
    return c;
}

char pointOnLine(int x, int y, int x1, int y1, int x2, int y2)
{
    if ((x1 <= x && x <= x2) || (x2 <= x && x <= x1))
        if ((y1 <= y && y <= y2) || (y2 <= y && y <= y1))
            if ((x-x1)*(y2-y1) == (y-y1)*(x2-x1))
                return 1;

    return 0;
}

char pointOnWallLine(int nWall, int x, int y)
{
    int x1, y1, x2, y2;
    getWallCoords(nWall, &x1, &y1, &x2, &y2);

    if (x1 == x && y1 == y) return 1;
    if (x2 == x && y2 == y) return 2;
    if (pointOnLine(x, y, x1, y1, x2, y2))
        return 3;

    return 0;
}

char pointOnWallLine(int x, int y, int* out)
{
    int i = numwalls;
    char r;

    while(--i >= 0)
    {
        if ((r = pointOnWallLine(i, x, y)) > 0)
        {
            if (out)
                *out = i;

            return r;
        }
    }

    return 0;
}

int findWallAtPos(int x, int y)
{
    int i = numwalls;
    while(--i >= 0 && (wall[i].x != x || wall[i].y != y));
    return i;
}

int findWallAtPos(int nSect, int x, int y)
{
    int s, e;
    getSectorWalls(nSect, &s, &e);

    while(s <= e)
    {
        if (wall[s].x == x && wall[s].y == y)
            return s;

        s++;
    }

    return -1;
}

int findSolidWallAtPos(int x, int y)
{
    int i = numwalls;
    while(--i >= 0 && (wall[i].nextwall >= 0 || wall[i].x != x || wall[i].y != y));
    return i;
}

int findWallAtPos(int x1, int y1, int x2, int y2)
{
    int tx1, ty1, tx2, ty2;
    int i = numwalls;

    while(--i >= 0)
    {
        getWallCoords(i, &tx1, &ty1, &tx2, &ty2);
        if (tx1 == x1 && ty1 == y1 && tx2 == x2 && ty2 == y2)
            return i;
    }

    return -1;
}

int findWallAtPos(int nSect, int x1, int y1, int x2, int y2)
{
    int s, e;
    int tx1, ty1, tx2, ty2;

    getSectorWalls(nSect, &s, &e);

    while(s <= e)
    {
        getWallCoords(s, &tx1, &ty1, &tx2, &ty2);
        if (tx1 == x1 && ty1 == y1 && tx2 == x2 && ty2 == y2)
            return s;

        s++;
    }

    return -1;
}

void insertPoint_OneSide(int nWall, int x, int y)
{
    int nSect;
    int i;

    nSect = sectorofwall(nWall);
    sector[nSect].wallnum++;
    for(i=nSect+1;i<numsectors;i++)
        sector[i].wallptr++;

    movewalls(nWall+1, 1);
    memcpy(&wall[nWall+1], &wall[nWall], sizeof(walltype));
    wall[nWall].point2 = nWall+1;
    wall[nWall+1].x = x;
    wall[nWall+1].y = y;

    fixrepeats(nWall);

    if ((wall[nWall].cstat & kWallFlipMask) == 0)
        AlignWalls(nWall, GetWallZPeg(nWall), wall[nWall].point2, GetWallZPeg(wall[nWall].point2), 1);

    fixrepeats(nWall+1);
    fixXWall(nWall+1);
}

int insertPoint(int nWall, int x, int y)
{
    int i, j;

    insertPoint_OneSide(nWall, x, y);
    if (wall[nWall].nextwall >= 0)
    {
        i = wall[nWall].nextwall;
        insertPoint_OneSide(i, x, y);
        j = wall[i].nextwall;

        wall[j+0].nextwall = i+1;
        wall[j+1].nextwall = i+0;
        wall[i+0].nextwall = j+1;
        wall[i+1].nextwall = j+0;

        return 2;
    }

    return 1;
}

void deletePoint(int nWall)
{
    int nSect, i;
    if ((nSect = sectorofwall(nWall)) >= 0)
    {
        if (sector[nSect].wallnum <= 2)
        {
            sectDelete(nSect);
            return;
        }

        sector[nSect].wallnum--;
        for(i=nSect+1;i<numsectors;i++)
            sector[i].wallptr--;

        wall[lastwall(nWall)].point2 = wall[nWall].point2;
        if (wall[nWall].nextwall >= 0)
            wallDetach(wall[nWall].nextwall);

        movewalls(nWall, -1);
    }
}

void wallDetach(int nWall)
{
    if (nWall >= 0)
    {
        wall[nWall].nextsector  = -1;
        wall[nWall].nextwall    = -1;
    }
}

void wallAttach(int nWall, int nNextS, int nNextW)
{
    if (nWall >= 0)
    {
        wall[nWall].nextsector  = nNextS;
        wall[nWall].nextwall    = nNextW;
    }
}

void wallAttach(int nWall, int nNext)
{
    wallAttach(nWall, wallGetSect(nNext), nNext);
    wallAttach(nNext, wallGetSect(nWall), nWall);
}

int wallGetSect(int nWall)
{
    int i, s, e;

    // Slower, but safier version
    // of sectorofwall() that
    // ignores nexsector
    // value

    if (rngok(nWall, 0, numwalls))
    {
        i = numsectors;
        while(--i >= 0)
        {
            getSectorWalls(i, &s, &e);
            if (irngok(nWall, s, e))
                return i;

        }
    }

    return -1;
}

double getWallLength(int nWall, int nGrid)
{
    int nLen = getWallLength(nWall);
    if (nGrid)
        return (double)(nLen / (2048>>nGrid));

    return (double)nLen;
}


void getWallCoords(int nWall, int* x1, int* y1, int* x2, int* y2)
{
    if (x1) *x1 = wall[nWall].x;
    if (y1) *y1 = wall[nWall].y;
    if (x2) *x2 = wall[wall[nWall].point2].x;
    if (y2) *y2 = wall[wall[nWall].point2].y;
}

short wallCstatAdd(int nWall, short cstat, char nextWall)
{
    setCstat(1, &wall[nWall].cstat, cstat);
    if (nextWall && wall[nWall].nextwall >= 0)
        setCstat(1, &wall[wall[nWall].nextwall].cstat, cstat);

    return wall[nWall].cstat;
}

short wallCstatRem(int nWall, short cstat, char nextWall)
{
    setCstat(0, &wall[nWall].cstat, cstat);
    if (nextWall && wall[nWall].nextwall >= 0)
        setCstat(0, &wall[wall[nWall].nextwall].cstat, cstat);

    return wall[nWall].cstat;
}

short wallCstatToggle(int nWall, short cstat, char nextWall)
{
    wall[nWall].cstat ^= cstat;
    if (nextWall && wall[nWall].nextwall >= 0)
    {
        if ((wall[nWall].cstat & cstat))
        {
            if (!(wall[wall[nWall].nextwall].cstat & cstat))
                wallCstatAdd(wall[nWall].nextwall, cstat, 0);
        }
        else
        {
            if ((wall[wall[nWall].nextwall].cstat & cstat))
                wallCstatRem(wall[nWall].nextwall, cstat, 0);
        }
    }

    return wall[nWall].cstat;
}

char wallVisible(int nWall)
{
    walltype* pWall = &wall[nWall];
    int nSect, nNextSect;
    int x[2], y[2];
    int i;
    
    nSect = sectorofwall(nWall);
    
    if ((nNextSect = pWall->nextsector) >= 0)
    {
        if ((pWall->cstat & kWallMasked) == 0 && (pWall->cstat & kWallOneWay) == 0)
        {
            getWallCoords(nWall, &x[0], &y[0], &x[1], &y[1]);
            for (i = 0; i < 2; i++)
            {
                if ((getceilzofslope(nSect, x[i], y[i]) < getceilzofslope(nNextSect, x[i], y[i]))
                    || (getflorzofslope(nSect, x[i], y[i]) > getflorzofslope(nNextSect, x[i], y[i])))
                            return 1;
            }
            
            return 0;
        }
    }

    return (klabs(sector[nSect].floorz - sector[nSect].ceilingz) > 0);
}

void wallRotateTile(int nWall, char enable)
{
    walltype* pWall = &wall[nWall];
    int nTile = pWall->picnum;
    
    if (enable)
    {
        if (gRotTile[nTile] > pWall->picnum)
        {
            pWall->picnum = gRotTile[nTile];
            return;
        }
        
        PICANM* pnm = &panm[pWall->picnum];
        int nBlank = tileSearchFreeRange(pnm->frames);
        char backward = (pnm->type == 3);
        int i = pnm->frames;
        
        dassert(nBlank > nTile);
        
        if (backward)
            nBlank += i;
        
        pWall->picnum = nBlank;
        
        do
        {
            artedCopyTile(nTile, nBlank);
            artedRotateTile(nBlank);
            gSysTiles.add(nBlank);
            
            gRotTile[nTile]  = nBlank;
            gRotTile[nBlank] = nTile;
            
            if (backward)
            {
                nBlank--;
                nTile--;
            }
            else
            {
                nBlank++;
                nTile++;
            }
            
            i--;
        }
        while(i >= 0);
    }
    else if (gRotTile[nTile] && gRotTile[nTile] < nTile)
    {
        pWall->picnum = gRotTile[nTile];
        return;
    }
}

char insideLoop(int x, int y, int nStartWall)
{
    int x1, y1, x2, y2;
    int i = nStartWall;
    int c;

    c = clockdir(i);
    do
    {
        getWallCoords(i, &x1, &y1, &x2, &y2);

        if (x1 >= x || x2 >= x)
        {
            if (y1 > y2)
            {
                swapValues(&x1, &x2);
                swapValues(&y1, &y2);
            }

            if (y1 <= y && y2 > y)
            {
                if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
                    c ^= 1;
            }
        }

        i = wall[i].point2;
    }
    while(i != nStartWall);
    return c;
}

int getSectorHeight(int nSector)
{
    int fz, cz;
    sectGetEdgeZ(nSector, &fz, &cz);
    return fz-cz;
}

char setAligntoWall(int nSect, int nWall)
{
    if ((nWall = nWall - sector[nSect].wallptr) >= 0)
    {
        sector[nSect].alignto = nWall;
        return sector[nSect].alignto;
    }

    return 0;
}

void setFirstWall(int nSect, int nWall)
{
    #if 1   
        // this one breaks point2 order
        int start, length, shift;
        int i, j, k;
        walltype tempWall;

        // rotate the walls using the shift copy algorithm

        start = sector[nSect].wallptr;
        length = sector[nSect].wallnum;

        dassert(nWall >= start && nWall < start + length);
        shift = nWall - start;

        int aWall = start + sector[nSect].alignto;
        int ax, ay;

        if (shift == 0)
            return;

        if (aWall != start)
            getWallCoords(aWall, &ax, &ay);

        i = k = start;

        for (int n = length; n > 0; n--)
        {
            if (i == k)
                tempWall = wall[i];

            j = i + shift;
            while (j >= start + length)
                j -= length;

            if (j == k)
            {
                wall[i] = tempWall;
                i = ++k;
            }
            else
            {
                wall[i] = wall[j];
                i = j;
            }
        }

        for (i = start; i < start + length; i++)
        {
            if (aWall != start)
            {
                if (wall[i].x == ax && wall[i].y == ay)
                    sector[nSect].alignto = i - start, aWall = start;
            }

            if ((wall[i].point2 -= shift) < start)
                wall[i].point2 += length;

            if (wall[i].nextwall >= 0)
                wall[wall[i].nextwall].nextwall = (short)i;
        }
        
    #else
        setfirstwall(nSect, nWall);
    #endif
    CleanUp();
}

char loopInside(int x, int y, int nWall)
{
    int i = nWall, x1, y1, x2, y2;
    unsigned int cnt = 0;
    
    do
    {
        getWallCoords(i, &x1, &y1, &x2, &y2);
        y1-=y, y2-=y;
        
        if ((y1^y2) < 0)
        {
            x1-=x, x2-=x;
            cnt ^= ((x1^x2) >= 0) ? (x1) : ((x1*y2-x2*y1)^y2);
        }
        
        i = wall[i].point2;
    }
    while(i != nWall);
    
    return ((cnt >> 31) > 0);
}

char loopInside(int nSect, POINT2D* pPoint, int c, char full)
{
    int nPrev, nAng, nLen, i, r;
    POINT2D *a = pPoint, *b;
    POSOFFS pos;

    for (i = 0; i < c; i++)
    {
        if (!inside(a[i].x, a[i].y, nSect))
            return 0;
    }

    if (full)
    {
        for (i = 0; i < c; i++)
        {
            if (i < c - 1)
            {
                a = &pPoint[i]; b = &pPoint[i+1];
            }
            else
            {
                a = &pPoint[0]; b = &pPoint[c-1];
            }

            nAng = (getangle(b->x - a->x, b->y - a->y) + kAng90) & kAngMask;
            nLen = exactDist(b->x - a->x, b->y - a->y);
            pos.New(nAng, a->x, a->y);

            while(pos.Distance() < nLen)
            {
                if (!inside(pos.x, pos.y, nSect))
                    return 0;

                r = ClipLow(nLen >> 4, 2);
                if (r % 2)
                    r++;

                nPrev = pos.Distance();
                while(nPrev == pos.Distance())
                    pos.Right(r+=2);
            }
        }
    }

    return 1;
}

char insidePoints(int nSect, POINT2D* point, int c)
{
    int s, e;
    getSectorWalls(nSect, &s, &e);
    while(s <= e)
    {
        if (!insidePoints(wall[s].x, wall[s].y, point, c))
            return 0;

        s++;
    }

    return 1;
}


char insidePoints(int x, int y, POINT2D* point, int c)
{
    int i, x1, y1, x2, y2;
    unsigned int cnt = 0;

    for (i = 0; i < c; i++)
    {
        if (i < c - 1)
        {
            x1 = point[i].x-x,  x2 = point[i+1].x-x;
            y1 = point[i].y-y,  y2 = point[i+1].y-y;
        }
        else
        {
            x1 = point[0].x-x,  x2 = point[c-1].x-x;
            y1 = point[0].y-y,  y2 = point[c-1].y-y;
        }

        if ((y1^y2) < 0)
            cnt ^= ((x1^x2) >= 0) ? (x1) : ((x1*y2-x2*y1)^y2);
    }

    return ((cnt >> 31) > 0);
}

char sectWallsInsidePoints(int nSect, POINT2D* point, int c)
{
    int s, e;

    // make sure all points inside a sector
    ///////////////////////

    if (loopInside(nSect, point, c, 1))
    {
        // make sure points didn't cover island walls
        ///////////////////////

        getSectorWalls(nSect, &s, &e);
        while(s <= e)
        {
            if (insidePoints(wall[s].x, wall[s].y, point, c) == 1)
                return 1;

            s++;
        }

        return 0;
    }

    return 1;
}

int insertLoop(int nSect, POINT2D* pInfo, int nCount, walltype* pWModel, sectortype* pSModel)
{
    walltype* pWall; sectortype* pSect;
    int nStartWall = (nSect >= 0) ? sector[nSect].wallptr : numwalls;
    int nWall = nStartWall, i = 0; char insertInside = (nSect >= 0);
    int t;

    if (!insertInside)
    {
        // create new white sector
        pSect = &sector[numsectors];

        if (pSModel)
        {
            memcpy(pSect, pSModel, sizeof(sectortype));
        }
        else
        {
            memset(pSect, 0, sizeof(sectortype));

            pSect->floorz   = 8192<<2;
            pSect->ceilingz = -pSect->floorz;
            pSect->extra    = -1;
        }

        pSect->wallptr  = nWall;
        pSect->wallnum  = nCount;
        nSect = numsectors++;
    }

    movewalls(nWall, nCount); // increases numwalls automatically

    while(i < nCount)
    {
        pWall = &wall[nWall];

        if (pWModel)
        {
            memcpy(pWall, pWModel, sizeof(walltype));
        }
        else
        {
            memset(pWall, 0, sizeof(walltype));
            pWall->xrepeat = pWall->yrepeat = 8;
            pWall->extra = -1;
        }

        pWall->point2       = ++nWall;
        pWall->nextwall     = -1;
        pWall->nextsector   = -1;

        pWall->x            = pInfo[i].x;
        pWall->y            = pInfo[i].y;
        i++;
    }

    pWall->point2 = nStartWall;

    if (clockdir(nStartWall) == 1)
        flipwalls(nStartWall, nStartWall + nCount);

    if (insertInside)
    {
        t = nSect;
        sector[t].wallnum += nCount;
        while(t++ < numsectors)
            sector[t].wallptr += nCount;

        if (clockdir(nStartWall) == 0)
            flipwalls(nStartWall, nStartWall + nCount);

        setFirstWall(nSect, nStartWall + nCount); // fix wallptr, so slope dir won't change
        nStartWall+=(sector[nSect].wallnum-nCount);
        
        for (i = 0; i < kMaxSprites; i++)
        {
            spritetype* pSpr = &sprite[i];
            if (pSpr->statnum >= kMaxStatus || nSect != pSpr->sectnum)
                continue;
            
            sprFixSector(pSpr);
        }
    }

    if (!pWModel)
    {
        nWall = nStartWall;
        do
        {
            fixrepeats(nWall);
            nWall = wall[nWall].point2;
        }
        while(nWall != nStartWall);
    }


    return nStartWall;
}

int insertLoop(int nSect, walltype* pWalls, int nCount, sectortype* pSModel)
{
    VOIDLIST point(sizeof(POINT2D));
    POINT2D buf; int n, i, j, k;

    for (i = 0; i < nCount; i++)
    {
        buf.x = pWalls[i].x;
        buf.y = pWalls[i].y;
        point.Add(&buf);
    }

    if ((n = insertLoop(nSect, (POINT2D*)point.First(), nCount, pWalls, pSModel)) >= 0)
    {
        i = n;
        j = 0;

        do
        {
            k = j; // flipped walls check
            while(pWalls[k].x != wall[i].x || pWalls[k].y != wall[i].y)
                k = DecRotate(k, nCount);
            
            wall[i].picnum      = pWalls[k].picnum;
            wall[i].overpicnum  = pWalls[k].overpicnum;
            wall[i].shade       = pWalls[k].shade;
            wall[i].pal         = pWalls[k].pal;
            wall[i].xrepeat     = pWalls[k].xrepeat;
            wall[i].yrepeat     = pWalls[k].yrepeat;
            wall[i].xpanning    = pWalls[k].xpanning;
            wall[i].ypanning    = pWalls[k].ypanning;
            wall[i].lotag       = pWalls[k].lotag;
            wall[i].hitag       = pWalls[k].hitag;
            wall[i].cstat       = pWalls[k].cstat;
            wall[i].extra       = pWalls[k].extra;
            wall[i].nextwall    = -1;
            wall[i].nextsector  = -1;

            j = IncRotate(j, nCount);
            i = wall[i].point2;
        }
        while(i != n);
    }

    return n;
}

void insertPoints(WALLPOINT_INFO* pInfo, int nCount)
{
    for (int i = 0; i < nCount; i++)
        insertpoint(pInfo[i].w, pInfo[i].x, pInfo[i].y);
}

int makeSquareSector(int ax, int ay, int area)
{
    int nWall = -1;
    LOOPSHAPE shape(kLoopShapeSquare, -1, ax, ay); // create square sector
    shape.Setup(ax + area, ay + area, NULL);
    switch(shape.StatusGet())
    {
        default:
            nWall = shape.Insert();
            // no break;
        case -1:
        case -2:
        case -4:
        case -5:
            shape.Stop();
            break;
    }

    return (nWall >= 0) ? sectorofwall(nWall) : nWall;
}

int redSectorCanMake(int nStartWall)
{

    int addwalls = -1;
    if (wall[nStartWall].nextwall >= 0) return -1;
    else if (numsectors >= kMaxSectors) return -4;
    else if ((addwalls = whitelinescan(nStartWall)) < numwalls) return -2;
    else if (addwalls >= kMaxWalls) return -3;
    else return addwalls;
}

char collectOuterWalls(int nWall, IDLIST* pLoop)
{
    if (wall[nWall].nextwall >= 0)
        return 0;

    IDLIST* pNode; int32_t* p;
    int i = nWall;
    int c = 0;

    do
    {
        c++;
        i = wall[i].point2;
        if (wall[i].nextwall >= 0)
        {
            pNode = new IDLIST(true);
            if (collectWallsOfNode(pNode, i) > 2)
            {
                for (p = pNode->GetPtr(); *p >= 0; p++)
                {
                    if (wall[*p].nextwall < 0)
                    {
                        i = *p;
                        break;
                    }
                }
            }

            delete(pNode);
        }

        if (!pLoop->Exists(i))
            pLoop->Add(i);
    }
    while(i != nWall && c < kMaxWalls);
    return (c < kMaxWalls);
}

void sectLoopMain(int nSect, int* s, int* e)
{
    int x, y, i, ts, te;
    getSectorWalls(nSect, &ts, &te);
    getWallCoords(ts, &x, &y);
    i = ts;

    while(ts <= te)
    {
        if (wall[ts].x <= x && wall[ts].y <= y)
        {
            x = wall[ts].x;
            y = wall[ts].y;
            i = ts;
        }

        ts++;
    }

    loopGetWalls(i, s, e);
}


int sectLoopTransfer(int nSectA, int nSectB, char flags)
{
    int s, e, ls, le;
    int done = 0;
    
    if (nSectA != nSectB)
    {
        getSectorWalls(nSectA, &s, &e);
        while(e >= s)
        {
            loopGetWalls(e, &ls, &le);
            
            if (clockdir(ls) == 1)
            {
                if (flags & 0x04)
                {
                    while (le >= ls && inside(wall[le].x, wall[le].y, nSectB)) le--;
                    if (le < ls && loopTransfer(ls, nSectB, flags) >= 0)
                        done++;
                }
                else if (loopTransfer(ls, nSectB, flags) >= 0)
                    done++;
            }
            
            e = --ls;
        }
    }
    
    return done;
}

int redSectorMake(int nWall)
{
    POINT2D buf; VOIDLIST point(sizeof(buf)); IDLIST list(true);
    int nSect = sectorofwall(nWall), nOSect = nSect, nNext;
    int32_t c, s, e, r = -5, *p;

    if (wall[nWall].nextwall >= 0)              return -1;
    if (nSect < 0)                              return -5;
    if (numsectors >= kMaxSectors)              return -4;
    if (!collectOuterWalls(nWall, &list))       return -2;
    if (numwalls+list.GetLength() >= kMaxWalls) return -3;

    for (c = 0, p = list.GetPtr(); *p >= 0; c++, p++)
    {
        buf.x = wall[*p].x, buf.y = wall[*p].y;
        point.Add(&buf);
    }

    if (insertLoop(-1, (POINT2D*)point.First(), c, &wall[nWall], &sector[nSect]) >= 0)
    {
        nSect = numsectors - 1;
        getSectorWalls(nSect, &s, &e);
        while(s <= e && (nNext = findNextWall(s)) >= 0)
        {
            wall[s].cstat = wall[nNext].cstat = 0;
            fixrepeats(nNext);
            fixrepeats(s);

            wallAttach(s, sectorofwall(nNext), nNext);
            wallAttach(nNext, nSect, s);
            s++;
        }

        if (s > e)
        {
            sectLoopTransfer(nOSect, nSect);
            worldSprCallFunc(sprFixSector);
            r = 0;
        }
        else
        {
            numsectors  -= 1;
            numwalls    -= c;
            r = -5;
        }
    }

    return r;
}

int redSectorMerge(int nThis, int nWith)
{
    int i, j, k, f, m, swal, ewal, tmp, nwalls = numwalls;
    short join[2]; join[0] = nThis, join[1] = nWith;

    for(i = 0; i < 2; i++)
    {
        getSectorWalls(join[i], &swal, &ewal);
        for(j = swal; j <= ewal; j++)
        {
            if (wall[j].cstat == 255)
                continue;

            tmp = i;
            if (wall[j].nextsector == join[1-tmp])
            {
                wall[j].cstat = 255;
                continue;
            }

            f = j;
            k = nwalls;
            do
            {
                memcpy(&wall[nwalls],&wall[f],sizeof(walltype));
                wall[nwalls].point2 = nwalls+1;
                nwalls++;
                wall[f].cstat = 255;

                f = wall[f].point2;
                if (wall[f].nextsector == join[1-tmp])
                {
                    f = wall[wall[f].nextwall].point2;
                    tmp = 1 - tmp;
                }
            }
            while ((wall[f].cstat != 255) && (wall[f].nextsector != join[1 - tmp]));
            wall[nwalls - 1].point2 = k;
        }
    }

    if (nwalls <= numwalls)
        return 0;

    memcpy(&sector[numsectors], &sector[join[0]], sizeof(sectortype));
    sector[numsectors].wallnum = nwalls - numwalls;
    sector[numsectors].wallptr = numwalls;

    for(i = numwalls;i < nwalls; i++)
    {
        if (wall[i].nextwall < 0) continue;
        wall[wall[i].nextwall].nextsector = numsectors;
        wall[wall[i].nextwall].nextwall = i;
    }

    for(i = 0; i < 2; i++)
    {
        getSectorWalls(join[i], &swal, &ewal);
        for(j = swal; j <= ewal; j++)
            wall[j].nextwall = wall[j].nextsector = -1;

        j = headspritesect[join[i]];
        while (j != -1)
        {
            k = nextspritesect[j];
            ChangeSpriteSect(j, numsectors);
            j = k;
        }
    }

    numwalls = nwalls, numsectors++;
    if (join[0] < join[1])
        join[1]--;

    deletesector(join[0]);
    deletesector(join[1]);
    return 0;
}

void setCstat(BOOL enable, short* pStat, int nStat)
{
    if (enable)
    {
        if (!(*pStat & nStat))
            *pStat |= (short)nStat;
    }
    else if ((*pStat & nStat))
        *pStat &= ~(short)nStat;
}

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt, int* zb, char flags)
{
    int t, cx, cy, xoff = 0;
    int nPic = pSpr->picnum, nAng = pSpr->ang;
    int xrep = pSpr->xrepeat, wh = tilesizx[nPic];

    *x1 = *x2 = pSpr->x;
    *y1 = *y2 = pSpr->y;

    if (flags & 0x01)
        xoff = panm[nPic].xcenter;

    if (flags & 0x02)
        xoff += pSpr->xoffset;

    if (pSpr->cstat & kSprFlipX)
        xoff = -xoff;

    if ((pSpr->cstat & kSprRelMask) == kSprFace)
        wh = (wh * 3) >> 2;

    cx = sintable[nAng & kAngMask] * xrep;
    cy = sintable[(nAng + kAng90 + kAng180) & kAngMask] * xrep;
    t = (wh>>1)+xoff;

    *x1 -= mulscale16(cx, t);   *x2 = *x1 + mulscale16(cx, wh);
    *y1 -= mulscale16(cy, t);   *y2 = *y1 + mulscale16(cy, wh);

    if (zt || zb)
    {
        int tzt, tzb;
        GetSpriteExtents(pSpr, &tzt, &tzb);
        if (zt)
            *zt = tzt;

        if (zb)
            *zb = tzb;
    }
}


void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags)
{
    int t, i, cx, cy, xoff = 0, yoff = 0;
    int nPic = pSpr->picnum, nAng = pSpr->ang & kAngMask;
    int xrep = pSpr->xrepeat, wh = tilesizx[nPic];
    int yrep = pSpr->yrepeat, hg = tilesizy[nPic];
    int nCos = sintable[(nAng + kAng90) & kAngMask];
    int nSin = sintable[nAng];

    *x1 = *x2 = *x3 = *x4 = pSpr->x;
    *y1 = *y2 = *y3 = *y4 =  pSpr->y;

    if (flags & 0x01)
    {
        xoff = panm[nPic].xcenter;
        yoff = panm[nPic].ycenter;
    }

    if ((flags & 0x02) && (pSpr->cstat & kSprRelMask) != kSprSloped)
    {
        xoff += pSpr->xoffset;
        yoff += pSpr->yoffset;
    }

    if (pSpr->cstat & kSprFlipX)
        xoff = -xoff;

    if (pSpr->cstat & kSprFlipY)
        yoff = -yoff;

    if (!(flags & 0x04))
    {
        if (wh % 2) wh++;
        if (hg % 2) hg++;
    }

    cx = ((wh>>1)+xoff)*xrep;
    cy = ((hg>>1)+yoff)*yrep;

    *x1 += dmulscale16(nSin, cx, nCos, cy);
    *y1 += dmulscale16(nSin, cy, -nCos, cx);

    t = wh*xrep;
    *x2 = *x1 - mulscale16(nSin, t);
    *y2 = *y1 + mulscale16(nCos, t);

    t = hg*yrep;
    i = -mulscale16(nCos, t);   *x3 = *x2 + i; *x4 = *x1 + i;
    i = -mulscale16(nSin, t);   *y3 = *y2 + i; *y4 = *y1 + i;
}

void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, int* zt, int* zb, char flags)
{
    GetSpriteExtents(pSpr, x1, y1, x2, y2, x3, y3, x4, y4, flags);

    if (zt || zb)
    {
        int tzt, tzb;
        GetSpriteExtents(pSpr, &tzt, &tzb);
        if (zt)
            *zt = tzt;

        if (zb)
            *zb = tzb;
    }
}

void ceilGetEdgeZ(int nSector, int* zBot, int* zTop)
{
    int s, e, x, y, z;
    sectortype* pSect = &sector[nSector];

    *zTop = *zBot = pSect->ceilingz;
    if ((pSect->ceilingstat & kSectSloped) && pSect->ceilingslope)
    {
        getSectorWalls(nSector, &s, &e);

        while(s <= e)
        {
            getWallCoords(s++, &x, &y);
            z = getceilzofslope(nSector, x, y);
            if (zBot && z < *zBot) *zBot = z;
            if (zTop && z > *zTop) *zTop = z;
        }
    }
}


void floorGetEdgeZ(int nSector, int* zBot, int* zTop)
{
    sectortype* pSect = &sector[nSector];
    int s, e, x, y, z;

    *zTop = *zBot = pSect->floorz;
    if ((pSect->floorstat & kSectSloped) && pSect->floorslope)
    {
        getSectorWalls(nSector, &s, &e);

        while(s <= e)
        {
            getWallCoords(s++, &x, &y);
            z = getflorzofslope(nSector, x, y);
            if (zBot && z > *zBot) *zBot = z;
            if (zTop && z < *zTop) *zTop = z;
        }
    }
}

char fixXSprite(int nID)
{
    int nOld;
    if ((nOld = sprite[nID].extra) > 0)
    {
        sprite[nID].extra                       = dbInsertXSprite(nID);
        xsprite[sprite[nID].extra]              = xsprite[nOld];
        xsprite[sprite[nID].extra].reference    = nID;
        return 1;
    }

    return 0;
}

char fixXSector(int nID)
{
    int nOld;
    if ((nOld = sector[nID].extra) > 0)
    {
        sector[nID].extra                       = dbInsertXSector(nID);
        xsector[sector[nID].extra]              = xsector[nOld];
        xsector[sector[nID].extra].reference    = nID;
        return 1;
    }

    return 0;
}

char fixXWall(int nID)
{
    int nOld;
    if ((nOld = wall[nID].extra) > 0)
    {
        wall[nID].extra                         = dbInsertXWall(nID);
        xwall[wall[nID].extra]                  = xwall[nOld];
        xwall[wall[nID].extra].reference        = nID;
        return 1;
    }

    return 0;
}

int worldSprCallFunc(HSPRITEFUNC pFunc, int nData)
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

int collectWallsOfNode(IDLIST* pList, int nWall, char flags)
{
    int nWallNum = numwalls;
    int32_t *p, l, n, t;

    pList->Add(nWall);
    t = nWall;

    do
    {
        if (wall[t].nextwall >= 0)
        {
            t = wall[wall[t].nextwall].point2;
            pList->Add(t);
        }
        else
        {
            t = nWall;
            do
            {
                n = lastwall(t);
                if (wall[n].nextwall >= 0)
                {
                    t = wall[n].nextwall;
                    pList->Add(t);
                }
                else
                {
                    pList->Add(n);
                    break;
                }

                nWallNum--;
            }
            while (t != nWall && nWallNum > 0);
            break;
        }

        nWallNum--;
    }
    while (t != nWall && nWallNum > 0);

    if (flags & 0x01)
    {
        if ((l = pList->GetLength()) > 0)
        {
            p = pList->GetPtr();
            while(--l >= 0)
            {
                n = p[l];
                if (wall[n].nextwall >= 0)
                    pList->AddIfNot(wall[n].nextwall), p = pList->GetPtr();
            }
        }
    }

    return pList->GetLength();
}

int isClockDir(POINT2D* p, int c)
{
    int minx, x0, x1, x2, y0, y1, y2;
    int i, themin; int64_t templong;
    int j;

    minx = 0x7fffffff;
    themin = -1;


    i = -1;
    j = IncRotate(i, c);

    do
    {
        i = IncRotate(i, c);
        j = IncRotate(j, c);

        if (p[j].x < minx)
        {
            minx = p[j].x;
            themin = i;
        }
    }
    while (j != 0);

    j = themin;
    x0 = p[j].x;
    y0 = p[j].y;

    j = IncRotate(j, c);
    x1 = p[j].x;
    y1 = p[j].y;

    j = IncRotate(j, c);
    x2 = p[j].x;
    y2 = p[j].y;

    if ((y1 >= y2) && (y1 <= y0)) return 0;
    if ((y1 >= y0) && (y1 <= y2)) return 1;

    templong = (x0-x1)*(y2-y1) - (x2-x1)*(y0-y1);
    if (templong < 0)
        return 0;
    else
        return 1;
}

void flipPoints(POINT2D* p, int c)
{
    int i = 0, h = c >> 1;
    POINT2D t;

    while(i < h)
        t = p[i], p[i] = p[c-i-1], p[c-i-1] = t, i++;
}

void rotatePoints(POINT2D* point, int numpoints, int s)
{
    int n = numpoints, i = 0, k = 0, j;
    POINT2D tmp;

    if (s > 0)
    {
        while(n > 0)
        {
            if (i == k)
                memcpy(&tmp, &point[i], sizeof(tmp));

            j = i+s;
            while (j >= numpoints)
                j -= numpoints;

            if (j == k)
            {
                memcpy(&point[i], &tmp, sizeof(tmp));
                i = ++k;
            }
            else
            {
                memcpy(&point[i], &point[j], sizeof(tmp));
                i = j;
            }

            n--;
        }
    }
}

int nextSpriteAt(int x, int y, int *prv)
{
    int i;
    for (i = ClipLow(*prv, 0); i < kMaxSprites; i++)
    {
        if (sprite[i].statnum >= kMaxStatus) continue;
        else if (sprite[i].x == x && sprite[i].y == y)
        {
            if (*prv == -1 || *prv < i)
            {
                *prv = i;
                return i;
            }
        }
    }

    *prv = -1;
    return -1;
}

char sectorToolDisableAll(char alsoFreeDraw)
{
    char r = 0;

    if (pGDoorSM)       r = 1, DELETE_AND_NULL(pGDoorSM);
    if (pGDoorR)        r = 1, DELETE_AND_NULL(pGDoorR);
    if (pGCircleW)      r = 1, DELETE_AND_NULL(pGCircleW);
    if (pGLShape)       r = 1, DELETE_AND_NULL(pGLShape);
    if (pGArc)          r = 1, DELETE_AND_NULL(pGArc);
    if (pGLoopSplit)    r = 1, DELETE_AND_NULL(pGLoopSplit);

    if (alsoFreeDraw && pGLBuild)
        r = 1, DELETE_AND_NULL(pGLBuild)

    return r;
}


char sectorToolEnable(int nType, int nData)
{
    char r = 0;

    switch(nType)
    {
        case kSectToolShape:
            if (rngok(nData, 0, LENGTH(gLoopShapeTypes)))
            {
                sectorToolDisableAll(1);
                pGLShape = new LOOPSHAPE(nData, sectorhighlight, mousxplc, mousyplc);
                r = 1;
            }
            break;
        case kSectToolLoopSplit:
            pGLoopSplit = new LOOPSPLIT();
            r = 1;
            break;
        case kSectToolDoorWiz:
            // other tools will be disabled in the dialog
            r = dlgDoorWizard();
            break;
        case kSectToolArcWiz:
            sectorToolDisableAll(1);
            pGArc = new SECTAUTOARC();
            r = 1;
            break;
        case kSectToolCurveWall:
            if (linehighlight >= 0)
            {
                sectorToolDisableAll(1);
                pGCircleW = new CIRCLEWALL(linehighlight, mousxplc, mousyplc);
                pGCircleW->count = gMisc.circlePoints;
                r = 1;
            }
            break;
        default:
            nType = LENGTH(gSectToolNames) - 1;
            break;
    }

    if (Beep(r)) scrSetMessage("%s started.", gSectToolNames[nType]);
    else scrSetMessage("Unable to start %s tool.", gSectToolNames[nType]);
    return r;
}

char sectorToolDlgLauncher()
{
    #define AFTERH(a, b) (a->left+a->width+b)
    #define AFTERV(a, b) (a->top+a->height+b)

    TextButton* bButton; NAMED_TYPE* pEntry;
    int i,  y = 8;
    int nCode;

    Window dialog(0, 0, 180, 200, "Sector tools");
    FieldSet* fDrawShape    = new FieldSet(8, 8, dialog.client->width-16, 0, "SHAPE DRAWING", kColorRed, kColorBlack, 0);

    for (i = 0; i < LENGTH(gLoopShapeTypes); i++, y += 22)
    {
        pEntry = &gLoopShapeTypes[i];
        bButton = new TextButton(8, y, fDrawShape->client->width-16, 22, pEntry->name, 1000+pEntry->id);
        fDrawShape->Insert(bButton);
    }

    fDrawShape->client->height += y;
    fDrawShape->height = fDrawShape->client->height+8;

    FieldSet* fOther        = new FieldSet(8, AFTERV(fDrawShape, 16), dialog.client->width-16, 104, "OTHER TOOLS", kColorRed, kColorBlack, 0);
    TextButton* bDoorWiz    = new TextButton(8, 8, fOther->client->width-16, 22, "Door wizard",                     100);
    TextButton* bArcWiz     = new TextButton(8, AFTERV(bDoorWiz, 0), fOther->client->width-16, 22, "Arc wizard",    101);
    TextButton* bCurveW     = new TextButton(8, AFTERV(bArcWiz, 0), fOther->client->width-16, 22, "Curve wall",     102);
    TextButton* bLoopSpl    = new TextButton(8, AFTERV(bCurveW, 0), fOther->client->width-16, 22, "Loop split",     103);

    fOther->Insert(bDoorWiz);
    fOther->Insert(bArcWiz);
    fOther->Insert(bCurveW);
    fOther->Insert(bLoopSpl);

    dialog.Insert(fDrawShape);
    dialog.Insert(fOther);

    dialog.client->height = AFTERV(fOther, 0);
    dialog.height = dialog.client->height+32;


    while( 1 )
    {
        ShowModal(&dialog);

        nCode = 0;
        if (dialog.endState != mrCancel)
        {
            if (fDrawShape->focus != &fDrawShape->head)
            {
                bButton = (TextButton*)((Container*)fDrawShape->client->focus);
                if ((nCode = sectorToolEnable(kSectToolShape, bButton->result - 1000)) <= 0)
                    continue;
            }
            else if (fOther->focus != &fOther->head)
            {
                bButton = (TextButton*)((Container*)fOther->client->focus);
                switch(bButton->result)
                {
                    case 100:
                        if ((nCode = sectorToolEnable(kSectToolDoorWiz, 0)) <= 0) continue;
                        break;
                    case 101:
                        if ((nCode = sectorToolEnable(kSectToolArcWiz, 0)) <= 0) continue;
                        break;
                    case 102:
                        if ((nCode = sectorToolEnable(kSectToolCurveWall, 0)) <= 0) continue;
                        break;
                    case 103:
                        if ((nCode = sectorToolEnable(kSectToolLoopSplit, 0)) <= 0) continue;
                        break;
                    default:
                        continue;
                }
            }
            else
            {
                continue;
            }
        }

        break;
    }

    return (nCode != 0);
}

int GetWallZPeg( int nWall )
{
    int z;

    int nSector = sectorofwall(nWall);
    int nNextSector = wall[nWall].nextsector;

    if (nNextSector == -1)
    {
        // one sided wall
        if ( wall[nWall].cstat & kWallOrgBottom )
            z = sector[nSector].floorz;
        else
            z = sector[nSector].ceilingz;
    }
    else
    {
        // two sided wall
        if ( wall[nWall].cstat & kWallOrgOutside )
            z = sector[nSector].ceilingz;
        else
        {
            // top step
            if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
                z = sector[nNextSector].ceilingz;
            // bottom step
            if (sector[nNextSector].floorz < sector[nSector].floorz)
                z = sector[nNextSector].floorz;
        }
    }
    return z;
}

char AlignWalls(int nTile, int z0, int z1, char doxpan, int w0_pan, int w0_rep, int w1_pan, int w1_rep)
{
    if (tilesizx[nTile] == 0 || tilesizy[nTile] == 0)
        return 0;

    uint8_t r = 0, t;

    //do the x alignment
    if (doxpan)
    {
        t = (uint8_t)((wall[w0_pan].xpanning + (wall[w0_rep].xrepeat<<3))%tilesizx[nTile]);
        if (t != wall[w1_pan].xpanning)
            wall[w1_pan].xpanning = t, r = 1;
    }

    uint8_t first_yPan = wall[w0_pan].ypanning;                                     // Y panning of first wall
    uint8_t first_yRepeat = wall[w0_rep].yrepeat;                                   // Y repeat of first wall
    int32_t yPan_offset = (((z1-z0) * first_yRepeat) / (tilesizy[nTile] << 3));     // y-panning offset.
    uint8_t second_yPan = (uint8_t)(yPan_offset + first_yPan);                      // The final y-panning for the second wall

    if (first_yRepeat != wall[w1_rep].yrepeat)
        wall[w1_rep].yrepeat = first_yRepeat, r = 1;

    if (second_yPan != wall[w1_pan].ypanning)
        wall[w1_pan].ypanning = second_yPan, r = 1;

    return r;
}

char AlignWalls(int w0, int z0, int w1, int z1, char doxpan)
{
    return AlignWalls(wall[w0].picnum, z0, z1, doxpan, w0, w0, w1, w1);
}

int ED32_AutoAlignWalls_GetWall(char bot, int32_t w)
{
    return (bot && (wall[w].cstat & kWallSwap) && wall[w].nextwall >= 0) ? wall[w].nextwall : w;
}

// flags:
//  0x01: more than once
//  0x02: (unused)
//  0x04: carry pixel width from first wall over to the rest
//  0x08: (unused)
//  0x10: iterate lastwall()s (point2 in reverse)
//  0x20: use special logic for 'bottom-swapped' walls
int32_t ED32_AutoAlignWalls(IDLIST* pDone, int w0, char flags, int32_t nrecurs)
{
    // based on eduke32
    ////////////////////////////////////

    static int numaligned, wall0;
    static uint32_t lenrepquot;
    static int32_t cstat0;

    int w0b, w1, w1b, z0, z1;
    int nTile;

    char dir = ((flags & 0x10) != 0);
    char bot = ((flags & 0x20) != 0);

    w1 = (dir) ? lastwall(w0) : wall[w0].point2;
    w0b = ED32_AutoAlignWalls_GetWall(bot, w0);
    nTile = wall[w0b].picnum;
    z0  = GetWallZPeg(w0);

    if (nrecurs == 0)
    {
        lenrepquot = getlenbyrep(getWallLength(w0), wall[w0].xrepeat), wall0 = w0;
        cstat0 = wall[w0b].cstat & kWallFlipMask;
        numaligned = 0;
        pDone->Add(w0);
    }

    //loop through walls at this vertex in point2 order
    while( 1 )
    {
        w1b = ED32_AutoAlignWalls_GetWall(bot, w1);

        //break if this wall would connect us in a loop
        if (pDone->Exists(w1))
            break;

        pDone->Add(w1);

        //break if reached back of left wall
        if (wall[w1].nextwall == w0)
            break;

        if (wall[w1b].picnum == nTile && wallVisible(w1b))
        {
            if ((flags & 0x04) && w1 != wall0)
                fixxrepeat(w1, lenrepquot);

            if (AlignWalls(nTile, GetWallZPeg(w0), GetWallZPeg(w1), 1, w0b, w0, w1b, w1))
                numaligned++;

            wall[w1b].cstat &= ~kWallFlipMask;
            wall[w1b].cstat |= cstat0;

            if ((flags & 0x01) == 0)
                return numaligned;

            //if wall was 1-sided, no need to recurse
            if (wall[w1].nextwall < 0)
            {
                w0 = w1;
                w0b = ED32_AutoAlignWalls_GetWall(bot, w0);
                w1 = (dir) ? lastwall(w0) : wall[w0].point2;
                continue;
            }

            ED32_AutoAlignWalls(pDone, w1, flags, nrecurs+1);
        }

        if (wall[w1].nextwall < 0)
            break;

        w1 = (dir) ? lastwall(wall[w1].nextwall)
                        : wall[wall[w1].nextwall].point2;
    }

    return numaligned;
}

int AutoAlignWalls(int nWall0, char flags)
{
    IDLIST* pDone = new IDLIST(1);
    int c = ED32_AutoAlignWalls(pDone, nWall0, flags, 0);
    delete(pDone);

    pDone = new IDLIST(1);
    ED32_AutoAlignWalls(pDone, nWall0, flags, 0); // second trial?
    delete(pDone);
    return c;
}

int AutoAlignSectors(int nStart, int nFor, IDLIST* pList)
{
    int sbfz, stfz, sbcz, stcz, tz, bz;
    int s, e, n;

    sectortype *pSect, *pSrc = &sector[nStart];
    getSectorWalls(nStart, &s, &e);

    floorGetEdgeZ(nStart, &sbfz, &stfz);
    ceilGetEdgeZ(nStart, &sbcz, &stcz);

    sectCstatRem(nStart, kSectRelAlign, nFor);
    pList->Add(nStart);

    do
    {
        if ((n = wall[s].nextsector) < 0 || pList->Exists(n))
            continue;

        if (sectCstatGet(n, kSectRelAlign, nFor)
            && sectCstatGet(n, kSectSloped, nFor))
                continue;

        pSect = &sector[n];
        if (nFor == OBJ_CEILING)
        {
            if (pSrc->ceilingpicnum != pSect->ceilingpicnum)
                continue;

            ceilGetEdgeZ(n, &bz, &tz);
            if (bz != sbcz && tz != stcz)
                continue;

            pSect->ceilingxpanning = pSrc->ceilingxpanning;
            pSect->ceilingypanning = pSrc->ceilingypanning;
        }
        else
        {
            if (pSrc->floorpicnum != pSect->floorpicnum)
                continue;


            floorGetEdgeZ(n, &bz, &tz);
            if (bz != sbfz && tz != stfz)
                continue;

            pSect->floorxpanning = pSrc->floorxpanning;
            pSect->floorypanning = pSrc->floorypanning;
        }

        if (sectCstatGet(nStart, kSectSwapXY, nFor)) sectCstatAdd(n, kSectSwapXY, nFor);
        else sectCstatRem(n, kSectSwapXY, nFor);

        if (sectCstatGet(nStart, kSectFlipX, nFor)) sectCstatAdd(n, kSectFlipX, nFor);
        else sectCstatRem(n, kSectFlipX, nFor);

        if (sectCstatGet(nStart, kSectFlipY, nFor)) sectCstatAdd(n, kSectFlipY, nFor);
        else sectCstatRem(n, kSectFlipY, nFor);

        AutoAlignSectors(n, nFor, pList);
    }
    while(++s <= e);

    return pList->GetLength();
}

XSPRITE* pathMarkerFind(XSECTOR* pXSect, XSPRITE *pXMark, IDLIST* pDone, char dir)
{
    spritetype* pSpr; XSPRITE* pXSpr;
    int nID, i;

    nID = (pXMark != NULL) ? ((dir) ? pXMark->data2 : pXMark->data1) : pXSect->data;

    for (i = headspritestat[kStatPathMarker]; i >= 0; i = nextspritestat[i])
    {
        pSpr = &sprite[i];
        if (!xspriRangeIsFine(pSpr->extra))
            continue;

        pXSpr = &xsprite[pSpr->extra];
        if ((dir && pXSpr->data1 == nID) || (!dir && pXSpr->data2 == nID))
        {
            if (!pDone || !pDone->Exists(i))
                return pXSpr;
        }
    }

    return NULL;
}

XSECTOR* pathMarkerFindSector(XSPRITE *pXMark)
{
    sectortype* pSect; XSECTOR* pXSect;
    XSPRITE *a, *s, *p;
    int i;
    
    IDLIST done(1);
    
    i = numsectors;
    while(--i >= 0)
    {
        pSect = &sector[i];
        
        if (pSect->extra < 0
            || (pSect->type != kSectorPath && pSect->type != kModernSectorPathSprite))
                continue;
        
        a = s = p = NULL;
        pXSect = &xsector[pSect->extra];
        while((a = pathMarkerFind(pXSect, a, &done, 1)) != NULL && a != p && a != s)
        {
            if (a == pXMark) return pXSect;
            done.Add(a->reference);
            p = a;
        }
    }
    
    return NULL;
}

void pathMarkerChangeData(int nOld, int nNew, char dir)
{
    spritetype* pSpr; XSPRITE* pXSpr;
    int nID, i;
    
    for (i = headspritestat[kStatPathMarker]; i >= 0; i = nextspritestat[i])
    {
        pSpr = &sprite[i];
        if (!xspriRangeIsFine(pSpr->extra))
            continue;

        pXSpr = &xsprite[pSpr->extra];
        if (dir == 1 && pXSpr->data2 == nOld)    pXSpr->data2 = nNew;
        if (dir != 1 && pXSpr->data1 == nOld)    pXSpr->data1 = nNew;
    }
}

char pathChopIsMarkerAllowed(XSPRITE* xa, XSPRITE* xb)
{
    int i, out = 0, in = 0;
    int j;
    
    if (sprite[xa->reference].hitag != sprite[xb->reference].hitag)
        return 0;
    
    if (xa->txID)
        return 0; // because can send multiple times
    
    if (xa->rxID)
    {
        if (xa->rxID == xb->rxID && xa->locked != xb->locked)
            return 0; // because can toggle lock
    }
    
    if (xa->data2 < 0 || xa->data1 == xa->data2)    return 0;
    if (xa->command == kCmdDudeFlagsSet)            return 0;
    if (xa->waitTime)                               return 0;
    if (xa->busyTime != xb->busyTime)               return 0;
    if (xa->dudeLockout)                            return 0;
    
    for (i = headspritestat[kStatPathMarker]; i >= 0; i = nextspritestat[i])
    {
        spritetype* c = &sprite[i];
        if (c->extra < 0)
            continue;
        
        XSPRITE* xc = &xsprite[c->extra];
        if ((xc->data1 == xa->data2 && ++out > 1) || (xc->data2 == xb->data1 && ++in > 1))
            return 0;
    }
    
    i = numsectors;
    while(--i >= 0)
    {
        if (sector[i].extra <= 0)
            continue;
        
        if (sector[i].type != kSectorPath && sector[i].type != kModernSectorPathSprite)
            continue;
        
        XSECTOR* pXSect = &xsector[sector[i].extra];
        XSPRITE* xt = NULL;
        
        for (j = headspritestat[kStatPathMarker]; j >= 0; j = nextspritestat[j])
        {
            spritetype* c = &sprite[j];
            if (c->extra < 0)
                continue;
            
            XSPRITE* xc = &xsprite[c->extra];
            if (xt == NULL)
            {
                if (xc->data1 == pXSect->data)
                    xt = xc;
            }
            else if (xt->data2 == xc->data1)
                xt = xc;
            
            if (xt == xa)
                return 0;
        }
        
    }
    
    return 1;
}

void pathChop(XSPRITE* xa, int nAng)
{
    spritetype *a, *b, *c; XSPRITE *xb;
    int i, da, a1, a2;
    int32_t* p;
    
    IDLIST found(1);
    
    for (i = headspritestat[kStatPathMarker]; i >= 0; i = nextspritestat[i])
    {
        b = &sprite[i];
        if (b->extra < 0 || found.Exists(i))
            continue;
        
        xb = &xsprite[b->extra];
        if (xb->data1 == xa->data2)
        {
            i = headspritestat[kStatPathMarker];
            found.Add(xa->reference);
            xa = xb;
        }
    }
    
    p = found.First();
    
    while(*p >= 0)
    {
        a = &sprite[*p]; p++; if (*p < 0) break;
        b = &sprite[*p]; p++; if (*p < 0) break;
        c = &sprite[*p];
        
        a1 = getangle(a->x - b->x, a->y - b->y) & kAngMask;
        a2 = getangle(b->x - c->x, b->y - c->y) & kAngMask;
        da = klabs(DANGLE(a1, a2));
        
        if (da <= nAng)
        {
            if (pathChopIsMarkerAllowed(&xsprite[a->extra], &xsprite[b->extra]))
            {
                DeleteSprite(b->index);
                found.Remove(b->index);
                
                xsprite[a->extra].data2 = xsprite[c->extra].data1;
                p = found.First();
                continue;
            }
        }
        
        p--;
    }
}
