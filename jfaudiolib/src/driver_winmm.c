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
 * WinMM CD and MIDI output driver
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include "asssys.h"
#include "assmisc.h"
#include "midifuncs.h"
#include "driver_winmm.h"
#include "linklist.h"

enum {
   WinMMErr_Warning = -2,
   WinMMErr_Error   = -1,
   WinMMErr_Ok      = 0,
	WinMMErr_Uninitialised,
    WinMMErr_NotifyWindow,
    WinMMErr_CDMCIOpen,
    WinMMErr_CDMCISetTimeFormat,
    WinMMErr_CDMCIPlay,
    WinMMErr_MIDIStreamOpen,
    WinMMErr_MIDIStreamRestart,
    WinMMErr_MIDICreateEvent,
    WinMMErr_MIDIPlayThread,
    WinMMErr_MIDICreateMutex
};

static int ErrorCode = WinMMErr_Ok;

enum {
    UsedByNothing = 0,
    UsedByMIDI = 1,
    UsedByCD = 2
};

static HWND notifyWindow = 0;
static int notifyWindowClassRegistered = 0;
static int notifyWindowUsedBy = UsedByNothing;

static UINT cdDeviceID = 0;
static DWORD cdPausePosition = 0;
static int cdPaused = 0;
static int cdLoop = 0;
static int cdPlayTrack = 0;

static BOOL midiInstalled = FALSE;
static HMIDISTRM midiStream = 0;
static UINT midiDeviceID = MIDI_MAPPER;
static void (*midiThreadService)(void) = 0;
static unsigned int midiThreadTimer = 0;
static unsigned int midiLastEventTime = 0;
static unsigned int midiThreadQueueTimer = 0;
static unsigned int midiThreadQueueTicks = 0;
static HANDLE midiThread = 0;
static HANDLE midiThreadQuitEvent = 0;
static HANDLE midiMutex = 0;
static BOOL midiStreamRunning = FALSE;
static int midiLastDivision = 0;
#define THREAD_QUEUE_INTERVAL 10      // 1/10 sec
#define MIDI_BUFFER_SPACE   (12*128)  // 128 note-on events

typedef struct MidiBuffer {
    struct MidiBuffer *next;
    struct MidiBuffer *prev;

    BOOL prepared;
    MIDIHDR hdr;
} MidiBuffer;

static volatile MidiBuffer activeMidiBuffers;
static volatile MidiBuffer spareMidiBuffers;
static MidiBuffer *currentMidiBuffer = 0;


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




int WinMMDrv_GetError(void)
{
	return ErrorCode;
}

const char *WinMMDrv_ErrorString( int ErrorNumber )
{
	const char *ErrorString;
	
   switch( ErrorNumber )
	{
      case WinMMErr_Warning :
      case WinMMErr_Error :
         ErrorString = WinMMDrv_ErrorString( ErrorCode );
         break;
			
      case WinMMErr_Ok :
         ErrorString = "WinMM ok.";
         break;
			
		case WinMMErr_Uninitialised:
			ErrorString = "WinMM uninitialised.";
			break;
			
        case WinMMErr_NotifyWindow:
            ErrorString = "Failed creating notification window for CD/MIDI.";
            break;

        case WinMMErr_CDMCIOpen:
            ErrorString = "MCI error: failed opening CD audio device.";
            break;

        case WinMMErr_CDMCISetTimeFormat:
            ErrorString = "MCI error: failed setting time format for CD audio device.";
            break;

        case WinMMErr_CDMCIPlay:
            ErrorString = "MCI error: failed playing CD audio track.";
            break;

        case WinMMErr_MIDIStreamOpen:
            ErrorString = "MIDI error: failed opening stream.";
            break;

        case WinMMErr_MIDIStreamRestart:
            ErrorString = "MIDI error: failed starting stream.";
            break;

        case WinMMErr_MIDICreateEvent:
            ErrorString = "MIDI error: failed creating play thread quit event.";
            break;

        case WinMMErr_MIDIPlayThread:
            ErrorString = "MIDI error: failed creating play thread.";
            break;

        case WinMMErr_MIDICreateMutex:
            ErrorString = "MIDI error: failed creating play mutex.";
            break;

		default:
			ErrorString = "Unknown WinMM error code.";
			break;
	}
	
	return ErrorString;

}


