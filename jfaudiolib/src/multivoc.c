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
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "linklist.h"
#include "sndcards.h"
#include "drivers.h"
#include "pitch.h"
#include "multivoc.h"
#include "assmisc.h"
#include "_multivc.h"

#define RoundFixed( fixedval, bits )            \
        (                                       \
          (                                     \
            (fixedval) + ( 1 << ( (bits) - 1 ) )\
          ) >> (bits)                           \
        )

#define IS_QUIET( ptr )  ( ( void * )( ptr ) == ( void * )&MV_VolumeTable[ 0 ] )

static int       MV_ReverbLevel;
static int       MV_ReverbDelay;
static VOLUME16 *MV_ReverbTable = NULL;

//static signed short MV_VolumeTable[ MV_MaxVolume + 1 ][ 256 ];
static signed short MV_VolumeTable[ 63 + 1 ][ 256 ];

//static Pan MV_PanTable[ MV_NumPanPositions ][ MV_MaxVolume + 1 ];
Pan MV_PanTable[ MV_NumPanPositions ][ 63 + 1 ];

int MV_Installed   = FALSE;
static int MV_TotalVolume = MV_MaxTotalVolume;
static int MV_MaxVoices   = 1;
static int MV_Recording;

static int MV_BufferSize = MixBufferSize;
static int MV_BufferLength;

static int MV_NumberOfBuffers = NumberOfBuffers;

static int MV_MixMode    = MONO_8BIT;
static int MV_Channels   = 1;
static int MV_Bits       = 8;

static int MV_Silence    = SILENCE_8BIT;
static int MV_SwapLeftRight = FALSE;

static int MV_RequestedMixRate;
int MV_MixRate;

static int MV_BuffShift;

static int MV_TotalMemory;

static int   MV_BufferEmpty[ NumberOfBuffers ];
char *MV_MixBuffer[ NumberOfBuffers + 1 ];

static VoiceNode *MV_Voices = NULL;

static volatile VoiceNode VoiceList;
static volatile VoiceNode VoicePool;

static int MV_MixPage      = 0;
static int MV_VoiceHandle  = MV_MinVoiceHandle;

static void ( *MV_CallBackFunc )( unsigned int ) = NULL;
static void ( *MV_RecordFunc )( char *ptr, int length ) = NULL;
static void ( *MV_MixFunction )( VoiceNode *voice, int buffer );

int MV_MaxVolume = 63;

char  *MV_HarshClipTable;
char  *MV_MixDestination;
short *MV_LeftVolume;
short *MV_RightVolume;
int    MV_SampleSize = 1;
int    MV_RightChannelOffset;

unsigned int MV_MixPosition;

int MV_ErrorCode = MV_Ok;

static int lockdepth = 0;
static int DisableInterrupts(void)
{
	if (lockdepth++ > 0) {
		return 0;
	}
	SoundDriver_PCM_Lock();
	return 0;
}

static void RestoreInterrupts(int a)
{
	(void)a;

	if (--lockdepth > 0) {
		return;
	}
	SoundDriver_PCM_Unlock();
}


/*---------------------------------------------------------------------
   Function: MV_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *MV_ErrorString
   (
   int ErrorNumber
   )

   {
   const char *ErrorString;

   switch( ErrorNumber )
      {
      case MV_Warning :
      case MV_Error :
         ErrorString = MV_ErrorString( MV_ErrorCode );
         break;

      case MV_Ok :
         ErrorString = "Multivoc ok.";
         break;

      case MV_UnsupportedCard :
         ErrorString = "Selected sound card is not supported by Multivoc.";
         break;

      case MV_NotInstalled :
         ErrorString = "Multivoc not installed.";
         break;
			
      case MV_DriverError :
         ErrorString = SoundDriver_PCM_ErrorString(SoundDriver_PCM_GetError());
         break;

      case MV_NoVoices :
         ErrorString = "No free voices available to Multivoc.";
         break;

      case MV_NoMem :
         ErrorString = "Out of memory in Multivoc.";
         break;

      case MV_VoiceNotFound :
         ErrorString = "No voice with matching handle found.";
         break;

      case MV_InvalidVOCFile :
         ErrorString = "Invalid VOC file passed in to Multivoc.";
         break;

      case MV_InvalidWAVFile :
         ErrorString = "Invalid WAV file passed in to Multivoc.";
         break;
			
      case MV_InvalidVorbisFile :
         ErrorString = "Invalid OggVorbis file passed in to Multivoc.";
         break;
			
      case MV_InvalidMixMode :
         ErrorString = "Invalid mix mode request in Multivoc.";
         break;

      case MV_NullRecordFunction :
         ErrorString = "Null record function passed to MV_StartRecording.";
         break;

      default :
         ErrorString = "Unknown Multivoc error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: MV_Mix

   Mixes the sound into the buffer.
---------------------------------------------------------------------*/

