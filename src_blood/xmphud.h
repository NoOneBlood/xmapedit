/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Xmapedit HUD implementation for both 2d and 3d modes.
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

#ifndef __XMPHUD_H
#define __XMPHUD_H
#include "xmphudlg.h"
#include "xmpinput.h"
#include "xmp2dscr.h"

#define kHudFontPackMax 7

enum
{
kHudLayoutNone          = 0,
kHudLayoutFull          = 1,
kHudLayoutCompact       = 2,
kHudLayoutSlim          = 3,
kHudLayoutDynamic       = 4,
kHudLayoutMax              ,
};

enum
{
kHudFontMsg             = 0,
kHudFontCont,
kHudFontStat,
kHudFontLogo,
kHudFontMax,
};

extern char* gHudLayoutNames[kHudLayoutMax];
extern char* gHudFontNames[kHudFontPackMax];

#pragma pack(push, 1)
struct MAPEDIT_HUD_STATUS
{
    unsigned int id;
    char  text[128];
    char  color[2][2];
};

struct MAPEDIT_HUD_WINDOW
{
    unsigned int x1             : 16;
    unsigned int y1             : 16;
    unsigned int x2             : 16;
    unsigned int y2             : 16;
    unsigned int mx             : 16;
    unsigned int my             : 16;
    unsigned int wh             : 16;
    unsigned int hg             : 16;
    
    void Setup(int x1, int y1, int x2, int y2)
    {
        this->x1 = x1;          this->x2 = x2;
        this->y1 = y1;          this->y2 = y2;
        this->wh = x2-x1;       this->hg = y2-y1;
        this->mx = x1+(wh>>1);  this->my = y1+(hg>>1);
    }
    
    void GetCoords(int* x1, int* y1, int* x2, int* y2)
    {
        *x1 = this->x1;     *x2 = this->x2;
        *y1 = this->y1;     *y2 = this->y2;
    }
    
    char Inside(MAPEDIT_HUD_WINDOW* w, char anyPart = 0)
    {
        if (anyPart)
        {
            if ((x2 > w->x1 && x1 < w->x2))
            {
                if (y2 > w->y1 && y1 < w->y2)
                    return 1;
            }
            
            return 0;
        }
        else
        {
            if (!irngok(x1, w->x1, w->x2)) return 0;
            if (!irngok(y1, w->y1, w->y2)) return 0;
            if (!irngok(x2, w->x1, w->x2)) return 0;
            if (!irngok(y2, w->y1, w->y2)) return 0;
        }
        
        return 1;
        
    }
};

struct MAPEDIT_HUD_LOGO
{
    char header[32], text[32];
    signed int   icon           : 16;
    signed int   iconShade      :  8;
    unsigned int iconScale      : 10;
    unsigned int iconPlu        :  8;
    unsigned int headx          : 14;
    unsigned int heady          : 14;
    unsigned int textx          : 14;
    unsigned int texty          : 14;
    unsigned int iconx          : 14;
    unsigned int icony          : 14;
};

struct MAPEDIT_HUD_MSG
{
    char message[256];
    signed   int showTicks;
    unsigned int scrollTicks;
    unsigned int scrollDir;
    unsigned int textPos;
    unsigned int textLen;
};

struct MAPEDIT_HUD_TILE
{
    signed   int nTile          : 16;
    unsigned int nPlu           : 8;
    signed   int nShade         : 8;
    unsigned int inside         : 1;
    unsigned int size           : 10;
    unsigned int dx             : 14;
    unsigned int dy             : 14;
    unsigned int marg           : 8;
    unsigned int draw           : 1;
    
};

struct MAPEDIT_HUD_SCREEN
{
    SCREEN2D screen2D;
    unsigned int draw           : 1;
};

struct MAPEDIT_HUD
{
    unsigned int draw       : 1;
    unsigned int wide       : 1;
    unsigned int wide800    : 1;
    unsigned int slim       : 1;
    unsigned int lgw        : 10;
    unsigned int tw         : 10;
    unsigned int layout     : 5;
    unsigned int fontPack   : 5;
    VOIDLIST statlist;
    
    MAPEDIT_HUD()
    {
        MAPEDIT_HUD_STATUS temp;
        memset(&temp, 0, sizeof(temp));
        statlist.Init(sizeof(MAPEDIT_HUD_STATUS), &temp);
    }
    
    ~MAPEDIT_HUD()
    {
        statlist.Clear();
    }
    
    ROMFONT* fonts[kHudFontMax][2];
    MAPEDIT_HUD_WINDOW main;
    MAPEDIT_HUD_WINDOW extrastat;
    MAPEDIT_HUD_WINDOW message;
    MAPEDIT_HUD_WINDOW content;
    MAPEDIT_HUD_WINDOW coords;
    MAPEDIT_HUD_WINDOW status;
    MAPEDIT_HUD_WINDOW tilebox;
    MAPEDIT_HUD_WINDOW logo;
    MAPEDIT_HUD_WINDOW bottom;
    MAPEDIT_HUD_WINDOW grid2d;
    MAPEDIT_HUD_WINDOW zoom2d;
    MAPEDIT_HUD_WINDOW editScrBox;
    MAPEDIT_HUD_LOGO logoPrefs;
    MAPEDIT_HUD_MSG msgData;
    MAPEDIT_HUD_TILE tileInfo;
    MAPEDIT_HUD_SCREEN editScr;
    void SetFontPack(int nFontPack);
    void SetView(int x1, int y1, int x2, int y2);
    void SetView(int nLayout, int nFontSet);
    void UpdateView();
    void UpdateMask();
    void SetLogo(char* header = NULL, char* text = "", int nTile = -1, int nPlu = 0, int nShade = 0);
    void SetTile(int nTile = -1, int nPlu = 0, int nShade = 0);
    void SetMsg();
    void SetMsg(char *__format, ...);
    void SetMsgImp(int nTime, char *__format, ...);
    void DrawIt();
    void DrawMask();
    void DrawLogo();
    void DrawStatList();
    void DrawStatus();
    void DrawStatus(MAPEDIT_HUD_WINDOW* w, short lCol, short bCol);
    void DrawTile();
    void DrawEditScreen();
    void ClearContent();
    void ClearMessageArea();
    void PrintGrid2d();
    void PrintZoom2d();
    void PrintMessage();
    void PrintCoords();
    void PrintStats();
    
    MAPEDIT_HUD_STATUS* StatusFind(MAPEDIT_HUD_STATUS* pEntry)
    {
        MAPEDIT_HUD_STATUS* p = (MAPEDIT_HUD_STATUS*)statlist.First();
        while(p->id)
        {
            if (p->id == pEntry->id)
                return p;
            
            p++;
        }
        
        return NULL;
    }
    
    char StatusAdd(MAPEDIT_HUD_STATUS* pEntry)
    {
        MAPEDIT_HUD_STATUS* p;
        if ((p = StatusFind(pEntry)) != NULL)
        {
            memcpy(p, pEntry, sizeof(*p));
            return 2;
        }
        
        statlist.Add(pEntry);
        return 1;
    }
    
    char StatusRem(MAPEDIT_HUD_STATUS* pEntry)
    {
        MAPEDIT_HUD_STATUS* p;
        if ((p = StatusFind(pEntry)) != NULL)
        {
            statlist.Remove(p);
            return 1;
        }
        
        return 0;
    }
    
    void CenterText(int* x, int* y, int wh, int len, int hg, ROMFONT* pFont);
    inline ROMFONT* GetFont(int nWhich, int nType = 0) { return fonts[nWhich][nType]; }
    inline int Height()         { return main.hg; }
    inline int HeightNoLogo()   { return (main.hg-(tw>>1))+1; } 
    inline int Width()          { return main.wh; }
};
#pragma pack(pop)

extern MAPEDIT_HUD gMapedHud;
void hudSetLayout(MAPEDIT_HUD* pHud, int layoutType, MOUSE* pMouse = NULL);
void hudSetFontPack(MAPEDIT_HUD* pHud, int fontPack, MOUSE* pMouse = NULL);
#endif