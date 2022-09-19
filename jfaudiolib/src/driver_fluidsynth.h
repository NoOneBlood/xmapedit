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

int FluidSynthDrv_GetError(void);
const char *FluidSynthDrv_ErrorString( int ErrorNumber );

int  FluidSynthDrv_MIDI_Init(midifuncs *, const char *);
void FluidSynthDrv_MIDI_Shutdown(void);
int  FluidSynthDrv_MIDI_StartPlayback(void (*service)(void));
void FluidSynthDrv_MIDI_HaltPlayback(void);
unsigned int FluidSynthDrv_MIDI_GetTick(void);
void FluidSynthDrv_MIDI_SetTempo(int tempo, int division);
void FluidSynthDrv_MIDI_Lock(void);
void FluidSynthDrv_MIDI_Unlock(void);
