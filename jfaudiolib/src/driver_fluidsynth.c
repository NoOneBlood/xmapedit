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
 * FluidSynth MIDI synthesiser output
 */

#include "midifuncs.h"
#include "driver_fluidsynth.h"
#include "asssys.h"
#include <fluidsynth.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <glob.h>

#if ((FLUIDSYNTH_VERSION_MAJOR < 1) || \
     (FLUIDSYNTH_VERSION_MAJOR == 1 && FLUIDSYNTH_VERSION_MINOR < 1) || \
     (FLUIDSYNTH_VERSION_MAJOR == 1 && FLUIDSYNTH_VERSION_MINOR == 1 && FLUIDSYNTH_VERSION_MICRO < 2))
#error "FluidSynth support requires version 1.1.2 or better"
#endif

enum {
   FSynthErr_Warning = -2,
   FSynthErr_Error   = -1,
   FSynthErr_Ok      = 0,
   FSynthErr_Uninitialised,
   FSynthErr_NewFluidSettings,
   FSynthErr_NewFluidSynth,
   FSynthErr_NewFluidAudioDriver,
   FSynthErr_NewFluidSequencer,
   FSynthErr_RegisterFluidSynth,
   FSynthErr_BadSoundFont,
   FSynthErr_NewFluidEvent,
   FSynthErr_PlayThread
};

static int ErrorCode = FSynthErr_Ok;
static char soundFontName[PATH_MAX+1] = "";
static const char *soundFontPaths[] = {
    "./*.sf2",
    "/usr/share/soundfonts/*.sf2",
    "/usr/share/sounds/sf2/*.sf2",
    NULL,
};

static fluid_settings_t * fluidsettings = 0;
static fluid_synth_t * fluidsynth = 0;
static fluid_audio_driver_t * fluidaudiodriver = 0;
static fluid_sequencer_t * fluidsequencer = 0;
static fluid_event_t * fluidevent = 0;
static short synthseqid = -1;

static pthread_t thread;
static int threadRunning = 0;
static volatile int threadQuit = 0;
static void (* threadService)(void) = 0;

static unsigned int threadTimer = 0;
static unsigned int threadQueueTimer = 0;
static int threadQueueTicks = 0;
#define THREAD_QUEUE_INTERVAL 20    // 1/20 sec


int FluidSynthDrv_GetError(void)
{
	return ErrorCode;
}

const char *FluidSynthDrv_ErrorString( int ErrorNumber )
{
	const char *ErrorString;
	
    switch( ErrorNumber )
	{
        case FSynthErr_Warning :
        case FSynthErr_Error :
            ErrorString = FluidSynthDrv_ErrorString( ErrorCode );
            break;

        case FSynthErr_Ok :
            ErrorString = "FluidSynth ok.";
            break;
			
		case FSynthErr_Uninitialised:
			ErrorString = "FluidSynth uninitialised.";
			break;

        case FSynthErr_NewFluidSettings:
            ErrorString = "Failed creating new fluid settings.";
            break;

        case FSynthErr_NewFluidSynth:
            ErrorString = "Failed creating new fluid synth.";
            break;

        case FSynthErr_NewFluidAudioDriver:
            ErrorString = "Failed creating new fluid audio driver.";
            break;

        case FSynthErr_NewFluidSequencer:
            ErrorString = "Failed creating new fluid sequencer.";
            break;

        case FSynthErr_RegisterFluidSynth:
            ErrorString = "Failed registering fluid synth with sequencer.";
            break;

        case FSynthErr_BadSoundFont:
            ErrorString = "Invalid or non-existent SoundFont.";
            break;

        case FSynthErr_NewFluidEvent:
            ErrorString = "Failed creating new fluid event.";
            break;

        case FSynthErr_PlayThread:
            ErrorString = "Failed creating playback thread.";
            break;

        default:
            ErrorString = "Unknown FluidSynth error.";
            break;
    }
        
	return ErrorString;
}

static inline void sequence_event(void)
{
    int result = 0;

    //fluid_sequencer_send_now(fluidsequencer, fluidevent);
    result = fluid_sequencer_send_at(fluidsequencer, fluidevent, threadTimer, 1);
    if (result < 0) {
        ASS_Message("FluidSynthDrv: fluidsynth could not queue event\n");
    }
}

static int find_soundfont(void)
{
    int pathn, found = 0;
    unsigned globi;
    glob_t globt;

    for (pathn = 0; !found && soundFontPaths[pathn]; pathn++) {
        if (strchr(soundFontPaths[pathn], '*') != NULL) {
            // it's a glob pattern, so find the use the first readable match
            memset(&globt, 0, sizeof(globt));
            if (glob(soundFontPaths[pathn], 0, NULL, &globt) == 0) {
                for (globi = 0; globi < globt.gl_pathc; globi++) {
                    if (access(globt.gl_pathv[globi], R_OK) == 0) {
                        strcpy(soundFontName, globt.gl_pathv[globi]);
                        found = 1;
                        break;
                    }
                }
            }
            globfree(&globt);
        } else {
            if (access(soundFontPaths[pathn], R_OK) == 0) {
                strcpy(soundFontName, soundFontPaths[pathn]);
                found = 1;
            }
        }
    }

    return found;
}

static void apply_params(const char *params, fluid_settings_t *settings)
{
    char *parseparams, *savepair = NULL;
    char *parampair, *paramname, *paramvalue;
    char *firstpair;
    int setok;

    if (!params || !params[0]) return;

    parseparams = malloc(strlen(params) + 1);
    strcpy(parseparams, params);
    firstpair = parseparams;
    while ((parampair = strtok_r(firstpair, " ", &savepair))) {
        firstpair = NULL;
        paramname = strtok_r(parampair, "=", &paramvalue);
        if (!paramname) {
            break;
        }
        if (strcmp(paramname, "soundfont") == 0 || strcmp(paramname, "soundbank") == 0) {
            strcpy(soundFontName, paramvalue);
            continue;
        }

        switch (fluid_settings_get_type(settings, paramname)) {
            case FLUID_NUM_TYPE:
                ASS_Message("FluidSynthDrv: setting '%s' to %f\n", paramname, atof(paramvalue));
                setok = fluid_settings_setnum(settings, paramname, atof(paramvalue));
                break;
            case FLUID_INT_TYPE:
                ASS_Message("FluidSynthDrv: setting '%s' to %d\n", paramname, atoi(paramvalue));
                setok = fluid_settings_setint(settings, paramname, atoi(paramvalue));
                break;
            case FLUID_STR_TYPE:
                ASS_Message("FluidSynthDrv: setting '%s' to '%s'\n", paramname, paramvalue);
                setok = fluid_settings_setstr(settings, paramname, paramvalue);
                break;
            default:
                setok = 0;
                break;
        }
        if (setok < 0) {
            ASS_Message("FluidSynthDrv: error setting '%s' to '%s'\n", paramname, paramvalue);
        }
    }

    free(parseparams);
}

static void Func_NoteOff( int channel, int key, int velocity )
{
    (void)velocity;

    fluid_event_noteoff(fluidevent, channel, key);
    sequence_event();
}

static void Func_NoteOn( int channel, int key, int velocity )
{
    fluid_event_noteon(fluidevent, channel, key, velocity);
    sequence_event();
}

static void Func_PolyAftertouch( int channel, int key, int pressure )
{
    (void)pressure;

    ASS_Message("FluidSynthDrv: key %d channel %d aftertouch\n", key, channel);
}

static void Func_ControlChange( int channel, int number, int value )
{
    fluid_event_control_change(fluidevent, channel, number, value);
    sequence_event();
}

static void Func_ProgramChange( int channel, int program )
{
    fluid_event_program_change(fluidevent, channel, program);
    sequence_event();
}

static void Func_ChannelAftertouch( int channel, int pressure )
{
    (void)pressure;

    ASS_Message("FluidSynthDrv: channel %d aftertouch\n", channel);
}

static void Func_PitchBend( int channel, int lsb, int msb )
{
    fluid_event_pitch_bend(fluidevent, channel, lsb | (msb << 7));
    sequence_event();
}

static void * threadProc(void * parm)
{
    struct timeval tv;
    int sleepAmount = 1000000 / THREAD_QUEUE_INTERVAL;
    unsigned int sequenceTime;

    (void)parm;

    // prime the pump
    threadTimer = fluid_sequencer_get_tick(fluidsequencer);
    threadQueueTimer = threadTimer + threadQueueTicks;
    while (threadTimer < threadQueueTimer) {
        if (threadService) {
            threadService();
        }
        threadTimer++;
    }
    
    while (!threadQuit) {
        tv.tv_sec = 0;
        tv.tv_usec = sleepAmount;

        select(0, NULL, NULL, NULL, &tv);

        sequenceTime = fluid_sequencer_get_tick(fluidsequencer);

        sleepAmount = 1000000 / THREAD_QUEUE_INTERVAL;
        if ((int)(threadTimer - sequenceTime) > threadQueueTicks) {
            // we're running ahead, so sleep for half the usual
            // amount and try again
            sleepAmount /= 2;
            continue;
        }

        threadQueueTimer = sequenceTime + threadQueueTicks;
        while (threadTimer < threadQueueTimer) {
            if (threadService) {
                threadService();
            }
            threadTimer++;
        }
    }

    return NULL;
}

int FluidSynthDrv_MIDI_Init(midifuncs *funcs, const char *params)
{
    int result;
    
    FluidSynthDrv_MIDI_Shutdown();
    memset(funcs, 0, sizeof(midifuncs));

    ASS_Message("FluidSynthDrv: using version %s, built with %s\n", fluid_version_str(), FLUIDSYNTH_VERSION);

    fluidsettings = new_fluid_settings();
    if (!fluidsettings) {
        ErrorCode = FSynthErr_NewFluidSettings;
        return FSynthErr_Error;
    }

    apply_params(params, fluidsettings);

    if (soundFontName[0]) {
        ASS_Message("FluidSynthDrv: using soundfont %s\n", soundFontName);
    } else if (find_soundfont()) {
        ASS_Message("FluidSynthDrv: using found soundfont %s\n", soundFontName);
    } else {
        fluid_settings_copystr(fluidsettings, "synth.default-soundfont", soundFontName,
            sizeof(soundFontName));
    }

    fluidsynth = new_fluid_synth(fluidsettings);
    if (!fluidsettings) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_NewFluidSynth;
        return FSynthErr_Error;
    }

    result = fluid_synth_sfload(fluidsynth, soundFontName, 1);
    if (result < 0) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_BadSoundFont;
        return FSynthErr_Error;
    }

    fluidsequencer = new_fluid_sequencer2(0);
    if (!fluidsettings) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_NewFluidSequencer;
        return FSynthErr_Error;
    }

    synthseqid = fluid_sequencer_register_fluidsynth(fluidsequencer, fluidsynth);
    if (synthseqid < 0) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_RegisterFluidSynth;
        return FSynthErr_Error;
    }

    fluidaudiodriver = new_fluid_audio_driver(fluidsettings, fluidsynth);
    if (!fluidsettings) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_NewFluidAudioDriver;
        return FSynthErr_Error;
    }

    fluidevent = new_fluid_event();
    if (!fluidevent) {
        FluidSynthDrv_MIDI_Shutdown();
        ErrorCode = FSynthErr_NewFluidEvent;
        return FSynthErr_Error;
    }

    fluid_event_set_source(fluidevent, -1);
    fluid_event_set_dest(fluidevent, synthseqid);
    
    funcs->NoteOff = Func_NoteOff;
    funcs->NoteOn  = Func_NoteOn;
    funcs->PolyAftertouch = Func_PolyAftertouch;
    funcs->ControlChange = Func_ControlChange;
    funcs->ProgramChange = Func_ProgramChange;
    funcs->ChannelAftertouch = Func_ChannelAftertouch;
    funcs->PitchBend = Func_PitchBend;
    
    return FSynthErr_Ok;
}

void FluidSynthDrv_MIDI_Shutdown(void)
{
    if (fluidaudiodriver) {
        delete_fluid_audio_driver(fluidaudiodriver);
    }
    if (synthseqid >= 0) {
        fluid_sequencer_unregister_client(fluidsequencer, synthseqid);
    }
    if (fluidsequencer) {
        delete_fluid_sequencer(fluidsequencer);
    }
    if (fluidsynth) {
        delete_fluid_synth(fluidsynth);
    }
    if (fluidsettings) {
        delete_fluid_settings(fluidsettings);
    }
    if (fluidevent) {
        delete_fluid_event(fluidevent);
    }
    synthseqid = -1;
    fluidevent = 0;
    fluidsequencer = 0;
    fluidaudiodriver = 0;
    fluidsynth = 0;
    fluidsettings = 0;
}

int FluidSynthDrv_MIDI_StartPlayback(void (*service)(void))
{
    FluidSynthDrv_MIDI_HaltPlayback();

    threadService = service;
    threadQuit = 0;

    if (pthread_create(&thread, NULL, threadProc, NULL)) {
        ASS_Message("FluidSynthDrv: pthread_create returned error\n");
        return FSynthErr_PlayThread;
    }

    threadRunning = 1;

    return 0;
}

void FluidSynthDrv_MIDI_HaltPlayback(void)
{
    void * ret;
    
    if (!threadRunning) {
        return;
    }

    threadQuit = 1;

    if (pthread_join(thread, &ret)) {
        ASS_Message("FluidSynthDrv: pthread_join returned error\n");
    }

    threadRunning = 0;
}

void FluidSynthDrv_MIDI_SetTempo(int tempo, int division)
{
    double tps;

    tps = ( (double) tempo * (double) division ) / 60.0;
    fluid_sequencer_set_time_scale(fluidsequencer, tps);

    threadQueueTicks = (int) ceil(tps / (double) THREAD_QUEUE_INTERVAL);
}

void FluidSynthDrv_MIDI_Lock(void)
{
}

void FluidSynthDrv_MIDI_Unlock(void)
{
}

