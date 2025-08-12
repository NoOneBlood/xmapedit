/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// Intends for automatic arc creation
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
#ifndef __XMPARCWIZ_H
#define __XMPARCWIZ_H

#include "common_game.h"
#include "xmp2dscr.h"

extern NAMED_TYPE gAutoArcErrors[];

class SECTAUTOARC
{
    private:
        LINE2D* lines;
        signed   int status         : 32;
        unsigned int numpieces      : 16;
        signed   int sideStep       : 16;
        unsigned int midStep        : 16;
        signed   int destSector     : 32;
        signed   int srcWall        : 32;
        unsigned int fullDist       : 32;
        unsigned int stepDist       : 32;
        unsigned int midDist        : 32;
        //----------------------------------------------------------------------------
        void Start();
        void Stop();
        int SetupPrivate(int nWall, int nGrid);
        int WallSearch(int nSect, int nRadius, int x1, int y1, int x2 = 0, int y2 = 0);
        void WallClean(int nWall, char which);
        char LoopIsRect(int s, int e);
        int BuildArc();
        //----------------------------------------------------------------------------
    public:
        signed   int heightPerc     : 8;
        unsigned int arcFlags       : 8;
        unsigned int insertNew      : 1;
        //----------------------------------------------------------------------------
        IDLIST* slist;
        SECTAUTOARC()   { Start(); }
        ~SECTAUTOARC()  { Stop(); }
        char Setup(int nWall, int nGrid);
        int Insert();
        char IterateMidStep(int dir, int nGrid);
        char IterateSideStep(int dir, int nGrid);
        void Draw(SCREEN2D* pScr);
        inline int NumPieces()
        {
            return numpieces;
        }

        inline int StatusGet()
        {
            return status;
        }

        inline void ClearList()
        {
            if (slist)
                delete(slist);

            slist = new IDLIST(true);
        }
        //----------------------------------------------------------------------------

};

char dlgArcWizard(SECTAUTOARC* pArc);
#endif