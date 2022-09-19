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
 * CoreAudio output driver for MultiVoc
 *
 * Resources found to be useful in implementing this:
 *   http://lists.apple.com/archives/coreaudio-api/2006/Jan/msg00010.html
 *   http://home.roadrunner.com/~jgglatt/tech/midifile/ppqn.htm
 */

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include "asssys.h"
#include "midifuncs.h"
#include "driver_coreaudio.h"

enum {
    CAErr_Warning = -2,
    CAErr_Error   = -1,
    CAErr_Ok      = 0,
    CAErr_Uninitialised,
    CAErr_AssembleAUGraph,
    CAErr_InitialiseAUGraph,
    CAErr_SetPCMFormat,
    CAErr_Mutex,
    CAErr_SetSoundBank
};

enum {
    CASystem_none = 0,
    CASystem_pcm = 1,
    CASystem_midi = 2
};

static int ErrorCode = CAErr_Ok;
static int Initialised = CASystem_none;
static pthread_mutex_t pcmmutex, midimutex;
static AUGraph graph = 0;
static AudioUnit mixerunit, pcmunit, synthunit;

static char *MixBuffer = 0;
static int MixBufferSize = 0;
static int MixBufferCount = 0;
static int MixBufferCurrent = 0;
static int MixBufferUsed = 0;
static void ( *MixCallBack )( void ) = 0;

static void ( *MidiCallBack )( void ) = 0;
static unsigned int MidiTick = 0;
static unsigned int MidiFramesPerTick = 0;
static unsigned int MidiFrameOffset = 0;
#define MIDI_NOTE_OFF         0x80
#define MIDI_NOTE_ON          0x90
#define MIDI_POLY_AFTER_TCH   0xA0
#define MIDI_CONTROL_CHANGE   0xB0
#define MIDI_PROGRAM_CHANGE   0xC0
#define MIDI_AFTER_TOUCH      0xD0
#define MIDI_PITCH_BEND       0xE0
#define MIDI_META_EVENT       0xFF
#define MIDI_END_OF_TRACK     0x2F
#define MIDI_TEMPO_CHANGE     0x51
#define MIDI_MONO_MODE_ON     0x7E
#define MIDI_ALL_NOTES_OFF    0x7B

static char soundBankName[PATH_MAX+1] = "";


static OSStatus pcmService(
                    void                         *inRefCon,
                    AudioUnitRenderActionFlags   *inActionFlags,
                    const AudioTimeStamp         *inTimeStamp,
                    UInt32                       inBusNumber,
                    UInt32                       inNumberFrames,
                    AudioBufferList              *ioData)
{
    UInt32 remaining, len, bufn;
    char *ptr, *sptr;

    (void)inRefCon; (void)inActionFlags; (void)inTimeStamp; (void)inBusNumber; (void)inNumberFrames;

    if (MixCallBack == 0) return noErr;

    CoreAudioDrv_PCM_Lock();
    for (bufn = 0; bufn < ioData->mNumberBuffers; bufn++) {
        remaining = ioData->mBuffers[bufn].mDataByteSize;
        ptr = ioData->mBuffers[bufn].mData;

        while (remaining > 0) {
            if (MixBufferUsed == MixBufferSize) {
                MixCallBack();
                
                MixBufferUsed = 0;
                MixBufferCurrent++;
                if (MixBufferCurrent >= MixBufferCount) {
                    MixBufferCurrent -= MixBufferCount;
                }
            }
            
            while (remaining > 0 && MixBufferUsed < MixBufferSize) {
                sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;
                
                len = MixBufferSize - MixBufferUsed;
                if (remaining < len) {
                    len = remaining;
                }
                
                memcpy(ptr, sptr, len);
                
                ptr += len;
                MixBufferUsed += len;
                remaining -= len;
            }
        }
    }
    CoreAudioDrv_PCM_Unlock();

    return noErr;
}

