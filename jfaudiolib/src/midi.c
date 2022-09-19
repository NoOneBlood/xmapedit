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
   module: MIDI.C

   author: James R. Dose
   date:   May 25, 1994

   Midi song file playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "sndcards.h"
#include "drivers.h"
#include "ll_man.h"
#include "music.h"
#include "_midi.h"
#include "midi.h"
#include "asssys.h"
#include "assmisc.h"

int MUSIC_EMIDIPersonality = EMIDI_GeneralMIDI;

static const int _MIDI_CommandLengths[ NUM_MIDI_CHANNELS ] =
   {
   0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0
   };

static int ( *_MIDI_RerouteFunctions[ NUM_MIDI_CHANNELS ] )
   (
   int event,
   int c1,
   int c2
   ) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static track *_MIDI_TrackPtr = NULL;
static int    _MIDI_TrackMemSize;
static int    _MIDI_NumTracks;

static int _MIDI_SongActive = FALSE;
static int _MIDI_SongLoaded = FALSE;
static int _MIDI_Loop = FALSE;

static int  _MIDI_Division;
static int  _MIDI_Tick    = 0;
static int  _MIDI_Beat    = 1;
static int  _MIDI_Measure = 1;
static unsigned _MIDI_Time;
static int  _MIDI_BeatsPerMeasure;
static int  _MIDI_TicksPerBeat;
static int  _MIDI_TimeBase;
static int  _MIDI_FPSecondsPerTick;
static unsigned _MIDI_TotalTime;
static int  _MIDI_TotalTicks;
static int  _MIDI_TotalBeats;
static int  _MIDI_TotalMeasures;

static unsigned int _MIDI_PositionInTicks;

static int  _MIDI_Context;

static int _MIDI_ActiveTracks;
static int _MIDI_TotalVolume = MIDI_MaxVolume;

static int _MIDI_ChannelVolume[ NUM_MIDI_CHANNELS ];
static int _MIDI_UserChannelVolume[ NUM_MIDI_CHANNELS ] =
   {
   256, 256, 256, 256, 256, 256, 256, 256,
   256, 256, 256, 256, 256, 256, 256, 256
   };

static midifuncs *_MIDI_Funcs = NULL;

static int Reset = FALSE;

int MIDI_Tempo = 120;

char MIDI_PatchMap[ 128 ];

static const unsigned char _GSResetSysex[] =
    {
    0xF0, 0x41, 0x10, 0x42, 0x12, 0x40,
    0x00, 0x7F, 0x00, 0x41, 0xF7
    };


/*---------------------------------------------------------------------
   Function: _MIDI_ReadNumber

   Reads a variable length number from a MIDI track.
---------------------------------------------------------------------*/

static int _MIDI_ReadNumber
   (
   void *from,
   size_t size
   )

   {
   unsigned char *FromPtr;
   int           value;

   if ( size > 4 )
      {
      size = 4;
      }

   FromPtr = ( unsigned char * )from;

   value = 0;
   while( size-- )
      {
      value <<= 8;
      value += *FromPtr++;
      }

   return( value );
   }


/*---------------------------------------------------------------------
   Function: _MIDI_ReadDelta

   Reads a variable length encoded delta delay time from the MIDI data.
---------------------------------------------------------------------*/

static int _MIDI_ReadDelta
   (
   track *ptr
   )

   {
   int           value;
   unsigned char c;

   GET_NEXT_EVENT( ptr, value );

   if ( value & 0x80 )
      {
      value &= 0x7f;
      do
         {
         GET_NEXT_EVENT( ptr, c );
         value = ( value << 7 ) + ( c & 0x7f );
         }
      while ( c & 0x80 );
      }

   return( value );
   }


/*---------------------------------------------------------------------
   Function: _MIDI_ResetTracks

   Sets the track pointers to the beginning of the song.
---------------------------------------------------------------------*/

static void _MIDI_ResetTracks
   (
   void
   )

   {
   int    i;
   track *ptr;

   _MIDI_Tick = 0;
   _MIDI_Beat = 1;
   _MIDI_Measure = 1;
   _MIDI_Time = 0;
   _MIDI_BeatsPerMeasure = 4;
   _MIDI_TicksPerBeat = _MIDI_Division;
   _MIDI_TimeBase = 4;

   _MIDI_PositionInTicks = 0;
   _MIDI_ActiveTracks    = 0;
   _MIDI_Context         = 0;

   ptr = _MIDI_TrackPtr;
   for( i = 0; i < _MIDI_NumTracks; i++ )
      {
      ptr->pos                    = ptr->start;
      ptr->delay                  = _MIDI_ReadDelta( ptr );
      ptr->active                 = ptr->EMIDI_IncludeTrack;
      ptr->RunningStatus          = 0;
      ptr->currentcontext         = 0;
      ptr->context[ 0 ].loopstart = ptr->start;
      ptr->context[ 0 ].loopcount = 0;

      if ( ptr->active )
         {
         _MIDI_ActiveTracks++;
         }

      ptr++;
      }
   }


/*---------------------------------------------------------------------
   Function: _MIDI_AdvanceTick

   Increment tick counters.
---------------------------------------------------------------------*/

