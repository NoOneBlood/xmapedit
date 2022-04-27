/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1997: Originally written by Nick Newhard.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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

#include "db.h"
#include "common_game.h"
#include "editor.h"
#include "edit2d.h"
#include "misc.h"
#include "tracker.h"
#include "trig.h"
#include "xmpstub.h"
#include "xmpmisc.h"

BOOL gDrawTracer = FALSE;

CXTracker::CXTracker()
{
    TrackClear();
}


void DrawVector(int x1, int y1, int x2, int y2, char c, int a5)
{
	draw2dArrowMarker(x1, y1, x2, y2, c, a5);
}

void CXTracker::TrackClear()
{
    m_type = 0;
    m_nSector = m_nWall = m_nSprite = -1;
    m_pSector = 0;
    m_pWall = 0;
    m_pSprite = 0;
    m_pXSector = 0;
    m_pXWall = 0;
    m_pXSprite = 0;
	
	gDrawTracer = FALSE;
}

void CXTracker::TrackSector(short nSector, char a2)
{

    TrackClear();
    if (nSector < 0 || nSector >= kMaxSectors || sector[nSector].extra <= 0)
        return;

    sectortype* pSector = &sector[nSector];
    XSECTOR* pXSector = &xsector[pSector->extra];
    if ((a2 && !pXSector->txID) || (!a2 && !pXSector->rxID))
        return;

    m_nSector = nSector;
    m_pSector = pSector;
    m_pXSector = pXSector;
    m_type = a2;
	
	gDrawTracer = TRUE;
}

void CXTracker::TrackWall(short nWall, char a2)
{

    TrackClear();
    if (nWall < 0 || nWall >= kMaxWalls || wall[nWall].extra <= 0)
        return;

    walltype* pWall = &wall[nWall];
    XWALL* pXWall = &xwall[pWall->extra];
    if ((a2 && !pXWall->txID) || (!a2 && !pXWall->rxID))
        return;

    m_nWall = nWall;
    m_pWall = pWall;
    m_pXWall = pXWall;
    m_type = a2;
	
	gDrawTracer = TRUE;
}

void CXTracker::TrackSprite(short nSprite, char a2)
{
    TrackClear();
    if (nSprite < 0 || nSprite >= kMaxSprites || sprite[nSprite].extra <= 0)
		return;
	
    spritetype* pSprite = &sprite[nSprite];
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
	if (!a2 && !pXSprite->rxID) return;
	else if (a2 && !pXSprite->txID && !isMultiTx(nSprite))
		return;
	
    m_nSprite = nSprite;
    m_pSprite = pSprite;
    m_pXSprite = pXSprite;
    m_type = a2;
	
	gDrawTracer = TRUE;
}

