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
