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
   file:   MULTIVOC.H

   author: James R. Dose
   date:   December 20, 1993

   Public header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __MULTIVOC_H
#define __MULTIVOC_H

#define MV_MinVoiceHandle  1

#if _MSC_VER <= 1310
	#ifndef _INTPTR_T_DEFINED
		#ifdef _WIN64
			typedef __int64             intptr_t;
		#else
			typedef _W64 int            intptr_t;
		#endif
		#define _INTPTR_T_DEFINED
	#endif

	#ifndef _UINTPTR_T_DEFINED
		#ifdef _WIN64
			typedef unsigned __int64    uintptr_t;
		#else
			typedef _W64 unsigned int   uintptr_t;
		#endif
		#define _UINTPTR_T_DEFINED
	#endif
#endif

extern int MV_ErrorCode;

enum MV_Errors
   {
   MV_Warning = -2,
   MV_Error   = -1,
   MV_Ok      = 0,
   MV_UnsupportedCard,
   MV_NotInstalled,
	MV_DriverError,
   MV_NoVoices,
   MV_NoMem,
   MV_VoiceNotFound,
   MV_InvalidVOCFile,
   MV_InvalidWAVFile,
	MV_InvalidVorbisFile,
   MV_InvalidMixMode,
   MV_NullRecordFunction
   };

const char *MV_ErrorString( int ErrorNumber );
int   MV_VoicePlaying( int handle );
int   MV_VoicePaused( int handle );
int   MV_KillAllVoices( void );
int   MV_Kill( int handle );
int   MV_PauseVoice( int handle, int pauseon );
int   MV_VoicesPlaying( void );
int   MV_VoiceAvailable( int priority );
int   MV_SetPitch( int handle, int pitchoffset );
int   MV_SetFrequency( int handle, int frequency );
int   MV_GetFrequency( int handle, int *frequency );
int   MV_EndLooping( int handle );
int   MV_SetPan( int handle, int vol, int left, int right );
int   MV_Pan3D( int handle, int angle, int distance );
void  MV_SetReverb( int reverb );
void  MV_SetFastReverb( int reverb );
int   MV_GetMaxReverbDelay( void );
int   MV_GetReverbDelay( void );
void  MV_SetReverbDelay( int delay );
int   MV_SetMixMode( int numchannels, int samplebits );
int   MV_StartPlayback( void );
void  MV_StopPlayback( void );
int   MV_StartRecording( int MixRate, void ( *function )( char *ptr, int length ) );
void  MV_StopRecord( void );
int   MV_StartDemandFeedPlayback( void ( *function )( char **ptr, unsigned int *length ),
         int rate, int pitchoffset, int vol, int left, int right,
         int priority, uintptr_t callbackval );
int   MV_PlayRaw( char *ptr, unsigned int length,
         unsigned rate, int pitchoffset, int vol, int left,
         int right, int priority, uintptr_t callbackval );
int   MV_PlayLoopedRaw( char *ptr, unsigned int length,
         char *loopstart, char *loopend, unsigned rate, int pitchoffset,
         int vol, int left, int right, int priority,
         uintptr_t callbackval );
int   MV_PlayWAV( char *ptr, unsigned int length, int pitchoffset, int vol, int left,
         int right, int priority, uintptr_t callbackval );
int   MV_PlayWAV3D( char *ptr, unsigned int length, int pitchoffset, int angle, int distance,
         int priority, uintptr_t callbackval );
int   MV_PlayRaw3D( char *ptr, unsigned int length, unsigned rate, int pitchoffset, int angle, int distance,
         int priority, uintptr_t callbackval );
int   MV_PlayLoopedWAV( char *ptr, unsigned int length, int loopstart, int loopend,
         int pitchoffset, int vol, int left, int right, int priority,
         uintptr_t callbackval );
int   MV_PlayVOC3D( char *ptr, unsigned int length, int pitchoffset, int angle, int distance,
         int priority, uintptr_t callbackval );
int   MV_PlayVOC( char *ptr, unsigned int length, int pitchoffset, int vol, int left, int right,
         int priority, uintptr_t callbackval );
int   MV_PlayLoopedVOC( char *ptr, unsigned int length, int loopstart, int loopend,
         int pitchoffset, int vol, int left, int right, int priority,
         uintptr_t callbackval );
int   MV_PlayVorbis3D( char *ptr, unsigned int length, int pitchoffset, int angle, int distance,
                    int priority, uintptr_t callbackval );
int   MV_PlayVorbis( char *ptr, unsigned int length, int pitchoffset, int vol, int left, int right,
                  int priority, uintptr_t callbackval );
int   MV_PlayLoopedVorbis( char *ptr, unsigned int length, int loopstart, int loopend,
                        int pitchoffset, int vol, int left, int right, int priority,
                        uintptr_t callbackval );
void  MV_CreateVolumeTable( int index, int volume, int MaxVolume );
void  MV_SetVolume( int volume );
int   MV_GetVolume( void );
void  MV_SetCallBack( void ( *function )( uintptr_t ) );
void  MV_SetReverseStereo( int setting );
int   MV_GetReverseStereo( void );
int   MV_Init( int soundcard, int * MixRate, int Voices, int * numchannels,
         int * samplebits, void * initdata );
int   MV_Shutdown( void );

#endif
