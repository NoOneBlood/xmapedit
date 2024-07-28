/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// A lite version sound.cpp, ambsound.cpp and sfx.cpp from Nblood merged and adapted for level editor's Preview Mode
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

#ifndef __XMPSND
#define __XMPSND
#include "music.h"
#include "fx_man.h"
#include "multivoc.h"

#define kMaxAmbChannel 64
#define kSampleChannelMax 32

struct SAMPLE2D
{
    int hVoice;
    char at4;
    DICTNODE *at5;
}; // 9 bytes

struct SFX
{
    int relVol;
    int pitch;
    int pitchRange;
    int format;
    int loopStart;
    char rawName[9];
};

struct BONKLE
{
    int lChan; // in terms of audio, left channel is main channel when using mono
    int rChan;
    DICTNODE* hSnd;
    int sfxId;
    spritetype* pSndSpr;
    int chanId;
    int pitch;
    int vol;
    POINT3D curPos;
    POINT3D oldPos;
    int sectnum;
    int format;
};

struct AMB_CHANNEL
{
    int at0;
    int at4;
    int at8;
    DICTNODE *atc;
    char *at10;
    int at14;
    int at18;
};

extern BONKLE Bonkle[256];
extern BONKLE* BonkleCache[256];
extern int FXVolume;
extern int MusicVolume;
extern int CDVolume;
extern int NumVoices;
extern int NumChannels;
extern int NumBits;
extern int MixRate;
extern int ReverseStereo;
extern int MusicDevice;
extern int FXDevice;
extern char *MusicParams;

int sndGetRate(int format);
void sndPlaySong(char *songName, bool bLoop);
void sndStopSong(void);
void sndStartSample(char *pzSound, int nVolume, int nChannel = -1);
void sndStartSample(unsigned int nSound, int nVolume, int nChannel = -1, bool bLoop = false);
void sndStartWavID(unsigned int nSound, int nVolume, int nChannel = -1);
void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel = -1);
void sndKillAllSounds(void);
void sndProcess(void);
void sndTerm(void);
void sndInit(void);

void sndSetFXVolume(int nVolume);
void sndSetMusicVolume(int nVolume);

extern Resource gSoundRes;
extern bool sndActive;

void sfxInit(void);
void sfxPlay3DSound(int x, int y, int z, int soundId);
void sfxPlay3DSound(spritetype *pSprite, int soundId, int chanId = -1, int nFlags = 0);
void sfxPlay3DSoundCP(spritetype* pSprite, int soundId, int chanId = -1, int nFlags = 0, int pitch = 0, int volume = 0);
void sfxKill3DSound(spritetype *pSprite, int chanId = -1, int soundId = -1);
void sfxKillAllSounds(void);
void sfxUpdate3DSounds(void);
void sfxKillSoundInternal(int i);


void ambProcess(void);
void ambKillAll(void);
void ambInit(void);
#endif