static void _MIDI_AdvanceTick
   (
   void
   )

   {
   _MIDI_PositionInTicks++;
   _MIDI_Time += _MIDI_FPSecondsPerTick;

   _MIDI_Tick++;
   while( _MIDI_Tick > _MIDI_TicksPerBeat )
      {
      _MIDI_Tick -= _MIDI_TicksPerBeat;
      _MIDI_Beat++;
      }
   while( _MIDI_Beat > _MIDI_BeatsPerMeasure )
      {
      _MIDI_Beat -= _MIDI_BeatsPerMeasure;
      _MIDI_Measure++;
      }
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SysEx

   Interpret SysEx Event.
---------------------------------------------------------------------*/

static void _MIDI_SysEx
   (
   track *Track
   )

   {
   int length;

   length = _MIDI_ReadDelta( Track );
   
   if (_MIDI_Funcs->SysEx && _MIDI_SongLoaded)
      {
      unsigned char *msg;
      
      msg = (unsigned char *)alloca(length + 1);
      
      msg[0] = MIDI_SYSEX;
      memcpy(&msg[1], Track->pos, length);

      // only send a sysex if it's not a GS reset
      if (memcmp(msg, _GSResetSysex, sizeof(_GSResetSysex)))
         {
         _MIDI_Funcs->SysEx(msg, length + 1);
         }
      }

   Track->pos += length;
   }


/*---------------------------------------------------------------------
   Function: _MIDI_MetaEvent

   Interpret Meta Event.
---------------------------------------------------------------------*/

static void _MIDI_MetaEvent
   (
   track *Track
   )

   {
   int   command;
   int   length;
   int   denominator;
   int   tempo;

   GET_NEXT_EVENT( Track, command );
   GET_NEXT_EVENT( Track, length );

   switch( command )
      {
      case MIDI_END_OF_TRACK :
         Track->active = FALSE;

         _MIDI_ActiveTracks--;
         break;

      case MIDI_TEMPO_CHANGE :
         tempo = 60000000L / _MIDI_ReadNumber( Track->pos, 3 );
         if (_MIDI_SongLoaded)
            {
            MIDI_SetTempo( tempo );
            }
         break;

      case MIDI_TIME_SIGNATURE :
         if ( ( _MIDI_Tick > 0 ) || ( _MIDI_Beat > 1 ) )
            {
            _MIDI_Measure++;
            }

         _MIDI_Tick = 0;
         _MIDI_Beat = 1;

         _MIDI_BeatsPerMeasure = (int)*Track->pos;
         denominator = (int)*( Track->pos + 1 );
         _MIDI_TimeBase = 1;
         while( denominator > 0 )
            {
            _MIDI_TimeBase += _MIDI_TimeBase;
            denominator--;
            }
         _MIDI_TicksPerBeat = ( _MIDI_Division * 4 ) / _MIDI_TimeBase;
         break;
      }

   Track->pos += length;
   }


/*---------------------------------------------------------------------
   Function: _MIDI_InterpretControllerInfo

   Interprets the MIDI controller info.
---------------------------------------------------------------------*/

static int _MIDI_InterpretControllerInfo
   (
   track *Track,
   int   TimeSet,
   int   channel,
   int   c1,
   int   c2
   )

   {
   track *trackptr;
   int tracknum;
   int loopcount;

   switch( c1 )
      {
      case MIDI_MONO_MODE_ON :
         Track->pos++;
         break;

      case MIDI_VOLUME :
         if ( !Track->EMIDI_VolumeChange )
            {
            _MIDI_SetChannelVolume( channel, c2 );
            }
         break;

      case EMIDI_INCLUDE_TRACK :
      case EMIDI_EXCLUDE_TRACK :
         break;

      case EMIDI_PROGRAM_CHANGE :
         if ( Track->EMIDI_ProgramChange )
            {
            _MIDI_Funcs->ProgramChange( channel, MIDI_PatchMap[ c2 & 0x7f ] );
            }
         break;

      case EMIDI_VOLUME_CHANGE :
         if ( Track->EMIDI_VolumeChange )
            {
            _MIDI_SetChannelVolume( channel, c2 );
            }
         break;

      case EMIDI_CONTEXT_START :
         break;

      case EMIDI_CONTEXT_END :
         if ( ( Track->currentcontext == _MIDI_Context ) ||
            ( _MIDI_Context < 0 ) ||
            ( Track->context[ _MIDI_Context ].pos == NULL ) )
            {
            break;
            }

         Track->currentcontext = _MIDI_Context;
         Track->context[ 0 ].loopstart = Track->context[ _MIDI_Context ].loopstart;
         Track->context[ 0 ].loopcount = Track->context[ _MIDI_Context ].loopcount;
         Track->pos           = Track->context[ _MIDI_Context ].pos;
         Track->RunningStatus = Track->context[ _MIDI_Context ].RunningStatus;

         if ( TimeSet )
            {
            break;
            }

         _MIDI_Time             = Track->context[ _MIDI_Context ].time;
         _MIDI_FPSecondsPerTick = Track->context[ _MIDI_Context ].FPSecondsPerTick;
         _MIDI_Tick             = Track->context[ _MIDI_Context ].tick;
         _MIDI_Beat             = Track->context[ _MIDI_Context ].beat;
         _MIDI_Measure          = Track->context[ _MIDI_Context ].measure;
         _MIDI_BeatsPerMeasure  = Track->context[ _MIDI_Context ].BeatsPerMeasure;
         _MIDI_TicksPerBeat     = Track->context[ _MIDI_Context ].TicksPerBeat;
         _MIDI_TimeBase         = Track->context[ _MIDI_Context ].TimeBase;
         TimeSet = TRUE;
         break;

      case EMIDI_LOOP_START :
      case EMIDI_SONG_LOOP_START :
         if ( c2 == 0 )
            {
            loopcount = EMIDI_INFINITE;
            }
         else
            {
            loopcount = c2;
            }

         if ( c1 == EMIDI_SONG_LOOP_START )
            {
            trackptr = _MIDI_TrackPtr;
            tracknum = _MIDI_NumTracks;
            }
         else
            {
            trackptr = Track;
            tracknum = 1;
            }

         while( tracknum > 0 )
            {
            trackptr->context[ 0 ].loopcount        = loopcount;
            trackptr->context[ 0 ].pos              = trackptr->pos;
            trackptr->context[ 0 ].loopstart        = trackptr->pos;
            trackptr->context[ 0 ].RunningStatus    = trackptr->RunningStatus;
            trackptr->context[ 0 ].active           = trackptr->active;
            trackptr->context[ 0 ].delay            = trackptr->delay;
            trackptr->context[ 0 ].time             = _MIDI_Time;
            trackptr->context[ 0 ].FPSecondsPerTick = _MIDI_FPSecondsPerTick;
            trackptr->context[ 0 ].tick             = _MIDI_Tick;
            trackptr->context[ 0 ].beat             = _MIDI_Beat;
            trackptr->context[ 0 ].measure          = _MIDI_Measure;
            trackptr->context[ 0 ].BeatsPerMeasure  = _MIDI_BeatsPerMeasure;
            trackptr->context[ 0 ].TicksPerBeat     = _MIDI_TicksPerBeat;
            trackptr->context[ 0 ].TimeBase         = _MIDI_TimeBase;
            trackptr++;
            tracknum--;
            }
         break;

      case EMIDI_LOOP_END :
      case EMIDI_SONG_LOOP_END :
         if ( ( c2 != EMIDI_END_LOOP_VALUE ) ||
            ( Track->context[ 0 ].loopstart == NULL ) ||
            ( Track->context[ 0 ].loopcount == 0 ) )
            {
            break;
            }

         if ( c1 == EMIDI_SONG_LOOP_END )
            {
            trackptr = _MIDI_TrackPtr;
            tracknum = _MIDI_NumTracks;
            _MIDI_ActiveTracks = 0;
            }
         else
            {
            trackptr = Track;
            tracknum = 1;
            _MIDI_ActiveTracks--;
            }

         while( tracknum > 0 )
            {
            if ( trackptr->context[ 0 ].loopcount != EMIDI_INFINITE )
               {
               trackptr->context[ 0 ].loopcount--;
               }

            trackptr->pos           = trackptr->context[ 0 ].loopstart;
            trackptr->RunningStatus = trackptr->context[ 0 ].RunningStatus;
            trackptr->delay         = trackptr->context[ 0 ].delay;
            trackptr->active        = trackptr->context[ 0 ].active;
            if ( trackptr->active )
               {
               _MIDI_ActiveTracks++;
               }

            if ( !TimeSet )
               {
               _MIDI_Time             = trackptr->context[ 0 ].time;
               _MIDI_FPSecondsPerTick = trackptr->context[ 0 ].FPSecondsPerTick;
               _MIDI_Tick             = trackptr->context[ 0 ].tick;
               _MIDI_Beat             = trackptr->context[ 0 ].beat;
               _MIDI_Measure          = trackptr->context[ 0 ].measure;
               _MIDI_BeatsPerMeasure  = trackptr->context[ 0 ].BeatsPerMeasure;
               _MIDI_TicksPerBeat     = trackptr->context[ 0 ].TicksPerBeat;
               _MIDI_TimeBase         = trackptr->context[ 0 ].TimeBase;
               TimeSet = TRUE;
               }

            trackptr++;
            tracknum--;
            }
         break;

      default :
         if ( _MIDI_Funcs->ControlChange )
            {
            _MIDI_Funcs->ControlChange( channel, c1, c2 );
            }
      }

   return TimeSet;
   }


/*---------------------------------------------------------------------
   Function: _MIDI_ServiceRoutine

   Task that interperates the MIDI data.
---------------------------------------------------------------------*/
static void _MIDI_ServiceRoutine
   (
   void
   )

   {
   int   event;
   int   channel;
   int   command;
   track *Track;
   int   tracknum;
   int   status;
   int   c1 = 0;
   int   c2 = 0;
   int   TimeSet = FALSE;

   if ( !_MIDI_SongActive )
      {
      return;
      }

   Track = _MIDI_TrackPtr;
   tracknum = 0;
   while( tracknum < _MIDI_NumTracks )
      {
      while ( ( Track->active ) && ( Track->delay == 0 ) )
         {
         GET_NEXT_EVENT( Track, event );

         if ( GET_MIDI_COMMAND( event ) == MIDI_SPECIAL )
            {
            switch( event )
               {
               case MIDI_SYSEX :
               case MIDI_SYSEX_CONTINUE :
                  _MIDI_SysEx( Track );
                  break;

               case MIDI_META_EVENT :
                  _MIDI_MetaEvent( Track );
                  break;
               }

            if ( Track->active )
               {
               Track->delay = _MIDI_ReadDelta( Track );
               }

            continue;
            }

         if ( event & MIDI_RUNNING_STATUS )
            {
            Track->RunningStatus = event;
            }
         else
            {
            event = Track->RunningStatus;
            Track->pos--;
            }

         channel = GET_MIDI_CHANNEL( event );
         command = GET_MIDI_COMMAND( event );

         if ( _MIDI_CommandLengths[ command ] > 0 )
            {
            GET_NEXT_EVENT( Track, c1 );
            if ( _MIDI_CommandLengths[ command ] > 1 )
               {
               GET_NEXT_EVENT( Track, c2 );
               }
            }

         if ( _MIDI_RerouteFunctions[ channel ] != NULL )
            {
            status = _MIDI_RerouteFunctions[ channel ]( event, c1, c2 );

            if ( status == MIDI_DONT_PLAY )
               {
               Track->delay = _MIDI_ReadDelta( Track );
               continue;
               }
            }

         switch ( command )
            {
            case MIDI_NOTE_OFF :
               if ( _MIDI_Funcs->NoteOff )
                  {
                  _MIDI_Funcs->NoteOff( channel, c1, c2 );
                  }
               break;

            case MIDI_NOTE_ON :
               if ( _MIDI_Funcs->NoteOn )
                  {
                  _MIDI_Funcs->NoteOn( channel, c1, c2 );
                  }
               break;

            case MIDI_POLY_AFTER_TCH :
               if ( _MIDI_Funcs->PolyAftertouch )
                  {
                  _MIDI_Funcs->PolyAftertouch( channel, c1, c2 );
                  }
               break;

            case MIDI_CONTROL_CHANGE :
               TimeSet = _MIDI_InterpretControllerInfo( Track, TimeSet,
                  channel, c1, c2 );
               break;

            case MIDI_PROGRAM_CHANGE :
               if ( ( _MIDI_Funcs->ProgramChange ) &&
                  ( !Track->EMIDI_ProgramChange ) )
                  {
                  _MIDI_Funcs->ProgramChange( channel, MIDI_PatchMap[ c1 & 0x7f ] );
                  }
               break;

            case MIDI_AFTER_TOUCH :
               if ( _MIDI_Funcs->ChannelAftertouch )
                  {
                  _MIDI_Funcs->ChannelAftertouch( channel, c1 );
                  }
               break;

            case MIDI_PITCH_BEND :
               if ( _MIDI_Funcs->PitchBend )
                  {
                  _MIDI_Funcs->PitchBend( channel, c1, c2 );
                  }
               break;

            default :
               break;
            }

         Track->delay = _MIDI_ReadDelta( Track );
         }

      Track->delay--;
      Track++;
      tracknum++;

      if ( _MIDI_ActiveTracks == 0 )
         {
         _MIDI_ResetTracks();
         if ( _MIDI_Loop )
            {
            tracknum = 0;
            Track = _MIDI_TrackPtr;
            }
         else
            {
            _MIDI_SongActive = FALSE;
            break;
            }
         }
      }

   _MIDI_AdvanceTick();
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SendControlChange

   Sends a control change to the proper device
---------------------------------------------------------------------*/

static int _MIDI_SendControlChange
   (
   int channel,
   int c1,
   int c2
   )

   {
   int status;

   if ( _MIDI_RerouteFunctions[ channel ] != NULL )
      {
      status = _MIDI_RerouteFunctions[ channel ]( 0xB0 + channel,
         c1, c2 );
      if ( status == MIDI_DONT_PLAY )
         {
         return( MIDI_Ok );
         }
      }

   if ( _MIDI_Funcs == NULL )
      {
      return( MIDI_Error );
      }

   if ( _MIDI_Funcs->ControlChange == NULL )
      {
      return( MIDI_Error );
      }

   _MIDI_Funcs->ControlChange( channel, c1, c2 );

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SendProgramChange

   Sends a program change to the proper device
---------------------------------------------------------------------*/

static int _MIDI_SendProgramChange
   (
   int channel,
   int c1
   )

   {
   int status;

   if ( _MIDI_RerouteFunctions[ channel ] != NULL )
      {
      status = _MIDI_RerouteFunctions[ channel ]( 0xC0 + channel,
         c1, 0 );
      if ( status == MIDI_DONT_PLAY )
         {
         return( MIDI_Ok );
         }
      }

   if ( _MIDI_Funcs == NULL )
      {
      return( MIDI_Error );
      }

   if ( _MIDI_Funcs->ProgramChange == NULL )
      {
      return( MIDI_Error );
      }

   _MIDI_Funcs->ProgramChange( channel, c1 );

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: MIDI_RerouteMidiChannel

   Sets callback function to reroute MIDI commands from specified
   function.
---------------------------------------------------------------------*/

void MIDI_RerouteMidiChannel
   (
   int channel,
   int ( *function )( int event, int c1, int c2 )
   )

   {
   if ( ( channel >= 1 ) && ( channel <= 16 ) )
      {
      _MIDI_RerouteFunctions[ channel - 1 ] = function;
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_AllNotesOff

   Sends all notes off commands on all midi channels.
---------------------------------------------------------------------*/

int MIDI_AllNotesOff
   (
   void
   )

   {
   int channel;

   SoundDriver_MIDI_Lock();

   for( channel = 0; channel < NUM_MIDI_CHANNELS; channel++ )
      {
      _MIDI_SendControlChange( channel, MIDI_HOLD1, 0 );
      _MIDI_SendControlChange( channel, MIDI_SOSTENUTO, 0 );   // TURRICAN's recommendation
      _MIDI_SendControlChange( channel, MIDI_ALL_NOTES_OFF, 0 );
      _MIDI_SendControlChange( channel, MIDI_ALL_SOUNDS_OFF, 0 );
      }

   SoundDriver_MIDI_Unlock();

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SetChannelVolume

   Sets the volume of the specified midi channel.
---------------------------------------------------------------------*/

static void _MIDI_SetChannelVolume
   (
   int channel,
   int volume
   )

   {
   int status;
   int remotevolume;

   _MIDI_ChannelVolume[ channel ] = volume;

   if ( _MIDI_RerouteFunctions[ channel ] != NULL )
      {
      remotevolume = volume * _MIDI_TotalVolume;
      remotevolume *= _MIDI_UserChannelVolume[ channel ];
      remotevolume /= MIDI_MaxVolume;
      remotevolume >>= 8;

      status = _MIDI_RerouteFunctions[ channel ]( 0xB0 + channel,
         MIDI_VOLUME, remotevolume );
      if ( status == MIDI_DONT_PLAY )
         {
         return;
         }
      }

   if ( _MIDI_Funcs == NULL )
      {
      return;
      }

   if ( _MIDI_Funcs->ControlChange == NULL )
      {
      return;
      }

   // For user volume
   volume *= _MIDI_UserChannelVolume[ channel ];

   if ( _MIDI_Funcs->SetVolume == NULL )
      {
      volume *= _MIDI_TotalVolume;
      volume /= MIDI_MaxVolume;
      }

   // For user volume
   volume >>= 8;

   _MIDI_Funcs->ControlChange( channel, MIDI_VOLUME, volume );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetUserChannelVolume

   Sets the volume of the specified midi channel.
---------------------------------------------------------------------*/

void MIDI_SetUserChannelVolume
   (
   int channel,
   int volume
   )

   {
   // Convert channel from 1-16 to 0-15
   channel--;

   volume = max( 0, volume );
   volume = min( volume, 256 );

   if ( ( channel >= 0 ) && ( channel < NUM_MIDI_CHANNELS ) )
      {
      _MIDI_UserChannelVolume[ channel ] = volume;

      SoundDriver_MIDI_Lock();
      _MIDI_SetChannelVolume( channel, _MIDI_ChannelVolume[ channel ] );
      SoundDriver_MIDI_Unlock();
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_ResetUserChannelVolume

   Sets the volume of the specified midi channel.
---------------------------------------------------------------------*/

void MIDI_ResetUserChannelVolume
   (
   void
   )

   {
   int channel;

   for( channel = 0; channel < NUM_MIDI_CHANNELS; channel++ )
      {
      _MIDI_UserChannelVolume[ channel ] = 256;
      }

   SoundDriver_MIDI_Lock();
   _MIDI_SendChannelVolumes();
   SoundDriver_MIDI_Unlock();
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SendChannelVolumes

   Sets the volume on all the midi channels.
---------------------------------------------------------------------*/

static void _MIDI_SendChannelVolumes
   (
   void
   )

   {
   int channel;

   for( channel = 0; channel < NUM_MIDI_CHANNELS; channel++ )
      {
      _MIDI_SetChannelVolume( channel, _MIDI_ChannelVolume[ channel ] );
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_Reset

   Resets the MIDI device to General Midi defaults.
---------------------------------------------------------------------*/

int MIDI_Reset
   (
   void
   )

   {
   int channel;

   MIDI_AllNotesOff();

   ASS_Sleep(50);
   
   SoundDriver_MIDI_Lock();

   for( channel = 0; channel < NUM_MIDI_CHANNELS; channel++ )
      {
      _MIDI_SendControlChange( channel, MIDI_RESET_ALL_CONTROLLERS, 0 );
      _MIDI_SendControlChange( channel, MIDI_RPN_MSB, MIDI_PITCHBEND_MSB );
      _MIDI_SendControlChange( channel, MIDI_RPN_LSB, MIDI_PITCHBEND_LSB );
      _MIDI_SendControlChange( channel, MIDI_DATAENTRY_MSB, 2 ); /* Pitch Bend Sensitivity MSB */
      _MIDI_SendControlChange( channel, MIDI_DATAENTRY_LSB, 0 ); /* Pitch Bend Sensitivity LSB */
      _MIDI_ChannelVolume[ channel ] = GENMIDI_DefaultVolume;
      _MIDI_SendControlChange( channel, MIDI_PAN, 64 );  // begin TURRICAN's recommendation
      _MIDI_SendControlChange( channel, MIDI_REVERB, 40 );
      _MIDI_SendControlChange( channel, MIDI_CHORUS, 0 );
      _MIDI_SendControlChange( channel, MIDI_BANK_SELECT_MSB, 0 );
      _MIDI_SendControlChange( channel, MIDI_BANK_SELECT_LSB, 0 );
      _MIDI_SendProgramChange( channel, 0 ); // end TURRICAN's recommendation
      }

   _MIDI_SendGSReset();
   _MIDI_SendChannelVolumes();

   SoundDriver_MIDI_Unlock();

   Reset = TRUE;

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetVolume

   Sets the total volume of the music.
---------------------------------------------------------------------*/

int MIDI_SetVolume
   (
   int volume
   )

   {
   int i;

   if ( _MIDI_Funcs == NULL )
      {
      return( MIDI_NullMidiModule );
      }

   volume = min( MIDI_MaxVolume, volume );
   volume = max( 0, volume );

   _MIDI_TotalVolume = volume;

   SoundDriver_MIDI_Lock();

   if ( _MIDI_Funcs->SetVolume )
      {
      _MIDI_Funcs->SetVolume( volume );

      for( i = 0; i < NUM_MIDI_CHANNELS; i++ )
         {
         if ( _MIDI_RerouteFunctions[ i ] != NULL )
            {
            _MIDI_SetChannelVolume( i, _MIDI_ChannelVolume[ i ] );
            }
         }
      }
   else
      {
      _MIDI_SendChannelVolumes();
      }

   SoundDriver_MIDI_Unlock();

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: MIDI_GetVolume

   Returns the total volume of the music.
---------------------------------------------------------------------*/

int MIDI_GetVolume
   (
   void
   )

   {
   int volume;

   if ( _MIDI_Funcs == NULL )
      {
      return( MIDI_NullMidiModule );
      }

   SoundDriver_MIDI_Lock();

   if ( _MIDI_Funcs->GetVolume )
      {
      volume = _MIDI_Funcs->GetVolume();
      }
   else
      {
      volume = _MIDI_TotalVolume;
      }

   SoundDriver_MIDI_Unlock();

   return( volume );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetContext

   Sets the song context.
---------------------------------------------------------------------*/

void MIDI_SetContext
   (
   int context
   )

   {
   if ( ( context > 0 ) && ( context < EMIDI_NUM_CONTEXTS ) )
      {
      _MIDI_Context = context;
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_GetContext

   Returns the current song context.
---------------------------------------------------------------------*/

int MIDI_GetContext
   (
   void
   )

   {
   return _MIDI_Context;
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetLoopFlag

   Sets whether the song should loop when finished or not.
---------------------------------------------------------------------*/

void MIDI_SetLoopFlag
   (
   int loopflag
   )

   {
   _MIDI_Loop = loopflag;
   }


/*---------------------------------------------------------------------
   Function: MIDI_ContinueSong

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MIDI_ContinueSong
   (
   void
   )

   {
   if ( _MIDI_SongLoaded )
      {
      _MIDI_SongActive = TRUE;
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_PauseSong

   Pauses playback of the current song.
---------------------------------------------------------------------*/

void MIDI_PauseSong
   (
   void
   )

   {
   if ( _MIDI_SongLoaded )
      {
      _MIDI_SongActive = FALSE;
      MIDI_AllNotesOff();
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_SongPlaying

   Returns whether a song is playing or not.
---------------------------------------------------------------------*/

int MIDI_SongPlaying
   (
   void
   )

   {
   return( _MIDI_SongActive );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetMidiFuncs

   Selects the routines that send the MIDI data to the music device.
---------------------------------------------------------------------*/

void MIDI_SetMidiFuncs
   (
   midifuncs *funcs
   )

   {
   _MIDI_Funcs = funcs;
   }


/*---------------------------------------------------------------------
   Function: MIDI_StopSong

   Stops playback of the currently playing song.
---------------------------------------------------------------------*/

void MIDI_StopSong
   (
   void
   )

   {
   if ( _MIDI_SongLoaded )
      {
      SoundDriver_MIDI_HaltPlayback();

      _MIDI_SongActive = FALSE;
      _MIDI_SongLoaded = FALSE;

      MIDI_Reset();
      
      _MIDI_ResetTracks();

      if ( _MIDI_Funcs->ReleasePatches )
         {
         _MIDI_Funcs->ReleasePatches();
         }

      free( _MIDI_TrackPtr );

      _MIDI_TrackPtr     = NULL;
      _MIDI_NumTracks    = 0;
      _MIDI_TrackMemSize = 0;

      _MIDI_TotalTime     = 0;
      _MIDI_TotalTicks    = 0;
      _MIDI_TotalBeats    = 0;
      _MIDI_TotalMeasures = 0;
      }
   }


/*---------------------------------------------------------------------
   Function: MIDI_PlaySong

   Begins playback of a MIDI song.
---------------------------------------------------------------------*/

int MIDI_PlaySong
   (
   unsigned char *song,
   int loopflag
   )

   {
   int    numtracks;
   int    format;
   int    headersize;
   int    tracklength;
   track *CurrentTrack;
   unsigned char *ptr;

   if ( _MIDI_SongLoaded )
      {
      MIDI_StopSong();
      }

   _MIDI_Loop = loopflag;

   if ( _MIDI_Funcs == NULL )
      {
      return( MIDI_NullMidiModule );
      }

   if ( *( unsigned int * )song != LITTLE32(MIDI_HEADER_SIGNATURE) )
      {
      return( MIDI_InvalidMidiFile );
      }

   song += 4;

   headersize      = _MIDI_ReadNumber( song, 4 );
   song += 4;
   format          = _MIDI_ReadNumber( song, 2 );
   _MIDI_NumTracks = _MIDI_ReadNumber( song + 2, 2 );
   _MIDI_Division  = _MIDI_ReadNumber( song + 4, 2 );
   if ( _MIDI_Division < 0 )
      {
      // If a SMPTE time division is given, just set to 96 so no errors occur
      _MIDI_Division = 96;
      }

   if ( format > MAX_FORMAT )
      {
      return( MIDI_UnknownMidiFormat );
      }

   ptr = song + headersize;

   if ( _MIDI_NumTracks == 0 )
      {
      return( MIDI_NoTracks );
      }

   _MIDI_TrackMemSize = _MIDI_NumTracks  * sizeof( track );
   _MIDI_TrackPtr = (track *) malloc(_MIDI_TrackMemSize);
   if ( !_MIDI_TrackPtr )
      {
      return( MIDI_NoMemory );
      }

   CurrentTrack = _MIDI_TrackPtr;
   numtracks    = _MIDI_NumTracks;
   while( numtracks-- )
      {
      if ( *( unsigned int * )ptr != LITTLE32(MIDI_TRACK_SIGNATURE) )
         {
         free( _MIDI_TrackPtr );

         _MIDI_TrackPtr = NULL;
         _MIDI_TrackMemSize = 0;

         return( MIDI_InvalidTrack );
         }

      tracklength = _MIDI_ReadNumber( ptr + 4, 4 );
      ptr += 8;
      CurrentTrack->start = ptr;
      ptr += tracklength;
      CurrentTrack++;
      }

   if ( _MIDI_Funcs->GetVolume != NULL )
      {
      _MIDI_TotalVolume = _MIDI_Funcs->GetVolume();
      }

   _MIDI_InitEMIDI();

   if ( _MIDI_Funcs->LoadPatch )
      {
      MIDI_LoadTimbres();
      }

   _MIDI_ResetTracks();

   if ( !Reset )
      {
      MIDI_Reset();
      }

   Reset = FALSE;

   MIDI_SetTempo( 120 );

    if (SoundDriver_MIDI_StartPlayback(_MIDI_ServiceRoutine) != MIDI_Ok) {
        return MIDI_DriverError;
    }

   _MIDI_SongLoaded = TRUE;
   _MIDI_SongActive = TRUE;

   return( MIDI_Ok );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetTempo

   Sets the song tempo.
---------------------------------------------------------------------*/

void MIDI_SetTempo
   (
   int tempo
   )

   {
   int tickspersecond;

   MIDI_Tempo = tempo;
   SoundDriver_MIDI_SetTempo(tempo, _MIDI_Division);

   tickspersecond = ( tempo * _MIDI_Division ) / 60;
   _MIDI_FPSecondsPerTick = ( 1 << TIME_PRECISION ) / tickspersecond;
   }


/*---------------------------------------------------------------------
   Function: MIDI_GetTempo

   Returns the song tempo.
---------------------------------------------------------------------*/

int MIDI_GetTempo
   (
   void
   )

   {
   return( MIDI_Tempo );
   }


/*---------------------------------------------------------------------
   Function: _MIDI_ProcessNextTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

static int _MIDI_ProcessNextTick
   (
   void
   )

   {
   int   event;
   int   channel;
   int   command;
   track *Track;
   int   tracknum;
   int   status;
   int   c1 = 0;
   int   c2 = 0;
   int   TimeSet = FALSE;

   Track = _MIDI_TrackPtr;
   tracknum = 0;
   while( ( tracknum < _MIDI_NumTracks ) && ( Track != NULL ) )
      {
      while ( ( Track->active ) && ( Track->delay == 0 ) )
         {
         GET_NEXT_EVENT( Track, event );

         if ( GET_MIDI_COMMAND( event ) == MIDI_SPECIAL )
            {
            switch( event )
               {
               case MIDI_SYSEX :
               case MIDI_SYSEX_CONTINUE :
                  _MIDI_SysEx( Track );
                  break;

               case MIDI_META_EVENT :
                  _MIDI_MetaEvent( Track );
                  break;
               }

            if ( Track->active )
               {
               Track->delay = _MIDI_ReadDelta( Track );
               }

            continue;
            }

         if ( event & MIDI_RUNNING_STATUS )
            {
            Track->RunningStatus = event;
            }
         else
            {
            event = Track->RunningStatus;
            Track->pos--;
            }

         channel = GET_MIDI_CHANNEL( event );
         command = GET_MIDI_COMMAND( event );

         if ( _MIDI_CommandLengths[ command ] > 0 )
            {
            GET_NEXT_EVENT( Track, c1 );
            if ( _MIDI_CommandLengths[ command ] > 1 )
               {
               GET_NEXT_EVENT( Track, c2 );
               }
            }

         if ( _MIDI_RerouteFunctions[ channel ] != NULL )
            {
            status = _MIDI_RerouteFunctions[ channel ]( event, c1, c2 );

            if ( status == MIDI_DONT_PLAY )
               {
               Track->delay = _MIDI_ReadDelta( Track );
               continue;
               }
            }

         switch ( command )
            {
            case MIDI_NOTE_OFF :
               break;

            case MIDI_NOTE_ON :
               break;

            case MIDI_POLY_AFTER_TCH :
               if ( _MIDI_Funcs->PolyAftertouch )
                  {
                  _MIDI_Funcs->PolyAftertouch( channel, c1, c2 );
                  }
               break;

            case MIDI_CONTROL_CHANGE :
               TimeSet = _MIDI_InterpretControllerInfo( Track, TimeSet,
                  channel, c1, c2 );
               break;

            case MIDI_PROGRAM_CHANGE :
               if ( ( _MIDI_Funcs->ProgramChange ) &&
                  ( !Track->EMIDI_ProgramChange ) )
                  {
                  _MIDI_Funcs->ProgramChange( channel, c1 );
                  }
               break;

            case MIDI_AFTER_TOUCH :
               if ( _MIDI_Funcs->ChannelAftertouch )
                  {
                  _MIDI_Funcs->ChannelAftertouch( channel, c1 );
                  }
               break;

            case MIDI_PITCH_BEND :
               if ( _MIDI_Funcs->PitchBend )
                  {
                  _MIDI_Funcs->PitchBend( channel, c1, c2 );
                  }
               break;

            default :
               break;
            }

         Track->delay = _MIDI_ReadDelta( Track );
         }

      Track->delay--;
      Track++;
      tracknum++;

      if ( _MIDI_ActiveTracks == 0 )
         {
         break;
         }
      }

   _MIDI_AdvanceTick();

   return( TimeSet );
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetSongTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MIDI_SetSongTick
   (
   unsigned int PositionInTicks
   )

   {
   if ( !_MIDI_SongLoaded )
      {
      return;
      }

   MIDI_PauseSong();

   if ( PositionInTicks < _MIDI_PositionInTicks )
      {
      SoundDriver_MIDI_Lock();
      _MIDI_ResetTracks();
      SoundDriver_MIDI_Unlock();

      MIDI_Reset();
      }

   SoundDriver_MIDI_Lock();
   while( _MIDI_PositionInTicks < PositionInTicks )
      {
      if ( _MIDI_ProcessNextTick() )
         {
         break;
         }
      if ( _MIDI_ActiveTracks == 0 )
         {
         _MIDI_ResetTracks();
         if ( !_MIDI_Loop )
            {
            SoundDriver_MIDI_Unlock();
            return;
            }
         break;
         }
      }
   SoundDriver_MIDI_Unlock();

   MIDI_SetVolume( _MIDI_TotalVolume );
   MIDI_ContinueSong();
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MIDI_SetSongTime
   (
   unsigned int milliseconds
   )

   {
   unsigned int mil;
   unsigned int sec;
   unsigned int newtime;

   if ( !_MIDI_SongLoaded )
      {
      return;
      }

   MIDI_PauseSong();

   mil = ( ( milliseconds % 1000 ) << TIME_PRECISION ) / 1000;
   sec = ( milliseconds / 1000 ) << TIME_PRECISION;
   newtime = sec + mil;

   if ( newtime < _MIDI_Time )
      {
      SoundDriver_MIDI_Lock();
      _MIDI_ResetTracks();
      SoundDriver_MIDI_Unlock();

      MIDI_Reset();
      }

   SoundDriver_MIDI_Lock();
   while( _MIDI_Time < newtime )
      {
      if ( _MIDI_ProcessNextTick() )
         {
         break;
         }
      if ( _MIDI_ActiveTracks == 0 )
         {
         _MIDI_ResetTracks();
         if ( !_MIDI_Loop )
            {
            SoundDriver_MIDI_Unlock();
            return;
            }
         break;
         }
      }
   SoundDriver_MIDI_Unlock();

   MIDI_SetVolume( _MIDI_TotalVolume );
   MIDI_ContinueSong();
   }


/*---------------------------------------------------------------------
   Function: MIDI_SetSongPosition

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MIDI_SetSongPosition
   (
   int measure,
   int beat,
   int tick
   )

   {
   unsigned int pos;

   if ( !_MIDI_SongLoaded )
      {
      return;
      }

   MIDI_PauseSong();

   pos = RELATIVE_BEAT( measure, beat, tick );

   if ( pos < RELATIVE_BEAT( _MIDI_Measure, _MIDI_Beat, _MIDI_Tick ) )
      {
      SoundDriver_MIDI_Lock();
      _MIDI_ResetTracks();
      SoundDriver_MIDI_Unlock();

      MIDI_Reset();
      }

   SoundDriver_MIDI_Lock();
   while( RELATIVE_BEAT( _MIDI_Measure, _MIDI_Beat, _MIDI_Tick ) < pos )
      {
      if ( _MIDI_ProcessNextTick() )
         {
         break;
         }
      if ( _MIDI_ActiveTracks == 0 )
         {
         _MIDI_ResetTracks();
         if ( !_MIDI_Loop )
            {
            SoundDriver_MIDI_Unlock();
            return;
            }
         break;
         }
      }
   SoundDriver_MIDI_Unlock();

   MIDI_SetVolume( _MIDI_TotalVolume );
   MIDI_ContinueSong();
   }


/*---------------------------------------------------------------------
   Function: MIDI_GetSongPosition

   Returns the position of the song pointer in Measures, beats, ticks.
---------------------------------------------------------------------*/

void MIDI_GetSongPosition
   (
   songposition *pos
   )

   {
   unsigned int mil;
   unsigned int sec;

   mil = ( _MIDI_Time & ( ( 1 << TIME_PRECISION ) - 1 ) ) * 1000;
   sec = _MIDI_Time >> TIME_PRECISION;
   pos->milliseconds = ( mil >> TIME_PRECISION ) + ( sec * 1000 );
   pos->tickposition = _MIDI_PositionInTicks;
   pos->measure      = _MIDI_Measure;
   pos->beat         = _MIDI_Beat;
   pos->tick         = _MIDI_Tick;
   }


/*---------------------------------------------------------------------
   Function: MIDI_GetSongLength

   Returns the length of the song.
---------------------------------------------------------------------*/

void MIDI_GetSongLength
   (
   songposition *pos
   )

   {
   unsigned int mil;
   unsigned int sec;

   mil = ( _MIDI_TotalTime & ( ( 1 << TIME_PRECISION ) - 1 ) ) * 1000;
   sec = _MIDI_TotalTime >> TIME_PRECISION;

   pos->milliseconds = ( mil >> TIME_PRECISION ) + ( sec * 1000 );
   pos->measure      = _MIDI_TotalMeasures;
   pos->beat         = _MIDI_TotalBeats;
   pos->tick         = _MIDI_TotalTicks;
   pos->tickposition = 0;
   }


/*---------------------------------------------------------------------
   Function: MIDI_InitEMIDI

   Sets up the EMIDI
---------------------------------------------------------------------*/

static void _MIDI_InitEMIDI
   (
   void
   )

   {
   int    event;
   int    command;
   int    channel;
   int    length;
   int    IncludeFound;
   track *Track;
   int    tracknum;
   int    c1;
   int    c2;

   _MIDI_ResetTracks();

   _MIDI_TotalTime     = 0;
   _MIDI_TotalTicks    = 0;
   _MIDI_TotalBeats    = 0;
   _MIDI_TotalMeasures = 0;

   Track = _MIDI_TrackPtr;
   tracknum = 0;
   while( ( tracknum < _MIDI_NumTracks ) && ( Track != NULL ) )
      {
      _MIDI_Tick = 0;
      _MIDI_Beat = 1;
      _MIDI_Measure = 1;
      _MIDI_Time = 0;
      _MIDI_BeatsPerMeasure = 4;
      _MIDI_TicksPerBeat = _MIDI_Division;
      _MIDI_TimeBase = 4;

      _MIDI_PositionInTicks = 0;
      _MIDI_ActiveTracks    = 0;
      _MIDI_Context         = -1;

      Track->RunningStatus = 0;
      Track->active        = TRUE;

      Track->EMIDI_ProgramChange = FALSE;
      Track->EMIDI_VolumeChange  = FALSE;
      Track->EMIDI_IncludeTrack  = TRUE;

      memset( Track->context, 0, sizeof( Track->context ) );

      while( Track->delay > 0 )
         {
         _MIDI_AdvanceTick();
         Track->delay--;
         }

      IncludeFound = FALSE;
      while ( Track->active )
         {
         GET_NEXT_EVENT( Track, event );

         if ( GET_MIDI_COMMAND( event ) == MIDI_SPECIAL )
            {
            switch( event )
               {
               case MIDI_SYSEX :
               case MIDI_SYSEX_CONTINUE :
                  _MIDI_SysEx( Track );
                  break;

               case MIDI_META_EVENT :
                  _MIDI_MetaEvent( Track );
                  break;
               }

            if ( Track->active )
               {
               Track->delay = _MIDI_ReadDelta( Track );
               while( Track->delay > 0 )
                  {
                  _MIDI_AdvanceTick();
                  Track->delay--;
                  }
               }

            continue;
            }

         if ( event & MIDI_RUNNING_STATUS )
            {
            Track->RunningStatus = event;
            }
         else
            {
            event = Track->RunningStatus;
            Track->pos--;
            }

         channel = GET_MIDI_CHANNEL( event );
         command = GET_MIDI_COMMAND( event );
         length = _MIDI_CommandLengths[ command ];

         if ( command == MIDI_CONTROL_CHANGE )
            {
            if ( *Track->pos == MIDI_MONO_MODE_ON )
               {
               length++;
               }
            GET_NEXT_EVENT( Track, c1 );
            GET_NEXT_EVENT( Track, c2 );
            length -= 2;

            switch( c1 )
               {
               case EMIDI_LOOP_START :
               case EMIDI_SONG_LOOP_START :
                  if ( c2 == 0 )
                     {
                     Track->context[ 0 ].loopcount = EMIDI_INFINITE;
                     }
                  else
                     {
                     Track->context[ 0 ].loopcount = c2;
                     }

                  Track->context[ 0 ].pos              = Track->pos;
                  Track->context[ 0 ].loopstart        = Track->pos;
                  Track->context[ 0 ].RunningStatus    = Track->RunningStatus;
                  Track->context[ 0 ].time             = _MIDI_Time;
                  Track->context[ 0 ].FPSecondsPerTick = _MIDI_FPSecondsPerTick;
                  Track->context[ 0 ].tick             = _MIDI_Tick;
                  Track->context[ 0 ].beat             = _MIDI_Beat;
                  Track->context[ 0 ].measure          = _MIDI_Measure;
                  Track->context[ 0 ].BeatsPerMeasure  = _MIDI_BeatsPerMeasure;
                  Track->context[ 0 ].TicksPerBeat     = _MIDI_TicksPerBeat;
                  Track->context[ 0 ].TimeBase         = _MIDI_TimeBase;
                  break;

               case EMIDI_LOOP_END :
               case EMIDI_SONG_LOOP_END :
                  if ( c2 == EMIDI_END_LOOP_VALUE )
                     {
                     Track->context[ 0 ].loopstart = NULL;
                     Track->context[ 0 ].loopcount = 0;
                     }
                  break;

               case EMIDI_INCLUDE_TRACK :
                  if ( EMIDI_AffectsCurrentCard( c2, MUSIC_EMIDIPersonality ) )
                     {
                     //printf( "Include track %d on card %d\n", tracknum, c2 );
                     IncludeFound = TRUE;
                     Track->EMIDI_IncludeTrack = TRUE;
                     }
                  else if ( !IncludeFound )
                     {
                     //printf( "Track excluded %d on card %d\n", tracknum, c2 );
                     IncludeFound = TRUE;
                     Track->EMIDI_IncludeTrack = FALSE;
                     }
                  break;

               case EMIDI_EXCLUDE_TRACK :
                  if ( EMIDI_AffectsCurrentCard( c2, MUSIC_EMIDIPersonality ) )
                     {
                     //printf( "Exclude track %d on card %d\n", tracknum, c2 );
                     Track->EMIDI_IncludeTrack = FALSE;
                     }
                  break;

               case EMIDI_PROGRAM_CHANGE :
                  if ( !Track->EMIDI_ProgramChange )
                     //printf( "Program change on track %d\n", tracknum );
                  Track->EMIDI_ProgramChange = TRUE;
                  break;

               case EMIDI_VOLUME_CHANGE :
                  if ( !Track->EMIDI_VolumeChange )
                     //printf( "Volume change on track %d\n", tracknum );
                  Track->EMIDI_VolumeChange = TRUE;
                  break;

               case EMIDI_CONTEXT_START :
                  if ( ( c2 > 0 ) && ( c2 < EMIDI_NUM_CONTEXTS ) )
                     {
                     Track->context[ c2 ].pos              = Track->pos;
                     Track->context[ c2 ].loopstart        = Track->context[ 0 ].loopstart;
                     Track->context[ c2 ].loopcount        = Track->context[ 0 ].loopcount;
                     Track->context[ c2 ].RunningStatus    = Track->RunningStatus;
                     Track->context[ c2 ].time             = _MIDI_Time;
                     Track->context[ c2 ].FPSecondsPerTick = _MIDI_FPSecondsPerTick;
                     Track->context[ c2 ].tick             = _MIDI_Tick;
                     Track->context[ c2 ].beat             = _MIDI_Beat;
                     Track->context[ c2 ].measure          = _MIDI_Measure;
                     Track->context[ c2 ].BeatsPerMeasure  = _MIDI_BeatsPerMeasure;
                     Track->context[ c2 ].TicksPerBeat     = _MIDI_TicksPerBeat;
                     Track->context[ c2 ].TimeBase         = _MIDI_TimeBase;
                     }
                  break;

               case EMIDI_CONTEXT_END :
                  break;
               }
            }

         Track->pos += length;
         Track->delay = _MIDI_ReadDelta( Track );

         while( Track->delay > 0 )
            {
            _MIDI_AdvanceTick();
            Track->delay--;
            }
         }

      _MIDI_TotalTime = max( _MIDI_TotalTime, _MIDI_Time );
      if ( RELATIVE_BEAT( _MIDI_Measure, _MIDI_Beat, _MIDI_Tick ) >
         RELATIVE_BEAT( _MIDI_TotalMeasures, _MIDI_TotalBeats,
         _MIDI_TotalTicks ) )
         {
         _MIDI_TotalTicks    = _MIDI_Tick;
         _MIDI_TotalBeats    = _MIDI_Beat;
         _MIDI_TotalMeasures = _MIDI_Measure;
         }

      Track++;
      tracknum++;
      }

   _MIDI_ResetTracks();
   }


/*---------------------------------------------------------------------
   Function: MIDI_LoadTimbres

   Preloads the timbres on cards that use patch-caching.
---------------------------------------------------------------------*/

void MIDI_LoadTimbres
   (
   void
   )

   {
   int    event;
   int    command;
   int    channel;
   int    length;
   int    Finished;
   track *Track;
   int    tracknum;

   Track = _MIDI_TrackPtr;
   tracknum = 0;
   while( ( tracknum < _MIDI_NumTracks ) && ( Track != NULL ) )
      {
      Finished = FALSE;
      while ( !Finished )
         {
         GET_NEXT_EVENT( Track, event );

         if ( GET_MIDI_COMMAND( event ) == MIDI_SPECIAL )
            {
            switch( event )
               {
               case MIDI_SYSEX :
               case MIDI_SYSEX_CONTINUE :
                  length = _MIDI_ReadDelta( Track );
                  Track->pos += length;
                  break;

               case MIDI_META_EVENT :
                  GET_NEXT_EVENT( Track, command );
                  GET_NEXT_EVENT( Track, length );

                  if ( command == MIDI_END_OF_TRACK )
                     {
                     Finished = TRUE;
                     }

                  Track->pos += length;
                  break;
               }

            if ( !Finished )
               {
               _MIDI_ReadDelta( Track );
               }

            continue;
            }

         if ( event & MIDI_RUNNING_STATUS )
            {
            Track->RunningStatus = event;
            }
         else
            {
            event = Track->RunningStatus;
            Track->pos--;
            }

         channel = GET_MIDI_CHANNEL( event );
         command = GET_MIDI_COMMAND( event );
         length = _MIDI_CommandLengths[ command ];

         if ( command == MIDI_CONTROL_CHANGE )
            {
            if ( *Track->pos == MIDI_MONO_MODE_ON )
               {
               length++;
               }

            if ( *Track->pos == EMIDI_PROGRAM_CHANGE )
               {
               _MIDI_Funcs->LoadPatch( *( Track->pos + 1 ) );
               }
            }

         if ( channel == MIDI_RHYTHM_CHANNEL )
            {
            if ( command == MIDI_NOTE_ON )
               {
               _MIDI_Funcs->LoadPatch( 128 + *Track->pos );
               }
            }
         else
            {
            if ( command == MIDI_PROGRAM_CHANGE )
               {
               _MIDI_Funcs->LoadPatch( *Track->pos );
               }
            }
         Track->pos += length;
         _MIDI_ReadDelta( Track );
         }
      Track++;
      tracknum++;
      }

   _MIDI_ResetTracks();
   }


/*---------------------------------------------------------------------
   Function: _MIDI_SendGSReset

   Sends a GS Reset sysex message.
---------------------------------------------------------------------*/
   
static void _MIDI_SendGSReset(void)
{
    if (_MIDI_Funcs->SysEx)
       {
       _MIDI_Funcs->SysEx(_GSResetSysex, sizeof(_GSResetSysex));
       ASS_Sleep(50);
       }
}

// vim:ts=3:sw=3:expandtab:
