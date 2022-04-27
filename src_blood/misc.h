/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
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
#pragma once
#include <stdio.h>
#include <string.h>

void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes);
bool FileRead(int hFile, void *buffer, unsigned int nBytes);
bool FileWrite(int hFile, void *buffer, unsigned int nBytes);
bool FileLoad(char *fname, void *buffer, unsigned int size);
bool FileSave(char *fname, void *buffer, unsigned int size);
int FileLength(FILE *);
unsigned int qrand(void);
void ChangeExtension(char *pzFile, const char *pzExt);