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

int WinMMDrv_GetError(void);
const char *WinMMDrv_ErrorString( int ErrorNumber );

int  WinMMDrv_CD_Init(void);
void WinMMDrv_CD_Shutdown(void);
int  WinMMDrv_CD_Play(int track, int loop);
void WinMMDrv_CD_Stop(void);
void WinMMDrv_CD_Pause(int pauseon);
int  WinMMDrv_CD_IsPlaying(void);
void WinMMDrv_CD_SetVolume(int volume);

int  WinMMDrv_MIDI_Init(midifuncs *, const char *);
void WinMMDrv_MIDI_Shutdown(void);
int  WinMMDrv_MIDI_StartPlayback(void (*service)(void));
void WinMMDrv_MIDI_HaltPlayback(void);
void WinMMDrv_MIDI_SetTempo(int tempo, int division);
void WinMMDrv_MIDI_Lock(void);
void WinMMDrv_MIDI_Unlock(void);