static void MV_Mix
   (
   VoiceNode *voice,
   int        buffer
   )

   {
   char          *start;
   int            length;
   int            voclength;
   unsigned int   position;
   unsigned int   rate;
   unsigned int   FixedPointBufferSize;

   if ( ( voice->length == 0 ) && ( voice->GetSound( voice ) != KeepPlaying ) )
      {
      return;
      }

   length               = MixBufferSize;
   FixedPointBufferSize = voice->FixedPointBufferSize;

   MV_MixDestination    = MV_MixBuffer[ buffer ];
   MV_LeftVolume        = voice->LeftVolume;
   MV_RightVolume       = voice->RightVolume;

   if ( ( MV_Channels == 2 ) && ( IS_QUIET( MV_LeftVolume ) ) )
      {
      MV_LeftVolume      = MV_RightVolume;
      MV_MixDestination += MV_RightChannelOffset;
      }

   // Add this voice to the mix
   while( length > 0 )
      {
      start    = voice->sound;
      rate     = voice->RateScale;
      position = voice->position;

      // Check if the last sample in this buffer would be
      // beyond the length of the sample block
      if ( ( position + FixedPointBufferSize ) >= voice->length )
         {
         if ( position < voice->length )
            {
            voclength = ( voice->length - position + rate - voice->channels ) / rate;
            }
         else
            {
            voice->GetSound( voice );
            return;
            }
         }
      else
         {
         voclength = length;
         }

      if (voice->mix) {
         voice->mix( position, rate, start, voclength );
      }

      voice->position = MV_MixPosition;

      length -= voclength;

      if ( voice->position >= voice->length )
         {
         // Get the next block of sound
         if ( voice->GetSound( voice ) != KeepPlaying )
            {
            return;
            }

         if ( length > (voice->channels - 1) )
            {
            // Get the position of the last sample in the buffer
            FixedPointBufferSize = voice->RateScale * ( length - voice->channels );
            }
         }
      }
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVoice

   Adds a voice to the play list.
---------------------------------------------------------------------*/

void MV_PlayVoice
   (
   VoiceNode *voice
   )

   {
   int flags;

   flags = DisableInterrupts();
   LL_SortedInsertion( &VoiceList, voice, prev, next, VoiceNode, priority );

   RestoreInterrupts( flags );
   }


/*---------------------------------------------------------------------
   Function: MV_StopVoice

   Removes the voice from the play list and adds it to the free list.
---------------------------------------------------------------------*/

static void MV_StopVoice
   (
   VoiceNode *voice
   )

   {
   int flags;

   flags = DisableInterrupts();

   // move the voice from the play list to the free list
   LL_Remove( voice, next, prev );
   LL_Add( (VoiceNode*) &VoicePool, voice, next, prev );

   RestoreInterrupts( flags );
   
   #ifdef HAVE_VORBIS
   if (voice->wavetype == Vorbis)
      {
      MV_ReleaseVorbisVoice(voice);
      }
   #endif
   }


/*---------------------------------------------------------------------
   Function: MV_ServiceVoc

   Starts playback of the waiting buffer and mixes the next one.

   JBF: no synchronisation happens inside MV_ServiceVoc nor the
        supporting functions it calls. This would cause a deadlock
        between the mixer thread in the driver vs the nested
        locking in the user-space functions of MultiVoc. The call
        to MV_ServiceVoc is synchronised in the driver.

        Known functions called by MV_ServiceVoc and its helpers:
           MV_Mix (and its MV_Mix*bit* workers)
           MV_GetNextVOCBlock
           MV_GetNextWAVBlock
           MV_SetVoiceMixMode
---------------------------------------------------------------------*/
static void MV_ServiceVoc
   (
   void
   )

   {
   VoiceNode *voice;
   VoiceNode *next;
	//int        flags;

   // Toggle which buffer we'll mix next
   MV_MixPage++;
   if ( MV_MixPage >= MV_NumberOfBuffers )
      {
      MV_MixPage -= MV_NumberOfBuffers;
      }

   if ( MV_ReverbLevel == 0 )
      {
      // Initialize buffer
      //Commented out so that the buffer is always cleared.
      //This is so the guys at Echo Speech can mix into the
      //buffer even when no sounds are playing.
      //if ( !MV_BufferEmpty[ MV_MixPage ] )
         {
         ClearBuffer_DW( MV_MixBuffer[ MV_MixPage ], MV_Silence, MV_BufferSize >> 2 );
         MV_BufferEmpty[ MV_MixPage ] = TRUE;
         }
      }
   else
      {
      char *end;
      char *source;
      char *dest;
      int   count;
      int   length;

      end = MV_MixBuffer[ 0 ] + MV_BufferLength;;
      dest = MV_MixBuffer[ MV_MixPage ];
      source = MV_MixBuffer[ MV_MixPage ] - MV_ReverbDelay;
      if ( source < MV_MixBuffer[ 0 ] )
         {
         source += MV_BufferLength;
         }

      length = MV_BufferSize;
      while( length > 0 )
         {
         count = length;
         if ( source + count > end )
            {
            count = end - source;
            }

         if ( MV_Bits == 16 )
            {
            if ( MV_ReverbTable != NULL )
               {
               MV_16BitReverb( source, dest, MV_ReverbTable, count / 2 );
               }
            else
               {
               MV_16BitReverbFast( source, dest, count / 2, MV_ReverbLevel );
               }
            }
         else
            {
            if ( MV_ReverbTable != NULL )
               {
               MV_8BitReverb( (signed char *) source, (signed char *) dest, MV_ReverbTable, count );
               }
            else
               {
               MV_8BitReverbFast( (signed char *) source, (signed char *) dest, count, MV_ReverbLevel );
               }
            }

         // if we go through the loop again, it means that we've wrapped around the buffer
         source  = MV_MixBuffer[ 0 ];
         dest   += count;
         length -= count;
         }
      }

   // Play any waiting voices
   //flags = DisableInterrupts();
	
   for( voice = VoiceList.next; voice != &VoiceList; voice = next )
      {
      if ( voice->Paused )
         {
         next = voice->next;
         continue;
         }
      
      MV_BufferEmpty[ MV_MixPage ] = FALSE;

      MV_MixFunction( voice, MV_MixPage );

      next = voice->next;

      // Is this voice done?
      if ( !voice->Playing )
         {
         //JBF: prevent a deadlock caused by MV_StopVoice grabbing the mutex again
			//MV_StopVoice( voice );
         LL_Remove( voice, next, prev );
         LL_Add( (VoiceNode*) &VoicePool, voice, next, prev );

         if ( MV_CallBackFunc )
            {
            MV_CallBackFunc( voice->callbackval );
            }
         }
      }
	
   //RestoreInterrupts(flags);
   }


/*---------------------------------------------------------------------
   Function: MV_GetNextVOCBlock

   Interperate the information of a VOC format sound file.
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextVOCBlock
   (
   VoiceNode *voice
   )

   {
   unsigned char *ptr;
   int            blocktype;
   int            lastblocktype;
   unsigned int   blocklength = 0;
   unsigned int   samplespeed = 0;
   unsigned int   tc = 0;
   int            packtype;
   int            voicemode;
   int            done;
   unsigned       BitsPerSample;
   unsigned       Channels;
   unsigned       Format;

   if ( voice->BlockLength > 0 )
      {
      voice->position    -= voice->length;
      voice->sound       += (voice->length >> 16) * (voice->channels * voice->bits / 8);
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;
      return( KeepPlaying );
      }

   ptr = ( unsigned char * )voice->NextBlock;

   voice->Playing = TRUE;

   voicemode = 0;
   lastblocktype = 0;
   packtype = 0;

   done = FALSE;
   while( !done )
      {
      // Stop playing if we get a NULL pointer
      if ( ptr == NULL )
         {
         voice->Playing = FALSE;
         done = TRUE;
         break;
         }

      blocktype = ( int )*ptr;
      blocklength = LITTLE32( *( unsigned int * )( ptr + 1 ) ) & 0x00ffffff;
      ptr += 4;

      switch( blocktype )
         {
         case 0 :
            // End of data
            if ( ( voice->LoopStart == NULL ) ||
               ( (intptr_t) voice->LoopStart >= ( (intptr_t) ptr - 4 ) ) )
               {
               voice->Playing = FALSE;
               done = TRUE;
               }
            else
               {
               voice->NextBlock    = voice->LoopStart;
               voice->BlockLength  = 0;
               voice->position     = 0;
               return MV_GetNextVOCBlock(voice);
               }
            break;

         case 1 :
            // Sound data block
            voice->bits  = 8;
            voice->channels = voicemode + 1;
            if ( lastblocktype != 8 )
               {
               tc = ( unsigned int )*ptr << 8;
               packtype = *( ptr + 1 );
               }

            ptr += 2;
            blocklength -= 2;

            samplespeed = 256000000L / ( voice->channels * ( 65536 - tc ) );
 
            // Skip packed or stereo data
            if ( ( packtype != 0 ) || ( voicemode != 0 && voicemode != 1 ) )
               {
               ptr += blocklength;
               }
            else
               {
               done = TRUE;
               }
            voicemode = 0;
            break;

         case 2 :
            // Sound continuation block
            samplespeed = voice->SamplingRate;
            done = TRUE;
            break;

         case 3 :
            // Silence
            // Not implimented.
            ptr += blocklength;
            break;

         case 4 :
            // Marker
            // Not implimented.
            ptr += blocklength;
            break;

         case 5 :
            // ASCII string
            // Not implimented.
            ptr += blocklength;
            break;

         case 6 :
            // Repeat begin
            if ( voice->LoopEnd == NULL )
               {
               voice->LoopCount = LITTLE16(*( unsigned short * )ptr);
               voice->LoopStart = (char *)((intptr_t) ptr + blocklength);
               }
            ptr += blocklength;
            break;

         case 7 :
            // Repeat end
            ptr += blocklength;
            if ( lastblocktype == 6 )
               {
               voice->LoopCount = 0;
               }
            else
               {
               if ( ( voice->LoopCount > 0 ) && ( voice->LoopStart != NULL ) )
                  {
                  ptr = (unsigned char *) voice->LoopStart;
                  if ( voice->LoopCount < 0xffff )
                     {
                     voice->LoopCount--;
                     if ( voice->LoopCount == 0 )
                        {
                        voice->LoopStart = NULL;
                        }
                     }
                  }
               }
            break;

         case 8 :
            // Extended block
            voice->bits  = 8;
            voice->channels = 1;
            tc = LITTLE16( *( unsigned short * )ptr );
            packtype = *( ptr + 2 );
            voicemode = *( ptr + 3 );
            ptr += blocklength;
            break;

         case 9 :
            // New sound data block
            samplespeed = LITTLE32( *( unsigned int * )ptr );
            BitsPerSample = ( unsigned )*( ptr + 4 );
            Channels = ( unsigned )*( ptr + 5 );
            Format = ( unsigned )LITTLE16( *( unsigned short * )( ptr + 6 ) );

            if ( ( BitsPerSample == 8 ) && ( Channels == 1 || Channels == 2 ) &&
               ( Format == VOC_8BIT ) )
               {
               ptr         += 12;
               blocklength -= 12;
               voice->bits  = 8;
               voice->channels = Channels;
               done         = TRUE;
               }
            else if ( ( BitsPerSample == 16 ) && ( Channels == 1 || Channels == 2 ) &&
               ( Format == VOC_16BIT ) )
               {
               ptr         += 12;
               blocklength -= 12;
               voice->bits  = 16;
               voice->channels = Channels;
               done         = TRUE;
               }
            else
               {
               ptr += blocklength;
               }
            break;

         default :
            // Unknown data.  Probably not a VOC file.
            voice->Playing = FALSE;
            done = TRUE;
            break;
         }

      lastblocktype = blocktype;
      }

   if ( voice->Playing )
      {
      voice->NextBlock    = (char *)ptr + blocklength;
      voice->sound        = (char *)ptr;

      voice->SamplingRate = samplespeed;
      voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;

      // Multiply by MixBufferSize - 1
      voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
         voice->RateScale;

      if ( voice->LoopEnd != NULL )
         {
         if ( blocklength > (intptr_t)voice->LoopEnd )
            {
            blocklength = (intptr_t)voice->LoopEnd;
            }
         else
            {
            voice->LoopEnd = (char *)(intptr_t)blocklength;
            }

         voice->LoopStart = voice->sound + (intptr_t)voice->LoopStart;
         voice->LoopEnd   = voice->sound + (intptr_t)voice->LoopEnd;
         voice->LoopSize  = voice->LoopEnd - voice->LoopStart;
         }

      if ( voice->bits == 16 )
         {
         blocklength /= 2;
         }
      if ( voice->channels == 2 )
         {
         blocklength /= 2;
         }

      voice->position     = 0;
      voice->length       = min( blocklength, 0x8000 );
      voice->BlockLength  = blocklength - voice->length;
      voice->length     <<= 16;

      MV_SetVoiceMixMode( voice );

      return( KeepPlaying );
      }

   return( NoMoreData );
   }


/*---------------------------------------------------------------------
   Function: MV_GetNextDemandFeedBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextDemandFeedBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength > 0 )
      {
      voice->position    -= voice->length;
      voice->sound       += voice->length >> 16;
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;

      return( KeepPlaying );
      }

   if ( voice->DemandFeed == NULL )
      {
      return( NoMoreData );
      }

   voice->position     = 0;
   ( voice->DemandFeed )( &voice->sound, &voice->BlockLength );
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   if ( ( voice->length > 0 ) && ( voice->sound != NULL ) )
      {
      return( KeepPlaying );
      }
   return( NoMoreData );
   }


/*---------------------------------------------------------------------
   Function: MV_GetNextRawBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextRawBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength <= 0 )
      {
      if ( voice->LoopStart == NULL )
         {
         voice->Playing = FALSE;
         return( NoMoreData );
         }

      voice->BlockLength = voice->LoopSize;
      voice->NextBlock   = voice->LoopStart;
      voice->length = 0;
      voice->position = 0;
      }

   voice->sound        = voice->NextBlock;
   voice->position    -= voice->length;
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->NextBlock   += voice->length * (voice->channels * voice->bits / 8);
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   return( KeepPlaying );
   }


/*---------------------------------------------------------------------
   Function: MV_GetNextWAVBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextWAVBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength <= 0 )
      {
      if ( voice->LoopStart == NULL )
         {
         voice->Playing = FALSE;
         return( NoMoreData );
         }

      voice->BlockLength = voice->LoopSize;
      voice->NextBlock   = voice->LoopStart;
      voice->length      = 0;
      voice->position    = 0;
      }

   voice->sound        = voice->NextBlock;
   voice->position    -= voice->length;
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->NextBlock   += voice->length * (voice->channels * voice->bits / 8);
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   return( KeepPlaying );
   }


/*---------------------------------------------------------------------
   Function: MV_ServiceRecord

   Starts recording of the waiting buffer.
---------------------------------------------------------------------*/

/*static void MV_ServiceRecord
   (
   void
   )

   {
   if ( MV_RecordFunc )
      {
      MV_RecordFunc( MV_MixBuffer[ 0 ] + MV_MixPage * MixBufferSize,
         MixBufferSize );
      }

   // Toggle which buffer we'll mix next
   MV_MixPage++;
   if ( MV_MixPage >= NumberOfBuffers )
      {
      MV_MixPage = 0;
      }
   }*/


/*---------------------------------------------------------------------
   Function: MV_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

static VoiceNode *MV_GetVoice
   (
   int handle
   )

   {
   VoiceNode *voice;
   int        flags;

   flags = DisableInterrupts();

   for( voice = VoiceList.next; voice != &VoiceList; voice = voice->next )
      {
      if ( handle == voice->handle )
         {
         break;
         }
      }

   RestoreInterrupts( flags );

   if ( voice == &VoiceList )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      voice = 0;
      }

   return( voice );
   }


/*---------------------------------------------------------------------
   Function: MV_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int MV_VoicePlaying
   (
   int handle
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( FALSE );
      }

   voice = MV_GetVoice( handle );

   if ( voice == NULL )
      {
      return( FALSE );
      }

   return( TRUE );
   }


/*---------------------------------------------------------------------
Function: MV_VoicePaused

Checks if the voice associated with the specified handle is
paused.
---------------------------------------------------------------------*/

int MV_VoicePaused
   (
   int handle
   )

   {
   VoiceNode *voice;
   
   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( FALSE );
      }
   
   voice = MV_GetVoice( handle );
   
   if ( voice == NULL )
      {
      return( FALSE );
      }
   
   return voice->Paused;
   }