static LRESULT CALLBACK notifyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case MM_MCINOTIFY:
            if (wParam == MCI_NOTIFY_SUCCESSFUL && lParam == cdDeviceID) {
                if (cdLoop && cdPlayTrack) {
                    WinMMDrv_CD_Play(cdPlayTrack, 1);
                }
            }
            break;
        default: break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static int openNotifyWindow(int useby)
{
    if (!notifyWindow) {
        if (!notifyWindowClassRegistered) {
            WNDCLASS wc;

            memset(&wc, 0, sizeof(wc));
            wc.lpfnWndProc = notifyWindowProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = "JFAudiolibNotifyWindow";

            if (!RegisterClass(&wc)) {
                return 0;
            }

            notifyWindowClassRegistered = 1;
        }

        notifyWindow = CreateWindow("JFAudiolibNotifyWindow", "", WS_POPUP,
                0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (!notifyWindow) {
            return 0;
        }
    }

    notifyWindowUsedBy |= useby;

    return 1;
}

static void closeNotifyWindow(int useby)
{
    notifyWindowUsedBy &= ~useby;

    if (!notifyWindowUsedBy && notifyWindow) {
        DestroyWindow(notifyWindow);
        notifyWindow = 0;
    }
}


int WinMMDrv_CD_Init(void)
{
    MCI_OPEN_PARMS mciopenparms;
    MCI_SET_PARMS mcisetparms;
    DWORD rv;

    WinMMDrv_CD_Shutdown();

    mciopenparms.lpstrDeviceType = "cdaudio";
    rv = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD_PTR) &mciopenparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Init MCI_OPEN err %d\n", (int) rv);
        ErrorCode = WinMMErr_CDMCIOpen;
        return WinMMErr_Error;
    }

    cdDeviceID = mciopenparms.wDeviceID;

    mcisetparms.dwTimeFormat = MCI_FORMAT_MSF;
    rv = mciSendCommand(cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR) &mcisetparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Init MCI_SET err %d\n", (int) rv);
        mciSendCommand(cdDeviceID, MCI_CLOSE, 0, 0);
        cdDeviceID = 0;

        ErrorCode = WinMMErr_CDMCISetTimeFormat;
        return WinMMErr_Error;
    }

    if (!openNotifyWindow(UsedByCD)) {
        mciSendCommand(cdDeviceID, MCI_CLOSE, 0, 0);
        cdDeviceID = 0;

        ErrorCode = WinMMErr_NotifyWindow;
        return WinMMErr_Error;
    }

    return WinMMErr_Ok;
}

void WinMMDrv_CD_Shutdown(void)
{
    if (cdDeviceID) {
        WinMMDrv_CD_Stop();

        mciSendCommand(cdDeviceID, MCI_CLOSE, 0, 0);
    }
    cdDeviceID = 0;

    closeNotifyWindow(UsedByCD);
}

int WinMMDrv_CD_Play(int track, int loop)
{
    MCI_PLAY_PARMS mciplayparms;
    MCI_STATUS_PARMS mcistatusparms;
    DWORD rv, start, end;
    BYTE minutes, seconds, frames;

    if (!cdDeviceID) {
        ErrorCode = WinMMErr_Uninitialised;
        return WinMMErr_Error;
    }

    cdPlayTrack = track;
    cdLoop = loop;
    cdPaused = 0;

    mcistatusparms.dwItem = MCI_STATUS_POSITION;
    mcistatusparms.dwTrack = track;
    rv = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_WAIT | MCI_TRACK | MCI_STATUS_ITEM, (DWORD_PTR) &mcistatusparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Play MCI_STATUS position err %d\n", (int) rv);
        ErrorCode = WinMMErr_CDMCIPlay;
        return WinMMErr_Error;
    }
    start = mcistatusparms.dwReturn;

    mcistatusparms.dwItem = MCI_STATUS_LENGTH;
    mcistatusparms.dwTrack = track;
    rv = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_WAIT | MCI_TRACK | MCI_STATUS_ITEM, (DWORD_PTR) &mcistatusparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Play MCI_STATUS length err %d\n", (int) rv);
        ErrorCode = WinMMErr_CDMCIPlay;
        return WinMMErr_Error;
    }
    end = mcistatusparms.dwReturn;

    minutes = MCI_MSF_MINUTE(start) + MCI_MSF_MINUTE(end);
    seconds = MCI_MSF_SECOND(start) + MCI_MSF_SECOND(end);
    frames = MCI_MSF_FRAME(start) + MCI_MSF_FRAME(end);
    while (frames >= 75) { seconds += 1; frames -= 75; }
    while (seconds >= 60) { minutes += 1; seconds -= 60; }

    mciplayparms.dwFrom = start;
    mciplayparms.dwTo = MCI_MAKE_MSF(minutes, seconds, frames);
    mciplayparms.dwCallback = (DWORD_PTR) notifyWindow;
    rv = mciSendCommand(cdDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR) &mciplayparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Play MCI_PLAY err %d\n", (int) rv);
        ErrorCode = WinMMErr_CDMCIPlay;
        return WinMMErr_Error;
    }

    return WinMMErr_Ok;
}

