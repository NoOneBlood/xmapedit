/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A libraray of functions for Preview mode feature.
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

#include "eventq.h"
#include "xmpsnd.h"
#include "xmpact.h"
#include "xmptrig.h"
#include "xmpfx.h"
#include "xmpgib.h"
#include "xmpror.h"
#include "db.h"
#include "inifile.h"
#include "nnexts.h"


#define kDeleteReally 333

enum {
kScrEffectQuake1			= 0,
kScrEffectQuake2			= 1, // for explosions
kScrEffectMax			   	   ,
};

struct DONEOFTOTAL
{
	unsigned int done				: 32;
	unsigned int total				: 32;
};

class PREVIEW_MODE {
	public:
	unsigned int triggerFlags		: 1;
	unsigned int translateObjects	: 1;
	unsigned int triggerStart		: 1;
	unsigned int forceStartPos		: 1;
	unsigned int enableSound		: 1;
	unsigned int modernMap			: 1;
	unsigned int enableMusic		: 1;
	unsigned int difficulty			: 3;
	unsigned int mode				: 3;
	unsigned int palette			: 8;
	unsigned int speed				: 8;
	unsigned int m1cmd				: 8;
	unsigned int m2cmd				: 8;
	unsigned int m3cmd				: 8;
	unsigned int missileType		: 8;
	unsigned int explosionType		: 8;
	unsigned int sectnum			: 14;
	unsigned int oVisibility		: 12;
	unsigned int levelTime			: 32;
	DONEOFTOTAL kills, secrets;
	int scrEffects[kScrEffectMax];
	uint32_t ticks;
	IniFile* pEpisode;
	void Init(IniFile* pIni, char* section);
	void Save(IniFile* pIni, char* section);
};

struct PREVIEW_MODE_KEYS {
	unsigned int key			: 8;
	unsigned int reqShift		: 1;
	unsigned int reqCtrl		: 1;
	unsigned int reqAlt			: 1;
	unsigned int mode			: 3;	// 2d, 3d, both
};

extern PREVIEW_MODE gPreview;


BOOL previewCheckKey(int key);
void previewSaveState();
void previewLoadState();
void previewInitGameLite();
void previewStart();
void previewProcess();
BOOL previewProcessKeys();
void previewStop();
void previewShowStatus();
void previewGetTriggerObject();
void previewProcessSprites();
char previewMorphDude(spritetype* pSprite, XSPRITE* pXSprite);
void previewKillDude(spritetype* pSprite, XSPRITE* pXSprite);
void previewDestroyThing(spritetype* pSprite, XSPRITE* pXSprite, int cmd);
int previewMenuDifficulty();
int previewMenuGameMode();
BOOL previewMenuProcess();
BOOL previewDoExplode(int nExplode = 0);
BOOL previewFireMissile(int nMissile = kMissileFireball);
void previewSpawnSeq();
void previewDelSprite(int nSprite);
void previewMessage(char *__format, ...);
void previewCheckPickup();
void previewPickupItem(spritetype* pSprite, XSPRITE* pXSprite);

#define kHiddenSpriteLoc 196608 << 5
void hideSprite(spritetype *pSprite);
void unhideSprite(spritetype* pSprite);

inline char previewSpriteRemoved(spritetype* pSpr)
{
	if (pSpr->statnum >= kMaxStatus)										return 1;
	else if (pSpr->x == kHiddenSpriteLoc && pSpr->y == kHiddenSpriteLoc)	return 2;
	else																	return 0;
}