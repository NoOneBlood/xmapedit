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

int DirectSoundDrv_GetError(void);
const char *DirectSoundDrv_ErrorString( int ErrorNumber );

int  DirectSoundDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata);
void DirectSoundDrv_PCM_Shutdown(void);
int  DirectSoundDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
                 int NumDivisions, void ( *CallBackFunc )( void ) );
void DirectSoundDrv_PCM_StopPlayback(void);
void DirectSoundDrv_PCM_Lock(void);
void DirectSoundDrv_PCM_Unlock(void);