void WinMMDrv_CD_Stop(void)
{
    MCI_GENERIC_PARMS mcigenparms;
    DWORD rv;

    if (!cdDeviceID) {
        return;
    }

    cdPlayTrack = 0;
    cdLoop = 0;
    cdPaused = 0;

    rv = mciSendCommand(cdDeviceID, MCI_STOP, 0, (DWORD_PTR) &mcigenparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_Stop MCI_STOP err %d\n", (int) rv);
    }
}

void WinMMDrv_CD_Pause(int pauseon)
{
    if (!cdDeviceID) {
        return;
    }

    if (cdPaused == pauseon) {
        return;
    }

    if (pauseon) {
        MCI_STATUS_PARMS mcistatusparms;
        MCI_GENERIC_PARMS mcigenparms;
        DWORD rv;

        mcistatusparms.dwItem = MCI_STATUS_POSITION;
        rv = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR) &mcistatusparms);
        if (rv) {
            ASS_Message("WinMMDrv: CD_Pause MCI_STATUS err %d\n", (int) rv);
            return;
        }

        cdPausePosition = mcistatusparms.dwReturn;

        rv = mciSendCommand(cdDeviceID, MCI_STOP, 0, (DWORD_PTR) &mcigenparms);
        if (rv) {
            ASS_Message("WinMMDrv: CD_Pause MCI_STOP err %d\n", (int) rv);
        }
    } else {
        MCI_PLAY_PARMS mciplayparms;
        DWORD rv;

        mciplayparms.dwFrom = cdPausePosition;
        mciplayparms.dwTo   = MCI_MAKE_TMSF(cdPlayTrack + 1, 0, 0, 0);
        mciplayparms.dwCallback = (DWORD_PTR) notifyWindow;
        rv = mciSendCommand(cdDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR) &mciplayparms);
        if (rv) {
            ASS_Message("WinMMDrv: CD_Pause MCI_PLAY err %d\n", (int) rv);
            return;
        }

        cdPausePosition = 0;
    }

    cdPaused = pauseon;
}

int WinMMDrv_CD_IsPlaying(void)
{
    MCI_STATUS_PARMS mcistatusparms;
    DWORD rv;

    if (!cdDeviceID) {
        return 0;
    }

    mcistatusparms.dwItem = MCI_STATUS_MODE;
    rv = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD_PTR) &mcistatusparms);
    if (rv) {
        ASS_Message("WinMMDrv: CD_IsPlaying MCI_STATUS err %d\n", (int) rv);
        return 0;
    }

    return (mcistatusparms.dwReturn == MCI_MODE_PLAY);
}

void WinMMDrv_CD_SetVolume(int volume)
{
}



// will append "err nnn (ssss)\n" to the end of the string it emits
static const char *mmsyserr_str(MMRESULT rv)
{
    switch (rv) {
        case MMSYSERR_NOERROR: return "MMSYSERR_NOERROR";
        case MMSYSERR_BADDEVICEID: return "MMSYSERR_BADDEVICEID";
        case MMSYSERR_NOTENABLED: return "MMSYSERR_NOTENABLED";
        case MMSYSERR_ALLOCATED: return "MMSYSERR_ALLOCATED";
        case MMSYSERR_INVALHANDLE: return "MMSYSERR_INVALHANDLE";
        case MMSYSERR_NODRIVER: return "MMSYSERR_NODRIVER";
        case MMSYSERR_NOMEM: return "MMSYSERR_NOMEM";
        case MMSYSERR_NOTSUPPORTED: return "MMSYSERR_NOTSUPPORTED";
        case MMSYSERR_BADERRNUM: return "MMSYSERR_BADERRNUM";
        case MMSYSERR_INVALFLAG: return "MMSYSERR_INVALFLAG";
        case MMSYSERR_INVALPARAM: return "MMSYSERR_INVALPARAM";
        case MMSYSERR_HANDLEBUSY: return "MMSYSERR_HANDLEBUSY";
        case MMSYSERR_INVALIDALIAS: return "MMSYSERR_INVALIDALIAS";
        case MMSYSERR_BADDB: return "MMSYSERR_BADDB";
        case MMSYSERR_KEYNOTFOUND: return "MMSYSERR_KEYNOTFOUND";
        case MMSYSERR_READERROR: return "MMSYSERR_READERROR";
        case MMSYSERR_WRITEERROR: return "MMSYSERR_WRITEERROR";
        case MMSYSERR_DELETEERROR: return "MMSYSERR_DELETEERROR";
        case MMSYSERR_VALNOTFOUND: return "MMSYSERR_VALNOTFOUND";
        case MMSYSERR_NODRIVERCB: return "MMSYSERR_NODRIVERCB";
        default: return "?";
    }
}

#if _MSC_VER <= 1310
void midi_error(int rv, const char* fmt, ...)
{
    return;
}
#else
#define midi_error(rv, fmt, ...) { \
    ASS_Message(fmt, __VA_ARGS__); \
    ASS_Message(" err %d (%s)\n", (int)(rv), mmsyserr_str(rv)); \
}
#endif
static void midi_dispose_buffer(MidiBuffer * node, const char * caller)
{
    MMRESULT rv;
    
    if (node->prepared) {
        rv = midiOutUnprepareHeader( (HMIDIOUT) midiStream, &node->hdr, sizeof(MIDIHDR) );
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: %s/midi_dispose_buffer midiOutUnprepareHeader", caller);
        }
        node->prepared = FALSE;
    }

    if (midiThread) {
        // remove the node from the activeMidiBuffers list
        LL_Remove( node, next, prev );
    
        // when playing, we keep the buffers
        LL_Add( (MidiBuffer*) &spareMidiBuffers, node, next, prev );
        //ASS_Message("WinMMDrv: %s/midi_dispose_buffer recycling buffer %p\n", caller, node);
    } else {
        // when not, we throw them away
        free(node);
        //ASS_Message("WinMMDrv: %s/midi_dispose_buffer freeing buffer %p\n", caller, node);
    }
}

static void midi_gc_buffers(void)
{
    MidiBuffer *node, *next;

    for ( node = activeMidiBuffers.next; node != &activeMidiBuffers; node = next ) {
        next = node->next;

        if (node->hdr.dwFlags & MHDR_DONE) {
            midi_dispose_buffer(node, "midi_gc_buffers");
        }
    }
}

static void midi_free_buffers(void)
{
    MidiBuffer *node, *next;

    //ASS_Message("WinMMDrv: waiting for active buffers to return\n");
    while (!LL_ListEmpty(&activeMidiBuffers, next, prev)) {
        // wait for Windows to finish with all the buffers queued
        midi_gc_buffers();
        //ASS_Message("WinMMDrv: waiting...\n");
        Sleep(10);
    }
    //ASS_Message("WinMMDrv: waiting over\n");

    for ( node = spareMidiBuffers.next; node != &spareMidiBuffers; node = next ) {
        next = node->next;
        LL_Remove( node, next, prev );
        free(node);
        //ASS_Message("WinMMDrv: midi_free_buffers freeing buffer %p\n", node);
    }
    
    assert(currentMidiBuffer == 0);
}

static void midi_flush_current_buffer(void)
{
    MMRESULT rv;
    MIDIEVENT * evt;
    BOOL needsPrepare = FALSE;

    if (!currentMidiBuffer) {
        return;
    }

    evt = (MIDIEVENT *) currentMidiBuffer->hdr.lpData;

    if (!midiThread) {
        // immediate messages don't use a MIDIEVENT header so strip it off and
        // make some adjustments

        currentMidiBuffer->hdr.dwBufferLength = currentMidiBuffer->hdr.dwBytesRecorded - 12;
        currentMidiBuffer->hdr.dwBytesRecorded = 0;
        currentMidiBuffer->hdr.lpData = (LPSTR) &evt->dwParms[0];
        
        if (currentMidiBuffer->hdr.dwBufferLength > 0) {
            needsPrepare = TRUE;
        }
    } else {
        needsPrepare = TRUE;
    }
    
    if (needsPrepare) {
        // playing a file, or sending a sysex when not playing means
        // we need to prepare the buffer
        rv = midiOutPrepareHeader( (HMIDIOUT) midiStream, &currentMidiBuffer->hdr, sizeof(MIDIHDR) );
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: midi_flush_current_buffer midiOutPrepareHeader");
            return;
        }

        currentMidiBuffer->prepared = TRUE;
    }

    if (midiThread) {
        // midi file playing, so send events to the stream

        LL_Add( (MidiBuffer*) &activeMidiBuffers, currentMidiBuffer, next, prev );

        rv = midiStreamOut(midiStream, &currentMidiBuffer->hdr, sizeof(MIDIHDR));
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: midi_flush_current_buffer midiStreamOut");
            midi_dispose_buffer(currentMidiBuffer, "midi_flush_current_buffer");
            return;
        }

        //ASS_Message("WinMMDrv: midi_flush_current_buffer queued buffer %p\n", currentMidiBuffer);
    } else {
        // midi file not playing, so send immediately
        
        if (currentMidiBuffer->hdr.dwBufferLength > 0) {
            rv = midiOutLongMsg( (HMIDIOUT) midiStream, &currentMidiBuffer->hdr, sizeof(MIDIHDR) );
            if (rv == MMSYSERR_NOERROR) {
                // busy-wait for Windows to be done with it
                while (!(currentMidiBuffer->hdr.dwFlags & MHDR_DONE)) ;
                
                //ASS_Message("WinMMDrv: midi_flush_current_buffer sent immediate long\n");
            } else {
                midi_error(rv, "WinMMDrv: midi_flush_current_buffer midiOutLongMsg");
            }
        } else {
            rv = midiOutShortMsg( (HMIDIOUT) midiStream, evt->dwEvent );
            if (rv == MMSYSERR_NOERROR) {
                //ASS_Message("WinMMDrv: midi_flush_current_buffer sent immediate short\n");
            } else {
                midi_error(rv, "WinMMDrv: midi_flush_current_buffer midiOutShortMsg");
            }
        }

        midi_dispose_buffer(currentMidiBuffer, "midi_flush_current_buffer");
    }
    
    currentMidiBuffer = 0;
}

static void midi_setup_event(int length, unsigned char ** data)
{
    MIDIEVENT * evt;

    evt = (MIDIEVENT *) ((intptr_t) currentMidiBuffer->hdr.lpData +
            currentMidiBuffer->hdr.dwBytesRecorded);

    evt->dwDeltaTime = midiThread ? (midiThreadTimer - midiLastEventTime) : 0;
    evt->dwStreamID = 0;

    if (length <= 3) {
        evt->dwEvent = (DWORD)MEVT_SHORTMSG << 24;
        *data = (unsigned char *) &evt->dwEvent;
    } else {
        evt->dwEvent = ((DWORD)MEVT_LONGMSG << 24) | (length & 0x00ffffff);
        *data = (unsigned char *) &evt->dwParms[0];
    }
}

/* Gets space in the buffer presently being filled.
   If insufficient space can be found in the buffer,
   what is there is flushed to the stream and a new
   buffer large enough is allocated.
   
   Returns a pointer to starting writing at in 'data'.
 */
static BOOL midi_get_buffer(int length, unsigned char ** data)
{
    int datalen;
    MidiBuffer * node;
    
    // determine the space to alloc.
    // the size of a MIDIEVENT is 3*sizeof(DWORD) = 12.
    // short messages need only that amount of space.
    // long messages need additional space equal to the length of
    //    the message, padded to 4 bytes
    
    if (length <= 3) {
        datalen = 12;
    } else {
        datalen = 12 + length;
        if ((datalen & 3) > 0) {
            datalen += 4 - (datalen & 3);
        }
    }
    
    if (!midiThread) {
        assert(currentMidiBuffer == 0);
    }
    
    if (currentMidiBuffer && (currentMidiBuffer->hdr.dwBufferLength -
            currentMidiBuffer->hdr.dwBytesRecorded) >= datalen) {
        // there was enough space in the current buffer, so hand that back
        midi_setup_event(length, data);
        
        currentMidiBuffer->hdr.dwBytesRecorded += datalen;
        
        return TRUE;
    }
    
    if (currentMidiBuffer) {
        // not enough space in the current buffer to accommodate the
        // new data, so flush it to the stream
        midi_flush_current_buffer();
        currentMidiBuffer = 0;
    }
    
    // check if there's a spare buffer big enough to hold the message
    if (midiThread) {
        for ( node = spareMidiBuffers.next; node != &spareMidiBuffers; node = node->next ) {
            if (node->hdr.dwBufferLength >= datalen) {
                // yes!
                LL_Remove( node, next, prev );
                
                node->hdr.dwBytesRecorded = 0;
                memset(node->hdr.lpData, 0, node->hdr.dwBufferLength);
                
                currentMidiBuffer = node;
                
                //ASS_Message("WinMMDrv: midi_get_buffer fetched buffer %p\n", node);
                break;
            }
        }
    }
    
    if (!currentMidiBuffer) {
        // there were no spare buffers, or none were big enough, so
        // allocate a new one
        int size;
        
        if (midiThread) {
            // playing a file, so allocate a buffer for more than
            // one event
            size = max(MIDI_BUFFER_SPACE, datalen);
        } else {
            // not playing a file, so allocate just a buffer for
            // the event we'll be sending immediately
            size = datalen;
        }
        
        node = (MidiBuffer *) malloc( sizeof(MidiBuffer) + size );
        if (node == 0) {
            return FALSE;
        }

        memset(node, 0, sizeof(MidiBuffer) + datalen);
        node->hdr.dwUser = (DWORD_PTR) node;
        node->hdr.lpData = (LPSTR) ((intptr_t)node + sizeof(MidiBuffer));
        node->hdr.dwBufferLength = size;
        node->hdr.dwBytesRecorded = 0;
        
        currentMidiBuffer = node;
        
        //ASS_Message("WinMMDrv: midi_get_buffer allocated buffer %p\n", node);
    }

    midi_setup_event(length, data);

    currentMidiBuffer->hdr.dwBytesRecorded += datalen;
    
    return TRUE;
}

static inline void midi_sequence_event(void)
{
    if (!midiThread) {
        // a midi event being sent out of playback (streaming) mode
        midi_flush_current_buffer();
        return;
    }
    
    //ASS_Message("WinMMDrv: midi_sequence_event buffered\n");

    // update the delta time counter
    midiLastEventTime = midiThreadTimer;
}

