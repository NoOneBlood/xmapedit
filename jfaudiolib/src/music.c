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
   module: MUSIC.C

   author: James R. Dose
   date:   March 25, 1994

   Device independant music playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sndcards.h"
#include "drivers.h"
#include "music.h"
#include "midi.h"
#include "ll_man.h"
#include "asssys.h"
#include "assmisc.h"

int MUSIC_ErrorCode = MUSIC_Ok;

static midifuncs MUSIC_MidiFunctions;

//static int       MUSIC_FadeLength;
//static int       MUSIC_FadeRate;
//static unsigned  MUSIC_CurrentFadeVolume;
//static unsigned  MUSIC_LastFadeVolume;
//static int       MUSIC_EndingFadeVolume;
//static task     *MUSIC_FadeTask = NULL;

#define MUSIC_SetErrorCode( status ) \
   MUSIC_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: MUSIC_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *MUSIC_ErrorString
   (
   int ErrorNumber
   )

   {
   const char *ErrorString;

   switch( ErrorNumber )
      {
      case MUSIC_Warning :
      case MUSIC_Error :
         ErrorString = MUSIC_ErrorString( MUSIC_ErrorCode );
         break;

      case MUSIC_Ok :
         ErrorString = "Music ok.";
         break;

      case MUSIC_DriverError :
         ErrorString = SoundDriver_MIDI_ErrorString(SoundDriver_MIDI_GetError());
         break;

      case MUSIC_InvalidCard :
         ErrorString = "Invalid Music device.";
         break;

      case MUSIC_MidiError :
         ErrorString = "Error playing MIDI file.";
         break;

      default :
         ErrorString = "Unknown Music error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int MUSIC_Init
   (
   int SoundCard,
   const char *params
   )

   {
   int i;
   int status;
   int mincard, maxcard, card;

   for( i = 0; i < 128; i++ )
      {
      MIDI_PatchMap[ i ] = i;
      }

   if (SoundCard == ASS_AutoDetect)
      {
      mincard = ASS_NoSound + 1;
      maxcard = ASS_NumSoundCards - 1;
      }
   else if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards)
      {
      MUSIC_ErrorCode = MUSIC_InvalidCard;
      return MUSIC_Error;
      }
   else
      {
      mincard = SoundCard;
      maxcard = SoundCard;
      }

   for (card = mincard; card <= maxcard; card++)
      {
      if (!SoundDriver_IsMIDISupported(card))
         {
         continue;
         }
      else if (SoundCard == ASS_AutoDetect)
         {
         ASS_Message("MUSIC_Init: trying %s\n", SoundDriver_GetName(card));
         }

      ASS_MIDISoundDriver = card;
      status = SoundDriver_MIDI_Init(&MUSIC_MidiFunctions, params);
      if (status == MUSIC_Ok)
         {
         MIDI_SetMidiFuncs( &MUSIC_MidiFunctions );
         return MUSIC_Ok;
         }
      }

   if (SoundCard == ASS_AutoDetect)
      {
      // A failure to autodetect falls back to no sound.
      ASS_MIDISoundDriver = ASS_NoSound;
      MIDI_SetMidiFuncs( &MUSIC_MidiFunctions );
      return MUSIC_Ok;
      }

   MUSIC_ErrorCode = MUSIC_DriverError;
   return MUSIC_Error;
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int MUSIC_Shutdown
   (
   void
   )

   {
   MIDI_StopSong();

   /*if ( MUSIC_FadeTask != NULL )
      {
      MUSIC_StopFade();
      }*/

   if (ASS_MIDISoundDriver >= 0) {
      SoundDriver_MIDI_Shutdown();
   }

   return MUSIC_Ok;
   }

int MUSIC_GetCurrentDriver(void)
{
    return ASS_MIDISoundDriver;
}

const char *MUSIC_GetCurrentDriverName(void)
{
    if (ASS_MIDISoundDriver < 0) {
        return "(none)";
    }

    return SoundDriver_GetName(ASS_MIDISoundDriver);
}

/*---------------------------------------------------------------------
   Function: MUSIC_SetMaxFMMidiChannel

   Sets the maximum MIDI channel that FM cards respond to.
---------------------------------------------------------------------*/

void MUSIC_SetMaxFMMidiChannel
   (
   int channel
   )

   {
   (void)channel;
   //AL_SetMaxMidiChannel( channel );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume
   (
   int volume
   )

   {
   volume = max( 0, volume );
   volume = min( volume, 255 );

   /*if ( MUSIC_SoundDevice != -1 )
      {
      MIDI_SetVolume( volume );
      }*/
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetMidiChannelVolume

   Sets the volume of music playback on the specified MIDI channel.
---------------------------------------------------------------------*/

void MUSIC_SetMidiChannelVolume
   (
   int channel,
   int volume
   )

   {
   MIDI_SetUserChannelVolume( channel, volume );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_ResetMidiChannelVolumes

   Sets the volume of music playback on all MIDI channels to full volume.
---------------------------------------------------------------------*/

void MUSIC_ResetMidiChannelVolumes
   (
   void
   )

   {
   MIDI_ResetUserChannelVolume();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int MUSIC_GetVolume
   (
   void
   )

   {
   /*if ( MUSIC_SoundDevice == -1 )
      {
      return( 0 );
      }*/
   return( MIDI_GetVolume() );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetLoopFlag

   Set whether the music will loop or end when it reaches the end of
   the song.
---------------------------------------------------------------------*/

void MUSIC_SetLoopFlag
   (
   int loopflag
   )

   {
   MIDI_SetLoopFlag( loopflag );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int MUSIC_SongPlaying
   (
   void
   )

   {
   return( MIDI_SongPlaying() );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue
   (
   void
   )

   {
   MIDI_ContinueSong();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause
   (
   void
   )

   {
   MIDI_PauseSong();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

int MUSIC_StopSong
   (
   void
   )

   {
   MUSIC_StopFade();
   MIDI_StopSong();
   MUSIC_SetErrorCode( MUSIC_Ok );
   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_PlaySong

   Begins playback of MIDI song.
---------------------------------------------------------------------*/

int MUSIC_PlaySong
   (
   void *song,
   unsigned int length,
   int loopflag
   )

   {
   int status;

   (void)length;

   MIDI_StopSong();
   status = MIDI_PlaySong( song, loopflag );
   if ( status != MIDI_Ok )
      {
      MUSIC_SetErrorCode( MUSIC_MidiError );
      return( MUSIC_Warning );
      }

   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetContext

   Sets the song context.
---------------------------------------------------------------------*/

void MUSIC_SetContext
   (
   int context
   )

   {
   MIDI_SetContext( context );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetContext

   Returns the current song context.
---------------------------------------------------------------------*/

int MUSIC_GetContext
   (
   void
   )

   {
   return MIDI_GetContext();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTick
   (
   unsigned int PositionInTicks
   )

   {
   MIDI_SetSongTick( PositionInTicks );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTime
   (
   unsigned int milliseconds
   )

   {
   MIDI_SetSongTime( milliseconds );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongPosition

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongPosition
   (
   int measure,
   int beat,
   int tick
   )

   {
   MIDI_SetSongPosition( measure, beat, tick );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongPosition

   Returns the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_GetSongPosition
   (
   songposition *pos
   )

   {
   MIDI_GetSongPosition( pos );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongLength

   Returns the length of the song.
---------------------------------------------------------------------*/

void MUSIC_GetSongLength
   (
   songposition *pos
   )

   {
   MIDI_GetSongLength( pos );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_FadeRoutine

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/
/*
static void MUSIC_FadeRoutine
   (
   task *Task
   )

   {
   int volume;

   MUSIC_CurrentFadeVolume += MUSIC_FadeRate;
   if ( MUSIC_FadeLength == 0 )
      {
      MIDI_SetVolume( MUSIC_EndingFadeVolume );
      TS_Terminate( Task );
      MUSIC_FadeTask = NULL;
      }
   else
      {
      MUSIC_FadeLength--;
//      if ( ( MUSIC_SoundDevice == GenMidi ) &&
//         ( ( MUSIC_FadeLength % 12 ) != 0 ) )
//         {
//         return;
//         }

      volume = MUSIC_CurrentFadeVolume >> 7;
      if ( MUSIC_LastFadeVolume != volume )
         {
         MUSIC_LastFadeVolume = volume;
         MIDI_SetVolume( volume );
         }
      }
   }
*/

/*---------------------------------------------------------------------
   Function: MUSIC_FadeVolume

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/

int MUSIC_FadeVolume
   (
   int tovolume,
   int milliseconds
   )

   {
   (void)tovolume; (void)milliseconds;
   /*int fromvolume;

   if ( ( MUSIC_SoundDevice == ProAudioSpectrum ) ||
      ( MUSIC_SoundDevice == SoundMan16 ) ||
      ( MUSIC_SoundDevice == GenMidi ) ||
      ( MUSIC_SoundDevice == SoundScape ) ||
      ( MUSIC_SoundDevice == SoundCanvas ) )
      {
      MIDI_SetVolume( tovolume );
      return( MUSIC_Ok );
      }

   if ( MUSIC_FadeTask != NULL )
      {
      MUSIC_StopFade();
      }

   tovolume = max( 0, tovolume );
   tovolume = min( 255, tovolume );
   fromvolume = MUSIC_GetVolume();

   MUSIC_FadeLength = milliseconds / 25;
   MUSIC_FadeRate   = ( ( tovolume - fromvolume ) << 7 ) / MUSIC_FadeLength;
   MUSIC_LastFadeVolume = fromvolume;
   MUSIC_CurrentFadeVolume = fromvolume << 7;
   MUSIC_EndingFadeVolume = tovolume;

   MUSIC_FadeTask = TS_ScheduleTask( MUSIC_FadeRoutine, 40, 1, NULL );
   if ( MUSIC_FadeTask == NULL )
      {
      MUSIC_SetErrorCode( MUSIC_TaskManError );
      return( MUSIC_Warning );
      }

   TS_Dispatch();*/
   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_FadeActive

   Returns whether the fade routine is active.
---------------------------------------------------------------------*/

int MUSIC_FadeActive
   (
   void
   )

   {
   return 0;//( MUSIC_FadeTask != NULL );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_StopFade

   Stops fading the music.
---------------------------------------------------------------------*/

void MUSIC_StopFade
   (
   void
   )

   {
   /*if ( MUSIC_FadeTask != NULL )
      {
      TS_Terminate( MUSIC_FadeTask );
      MUSIC_FadeTask = NULL;
      }*/
   }


/*---------------------------------------------------------------------
   Function: MUSIC_RerouteMidiChannel

   Sets callback function to reroute MIDI commands from specified
   function.
---------------------------------------------------------------------*/

void MUSIC_RerouteMidiChannel
   (
   int channel,
   int ( *function )( int event, int c1, int c2 )
   )

   {
   MIDI_RerouteMidiChannel( channel, function );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_RegisterTimbreBank

   Halts playback of all sounds.
---------------------------------------------------------------------*/

void MUSIC_RegisterTimbreBank
   (
   unsigned char *timbres
   )

   {
   (void)timbres;
   //AL_RegisterTimbreBank( timbres );
   }
