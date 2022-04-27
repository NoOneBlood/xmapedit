/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version of mirrors.cpp adapted for level editor's Preview Mode
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


#include "compat.h"
#include "baselayer.h"
#include "common_game.h"
#include "db.h"
#include "gameutil.h"
#include "tile.h"
#include "trig.h"
#include "misc.h"
#include "xmpror.h"
#include "xmpview.h"
#include "xmpwarp.h"

int mirrorcnt, mirrorsector, mirrorwall[4];
short mirrorTextureWidth = 0;
short mirrorTextureHeight = 0;
int mirrorLabelPic = -1;


typedef struct
{
    int at0;
    int at4;
    int at8;
    int atc;
    int at10;
    int at14;
} MIRROR;

MIRROR mirror[kMaxROR];

void InitMirrors(void)
{

    int i;
	mirrorcnt = 0;
	
	// save size to restore when stop previewing the map
 	mirrorTextureWidth = tilesizx[504];
	mirrorTextureHeight = tilesizy[504];  
	tilesizx[504] = tilesizy[504] = 0;
	
	// use dynamic location of ROR pics (see tileInitSystemTiles())
	/////////////////
	mirrorLabelPic = gSysTiles.editorMirrorStart;
	if (mirrorLabelPic == -1)
		mirrorLabelPic = 504;

	for( i = 0; i < kMaxROR; i++)
	{
		tilesizx[i + mirrorLabelPic] = 0;
		tilesizy[i + mirrorLabelPic] = 0;
	}
	/////////////////////////////////////////
	
    for (int i = numwalls - 1; i >= 0; i--)
    {
        if (mirrorcnt == kMaxROR) break;
		if (wall[i].overpicnum == 504)
        {
            if (wall[i].extra > 0 && wall[i].lotag == kWallStack)
            {
                wall[i].overpicnum = (short)(mirrorLabelPic + mirrorcnt);
                mirror[mirrorcnt].at14 = i;
                mirror[mirrorcnt].at0 = 0;
                wall[i].cstat |= 32;
                int j, tmp = xwall[wall[i].extra].data;
                for (j = numwalls - 1; j >= 0; j--)
                {
                    if (j == i)
                        continue;
                    if (wall[j].extra > 0 && wall[j].lotag == kWallStack)
                    {
                        if (tmp != xwall[wall[j].extra].data)
                            continue;
                        wall[i].hitag = j;
                        wall[j].hitag = i;
                        mirror[mirrorcnt].at4 = j;
                        break;
                    }
                }
                if (j < 0)
                    ThrowError("wall[%d] has no matching wall link! (data=%d)\n", i, tmp);
                mirrorcnt++;
            }
            continue;
        }
		
        if (wall[i].picnum == 504)
        {
            mirror[mirrorcnt].at4 = i;
            mirror[mirrorcnt].at14 = i;
            wall[i].picnum = (short)(mirrorLabelPic + mirrorcnt);
            mirror[mirrorcnt].at0 = 0;
            wall[i].cstat |= 32;
            mirrorcnt++;
            continue;
        }
    }
	
    for (int i = numsectors - 1; i >= 0; i--)
    {
        if (mirrorcnt >= 15)
            break;

        if (sector[i].floorpicnum == 504)
        {
            int nLink = gUpperLink[i];
            if (nLink < 0)
                continue;
            int nLink2 = sprite[nLink].owner /*& 0xfff*/;
            int j = sprite[nLink2].sectnum;
            if (sector[j].ceilingpicnum != 504)
                ThrowError("Lower link sector %d doesn't have mirror picnum\n", j);
            mirror[mirrorcnt].at0 = 2;
            mirror[mirrorcnt].at8 = sprite[nLink2].x-sprite[nLink].x;
            mirror[mirrorcnt].atc = sprite[nLink2].y-sprite[nLink].y;
            mirror[mirrorcnt].at10 = sprite[nLink2].z-sprite[nLink].z;
            mirror[mirrorcnt].at14 = i;
            mirror[mirrorcnt].at4 = j;
            sector[i].floorpicnum = (short)(mirrorLabelPic + mirrorcnt);
            mirrorcnt++;
            mirror[mirrorcnt].at0 = 1;
            mirror[mirrorcnt].at8 = sprite[nLink].x-sprite[nLink2].x;
            mirror[mirrorcnt].atc = sprite[nLink].y-sprite[nLink2].y;
            mirror[mirrorcnt].at10 = sprite[nLink].z-sprite[nLink2].z;
            mirror[mirrorcnt].at14 = j;
            mirror[mirrorcnt].at4 = i;
            sector[j].ceilingpicnum = (short)(mirrorLabelPic + mirrorcnt);
            mirrorcnt++;
        }
    }
    mirrorsector = numsectors;
    for (int i = 0; i < 4; i++)
    {
        mirrorwall[i] = numwalls+i;
        wall[mirrorwall[i]].picnum = 504;
        wall[mirrorwall[i]].overpicnum = 504;
        wall[mirrorwall[i]].cstat = 0;
        wall[mirrorwall[i]].nextsector = -1;
        wall[mirrorwall[i]].nextwall = -1;
        wall[mirrorwall[i]].point2 = numwalls+i+1;
    }
    wall[mirrorwall[3]].point2 = mirrorwall[0];
    sector[mirrorsector].ceilingpicnum = 504;
    sector[mirrorsector].floorpicnum = 504;
    sector[mirrorsector].wallptr = mirrorwall[0];
    sector[mirrorsector].wallnum = 4;
}

void TranslateMirrorColors(int nShade, int nPalette)
{
	begindrawing();
	nShade = ClipRange(nShade, 0, 63);
    BYTE *pMap = (BYTE*)(palookup[nPalette] + (nShade<<8));
	BYTE *pFrame = (BYTE*)frameplace;
    int nPixels = xdim*ydim;
    for (int i = 0; i < nPixels; i++, pFrame++)
        *pFrame = pMap[*pFrame];
	enddrawing();
}

void DrawMirrors(int x, int y, int z, int a, int horiz)
{
    for (int i = mirrorcnt - 1; i >= 0; i--)
    {
        int nTile = mirrorLabelPic + i;
        if (!TestBitString(gotpic, nTile))
			continue;
        
            ClearBitString(gotpic, nTile);
            switch (mirror[i].at0) {
            case 0:
            {
                int nWall = mirror[i].at4;
                int nSector = sectorofwall(nWall);
                walltype *pWall = &wall[nWall];
                int nNextWall = pWall->nextwall;
                int nNextSector = pWall->nextsector;
                pWall->nextwall = mirrorwall[0];
                pWall->nextsector = mirrorsector;
                wall[mirrorwall[0]].nextwall = nWall;
                wall[mirrorwall[0]].nextsector = nSector;
                wall[mirrorwall[0]].x = wall[pWall->point2].x;
                wall[mirrorwall[0]].y = wall[pWall->point2].y;
                wall[mirrorwall[1]].x = pWall->x;
                wall[mirrorwall[1]].y = pWall->y;
                wall[mirrorwall[2]].x = wall[mirrorwall[1]].x+(wall[mirrorwall[1]].x-wall[mirrorwall[0]].x)*16;
                wall[mirrorwall[2]].y = wall[mirrorwall[1]].y+(wall[mirrorwall[1]].y-wall[mirrorwall[0]].y)*16;
                wall[mirrorwall[3]].x = wall[mirrorwall[0]].x+(wall[mirrorwall[0]].x-wall[mirrorwall[1]].x)*16;
                wall[mirrorwall[3]].y = wall[mirrorwall[0]].y+(wall[mirrorwall[0]].y-wall[mirrorwall[1]].y)*16;
                sector[mirrorsector].floorz = sector[nSector].floorz;
                sector[mirrorsector].ceilingz = sector[nSector].ceilingz;
                
				int cx, cy; short ca;
                if (wall[nWall].lotag == kWallStack)
                {
                     cx = x - (wall[pWall->hitag].x-wall[pWall->point2].x);
                     cy = y - (wall[pWall->hitag].y-wall[pWall->point2].y);
                     ca = a;
                }
                else
                {
                    preparemirror(x,y,z,a,horiz,nWall,mirrorsector,&cx,&cy,&ca);
                }

				drawrooms(cx, cy, z, ca, horiz, mirrorsector | kMaxSectors);
				viewProcessSprites(cx, cy, z, ca);
				drawmasks();

                if (wall[nWall].lotag != kWallStack)
					completemirror();
                if (wall[nWall].pal != 0 || wall[nWall].shade != 0)
                    TranslateMirrorColors(wall[nWall].shade, wall[nWall].pal);
                
				pWall->nextwall = nNextWall;
                pWall->nextsector = nNextSector;
                return;
            }
            case 1:
			case 2:
            {
			   int nSector = mirror[i].at4;
			   
				drawrooms(x+mirror[i].at8, y+mirror[i].atc, z+mirror[i].at10, a, horiz, nSector | kMaxSectors);
				viewProcessSprites(x+mirror[i].at8, y+mirror[i].atc, z+mirror[i].at10, a);
				drawmasks();

                for (i = 0; i < kMaxROR; i++)
					ClearBitString(gotpic, mirrorLabelPic + i);
                return;
            }
        }
    }
}