static OSStatus midiService(
                            void                        *inRefCon,
                            AudioUnitRenderActionFlags  *ioActionFlags,
                            const AudioTimeStamp        *inTimeStamp,
                            UInt32                      inBusNumber,
                            UInt32                      inNumberFrames,
                            AudioBufferList             *ioData)
{
    int secondsThisCall = (inNumberFrames << 16) / 44100;

    (void)inRefCon; (void)inTimeStamp; (void)inBusNumber; (void)ioData;

    if (MidiCallBack == 0) return noErr;
    
    if (!(*ioActionFlags & kAudioUnitRenderAction_PreRender)) return noErr;
    
    CoreAudioDrv_MIDI_Lock();
    while (MidiFrameOffset < inNumberFrames) {
        MidiCallBack();
        
        MidiTick++;
        MidiFrameOffset += MidiFramesPerTick;
    }
    MidiFrameOffset -= inNumberFrames;
    CoreAudioDrv_MIDI_Unlock();
    
    return noErr;
}



int CoreAudioDrv_GetError(void)
{
    return ErrorCode;
}

const char *CoreAudioDrv_ErrorString( int ErrorNumber )
{
    const char *ErrorString;
    
    switch( ErrorNumber )
    {
        case CAErr_Warning :
        case CAErr_Error :
            ErrorString = CoreAudioDrv_ErrorString( ErrorCode );
            break;
            
        case CAErr_Ok :
            ErrorString = "CoreAudio ok.";
            break;
            
        case CAErr_Uninitialised:
            ErrorString = "CoreAudio uninitialised.";
            break;
            
        case CAErr_AssembleAUGraph:
            ErrorString = "CoreAudio error: could not assemble Audio Unit graph.";
            break;
        
        case CAErr_InitialiseAUGraph:
            ErrorString = "CoreAudio error: could not initialise Audio Unit graph.";
            break;
            
        case CAErr_SetPCMFormat:
            ErrorString = "CoreAudio error: could not set PCM format.";
            break;

        case CAErr_Mutex:
            ErrorString = "CoreAudio error: could not initialise mutex.";
            break;
            
        default:
            ErrorString = "Unknown CoreAudio error code.";
            break;
    }
    
    return ErrorString;
}


#define check_result(fcall, errval) \
if ((result = (fcall)) != noErr) {\
    ASS_Message("CoreAudioDrv: error %d at line %d:" #fcall "\n", (int)result, __LINE__);\
    ErrorCode = errval;\
    return CAErr_Error;\
}

static int initialise_graph(int subsystem)
{
    OSStatus result;
    AudioComponentDescription desc;
    AURenderCallbackStruct callback;
    AudioStreamBasicDescription pcmDesc;
    AUNode outputnode, mixernode, pcmnode, synthnode;

    memset(&pcmDesc, 0, sizeof(pcmDesc));
    
    if (Initialised) {
        Initialised |= subsystem;
        return CAErr_Ok;
    }
    
    if (pthread_mutex_init(&pcmmutex, 0) < 0) {
        ErrorCode = CAErr_Mutex;
        return CAErr_Error;
    }
    if (pthread_mutex_init(&midimutex, 0) < 0) {
        ErrorCode = CAErr_Mutex;
        return CAErr_Error;
    }
    
    // create the graph
    check_result(NewAUGraph(&graph), CAErr_AssembleAUGraph);
    
    // add the output node to the graph
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    check_result(AUGraphAddNode(graph, &desc, &outputnode), CAErr_AssembleAUGraph);

    // add the mixer node to the graph
    desc.componentType = kAudioUnitType_Mixer;
    desc.componentSubType = kAudioUnitSubType_StereoMixer;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    check_result(AUGraphAddNode(graph, &desc, &mixernode), CAErr_AssembleAUGraph);
    
    // add a node for PCM audio
    desc.componentType = kAudioUnitType_FormatConverter;
    desc.componentSubType = kAudioUnitSubType_AUConverter;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    check_result(AUGraphAddNode(graph, &desc, &pcmnode), CAErr_AssembleAUGraph);
    
    // add a node for a MIDI synth
    desc.componentType = kAudioUnitType_MusicDevice;
    desc.componentSubType = kAudioUnitSubType_DLSSynth;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    check_result(AUGraphAddNode(graph, &desc, &synthnode), CAErr_AssembleAUGraph);
    
    // connect the nodes together
    check_result(AUGraphConnectNodeInput(graph, pcmnode, 0, mixernode, 0), CAErr_AssembleAUGraph);
    check_result(AUGraphConnectNodeInput(graph, synthnode, 0, mixernode, 1), CAErr_AssembleAUGraph);
    check_result(AUGraphConnectNodeInput(graph, mixernode, 0, outputnode, 0), CAErr_AssembleAUGraph);
    
    // open the nodes
    check_result(AUGraphOpen(graph), CAErr_AssembleAUGraph);

    // get the units
    check_result(AUGraphNodeInfo(graph, pcmnode, NULL, &pcmunit), CAErr_AssembleAUGraph);
    check_result(AUGraphNodeInfo(graph, synthnode, NULL, &synthunit), CAErr_AssembleAUGraph);
    check_result(AUGraphNodeInfo(graph, mixernode, NULL, &mixerunit), CAErr_AssembleAUGraph);
    
    // set the mixer bus count
    UInt32 buscount = 2;
    check_result(AudioUnitSetProperty(mixerunit,
                    kAudioUnitProperty_ElementCount,
                    kAudioUnitScope_Input,
                    0,
                    &buscount,
                    sizeof(buscount)), CAErr_InitialiseAUGraph);

    // set the pcm audio callback
    callback.inputProc = pcmService;
    callback.inputProcRefCon = 0;
    check_result(AUGraphSetNodeInputCallback(graph, pcmnode, 0, &callback), CAErr_InitialiseAUGraph);

    // set a default pcm audio format
    pcmDesc.mFormatID = kAudioFormatLinearPCM;
    pcmDesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    pcmDesc.mChannelsPerFrame = 2;
    pcmDesc.mSampleRate = 44100;
    pcmDesc.mBitsPerChannel = 16;
    pcmDesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
#ifdef __POWERPC__
    pcmDesc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
    pcmDesc.mFramesPerPacket = 1;
    pcmDesc.mBytesPerFrame = pcmDesc.mBitsPerChannel * pcmDesc.mChannelsPerFrame / 8;
    pcmDesc.mBytesPerPacket = pcmDesc.mBytesPerFrame * pcmDesc.mFramesPerPacket;

    check_result(AudioUnitSetProperty(pcmunit,
                    kAudioUnitProperty_StreamFormat,
                    kAudioUnitScope_Input,
                    0,
                    &pcmDesc,
                    sizeof(pcmDesc)), CAErr_SetPCMFormat);

    // Set a sound bank for the DLS synth
    if (soundBankName[0]) {
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL,
                           (const UInt8 *)soundBankName, strlen(soundBankName), FALSE);
        if (url) {
            check_result(AudioUnitSetProperty(synthunit,
                            kMusicDeviceProperty_SoundBankURL,
                            kAudioUnitScope_Global,
                            0,
                            &url,
                            sizeof(url)), CAErr_SetSoundBank);
            CFRelease(url);
        }
    }

    // set the synth notify callback
    check_result(AudioUnitAddRenderNotify(synthunit, midiService, NULL), CAErr_InitialiseAUGraph);

    // initialise the graph
    check_result(AUGraphInitialize(graph), CAErr_InitialiseAUGraph);

    // start the graph
    check_result(AUGraphStart(graph), CAErr_InitialiseAUGraph);
    
    //CAShow(graph);
    
    Initialised |= subsystem;
    return CAErr_Ok;
}

