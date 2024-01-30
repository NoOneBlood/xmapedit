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


#include "build.h"
#include "cache1d.h"
#include "baselayer.h"
#include "winlayer.h"
#include "resource.h"
#include "gameutil.h"
#include "xmpsnd.h"
#include "xmpconf.h"
#include "db.h"


////////////////
bool gStereo = true;
/////////////////////////////////////

POINT2D earL, earR, earL0, earR0; // Ear position
VECTOR2D earVL, earVR; // Ear velocity ?
int lPhase, rPhase, lVol, rVol, lPitch, rPitch, nBonkles = 0, nAmbChannels = 0;

BONKLE Bonkle[256];
BONKLE *BonkleCache[256];
AMB_CHANNEL ambChannels[kMaxAmbChannel];

/// SOUND //////////////////////////////////////////////////////

int FXVolume;
int MusicVolume;
int CDVolume;
int NumVoices;
int NumChannels;
int NumBits;
int MixRate;
int ReverseStereo;
int MusicDevice = -1;
int FXDevice = -1;
char *MusicParams;

Resource gSoundRes;

int soundRates[13] = {
    11025,
    11025,
    11025,
    11025,
    11025,
    22050,
    22050,
    22050,
    22050,
    44100,
    44100,
    44100,
    44100,
};

void SoundCallback(uintptr_t val)
{
    int *phVoice = (int*)val;
    *phVoice = 0;
}

void InitSoundDevice(void)
{
    int fxdevicetype;
    // if they chose None lets return
    if (FXDevice < 0) {
        return;
    } else if (FXDevice == 0) {
        fxdevicetype = ASS_AutoDetect;
    } else {
        fxdevicetype = FXDevice - 1;
    }
#ifdef _WIN32
    void *initdata = (void *)win_gethwnd(); // used for DirectSound
#else
    void *initdata = NULL;
#endif
    int nStatus;
    nStatus = FX_Init(fxdevicetype, NumVoices, &NumChannels, &NumBits, &MixRate, initdata);
    if (nStatus != FX_Ok)
        ThrowError(FX_ErrorString(nStatus));
    if (ReverseStereo == 1)
        FX_SetReverseStereo(!FX_GetReverseStereo());
    FX_SetVolume(FXVolume);
    nStatus = FX_SetCallBack(SoundCallback);
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void DeinitSoundDevice(void)
{
    if (FXDevice == -1)
        return;
    int nStatus = FX_Shutdown();
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void InitMusicDevice(void)
{
    int musicdevicetype;

    // if they chose None lets return
    if (MusicDevice < 0) {
       return;
    } else if (MusicDevice == 0) {
       musicdevicetype = ASS_AutoDetect;
    } else {
       musicdevicetype = MusicDevice - 1;
    }

    int nStatus = MUSIC_Init(musicdevicetype, MusicParams);
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
    // DICTNODE *hTmb = gSoundRes.Lookup("GMTIMBRE", "TMB");
    // if (hTmb)
    //     AL_RegisterTimbreBank((unsigned char*)gSoundRes.Load(hTmb));
    MUSIC_SetVolume(MusicVolume);
}

void DeinitMusicDevice(void)
{
    if (MusicDevice == -1)
        return;
    // FX_StopAllSounds();
    int nStatus = MUSIC_Shutdown();
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
}

bool sndActive = false;

int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}

SAMPLE2D Channel[kSampleChannelMax];

SAMPLE2D * FindChannel(void)
{
    for (int i = kSampleChannelMax-1; i >= 0; i--)
        if (Channel[i].at5 == 0)
            return &Channel[i];
    ThrowError("No free channel available for sample");
    return NULL;
}

char *pSongPtr = NULL;
void sndPlaySong(char *songName, bool bLoop)
{  
   DICTNODE *hSong;
   if (MusicDevice == -1 || !songName || (hSong = gSoundRes.Lookup(songName, "MID")) == NULL) return;
   else if (pSongPtr)
	   sndStopSong();
	
   if ((pSongPtr = (char *)malloc(hSong->size)) != NULL)
   {
		gSoundRes.Load(hSong, pSongPtr);
		MUSIC_SetVolume(MusicVolume);
		MUSIC_PlaySong(pSongPtr, hSong->size, bLoop);
   }
}

void sndSetMusicVolume(int nVolume)
{
    if (MusicDevice == -1)
        return;
    MusicVolume = nVolume;
    MUSIC_SetVolume(nVolume);
}

void sndSetFXVolume(int nVolume)
{
    if (FXDevice == -1)
        return;
    FXVolume = nVolume;
    FX_SetVolume(nVolume);
}

void sndStopSong(void)
{
	if (MusicDevice >= 0)
		MUSIC_StopSong();

	if (pSongPtr)
	{
		free(pSongPtr);
		pSongPtr = NULL;
	}
}


void sndKillSound(SAMPLE2D *pChannel);

void sndStartSample(char *pzSound, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    if (!strlen(pzSound))
        return;
    dassert(nChannel >= -1 && nChannel < kSampleChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pzSound, "RAW");
    if (!pChannel->at5)
        return;
    int nSize = pChannel->at5->size;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(1), 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kSampleChannelMax);
    DICTNODE *hSfx = gSoundRes.Lookup(nSound, "SFX");
    if (!hSfx)
        return;
    SFX *pEffect = (SFX*)gSoundRes.Lock(hSfx);
    dassert(pEffect != NULL);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!pChannel->at5)
        return;
    if (nVolume < 0)
        nVolume = pEffect->relVol;
    nVolume *= 80;
    nVolume = ClipRange(nVolume, 0, 255); // clamp to range that audiolib accepts
    int nSize = pChannel->at5->size;
    int nLoopEnd = nSize - 1;
    if (nLoopEnd < 0)
        nLoopEnd = 0;
    if (nSize <= 0)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    if (nChannel < 0)
        bLoop = false;
    if (bLoop)
    {
        pChannel->hVoice = FX_PlayLoopedRaw(pData, nSize, pData + pEffect->loopStart, pData + nLoopEnd, sndGetRate(pEffect->format),
            0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
        pChannel->at4 |= 1;
    }
    else
    {
        pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(pEffect->format), 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
}

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kSampleChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(nSound, "WAV");
    if (!pChannel->at5)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_PlayWAV(pData, pChannel->at5->size, 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndKillSound(SAMPLE2D *pChannel)
{
    if (pChannel->at4 & 1)
    {
        FX_EndLooping(pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
    FX_StopSound(pChannel->hVoice);
}

void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kSampleChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    int hFile = kopen4load(pzFile, 0);
    if (hFile == -1)
        return;
    int nLength = kfilelength(hFile);
    char *pData = (char*)gSoundRes.Alloc(nLength);
    if (!pData)
    {
        kclose(hFile);
        return;
    }
    kread(hFile, pData, kfilelength(hFile));
    kclose(hFile);
    pChannel->at5 = (DICTNODE*)pData;
    pChannel->at4 |= 2;
    pChannel->hVoice = FX_PlayWAV(pData, nLength, 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndKillAllSounds(void)
{
    if (FXDevice == -1)
        return;
    for (int i = 0; i < kSampleChannelMax; i++)
    {
        SAMPLE2D *pChannel = &Channel[i];
        if (pChannel->hVoice > 0)
            sndKillSound(pChannel);
        if (pChannel->at5)
        {
            if (pChannel->at4 & 2)
            {
#if 0
                free(pChannel->at5);
#else
                gSoundRes.Free(pChannel->at5);
#endif
                pChannel->at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(pChannel->at5);
            }
            pChannel->at5 = 0;
        }
    }
}

void sndProcess(void)
{
    if (FXDevice == -1)
        return;
    for (int i = 0; i < kSampleChannelMax; i++)
    {
        if (Channel[i].hVoice <= 0 && Channel[i].at5)
        {
            if (Channel[i].at4 & 2)
            {
                gSoundRes.Free(Channel[i].at5);
                Channel[i].at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(Channel[i].at5);
            }
            Channel[i].at5 = 0;
        }
    }
}



void sndTerm(void)
{
    if (!sndActive)
        return;
    sndActive = false;
    sndStopSong();
    DeinitSoundDevice();
    DeinitMusicDevice();
}

void sndInit(void)
{
    pSongPtr = NULL;
    memset(Channel, 0, sizeof(Channel));
	if (!sndActive)
	{
		gSoundRes.Init(gPaths.soundRFF);
		InitSoundDevice();
		InitMusicDevice();
    }
	sndActive = true;
}

/// SFX //////////////////////////////////////////////////////
void sfxInit(void)
{
    for (int i = 0; i < 256; i++)
        BonkleCache[i] = &Bonkle[i];
    nBonkles = 0;
}

int Vol3d(int angle, int dist)
{
    return dist - mulscale16(dist, 0x2000 - mulscale30(0x2000, Cos(angle)));
}

void Calc3DValues(BONKLE *pBonkle)
{
    int dx = pBonkle->curPos.x - posx;
    int dy = pBonkle->curPos.y - posy;
    int dz = pBonkle->curPos.z - posz;
    int angle = getangle(dx, dy);
    dx >>= 4;
    dy >>= 4;
    dz >>= 8;
    int distance = ksqrt(dx*dx + dy * dy + dz * dz);
    distance = ClipLow((distance >> 2) + (distance >> 3), 64);
    int v14, v18;
    v14 = v18 = scale(pBonkle->vol, 80, distance);
    int sinVal = Sin(angle);
    int cosVal = Cos(angle);
    int v8 = dmulscale30r(cosVal, pBonkle->curPos.x - pBonkle->oldPos.x, sinVal, pBonkle->curPos.y - pBonkle->oldPos.y);

    int distanceL = approxDist(pBonkle->curPos.x - earL.x, pBonkle->curPos.y - earL.y);
    lVol = Vol3d(angle - (ang - 85), v18);
    int phaseLeft = mulscale16r(distanceL, pBonkle->format == 1 ? 4114 : 8228);
    lPitch = scale(pBonkle->pitch, dmulscale30r(cosVal, earVL.dx, sinVal, earVL.dy) + 5853, v8 + 5853);
    if (lPitch < 0 || lPitch > pBonkle->pitch * 4)
        lPitch = pBonkle->pitch;

    int distanceR = approxDist(pBonkle->curPos.x - earR.x, pBonkle->curPos.y - earR.y);
    rVol = Vol3d(angle - (ang + 85), v14);
    int phaseRight = mulscale16r(distanceR, pBonkle->format == 1 ? 4114 : 8228);
    rPitch = scale(pBonkle->pitch, dmulscale30r(cosVal, earVR.dx, sinVal, earVR.dy) + 5853, v8 + 5853);
    if (rPitch < 0 || rPitch > pBonkle->pitch * 4)
        rPitch = pBonkle->pitch;

    int phaseMin = ClipHigh(phaseLeft, phaseRight);
    lPhase = phaseRight - phaseMin;
    rPhase = phaseLeft - phaseMin;
}

void sfxPlay3DSound(int x, int y, int z, int soundId)
{
	if (FXDevice == -1 || soundId < 0 || !sndActive)
			return;
    
    DICTNODE *hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes)return;

    SFX *pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes) return;

    int v1c, v18;
    v1c = v18 = mulscale16(pEffect->pitch, sndGetRate(pEffect->format));
    if (nBonkles >= 256)
        return;
    BONKLE *pBonkle = BonkleCache[nBonkles++];
    pBonkle->pSndSpr = NULL;
    pBonkle->curPos.x = x;
    pBonkle->curPos.y = y;
    pBonkle->curPos.z = z;
    FindSector(x, y, z, &pBonkle->sectnum);
    pBonkle->oldPos = pBonkle->curPos;
    pBonkle->sfxId = soundId;
    pBonkle->hSnd = hRes;
    pBonkle->vol = pEffect->relVol;
    pBonkle->pitch = v18;
    pBonkle->format = pEffect->format;
    int size = hRes->size;
    char *pData = (char*)gSoundRes.Lock(hRes);
    Calc3DValues(pBonkle);
    int priority = 1;
	if (priority < lVol)  priority = lVol;
    if (priority < rVol)  priority = rVol;
    if (gStereo)
    {
       //MV_Lock();
        pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, (intptr_t)&pBonkle->lChan);
        pBonkle->rChan = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, (intptr_t)&pBonkle->rChan);
       //MV_Unlock();
    }
    else
    {
        pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, v1c, 0, lVol, lVol, rVol, priority, (intptr_t)&pBonkle->lChan);
        pBonkle->rChan = 0;
    }
}

void sfxPlay3DSound(spritetype *pSprite, int soundId, int chanId, int nFlags)
{
	if (FXDevice == -1 || soundId < 0 || !sndActive || !pSprite)
			return;

    DICTNODE *hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes)
        return;

    SFX *pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes)
        return;
    int size = hRes->size;
    if (size <= 0)
        return;
    int v14;
    v14 = mulscale16(pEffect->pitch, sndGetRate(pEffect->format));
    BONKLE *pBonkle = NULL;
    if (chanId >= 0)
    {
        int i;
        for (i = 0; i < nBonkles; i++)
        {
            pBonkle = BonkleCache[i];
            if (pBonkle->chanId == chanId && (pBonkle->pSndSpr == pSprite || (nFlags & 1) != 0))
            {
                if ((nFlags & 4) != 0 && pBonkle->chanId == chanId)
                    return;
                if ((nFlags & 2) != 0 && pBonkle->sfxId == soundId)
                    return;
                if (pBonkle->lChan > 0)
                    FX_StopSound(pBonkle->lChan);
                if (pBonkle->rChan > 0)
                    FX_StopSound(pBonkle->rChan);
                if (pBonkle->hSnd)
                {
                    gSoundRes.Unlock(pBonkle->hSnd);
                    pBonkle->hSnd = NULL;
                }
                break;
            }
        }
        if (i == nBonkles)
        {
            if (nBonkles >= 256)
                return;
            pBonkle = BonkleCache[nBonkles++];
        }
        pBonkle->pSndSpr = pSprite;
        pBonkle->chanId = chanId;
    }
    else
    {
        if (nBonkles >= 256)
            return;
        pBonkle = BonkleCache[nBonkles++];
        pBonkle->pSndSpr = NULL;
    }
    pBonkle->curPos.x = pSprite->x;
    pBonkle->curPos.y = pSprite->y;
    pBonkle->curPos.z = pSprite->z;
    pBonkle->sectnum = pSprite->sectnum;
    pBonkle->oldPos = pBonkle->curPos;
    pBonkle->sfxId = soundId;
    pBonkle->hSnd = hRes;
    pBonkle->vol = pEffect->relVol;
    pBonkle->pitch = v14;
    Calc3DValues(pBonkle);
    int priority = 1;
    if (priority < lVol)
        priority = lVol;
    if (priority < rVol)
        priority = rVol;
    int loopStart = pEffect->loopStart;
    int loopEnd = ClipLow(size - 1, 0);
    if (chanId < 0)
        loopStart = -1;
    //MV_Lock();
    char *pData = (char*)gSoundRes.Lock(hRes);
    if (loopStart >= 0)
    {
        if (gStereo)
        {
            pBonkle->lChan = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, lPitch, 0, lVol, lVol, 0, priority, (intptr_t)&pBonkle->lChan);
            pBonkle->rChan = FX_PlayLoopedRaw(pData + rPhase, size - rPhase, pData + loopStart, pData + loopEnd, rPitch, 0, rVol, 0, rVol, priority, (intptr_t)&pBonkle->rChan);
        }
        else
        {
            pBonkle->lChan = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, v14, 0, lVol, lVol, rVol, priority, (intptr_t)&pBonkle->lChan);
            pBonkle->rChan = 0;
        }
    }
    else
    {
        pData = (char*)gSoundRes.Lock(pBonkle->hSnd);
        if (gStereo)
        {
            pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, (intptr_t)&pBonkle->lChan);
            pBonkle->rChan = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, (intptr_t)&pBonkle->rChan);
        }
        else
        {
            pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, v14, 0, lVol, lVol, rVol, priority, (intptr_t)&pBonkle->lChan);
            pBonkle->rChan = 0;
        }
    }
    //MV_Unlock();
}