/*---------------------------------------------------------------------
   Function: MV_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int MV_KillAllVoices
   (
   void
   )

   {
   VoiceNode * voice, * next;
   int        flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   flags = DisableInterrupts();
       
   // Remove all the voices from the list
   for( voice = VoiceList.next; voice != &VoiceList; voice = next )
      {
      next = voice->next;
      if (voice->priority < MV_MUSIC_PRIORITY)
         {
         MV_Kill( voice->handle );
         }
      }

   RestoreInterrupts(flags);

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_Kill
   (
   int handle
   )

   {
   VoiceNode *voice;
   int        flags;
   unsigned int callbackval;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   flags = DisableInterrupts();

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      RestoreInterrupts( flags );
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   callbackval = voice->callbackval;

   MV_StopVoice( voice );

   RestoreInterrupts( flags );

   if ( MV_CallBackFunc )
      {
      MV_CallBackFunc( callbackval );
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_PauseVoice

   Pauses or resumes output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_PauseVoice
(
 int handle,
 int pauseon
 )

{
   VoiceNode *voice;
   int        flags;
   
   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
   }
   
   flags = DisableInterrupts();
   
   voice = MV_GetVoice( handle );
   if ( voice == NULL )
   {
      RestoreInterrupts( flags );
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
   }
   
   voice->Paused = pauseon;
   
   RestoreInterrupts( flags );
   
   return( MV_Ok );
}


/*---------------------------------------------------------------------
   Function: MV_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/

int MV_VoicesPlaying
   (
   void
   )

   {
   VoiceNode   *voice;
   int         NumVoices = 0;
   int         flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( 0 );
      }

   flags = DisableInterrupts();

   for( voice = VoiceList.next; voice != &VoiceList; voice = voice->next )
      {
      NumVoices++;
      }

   RestoreInterrupts( flags );

   return( NumVoices );
   }


/*---------------------------------------------------------------------
   Function: MV_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

VoiceNode *MV_AllocVoice
   (
   int priority
   )

   {
   VoiceNode   *voice;
   VoiceNode   *node;
   int          flags;

//return( NULL );
   if ( MV_Recording )
      {
      return( NULL );
      }

   flags = DisableInterrupts();

   // Check if we have any free voices
   if ( LL_Empty( &VoicePool, next, prev ) )
      {
      // check if we have a higher priority than a voice that is playing.
      voice = VoiceList.next;
      for( node = voice->next; node != &VoiceList; node = node->next )
         {
         if ( node->priority < voice->priority )
            {
            voice = node;
            }
         }

      if ( priority >= voice->priority )
         {
         MV_Kill( voice->handle );
         }
      }

   // Check if any voices are in the voice pool
   if ( LL_Empty( &VoicePool, next, prev ) )
      {
      // No free voices
      RestoreInterrupts( flags );
      return( NULL );
      }

   voice = VoicePool.next;
   LL_Remove( voice, next, prev );
   RestoreInterrupts( flags );

   // Find a free voice handle
   do
      {
      MV_VoiceHandle++;
      if ( MV_VoiceHandle < MV_MinVoiceHandle )
         {
         MV_VoiceHandle = MV_MinVoiceHandle;
         }
      }
   while( MV_VoicePlaying( MV_VoiceHandle ) );

   voice->handle = MV_VoiceHandle;

   return( voice );
   }


/*---------------------------------------------------------------------
   Function: MV_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int MV_VoiceAvailable
   (
   int priority
   )

   {
   VoiceNode   *voice;
   VoiceNode   *node;
   int          flags;

   // Check if we have any free voices
   if ( !LL_Empty( &VoicePool, next, prev ) )
      {
      return( TRUE );
      }

   flags = DisableInterrupts();

   // check if we have a higher priority than a voice that is playing.
   voice = VoiceList.next;
   for( node = VoiceList.next; node != &VoiceList; node = node->next )
      {
      if ( node->priority < voice->priority )
         {
         voice = node;
         }
      }

   RestoreInterrupts( flags );

   if ( ( voice != &VoiceList ) && ( priority >= voice->priority ) )
      {
      return( TRUE );
      }

   return( FALSE );
   }


/*---------------------------------------------------------------------
   Function: MV_SetVoicePitch

   Sets the pitch for the specified voice.
---------------------------------------------------------------------*/

