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
 * libSDL output driver for MultiVoc
 */

#ifdef __APPLE__
# if HAVE_SDL == 2
#  include <SDL2/SDL.h>
# else
#  include <SDL/SDL.h>
# endif
#else
# include <SDL.h>
#endif
#include "asssys.h"
#include "driver_sdl.h"

enum {
    SDLErr_Warning = -2,
    SDLErr_Error   = -1,
    SDLErr_Ok      = 0,
    SDLErr_Uninitialised,
    SDLErr_InitSubSystem,
    SDLErr_OpenAudio,
    SDLErr_CDOpen,
    SDLErr_CDCannotPlayTrack,
    SDLErr_CDCreateSemaphore,
    SDLErr_CDCreateThread,
    SDLErr_CDNotSupported
};

static int ErrorCode = SDLErr_Ok;
static int Initialised = 0;
static int Playing = 0;
static int StartedSDL = 0;      // SDL services in use (1 = sound, 2 = CDA)
static unsigned StartedSDLInit = 0;  // SDL services we initialised (0x80000000 means we used SDL_Init)

#if (SDL_MAJOR_VERSION == 1)
static SDL_CD *CDRom = 0;
static SDL_Thread *CDWatchThread = 0;
static SDL_sem *CDWatchKillSem = 0;
static int CDLoop = 0;
static int CDTrack = 0;
#endif

static char *MixBuffer = 0;
static int MixBufferSize = 0;
static int MixBufferCount = 0;
static int MixBufferCurrent = 0;
static int MixBufferUsed = 0;
static void ( *MixCallBack )( void ) = 0;

static void fillData(void * userdata, Uint8 * ptr, int remaining)
{
    int len;
    char *sptr;

    (void)userdata;

    while (remaining > 0) {
        if (MixBufferUsed == MixBufferSize) {
            MixCallBack();
            
            MixBufferUsed = 0;
            MixBufferCurrent++;
            if (MixBufferCurrent >= MixBufferCount) {
                MixBufferCurrent -= MixBufferCount;
            }
        }
        
        while (remaining > 0 && MixBufferUsed < MixBufferSize) {
            sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;
            
            len = MixBufferSize - MixBufferUsed;
            if (remaining < len) {
                len = remaining;
            }
            
            memcpy(ptr, sptr, len);
            
            ptr += len;
            MixBufferUsed += len;
            remaining -= len;
        }
    }
}


int SDLDrv_GetError(void)
{
    return ErrorCode;
}

const char *SDLDrv_ErrorString( int ErrorNumber )
{
    const char *ErrorString;
	
    switch( ErrorNumber ) {
        case SDLErr_Warning :
        case SDLErr_Error :
            ErrorString = SDLDrv_ErrorString( ErrorCode );
            break;

        case SDLErr_Ok :
            ErrorString = "SDL Audio ok.";
            break;
			
        case SDLErr_Uninitialised:
            ErrorString = "SDL Audio uninitialised.";
            break;

        case SDLErr_InitSubSystem:
            ErrorString = "SDL Audio: error in Init or InitSubSystem.";
            break;

        case SDLErr_OpenAudio:
            ErrorString = "SDL Audio: error in OpenAudio.";
            break;
            
        case SDLErr_CDOpen:
            ErrorString = "SDL CD: error opening cd device.";
            break;
        
        case SDLErr_CDCannotPlayTrack:
            ErrorString = "SDL CD: cannot play the requested track.";
            break;
            
        case SDLErr_CDCreateSemaphore:
            ErrorString = "SDL CD: could not create looped CD playback semaphore.";
            break;
        
        case SDLErr_CDCreateThread:
            ErrorString = "SDL CD: could not create looped CD playback thread.";
            break;

        case SDLErr_CDNotSupported:
            ErrorString = "SDL CD is not supported";
            break;

        default:
            ErrorString = "Unknown SDL Audio error code.";
            break;
    }

    return ErrorString;
}

int SDLDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata)
{
    Uint32 inited;
    int err = 0;
    SDL_AudioSpec spec, actual;

    (void)initdata;

    if (Initialised) {
        SDLDrv_PCM_Shutdown();
    }

    inited = SDL_WasInit(SDL_INIT_EVERYTHING);

    if (inited == 0) {
        // nothing was initialised
        err = SDL_Init(SDL_INIT_AUDIO);
        StartedSDLInit |= 0x80000000 + SDL_INIT_AUDIO;
    } else if (!(inited & SDL_INIT_AUDIO)) {
        err = SDL_InitSubSystem(SDL_INIT_AUDIO);
        StartedSDLInit |= SDL_INIT_AUDIO;
    }

    if (err < 0) {
        ErrorCode = SDLErr_InitSubSystem;
        return SDLErr_Error;
    }

    StartedSDL |= SDL_INIT_AUDIO;

    #if (SDL_MAJOR_VERSION == 1)
    {
        char drivername[256] = "(error)";
        SDL_AudioDriverName(drivername, sizeof(drivername));
        ASS_Message("SDLDrv: audio driver: %s\n", drivername);
    }
    #else
    {
        const char * drivername = SDL_GetCurrentAudioDriver();
        ASS_Message("SDLDrv: audio driver: %s\n", drivername ? drivername : "(error)");
    }
    #endif

    memset(&spec, 0, sizeof(spec));
    spec.freq = *mixrate;
    spec.format = (*samplebits == 8) ? AUDIO_U8 : AUDIO_S16SYS;
    spec.channels = *numchannels;
    spec.callback = fillData;
    spec.userdata = 0;

    // Mix buffer to be a power of 2, min 512 samples, and 4096 samples at 48kHz.
    spec.samples = 512;
    while (spec.samples < (4096 * *mixrate / 48000))
        spec.samples += spec.samples;

    memset(&actual, 0, sizeof(actual));

    err = SDL_OpenAudio(&spec, &actual);
    if (err < 0) {
        ErrorCode = SDLErr_OpenAudio;
        return SDLErr_Error;
    }

    if (actual.freq == 0 || actual.channels == 0) {
        // hack for when SDL said it opened the audio, but clearly didn't
        SDL_CloseAudio();
        ErrorCode = SDLErr_OpenAudio;
        return SDLErr_Error;
    }

    err = 0;

    *mixrate = actual.freq;
    if (actual.format == AUDIO_U8 || actual.format == AUDIO_S16SYS) {
        *samplebits = actual.format & 0xff;
    } else {
        const char *format;
        switch (actual.format) {
            case AUDIO_U8: format = "AUDIO_U8"; break;
            case AUDIO_S8: format = "AUDIO_S8"; break;
            case AUDIO_U16LSB: format = "AUDIO_U16LSB"; break;
            case AUDIO_S16LSB: format = "AUDIO_S16LSB"; break;
            case AUDIO_U16MSB: format = "AUDIO_U16MSB"; break;
            case AUDIO_S16MSB: format = "AUDIO_S16MSB"; break;
            default: format = "?!"; break;
        }
        ASS_Message("SDLDrv: audio format: %s\n", format);
        ErrorCode = SDLErr_OpenAudio;
        err = 1;
    }
    if (actual.channels == 1 || actual.channels == 2) {
        *numchannels = actual.channels;
    } else {
        ASS_Message("SDLDrv: audio channels: %d\n", actual.channels);
        ErrorCode = SDLErr_OpenAudio;
        err = 1;
    }
    // ASS_Message("SDLDrv: actual samples = %d vs spec samples = %d\n", actual.samples, spec.samples);

    if (err) {
        SDL_CloseAudio();
        return SDLErr_Error;
    } else {
        Initialised = 1;
        return SDLErr_Ok;
    }
}

void SDLDrv_PCM_Shutdown(void)
{
    if (!Initialised) {
        return;
    }

    if (StartedSDLInit & SDL_INIT_AUDIO) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        StartedSDLInit &= ~SDL_INIT_AUDIO;
    }

    if (StartedSDLInit == 0x80000000) {
        SDL_Quit();
        StartedSDLInit = 0;
    }

    StartedSDL &= ~SDL_INIT_AUDIO;
}

int SDLDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
						int NumDivisions, void ( *CallBackFunc )( void ) )
{
    if (!Initialised) {
        ErrorCode = SDLErr_Uninitialised;
        return SDLErr_Error;
    }
    
    if (Playing) {
        SDLDrv_PCM_StopPlayback();
    }

    MixBuffer = BufferStart;
    MixBufferSize = BufferSize;
    MixBufferCount = NumDivisions;
    MixBufferCurrent = 0;
    MixBufferUsed = 0;
    MixCallBack = CallBackFunc;
    
    // prime the buffer
    MixCallBack();

    SDL_PauseAudio(0);
    
    Playing = 1;
    
    return SDLErr_Ok;
}

void SDLDrv_PCM_StopPlayback(void)
{
    if (!Initialised || !Playing) {
        return;
    }

    SDL_PauseAudio(1);
	
    Playing = 0;
}

void SDLDrv_PCM_Lock(void)
{
    SDL_LockAudio();
}

void SDLDrv_PCM_Unlock(void)
{
    SDL_UnlockAudio();
}


#if (SDL_MAJOR_VERSION == 1)
static int cdWatchThread(void * v)
{
    CDstatus status;
    
    do {
        switch (SDL_SemWaitTimeout(CDWatchKillSem, 500)) {
            case SDL_MUTEX_TIMEDOUT:
                // poll the status, restart if stopped
                if (!CDLoop) {
                    return 0;
                }
                
                status = SDL_CDStatus(CDRom);
                
                if (status == CD_TRAYEMPTY) {
                    return 0;
                } else if (status == CD_STOPPED) {
                    // play
                    if (SDL_CDPlayTracks(CDRom, CDTrack, 0, 1, 0) < 0) {
                        return -1;
                    }
                }
                
                break;
            case -1:
                return -1;
            case 0:
                // told to quit, so do it
                return 0;
        }
    } while (1);
    
    return 0;
}
#endif