// by NoOne: same as previous, but allows to set custom pitch for sound AND volume.
void sfxPlay3DSoundCP(spritetype* pSprite, int soundId, int chanId, int nFlags, int pitch, int volume)
{
	if (FXDevice == -1 || soundId < 0 || !sndActive)
			return;
		
    DICTNODE* hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes) return;

    SFX* pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes) return;
    int size = hRes->size;
    if (size <= 0) return;
    
    if (pitch <= 0) pitch = pEffect->pitch;
    else pitch -= Random(pEffect->pitchRange);

    int v14;
    v14 = mulscale16(pitch, sndGetRate(pEffect->format));
    
    BONKLE * pBonkle = NULL;
    if (chanId >= 0)
    {
        int i;
        for (i = 0; i < nBonkles; i++)
        {
            pBonkle = BonkleCache[i];
            if (pBonkle->chanId == chanId && (pBonkle->pSndSpr == pSprite || (nFlags & 1) != 0))
            {
                if ((nFlags & 4) != 0 && pBonkle->chanId == chanId)
                    return;
                if ((nFlags & 2) != 0 && pBonkle->sfxId == soundId)
                    return;
                if (pBonkle->lChan > 0)
                    FX_StopSound(pBonkle->lChan);
                if (pBonkle->rChan > 0)
                    FX_StopSound(pBonkle->rChan);
                if (pBonkle->hSnd)
                {
                    gSoundRes.Unlock(pBonkle->hSnd);
                    pBonkle->hSnd = NULL;
                }
                break;
            }
        }
        if (i == nBonkles)
        {
            if (nBonkles >= 256)
                return;
            pBonkle = BonkleCache[nBonkles++];
        }
        pBonkle->pSndSpr = pSprite;
        pBonkle->chanId = chanId;
    }
    else
    {
        if (nBonkles >= 256)
            return;
        pBonkle = BonkleCache[nBonkles++];
        pBonkle->pSndSpr = NULL;
    }
    pBonkle->curPos.x = pSprite->x;
    pBonkle->curPos.y = pSprite->y;
    pBonkle->curPos.z = pSprite->z;
    pBonkle->sectnum = pSprite->sectnum;
    pBonkle->oldPos = pBonkle->curPos;
    pBonkle->sfxId = soundId;
    pBonkle->hSnd = hRes;
    pBonkle->vol = ((volume == 0) ? pEffect->relVol : ((volume == -1) ? 0 : ((volume > 255) ? 255 : volume)));
    pBonkle->pitch = v14;
    Calc3DValues(pBonkle);
    int priority = 1;
    if (priority < lVol)
        priority = lVol;
    if (priority < rVol)
        priority = rVol;
    int loopStart = pEffect->loopStart;
    int loopEnd = ClipLow(size - 1, 0);
    if (chanId < 0)
        loopStart = -1;
    //MV_Lock();
    char* pData = (char*)gSoundRes.Lock(hRes);
    if (loopStart >= 0)
    {
        if (gStereo)
        {
            pBonkle->lChan = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, lPitch, 0, lVol, lVol, 0, priority, (intptr_t)& pBonkle->lChan);
            pBonkle->rChan = FX_PlayLoopedRaw(pData + rPhase, size - rPhase, pData + loopStart, pData + loopEnd, rPitch, 0, rVol, 0, rVol, priority, (intptr_t)& pBonkle->rChan);
        }
        else
        {
            pBonkle->lChan = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, v14, 0, lVol, lVol, rVol, priority, (intptr_t)& pBonkle->lChan);
            pBonkle->rChan = 0;
        }
    }
    else
    {
        pData = (char*)gSoundRes.Lock(pBonkle->hSnd);
        if (gStereo)
        {
            pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, (intptr_t)& pBonkle->lChan);
            pBonkle->rChan = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, (intptr_t)& pBonkle->rChan);
        }
        else
        {
            pBonkle->lChan = FX_PlayRaw(pData + lPhase, size - lPhase, v14, 0, lVol, lVol, rVol, priority, (intptr_t)& pBonkle->lChan);
            pBonkle->rChan = 0;
        }
    }
    //MV_Unlock();
}

void sfxKillSoundInternal(int i)
{
    BONKLE *pBonkle = BonkleCache[i];
    if (pBonkle->lChan > 0)
    {
        FX_EndLooping(pBonkle->lChan);
        FX_StopSound(pBonkle->lChan);
    }
    if (pBonkle->rChan > 0)
    {
        FX_EndLooping(pBonkle->rChan);
        FX_StopSound(pBonkle->rChan);
    }
    if (pBonkle->hSnd)
    {
        gSoundRes.Unlock(pBonkle->hSnd);
        pBonkle->hSnd = NULL;
    }
    BonkleCache[i] = BonkleCache[--nBonkles];
    BonkleCache[nBonkles] = pBonkle;
}

void sfxKill3DSound(spritetype *pSprite, int chanId, int soundId)
{
    if (!pSprite)
        return;
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        BONKLE *pBonkle = BonkleCache[i];
        if (pBonkle->pSndSpr == pSprite && (chanId < 0 || chanId == pBonkle->chanId) && (soundId < 0 || soundId == pBonkle->sfxId))
        {
            sfxKillSoundInternal(i);
            break;
        }
    }
}

void sfxKillAllSounds(void)
{
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        sfxKillSoundInternal(i);
    }
}


void sfxUpdate3DSounds(void)
{
    int dx = mulscale30(Cos(ang + 512), 43);
    earL0 = earL;
    int dy = mulscale30(Sin(ang + 512), 43);
    earR0 = earR;
    earL.x = posx - dx;
    earL.y = posy - dy;
    earR.x = posx + dx;
    earR.y = posy + dy;
    earVL.dx = earL.x - earL0.x;
    earVL.dy = earL.y - earL0.y;
    earVR.dx = earR.x - earR0.x;
    earVR.dy = earR.y - earR0.y;
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        BONKLE *pBonkle = BonkleCache[i];
        if (pBonkle->lChan > 0 || pBonkle->rChan > 0)
        {
            if (!pBonkle->hSnd)
                continue;
            if (pBonkle->pSndSpr)
            {
                pBonkle->oldPos = pBonkle->curPos;
                pBonkle->curPos.x = pBonkle->pSndSpr->x;
                pBonkle->curPos.y = pBonkle->pSndSpr->y;
                pBonkle->curPos.z = pBonkle->pSndSpr->z;
                pBonkle->sectnum = pBonkle->pSndSpr->sectnum;
            }
            Calc3DValues(pBonkle);
            //MV_Lock();
            if (pBonkle->lChan > 0)
            {
                if (pBonkle->rChan > 0)
                {
                    FX_SetPan(pBonkle->lChan, lVol, lVol, 0);
                    FX_SetFrequency(pBonkle->lChan, lPitch);
                }
                else
                    FX_SetPan(pBonkle->lChan, lVol, lVol, rVol);
            }
            if (pBonkle->rChan > 0)
            {
                FX_SetPan(pBonkle->rChan, rVol, 0, rVol);
                FX_SetFrequency(pBonkle->rChan, rPitch);
            }
            //MV_Unlock();
        }
        else
        {
            gSoundRes.Unlock(pBonkle->hSnd);
            pBonkle->hSnd = NULL;
            BonkleCache[i] = BonkleCache[--nBonkles];
            BonkleCache[nBonkles] = pBonkle;
        }
    }
}

