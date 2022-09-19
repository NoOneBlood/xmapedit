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


int SDLDrv_GetError(void);
const char *SDLDrv_ErrorString( int ErrorNumber );

int  SDLDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata);
void SDLDrv_PCM_Shutdown(void);
int  SDLDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
                 int NumDivisions, void ( *CallBackFunc )( void ) );
void SDLDrv_PCM_StopPlayback(void);
void SDLDrv_PCM_Lock(void);
void SDLDrv_PCM_Unlock(void);

int  SDLDrv_CD_Init(void);
void SDLDrv_CD_Shutdown(void);
int  SDLDrv_CD_Play(int track, int loop);
void SDLDrv_CD_Stop(void);
void SDLDrv_CD_Pause(int pauseon);
int  SDLDrv_CD_IsPlaying(void);
void SDLDrv_CD_SetVolume(int volume);

