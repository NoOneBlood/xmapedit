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
 * OggVorbis source support for MultiVoc
 */

#ifdef HAVE_VORBIS

#ifdef __APPLE__
# include <vorbis/vorbisfile.h>
#else
# include "vorbis/vorbisfile.h"
#endif

#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <unistd.h>
#endif
#include <errno.h>
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"
#include "asssys.h"
#include "assmisc.h"

typedef struct {
   char * ptr;
   size_t length;
   size_t pos;
   
   OggVorbis_File vf;
   
   char block[0x8000];
   int lastbitstream;
} vorbis_data;

static size_t read_vorbis(void * ptr, size_t size, size_t nmemb, void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   size_t nread = 0;
   size_t bytes, woffset = 0;

   errno = 0;

   if (vorb->length == vorb->pos) {
      return 0;
   }
   
   for (; nmemb > 0; nmemb--, nread++) {
      bytes = vorb->length - vorb->pos;
      if (size < bytes) {
         bytes = size;
      }

      memcpy((char *)ptr + woffset, vorb->ptr + vorb->pos, bytes);
      vorb->pos += bytes;
      woffset += bytes;

      if (vorb->length == vorb->pos) {
         nread++;
         break;
      }
   }
   
   return nread;
}

static int seek_vorbis(void * datasource, ogg_int64_t offset, int whence)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   switch (whence) {
      case SEEK_SET: vorb->pos = 0; break;
      case SEEK_CUR: break;
      case SEEK_END: vorb->pos = vorb->length; break;
   }
   
   vorb->pos += offset;
   if (vorb->pos < 0) {
      vorb->pos = 0;
   } else if (vorb->pos > vorb->length) {
      vorb->pos = vorb->length;
   }
   
   return vorb->pos;
}

static int close_vorbis(void * datasource)
{
   (void)datasource;
   return 0;
}

static long tell_vorbis(void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   return vorb->pos;
}

static ov_callbacks vorbis_callbacks = {
   read_vorbis,
   seek_vorbis,
   close_vorbis,
   tell_vorbis
};


/*---------------------------------------------------------------------
Function: MV_GetNextVorbisBlock

Controls playback of OggVorbis data
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextVorbisBlock
(
 VoiceNode *voice
 )

{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   int bytes = 0, bytesread = 0;
   int bitstream = 0, err = 0;

   voice->Playing = TRUE;
   
   bytesread = 0;
   do {
      bytes = ov_read(&vd->vf, vd->block + bytesread, sizeof(vd->block) - bytesread, 0, 2, 1, &bitstream);
      //ASS_Message("ov_read = %d\n", bytes);
      if (bytes == OV_HOLE) continue;
      if (bytes == 0) {
         if (voice->LoopStart) {
            err = ov_pcm_seek_page(&vd->vf, 0);
            if (err != 0) {
               ASS_Message("MV_GetNextVorbisBlock ov_pcm_seek_page_lap: err %d\n", err);
            } else {
               continue;
            }
         } else {
           break;
         }
      } else if (bytes < 0) {
         ASS_Message("MV_GetNextVorbisBlock ov_read: err %d\n", bytes);
         voice->Playing = FALSE;
         return NoMoreData;
      }

      bytesread += bytes;
   } while (bytesread < (int)sizeof(vd->block));

   if (bytesread == 0) {
      voice->Playing = FALSE;
      return NoMoreData;
   }
      
   if (bitstream != vd->lastbitstream) {
      vorbis_info * vi = 0;
      
      vi = ov_info(&vd->vf, -1);
      if (!vi) {
         voice->Playing = FALSE;
         return NoMoreData;
      }
      
      if (vi->channels != 1 && vi->channels != 2) {
         voice->Playing = FALSE;
         return NoMoreData;
      }
      
      voice->channels = vi->channels;
      voice->SamplingRate = vi->rate;
      voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
      voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
         voice->RateScale;
      MV_SetVoiceMixMode( voice );
   }
   vd->lastbitstream = bitstream;
   
   bytesread /= 2 * voice->channels;
   
   voice->position    = 0;
   voice->sound       = vd->block;
   voice->BlockLength = 0;
   voice->length      = bytesread << 16;
   
   return( KeepPlaying );
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis3D

Begin playback of sound data at specified angle and distance
from listener.
---------------------------------------------------------------------*/

int MV_PlayVorbis3D
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
   
   status = MV_PlayVorbis( ptr, ptrlength, pitchoffset, mid, left, right, priority,
                           callbackval );
   
   return( status );
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int MV_PlayVorbis
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
   
   status = MV_PlayLoopedVorbis( ptr, ptrlength, -1, -1, pitchoffset, vol, left, right,
                                 priority, callbackval );
   
   return( status );
}


/*---------------------------------------------------------------------
Function: MV_PlayLoopedVorbis

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedVorbis
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
   vorbis_data * vd = 0;
   vorbis_info * vi = 0;

   (void)loopend;

   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
   }
   
   vd = (vorbis_data *) malloc( sizeof(vorbis_data) );
   if (!vd) {
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   memset(vd, 0, sizeof(vorbis_data));
   vd->ptr = ptr;
   vd->pos = 0;
   vd->length = ptrlength;
   vd->lastbitstream = -1;
   
   status = ov_open_callbacks((void *) vd, &vd->vf, 0, 0, vorbis_callbacks);
   if (status < 0) {
      ASS_Message("MV_PlayLoopedVorbis: err %d\n", status);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   vi = ov_info(&vd->vf, 0);
   if (!vi) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   if (vi->channels != 1 && vi->channels != 2) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
   {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
   }
   
   voice->wavetype    = Vorbis;
   voice->bits        = 16;
   voice->channels    = vi->channels;
   voice->extra       = (void *) vd;
   voice->GetSound    = MV_GetNextVorbisBlock;
   voice->NextBlock   = vd->block;
   voice->DemandFeed  = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->length      = 0;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;
   voice->LoopStart   = (char *)(intptr_t)(loopstart >= 0 ? TRUE : FALSE);
   voice->LoopEnd     = 0;
   voice->LoopSize    = 0;
   voice->Playing     = TRUE;
   voice->Paused      = FALSE;
   
   voice->SamplingRate = vi->rate;
   voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
   voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
      voice->RateScale;
   MV_SetVoiceMixMode( voice );

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );
   
   return( voice->handle );
}


void MV_ReleaseVorbisVoice( VoiceNode * voice )
{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   
   if (voice->wavetype != Vorbis) {
      return;
   }
   
   ov_clear(&vd->vf);
   free(vd);
   
   voice->extra = 0;
}

#endif //HAVE_VORBIS
