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

#include "cd.h"
#include "drivers.h"
#include "asssys.h"
#include <assert.h>

static int ErrorCode = CD_Ok;

int CD_GetError(void)
{
    return ErrorCode;
}

const char * CD_ErrorString(int code)
{
    switch (code) {
        case CD_Error:
            // make sure not to create an endless loop here
            assert(ErrorCode != CD_Error);
            if(ErrorCode != CD_Error)
                return CD_ErrorString(ErrorCode);
            return "General CD error";

        case CD_Ok:
            return "No error.";

        case CD_InvalidCard:
            return "Invalid or unsupported card.";

        case CD_DriverError:
            return SoundDriver_CD_ErrorString(SoundDriver_CD_GetError());

        default:
            return "Unknown error code.";
    }
}

int CD_Init(int SoundCard)
{
    int err;
    int mincard, maxcard, card;

    if (SoundCard == ASS_AutoDetect) {
        mincard = ASS_NoSound + 1;
        maxcard = ASS_NumSoundCards - 1;
    } else if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards) {
        ErrorCode = CD_InvalidCard;
        return CD_Error;
    } else {
        mincard = SoundCard;
        maxcard = SoundCard;
    }

    for (card = mincard; card <= maxcard; card++) {
        if (!SoundDriver_IsCDSupported(card)) {
            continue;
        } else if (SoundCard == ASS_AutoDetect) {
            ASS_Message("CD_Init: trying %s\n", SoundDriver_GetName(card));
        }

        ASS_CDSoundDriver = card;
        err = SoundDriver_CD_Init();
        if (err == CD_Ok) {
            return CD_Ok;
        }
    }

    if (SoundCard == ASS_AutoDetect) {
        // A failure to autodetect falls back to no sound.
        ASS_CDSoundDriver = ASS_NoSound;
        return CD_Ok;
    }

    ErrorCode = CD_DriverError;
    return CD_Error;
}

int  CD_GetCurrentDriver(void)
{
    return ASS_CDSoundDriver;
}

const char * CD_GetCurrentDriverName(void)
{
    return SoundDriver_GetName(ASS_CDSoundDriver);
}


void CD_Shutdown(void)
{
    SoundDriver_CD_Shutdown();
}

int CD_Play(int track, int loop)
{
    int err;
    
    err = SoundDriver_CD_Play(track, loop);
    if (err != CD_Ok) {
        // the actual error string will be obtained from the driver
        ErrorCode = CD_DriverError;
        return CD_Error;
    }

    return CD_Ok;
}

void CD_Stop(void)
{
    SoundDriver_CD_Stop();
}

void CD_Pause(int pauseon)
{
    SoundDriver_CD_Pause(pauseon);
}

int CD_IsPlaying(void)
{
    return SoundDriver_CD_IsPlaying();
}

void CD_SetVolume(int volume)
{
    SoundDriver_CD_SetVolume(volume);
}

// vim:ts=4:expandtab:

