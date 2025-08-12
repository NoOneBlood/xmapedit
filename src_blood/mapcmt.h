/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Xmapedit map commentary system for level designers.
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


#ifndef __MAPCMT_H
#define __MAPCMT_H

#include "common_game.h"
#include "xmp2dscr.h"

#define kCommentMaxLength 128
#define kCommentExt ".mct"
#define kCommentVersion 1

extern short cmthglt;

struct MAP_COMMENT
{
    int cx, cy;
    int tx, ty;
    int objType, objIdx;
    int width[3], heigh[3];
    char text[kCommentMaxLength];
    unsigned int foreColor          : 8;
    signed   int backColor          : 9;
    unsigned int thickTail          : 1;
    unsigned int fontID             : 4;
    unsigned int id                 : 8;
    unsigned int initRebind         : 1;
};

class MAP_COMMENT_MGR {

    enum {
    kFontSmall          = 3,
    kFontNormal         = 1,
    kFontLarge          = 5,
    };

    #define kDefaultCRC 0
    #define kDefaultFont kFontNormal
    #define kNoTail 0x7FFFFFF

    private:
        int CRC;
        int deFonts[3], cuFonts[10];
    public:
        MAP_COMMENT* comments;
        unsigned int commentsCount;

        MAP_COMMENT_MGR();
        int  LoadFromIni(IniFile* pFile);
        int  SaveToIni(IniFile* pFile);
        int  LoadFromIni(char* filename);
        int  SaveToIni(char* filename);
        int  GetCRC();
        void SetCRC(int nCRC);
        BOOL CompareCRC(int nCRC);
        void Clear(MAP_COMMENT* cmt);
        int  RebindMatching(int srcType, int srcIdx, int destType, int destIdx, BOOL once);
        void BindTo(int cmtID, int objType, int objIdx);
        int  IsBind(int objType, int objIdx);
        int  IsBind(int cmtID, int* objIdx = NULL);
        void Draw(SCREEN2D* pScr);
        int  ClosestToPoint(int nTresh, int x, int y, int zoome);
        void SetXYBody(int cmtID, int x, int y);
        void SetXYTail(int cmtID, int x, int y);
        void GetXYTail(int cmtID, int* x, int* y);
        void RemoveTail(int cmtID);
        void Format(MAP_COMMENT* cmt);
        void FormatGetByZoom(MAP_COMMENT* cmt, int zoome, int *wh = NULL, int *hg = NULL, int *font = NULL);
        int  ShowDialog(int xpos1, int ypos1, int xpos2, int ypos2, int cmtID = -1);
        int  ShowBindMenu(int cmtID, int xpos, int ypos);
        int  Add(MAP_COMMENT* cmt);
        int  Delete(MAP_COMMENT* cmt);
        void DeleteAll();
        void ResetAllTails();
        void Unbind(int cmtID);
        void UpdateTailCoords(MAP_COMMENT* cmt);
        void Cleanup();
};

extern MAP_COMMENT_MGR gCommentMgr;

#endif