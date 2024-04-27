/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Implementation of QAV animation files editor (QAVEDIT).
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

#ifndef __XMPQAVED
#define __XMPQAVED
#include "xmpqav.h"
#include "xmpinput.h"

class QAVEDIT
{
	#define kMaxLayers	8
	#define kMaxFrames  4096
	#define kAnimSize   sizeof(QAV) + (sizeof(FRAMEINFO)*kMaxFrames)
	#define kImportTimes 512

	private:
	#pragma pack(push, 1)
	struct TIME
	{
		int ticks;
		int factor;
		int count;
	};
	
	struct CLIPBOARD
	{
		FRAMEINFO	frame;
		unsigned int layer		: 4;
		unsigned int ok			: 1;
	};
	#pragma pack(pop)
	
	QAV        *pQav;
	FRAMEINFO  *pFrame;
	TILE_FRAME *pLayer;
	SOUNDINFO  *pSound;
	CLIPBOARD   cpbrd;
	TIME time;
	POINT2D origin;
	MOUSE mouse;
	unsigned int newQav			:  1;
	unsigned int edit3d			:  1;
	unsigned int playing		:  3;
	unsigned int frame			: 32;
	unsigned int layer			:  4;
	signed   int viewshade		:  8;
	unsigned int viewplu		:  8;
	signed   int CRC			: 32;
	signed   int rffID			: 32;
	char *pImportPath;
	public:
	// -------------------------------------
	void Start(char* filename = NULL);
	void ProcessLoop();
	void Quit();
	// -------------------------------------
	BOOL AnimNew();
	BOOL AnimLoad(char* filename, QAV** pOut = NULL);
	BOOL AnimSave(char* filename);
	void AnimImportDlg();
	void AnimDurationSet() { pQav->duration = pQav->nFrames * pQav->ticksPerFrame; }
	void AnimOriginSet(POINT2D* pOrigin, BOOL adjust);
	int  AnimOriginSelect();
	void AnimStopPlaying(BOOL reset = TRUE);
	// -------------------------------------
	FRAMEINFO* FrameGet(int nFrame = -1);
	void FrameInsert(int nFrame, bool after = true);
	void FrameDelete(int nFrame);
	void FrameClean(FRAMEINFO* pFrame);
	void FrameDraw(FRAMEINFO* pFrame);
	// -------------------------------------
	TILE_FRAME* LayerGet(int nFrame = -1, int nLayer = -1);
	void LayerDelete(int nFrame, int nLayer);
	void LayerClean(int nFrame, int nLayer);
	void LayerHighlight(int nFrame, int nLayer);
	void LayerClip(int nFrame, int nLayer);
	void LayerSetTile(int nFrame, int nLayer, int nTile = -1);
	// -------------------------------------
	SOUNDINFO* SoundGet(int nFrame = -1);
	// -------------------------------------
};


extern QAVEDIT gQaved;
#endif