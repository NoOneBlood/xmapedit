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
#include <stdio.h>
#include <string.h>
#include "common_game.h"

#include "misc.h"

void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes)
{
    unsigned int i;
    char ch;
    if (!pRes || !*pRes || *((char*)*pRes) == 0)
        return NULL;
    for (i = 0; i < nBytes; i++)
    {
        ch = *((char*)*pRes);
        if(ch == 0 || ch == '\n')
            break;
        buffer[i] = ch;
        *pRes = ((char*)*pRes)+1;
    }
    if (*((char*)*pRes) == '\n' && i < nBytes)
    {
        ch = *((char*)*pRes);
        buffer[i] = ch;
        *pRes = ((char*)*pRes)+1;
        i++;
    }
    else
    {
        while (true)
        {
            ch = *((char*)*pRes);
            if (ch == 0 || ch == '\n')
                break;
            *pRes = ((char*)*pRes)+1;
        }
        if (*((char*)*pRes) == '\n')
            *pRes = ((char*)*pRes)+1;
    }
    if (i < nBytes)
        buffer[i] = 0;
    return *pRes;
}


bool FileRead(int hFile, void *buffer, unsigned int nBytes)
{
	return kread(hFile, buffer, nBytes) == nBytes;
}

bool FileWrite(int hFile, void *buffer, unsigned int nBytes)
{
	return write(hFile, buffer, nBytes) == nBytes;
}

bool FileLoad(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;

	dassert(buffer != NULL);

	hFile = kopen4load(fname, 0);

	if ( hFile == -1 )
		return FALSE;

	n = kread(hFile, buffer, size);
	kclose(hFile);

	return n == size;
}

bool FileSave(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;

	dassert(buffer != NULL);

	hFile = open(fname, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, _S_IWRITE);

	if ( hFile == -1 )
		return FALSE;

	n = write(hFile, buffer, size);
	close(hFile);

	return n == size;
}

unsigned int randSeed = 1;

unsigned int qrand(void)
{
    if (randSeed&0x80000000)
        randSeed = ((randSeed<<1)^0x20000004)|0x1;
    else
        randSeed = randSeed<<1;
    return randSeed&0x7fff;
}

void ChangeExtension(char *pzFile, const char *pzExt)
{
	char ext[_MAX_PATH];
	int const nLength = strlen(pzFile);
    char *pDot = pzFile+nLength;
	int i = 0;

	if (pzExt[0] != '.')
		ext[0] = '.', i++;
	
	sprintf(&ext[i], pzExt);
    for (i = nLength-1; i >= 0; i--)
    {
        if (pzFile[i] == '/' || pzFile[i] == '\\') break;
        else if (pzFile[i] == '.')
        {
            pDot = pzFile+i;
            break;
        }
    }
    *pDot = '\0';
    strcat(pDot, ext);
}



