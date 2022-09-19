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
   module: FX_MAN.C

   author: James R. Dose
   date:   March 17, 1994

   Device independant sound effect routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "sndcards.h"
#include "drivers.h"
#include "multivoc.h"
#include "fx_man.h"
#include "asssys.h"
#include "assmisc.h"

int FX_ErrorCode = FX_Ok;
int FX_Installed = FALSE;

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );


/*---------------------------------------------------------------------
   Function: FX_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *FX_ErrorString
   (
   int ErrorNumber
   )

   {
   const char *ErrorString;

   switch( ErrorNumber )
      {
      case FX_Warning :
      case FX_Error :
         ErrorString = FX_ErrorString( FX_ErrorCode );
         break;

      case FX_Ok :
         ErrorString = "Fx ok.";
         break;

      case FX_ASSVersion :
         ErrorString = "Apogee Sound System Version " ASS_VERSION_STRING "  "
            "Programmed by Jim Dose\n"
            "(c) Copyright 1995 James R. Dose.  All Rights Reserved.\n";
         break;
		
      case FX_SoundCardError :
			ErrorString = SoundDriver_PCM_ErrorString(SoundDriver_PCM_GetError());
			break;

      case FX_InvalidCard :
         ErrorString = "Invalid Sound Fx device.";
         break;

      case FX_MultiVocError :
         ErrorString = MV_ErrorString( MV_Error );
         break;

      default :
         ErrorString = "Unknown Fx error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: FX_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int FX_Init
   (
   int SoundCard,
   int numvoices,
   int * numchannels,
   int * samplebits,
   int * mixrate,
   void * initdata
   )

   {
   int devicestatus;
   int mincard, maxcard, card;

   if ( FX_Installed )
      {
      FX_Shutdown();
      }

   if (SoundCard == ASS_AutoDetect)
      {
      mincard = ASS_NoSound + 1;
      maxcard = ASS_NumSoundCards - 1;
      }
   else if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards)
      {
      FX_SetErrorCode( FX_InvalidCard );
      return FX_Error;
      }
   else
      {
      mincard = SoundCard;
      maxcard = SoundCard;
      }

   for (card = mincard; card <= maxcard; card++)
      {
      if (!SoundDriver_IsPCMSupported(card))
         {
         continue;
         }
      else if (SoundCard == ASS_AutoDetect)
         {
         ASS_Message("FX_Init: trying %s\n", SoundDriver_GetName(card));
         }

      devicestatus = MV_Init( card, mixrate, numvoices, numchannels, samplebits, initdata );
      if (devicestatus == MV_Ok)
         {
         FX_Installed = TRUE;
         return FX_Ok;
         }
      }

   if (SoundCard == ASS_AutoDetect)
      {
      // A failure to autodetect falls back to no sound.
      card = ASS_NoSound;

      devicestatus = MV_Init( card, mixrate, numvoices, numchannels, samplebits, initdata );
      if ( devicestatus == MV_Ok )
         {
         FX_Installed = TRUE;
         return FX_Ok;
         }
      }

   FX_SetErrorCode( FX_MultiVocError );
   return FX_Error;
   }


/*---------------------------------------------------------------------
   Function: FX_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int FX_Shutdown
   (
   void
   )

   {
   int status;

   if ( !FX_Installed )
      {
      return( FX_Ok );
      }

	status = MV_Shutdown();
	if ( status != MV_Ok )
		{
		FX_SetErrorCode( FX_MultiVocError );
		status = FX_Error;
		}

   FX_Installed = FALSE;

   return( status );
   }


int FX_GetCurrentDriver(void)
{
    return ASS_PCMSoundDriver;
}

const char *FX_GetCurrentDriverName(void)
{
    if (ASS_PCMSoundDriver < 0) {
        return "(none)";
    }

    return SoundDriver_GetName(ASS_PCMSoundDriver);
}


/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int FX_SetCallBack
   (
   void ( *function )( unsigned int )
   )

   {
   int status;

   status = FX_Ok;

	MV_SetCallBack( function );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume
   (
   int volume
   )

   {
	MV_SetVolume( volume );
   }


/*---------------------------------------------------------------------
   Function: FX_GetVolume

   Returns the volume of the current sound device.
---------------------------------------------------------------------*/

int FX_GetVolume
   (
   void
   )

   {
   int volume;

	volume = MV_GetVolume();

   return( volume );
   }


/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo
   (
   int setting
   )

   {
   MV_SetReverseStereo( setting );
   }


/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int FX_GetReverseStereo
   (
   void
   )

   {
   return MV_GetReverseStereo();
   }


