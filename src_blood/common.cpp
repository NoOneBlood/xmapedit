/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Originally written by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// Various functions and global definitions for XMAPEDIT
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

#include "build.h"
#include "baselayer.h"
#include "common_game.h"

PICANM* panm;
int gFrameClock;
int gFrameTicks;
int gFrame;
int gFrameRate;
int gGamma;

Resource gSysRes;

static const char *_module;
static int _line;
unsigned int randSeed = 1;

int costable[2048];
BYTE OctantTable[8] = { 5, 6, 2, 1, 4, 7, 3, 0 };

unsigned int qrand(void)
{
    if (randSeed&0x80000000)
        randSeed = ((randSeed<<1)^0x20000004)|0x1;
    else
        randSeed = randSeed<<1;
    return randSeed&0x7fff;
}

int GetOctant(int x, int y)
{
    int vc = klabs(x)-klabs(y);
    return OctantTable[7-(x<0)-(y<0)*2-(vc<0)*4];
}

void RotateVector(int *dx, int *dy, int nAngle)
{
    int ox = *dx;
    int oy = *dy;
    *dx = dmulscale30r(ox, Cos(nAngle), -oy, Sin(nAngle));
    *dy = dmulscale30r(ox, Sin(nAngle), oy, Cos(nAngle));
}

void RotatePoint(int *x, int *y, int nAngle, int ox, int oy)
{
    int dx = *x-ox;
    int dy = *y-oy;
    *x = ox+dmulscale30r(dx, Cos(nAngle), -dy, Sin(nAngle));
    *y = oy+dmulscale30r(dx, Sin(nAngle), dy, Cos(nAngle));
}

void trigInit(Resource &Res)
{
    register int i;
	DICTNODE *pTable = Res.Lookup("cosine","dat");
	if (!pTable)				ThrowError("Cosine table not found");
	if (pTable->size != 2048)	ThrowError("Cosine table incorrect size");	
    memcpy(costable, Res.Load(pTable), pTable->size);
#if B_BIG_ENDIAN == 1
    for (i = 0; i < 512; i++)			costable[i] = B_LITTLE32(costable[i]);
#endif
    costable[512] = 0;
    for (i = 513; i <= 1024; i++)		costable[i] = -costable[1024-i];
    for (i = 1025; i < 2048; i++)		costable[i] = costable[2048 - i];
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

bool FileLoad(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;

	dassert(buffer != NULL);
	if ((hFile = open(fname, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) < 0)
		return FALSE;
	
	n = read(hFile, buffer, size);
	close(hFile);

	return (n == size);
}

bool FileSave(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;

	dassert(buffer != NULL);
	if ((hFile = open(fname, O_CREAT|O_WRONLY|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE)) < 0)
		return FALSE;

	n = write(hFile, buffer, size);
	close(hFile);

	return (n == size);
}

void _SetErrorLoc(const char *pzFile, int nLine)
{
    _module = pzFile;
    _line = nLine;
}

void _ThrowError(const char *pzFormat, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, pzFormat);
    vsprintf(buffer, pzFormat, args);
    buildprintf("%s(%i): %s\n", _module, _line, buffer);
    wm_msgbox("Error", "%s(%i): %s\n", _module, _line, buffer);

    fflush(NULL);
    exit(0);
}


void __dassert(const char * pzExpr, const char * pzFile, int nLine)
{
    buildprintf("Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);
    wm_msgbox("Assertion failed", "Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);

    fflush(NULL);
    exit(0);
}