static void MV_SetVoicePitch
   (
   VoiceNode *voice,
   unsigned int rate,
   int pitchoffset
   )

   {
   voice->SamplingRate = rate;
   voice->PitchScale   = PITCH_GetScale( pitchoffset );
   voice->RateScale    = ( rate * voice->PitchScale ) / MV_MixRate;

   // Multiply by MixBufferSize - 1
   voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
      voice->RateScale;
   }


/*---------------------------------------------------------------------
   Function: MV_SetPitch

   Sets the pitch for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_SetPitch
   (
   int handle,
   int pitchoffset
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   MV_SetVoicePitch( voice, voice->SamplingRate, pitchoffset );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_SetFrequency

   Sets the frequency for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_SetFrequency
   (
   int handle,
   int frequency
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   MV_SetVoicePitch( voice, frequency, 0 );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_GetFrequency

   Gets the frequency for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_GetFrequency
   (
   int handle,
   int *frequency
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

    if ( voice->SamplingRate == 0 )
      {
      voice->GetSound( voice );
      }

   *frequency = voice->SamplingRate;

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_GetVolumeTable

   Returns a pointer to the volume table associated with the specified
   volume.
---------------------------------------------------------------------*/

static short *MV_GetVolumeTable
   (
   int vol
   )

   {
   int volume;
   short *table;

   volume = MIX_VOLUME( vol );

   table = (short *) &MV_VolumeTable[ volume ];

   return( table );
   }


