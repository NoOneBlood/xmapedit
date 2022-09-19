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

#ifndef __CD_H
#define __CD_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CD_Error = -1,
    CD_Ok,
    CD_InvalidCard,
    CD_DriverError
};

int  CD_GetError(void);
const char * CD_ErrorString(int code);
int  CD_Init(int SoundCard);
int  CD_GetCurrentDriver(void);
const char * CD_GetCurrentDriverName(void);
void CD_Shutdown(void);
int  CD_Play(int track, int loop);
void CD_Stop(void);
void CD_Pause(int pauseon);
int  CD_IsPlaying(void);
void CD_SetVolume(int volume);

#ifdef __cplusplus
}
#endif

#endif
