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
 * Stub driver for no output
 */

#include "midifuncs.h"
#include <string.h>

int NoSoundDrv_GetError(void)
{
	return 0;
}

const char *NoSoundDrv_ErrorString( int ErrorNumber )
{
	(void)ErrorNumber;
	return "No sound, Ok.";
}

int NoSoundDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata)
{
	(void)mixrate; (void)numchannels; (void)samplebits; (void)initdata;
	return 0;
}

void NoSoundDrv_PCM_Shutdown(void)
{
}

int NoSoundDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
						int NumDivisions, void ( *CallBackFunc )( void ) )
{
	(void)BufferStart; (void)BufferSize; (void)NumDivisions; (void)CallBackFunc;
	return 0;
}

void NoSoundDrv_PCM_StopPlayback(void)
{
}

void NoSoundDrv_PCM_Lock(void)
{
}

void NoSoundDrv_PCM_Unlock(void)
{
}


int NoSoundDrv_CD_Init(void)
{
    return 0;
}

void NoSoundDrv_CD_Shutdown(void)
{
}

int NoSoundDrv_CD_Play(int track, int loop)
{
    (void)track; (void)loop;
    return 0;
}

void NoSoundDrv_CD_Stop(void)
{
}

void NoSoundDrv_CD_Pause(int pauseon)
{
    (void)pauseon;
}

int NoSoundDrv_CD_IsPlaying(void)
{
    return 0;
}

void NoSoundDrv_CD_SetVolume(int volume)
{
    (void)volume;
}

int NoSoundDrv_MIDI_Init(midifuncs *funcs, const char *params)
{
    (void)params;
    memset(funcs, 0, sizeof(midifuncs));
    return 0;
}

void NoSoundDrv_MIDI_Shutdown(void)
{
}

int  NoSoundDrv_MIDI_StartPlayback(void (*service)(void))
{
    (void)service;
    return 0;
}

void NoSoundDrv_MIDI_HaltPlayback(void)
{
}

unsigned int NoSoundDrv_MIDI_GetTick(void)
{
    return 0;
}

void NoSoundDrv_MIDI_SetTempo(int tempo, int division)
{
    (void)tempo; (void)division;
}

void NoSoundDrv_MIDI_Lock(void)
{
}

void NoSoundDrv_MIDI_Unlock(void)
{
}

