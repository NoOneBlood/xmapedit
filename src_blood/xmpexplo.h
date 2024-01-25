/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Game GUI based directory explorer that supports thumbnail preview generation.
// Includes simple thumbnail cache system.
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
#ifndef __XMPEXPLO
#define __XMPEXPLO

enum
{
	kDirExpSave 		= 0x0001,	// select for save
	kDirExpMulti 		= 0x0002,	// select for multi-load
	kDirExpRff			= 0x0004,	// search in game rff when selecting for load and file not found on disk
	kDirExpNoModPtr		= 0x0008,	// do not modify path argument
	kDirExpNoKeepPath	= 0x0010,	// do not keep current path when aborting selection
	kDirExpNoFree		= 0x0020,	// do not realloc to selected files count
};

enum
{
	kDirExpFile			= 0x0001,	// normal file
	kDirExpDir			= 0x0002,	// directory
	kDirExpDisk			= 0x0004,	// disk
	kDirExpDirMask		= 0x0006,	// disk or directory
	kDirExpSelected		= 0x0008,	// selected
	kDirExpNoThumb		= 0x0010,	// has no thumbnail
};

char* dirBrowse(char* pAPath, char* pFilter, char* pTitle = NULL, int flags = 0);
char* browseOpen(char* pPath, char* pFilter, char* pTitle = NULL, int flags = 0);
char* browseOpenFS(char* pPath, char* pFilter, char* pTitle = NULL, int flags = 0);
char* browseOpenMany(char* pPath, char* pFilter, char* pTitle = NULL, int flags = 0);
char* browseSave(char* pPath, char* pFilter, char* pTitle = NULL, int flags = 0);
char dirBrowseEnumItems(int* idx, char* out, int which);
char dirBrowseEnumSelected(int* idx, char* out);
int dirBrowseCountItems(int which);
int dirBrowseCountSelected();
#endif