/*---------------------------------------------------------------------
   Function: FX_SetReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetReverb
   (
   int reverb
   )

   {
   MV_SetReverb( reverb );
   }


/*---------------------------------------------------------------------
   Function: FX_SetFastReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetFastReverb
   (
   int reverb
   )

   {
   MV_SetFastReverb( reverb );
   }


/*---------------------------------------------------------------------
   Function: FX_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int FX_GetMaxReverbDelay
   (
   void
   )

   {
   return MV_GetMaxReverbDelay();
   }


/*---------------------------------------------------------------------
   Function: FX_GetReverbDelay

   Returns the current delay time for reverb.
---------------------------------------------------------------------*/

int FX_GetReverbDelay
   (
   void
   )

   {
   return MV_GetReverbDelay();
   }


/*---------------------------------------------------------------------
   Function: FX_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void FX_SetReverbDelay
   (
   int delay
   )

   {
   MV_SetReverbDelay( delay );
   }


/*---------------------------------------------------------------------
   Function: FX_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int FX_VoiceAvailable
   (
   int priority
   )

   {
   return MV_VoiceAvailable( priority );
   }

/*---------------------------------------------------------------------
   Function: FX_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int FX_EndLooping
   (
   int handle
   )

   {
   int status;

   status = MV_EndLooping( handle );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }

/*---------------------------------------------------------------------
   Function: FX_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPan
   (
   int handle,
   int vol,
   int left,
   int right
   )

   {
   int status;

   status = MV_SetPan( handle, vol, left, right );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetPitch

   Sets the pitch of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPitch
   (
   int handle,
   int pitchoffset
   )

   {
   int status;

   status = MV_SetPitch( handle, pitchoffset );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetFrequency

   Sets the frequency of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_SetFrequency
   (
   int handle,
   int frequency
   )

   {
   int status;

   status = MV_SetFrequency( handle, frequency );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_GetFrequency

   Gets the frequency of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_GetFrequency
   (
   int handle,
   int *frequency
   )

   {
   int status;

   status = MV_GetFrequency( handle, frequency );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayVOC
   (
   char *ptr,
   unsigned int ptrlength,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayVOC( ptr, ptrlength, pitchoffset, vol, left, right,
      priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayLoopedVOC
   (
   char *ptr,
   unsigned int ptrlength,
   int loopstart,
   int loopend,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayLoopedVOC( ptr, ptrlength, loopstart, loopend, pitchoffset,
      vol, left, right, priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayWAV
   (
   char *ptr,
   unsigned int ptrlength,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayWAV( ptr, ptrlength, pitchoffset, vol, left, right,
      priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayLoopedWAV
   (
   char *ptr,
   unsigned int ptrlength,
   int loopstart,
   int loopend,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayLoopedWAV( ptr, ptrlength, loopstart, loopend,
      pitchoffset, vol, left, right, priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int FX_PlayVOC3D
   (
   char *ptr,
   unsigned int ptrlength,
   int pitchoffset,
   int angle,
   int distance,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayVOC3D( ptr, ptrlength, pitchoffset, angle, distance,
      priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int FX_PlayWAV3D
   (
   char *ptr,
   unsigned int ptrlength,
   int pitchoffset,
   int angle,
   int distance,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayWAV3D( ptr, ptrlength, pitchoffset, angle, distance,
      priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayRaw3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int FX_PlayRaw3D
   (
   char *ptr,
   unsigned int ptrlength,
   unsigned rate,
   int pitchoffset,
   int angle,
   int distance,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayRaw3D( ptr, ptrlength, rate, pitchoffset, angle, distance,
      priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayRaw
   (
   char *ptr,
   unsigned int length,
   unsigned rate,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayRaw( ptr, length, rate, pitchoffset,
      vol, left, right, priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayLoopedRaw
   (
   char *ptr,
   unsigned int length,
   char *loopstart,
   char *loopend,
   unsigned rate,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_PlayLoopedRaw( ptr, length, loopstart, loopend,
      rate, pitchoffset, vol, left, right, priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int FX_Pan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
   int status;

   status = MV_Pan3D( handle, angle, distance );
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SoundActive

   Tests if the specified sound is currently playing.
---------------------------------------------------------------------*/

int FX_SoundActive
   (
   int handle
   )

   {
   return( MV_VoicePlaying( handle ) );
   }


/*---------------------------------------------------------------------
   Function: FX_SoundsPlaying

   Reports the number of voices playing.
---------------------------------------------------------------------*/

int FX_SoundsPlaying
   (
   void
   )

   {
   return( MV_VoicesPlaying() );
   }


/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int FX_StopSound
   (
   int handle
   )

   {
   int status;

   status = MV_Kill( handle );
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      return( FX_Warning );
      }

   return( FX_Ok );
   }


/*---------------------------------------------------------------------
   Function: FX_PauseSound

   Pauses or resumes playback of a specific voice
---------------------------------------------------------------------*/