static int uninitialise_graph(int subsystem)
{
    OSStatus result;

    if (!Initialised) {
        return CAErr_Ok;
    }
    
    Initialised &= ~subsystem;
    if (Initialised) {
        // a subsystem is still using the graph so leave it active
        return CAErr_Ok;
    }
    
    CoreAudioDrv_PCM_Lock();
    CoreAudioDrv_MIDI_Lock();
    
    // clean up the graph
    AUGraphStop(graph);
    DisposeAUGraph(graph);
    graph = 0;
    
    CoreAudioDrv_MIDI_Unlock();
    CoreAudioDrv_PCM_Unlock();
    
    pthread_mutex_destroy(&pcmmutex);
    pthread_mutex_destroy(&midimutex);
    
    return CAErr_Ok;
}    

static void parse_params(const char *params)
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
        if (strcmp(paramname, "soundbank") == 0 || strcmp(paramname, "soundfont") == 0) {
            ASS_Message("CoreAudioDrv: using sound bank %s\n", paramvalue);
            strcpy(soundBankName, paramvalue);
            continue;
        }
    }

    free(parseparams);
}

int CoreAudioDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata)
{
    OSStatus result = noErr;
    AudioStreamBasicDescription pcmDesc;

    (void)initdata;

    result = initialise_graph(CASystem_pcm);
    if (result != CAErr_Ok) {
        return result;
    }

    // set the requested pcm audio format
    pcmDesc.mFormatID = kAudioFormatLinearPCM;
    pcmDesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    pcmDesc.mChannelsPerFrame = *numchannels;
    pcmDesc.mSampleRate = *mixrate;
    pcmDesc.mBitsPerChannel = *samplebits;
    pcmDesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
#ifdef __POWERPC__
    pcmDesc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
    pcmDesc.mFramesPerPacket = 1;
    pcmDesc.mBytesPerFrame = pcmDesc.mBitsPerChannel * pcmDesc.mChannelsPerFrame / 8;
    pcmDesc.mBytesPerPacket = pcmDesc.mBytesPerFrame * pcmDesc.mFramesPerPacket;
    
    check_result(AudioUnitSetProperty(pcmunit,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input,
                                      0,
                                      &pcmDesc,
                                      sizeof(pcmDesc)), CAErr_SetPCMFormat);
    
    return CAErr_Ok;
}

void CoreAudioDrv_PCM_Shutdown(void)
{
    CoreAudioDrv_PCM_StopPlayback();
    
    uninitialise_graph(CASystem_pcm);
}

int CoreAudioDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
                                  int NumDivisions, void ( *CallBackFunc )( void ) )
{
    OSStatus result;

    if (!(Initialised & CASystem_pcm)) {
        ErrorCode = CAErr_Uninitialised;
        return CAErr_Error;
    }
    
    CoreAudioDrv_PCM_Lock();

    MixBuffer = BufferStart;
    MixBufferSize = BufferSize;
    MixBufferCount = NumDivisions;
    MixBufferCurrent = 0;
    MixBufferUsed = 0;
    MixCallBack = CallBackFunc;
    
    // prime the buffer
    MixCallBack();
    
    CoreAudioDrv_PCM_Unlock();
    
    return CAErr_Ok;
}

void CoreAudioDrv_PCM_StopPlayback(void)
{
    OSStatus result;
    AURenderCallbackStruct callback;

    if (!MixCallBack) {
        return;
    }
    
    CoreAudioDrv_PCM_Lock();

    MixCallBack = 0;

    CoreAudioDrv_PCM_Unlock();
}

void CoreAudioDrv_PCM_Lock(void)
{
    if (!(Initialised & CASystem_pcm)) {
        return;
    }

    pthread_mutex_lock(&pcmmutex);
}

void CoreAudioDrv_PCM_Unlock(void)
{
    if (!(Initialised & CASystem_pcm)) {
        return;
    }

    pthread_mutex_unlock(&pcmmutex);
}


static void Func_NoteOff( int channel, int key, int velocity )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_NOTE_OFF | channel,
                         key,
                         velocity,
                         MidiFrameOffset);
}

static void Func_NoteOn( int channel, int key, int velocity )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_NOTE_ON | channel,
                         key,
                         velocity,
                         MidiFrameOffset);
}

static void Func_PolyAftertouch( int channel, int key, int pressure )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_POLY_AFTER_TCH | channel,
                         key,
                         pressure,
                         MidiFrameOffset);
}

static void Func_ControlChange( int channel, int number, int value )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_CONTROL_CHANGE | channel,
                         number,
                         value,
                         MidiFrameOffset);
}

static void Func_ProgramChange( int channel, int program )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_PROGRAM_CHANGE | channel,
                         program,
                         0,
                         MidiFrameOffset);
}

static void Func_ChannelAftertouch( int channel, int pressure )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_AFTER_TOUCH | channel,
                         pressure,
                         0,
                         MidiFrameOffset);
}

static void Func_PitchBend( int channel, int lsb, int msb )
{
    MusicDeviceMIDIEvent(synthunit,
                         MIDI_PITCH_BEND | channel,
                         lsb,
                         msb,
                         MidiFrameOffset);
}

static void Func_SysEx( const unsigned char * data, int length )
{
    MusicDeviceSysEx(synthunit, data, length);
}


int CoreAudioDrv_MIDI_Init(midifuncs *funcs, const char *params)
{
    OSStatus result;
    
    memset(funcs, 0, sizeof(midifuncs));

    parse_params(params);

    result = initialise_graph(CASystem_midi);
    if (result != CAErr_Ok) {
        return result;
    }
    
    funcs->NoteOff = Func_NoteOff;
    funcs->NoteOn  = Func_NoteOn;
    funcs->PolyAftertouch = Func_PolyAftertouch;
    funcs->ControlChange = Func_ControlChange;
    funcs->ProgramChange = Func_ProgramChange;
    funcs->ChannelAftertouch = Func_ChannelAftertouch;
    funcs->PitchBend = Func_PitchBend;
    funcs->SysEx = Func_SysEx;

    return CAErr_Ok;
}

void CoreAudioDrv_MIDI_Shutdown(void)
{
    CoreAudioDrv_MIDI_HaltPlayback();
    
    uninitialise_graph(CASystem_midi);
}

int  CoreAudioDrv_MIDI_StartPlayback(void (*service)(void))
{
    if (!(Initialised & CASystem_midi)) {
        ErrorCode = CAErr_Uninitialised;
        return CAErr_Error;
    }

    CoreAudioDrv_MIDI_Lock();

    MidiCallBack = service;
    MidiTick = 0;
    MidiFrameOffset = 0;
    MidiFramesPerTick = (44100 * ((60 << 16) / (120 * 96))) >> 16;

    CoreAudioDrv_MIDI_Unlock();
    
    return 0;
}

void CoreAudioDrv_MIDI_HaltPlayback(void)
{
    if (!MixCallBack) {
        return;
    }
    
    CoreAudioDrv_MIDI_Lock();
    
    MidiCallBack = 0;
    
    CoreAudioDrv_MIDI_Unlock();
}

unsigned int CoreAudioDrv_MIDI_GetTick(void)
{
    return MidiTick;
}

void CoreAudioDrv_MIDI_SetTempo(int tempo, int division)
{
    int secondspertick = (60 << 16) / (tempo * division);
    MidiFramesPerTick = (44100 * secondspertick) >> 16;
}

void CoreAudioDrv_MIDI_Lock(void)
{
    if (!(Initialised & CASystem_midi)) {
        return;
    }
    
    pthread_mutex_lock(&midimutex);
}

void CoreAudioDrv_MIDI_Unlock(void)
{
    if (!(Initialised & CASystem_midi)) {
        return;
    }
    
    pthread_mutex_unlock(&midimutex);
}