static void Func_NoteOff( int channel, int key, int velocity )
{
    unsigned char * data;

    if (midi_get_buffer(3, &data)) {
        data[0] = MIDI_NOTE_OFF | channel;
        data[1] = key;
        data[2] = velocity;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: note off error\n");
}

static void Func_NoteOn( int channel, int key, int velocity )
{
    unsigned char * data;

    if (midi_get_buffer(3, &data)) {
        data[0] = MIDI_NOTE_ON | channel;
        data[1] = key;
        data[2] = velocity;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: note on error\n");
}

static void Func_PolyAftertouch( int channel, int key, int pressure )
{
    unsigned char * data;

    if (midi_get_buffer(3, &data)) {
        data[0] = MIDI_POLY_AFTER_TCH | channel;
        data[1] = key;
        data[2] = pressure;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: poly aftertouch error\n");
}

static void Func_ControlChange( int channel, int number, int value )
{
    unsigned char * data;

    if (midi_get_buffer(3, &data)) {
        data[0] = MIDI_CONTROL_CHANGE | channel;
        data[1] = number;
        data[2] = value;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: control change error\n");
}

static void Func_ProgramChange( int channel, int program )
{
    unsigned char * data;

    if (midi_get_buffer(2, &data)) {
        data[0] = MIDI_PROGRAM_CHANGE | channel;
        data[1] = program;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: program change error\n");
}

static void Func_ChannelAftertouch( int channel, int pressure )
{
    unsigned char * data;

    if (midi_get_buffer(2, &data)) {
        data[0] = MIDI_AFTER_TOUCH | channel;
        data[1] = pressure;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: channel aftertouch error\n");
}

static void Func_PitchBend( int channel, int lsb, int msb )
{
    unsigned char * data;

    if (midi_get_buffer(3, &data)) {
        data[0] = MIDI_PITCH_BEND | channel;
        data[1] = lsb;
        data[2] = msb;
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: pitch bend error\n");
}

static void Func_SysEx( const unsigned char * data, int length )
{
    unsigned char * wdata;
    
    if (midi_get_buffer(length, &wdata)) {
        memcpy(wdata, data, length);
        midi_sequence_event();
    } else ASS_Message("WinMMDrv: sysex error\n");
}

int WinMMDrv_MIDI_Init(midifuncs * funcs, const char *params)
{
    MMRESULT rv;

    if (midiInstalled) {
        WinMMDrv_MIDI_Shutdown();
    }

    memset(funcs, 0, sizeof(midifuncs));

    LL_Reset( (MidiBuffer*) &activeMidiBuffers, next, prev );
    LL_Reset( (MidiBuffer*) &spareMidiBuffers, next, prev );

    midiMutex = CreateMutex(0, FALSE, 0);
    if (!midiMutex) {
        ErrorCode = WinMMErr_MIDICreateMutex;
        return WinMMErr_Error;
    }

    rv = midiStreamOpen(&midiStream, &midiDeviceID, 1, (DWORD_PTR) 0, (DWORD_PTR) 0, CALLBACK_NULL);
    if (rv != MMSYSERR_NOERROR) {
        CloseHandle(midiMutex);
        midiMutex = 0;

        midi_error(rv, "WinMMDrv: MIDI_Init midiStreamOpen");
        ErrorCode = WinMMErr_MIDIStreamOpen;
        return WinMMErr_Error;
    }
    
    funcs->NoteOff = Func_NoteOff;
    funcs->NoteOn  = Func_NoteOn;
    funcs->PolyAftertouch = Func_PolyAftertouch;
    funcs->ControlChange = Func_ControlChange;
    funcs->ProgramChange = Func_ProgramChange;
    funcs->ChannelAftertouch = Func_ChannelAftertouch;
    funcs->PitchBend = Func_PitchBend;
    funcs->SysEx = Func_SysEx;

    midiInstalled = TRUE;
    
    return WinMMErr_Ok;
}

void WinMMDrv_MIDI_Shutdown(void)
{
    MMRESULT rv;

    if (!midiInstalled) {
        return;
    }

    WinMMDrv_MIDI_HaltPlayback();

    if (midiStream) {
        rv = midiStreamClose(midiStream);
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: MIDI_Shutdown midiStreamClose");
        }
    }

    if (midiMutex) {
        CloseHandle(midiMutex);
    }

    midiStream = 0;
    midiMutex = 0;

    midiInstalled = FALSE;
}

static DWORD midi_get_tick(void)
{
    MMRESULT rv;
    MMTIME mmtime;

    mmtime.wType = TIME_TICKS;

    rv = midiStreamPosition(midiStream, &mmtime, sizeof(MMTIME));
    if (rv != MMSYSERR_NOERROR) {
        midi_error(rv, "WinMMDrv: midi_get_tick midiStreamPosition");
        return 0;
    }

    return mmtime.u.ticks;
}

static DWORD WINAPI midiDataThread(LPVOID lpParameter)
{
    DWORD waitret;
    DWORD sequenceTime;
    DWORD sleepAmount = 100 / THREAD_QUEUE_INTERVAL;
    
    ASS_Message("WinMMDrv: midiDataThread started\n");

    midiThreadTimer = midi_get_tick();
    midiLastEventTime = midiThreadTimer;
    midiThreadQueueTimer = midiThreadTimer + midiThreadQueueTicks;

    WinMMDrv_MIDI_Lock();
    midi_gc_buffers();
    while (midiThreadTimer < midiThreadQueueTimer) {
        if (midiThreadService) {
            midiThreadService();
        }
        midiThreadTimer++;
    }
    midi_flush_current_buffer();
    WinMMDrv_MIDI_Unlock();

    do {
        waitret = WaitForSingleObject(midiThreadQuitEvent, sleepAmount);
        if (waitret == WAIT_OBJECT_0) {
            ASS_Message("WinMMDrv: midiDataThread exiting\n");
            break;
        } else if (waitret == WAIT_TIMEOUT) {
            // queue a tick
            sequenceTime = midi_get_tick();

            sleepAmount = 100 / THREAD_QUEUE_INTERVAL;
            if ((int)(midiThreadTimer - sequenceTime) > midiThreadQueueTicks) {
                // we're running ahead, so sleep for half the usual
                // amount and try again
                sleepAmount /= 2;
                continue;
            }

            midiThreadQueueTimer = sequenceTime + midiThreadQueueTicks;

            WinMMDrv_MIDI_Lock();
            midi_gc_buffers();
            while (midiThreadTimer < midiThreadQueueTimer) {
                if (midiThreadService) {
                    midiThreadService();
                }
                midiThreadTimer++;
            }
            midi_flush_current_buffer();
            WinMMDrv_MIDI_Unlock();

        } else {
            ASS_Message("WinMMDrv: midiDataThread wfmo err %d\n", (int) waitret);
        }
    } while (1);

    return 0;
}

int WinMMDrv_MIDI_StartPlayback(void (*service)(void))
{
    DWORD thID;
	MMRESULT rv;

    WinMMDrv_MIDI_HaltPlayback();

    midiThreadService = service;

    midiThreadQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!midiThreadQuitEvent) {
        ErrorCode = WinMMErr_MIDICreateEvent;
        return WinMMErr_Error;
    }

    if (!midiStreamRunning) {
        rv = midiStreamRestart(midiStream);
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "MIDI_StartPlayback midiStreamRestart");
            WinMMDrv_MIDI_HaltPlayback();
            ErrorCode = WinMMErr_MIDIStreamRestart;
            return WinMMErr_Error;
        }

        midiStreamRunning = TRUE;
    }

    midiThread = CreateThread(NULL, 0, midiDataThread, 0, 0, &thID);
    if (!midiThread) {
        WinMMDrv_MIDI_HaltPlayback();
        ErrorCode = WinMMErr_MIDIPlayThread;
        return WinMMErr_Error;
    }

    return WinMMErr_Ok;
}

void WinMMDrv_MIDI_HaltPlayback(void)
{
    MMRESULT rv;
    
    if (midiThread) {
        SetEvent(midiThreadQuitEvent);

        WaitForSingleObject(midiThread, INFINITE);
        ASS_Message("WinMMDrv: MIDI_HaltPlayback synched\n");

        CloseHandle(midiThread);
    }

    if (midiThreadQuitEvent) {
        CloseHandle(midiThreadQuitEvent);
    }
    
    if (midiStreamRunning) {
        ASS_Message("WinMMDrv: stopping stream\n");
        rv = midiStreamStop(midiStream);
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: MIDI_HaltPlayback midiStreamStop");
        }
        ASS_Message("WinMMDrv: stream stopped\n");
    
        midiStreamRunning = FALSE;
    }

    midi_free_buffers();
    
    midiThread = 0;
    midiThreadQuitEvent = 0;
}

void WinMMDrv_MIDI_SetTempo(int tempo, int division)
{
    MMRESULT rv;
    MIDIPROPTEMPO propTempo;
    MIDIPROPTIMEDIV propTimediv;
    BOOL running = midiStreamRunning;

    //ASS_Message("WinMMDrv: MIDI_SetTempo %d/%d\n", tempo, division);

    propTempo.cbStruct = sizeof(MIDIPROPTEMPO);
    propTempo.dwTempo = 60000000l / tempo;
    propTimediv.cbStruct = sizeof(MIDIPROPTIMEDIV);
    propTimediv.dwTimeDiv = division;

    if (midiLastDivision != division) {
        // changing the division means halting the stream
        WinMMDrv_MIDI_HaltPlayback();

        rv = midiStreamProperty(midiStream, (LPBYTE) &propTimediv, MIDIPROP_SET | MIDIPROP_TIMEDIV);
        if (rv != MMSYSERR_NOERROR) {
            midi_error(rv, "WinMMDrv: MIDI_SetTempo midiStreamProperty timediv");
        }
    }

    rv = midiStreamProperty(midiStream, (LPBYTE) &propTempo, MIDIPROP_SET | MIDIPROP_TEMPO);
    if (rv != MMSYSERR_NOERROR) {
        midi_error(rv, "WinMMDrv: MIDI_SetTempo midiStreamProperty tempo");
    }

    if (midiLastDivision != division) {
        if (running && WinMMDrv_MIDI_StartPlayback(midiThreadService) != WinMMErr_Ok) {
            return;
        }

        midiLastDivision = division;
    }

    midiThreadQueueTicks = (int) ceil( ( ( (double) tempo * (double) division ) / 60.0 ) /
            (double) THREAD_QUEUE_INTERVAL );
    if (midiThreadQueueTicks <= 0) {
        midiThreadQueueTicks = 1;
    }
}

void WinMMDrv_MIDI_Lock(void)
{
    DWORD err;

    err = WaitForSingleObject(midiMutex, INFINITE);
    if (err != WAIT_OBJECT_0) {
        ASS_Message("WinMMDrv: midiMutex lock: wfso %d\n", (int) err);
    }
}

void WinMMDrv_MIDI_Unlock(void)
{
    ReleaseMutex(midiMutex);
}

// vim:ts=4:sw=4:expandtab:
