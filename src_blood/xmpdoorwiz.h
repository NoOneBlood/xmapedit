/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// Intends for various automatic door sector creation
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
#ifndef __XMPDOORWIZ_H
#define __XMPDOORWIZ_H

#include "common_game.h"
#include "xmp2dscr.h"

extern NAMED_TYPE gDoorWizardErrors[];

char dlgDoorWizard();

class DOOR_ROTATE
{
    public:
        //-------------------------------------------
        signed   int status             : 32;
        //-------------------------------------------
        struct SIDEINFO
        {
            unsigned int doorSector     : 16;
            unsigned int available      : 1;
            unsigned int left           : 1;
            POINT2D point[5];
        };
        //-------------------------------------------
        struct
        {
            LINE2D line;
            SIDEINFO side[2];
            unsigned int twoSides       : 1;
            unsigned int canDraw        : 1;
            unsigned int inside         : 1;
            unsigned int solidWall      : 1;
            unsigned int dir            : 1;
            unsigned int mode           : 2;
            unsigned int parentSector   : 16;
            unsigned int nextSector     : 16;
            unsigned int angle          : 11;
            unsigned int width          : 32;
            unsigned int height         : 32;
            unsigned int trPush         : 1;
            unsigned int reverse        : 1;
            signed   int shade          : 8;
            signed   int rotateAngle    : 11;
        }
        info;
        //-------------------------------------------
        void Start();
        void Stop();
        char Setup(int nWall, int nWidth, int x, int y);
        char SetupPoints(POSOFFS pos, SIDEINFO* pSide);
        void SetupSector(SIDEINFO* pSide);
        void SetupWalls(SIDEINFO* pSide);
        void Draw(SCREEN2D* pScr);
        void Insert();
        //-------------------------------------------
        inline int StatusGet()      { return status; }
        inline void StatusClear()   { status = 0;    }
        //-------------------------------------------
        DOOR_ROTATE()   { Start(); }
        ~DOOR_ROTATE()  { Stop(); }

};


class DOOR_SLIDEMARKED
{
    public:
        //-------------------------------------------
        signed   int status             : 32;
        //-------------------------------------------
        struct
        {
            unsigned int type           : 2;
            unsigned int twoSides       : 1;
            unsigned int reverse        : 1;
            unsigned int trPush         : 1;
        }
        prefs;
        //-------------------------------------------
        struct
        {
            LINE2D line;
            WALLPOINT_INFO points[2][6];
            int srcWall[2];
            unsigned int twoSides       : 1;
            unsigned int canDraw        : 1;
            unsigned int angle          : 12;
            unsigned int width          : 32;
            unsigned int height         : 32;
            signed   int padding        : 32;
            unsigned int sector         : 16;
            unsigned int channel        : 10;
            unsigned int reqPoints      : 8;
        }
        info;
        //-------------------------------------------
        void Start(int nType, int nPad);
        void Stop();
        char Setup(int nWall, int nWidth, int x, int y);
        void SetupPoints(POSOFFS pos, int nSide, int nStartWal, int nAng = -1);
        void SetupWalls(int nStartWall, char reverse);
        void SetupSector(int nSect);
        void SetPadding(int nPad);
        void Draw(SCREEN2D* pScr);
        void Insert();
        //-------------------------------------------
        inline int StatusGet()                  { return status; }
        inline void StatusClear()               { status = 0;    }
        //-------------------------------------------
        DOOR_SLIDEMARKED(int nType, int nPad)   { Start(nType, nPad); }
        ~DOOR_SLIDEMARKED()                     { Stop(); }

};
#endif