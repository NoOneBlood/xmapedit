/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
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
#include "seq.h"


#define kSeqExt ".seq"
#define kSeqMaxFrames	1024
#define kSeqMemSize sizeof(Seq) + kSeqMaxFrames * sizeof(SEQFRAME)


struct SEQEDIT {
	
	POINT2D origin;
	SEQFRAME clipboard;
	char* filename;
	unsigned int clipboardOk	: 1;
	unsigned int edit3d			: 1;
	unsigned int asksave		: 1;
	unsigned int curPal			: 8;
	unsigned int rffID;

};

extern SEQEDIT gSeqEd;

void seqeditStart(char* seqFile);
void seqeditProcess();


void seqeditPrintFlags(int x, int y, char* buff, QFONT* pFont, int bcol = 1, int tcol = 0);
void seqeditObjectInit(BOOL cpy);
void seqeditObjectUpdate(SEQFRAME* pFrame);
void seqeditDrawTile(SEQFRAME *pFrame, int zoom, int *nTileArg, int nOctant);
void seqFrameSetTile(SEQFRAME* pFrame, int nTile);
void seqeditSetFrameDefaults(SEQFRAME* pFrame);
void seqeditNewAnim();
BOOL SEQSave();
BOOL SEQLoad(Seq* out);
#endif