/// AMBIENT //////////////////////////////////////////////////////

void ambProcess(void)
{
	if (FXDevice == -1 || !sndActive) return;
    for (int nSprite = headspritestat[kStatAmbience]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->owner < 0 || pSprite->owner >= kMaxAmbChannel)
            continue;
        int nXSprite = pSprite->extra;
        if (nXSprite > 0 && nXSprite < kMaxXSprites)
        {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->state)
            {
                int dx = pSprite->x-posx;
                int dy = pSprite->y-posy;
                int dz = pSprite->z-posz;
                dx >>= 4;
                dy >>= 4;
                dz >>= 8;
                int nDist = ksqrt(dx*dx+dy*dy+dz*dz);
                int vs = mulscale16(pXSprite->data4, pXSprite->busy);
				ambChannels[pSprite->owner].at4 += ClipRange(scale(nDist, pXSprite->data1, pXSprite->data2, vs, 0), 0, vs);
            }
        }
    }
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        if (pChannel->at0 > 0)
            FX_SetPan(pChannel->at0, pChannel->at4, pChannel->at4, pChannel->at4);
        else
        {
            int end = ClipLow(pChannel->at14-1, 0);
            pChannel->at0 = FX_PlayLoopedRaw(pChannel->at10, pChannel->at14, pChannel->at10, pChannel->at10+end, sndGetRate(pChannel->at18), 0,
                pChannel->at4, pChannel->at4, pChannel->at4, pChannel->at4, (intptr_t)&pChannel->at0);
        }
        pChannel->at4 = 0;
    }
}