int FX_PauseSound
   (
   int handle,
   int pauseon
   )

   {
   int status;
   
   status = MV_PauseVoice( handle, pauseon );
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      return( FX_Warning );
      }
   
   return( FX_Ok );
   }


/*---------------------------------------------------------------------
   Function: FX_StopAllSounds

   Halts playback of all sounds.
---------------------------------------------------------------------*/

int FX_StopAllSounds
   (
   void
   )

   {
   int status;

   status = MV_KillAllVoices();
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      return( FX_Warning );
      }

   return( FX_Ok );
   }


/*---------------------------------------------------------------------
   Function: FX_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int FX_StartDemandFeedPlayback
   (
   void ( *function )( char **ptr, unsigned int *length ),
   int rate,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned int callbackval
   )

   {
   int handle;

   handle = MV_StartDemandFeedPlayback( function, rate,
      pitchoffset, vol, left, right, priority, callbackval );
   if ( handle < MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }

   return( handle );
   }


/*---------------------------------------------------------------------
   Function: FX_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int FX_StartRecording
   (
   int MixRate,
   void ( *function )( char *ptr, int length )
   )

   {
   int status;

	(void)MixRate; (void)function;

	FX_SetErrorCode( FX_InvalidCard );
	status = FX_Warning;

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void FX_StopRecord
   (
   void
   )

   {
   }


/*---------------------------------------------------------------------
   Function: FX_PlayAuto

   Play a sound, autodetecting the format.
---------------------------------------------------------------------*/
int FX_PlayAuto( char *ptr, unsigned int length, int pitchoffset, int vol,
                 int left, int right, int priority, unsigned int callbackval )
{
   int handle = -1;
   
   if (!memcmp("Creative Voice File\x1a", ptr, 20)) {
      handle = MV_PlayVOC(ptr, length, pitchoffset, vol, left, right, priority, callbackval);
   } else if (!memcmp("RIFF", ptr, 4) && !memcmp("WAVE", ptr + 8, 4)) {
      handle = MV_PlayWAV(ptr, length, pitchoffset, vol, left, right, priority, callbackval);
   #ifdef HAVE_VORBIS
   } else if (!memcmp("OggS", ptr, 4)) {
      handle = MV_PlayVorbis(ptr, length, pitchoffset, vol, left, right, priority, callbackval);
   #endif
   }
   
   if ( handle < MV_Ok )
   {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
   }
   
   return handle;
}

/*---------------------------------------------------------------------
   Function: FX_PlayLoopedAuto

   Play a looped sound, autodetecting the format.
---------------------------------------------------------------------*/
int FX_PlayLoopedAuto( char *ptr, unsigned int length, int loopstart, int loopend,
                       int pitchoffset, int vol, int left, int right, int priority,
                       unsigned int callbackval )
{
   int handle = -1;
   
   if (!memcmp("Creative Voice File\x1a", ptr, 20)) {
      handle = MV_PlayLoopedVOC(ptr, length, loopstart, loopend, pitchoffset,
                              vol, left, right, priority, callbackval);
   } else if (!memcmp("RIFF", ptr, 4) && !memcmp("WAVE", ptr + 8, 4)) {
      handle = MV_PlayLoopedWAV(ptr, length, loopstart, loopend, pitchoffset,
                              vol, left, right, priority, callbackval);
   #ifdef HAVE_VORBIS
   } else if (!memcmp("OggS", ptr, 4)) {
      handle = MV_PlayLoopedVorbis(ptr, length, loopstart, loopend, pitchoffset,
                              vol, left, right, priority, callbackval);
   #endif
   }
   
   if ( handle < MV_Ok )
   {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
   }
   
   return handle;
}

/*---------------------------------------------------------------------
   Function: FX_PlayAuto3D

   Play a positioned sound, autodetecting the format.
---------------------------------------------------------------------*/
int FX_PlayAuto3D( char *ptr, unsigned int length, int pitchoffset, int angle,
                   int distance, int priority, unsigned int callbackval )
{
   int handle = -1;
   
   if (!memcmp("Creative Voice File\x1a", ptr, 20)) {
      handle = MV_PlayVOC3D(ptr, length, pitchoffset, angle, distance, priority, callbackval);
   } else if (!memcmp("RIFF", ptr, 4) && !memcmp("WAVE", ptr + 8, 4)) {
      handle = MV_PlayWAV3D(ptr, length, pitchoffset, angle, distance, priority, callbackval);
   #ifdef HAVE_VORBIS
   } else if (!memcmp("OggS", ptr, 4)) {
      handle = MV_PlayVorbis3D(ptr, length, pitchoffset, angle, distance, priority, callbackval);
   #endif
   }
   
   if ( handle < MV_Ok )
   {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
   }
   
   return handle;
}

// vim:ts=3:expandtab:

