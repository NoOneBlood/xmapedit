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
#ifndef SEQ_H
#define SEQ_H

enum {
kSeqLoop        = 0x01,
kSeqRemove      = 0x02,
};

struct SEQFRAME {
    unsigned int tile               : 12;
    unsigned int transparent        : 1; // transparent
    unsigned int transparent2       : 1; // transparent
    unsigned int blockable          : 1; // blockable
    unsigned int hittable           : 1; // hittable
    unsigned int xrepeat            : 8; // xrepeat
    unsigned int yrepeat            : 8; // yrepeat
    signed int shade                : 8; // shade
    unsigned int pal                : 5; // palette
    unsigned int trigger            : 1; //
    unsigned int smoke              : 1; //
    unsigned int autoaim            : 1; //
    unsigned int pushable           : 1; //
    unsigned int playSound          : 1; //
    unsigned int invisible          : 1; // invisible
    unsigned int xflip              : 1; //
    unsigned int yflip              : 1; //
    unsigned int tile2              : 4;
    unsigned soundRange             : 4; // (by NoOne) random sound range relative to global SEQ sound
    unsigned surfaceSound           : 1; // (by NoOne) trigger surface sound when moving / touching
    unsigned reserved               : 2;
};

struct Seq {
    char signature[4];
    short version;
    short nFrames; // at6
    short ticksPerFrame;
    short nSoundID;
    int flags;
    SEQFRAME frames[];
    void Preload(void);
    void Precache(void);
};

struct ACTIVE
{
    unsigned char type;
    unsigned short xindex;
};

struct SEQINST
{
    DICTNODE *hSeq;
    Seq *pSequence;
    int nSeq;
    int nCallbackID;
    short timeCount;
    unsigned char frameIndex;
    char isPlaying;
    void Update(ACTIVE *pActive);
};

int seqGetTile(SEQFRAME* pFrame);
int seqRegisterClient(void(*pClient)(int, int));
void seqPrecacheId(int id);
SEQINST* GetInstance(int nType, int nXIndex);
void UnlockInstance(SEQINST *pInst);
void seqSpawn(int nSeq, int nType, int nXIndex, int nCallbackID = -1);
void seqKill(int nType, int nXIndex);
void seqKillAll(void);
int seqGetStatus(int nType, int nXIndex);
int seqGetID(int nType, int nXIndex);
void seqProcess(int nTicks);

void UpdateFloor(int nXSector, SEQFRAME *pFrame);
void UpdateCeiling(int nXSector, SEQFRAME *pFrame);
void UpdateMasked(int nXWall, SEQFRAME *pFrame);
void UpdateWall(int nXWall, SEQFRAME *pFrame);
void UpdateSprite(int nXSprite, SEQFRAME *pFrame);
#endif