void ambKillAll(void)
{
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        if (pChannel->at0 > 0)
        {
            FX_EndLooping(pChannel->at0);
            FX_StopSound(pChannel->at0);
        }
        if (pChannel->atc)
        {
            gSoundRes.Unlock(pChannel->atc);
            pChannel->atc = NULL;
        }
    }
    nAmbChannels = 0;
}

void ambInit(void)
{
    ambKillAll();
    memset(ambChannels, 0, sizeof(ambChannels));
    for (int nSprite = headspritestat[kStatAmbience]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].extra <= 0 || sprite[nSprite].extra >= kMaxXSprites) continue;
        
        XSPRITE *pXSprite = &xsprite[sprite[nSprite].extra];
        if (pXSprite->data1 >= pXSprite->data2) continue;
        
        int i; AMB_CHANNEL *pChannel = ambChannels;
        for (i = 0; i < nAmbChannels; i++, pChannel++)
            if (pXSprite->data3 == pChannel->at8) break;
        
        if (i == nAmbChannels) {
            
            if (i >= kMaxAmbChannel) {
                sprite[nSprite].owner = -1;
                continue;
            }
                    
            int nSFX = pXSprite->data3;
            DICTNODE *pSFXNode = gSoundRes.Lookup(nSFX, "SFX");
            if (!pSFXNode) {
                //ThrowError("Missing sound #%d used in ambient sound generator %d\n", nSFX);
                //viewSetSystemMessage("Missing sound #%d used in ambient sound generator #%d\n", nSFX, nSprite);
                //actPostSprite(nSprite, kStatDecoration);
                continue;
            }

            SFX *pSFX = (SFX*)gSoundRes.Load(pSFXNode);
            DICTNODE *pRAWNode = gSoundRes.Lookup(pSFX->rawName, "RAW");
            if (!pRAWNode) {
                //ThrowError("Missing RAW sound \"%s\" used in ambient sound generator %d\n", pSFX->rawName, nSFX);
                //viewSetSystemMessage("Missing RAW sound \"%s\" used in ambient sound generator #%d\n", pSFX->rawName, nSprite);
                //actPostSprite(nSprite, kStatDecoration);
                continue;
            }
            
            if (pRAWNode->size > 0) {
                pChannel->at14 = pRAWNode->size;
                pChannel->at8 = nSFX;
                pChannel->atc = pRAWNode;
                pChannel->at14 = pRAWNode->size;
                pChannel->at10 = (char*)gSoundRes.Lock(pRAWNode);
                pChannel->at18 = pSFX->format;
                nAmbChannels++;
            }

        }

        sprite[nSprite].owner = i;
    }
}