/*---------------------------------------------------------------------
   Function: MV_SetVoiceMixMode

   Selects which method should be used to mix the voice.

 8Bit  16Bit  8Bit  16Bit |  8Bit  16Bit  8Bit  16Bit |
 Mono  Mono   Ster  Ster  |  Mono  Mono   Ster  Ster  |  Mixer
 Out   Out    Out   Out   |  In    In     In    In    |
--------------------------+---------------------------+-------------
  X                       |         X                 | Mix8BitMono16
  X                       |   X                       | Mix8BitMono
               X          |         X                 | Mix8BitStereo16
               X          |   X                       | Mix8BitStereo
        X                 |         X                 | Mix16BitMono16
        X                 |   X                       | Mix16BitMono
                     X    |         X                 | Mix16BitStereo16
                     X    |   X                       | Mix16BitStereo
--------------------------+---------------------------+-------------
                     X    |                      X    | Mix16BitStereo16Stereo
                     X    |                X          | Mix16BitStereo8Stereo
					X          |                      X    | Mix8BitStereo16Stereo
               X          |                X          | Mix8BitStereo8Stereo
		  X                 |                      X    | Mix16BitMono16Stereo
        X                 |                X          | Mix16BitMono8Stereo
  X                       |                      X    | Mix8BitMono16Stereo
  X                       |                X          | Mix8BitMono8Stereo

---------------------------------------------------------------------*/

void MV_SetVoiceMixMode
   (
   VoiceNode *voice
   )

   {
   //int flags;
   int test;

   //flags = DisableInterrupts();

   test = T_DEFAULT;
   if ( MV_Bits == 8 )
      {
      test |= T_8BITS;
      }

   if ( MV_Channels == 1 )
      {
      test |= T_MONO;
      }
   else
      {
      if ( IS_QUIET( voice->RightVolume ) )
         {
         test |= T_RIGHTQUIET;
         }
      else if ( IS_QUIET( voice->LeftVolume ) )
         {
         test |= T_LEFTQUIET;
         }
      }
	
   if ( voice->bits == 16 )
      {
      test |= T_16BITSOURCE;
      }
	
	if ( voice->channels == 2 )
      {
      test |= T_STEREOSOURCE;
		test &= ~(T_RIGHTQUIET | T_LEFTQUIET);
      }

   switch( test )
      {
      case T_8BITS | T_MONO | T_16BITSOURCE :
         voice->mix = MV_Mix8BitMono16;
         break;

      case T_8BITS | T_MONO :
         voice->mix = MV_Mix8BitMono;
         break;

      case T_8BITS | T_16BITSOURCE | T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_Mix8BitMono16;
         break;

      case T_8BITS | T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_Mix8BitMono;
         break;

      case T_8BITS | T_16BITSOURCE | T_RIGHTQUIET :
         voice->mix = MV_Mix8BitMono16;
         break;

      case T_8BITS | T_RIGHTQUIET :
         voice->mix = MV_Mix8BitMono;
         break;

      case T_8BITS | T_16BITSOURCE :
         voice->mix = MV_Mix8BitStereo16;
         break;

      case T_8BITS :
         voice->mix = MV_Mix8BitStereo;
         break;

      case T_MONO | T_16BITSOURCE :
         voice->mix = MV_Mix16BitMono16;
         break;

      case T_MONO :
         voice->mix = MV_Mix16BitMono;
         break;

      case T_16BITSOURCE | T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_Mix16BitMono16;
         break;

      case T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_Mix16BitMono;
         break;

      case T_16BITSOURCE | T_RIGHTQUIET :
         voice->mix = MV_Mix16BitMono16;
         break;

      case T_RIGHTQUIET :
         voice->mix = MV_Mix16BitMono;
         break;

      case T_16BITSOURCE :
         voice->mix = MV_Mix16BitStereo16;
         break;

      case T_SIXTEENBIT_STEREO :
         voice->mix = MV_Mix16BitStereo;
         break;
			
		case T_16BITSOURCE | T_STEREOSOURCE:
			voice->mix = MV_Mix16BitStereo16Stereo;
			break;

		case T_16BITSOURCE | T_STEREOSOURCE | T_8BITS:
			voice->mix = MV_Mix8BitStereo16Stereo;
			break;
			
		case T_16BITSOURCE | T_STEREOSOURCE | T_MONO:
			voice->mix = MV_Mix16BitMono16Stereo;
			break;
			
		case T_16BITSOURCE | T_STEREOSOURCE | T_8BITS | T_MONO:
			voice->mix = MV_Mix8BitMono16Stereo;
			break;
			
		case T_STEREOSOURCE:
			voice->mix = MV_Mix16BitStereo8Stereo;
			break;
         
		case T_STEREOSOURCE | T_8BITS:
			voice->mix = MV_Mix8BitStereo8Stereo;
			break;
			
		case T_STEREOSOURCE | T_MONO:
			voice->mix = MV_Mix16BitMono8Stereo;
			break;
			
		case T_STEREOSOURCE | T_8BITS | T_MONO:
			voice->mix = MV_Mix8BitMono8Stereo;
			break;
			
      default :
         voice->mix = 0;
      }

   //RestoreInterrupts( flags );
   }


/*---------------------------------------------------------------------
   Function: MV_SetVoiceVolume

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

void MV_SetVoiceVolume
   (
   VoiceNode *voice,
   int vol,
   int left,
   int right
   )

   {
   if ( MV_Channels == 1 )
      {
      left  = vol;
      right = vol;
      }

   if ( MV_SwapLeftRight )
      {
      // SBPro uses reversed panning
      voice->LeftVolume  = MV_GetVolumeTable( right );
      voice->RightVolume = MV_GetVolumeTable( left );
      }
   else
      {
      voice->LeftVolume  = MV_GetVolumeTable( left );
      voice->RightVolume = MV_GetVolumeTable( right );
      }

   MV_SetVoiceMixMode( voice );
   }


/*---------------------------------------------------------------------
   Function: MV_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int MV_EndLooping
   (
   int handle
   )

   {
   VoiceNode *voice;
   int        flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   flags = DisableInterrupts();

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      RestoreInterrupts( flags );
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Warning );
      }

   voice->LoopCount = 0;
   voice->LoopStart = NULL;
   voice->LoopEnd   = NULL;

   RestoreInterrupts( flags );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_SetPan
   (
   int handle,
   int vol,
   int left,
   int right
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Warning );
      }

   MV_SetVoiceVolume( voice, vol, left, right );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_Pan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_SetPan( handle, mid, left, right );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_SetReverb

   Sets the level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverb
   (
   int reverb
   )

   {
   MV_ReverbLevel = MIX_VOLUME( reverb );
   MV_ReverbTable = &MV_VolumeTable[ MV_ReverbLevel ];
   }


/*---------------------------------------------------------------------
   Function: MV_SetFastReverb

   Sets the level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetFastReverb
   (
   int reverb
   )

   {
   MV_ReverbLevel = max( 0, min( 16, reverb ) );
   MV_ReverbTable = NULL;
   }


/*---------------------------------------------------------------------
   Function: MV_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int MV_GetMaxReverbDelay
   (
   void
   )

   {
   int maxdelay;

   maxdelay = MixBufferSize * MV_NumberOfBuffers;

   return maxdelay;
   }


/*---------------------------------------------------------------------
   Function: MV_GetReverbDelay

   Returns the current delay time for reverb.
---------------------------------------------------------------------*/

int MV_GetReverbDelay
   (
   void
   )

   {
   return MV_ReverbDelay / MV_SampleSize;
   }


/*---------------------------------------------------------------------
   Function: MV_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverbDelay
   (
   int delay
   )

   {
   int maxdelay;

   maxdelay = MV_GetMaxReverbDelay();
   MV_ReverbDelay = max( MixBufferSize, min( delay, maxdelay ) );
   MV_ReverbDelay *= MV_SampleSize;
   }


/*---------------------------------------------------------------------
   Function: MV_SetMixMode

   Prepares Multivoc to play stereo of mono digitized sounds.
---------------------------------------------------------------------*/

