/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
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

extern "C" {
#include "a.h"
}

#include "common_game.h"
#include "gfx.h"
#include "screen.h"
#include "gui.h"
#include "inifile.h"
#include "xmpstr.h"
#include "xmpconf.h"
#include "xmpmisc.h"

PALETTE gamepal;
bool DacInvalid = true;
static char(*gammaTable)[256];
RGB curDAC[256];
RGB baseDAC[256];
static RGB fromDAC[256];
static RGB toRGB;
RGB *palTable[5];
static int curPalette;
static int curGamma;
int gGammaLevels;
char gFogMode = 0;
int32_t gBrightness;

char scrFindClosestColor(int red, int green, int blue)
{
    int dist = 0x7fffffff;
    int best;
    for (int i = 0; i < 256; i++)
    {
        int sum = (palette[i*3+1]-green)*(palette[i*3+1]-green);
        if (sum >= dist) continue;
        sum += (palette[i*3+0]-red)*(palette[i*3+0]-red);
        if (sum >= dist) continue;
        sum += (palette[i*3+2]-blue)*(palette[i*3+2]-blue);
        if (sum >= dist) continue;
        best = i;
        dist = sum;
        if (sum == 0)
            break;
    }
    return best;
}

void scrCreateStdColors(void)
{
	int i = 0, j = 0; RESHANDLE hRes = NULL;
	ININODE* prev = NULL; char* key = NULL, *value = NULL;
	IniFile* colors = NULL;
	RGB rgb;
		
	memset(gStdColor, 0, sizeof(gStdColor));

	if (fileExists(kStdPalDB)) colors = new IniFile(kStdPalDB);
	else if ((hRes = gGuiRes.Lookup((unsigned int)2, "INI")) != NULL)
		colors = new IniFile((BYTE*)gGuiRes.Load(hRes), hRes->size, kStdPalDB);
	else
		ThrowError("Standard color palette is not found!");
	

	while (i < LENGTH(gStdColor) && colors->GetNextString(NULL, &key, &value, &prev, "StandardColors"))
	{
		if (!key || !value || (j = atoi(key)) < 0 || j >= LENGTH(gStdColor))
			continue;
		
		rgb.r = (uint8_t)enumStrGetInt(0, value);
		rgb.g = (uint8_t)enumStrGetInt(1);
		rgb.b = (uint8_t)enumStrGetInt(2);
		gStdColor[j] = scrFindClosestColor(rgb.r, rgb.g, rgb.b);
		i++;
	}
	
	delete colors;

}

void gSetDacRange(int start, int end, RGB *pPal)
{
    // UNREFERENCED_PARAMETER(start);
    // UNREFERENCED_PARAMETER(end);
#if USE_POLYMOST
    if (getrendermode() == 0)
        return;
#endif
    {
        for (int i = start; i < start + end; i++)
        {
            palette[i*3+0] = pPal[i].r >> 2;
            palette[i*3+1] = pPal[i].g >> 2;
            palette[i*3+2] = pPal[i].b >> 2;
        }
        setbrightness(/*gBrightness >> 2*/0, palette, 0);
    }
}

void scrLoadPLUs(void)
{
	parallaxvisibility = 512;
	int i; RESHANDLE hFile = NULL;
	
	if (gFogMode)
    {
		hFile = gSysRes.Lookup("FOG", "FLU");
		if (hFile != NULL)
		{
			parallaxvisibility = 3072;
			palookup[0] = (BYTE *)gSysRes.Lock(hFile);
			for (i = 1; i < kPluMax; i++)
				palookup[i] = palookup[0];
		}
		return;
    }
	
	for (i = 0; i < kPluMax; i++)
	{
		hFile = gSysRes.Lookup(i, "PLU");
		if (hFile != NULL && gSysRes.Size(hFile) / 256 == 64)
		{
			palookup[i] = (BYTE*)gSysRes.Lock(hFile);
			palookup[i][255] = 255; // fix PLUs that remaps transparency  (kPluGrey is a fine example)
		}
		else
		{
			palookup[i] = palookup[0];
		}
	}
}

extern "C" unsigned char *transluc;

void scrLoadPalette(void)
{
	int i; RESHANDLE hFile = NULL;
    buildprintf("Loading palettes\n");
	for (i = 0; i < kPalMax; i++)
	{
		if ((hFile = gSysRes.Lookup(i, "PAL")) != NULL)
			palTable[i] = (RGB *)gSysRes.Lock(hFile);
		else if (i == kPal0) // do error only for first palette
			ThrowError("Error loading main palette.");
	}
	
    memcpy(palette, palTable[0], sizeof(palette));
	
	// copy normal palette here so img2tile functions works correct
	memcpy(gamepal, palTable[0], sizeof(PALETTE));
	
    numpalookups = 64;
    scrLoadPLUs();

    buildprintf("Loading translucency table\n");
    if ((hFile = gSysRes.Lookup("TRANS", "TLU")) == NULL)
        ThrowError("TRANS.TLU not found");
    
	transluc = (unsigned char*)gSysRes.Lock(hFile);
    fixtransluscence(transluc);

}

void scrSetMessage(char *__format, ...)
{
	SCRMESSAGE* pMsg = &gScreen.msg[0];
	register int i = LENGTH(gScreen.msg); char buffer[sizeof(pMsg->text)];
	
	va_list argptr; va_start(argptr, __format);
	vsprintf(buffer, __format, argptr);
	va_end(argptr);
	
	//if (strcmp(pMsg->text, buffer) != 0)
	//{
		gScreen.msgShowCur = ClipHigh(gScreen.msgShowCur + 1, ClipHigh(gScreen.msgShowTotal, i));
		while(--i > 0)
			gScreen.msg[i] = gScreen.msg[i-1];
		
		sprintf(pMsg->text, buffer);
	//}
	
	pMsg->time = totalclock+(gScreen.msgShowCur<<3)+gScreen.msgTime;
}

void scrDisplayMessage()
{
	if (!gScreen.msgShowCur)
		return;
		
	register int i; register int y = windowy1 + 1;
	QFONT* pFont = qFonts[gScreen.msgFont]; SCRMESSAGE* pMsg; 
	char nColor = kColorWhite;
	
	for (i = 0; i < gScreen.msgShowCur; i++)
	{
		pMsg = &gScreen.msg[i];
		if (totalclock < pMsg->time)
		{
			gfxPrinTextShadow(windowx1+1, y, clr2std(nColor), pMsg->text, pFont);
			y+=perc2val(140, pFont->height);
			nColor++;
		}
		else
		{
			gScreen.msgShowCur--;
			pMsg->time = 0;
		}
	}
}

void scrSetPalette(int palId, bool updDac)
{
    if (palId >= 0)
	{
		curPalette = palId;
		scrSetGamma(0);
	}
	
	if (updDac)
		scrSetDac();
}

void scrSetGamma(int nGamma)
{
    curGamma = nGamma;
    for (int i = 0; i < 256; i++)
    {
        baseDAC[i].r = gammaTable[curGamma][palTable[curPalette][i].r];
        baseDAC[i].g = gammaTable[curGamma][palTable[curPalette][i].g];
        baseDAC[i].b = gammaTable[curGamma][palTable[curPalette][i].b];
    }
    DacInvalid = 1;
}

void scrSetupFade(char red, char green, char blue)
{
    memcpy(fromDAC, curDAC, sizeof(fromDAC));
    toRGB.r = red;
    toRGB.g = green;
    toRGB.b = blue;
}

void scrSetupUnfade(void)
{
    memcpy(fromDAC, baseDAC, sizeof(fromDAC));
}

void scrFadeAmount(int amount)
{
	for (int i = 0; i < 256; i++)
	{
		curDAC[i].r = interpolate(fromDAC[i].r, toRGB.r, amount);
        curDAC[i].g = interpolate(fromDAC[i].g, toRGB.g, amount);
        curDAC[i].b = interpolate(fromDAC[i].b, toRGB.b, amount);
	}
	gSetDacRange(0, 256, curDAC);
}

void scrSetDac(void)
{
	if (DacInvalid)
		gSetDacRange(0, 256, baseDAC);
	DacInvalid = 0;
}

void scrSetDac2(unsigned char* dapal, unsigned char* dapalgamma)
{
    for (int i = 0; i < 256; i++)
    {
        dapal[i*3+0] = palTable[curPalette][i].r;
        dapal[i*3+1] = palTable[curPalette][i].g;
        dapal[i*3+2] = palTable[curPalette][i].b;
        dapalgamma[i*3+0] = baseDAC[i].r;
        dapalgamma[i*3+1] = baseDAC[i].g;
        dapalgamma[i*3+2] = baseDAC[i].b;
    }
}

void scrInit(void)
{
    curPalette = 0; curGamma = 0;
    buildprintf("Loading gamma correction table\n");
    DICTNODE *pGamma = gSysRes.Lookup("gamma", "DAT");
    if (!pGamma)
        ThrowError("Gamma table not found");
	
    gGammaLevels = pGamma->size / 256;
    gammaTable = (char(*)[256])gSysRes.Lock(pGamma);
}

void scrUnInit(bool engineUninit)
{
    memset(palookup, 0, sizeof(palookup));
	transluc = NULL;
    if (engineUninit)
        uninitengine();
}


void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits)
{
    resetvideomode();
    if (setgamemode(vidMode, XRes, YRes, nBits) < 0)
    {
        buildprintf("Failure setting video mode %dx%dx%d %s! Trying next mode...\n", XRes, YRes,
                    nBits, vidMode ? "fullscreen" : "windowed");

        int resIdx = 0;

        for (int i=0; i < validmodecnt; i++)
        {
            if (validmode[i].xdim == XRes && validmode[i].ydim == YRes)
            {
                resIdx = i;
                break;
            }
        }

        int const savedIdx = resIdx;
        int bpp = nBits;

        while (setgamemode(0, validmode[resIdx].xdim, validmode[resIdx].ydim, bpp) < 0)
        {
            buildprintf("Failure setting video mode %dx%dx%d windowed! Trying next mode...\n",
                        validmode[resIdx].xdim, validmode[resIdx].ydim, bpp);

            if (++resIdx == validmodecnt)
            {
                if (bpp == 8)
                    ThrowError("Fatal error: unable to set any video mode!");

                resIdx = savedIdx;
                bpp = 8;
            }
        }
    }
    
    scrSetPalette(curPalette);
    gfxSetClip(windowx1, windowy1, windowx2, windowy2);
}

unsigned char* pScrSave = NULL;
unsigned int nScrSaveSiz = 0;

void scrSave()
{
	nScrSaveSiz = bytesperline * ydim;
	
	if (pScrSave)
		free(pScrSave);
	
	pScrSave = (unsigned char*)malloc(nScrSaveSiz);
	if (pScrSave)
		memcpy(pScrSave, (void*)frameplace, nScrSaveSiz);
}

void scrRestore(char freeIt)
{
	int nPrevSiz = nScrSaveSiz;
	int nCurSiz = bytesperline * ydim;
	nCurSiz = ClipHigh(nCurSiz, nPrevSiz);
	
	if (pScrSave)
	{
		memcpy((void*)frameplace, pScrSave, nCurSiz);
		if (freeIt)
			free(pScrSave), pScrSave = NULL;
	}
	
	if (freeIt)
		nScrSaveSiz = 0;
}

void scrNextPage(void)
{
    nextpage();
}
