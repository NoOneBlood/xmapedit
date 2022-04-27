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

#include "common_game.h"
#include "gui.h"
#include "grdshd.h"
#include "misc.h"
#include "tile.h"
#include "screen.h"
#include "xmpstub.h"
#include "xmpmisc.h"

enum {
mrRight = mrUser,
mrBoth1,
mrBoth2,
mrLeft,
};

BOOL gResetHighlight = FALSE;
GRSHIGHLIGHT gHglt[kMaxHgltObjects];
short gHgltc = 0;

int grshShadeWalls(BOOL toggle) {
	
	schar shade = 0; 	  			static BYTE step = 4;
	schar sshade = 0;				static schar mshade = 64 - 1;
 	ushort stepc = 0;				static ushort stepRng = 1;
	static schar direction = -1;	static schar prevDir = -1;	
	int i = 0;
	
	switch (direction) {
		default: // start shade is a shade of first highlighted object
			sshade = gHglt[0].shade;
			break;
		case mrLeft: // start shade is a shade of last highlighted object
			sshade = gHglt[gHgltc - 1].shade;
			break;
		case mrBoth1: // start shade is a shade of middle highlighted object
			sshade = (schar) (gHgltc >> 1);
			if (sshade << 1 == gHgltc) sshade--;
			sshade = gHglt[sshade].shade;
			break;
	}

	sprintf(buffer, "Gradient shading [%d]", gHgltc);
	Window dialog(0, 0, 180, 180, buffer);

	Label *startShL = new Label(4, 10, "BASE SHADE. . . . . . . .");
	EditNumber *startShE = new EditNumber(130, 4, 40, 16, sshade);
	
	Label *stepShL = new Label(4, 30, "SHADE &STEP. . . . . . . .");
	EditNumber *stepShE = new EditNumber(130, 24, 40, 16, step);

	Label *maxShL = new Label(4, 50, "MAX   SHADE. . . . . . . .");
	EditNumber *maxShE = new EditNumber(130, 44, 40, 16, mshade);

	Label *stepRgL = new Label(4, 70, "OBJECT STEP. . . . . . .");
	EditNumber *stepRgE = new EditNumber(130, 64, 40, 16, stepRng);
	
	Label *text1 = new Label(10, 90, "Base shade greater than");
	Label *text2 = new Label(20, 100, "max leads to brighten");
	Label *text3 = new Label(67, 110, "objects.");
	
	Label *pbDir = new Label(5, 130, ". . . . . . . .DIRECTION. . . . . . . .");
	
	sprintf(buffer, "--->");
	int len = gfxGetTextLen(buffer, qFonts[0]) + 15;
	sprintf(buffer, " ");
	
	TextButton *pbRight = new TextButton(3, 140, len, 20, "--->", mrRight);
	pbRight->hotKey = '1';
	
	TextButton *pbBoth  = new TextButton(45, 140, len, 20, "<-->", mrBoth1);
	pbBoth->hotKey = '2';
	
	TextButton *pbBoth2  = new TextButton(87, 140, len, 20, "-><-", mrBoth2);
	pbBoth2->hotKey = '3';
	
	TextButton *pbLeft  = new TextButton(129, 140, len, 20, "<---", mrLeft);
	pbLeft->hotKey = '4';

	dialog.Insert(startShL);
	dialog.Insert(startShE);
	dialog.Insert(stepShL);
	dialog.Insert(stepShE);
	dialog.Insert(maxShL);
	dialog.Insert(maxShE);
	dialog.Insert(stepRgL);
	dialog.Insert(stepRgE);
	dialog.Insert(text1);
	dialog.Insert(text2);
	dialog.Insert(text3);
	dialog.Insert(pbDir);
	dialog.Insert(pbRight);
	dialog.Insert(pbBoth);
	dialog.Insert(pbBoth2);
	dialog.Insert(pbLeft);
	
	if (!toggle || direction < 0) {
		
		prevDir = direction;
		ShowModal(&dialog);
		direction = (schar)dialog.endState;
		
	} else if (++direction > mrLeft) {
		
		direction = mrRight;

	}
	
	startShE->value = ClipRange(startShE->value, -128, 64 - 1);
	maxShE->value = ClipRange(maxShE->value, -128, 64 - 1);
	stepShE->value = ClipHigh(abs(stepShE->value), 64 >> 1);
	stepRgE->value = ClipRange(stepRgE->value, 1, gHgltc - 1);
	
	mshade = (schar)maxShE->value;    step = (BYTE)stepShE->value;
	shade =  (schar)startShE->value;  stepRng = (ushort)stepRgE->value;
	sshade = shade;

	switch (direction) {
		case mrRight:
			for (i = 0; i < gHgltc; i++){
				gHglt[i].shade = shade;
				if (++stepc < stepRng) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}
			scrSetMessage("shade direction: --->");
			break;
		case mrLeft:
			for (i = gHgltc - 1; i >= 0; i--){
				gHglt[i].shade = shade;
				if (++stepc < stepRng) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}
			scrSetMessage("shade direction: <---");
			break;
		case mrBoth1:// to edges
			for (i = (gHgltc - 1) >> 1, step<<=1 ; i < gHgltc; i++){
				gHglt[i].shade = shade;
				if (++stepc < stepRng >> 1) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}

			for (i = (gHgltc - 1) >> 1, shade = (schar)startShE->value; i >= 0; i--){
				gHglt[i].shade = shade;
				if (++stepc < stepRng >> 1) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}
			
			step = (BYTE)stepShE->value; // reset back because it's static.
			scrSetMessage("shade direction: <-->");
			break;
		case mrBoth2: // to middle
			for (i = 0, step<<=1; i < gHgltc >> 1; i++){
				gHglt[i].shade = shade;
				if (++stepc < stepRng >> 1) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}
			
			for (i = gHgltc - 1, shade = (schar)startShE->value; i >= gHgltc >> 1; i--){
				gHglt[i].shade = shade;
				if (++stepc < stepRng >> 1) continue;
				else if (sshade > mshade) shade = (schar)ClipLow(shade - step, mshade);
				else shade = (schar)ClipHigh(shade + step, mshade);
				stepc = 0;
			}
			
			step = (BYTE)stepShE->value; // reset back because it's static.
			scrSetMessage("shade direction: -><-");
			break;
		case mrCancel:
			scrSetMessage("Gradient shade aborted by user");
			break;
		default:
			direction = prevDir;
			break;
	}
	
	if (direction >= mrRight && direction <= mrLeft) {
		grshUnhgltObjects(-1, FALSE);
		gResetHighlight = TRUE; // reset highlight to zero on next highlight attempt
		BeepOk();
	}

	return direction;
}