int MV_SetMixMode
   (
   int numchannels,
   int samplebits
   )

   {
   int mode;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   mode = 0;
   if ( numchannels == 2 )
      {
      mode |= STEREO;
      }
   if ( samplebits == 16 )
      {
      mode |= SIXTEEN_BIT;
      }

   MV_MixMode = mode;

   MV_Channels = 1;
   if ( MV_MixMode & STEREO )
      {
      MV_Channels = 2;
      }

   MV_Bits = 8;
   if ( MV_MixMode & SIXTEEN_BIT )
      {
      MV_Bits = 16;
      }

   MV_BuffShift  = 7 + MV_Channels;
   MV_SampleSize = sizeof( MONO8 ) * MV_Channels;

   if ( MV_Bits == 8 )
      {
      MV_Silence = SILENCE_8BIT;
      }
   else
      {
      MV_Silence     = SILENCE_16BIT;
      MV_BuffShift  += 1;
      MV_SampleSize *= 2;
      }

   MV_BufferSize = MixBufferSize * MV_SampleSize;
   MV_NumberOfBuffers = TotalBufferSize / MV_BufferSize;
   MV_BufferLength = TotalBufferSize;

   MV_RightChannelOffset = MV_SampleSize / 2;

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_StartPlayback

   Starts the sound playback engine.
---------------------------------------------------------------------*/

int MV_StartPlayback
   (
   void
   )

   {
   int status;
   int buffer;

   // Initialize the buffers
   ClearBuffer_DW( MV_MixBuffer[ 0 ], MV_Silence, TotalBufferSize >> 2 );
   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_BufferEmpty[ buffer ] = TRUE;
      }

   // Set the mix buffer variables
   MV_MixPage = 1;

   MV_MixFunction = MV_Mix;

//JIM
//   MV_MixRate = MV_RequestedMixRate;
//   return( MV_Ok );

   // Start playback
   status = SoundDriver_PCM_BeginPlayback(MV_MixBuffer[0], MV_BufferSize,
												  MV_NumberOfBuffers, MV_ServiceVoc);
   if (status != MV_Ok) {
      MV_SetErrorCode(MV_DriverError);
      return MV_Error;
   }
	
   MV_MixRate = MV_RequestedMixRate;

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_StopPlayback

   Stops the sound playback engine.
---------------------------------------------------------------------*/

void MV_StopPlayback
   (
   void
   )

   {
   VoiceNode   *voice;
   VoiceNode   *next;
   int          flags;

   // Stop sound playback
   SoundDriver_PCM_StopPlayback();

   // Make sure all callbacks are done.
   flags = DisableInterrupts();

   for( voice = VoiceList.next; voice != &VoiceList; voice = next )
      {
      next = voice->next;

      MV_StopVoice( voice );

      if ( MV_CallBackFunc )
         {
         MV_CallBackFunc( voice->callbackval );
         }
      }

   RestoreInterrupts( flags );
   }


/*---------------------------------------------------------------------
   Function: MV_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int MV_StartRecording
   (
   int MixRate,
   void ( *function )( char *ptr, int length )
   )

   {
	(void)MixRate; (void)function;

	MV_SetErrorCode( MV_UnsupportedCard );
	return( MV_Error );
   }


/*---------------------------------------------------------------------
   Function: MV_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void MV_StopRecord
   (
   void
   )

   {
   }


/*---------------------------------------------------------------------
   Function: MV_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int MV_StartDemandFeedPlayback
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
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->wavetype    = DemandFeed;
   voice->bits        = 8;
	voice->channels    = 1;
   voice->GetSound    = MV_GetNextDemandFeedBlock;
   voice->NextBlock   = NULL;
   voice->DemandFeed  = function;
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->position    = 0;
   voice->sound       = NULL;
   voice->length      = 0;
   voice->BlockLength = 0;
   voice->Playing     = TRUE;
   voice->Paused      = FALSE;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;

   MV_SetVoicePitch( voice, rate, pitchoffset );
   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );

   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayRaw

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayRaw
   (
   char *ptr,
   unsigned int length,
   unsigned rate,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   int status;

   status = MV_PlayLoopedRaw( ptr, length, NULL, NULL, rate, pitchoffset,
      vol, left, right, priority, callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedRaw

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedRaw
   (
   char *ptr,
   unsigned int length,
   char *loopstart,
   char *loopend,
   unsigned rate,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->wavetype    = Raw;
   voice->bits        = 8;
	voice->channels    = 1;
   voice->GetSound    = MV_GetNextRawBlock;
   voice->Playing     = TRUE;
   voice->Paused      = FALSE;
   voice->NextBlock   = ptr;
   voice->position    = 0;
   voice->BlockLength = length;
   voice->length      = 0;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;
   voice->LoopStart   = loopstart;
   voice->LoopEnd     = loopend;
   voice->LoopSize    = ( voice->LoopEnd - voice->LoopStart ) + 1;

   MV_SetVoicePitch( voice, rate, pitchoffset );
   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );

   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayWAV
   (
   char *ptr,
   unsigned int length,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   int status;

   status = MV_PlayLoopedWAV( ptr, length, -1, -1, pitchoffset, vol, left, right,
      priority, callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayWAV3D
   (
   char *ptr,
   unsigned int length,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned int callbackval
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayWAV( ptr, length, pitchoffset, mid, left, right, priority,
      callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayRaw3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayRaw3D
   (
   char *ptr,
   unsigned int length,
   unsigned rate,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned int callbackval
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayRaw( ptr, length, rate, pitchoffset, mid, left, right,
      priority, callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedWAV
   (
   char *ptr,
   unsigned int ptrlength,
   int   loopstart,
   int   loopend,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   riff_header   riff;
   format_header format;
   data_header   data;
   VoiceNode     *voice;
   int length;

   (void)ptrlength; (void)loopend;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

	memcpy(&riff, ptr, sizeof(riff_header));
	riff.file_size   = LITTLE32(riff.file_size);
	riff.format_size = LITTLE32(riff.format_size);

   if ( ( memcmp( riff.RIFF, "RIFF", 4 ) != 0 ) ||
      ( memcmp( riff.WAVE, "WAVE", 4 ) != 0 ) ||
      ( memcmp( riff.fmt, "fmt ", 4) != 0 ) )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

	memcpy(&format, ptr + sizeof(riff_header), sizeof(format_header));
	format.wFormatTag      = LITTLE16(format.wFormatTag);
	format.nChannels       = LITTLE16(format.nChannels);
	format.nSamplesPerSec  = LITTLE32(format.nSamplesPerSec);
	format.nAvgBytesPerSec = LITTLE32(format.nAvgBytesPerSec);
	format.nBlockAlign     = LITTLE16(format.nBlockAlign);
	format.nBitsPerSample  = LITTLE16(format.nBitsPerSample);
	
	memcpy(&data, ptr + sizeof(riff_header) + riff.format_size, sizeof(data_header));
	data.size = LITTLE32(data.size);

   // Check if it's PCM data.
   if ( format.wFormatTag != 1 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   if ( format.nChannels != 1 && format.nChannels != 2 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   if ( ( format.nBitsPerSample != 8 ) &&
      ( format.nBitsPerSample != 16 ) )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   if ( memcmp( data.DATA, "data", 4 ) != 0 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->wavetype    = WAV;
   voice->bits        = format.nBitsPerSample;
	voice->channels    = format.nChannels;
   voice->GetSound    = MV_GetNextWAVBlock;

   length = data.size;
   if ( voice->bits == 16 )
      {
      data.size  &= ~1;
      length     /= 2;
      }
   if ( voice->channels == 2 )
      {
      data.size &= ~1;
      length    /= 2;
      }

   voice->Playing     = TRUE;
   voice->Paused      = FALSE;
   voice->DemandFeed  = NULL;
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->position    = 0;
   voice->length      = 0;
   voice->BlockLength = length;
   voice->NextBlock   = ( char * )( (intptr_t) ptr + sizeof(riff_header) + riff.format_size + sizeof(data_header) );
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;
   voice->LoopStart   = loopstart >= 0 ? voice->NextBlock : NULL;
   voice->LoopEnd     = NULL;
   voice->LoopSize    = length;

   MV_SetVoicePitch( voice, format.nSamplesPerSec, pitchoffset );
   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );

   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayVOC3D
   (
   char *ptr,
   unsigned int ptrlength,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned int callbackval
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayVOC( ptr, ptrlength, pitchoffset, mid, left, right, priority,
      callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayVOC
   (
   char *ptr,
   unsigned int ptrlength,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   int status;

   status = MV_PlayLoopedVOC( ptr, ptrlength, -1, -1, pitchoffset, vol, left, right,
      priority, callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedVOC
   (
   char *ptr,
   unsigned int ptrlength,
   int   loopstart,
   int   loopend,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned int callbackval
   )

   {
   VoiceNode   *voice;
   int          status;

   (void)ptrlength;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Make sure it's a valid VOC file.
   status = memcmp( ptr, "Creative Voice File", 19 );
   if ( status != 0 )
      {
      MV_SetErrorCode( MV_InvalidVOCFile );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->wavetype    = VOC;
   voice->bits        = 8;
   voice->channels    = 1;
   voice->GetSound    = MV_GetNextVOCBlock;
   voice->NextBlock   = ptr + LITTLE16(*( unsigned short * )( ptr + 0x14 ));
   voice->DemandFeed  = NULL;
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->length      = 0;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;
   voice->LoopStart   = loopstart >= 0 ? voice->NextBlock : 0;
   voice->LoopEnd     = 0;
   voice->LoopSize    = loopend - loopstart + 1;

   if ( loopstart < 0 )
      {
      voice->LoopStart = NULL;
      voice->LoopEnd   = NULL;
      }

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );

   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_CreateVolumeTable

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CreateVolumeTable
   (
   int index,
   int volume,
   int MaxVolume
   )

   {
   int val;
   int level;
   int i;

   level = ( volume * MaxVolume ) / MV_MaxTotalVolume;
   if ( MV_Bits == 16 )
      {
      for( i = 0; i < 65536; i += 256 )
         {
         val   = i - 0x8000;
         val  *= level;
         val  /= MV_MaxVolume;
         MV_VolumeTable[ index ][ i / 256 ] = val;
         }
      }
   else
      {
      for( i = 0; i < 256; i++ )
         {
         val   = i - 0x80;
         val  *= level;
         val  /= MV_MaxVolume;
         MV_VolumeTable[ volume ][ i ] = val;
         }
      }
   }


/*---------------------------------------------------------------------
   Function: MV_CalcVolume

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

static void MV_CalcVolume
   (
   int MaxVolume
   )

   {
   int volume;

   for( volume = 0; volume < 128; volume++ )
      {
      MV_HarshClipTable[ volume ] = 0;
      MV_HarshClipTable[ volume + 384 ] = 255;
      }
   for( volume = 0; volume < 256; volume++ )
      {
      MV_HarshClipTable[ volume + 128 ] = volume;
      }

   // For each volume level, create a translation table with the
   // appropriate volume calculated.
   for( volume = 0; volume <= MV_MaxVolume; volume++ )
      {
      MV_CreateVolumeTable( volume, volume, MaxVolume );
      }
   }


/*---------------------------------------------------------------------
   Function: MV_CalcPanTable

   Create the table used to determine the stereo volume level of
   a sound located at a specific angle and distance from the listener.
---------------------------------------------------------------------*/

static void MV_CalcPanTable
   (
   void
   )

   {
   int   level;
   int   angle;
   int   distance;
   int   HalfAngle;
   int   ramp;

   HalfAngle = ( MV_NumPanPositions / 2 );

   for( distance = 0; distance <= MV_MaxVolume; distance++ )
      {
      level = ( 255 * ( MV_MaxVolume - distance ) ) / MV_MaxVolume;
      for( angle = 0; angle <= HalfAngle / 2; angle++ )
         {
         ramp = level - ( ( level * angle ) /
            ( MV_NumPanPositions / 4 ) );

         MV_PanTable[ angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle - angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle + angle ][ distance ].left = level;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].left = level;

         MV_PanTable[ angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle - angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle + angle ][ distance ].right = ramp;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].right = ramp;
         }
      }
   }


