/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by Nuke.YKT.
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
#include "common_game.h"
#include "tile.h"
#include "screen.h"
#include "db.h"

int qanimateoffs(short a1, short a2)
{
    int offset = 0;
    if (a1 >= 0 && a1 < kMaxTiles)
    {
        int frames = panm[a1].frames;
        if (frames > 0)
        {
            int vd;
            if ((a2&0xc000) == 0x8000)
                vd = (crc32once((unsigned char*)&a2, 2)+totalclock)>>panm[a1].speed;
            else
                vd = totalclock>>panm[a1].speed;
            switch (panm[a1].type)
            {
            case 1:
                offset = vd % (2*frames);
                if (offset >= frames)
                    offset = 2*frames-offset;
                break;
            case 2:
                offset = vd % (frames+1);
                break;
            case 3:
                offset = -(vd % (frames+1));
                break;
            }
        }
    }
    return offset;
}

void qloadpalette(void)
{
    scrLoadPalette();
}

int qgetpalookup(int a1, int a2)
{
    if (gFogMode)
        return ClipHigh(a1 >> 8, 15) * 16 + ClipRange(a2, 0, 15);
    else
        return ClipRange((a1 >> 8) + a2, 0, 63);
}

void qsetbrightness(unsigned char *dapal, unsigned char *dapalgamma)
{
    scrSetGamma(gGamma);
    scrSetDac(dapal, dapalgamma);
}

void qprintext(int x, int y, short col, short backcol, const char *name, char nFont)
{
    ROMFONT* pFont = &vFonts[nFont];

    if (backcol >= 0)
    {
        int l = strlen(name)*pFont->ls;
        gfxSetColor(backcol);
        gfxFillBox(x, y, x+l, y+pFont->lh);
    }

    printext2(x, y, col, (char*)name, pFont);
}

void HookReplaceFunctions(void)
{
    animateoffs_replace         = qanimateoffs;
    loadpalette_replace         = qloadpalette;
    getpalookup_replace         = qgetpalookup;
    initspritelists_replace     = qinitspritelists;
    insertsprite_replace        = qinsertsprite;
    deletesprite_replace        = qdeletesprite;
    changespritesect_replace    = qchangespritesect;
    changespritestat_replace    = qchangespritestat;
    loadvoxel_replace           = qloadvoxel;
    setbrightness_replace       = qsetbrightness;
    loadtile_replace            = qloadtile;
    printext_replace            = qprintext;
}