short grshHighlighted(int otype, int idx){
	
	if (gHgltc <= 0) return -1;
	for (short i = 0; i < kMaxHgltObjects; i++) {
		if (gHglt[i].type == otype && gHglt[i].idx == idx)
			return i;
		
	}
	return -1;
	
}

void grshUnhgltObjects(int hidx, BOOL erase) {

	for (int i = ClipLow(hidx, 0); i < kMaxHgltObjects; i++) {
		
		if (gHglt[i].idx < 0)
			continue;
		
		int idx = gHglt[i].idx;
		switch(gHglt[i].type){
			case OBJ_WALL:
			case OBJ_MASKED:
				wall[idx].picnum  = gHglt[i].picnum;
				wall[idx].shade   = gHglt[i].shade;
				break;
			case OBJ_FLOOR:
				sector[idx].floorpicnum = gHglt[i].picnum;
				sector[idx].floorshade  = gHglt[i].shade;
				break;
			case OBJ_CEILING:
				sector[idx].ceilingpicnum = gHglt[i].picnum;
				sector[idx].ceilingshade  = gHglt[i].shade;
				break;
		}
		
		if (!erase) continue;
		gHglt[i].type = -1;
		gHglt[i].idx  = -1;
		if (--gHgltc <= 0)
			break;
		
	}
	
}

BOOL grshAddObjects(schar otype, short idx) {
	
	BOOL ALLOW = FALSE;
	switch (otype) {
		case OBJ_WALL:
		case OBJ_MASKED:
			gHglt[gHgltc].picnum  = wall[idx].picnum;
			gHglt[gHgltc].shade   = wall[idx].shade;
			ALLOW = TRUE;
			break;
		case OBJ_FLOOR:
			if (sector[idx].floorstat & kSectParallax) {
				scrSetMessage("Floor must be non-parallaxed.");
				break;
			}
			gHglt[gHgltc].picnum  = sector[idx].floorpicnum;
			gHglt[gHgltc].shade  = sector[idx].floorshade;
			ALLOW = TRUE;
			break;
		case OBJ_CEILING:
			if (sector[idx].ceilingstat & kSectParallax) {
				scrSetMessage("Ceiling must be non-parallaxed.");
				break;
			}
			gHglt[gHgltc].picnum  = sector[idx].ceilingpicnum;
			gHglt[gHgltc].shade  = sector[idx].ceilingshade;
			ALLOW = TRUE;
			break;
	}
	
	if (ALLOW) {
		
		gHglt[gHgltc].type = otype;
		gHglt[gHgltc].idx  = idx;
		gHgltc++;
		
	}
	
	return ALLOW;
	
}

void grshHgltObjects(int idx) {
	
	int i = 0;
	if (gHgltc < 1)
		return;
	
	for (i = 0; i < gHgltc; i++) {
		if (gHglt[i].type < 0 || gHglt[i].idx < 0)
			continue;
		
		idx = gHglt[i].idx;
		switch(gHglt[i].type){
			case OBJ_WALL:
			case OBJ_MASKED:
				wall[idx].picnum  = (short)gSysTiles.grdshdBg;
				wall[idx].shade   = -127;
				break;
			case OBJ_FLOOR:
				sector[idx].floorpicnum = (short)gSysTiles.grdshdBg;
				sector[idx].floorshade  = -127;
				break;
			case OBJ_CEILING:
				sector[idx].ceilingpicnum = (short)gSysTiles.grdshdBg;
				sector[idx].ceilingshade  = -127;
				break;
		}
	}
}