/*---------------------------------------------------------------------
   Function: MV_SetVolume

   Sets the volume of digitized sound playback.
---------------------------------------------------------------------*/

void MV_SetVolume
   (
   int volume
   )

   {
   volume = max( 0, volume );
   volume = min( volume, MV_MaxTotalVolume );

   MV_TotalVolume = volume;

   // Calculate volume table
   MV_CalcVolume( volume );
   }


/*---------------------------------------------------------------------
   Function: MV_GetVolume

   Returns the volume of digitized sound playback.
---------------------------------------------------------------------*/

int MV_GetVolume
   (
   void
   )

   {
   return( MV_TotalVolume );
   }


/*---------------------------------------------------------------------
   Function: MV_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/

void MV_SetCallBack
   (
   void ( *function )( unsigned int )
   )

   {
   MV_CallBackFunc = function;
   }


/*---------------------------------------------------------------------
   Function: MV_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void MV_SetReverseStereo
   (
   int setting
   )

   {
   MV_SwapLeftRight = setting;
   }


/*---------------------------------------------------------------------
   Function: MV_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int MV_GetReverseStereo
   (
   void
   )

   {
   return( MV_SwapLeftRight );
   }


/*---------------------------------------------------------------------
   Function: MV_Init

   Perform the initialization of variables and memory used by
   Multivoc.
---------------------------------------------------------------------*/

int MV_Init
   (
   int soundcard,
   int * MixRate,
   int Voices,
   int * numchannels,
   int * samplebits,
   void * initdata
   )

   {
   char *ptr;
   int  status;
   int  buffer;
   int  index;

   if ( MV_Installed )
      {
      MV_Shutdown();
      }

   MV_SetErrorCode( MV_Ok );

   MV_TotalMemory = Voices * sizeof( VoiceNode ) + sizeof( HARSH_CLIP_TABLE_8 ) + TotalBufferSize;
	ptr = (char *) malloc( MV_TotalMemory );
   if ( !ptr )
      {
      MV_SetErrorCode( MV_NoMem );
      return( MV_Error );
      }
   
   memset(ptr, 0, MV_TotalMemory);

   MV_Voices = ( VoiceNode * )ptr;
	ptr += Voices * sizeof( VoiceNode );
	
   MV_HarshClipTable = ptr;
	ptr += sizeof(HARSH_CLIP_TABLE_8);
	
   // Set number of voices before calculating volume table
   MV_MaxVoices = Voices;

   LL_Reset( (VoiceNode*) &VoiceList, next, prev );
   LL_Reset( (VoiceNode*) &VoicePool, next, prev );

   for( index = 0; index < Voices; index++ )
      {
      LL_Add( (VoiceNode*) &VoicePool, &MV_Voices[ index ], next, prev );
      }

   MV_SetReverseStereo( FALSE );
	
	ASS_PCMSoundDriver = soundcard;

   // Initialize the sound card
	status = SoundDriver_PCM_Init(MixRate, numchannels, samplebits, initdata);
	if ( status != MV_Ok ) {
		MV_SetErrorCode( MV_DriverError );
	}

   if ( MV_ErrorCode != MV_Ok )
      {
      status = MV_ErrorCode;

      free( MV_Voices );
      MV_Voices      = NULL;
      MV_HarshClipTable = NULL;
      MV_TotalMemory = 0;

      MV_SetErrorCode( status );
      return( MV_Error );
      }

   MV_Installed    = TRUE;
   MV_CallBackFunc = NULL;
   MV_RecordFunc   = NULL;
   MV_Recording    = FALSE;
   MV_ReverbLevel  = 0;
   MV_ReverbTable  = NULL;

   // Set the sampling rate
   MV_RequestedMixRate = *MixRate;

   // Set Mixer to play stereo digitized sound
   MV_SetMixMode( *numchannels, *samplebits );
   MV_ReverbDelay = MV_BufferSize * 3;

   // Make sure we don't cross a physical page
   MV_MixBuffer[ MV_NumberOfBuffers ] = ptr;
   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_MixBuffer[ buffer ] = ptr;
      ptr += MV_BufferSize;
      }

   // Calculate pan table
   MV_CalcPanTable();

   MV_SetVolume( MV_MaxTotalVolume );

   // Start the playback engine
   status = MV_StartPlayback();
   if ( status != MV_Ok )
      {
      // Preserve error code while we shutdown.
      status = MV_ErrorCode;
      MV_Shutdown();
      MV_SetErrorCode( status );
      return( MV_Error );
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Shutdown

   Restore any resources allocated by Multivoc back to the system.
---------------------------------------------------------------------*/

int MV_Shutdown
   (
   void
   )

   {
   int      buffer;

   if ( !MV_Installed )
      {
      return( MV_Ok );
      }

   MV_KillAllVoices();

   MV_Installed = FALSE;

   // Stop the sound recording engine
   if ( MV_Recording )
      {
      MV_StopRecord();
      }

   // Stop the sound playback engine
   MV_StopPlayback();

   // Shutdown the sound card
	SoundDriver_PCM_Shutdown();

   // Free any voices we allocated
   free( MV_Voices );
   MV_Voices      = NULL;
   MV_TotalMemory = 0;

   LL_Reset( (VoiceNode*) &VoiceList, next, prev );
   LL_Reset( (VoiceNode*) &VoicePool, next, prev );

   MV_MaxVoices = 1;

   // Release the descriptor from our mix buffer
   for( buffer = 0; buffer < NumberOfBuffers; buffer++ )
      {
      MV_MixBuffer[ buffer ] = NULL;
      }

   return( MV_Ok );
   }

// vim:ts=3:expandtab:
