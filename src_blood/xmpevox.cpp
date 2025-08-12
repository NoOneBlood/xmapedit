/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// External voxels reader.
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

#include "xmpconf.h"
#include "xmpevox.h"
#include "tile.h"

char* extVoxelPath[kMaxTiles]; // these are dynamically allocated!

void extVoxInit()
{
    int i, j; short voxid = kMaxVoxels - 1;
    int nView, nPrevNode = -1;
    char* key, *value;

    extVoxUninit();

    // read external voxels info
    if (gMisc.externalModels && fileExists(gPaths.voxelEXT))
    {
        IniFile* pExtVox = new IniFile(gPaths.voxelEXT, INI_SKIPCM|INI_SKIPZR);
        while (pExtVox->GetNextString(NULL, &key, &value, &nPrevNode))
        {
            if (!isIdKeyword(key, "Tile", &i) || !rngok(i, 0, kMaxTiles)) continue;
            else if (!tilesizx[i] || !tilesizy[i]) continue;
            else
            {
                if (value[0] == '*')
                {
                    // force kSprViewVoxSpin flag
                    nView = kSprViewVoxSpin;
                    value = &value[1];
                }
                else
                {
                    nView = kSprViewVox;
                }

                if (!fileExists(value)) continue;
                if ((extVoxelPath[i] = (char*)malloc(strlen(value)+1)) == NULL)
                    break;

                sprintf(extVoxelPath[i], "%s", value);
                panm[i].view    = nView;
            }

            for (j = 0; j < kMaxTiles; j++)
            {
                if (voxid < 0) { ThrowError("Max(%d) models reached!", kMaxVoxels); }
                else if (tiletovox[j] == voxid)
                    voxid--, j = 0;
            }

            //voxelIndex[i] = voxid;  // overrides RFF voxels
            tiletovox[i]  = voxid;
        }

        delete(pExtVox);
    }
}



void extVoxUninit()
{
    for (int i = 0; i < kMaxTiles; i++)
    {
        panm[i].view = viewType[i];
        tiletovox[i] = -1;

        if (extVoxelPath[i])
        {
            free(extVoxelPath[i]);
            extVoxelPath[i] = NULL;
        }
    }
}