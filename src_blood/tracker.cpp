/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// X-object tracker for xmapedit.
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

#include "tracker.h"
#include "xmpstub.h"
#include "nnexts.h"
#include "preview.h"

unsigned char CXTracker::tx;
OBJECT_LIST* CXTracker::pList;
OBJECT CXTracker::src;

void CXTracker::Clear()
{
	if (pList)
		delete(pList);
	
	pList			= NULL;
	src.type		= OBJ_NONE;
	src.index		= 0;
	tx = 0;
}


int CXTracker::Track(int nType, int nID, char send)
{
	int mtx[4], i, nRetn = 0;
	IDLIST chl(true);
	Clear();
	
	tx = send;
	switch(nType)
	{
		case OBJ_WALL:
			if (nID < numwalls && wall[nID].extra > 0)
				chl.Add((tx) ? xwall[wall[nID].extra].txID : xwall[wall[nID].extra].rxID);
			
			break;
		case OBJ_SECTOR:
			if (nID < numsectors && sector[nID].extra > 0)
				chl.Add((tx) ? xsector[sector[nID].extra].txID : xsector[sector[nID].extra].rxID);
			
			break;
		case OBJ_SPRITE:
			if (sprite[nID].statnum < kMaxStatus && sprite[nID].extra > 0)
			{
				if (tx)
				{
					if (isMultiTx(nID))
					{
						if (multiTxGetRange(nID, mtx))
						{
							while(mtx[0] <= mtx[1])
								chl.Add(mtx[0]++);
						}
						else
						{
							for (i = 0; i < LENGTH(mtx); i++)
								chl.Add(mtx[i]);
						}
					}
					else
					{
						chl.Add(xsprite[sprite[nID].extra].txID);
					}
				}
				else
				{
					chl.Add(xsprite[sprite[nID].extra].rxID);
				}
			}
			break;
	}
	
	if (chl.GetLength())
	{
		src.type		= nType;
		src.index		= nID;
		
		pList = new(OBJECT_LIST);
		int32_t* pDb = chl.GetPtr();
		
		while(*pDb != -1)
		{
			if (rngok(*pDb, 1, kChannelUserMax))
				collectObjectsByChannel(*pDb, tx, pList);
			
			pDb++;
		}
		
		nRetn = pList->Length();
		if (!nRetn)
			Clear();
	}
	
	return nRetn;
}

void CXTracker::Draw(SCREEN2D* pScr)
{
	if (!pList)
		return;
	
	char h = (gFrameClock & 16);
	int x1, y1, x2, y2;
	char color, draw;

	switch(src.type)
	{
		case OBJ_SECTOR:
			avePointSector(src.index, &x1, &y1);
			break;
		case OBJ_WALL:
			avePointWall(src.index, &x1, &y1);
			break;
		case OBJ_SPRITE:
			if (gPreviewMode && previewSpriteRemoved(&sprite[src.index])) return;
			x1 = sprite[src.index].x;
			y1 = sprite[src.index].y;
			break;
		default:
			Clear();
			return;
	}
	
	x1 = pScr->cscalex(x1);
	y1 = pScr->cscaley(y1);
	OBJECT* pDb = pList->Ptr();
	
	while(pDb->type != OBJ_NONE)
	{
		draw = 1;
		if (pDb->type != src.type || pDb->index != src.index)
		{
			switch(pDb->type)
			{
				case OBJ_SECTOR:
					avePointSector(pDb->index, &x2, &y2);
					color = pScr->ColorGet(kClrTracerSec, h);
					break;
				case OBJ_WALL:
					avePointWall(pDb->index, &x2, &y2);
					color = pScr->ColorGet(kClrTracerWal, h);
					break;
				case OBJ_SPRITE:
					if (gPreviewMode && previewSpriteRemoved(&sprite[pDb->index]))
					{
						draw = 0;
					}
					else
					{
						color = pScr->ColorGet(kClrTracerSpr, h);
						x2 = sprite[pDb->index].x;
						y2 = sprite[pDb->index].y;
					}
					break;
			}
			
			if (draw)
			{
				x2 = pScr->cscalex(x2);
				y2 = pScr->cscaley(y2);
				
				if (!tx)
					pScr->DrawArrow(x2, y2, x1, y1, color, pScr->data.zoom, kAng15 + kAng5, 0);
				else
					pScr->DrawArrow(x1, y1, x2, y2, color, pScr->data.zoom, kAng15 + kAng5, 0);
			}
		}
		
		pDb++;
	}
}

void CXTracker::Cleanup()
{
	if (pList)
		Track(src.type, src.index, tx);
}

char CXTracker::HaveObjects()
{
	return (pList != NULL);
}