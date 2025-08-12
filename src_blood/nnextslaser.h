/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024: Originally written by NoOne.
// A class that builds ray of tsprites similar to laser with support of sector
// intersection. This file provides functionality for kModernLaserGen types
// which is part of nnexts.cpp. More info at http://cruo.bloodgame.ru/xxsystem
//
//
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

#ifndef __NNEXTLASER_H
#define __NNEXTLASER_H

#include "common_game.h"
#include "db.h"
#include "screen.h"
#include "xmpmaped.h"

#define kLaserMaxColors     10

class LASER
{
    public:
        //-------------------------------------------------------------------
        static signed char colors[kLaserMaxColors][3];
        static unsigned int tile[2][kLaserMaxColors];
        static unsigned int size[2][3][256];
        static struct CAMERA
        {
            int x, y, z;
            int a, s, h;
        }
        cam;
        //-------------------------------------------------------------------
        unsigned int numpieces          : 32;
        unsigned int piecescapacity     : 32;
        unsigned int maxdist            : 16;
        unsigned int xrepeat            : 8;
        unsigned int yrepeat            : 8;
        unsigned int thickness          : 8;
        unsigned int color              : 4;
        unsigned int vertical           : 1;
        unsigned int direction          : 1;
        unsigned int nodudes            : 1;
        unsigned int noplayers          : 1;
        unsigned int nothings           : 1;
        unsigned int nomasks            : 1;
        unsigned int nodead             : 1;
        unsigned int hasignoreflags     : 1;
        unsigned int usehitscan         : 1;
        unsigned int troncehit          : 1;
        unsigned int troncelast         : 1;
        unsigned int strictcmd          : 1;
        unsigned int raydone            : 1;

        spritetype* pOwn; XSPRITE* pXOwn;
        BITSECTOR piecesectmap;

        struct LASERSCAN
        {
            int32_t srcx, srcy, srcz, srcs, srca, srch;
            int32_t hitx, hity, hitz;
            int16_t hitsprite;
        }
        newscan, oldscan;

        struct RAYPIECE
        {
            int32_t x, y, z;
            int16_t s;
            uint8_t w;
            uint8_t h;
        }
        *pieces;

        //-------------------------------------------------------------------
        static void GenerateTiles(char nID);
        static void GenerateTables();
        //-------------------------------------------------------------------
        LASER(spritetype* pLaser);
        ~LASER();
        void Init(spritetype* pLaser);
        //-------------------------------------------------------------------
        spritetype* Put(RAYPIECE* pPiece);
        //-------------------------------------------------------------------
        char PiecePutH(RAYPIECE* pPiece, int nDAng1, int nDAng2);
        char PiecePutV(RAYPIECE* pPiece, int nDAng1, int nDAng2);

        void PieceListAdd(RAYPIECE* pPiece);
        void PieceListGrow(int nBy = 16);
        //-------------------------------------------------------------------
        void RayCollectPiecesH(int nSect, int nFullDist, POSOFFS* pos);
        void RayCollectPiecesV(int nSect, int nDist, POSOFFS* pos);
        char RayShow(SCREEN2D* pScr);
        char RayShow(int nSect);
        //-------------------------------------------------------------------
        void Process();
        void ScanV(LASERSCAN* l);
        void ScanH(LASERSCAN* l);
        void SetState(char nState);
        int LookupRepeat(unsigned int* pSiz, int nDist, unsigned char = 255);
        #if 0
        char PieceBehindCam(RAYPIECE* pPiece);
        #endif
        char UpdateProperties();
        char SpriteAllowed(spritetype* pSpr);
        //-------------------------------------------------------------------
        inline void RayDelete()
        {
            memset(piecesectmap, 0, sizeof(piecesectmap));
            numpieces = 0;
        }

        inline int AreScansDifferent()
        {
            uint8_t* o = (uint8_t*)&oldscan;
            uint8_t* n = (uint8_t*)&newscan;

            int s = sizeof(oldscan);
            while(s-- && o[s] == n[s]);
            return s;
        }

        inline char Fade(int a, int b, int r)   { return a + mulscale30(b, Sin(gFrameClock * kAng360 / r)); }
        inline char CheckFlags(int flag)        { return ((pOwn->flags & flag) != 0);                       }
        inline int Distance(int dx, int dy)     { return ksqrt(dx*dx+dy*dy);                                }
        inline int GetTile()                    { return tile[vertical][color];                             }
};
#endif