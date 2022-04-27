/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Game GUI based directory explorer and it's variations.
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
#ifndef __XMPEXPLO
#define __XMPEXPLO
enum {
kDirExpTypeOpen 	= 0x0000,
kDirExpTypeSave 	= 0x0001,
};

enum {
kDirExpNone			= 0x0000,
kDirExpAllowRff		= 0x0001,
kDirExpBak			= 0x0002,
kDirExpMulti 		= 0x0004,
kDirExpMultiOrder	= 0x0008,
kDirExpNoDir		= 0x0010,
};

enum {
kDirExpFile			= 0x0001,	// normal file
kDirExpDir			= 0x0002,	// directory
kDirExpInside		= 0x0004,	// inside archive
kDirExpFileBak		= 0x0008,	// BAK file
};

struct DIR_EXPLORER
{
	unsigned int sel		: 1;
	unsigned int selid		: 10;
	unsigned int attr		: 8;
	char  fullname[32];
	char  basename[32];
	char  ext[32];
};

char* dirBrowse(char* title, char* path, char* ext, BYTE dlgType = kDirExpTypeOpen, char flags = kDirExpAllowRff);
int dirBrowseGetFiles(char* path, char* reqExt, DIR_EXPLORER* out, int* dirs, int* files, char flags = 0);
BOOL enumSelected(int* idx, char* out);
int countSelected();
#endif