void CXTracker::Draw(int a1, int a2, int a3) {
    
	if (!gDrawTracer)
		return;
	
	BOOL isMulti = FALSE;
    int txId, rxId, x, y, i;
    char v4 = h;
	
	if (m_type)
    {
	
		if (m_pXSector)
        {
            
			if (m_nSector >= numsectors || sector[m_nSector].extra <= 0) {
				TrackClear();
				return;
			}
			
			txId = m_pXSector->txID;
            walltype* pWall2 = &wall[m_pSector->wallptr];
			x = mulscale14(pWall2->x - a1, a3)+halfxdim16;
            y = mulscale14(pWall2->y - a2, a3)+midydim16;
        }
        else if (m_pXWall)
        {

			if (m_nWall >= numwalls || wall[m_nWall].extra <= 0) {
				TrackClear();
				return;
			}
			
			txId = m_pXWall->txID;
            walltype* pWall2 = &wall[m_pWall->point2];
            int dx = (pWall2->x-m_pWall->x) >> 1; 
            int dy = (pWall2->y-m_pWall->y) >> 1; 
            int nx = m_pWall->x + dx;
            int ny = m_pWall->y + dy;

            x = mulscale14(nx - a1, a3)+halfxdim16;
            y = mulscale14(ny - a2, a3)+midydim16;
        }
        else if (m_pXSprite)
        {
            
			if (sprite[m_nSprite].statnum >= kMaxStatus || sprite[m_nSprite].extra <= 0) {
				TrackClear();
				return;
			}
			
			txId = m_pXSprite->txID;
			x = mulscale14(m_pSprite->x - a1, a3)+halfxdim16;
            y = mulscale14(m_pSprite->y - a2, a3)+midydim16;
			isMulti = isMultiTx(m_nSprite);

        }
        else
        {
            return;
        }
        
		
		if (!isMulti && txId == 0)
            return;
        
		for (i = 0; i < kMaxSprites; i++) {
            spritetype* pSprite = &sprite[i];
            if (pSprite->extra <= 0 || pSprite->statnum >= kMaxStatus ) continue;
			else if (pSprite->index == m_nSprite || (rxId = xsprite[pSprite->extra].rxID) == 0) continue;
			else if ((!isMulti && rxId != txId) || (isMulti && !multiTxPointsRx(rxId, m_nSprite)))
				continue;
			
			int x2 = mulscale14(pSprite->x-a1, a3)+halfxdim16;
			int y2 = mulscale14(pSprite->y-a2, a3)+midydim16;
			DrawVector(x, y, x2, y2, clr2std(v4 ^ 3), a3);
           
        }
		
        for (i = 0; i < numwalls; i++) {
            walltype* pWall = &wall[i];
			if ((pWall->extra <= 0) || (i == m_nWall) || (rxId = xwall[pWall->extra].rxID) == 0) continue;
			else if ((!isMulti && rxId != txId) || (isMulti && !multiTxPointsRx(rxId, m_nSprite)))
				continue;
			
			walltype* pWall2 = &wall[pWall->point2];
			int dx = (pWall2->x-pWall->x) >> 1;
			int dy = (pWall2->y-pWall->y) >> 1; 
			int nx = pWall->x + dx;
			int ny = pWall->y + dy;
			int x2 = mulscale14(nx-a1, a3)+halfxdim16;
			int y2 = mulscale14(ny-a2, a3)+midydim16;
			DrawVector(x, y, x2, y2, clr2std(v4^4), a3);
            
        }
        
		for (i = 0; i < numsectors; i++) {
            sectortype* pSector = &sector[i];
			if ((pSector->extra <= 0) || (i == m_nSector) || (rxId = xsector[pSector->extra].rxID) == 0) continue;
			else if ((!isMulti && rxId != txId) || (isMulti && !multiTxPointsRx(rxId, m_nSprite)))
				continue;
			
			walltype* pWall = &wall[pSector->wallptr];
			int x2 = mulscale14(pWall->x-a1, a3)+halfxdim16;
			int y2 = mulscale14(pWall->y-a2, a3)+midydim16;
			DrawVector(x, y, x2, y2, clr2std(v4^6), a3);
        }
    }
    else
    {
        if (m_pXSector)
        {
            rxId = m_pXSector->rxID;
            walltype* pWall2 = &wall[m_pSector->wallptr];
            x = mulscale14(pWall2->x - a1, a3)+halfxdim16;
            y = mulscale14(pWall2->y - a2, a3)+midydim16;
        }
        else if (m_pXWall)
        {
            rxId = m_pXWall->rxID;
            walltype* pWall2 = &wall[m_pWall->point2];
            int dx = (pWall2->x-m_pWall->x) >> 1; 
            int dy = (pWall2->y-m_pWall->y) >> 1; 
            int nx = m_pWall->x + dx;
            int ny = m_pWall->y + dy;

            x = mulscale14(nx - a1, a3)+halfxdim16;
            y = mulscale14(ny - a2, a3)+midydim16;
        }
        else if (m_pXSprite)
        {
            rxId = m_pXSprite->rxID;
            x = mulscale14(m_pSprite->x - a1, a3)+halfxdim16;
            y = mulscale14(m_pSprite->y - a2, a3)+midydim16;
        }
        else
        {
            return;
        }
        
		if (rxId == 0)
            return;
        
		for (short i = 0; i < kMaxSprites; i++) {
            
			spritetype* pSprite = &sprite[i];
            if (pSprite->statnum >= kMaxStatus || pSprite->extra <= 0 || pSprite->index == m_nSprite)
				continue;
			
			isMulti = isMultiTx(i); txId = xsprite[pSprite->extra].txID;
			if ((isMulti && !multiTxPointsRx(rxId, i)) || (!isMulti && txId != rxId))
				continue;

			int x2 = mulscale14(pSprite->x-a1, a3)+halfxdim16;
            int y2 = mulscale14(pSprite->y-a2, a3)+midydim16;
            DrawVector(x2, y2, x, y, clr2std(v4 ^ 11), a3);
			
        }
		
        for (i = 0; i < numwalls; i++) {
           
		    walltype* pWall = &wall[i];
            if (pWall->extra <= 0 || i == m_nWall || (txId = xwall[pWall->extra].txID) != rxId)
                continue;

			walltype* pWall2 = &wall[pWall->point2];
			int dx = (pWall2->x-pWall->x) >> 1;
			int dy = (pWall2->y-pWall->y) >> 1; 
			int nx = pWall->x + dx;
			int ny = pWall->y + dy;
			int x2 = mulscale14(nx-a1, a3)+halfxdim16;
			int y2 = mulscale14(ny-a2, a3)+midydim16;
			DrawVector(x2, y2, x, y, clr2std(v4^12), a3);
            
        }
		
        for (i = 0; i < numsectors; i++) {
            
			sectortype* pSector = &sector[i];
            if (pSector->extra <= 0 || i == m_nSector || (txId = xsector[pSector->extra].txID) != rxId)
                continue;
            
			walltype* pWall = &wall[pSector->wallptr];
			int x2 = mulscale14(pWall->x-a1, a3)+halfxdim16;
			int y2 = mulscale14(pWall->y-a2, a3)+midydim16;
			DrawVector(x2, y2, x, y, clr2std(v4^14), a3);
           
        }
    }
}

CXTracker gXTracker;
