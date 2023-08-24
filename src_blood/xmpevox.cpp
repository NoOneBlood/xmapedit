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

#include "inifile.h"
#include "xmpconf.h"
#include "xmpevox.h"
#include "xmpmisc.h"
#include "tile.h"

void extVoxInit()
{
	int voxid = kMaxVoxels - 1;
	int i, j; 
	
	extVoxUninit();
	
	// read external voxels info
	if (gMisc.externalModels && fileExists(gPaths.voxelEXT))
	{
		IniFile* pExtVox = new IniFile(gPaths.voxelEXT);
		char saved = '\0'; ININODE* prev = NULL;
		char* key = NULL; char* value; char* end = NULL;
		char tileTok[] = "tile"; int keylen = strlen(tileTok);
		int nView;
		
		while (pExtVox->GetNextString(NULL, &key, &value, &prev))
		{
			if (!key || !value || strlen(key) <= keylen)
				continue;

			end =& key[keylen], saved = *end, *end = '\0';
			if (stricmp(tileTok, key) != 0)
				continue;
				
			*end = saved, key =& key[keylen];
			if ((i = atoi(key)) < 0 || i >= kMaxTiles) continue;
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

				if (!fileExists(value))
					continue;
				
				panm[i].view = nView;
			}
			
			for (j = 0; j < kMaxTiles; j++)
			{
				if (voxid < 0) { ThrowError("Max(%d) models reached!", kMaxVoxels); }
				else if (tiletovox[j] == voxid)
					voxid--, j = 0;
			}
			
			gSysRes.AddExternalResource(value, voxid);
			voxelIndex[i] = voxid;  // overrides RFF voxels
			tiletovox[i]  = voxid;
		}
		
		delete(pExtVox);
	}
}


void extVoxUninit()
{
	int i = kMaxTiles;
	while(--i >= 0)
	{
		panm[i].view = viewType[i];
		tiletovox[i] = -1;
	}	
}