int SDLDrv_CD_Init(void)
{
#if (SDL_MAJOR_VERSION == 1)
    Uint32 inited;
    Uint32 err = 0;
    int i;
    
    SDLDrv_CD_Shutdown();
    
    inited = SDL_WasInit(SDL_INIT_EVERYTHING);
    
    if (inited == 0) {
        // nothing was initialised
        err = SDL_Init(SDL_INIT_CDROM);
        StartedSDLInit |= 0x80000000 + SDL_INIT_CDROM;
    } else if (!(inited & SDL_INIT_CDROM)) {
        err = SDL_InitSubSystem(SDL_INIT_CDROM);
        StartedSDLInit |= SDL_INIT_CDROM;
    }
    
    if (err < 0) {
        ErrorCode = SDLErr_InitSubSystem;
        return SDLErr_Error;
    }
    
    StartedSDL |= SDL_INIT_CDROM;
    
    ASS_Message("SDLDrv: num CD drives: %d\n", SDL_CDNumDrives());
    
    CDRom = SDL_CDOpen(0);
    if (!CDRom) {
        ErrorCode = SDLErr_CDOpen;
        return SDLErr_Error;
    }
    
    ASS_Message("SDLDrv: num CD tracks: %d\n", CDRom->numtracks);
    for (i = 0; i < CDRom->numtracks; i++) {
        ASS_Message("SDLDrv: CD track %d - %s, %dsec\n",
                CDRom->track[i].id,
                CDRom->track[i].type == SDL_AUDIO_TRACK ? "audio" : "data",
                CDRom->track[i].length / CD_FPS
                );
    }
    
    return SDLErr_Ok;
#else
    ErrorCode = SDLErr_CDNotSupported;
    return SDLErr_Error;
#endif
}

void SDLDrv_CD_Shutdown(void)
{
#if (SDL_MAJOR_VERSION == 1)
    SDLDrv_CD_Stop();
    
    if (CDRom) {
        SDL_CDClose(CDRom);
        CDRom = 0;
    }
    
    if (StartedSDLInit & SDL_INIT_CDROM) {
        SDL_QuitSubSystem(SDL_INIT_CDROM);
        StartedSDLInit &= ~SDL_INIT_CDROM;
    }

    if (StartedSDLInit == 0x80000000) {
        SDL_Quit();
        StartedSDLInit = 0;
    }

    StartedSDL &= ~SDL_INIT_CDROM;
#endif
}

int SDLDrv_CD_Play(int track, int loop)
{
#if (SDL_MAJOR_VERSION == 1)
    if (!CDRom) {
        ErrorCode = SDLErr_Uninitialised;
        return SDLErr_Error;
    }
    
    SDLDrv_CD_Stop();
    
    if (SDL_CDPlayTracks(CDRom, track, 0, 1, 0) < 0) {
        ErrorCode = SDLErr_CDCannotPlayTrack;
        return SDLErr_Error;
    }
    
    CDLoop = loop;
    CDTrack = track;
    
    if (loop) {
        CDWatchKillSem = SDL_CreateSemaphore(0);
        if (!CDWatchKillSem) {
            CDLoop = 0;
            ErrorCode = SDLErr_CDCreateSemaphore;
            return SDLErr_Warning;   // play, but we won't be looping
        }
        CDWatchThread = SDL_CreateThread(cdWatchThread, (void *)NULL);
        if (!CDWatchThread) {
            SDL_DestroySemaphore(CDWatchKillSem);
            CDWatchKillSem = 0;
            
            CDLoop = 0;
            ErrorCode = SDLErr_CDCreateThread;
            return SDLErr_Warning;  // play, but we won't be looping
        }
    }
    
    return SDLErr_Ok;
#else
    (void)track; (void)loop;

    ErrorCode = SDLErr_CDNotSupported;
    return SDLErr_Error;
#endif
}

void SDLDrv_CD_Stop(void)
{
#if (SDL_MAJOR_VERSION == 1)
    if (!CDRom) {
        return;
    }
    
    if (CDWatchKillSem) {
        if (CDWatchThread) {
            SDL_SemPost(CDWatchKillSem);
            SDL_WaitThread(CDWatchThread, 0);
        }
        SDL_DestroySemaphore(CDWatchKillSem);
    }
    CDWatchKillSem = 0;
    CDWatchThread = 0;
            
    SDL_CDStop(CDRom);
#endif
}

void SDLDrv_CD_Pause(int pauseon)
{
#if (SDL_MAJOR_VERSION == 1)
    if (!CDRom) {
        return;
    }
    
    if (pauseon) {
        SDL_CDPause(CDRom);
    } else {
        SDL_CDResume(CDRom);
    }
#else
    (void)pauseon;
#endif
}

int SDLDrv_CD_IsPlaying(void)
{
#if (SDL_MAJOR_VERSION == 1)
    if (!CDRom) {
        return 0;
    }
    return SDL_CDStatus(CDRom) == CD_PLAYING;
#else
    return 0;
#endif
}

void SDLDrv_CD_SetVolume(int volume)
{
    (void)volume;
}
