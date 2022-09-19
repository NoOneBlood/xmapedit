/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

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
/**********************************************************************
   module: FX_MAN.H

   author: James R. Dose
   date:   March 17, 1994

   Public header for FX_MAN.C

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __FX_MAN_H
#define __FX_MAN_H

#include "sndcards.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int FX_ErrorCode;

enum FX_ERRORS
   {
   FX_Warning = -2,
   FX_Error = -1,
   FX_Ok = 0,
   FX_ASSVersion,
   FX_SoundCardError,
   FX_InvalidCard,
   FX_MultiVocError,
   };

#define FX_MUSIC_PRIORITY	0x7fffffffl


const char *FX_ErrorString( int ErrorNumber );
int   FX_Init( int SoundCard, int numvoices, int * numchannels, int * samplebits, int * mixrate, void * initdata );
int   FX_Shutdown( void );
int   FX_GetCurrentDriver(void);
const char *FX_GetCurrentDriverName(void);
int   FX_SetCallBack( void ( *function )( unsigned int ) );
void  FX_SetVolume( int volume );
int   FX_GetVolume( void );

void  FX_SetReverseStereo( int setting );
int   FX_GetReverseStereo( void );
void  FX_SetReverb( int reverb );
void  FX_SetFastReverb( int reverb );
int   FX_GetMaxReverbDelay( void );
int   FX_GetReverbDelay( void );
void  FX_SetReverbDelay( int delay );

int FX_VoiceAvailable( int priority );
int FX_EndLooping( int handle );
int FX_SetPan( int handle, int vol, int left, int right );
int FX_SetPitch( int handle, int pitchoffset );
int FX_SetFrequency( int handle, int frequency );
int FX_GetFrequency( int handle, int *frequency );

int FX_PlayVOC( char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
       int priority, unsigned int callbackval );
int FX_PlayLoopedVOC( char *ptr, unsigned int ptrlength, int loopstart, int loopend,
       int pitchoffset, int vol, int left, int right, int priority,
       unsigned int callbackval );
int FX_PlayWAV( char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
       int priority, unsigned int callbackval );
int FX_PlayLoopedWAV( char *ptr, unsigned int ptrlength, int loopstart, int loopend,
       int pitchoffset, int vol, int left, int right, int priority,
       unsigned int callbackval );
int FX_PlayVOC3D( char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
       int priority, unsigned int callbackval );
int FX_PlayWAV3D( char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
       int priority, unsigned int callbackval );
int FX_PlayRaw3D( char *ptr, unsigned int ptrlength, unsigned rate,
       int pitchoffset, int angle, int distance, int priority, unsigned int callbackval );

int FX_PlayAuto( char *ptr, unsigned int ptrlength, int pitchoffset, int vol, int left, int right,
                int priority, unsigned int callbackval );
int FX_PlayLoopedAuto( char *ptr, unsigned int ptrlength, int loopstart, int loopend,
                      int pitchoffset, int vol, int left, int right, int priority,
                      unsigned int callbackval );
int FX_PlayAuto3D( char *ptr, unsigned int ptrlength, int pitchoffset, int angle, int distance,
                  int priority, unsigned int callbackval );

int FX_PlayRaw( char *ptr, unsigned int length, unsigned rate,
       int pitchoffset, int vol, int left, int right, int priority,
       unsigned int callbackval );
int FX_PlayLoopedRaw( char *ptr, unsigned int length, char *loopstart,
       char *loopend, unsigned rate, int pitchoffset, int vol, int left,
       int right, int priority, unsigned int callbackval );
int FX_Pan3D( int handle, int angle, int distance );
int FX_SoundActive( int handle );
int FX_SoundsPlaying( void );
int FX_StopSound( int handle );
int FX_PauseSound( int handle, int pauseon );
int FX_StopAllSounds( void );
int FX_StartDemandFeedPlayback( void ( *function )( char **ptr, unsigned int *length ),
       int rate, int pitchoffset, int vol, int left, int right,
       int priority, unsigned int callbackval );
int  FX_StartRecording( int MixRate, void ( *function )( char *ptr, int length ) );
void FX_StopRecord( void );

#ifdef __cplusplus
}
#endif

#endif
