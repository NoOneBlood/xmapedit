/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// Dynamic fire functions adapted for level editor.
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
#include "fire.h"
#include "tile.h"

char CoolTable[1024], FrameBuffer[17280], SeedBuffer[16][128];
int fireSize = 128, gDamping = 6;
bool fireInited = true;
char *gCLU;

void InitSeedBuffers(void)
{
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < fireSize; j += 2)
            SeedBuffer[i][j] = SeedBuffer[i][j+1] = (BYTE)rand();
}

void BuildCoolTable(void)
{
    for (int i = 0; i < 1024; i++)
        CoolTable[i] = ClipLow((i-gDamping) / 4, 0);
}

void CellularFrame(char *pFrame, int sizeX, int sizeY)
{
    int nSquare = sizeX * sizeY;
    unsigned char *pPtr1 = (unsigned char*)pFrame;
    while (nSquare--)
    {
        unsigned char *pPtr2 = pPtr1+sizeX;
        int sum = *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
        if (*(pPtr2+sizeX) > 96)
        {
            pPtr2 += sizeX;
            sum += *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
            sum >>= 1;
        }
        *pPtr1 = CoolTable[sum];
        pPtr1++;
    }
}

void DoFireFrame(void)
{
    int nRand = qrand()&15;
    for (int i = 0; i < 3; i++)
        memcpy(FrameBuffer+16896+i*128, SeedBuffer[nRand], 128);

    CellularFrame(FrameBuffer, 128, 132);
    BYTE *pData = tileLoadTile(2342);
	if (pData)
	{
		BYTE *pSource = (BYTE*)FrameBuffer;
		int x = fireSize;
		do
		{
			int y = fireSize;
			BYTE *pDataBak = pData;
			do
			{
				*pData = gCLU[*pSource];
				pSource++;
				pData += fireSize;
			} while (--y);
			pData = pDataBak + 1;
		} while (--x);
	}
}

void FireInit(void)
{
	// fire processing may ruin everything if there is no tile with proper size
	if (tilesizx[2342] != fireSize || tilesizy[2342] != fireSize)
	{
		if (tileLoadTile(2342)) tilePurgeTile(2342); 		// erase tile if it exists
		if (!tileAllocTile(2342, fireSize, fireSize, 0, 0)) // allocate new tile
		{
			fireInited = false;
			return;
		}
	}
	
	memset(FrameBuffer, 0, sizeof(FrameBuffer));
    BuildCoolTable();
	InitSeedBuffers();
   
    DICTNODE *pNode = gSysRes.Lookup("RFIRE", "CLU");
    if (!pNode)
        ThrowError("RFIRE.CLU not found");
    
	gCLU = (char*)gSysRes.Lock(pNode);
    for (int i = 0; i < 100; i++)
        DoFireFrame();
}

void FireProcess(void)
{
	if (!fireInited)
		return;

	static int32_t lastUpdate = 0;
	if (totalclock < lastUpdate || lastUpdate + 2 < totalclock)
    {
        DoFireFrame();
        lastUpdate = totalclock;
    }
}