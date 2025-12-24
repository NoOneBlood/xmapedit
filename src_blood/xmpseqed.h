/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025: Originally written by NoOne.
// Implementation of SEQ animation files editor (SEQEDIT).
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
#ifndef __XMPSEQED
#define __XMPSEQED
#include "common_game.h"
#include "xmpstub.h"
#include "xmphudlg.h"
#include "seq.h"

class SEQEDIT
{
    protected:
        struct CLIPBOARD
        {
            SEQFRAME frame;
            unsigned int ok         : 1;
        }
        clipboard;
        
        struct REFINFO
        {
            uint32_t pic;
            uint8_t  pal, xsiz, ysiz;
            int32_t  z;
        }
        refinfo;
        
        Seq* pSeq;
        SEQFRAME* pFrame;
        
        MAPEDIT_HUD* pHud;
        XSECTOR* pViewXSect;
        POINT2D origin;
        OBJECT obj;
        
        unsigned int playing        : 3;
        unsigned int edit3d         : 1;
        
        unsigned int viewBorders    : 1;
        unsigned int viewOctant     : 4;
        unsigned int viewZ          : 4;
        unsigned int viewSurface    : 8;
        signed   int viewShade      : 8;
        unsigned int viewPal        : 8;
        
        unsigned int nFrame         : 32;
        unsigned int zoom           : 32;
        signed   int CRC            : 32;
        signed   int rffID          : 32;
        // -------------------------------------
        void ProcessLoop();
        void Quit();
        // -------------------------------------
        char AnimNew();
        char AnimLoad(char* filename, Seq** pOut = NULL);
        char AnimSave(char* filename);
        void AnimStartPlaying(char flags = 0x0);
        void AnimStopPlaying();
        // -------------------------------------
        void FrameClean(SEQFRAME* pFrame);
        void FrameDraw(SEQFRAME* pFrame);
        // -------------------------------------
        void HudShowInfo();
        void HudEditInfo();
        void HudUpdateStatus();
        // -------------------------------------
        void ObjectUpdate(char nType = 0);
        // -------------------------------------
        void SoundPlay(int nID);
        void SoundStop();
        // -------------------------------------
        inline int  GetDefaultZoom()        { return mulscale8(0x10000, perc2val(75, ydim-pHud->Height())); }
        inline void ObjectReset()           { ObjectUpdate(1); };

    public:
        // -------------------------------------
        void Start(char* filename = NULL);
        
};

extern DIALOG_ITEM dlgSeqedit[];
extern SEQEDIT gSeqed;
#endif