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
#ifndef __PREVIEW_H
#define __PREVIEW_H
#include "db.h"
#include "eventq.h"
#include "xmpsnd.h"
#include "xmpact.h"
#include "xmptrig.h"
#include "xmpfx.h"
#include "xmpgib.h"
#include "xmpror.h"
#include "nnexts.h"

enum {
kScrEffectQuake1            = 0,
kScrEffectQuake2            = 1, // for explosions
kScrEffectMax                  ,
};


struct PREVIEW_MODE_KEYS {
    unsigned int key            : 8;
    unsigned int reqShift       : 1;
    unsigned int reqCtrl        : 1;
    unsigned int reqAlt         : 1;
    unsigned int mode           : 3;    // 2d, 3d, both
};

class PREVIEW_MODE
{
    public:
        unsigned int triggerFlags       : 1;
        unsigned int translateObjects   : 1;
        unsigned int triggerStart       : 1;
        unsigned int forceStartPos      : 1;
        unsigned int enableSound        : 1;
        unsigned int modernMap          : 1;
        unsigned int enableMusic        : 1;
        unsigned int difficulty         : 3;
        unsigned int mode               : 3;
        unsigned int speed              : 8;
        unsigned int m1cmd              : 8;
        unsigned int m2cmd              : 8;
        unsigned int m3cmd              : 8;
        unsigned int missileType        : 8;
        unsigned int explosionType      : 8;
        unsigned int sectnum            : 14;
        unsigned int levelTime          : 32;
        DONEOFTOTAL kills, secrets;
        int scrEffects[kScrEffectMax];
        SNAPSHOT_MANAGER* pState;
        IniFile* pEpisode;
        TIMER timer;
        
        void Start();
        void Process();
        void Stop();
        
        void TriggerObjectProcess();
        int  TriggerObjectCheck(OBJECT* pObj);
        void PickupProcess(int x, int y, int z, int nSect);
        void ItemPickup(spritetype* pSpr);
        void DudeKill(spritetype* pSpr);
        char DudeMorph(spritetype* pSpr);
        void ThingKill(spritetype* pSpr, int nCmd);
        void MapMessage(int nCmd);
        char ShowMenu(void);
        void ShowStatus(void);
        char DoExplosion(void);
        char FireMissile(void);
        char SpawnSeq(void);
        
        char KeyCheck(char key, char ctrl, char alt, char shift);
        char KeyProcess(char key, char ctrl, char alt, char shift);
        
        void Init(IniFile* pIni, char* section);
        void Save(IniFile* pIni, char* section);
};


extern PREVIEW_MODE gPreview;
#endif