/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025: Originally written by NoOne.
// Loop split feature.
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
#ifndef __XMPLOOPS_H
#define __XMPLOOPS_H
#include "common_game.h"
#include "xmp2dscr.h"

extern NAMED_TYPE gLoopSplitErrors[];

class LOOPSPLIT
{
    private:
        VOIDLIST* list;
        signed   int status         : 32;
        signed   int srcwall        : 32;
        signed   int destsector     : 32;
        signed   int angoffs        : 11;
        unsigned int skipflag       : 1;
        unsigned int cross          : 2;
        int SetupPrivate(int nWall);
        void Start();
        void Stop();
    public:
        LOOPSPLIT()                     { Start(); }
        ~LOOPSPLIT()                    { Stop(); }
        char Setup(int nWall);
        int  Insert();
        void Draw(SCREEN2D* pScr);
        char IterateAngleOffset(char dir);
        inline int StatusGet()          { return status; }
        inline char ToggleSkipFlag()    { return ((skipflag = !skipflag) > 0); }

};
#endif