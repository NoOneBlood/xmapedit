/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Gradient shading feature.
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
#include "xmpmisc.h"
#include "grdshd.h"
#include "screen.h"

enum {
mrMin	= mrUser,
mrRight = mrMin,
mrBoth1,
mrBoth2,
mrLeft,
mrMax,
};

OBJECT_LIST gListGrd;

void processShade(OBJECT* pObj, int* nShade, int nMaxShade, int nStep, int nStepRange, int* nStepCnt)
{
	setShadeOf(*nShade, pObj->type, pObj->index);
	
	*nStepCnt = *nStepCnt + 1;
	if (*nStepCnt >= nStepRange)
	{
		*nShade = ClipRange(*nShade + nStep, -nMaxShade, nMaxShade);
		*nStepCnt = 0;
	}
}

int grshShadeWalls(char toggle)
{
	char buff[64], *pShadeStr;
	int nStep, nStepCnt, nShade, dwh, dhg;
	int nBaseShade, nHalf, tStp, tShd;
	int i = 0;
	
	static unsigned char nDir		= mrLeft;
	static signed   char nMaxShade	= NUMPALOOKUPS(1);
	static unsigned char nStepRange = 1;
	static unsigned char nShadeStep	= 4;
	
	int nLen = gListGrd.Length();
	OBJECT* pDb  = gListGrd.Ptr();
	OBJECT* pObj = pDb;
	
	nHalf = nLen >> 1;
	if ((nLen % 2) == 0)
		nHalf--;
	
	switch (nDir)
	{
		case mrLeft:	i = nLen-1;		break;
		case mrBoth1:	i = nHalf;		break;
	}
	
	pObj = gListGrd.Ptr(i);
	nBaseShade = getShadeOf(pObj->type, pObj->index);
	
	sprintf(buff, "Gradient shading [%d]", nLen);
	Window dialog(0, 0, 180, 180, buff);
	dialog.getSize(&dwh, &dhg);

	Label *startShL = new Label(4, 10, "BASE SHADE. . . . . . . .");
	EditNumber *startShE = new EditNumber(130, 4, 40, 16, nBaseShade, 0, -128, NUMPALOOKUPS(1));
	
	Label *stepShL = new Label(4, 30, "SHADE &STEP. . . . . . . .");
	EditNumber *stepShE = new EditNumber(130, 24, 40, 16, nShadeStep, 0, 1, NUMPALOOKUPS(1)>>1);

	Label *maxShL = new Label(4, 50, "MAX   SHADE. . . . . . . .");
	EditNumber *maxShE = new EditNumber(130, 44, 40, 16, nMaxShade, 0, -128, NUMPALOOKUPS(1));

	Label *stepRgL = new Label(4, 70, "OBJECT STEP. . . . . . .");
	EditNumber *stepRgE = new EditNumber(130, 64, 40, 16, nStepRange, 0, 1, nLen-1);
	
	Text* pText = new Text(0, 90, dwh, pFont->height*3,
	"Base shade greater than\n"
	"max leads to brighten\n"
	"objects.",
	0x03);
	
	Label *pbDir = new Label(5, 130, ". . . . . . . .DIRECTION. . . . . . . .");
	
	int bwh = dwh>>2;
	TextButton *pbRight		= new TextButton(3, 140, bwh, 20, "--->", mrRight);
	TextButton *pbBoth		= new TextButton(45, 140, bwh, 20, "<-->", mrBoth1);
	TextButton *pbBoth2		= new TextButton(87, 140, bwh, 20, "-><-", mrBoth2);
	TextButton *pbLeft		= new TextButton(129, 140, bwh, 20, "<---", mrLeft);

	dialog.Insert(startShL);
	dialog.Insert(startShE);
	dialog.Insert(stepShL);
	dialog.Insert(stepShE);
	dialog.Insert(maxShL);
	dialog.Insert(maxShE);
	dialog.Insert(stepRgL);
	dialog.Insert(stepRgE);
	dialog.Insert(pText);
	dialog.Insert(pbDir);
	dialog.Insert(pbRight);
	dialog.Insert(pbBoth);
	dialog.Insert(pbBoth2);
	dialog.Insert(pbLeft);
	
	if (!toggle)
	{
		ShowModal(&dialog);
		if (dialog.endState == mrCancel)
			return mrCancel;
		
		// find a button we are focusing on
		Widget* pFocus = ((Container*)dialog.focus)->focus;
		
		if (pFocus == pbRight)			nDir = pbRight->result;
		else if (pFocus == pbBoth)		nDir = pbBoth->result;
		else if (pFocus == pbBoth2) 	nDir = pbBoth2->result;
		else if (pFocus == pbLeft)		nDir = pbLeft->result;
		// else use previous
	}
	else if (++nDir > mrLeft)
	{
		nDir = mrRight;
	}
	
	
	nMaxShade = maxShE->value;			nShadeStep = stepShE->value;
	nShade = startShE->value;			nStepRange = stepRgE->value;
	tShd = nShade;						nBaseShade 	= nShade;
	nStepCnt = 0;						tStp		= nStepCnt;
	
	nStep = (nBaseShade > nMaxShade) ? -nShadeStep : nShadeStep;
	
	switch (nDir)
	{
		case mrRight:
			for (i = 0; i < nLen; i++)
				processShade(&pDb[i], &nShade, nMaxShade, nStep, nStepRange, &nStepCnt);
			
			pShadeStr = pbRight->text;
			break;
		case mrLeft:
			for (i = nLen-1; i >= 0; i--)
				processShade(&pDb[i], &nShade, nMaxShade, nStep, nStepRange, &nStepCnt);
			
			pShadeStr = pbLeft->text;
			break;
		case mrBoth1:// from middle to edges
			nStep<<=1;
			for (i = nHalf; i >= 0; i--)
			{
				processShade(&pDb[i], &tShd, nMaxShade, nStep, nStepRange, &tStp);
				processShade(&pDb[nLen-i-1], &nShade, nMaxShade, nStep, nStepRange, &nStepCnt);
			}
			pShadeStr = pbBoth->text;
			break;
		case mrBoth2: // from edges to middle
			nStep<<=1;
			for (i = 0; i <= nHalf; i++)
			{
				processShade(&pDb[i], &tShd, nMaxShade, nStep, nStepRange, &tStp);
				processShade(&pDb[nLen-i-1], &nShade, nMaxShade, nStep, nStepRange, &nStepCnt);
			}
			pShadeStr = pbBoth2->text;
			break;
	}
	
	BeepOk();
	scrSetMessage("Shade direction: %s", pShadeStr);
	return nDir;
}