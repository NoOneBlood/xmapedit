/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * Abstraction layer for hiding the various supported sound devices
 * behind a common and opaque interface called on by MultiVoc.
 */

#include "drivers.h"

//#define NO_COREAUDIO
//#define NO_DIRECTSOUND

#include "driver_nosound.h"

#ifdef HAVE_SDL
# include "driver_sdl.h"
#endif

#if defined __APPLE__ && !defined NO_COREAUDIO
# include "driver_coreaudio.h"
#endif

#ifdef _WIN32
# include "driver_directsound.h"
# include "driver_winmm.h"
#endif

#ifdef HAVE_FLUIDSYNTH
# include "driver_fluidsynth.h"
#endif

#ifdef HAVE_ALSA
# include "driver_alsa.h"
#endif

int ASS_PCMSoundDriver = -1;
int ASS_CDSoundDriver = -1;
int ASS_MIDISoundDriver = -1;

#define UNSUPPORTED_PCM         0,0,0,0,0,0
#define UNSUPPORTED_CD          0,0,0,0,0,0,0
#define UNSUPPORTED_MIDI        0,0,0,0,0,0,0
#define UNSUPPORTED_COMPLETELY  0,0, UNSUPPORTED_PCM, UNSUPPORTED_CD, UNSUPPORTED_MIDI

static struct {
    const char * DriverName;
    
    int          (* GetError)(void);
    const char * (* ErrorString)(int);

    int          (* PCM_Init)(int *, int *, int *, void *);
    void         (* PCM_Shutdown)(void);
    int          (* PCM_BeginPlayback)(char *, int, int, void ( * )(void) );
    void         (* PCM_StopPlayback)(void);
    void         (* PCM_Lock)(void);
    void         (* PCM_Unlock)(void);

    int          (* CD_Init)(void);
    void         (* CD_Shutdown)(void);
    int          (* CD_Play)(int track, int loop);
    void         (* CD_Stop)(void);
    void         (* CD_Pause)(int pauseon);
    int          (* CD_IsPlaying)(void);
    void         (* CD_SetVolume)(int volume);

    int          (* MIDI_Init)(midifuncs *, const char *);
    void         (* MIDI_Shutdown)(void);
    int          (* MIDI_StartPlayback)(void (*service)(void));
    void         (* MIDI_HaltPlayback)(void);
    void         (* MIDI_SetTempo)(int tempo, int division);
    void         (* MIDI_Lock)(void);
    void         (* MIDI_Unlock)(void);
} SoundDrivers[ASS_NumSoundCards] = {
    
    // Everyone gets the "no sound" driver
    {
        "No Sound",
        NoSoundDrv_GetError,
        NoSoundDrv_ErrorString,
        NoSoundDrv_PCM_Init,
        NoSoundDrv_PCM_Shutdown,
        NoSoundDrv_PCM_BeginPlayback,
        NoSoundDrv_PCM_StopPlayback,
        NoSoundDrv_PCM_Lock,
        NoSoundDrv_PCM_Unlock,
        NoSoundDrv_CD_Init,
        NoSoundDrv_CD_Shutdown,
        NoSoundDrv_CD_Play,
        NoSoundDrv_CD_Stop,
        NoSoundDrv_CD_Pause,
        NoSoundDrv_CD_IsPlaying,
        NoSoundDrv_CD_SetVolume,
        NoSoundDrv_MIDI_Init,
        NoSoundDrv_MIDI_Shutdown,
        NoSoundDrv_MIDI_StartPlayback,
        NoSoundDrv_MIDI_HaltPlayback,
        NoSoundDrv_MIDI_SetTempo,
        NoSoundDrv_MIDI_Lock,
        NoSoundDrv_MIDI_Unlock,
    },

    // OS X CoreAudio
    {
        "Core Audio",
    #if defined __APPLE__ && !defined NO_COREAUDIO
        CoreAudioDrv_GetError,
        CoreAudioDrv_ErrorString,
        CoreAudioDrv_PCM_Init,
        CoreAudioDrv_PCM_Shutdown,
        CoreAudioDrv_PCM_BeginPlayback,
        CoreAudioDrv_PCM_StopPlayback,
        CoreAudioDrv_PCM_Lock,
        CoreAudioDrv_PCM_Unlock,
        UNSUPPORTED_CD,
        CoreAudioDrv_MIDI_Init,
        CoreAudioDrv_MIDI_Shutdown,
        CoreAudioDrv_MIDI_StartPlayback,
        CoreAudioDrv_MIDI_HaltPlayback,
        CoreAudioDrv_MIDI_SetTempo,
        CoreAudioDrv_MIDI_Lock,
        CoreAudioDrv_MIDI_Unlock,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },

    // Windows DirectSound
    {
        "DirectSound",
    #if defined _WIN32 && !defined NO_DIRECTSOUND
        DirectSoundDrv_GetError,
        DirectSoundDrv_ErrorString,
        DirectSoundDrv_PCM_Init,
        DirectSoundDrv_PCM_Shutdown,
        DirectSoundDrv_PCM_BeginPlayback,
        DirectSoundDrv_PCM_StopPlayback,
        DirectSoundDrv_PCM_Lock,
        DirectSoundDrv_PCM_Unlock,
        UNSUPPORTED_CD,
        UNSUPPORTED_MIDI,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },

    // Windows MultiMedia system
    {
        "WinMM",
    #ifdef _WIN32
        WinMMDrv_GetError,
        WinMMDrv_ErrorString,

        UNSUPPORTED_PCM,

        WinMMDrv_CD_Init,
        WinMMDrv_CD_Shutdown,
        WinMMDrv_CD_Play,
        WinMMDrv_CD_Stop,
        WinMMDrv_CD_Pause,
        WinMMDrv_CD_IsPlaying,
        WinMMDrv_CD_SetVolume,
        WinMMDrv_MIDI_Init,
        WinMMDrv_MIDI_Shutdown,
        WinMMDrv_MIDI_StartPlayback,
        WinMMDrv_MIDI_HaltPlayback,
        WinMMDrv_MIDI_SetTempo,
        WinMMDrv_MIDI_Lock,
        WinMMDrv_MIDI_Unlock,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },

    // FluidSynth MIDI synthesiser
    {
        "FluidSynth",
    #ifdef HAVE_FLUIDSYNTH
        FluidSynthDrv_GetError,
        FluidSynthDrv_ErrorString,

        UNSUPPORTED_PCM,
        UNSUPPORTED_CD,

        FluidSynthDrv_MIDI_Init,
        FluidSynthDrv_MIDI_Shutdown,
        FluidSynthDrv_MIDI_StartPlayback,
        FluidSynthDrv_MIDI_HaltPlayback,
        FluidSynthDrv_MIDI_SetTempo,
        FluidSynthDrv_MIDI_Lock,
        FluidSynthDrv_MIDI_Unlock,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },

    // ALSA MIDI synthesiser
    {
        "ALSA",
    #ifdef HAVE_ALSA
        ALSADrv_GetError,
        ALSADrv_ErrorString,

        UNSUPPORTED_PCM,
        UNSUPPORTED_CD,

        ALSADrv_MIDI_Init,
        ALSADrv_MIDI_Shutdown,
        ALSADrv_MIDI_StartPlayback,
        ALSADrv_MIDI_HaltPlayback,
        ALSADrv_MIDI_SetTempo,
        ALSADrv_MIDI_Lock,
        ALSADrv_MIDI_Unlock,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },

    // Simple DirectMedia Layer
    {
        "SDL",
    #ifdef HAVE_SDL
        SDLDrv_GetError,
        SDLDrv_ErrorString,
        SDLDrv_PCM_Init,
        SDLDrv_PCM_Shutdown,
        SDLDrv_PCM_BeginPlayback,
        SDLDrv_PCM_StopPlayback,
        SDLDrv_PCM_Lock,
        SDLDrv_PCM_Unlock,
        SDLDrv_CD_Init,
        SDLDrv_CD_Shutdown,
        SDLDrv_CD_Play,
        SDLDrv_CD_Stop,
        SDLDrv_CD_Pause,
        SDLDrv_CD_IsPlaying,
        SDLDrv_CD_SetVolume,
        UNSUPPORTED_MIDI,
    #else
        UNSUPPORTED_COMPLETELY,
    #endif
    },
};


int SoundDriver_IsPCMSupported(int driver)
{
	return (SoundDrivers[driver].PCM_Init != 0);
}

int SoundDriver_IsCDSupported(int driver)
{
	return (SoundDrivers[driver].CD_Init != 0);
}

int SoundDriver_IsMIDISupported(int driver)
{
	return (SoundDrivers[driver].MIDI_Init != 0);
}

const char * SoundDriver_GetName(int driver)
{
    return SoundDrivers[driver].DriverName;
}

int SoundDriver_PCM_GetError(void)
{
	if (!SoundDriver_IsPCMSupported(ASS_PCMSoundDriver)) {
		return -1;
	}
	return SoundDrivers[ASS_PCMSoundDriver].GetError();
}

const char * SoundDriver_PCM_ErrorString( int ErrorNumber )
{
	if (ASS_PCMSoundDriver < 0 || ASS_PCMSoundDriver >= ASS_NumSoundCards) {
		return "No sound driver selected.";
	}
	if (!SoundDriver_IsPCMSupported(ASS_PCMSoundDriver)) {
		return "Unsupported sound driver selected.";
	}
	return SoundDrivers[ASS_PCMSoundDriver].ErrorString(ErrorNumber);
}

int SoundDriver_CD_GetError(void)
{
	if (!SoundDriver_IsCDSupported(ASS_CDSoundDriver)) {
		return -1;
	}
	return SoundDrivers[ASS_CDSoundDriver].GetError();
}

const char * SoundDriver_CD_ErrorString( int ErrorNumber )
{
	if (ASS_CDSoundDriver < 0 || ASS_CDSoundDriver >= ASS_NumSoundCards) {
		return "No sound driver selected.";
	}
	if (!SoundDriver_IsCDSupported(ASS_CDSoundDriver)) {
		return "Unsupported sound driver selected.";
	}
	return SoundDrivers[ASS_CDSoundDriver].ErrorString(ErrorNumber);
}

int SoundDriver_MIDI_GetError(void)
{
	if (!SoundDriver_IsMIDISupported(ASS_MIDISoundDriver)) {
		return -1;
	}
	return SoundDrivers[ASS_MIDISoundDriver].GetError();
}

const char * SoundDriver_MIDI_ErrorString( int ErrorNumber )
{
	if (ASS_MIDISoundDriver < 0 || ASS_MIDISoundDriver >= ASS_NumSoundCards) {
		return "No sound driver selected.";
	}
	if (!SoundDriver_IsMIDISupported(ASS_MIDISoundDriver)) {
		return "Unsupported sound driver selected.";
	}
	return SoundDrivers[ASS_MIDISoundDriver].ErrorString(ErrorNumber);
}

int SoundDriver_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata)
{
	return SoundDrivers[ASS_PCMSoundDriver].PCM_Init(mixrate, numchannels, samplebits, initdata);
}

void SoundDriver_PCM_Shutdown(void)
{
	SoundDrivers[ASS_PCMSoundDriver].PCM_Shutdown();
}

int SoundDriver_PCM_BeginPlayback(char *BufferStart, int BufferSize,
		int NumDivisions, void ( *CallBackFunc )( void ) )
{
	return SoundDrivers[ASS_PCMSoundDriver].PCM_BeginPlayback(BufferStart,
			BufferSize, NumDivisions, CallBackFunc);
}

void SoundDriver_PCM_StopPlayback(void)
{
	SoundDrivers[ASS_PCMSoundDriver].PCM_StopPlayback();
}

void SoundDriver_PCM_Lock(void)
{
	SoundDrivers[ASS_PCMSoundDriver].PCM_Lock();
}

void SoundDriver_PCM_Unlock(void)
{
	SoundDrivers[ASS_PCMSoundDriver].PCM_Unlock();
}

int  SoundDriver_CD_Init(void)
{
    return SoundDrivers[ASS_CDSoundDriver].CD_Init();
}

void SoundDriver_CD_Shutdown(void)
{
    SoundDrivers[ASS_CDSoundDriver].CD_Shutdown();
}

int  SoundDriver_CD_Play(int track, int loop)
{
    return SoundDrivers[ASS_CDSoundDriver].CD_Play(track, loop);
}

void SoundDriver_CD_Stop(void)
{
    SoundDrivers[ASS_CDSoundDriver].CD_Stop();
}

void SoundDriver_CD_Pause(int pauseon)
{
    SoundDrivers[ASS_CDSoundDriver].CD_Pause(pauseon);
}

int SoundDriver_CD_IsPlaying(void)
{
    return SoundDrivers[ASS_CDSoundDriver].CD_IsPlaying();
}

void SoundDriver_CD_SetVolume(int volume)
{
    SoundDrivers[ASS_CDSoundDriver].CD_SetVolume(volume);
}

int  SoundDriver_MIDI_Init(midifuncs *funcs, const char *params)
{
    return SoundDrivers[ASS_MIDISoundDriver].MIDI_Init(funcs, params);
}

void SoundDriver_MIDI_Shutdown(void)
{
    SoundDrivers[ASS_MIDISoundDriver].MIDI_Shutdown();
}

int  SoundDriver_MIDI_StartPlayback(void (*service)(void))
{
    return SoundDrivers[ASS_MIDISoundDriver].MIDI_StartPlayback(service);
}

void SoundDriver_MIDI_HaltPlayback(void)
{
    SoundDrivers[ASS_MIDISoundDriver].MIDI_HaltPlayback();
}

void SoundDriver_MIDI_SetTempo(int tempo, int division)
{
    SoundDrivers[ASS_MIDISoundDriver].MIDI_SetTempo(tempo, division);
}

void SoundDriver_MIDI_Lock(void)
{
	SoundDrivers[ASS_MIDISoundDriver].MIDI_Lock();
}

void SoundDriver_MIDI_Unlock(void)
{
	SoundDrivers[ASS_MIDISoundDriver].MIDI_Unlock();
}

// vim:ts=4:sw=4:expandtab:
