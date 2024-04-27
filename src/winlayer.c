// Windows interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)
//
// This is all very ugly.

#ifndef _WIN32
#error winlayer.c is for Windows only.
#endif

#define WIN32_LEAN_AND_MEAN
#if SUBSYS == 400 || SUBSYS == 401
	#define WINVER 0x0400
	#if SUBSYS == 401
		// must define NT for mouse wheel support in Win98?
		#if MOUSETYPE == 0
			#define _WIN32_WINNT 0x0400
		#endif
	#endif
#elif SUBSYS == 501
	#define WINVER 0x0501
	#define _WIN32_WINNT 0x0501
#else
	#define WINVER 0x0600
	#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>

#if _MSC_VER <= 1400
	#ifdef DEBUGGINGAIDS
		#if SUBSYS < 501
			// this is for IsDebuggerPresent()
			#ifndef COMPILE_NEWAPIS_STUBS
				#define COMPILE_NEWAPIS_STUBS
				#define WANT_ISDEBUGGERPRESENT_WRAPPER
				#include <newapis.h>
			#endif
		#endif
	#endif
#endif

#if HAVE_JOYSTICK_SUPP == 1
#include <xinput.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <commdlg.h>

#include "build.h"

#if USE_OPENGL
#include "glbuild.h"
#include "wglext.h"
#endif


#include "winlayer.h"
#include "pragmas.h"
#include "a.h"
#include "osd.h"

#if MOUSETYPE == 1
	#define DIRECTINPUT_VERSION 0x0700
	#include "..\dxsdk\include\dinput.h"
	
	LPDIRECTINPUT  g_lpDI;
	LPDIRECTINPUTDEVICE g_pMouse;
	DIPROPDWORD dipdw;

	#define iDirectInputCreate(a,b,c,d)	pDirectInputCreate(a,b,c,d)
	HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);
#endif

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

int   _buildargc = 0;
const char **_buildargv = NULL;
static char *argvbuf = NULL;

// Windows crud
static HINSTANCE hInstance = 0;
static HWND hWindow = 0;
static HDC hDCWindow = NULL;
#define WINDOW_CLASS "xmapedit"
static BOOL window_class_registered = FALSE;
static HANDLE instanceflag = NULL;

int    backgroundidle = 1;
static char apptitle[256] = "Build Engine";
static char wintitle[256] = "";

static WORD sysgamma[3][256];
extern int gammabrightness;
extern float curgamma;

#if USE_OPENGL
// OpenGL stuff
static HGLRC hGLRC = 0;
static HANDLE hGLDLL;
static glbuild8bit gl8bit;
static unsigned char nogl=0;
static unsigned char *frame = NULL;

static HWND hGLWindow = NULL;
static HWND dummyhGLwindow = NULL;
static HDC hDCGLWindow = NULL;

static struct winlayer_glfuncs {
	HGLRC (WINAPI * wglCreateContext)(HDC);
	BOOL (WINAPI * wglDeleteContext)(HGLRC);
	PROC (WINAPI * wglGetProcAddress)(LPCSTR);
	BOOL (WINAPI * wglMakeCurrent)(HDC,HGLRC);
	BOOL (WINAPI * wglSwapBuffers)(HDC);

	const char * (WINAPI * wglGetExtensionsStringARB)(HDC hdc);
	BOOL (WINAPI * wglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
	HGLRC (WINAPI * wglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int *attribList);
	BOOL (WINAPI * wglSwapIntervalEXT)(int interval);
	int (WINAPI * wglGetSwapIntervalEXT)(void);

	int have_ARB_create_context_profile;
} wglfunc;
#endif

static LPTSTR GetWindowsErrorMsg(DWORD code);
static const char * getwindowserrorstr(DWORD code);
static void ShowErrorBox(const char *m);
static BOOL CheckWinVersion(void);
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void fetchkeynames(void);
static void updatemouse(void);
static void updatejoystick(void);
static void UninitDIB(void);
static int SetupDIB(int width, int height);
static void UninitOpenGL(void);
static int SetupOpenGL(int width, int height, int bitspp, int cover);
static BOOL RegisterWindowClass(void);
static BOOL CreateAppWindow(int width, int height, int bitspp, int fs, int refresh);
static void DestroyAppWindow(void);
static void UpdateAppWindowTitle(void);
void SetNumLock( BOOL bState );
static void shutdownvideo(void);

RECT  windowrct;
POINT windowpos, windowcen;

// video
static int desktopxdim=0,desktopydim=0,desktopbpp=0, desktopmodeset=0;
int xres=-1, yres=-1, fullscreen=0, bpp=0, bytesperline=0, imageSize=0;
intptr_t frameplace=0;

static unsigned int modeschecked = 0, maxrefreshfreq = 60;
char modechange = 1, offscreenrendering = 0, videomodereset = 0;
int glswapinterval = 1, glcolourdepth=32;

// input and events
int inputdevices=0;
char quitevent=0, appactive =1;
int mousex=0, mousey=0, mouseb=0;
static unsigned int mousewheel[2] = { 0,0 };
#define MouseWheelFakePressTime (100)	// getticks() is a 1000Hz timer, and the button press is faked for 100ms
int joyaxis[8], joyb=0;
char joynumaxes=0, joynumbuttons=0;

static char taskswitching=1;

char keystatus[256];
int keyfifo[KEYFIFOSIZ];
unsigned char keyasciififo[KEYFIFOSIZ];
int keyfifoplc, keyfifoend;
int keyasciififoplc, keyasciififoend;
static char keynames[256][24];
static const int wscantable[256], wxscantable[256];


static unsigned int mouseclock = 0;
static char moustat = 0, mousegrab = 0;
static int joyblast=0;

static int xinputusernum = -1;

void (*keypresscallback)(int,int) = 0;
void (*mousepresscallback)(int,int) = 0;
void (*joypresscallback)(int,int) = 0;




//-------------------------------------------------------------------------------------------------
//  MAIN CRAP
//=================================================================================================


//
// win_gethwnd() -- gets the window handle
//
intptr_t win_gethwnd(void)
{
	return (intptr_t)hWindow;
}


//
// win_gethinstance() -- gets the application instance
//
intptr_t win_gethinstance(void)
{
	return (intptr_t)hInstance;
}


//
// win_allowtaskswitching() -- captures/releases alt+tab hotkeys
//
void win_allowtaskswitching(int onf)
{
	if (onf == taskswitching) return;

	if (onf) {
		UnregisterHotKey(0,0);
		UnregisterHotKey(0,1);
	} else {
		RegisterHotKey(0,0,MOD_ALT,VK_TAB);
		RegisterHotKey(0,1,MOD_ALT|MOD_SHIFT,VK_TAB);
	}

	taskswitching = onf;
}


//
// win_checkinstance() -- looks for another instance of a Build app
//
int win_checkinstance(void)
{
	if (!instanceflag) return 0;
	return (WaitForSingleObject(instanceflag,0) == WAIT_TIMEOUT);
}


//
// wm_msgbox/wm_ynbox() -- window-manager-provided message boxes
//
int wm_msgbox(const char *name, const char *fmt, ...)
{
	char buf[1000];
	va_list va;

	va_start(va,fmt);
	vsprintf(buf,fmt,va);
	va_end(va);

	MessageBox(hWindow,buf,name,MB_OK|MB_TASKMODAL);
	return 0;
}
int wm_ynbox(const char *name, const char *fmt, ...)
{
	char buf[1000];
	va_list va;
	int r;

	va_start(va,fmt);
	vsprintf(buf,fmt,va);
	va_end(va);

	r = MessageBox((HWND)win_gethwnd(),buf,name,MB_YESNO|MB_TASKMODAL);
	if (r==IDYES) return 1;
	return 0;
}

//
// wm_filechooser() -- display a file selector dialogue box
//
int wm_filechooser(const char *initialdir, const char *initialfile, const char *type, int foropen, char **choice)
{
	#if 0
	OPENFILENAME ofn;
	char filter[100], *filterp = filter;
	char filename[BMAX_PATH+1] = "";

	*choice = NULL;

	if (!foropen && initialfile) {
		strcpy(filename, initialfile);
	}

	// ext Files\0*.ext\0\0
	memset(filter, 0, sizeof(filter));
	sprintf(filterp, "%s Files", type);
	filterp += strlen(filterp) + 1;
	sprintf(filterp, "*.%s", type);

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrInitialDir = initialdir;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = type;

	if (foropen ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn)) {
		*choice = strdup(filename);
		return 1;
	} else {
		return 0;
	}
	#else
		return 0;
	#endif
}

//
// wm_setapptitle() -- changes the application title
//
void wm_setapptitle(const char *name)
{
	if (name) {
		Bstrncpy(apptitle, name, sizeof(apptitle)-1);
		apptitle[ sizeof(apptitle)-1 ] = 0;
	}

	UpdateAppWindowTitle();
	
	#ifdef HAVE_START_WINDOW
	startwin_settitle(apptitle);
	#endif
}

//
// wm_setwindowtitle() -- changes the rendering window title
//
void wm_setwindowtitle(const char *name)
{
	if (name) {
		Bstrncpy(wintitle, name, sizeof(wintitle)-1);
		wintitle[ sizeof(wintitle)-1 ] = 0;
	}

	UpdateAppWindowTitle();
}

//
// SignalHandler() -- called when we've sprung a leak
//
static void SignalHandler(int signum)
{
	switch (signum) {
		case SIGSEGV:
			buildputs("Fatal Signal caught: SIGSEGV. Bailing out.\n");
			uninitsystem();
			if (stdout) fclose(stdout);
			break;
		default:
			break;
	}
}

char* osNames[] = {
	
	"95",
	"98",
	"XP",
	"Vista",
	
};

//
// WinMain() -- main Windows entry point
//
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	int r;
	char *argp;
	FILE *fp;
	HDC hdc;
	hInstance = hInst;
	
	if (CheckWinVersion() || hPrevInst)
	{
		char tmp[16];
		#if SUBSYS <= 400
			r = 0;
		#elif SUBSYS <= 401
			r = 1;
		#elif SUBSYS <= 501
			r = 2;
		#else
			r = 3;
		#endif
		
		sprintf(tmp, "This application designed for Microsoft Windows %s or newer.", osNames[r]);
		MessageBox(0, tmp, apptitle, MB_OK|MB_ICONSTOP);
		return -1;
	}
	

	hdc = GetDC(NULL);
	r = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL, hdc);
	if (r < 8)
	{
		MessageBox(0, "This application requires a desktop colour depth of 256-colours or more.",
			apptitle, MB_OK|MB_ICONSTOP);
		return -1;
	}
	
	// carve up the commandline into more recognizable pieces
	argvbuf = strdup(GetCommandLine());
	_buildargc = 0;
	if (argvbuf) {
		char quoted = 0, instring = 0, swallownext = 0;
		char *p,*wp; int i;
		for (p=wp=argvbuf; *p; p++) {
			if (*p == ' ') {
				if (instring && !quoted) {
					// end of a string
					*(wp++) = 0;
					instring = 0;
				} else if (instring) {
					*(wp++) = *p;
				}
			} else if (*p == '"' && !swallownext) {
				if (instring && quoted) {
					// end of a string
					if (p[1] == ' ') {
						*(wp++) = 0;
						instring = 0;
						quoted = 0;
					} else {
						quoted = 0;
					}
				} else if (instring && !quoted) {
					quoted = 1;
				} else if (!instring) {
					instring = 1;
					quoted = 1;
					_buildargc++;
				}
			} else if (*p == '\\' && p[1] == '"' && !swallownext) {
				swallownext = 1;
			} else {
				if (!instring) _buildargc++;
				instring = 1;
				*(wp++) = *p;
				swallownext = 0;
			}
		}
		*wp = 0;

		_buildargv = malloc(sizeof(char*)*_buildargc);
		wp = argvbuf;
		for (i=0; i<_buildargc; i++,wp++) {
			_buildargv[i] = wp;
			while (*wp) wp++;
		}
	}

	// pipe standard outputs to files
	if ((argp = Bgetenv("BUILD_LOGSTDOUT")) != NULL)
		if (!Bstrcasecmp(argp, "TRUE")) {
			fp = freopen("stdout.txt", "w", stdout);
			if (!fp) {
				fp = fopen("stdout.txt", "w");
			}
			if (fp) setvbuf(fp, 0, _IONBF, 0);
			*stdout = *fp;
			*stderr = *fp;
		}

	// install signal handlers
	signal(SIGSEGV, SignalHandler);

	if (RegisterWindowClass())
		return -1;

	atexit(uninitsystem);

	instanceflag = CreateSemaphore(NULL, 1,1, WINDOW_CLASS);

	#ifdef HAVE_START_WINDOW
	startwin_open();
	#endif
	
	baselayer_init();
	r = app_main(_buildargc, _buildargv);
	fclose(stdout);
	
	#ifdef HAVE_START_WINDOW
	startwin_close();
	#endif
	
	if (instanceflag) CloseHandle(instanceflag);
	if (argvbuf) free(argvbuf);
	return r;
}


static int set_maxrefreshfreq(const osdfuncparm_t *parm)
{
	int freq;
	if (parm->numparms == 0) {
		if (maxrefreshfreq == 0)
			buildputs("maxrefreshfreq = No maximum\n");
		else
			buildprintf("maxrefreshfreq = %d Hz\n",maxrefreshfreq);
		return OSDCMD_OK;
	}
	if (parm->numparms != 1) return OSDCMD_SHOWHELP;

	freq = Batol(parm->parms[0]);
	if (freq < 0) return OSDCMD_SHOWHELP;

	maxrefreshfreq = (unsigned)freq;
	modeschecked = 0;

	return OSDCMD_OK;
}

#if USE_OPENGL
static int set_glswapinterval(const osdfuncparm_t *parm)
{
	int interval;

	if (!wglfunc.wglSwapIntervalEXT || nogl) {
		buildputs("glswapinterval is not adjustable\n");
		return OSDCMD_OK;
	}
	if (parm->numparms == 0) {
		buildprintf("glswapinterval = %d\n", glswapinterval);
		return OSDCMD_OK;
	}
	if (parm->numparms != 1) return OSDCMD_SHOWHELP;

	interval = Batol(parm->parms[0]);
	if (interval < 0 || interval > 2) return OSDCMD_SHOWHELP;

	glswapinterval = interval;
	wglfunc.wglSwapIntervalEXT(interval);

	return OSDCMD_OK;
}
#endif

//
// initsystem() -- init systems
//
int initsystem(void)
{
	DEVMODE desktopmode;

	buildputs("Initialising Windows system interface\n");

	// get the desktop dimensions before anything changes them
	ZeroMemory(&desktopmode, sizeof(DEVMODE));
	desktopmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&desktopmode);

	desktopxdim = desktopmode.dmPelsWidth;
	desktopydim = desktopmode.dmPelsHeight;
	desktopbpp  = desktopmode.dmBitsPerPel;

	memset(curpalette, 0, sizeof(palette_t) * 256);

	atexit(uninitsystem);

	frameplace=0;

#if USE_OPENGL
	memset(&wglfunc, 0, sizeof(wglfunc));
	nogl = loadgldriver(getenv("BUILD_GLDRV"));
	if (!nogl) {
		// Load the core WGL functions.
		wglfunc.wglGetProcAddress = getglprocaddress("wglGetProcAddress", 0);
		wglfunc.wglCreateContext  = getglprocaddress("wglCreateContext", 0);
		wglfunc.wglDeleteContext  = getglprocaddress("wglDeleteContext", 0);
		wglfunc.wglMakeCurrent    = getglprocaddress("wglMakeCurrent", 0);
		wglfunc.wglSwapBuffers    = getglprocaddress("wglSwapBuffers", 0);
		nogl = !wglfunc.wglGetProcAddress ||
		 	   !wglfunc.wglCreateContext ||
		 	   !wglfunc.wglDeleteContext ||
		 	   !wglfunc.wglMakeCurrent ||
		 	   !wglfunc.wglSwapBuffers;
	}
	if (nogl) {
		buildputs("Failed loading OpenGL driver. GL modes will be unavailable.\n");
		memset(&wglfunc, 0, sizeof(wglfunc));
	} else {
		OSD_RegisterFunction("glswapinterval", "glswapinterval: frame swap interval for OpenGL modes (0 = no vsync, max 2)", set_glswapinterval);
	}
#endif

	OSD_RegisterFunction("maxrefreshfreq", "maxrefreshfreq: maximum display frequency to set for OpenGL Polymost modes (0=no maximum)", set_maxrefreshfreq);
	return 0;
}


//
// uninitsystem() -- uninit systems
//
void uninitsystem(void)
{
	DestroyAppWindow();
	
	#ifdef HAVE_START_WINDOW
	startwin_close();
	#endif

	uninitinput();
	uninittimer();

	win_allowtaskswitching(1);

	shutdownvideo();
#if USE_OPENGL
	glbuild_unloadfunctions();
	memset(&wglfunc, 0, sizeof(wglfunc));
	unloadgldriver();
#endif
}


//
// initputs() -- prints a string to the intitialization window
//
void initputs(const char *buf)
{
	#ifdef HAVE_START_WINDOW
	startwin_puts(buf);
	#endif
}


//
// debugprintf() -- sends a debug string to the debugger
//
void debugprintf(const char *f, ...)
{
#ifdef DEBUGGINGAIDS
	va_list va;
	char buf[1024];

	va_start(va,f);
	Bvsnprintf(buf, 1024, f, va);
	va_end(va);
	
	#if SUBSYS > 400
		if (IsDebuggerPresent())
		{
			OutputDebugString(buf);
		}
		else
		{
			fputs(buf, stdout);
		}
	#else
		fputs(buf, stdout);
	#endif
#endif
}


//
// handleevents() -- process the Windows message queue
//   returns !0 if there was an important event worth checking (like quitting)
//
static int eatosdinput = 0;
int handleevents(void)
{
	int rv=0;
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			quitevent = 1;

		#ifdef HAVE_START_WINDOW
		if (startwin_idle((void*)&msg) > 0) continue;
		#endif
		
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	if (quitevent)
		rv = -1;
	
	if (appactive)
	{
		eatosdinput = 0;
		updatemouse();
		updatejoystick();
	}
	else
	{
		rv = -1;
	}

	sampletimer();
	return rv;
}

//-------------------------------------------------------------------------------------------------
//  TIMER
//=================================================================================================

static int64_t timerfreq=0;
static int timerlastsample=0;
static int timerticspersec=0;
static void (*usertimercallback)(void) = NULL;

//  This timer stuff is all Ken's idea.

//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void (*installusertimercallback(void (*callback)(void)))(void)
{
	void (*oldtimercallback)(void);

	oldtimercallback = usertimercallback;
	usertimercallback = callback;

	return oldtimercallback;
}


//
// inittimer() -- initialise timer
//
int inittimer(int tickspersecond)
{
	int64_t t;

	if (timerfreq) return 0;	// already installed

	buildputs("Initialising timer\n");

	// OpenWatcom seems to want us to query the value into a local variable
	// instead of the global 'timerfreq' or else it gets pissed with an
	// access violation
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&t)) {
		ShowErrorBox("Failed fetching timer frequency");
		return -1;
	}
	timerfreq = t;
	timerticspersec = tickspersecond;
	QueryPerformanceCounter((LARGE_INTEGER*)&t);
	timerlastsample = (int)(t*timerticspersec / timerfreq);

	usertimercallback = NULL;

	return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
	if (!timerfreq) return;

	timerfreq=0;
	timerticspersec = 0;
}

//
// sampletimer() -- update totalclock
//
void sampletimer(void)
{
	int64_t i;
	int n;

	if (!timerfreq) return;

	QueryPerformanceCounter((LARGE_INTEGER*)&i);
	n = (int)(i*timerticspersec / timerfreq) - timerlastsample;
	if (n>0) {
		totalclock += n;
		timerlastsample += n;
	}

	if (usertimercallback) for (; n>0; n--) usertimercallback();
}


//
// getticks() -- returns the millisecond ticks count
//
unsigned int getticks(void)
{
	int64_t i;
	if (timerfreq == 0) return 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&i);
	return (unsigned int)(i*INT64_C(1000)/timerfreq);
}


//
// getusecticks() -- returns the microsecond ticks count
//
unsigned int getusecticks(void)
{
	int64_t i;
	if (timerfreq == 0) return 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&i);
	return (unsigned int)(i*INT64_C(1000000)/timerfreq);
}


//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int gettimerfreq(void)
{
	return timerticspersec;
}


//-------------------------------------------------------------------------------------------------
//  INPUT (MOUSE/KEYBOARD)
//=================================================================================================

// I don't see any pressing need to store the key-up events yet
#define SetKey(key,state) { \
	keystatus[key] = state; \
		if (state) { \
	keyfifo[keyfifoend] = key; \
	keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = state; \
	keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1)); \
		} \
}


//
// initinput() -- init input system
//
int initinput(void)
{
	moustat=0;
	memset(keystatus, 0, sizeof(keystatus));
	keyfifoplc = keyfifoend = 0;
	keyasciififoplc = keyasciififoend = 0;
	
	SetNumLock(0);
	
	#if MOUSETYPE == 1
	{
		g_lpDI = NULL;
		pDirectInputCreate = DirectInputCreateA;
		if FAILED(iDirectInputCreate(hInstance, DIRECTINPUT_VERSION, &g_lpDI, NULL)) 
		{ 
			buildputs("Failed to initialize Direct Input.\n");
			return -1;
		}
	}
	#endif
	
	#if HAVE_JOYSTICK_SUPP == 1
	{
		DWORD usernum, result;
		XINPUT_CAPABILITIES caps;
		inputdevices = 1, joynumaxes = 0, joynumbuttons = 0;
		
		fetchkeynames();
		buildputs("Initialising game controllers\n");
		for (usernum = 0; usernum < XUSER_MAX_COUNT; usernum++)
		{
			result = XInputGetCapabilities(usernum, XINPUT_FLAG_GAMEPAD, &caps);
			if (result == ERROR_SUCCESS && xinputusernum < 0)
			{
				xinputusernum = (int)usernum;
				inputdevices |= 4;

				joynumbuttons = 15;
				joynumaxes = 6;
			}
		}
		if (xinputusernum >= 0)
		{
			buildprintf("  - Using controller in port %d\n", xinputusernum);
		}
		else
		{
			buildputs("  - No usable controller found\n");
		}
	}
	#endif
	return 0;
}


//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
	uninitmouse();

	xinputusernum = -1;
	inputdevices &= ~4;
}


//
// bgetchar, bkbhit, bflushchars -- character-based input functions
//
unsigned char bgetchar(void)
{
	unsigned char c;
	if (keyasciififoplc == keyasciififoend) return 0;
	c = keyasciififo[keyasciififoplc];
	keyasciififoplc = ((keyasciififoplc+1)&(KEYFIFOSIZ-1));
	return c;
}

int bkbhit(void)
{
	return (keyasciififoplc != keyasciififoend);
}

void bflushchars(void)
{
	keyasciififoplc = keyasciififoend = 0;
}


//
// set{key|mouse|joy}presscallback() -- sets a callback which gets notified when keys are pressed
//
void setkeypresscallback(void (*callback)(int, int)) { keypresscallback = callback; }
void setmousepresscallback(void (*callback)(int, int)) { mousepresscallback = callback; }
void setjoypresscallback(void (*callback)(int, int)) { joypresscallback = callback; }


void UpdateWindowInfo(HWND hWnd)
{
	const int kWndPad = 10;
	int twh, thg;
	
	
	if (!hWnd)
	{
		memset(&windowpos, 0, sizeof(windowpos));
		memset(&windowcen, 0, sizeof(windowcen));
		memset(&windowrct, 0, sizeof(windowrct));
	}
	else
	{
		GetWindowRect(hWnd, &windowrct);
		
		if (fullscreen)
		{
			windowrct.top  += kWndPad; windowrct.bottom -= kWndPad;
			windowrct.left += kWndPad; windowrct.right  -= kWndPad;
		}
		else
		{
			twh = (windowrct.right-windowrct.left)-xres;
			thg = (windowrct.bottom-windowrct.top)-yres;
			
			windowrct.left += twh; windowrct.right += twh;	// borders
			windowrct.top  += thg;							// title
		}
		
		windowcen.x = (windowrct.right  + windowrct.left) >> 1;
		windowcen.y = (windowrct.bottom + windowrct.top)  >> 1;
		
		windowpos.x = windowrct.left;
		windowpos.y = windowrct.top;
	}
}

void CenterCursor(HWND hWnd)
{
	ClipCursor(NULL); // unclip so SetCursorPos works!
	SetCursorPos(windowcen.x, windowcen.y);
	ClipCursor(&windowrct);
}

//
// initmouse() -- init mouse input
//
int initmouse(void)
{
	if (moustat)
		return 0;
	
	buildputs("Initialising mouse\n");

	#if MOUSETYPE == 1
	{
		LPVOID pvRef; HRESULT hr;
		if FAILED(IDirectInput_CreateDevice(g_lpDI, &GUID_SysMouse, &g_pMouse, NULL)) 
		{ 
			buildputs("Could not create mouse device.\n");
			return -1;
		}
		
		if FAILED(IDirectInputDevice_SetDataFormat(g_pMouse, &c_dfDIMouse)) 
		{ 
			buildputs("Could not set data to mouse format.\n");
			return -1;
		}
		
		if FAILED(IDirectInputDevice_SetCooperativeLevel(g_pMouse, hWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) 
		{ 
			buildputs("Could not set mouse coop level.\n");
			return -1;
		}
		
		dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = 16;
		
		if (FAILED(IDirectInputDevice_SetProperty(g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph)))
		{
			buildputs("Could not set mouse buffer size.\n");
			return -1;
		}
	}	
	#elif MOUSETYPE == 2
	{	
		RAWINPUTDEVICE rid;

		// Register for mouse raw input.
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;
		rid.dwFlags = 0;	// We want legacy events when the mouse is not grabbed, so no RIDEV_NOLEGACY.
		rid.hwndTarget = NULL;
		if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
		{
			buildprintf("initinput: could not register for raw mouse input (%s)\n",
				getwindowserrorstr(GetLastError()));
			return -1;
		}
	}
	#endif
	
	
	// grab input
	moustat=1;
	inputdevices |= 2;
	if (appactive)
	{
		CenterCursor(hWindow);
		grabmouse(1);
		
	}
	return 0;
}


//
// uninitmouse() -- uninit mouse input
//
void uninitmouse(void)
{
	if (!moustat)
		return;
	
	grabmouse(0);
	moustat = mousegrab = 0;
	
	#if MOUSETYPE == 1
	{
		if (g_pMouse)
		{
			IDirectInputDevice_Release(g_pMouse);
			g_pMouse = NULL;
		}

		if (g_lpDI)
		{
			IDirectInput_Release(g_lpDI);
			g_lpDI = NULL;
		}
	}	
	#elif MOUSETYPE == 2
	{
		RAWINPUTDEVICE rid;
		// Unregister for mouse raw input.
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;
		rid.dwFlags = RIDEV_REMOVE;
		rid.hwndTarget = NULL;
		if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
		{
			buildprintf("initinput: could not unregister for raw mouse input (%s)\n",
				getwindowserrorstr(GetLastError()));
		}
	}
	#endif
}

#if MOUSETYPE == 1
void dinputReadMouse()
{
	HRESULT       		hr;
	DIDEVICEOBJECTDATA	od;
	DWORD              	dwElements;
	int i;
	
	if (!moustat || !mousegrab)
		return;
	
	while( 1 )
	{
		dwElements = 1;
		hr = IDirectInputDevice_GetDeviceData(g_pMouse, sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
		{
			// let's try to re-grab mouse
			grabmouse(0); grabmouse(1);
			continue;
		}
		
		if (dwElements == 0 || FAILED(hr))
			break;

		switch (od.dwOfs) {
			case DIMOFS_X:
				mousex += od.dwData;
				break;
			case DIMOFS_Y:
				mousey += od.dwData;
				break;
			case DIMOFS_Z:
				i = (signed int)od.dwData;
				i = (i < 0) ? 1 : 0;
				mousewheel[i] = getticks();
				mouseb |= (16 << i);
				break;
			case DIMOFS_BUTTON0:
				if (od.dwData & 0x80) mouseb |= 0x01;
				else mouseb &= ~0x01;
				break;
			case DIMOFS_BUTTON1:
				if (od.dwData & 0x80) mouseb |= 0x02;
				else mouseb &= ~0x02;
				break;
			case DIMOFS_BUTTON2:
				if (od.dwData & 0x80) mouseb |= 0x04;
				else mouseb &= ~0x04;
				break;
		}
	}
}
#endif

static void updatemouse(void)
{
	if (!mousegrab)
		return;
	
	mouseclock = getticks();
	
	#if MOUSETYPE == 1
		dinputReadMouse();
	#endif
	
	if (mousex || mousey)
		CenterCursor(hWindow);
	
	// we only want the wheel to signal once, but hold the state for a moment
	if (mousewheel[0] > 0 && mouseclock - mousewheel[0] > MouseWheelFakePressTime)
	{
		if (mousepresscallback) mousepresscallback(5,0);
		mousewheel[0] = 0; mouseb &= ~16;
	}
	
	if (mousewheel[1] > 0 && mouseclock - mousewheel[1] > MouseWheelFakePressTime)
	{
		if (mousepresscallback) mousepresscallback(6,0);
		mousewheel[1] = 0; mouseb &= ~32;
	}
}


//
// grabmouse() -- show/hide mouse cursor
//
void grabmouse(int a)
{	
	if (!moustat || a == mousegrab)
		return;
	
	if (a)
	{
		#if MOUSETYPE == 1
		{
			HRESULT       hr;
			DIDEVICEOBJECTDATA	od;
			DWORD              dwElements;
			int nPriority = -1;
			
			if (FAILED(hr = IDirectInputDevice_Acquire(g_pMouse)))
			{
				if (hr == DIERR_OTHERAPPHASPRIO)
				{
					nPriority = GetPriorityClass(GetCurrentProcess());
					SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
				}
				
				while( 1 )
				{
					dwElements = 1;
					hr = IDirectInputDevice_GetDeviceData(g_pMouse, sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
					if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
					{
						IDirectInputDevice_Acquire(g_pMouse);
						break;
					}
				}
				
				if (nPriority != -1)
					SetPriorityClass(GetCurrentProcess(), nPriority);
			}
		}
		#elif MOUSETYPE == 0
		{
			SetCapture(hWindow);
		}
		#endif
		
		UpdateWindowInfo(hWindow);
		CenterCursor(hWindow);
		ShowCursor(FALSE);
	}
	else
	{
		#if MOUSETYPE == 1
			IDirectInputDevice_Unacquire(g_pMouse);
		#elif MOUSETYPE == 0
			ReleaseCapture();
		#endif
			
		ShowCursor(TRUE);
		ClipCursor(NULL);
	}
	
	mousegrab = a;
	mousex = 0;
	mousey = 0;
	mouseb = 0;
}

//
// readmousexy() -- return mouse motion information
//
void readmousexy(int *x, int *y)
{
	if (!moustat || !mousegrab)
	{
		*x = *y = 0;
		return;
	}
	
	*x = mousex;
	*y = mousey;
	mousex = 0;
	mousey = 0;
}


//
// readmousebstatus() -- return mouse button information
//
void readmousebstatus(int *b)
{
	if (!moustat || !mousegrab)
	{
		*b = 0;
		return;
	}
	
	*b = mouseb;
}


static void updatejoystick(void)
{
	#if HAVE_JOYSTICK_SUPP == 1
		XINPUT_STATE state;
		if (xinputusernum < 0)
			return;

		ZeroMemory(&state, sizeof(state));
		if (XInputGetState(xinputusernum, &state) != ERROR_SUCCESS) {
			buildputs("Joystick error, disabling.\n");
			joyb = 0;
			memset(joyaxis, 0, sizeof(joyaxis));
			xinputusernum = -1;
			return;
		}

		// We use SDL's game controller button order for BUILD:
		//   A, B, X, Y, Back, (Guide), Start, LThumb, RThumb,
		//   LShoulder, RShoulder, DPUp, DPDown, DPLeft, DPRight
		// So we must shuffle XInput around.
		joyb = ((state.Gamepad.wButtons & 0xF000) >> 12) |		// A,B,X,Y
			   ((state.Gamepad.wButtons & 0x0020) >> 1) |		// Back
			   ((state.Gamepad.wButtons & 0x0010) << 2) |       // Start
			   ((state.Gamepad.wButtons & 0x03C0) << 1) |       // LThumb,RThumb,LShoulder,RShoulder
			   ((state.Gamepad.wButtons & 0x000F) << 8);		// DPadUp,Down,Left,Right

		joyaxis[0] = state.Gamepad.sThumbLX;
		joyaxis[1] = -state.Gamepad.sThumbLY;
		joyaxis[2] = state.Gamepad.sThumbRX;
		joyaxis[3] = -state.Gamepad.sThumbRY;
		joyaxis[4] = (state.Gamepad.bLeftTrigger >> 1) | ((int)state.Gamepad.bLeftTrigger << 7);	// Extend to 0-32767
		joyaxis[5] = (state.Gamepad.bRightTrigger >> 1) | ((int)state.Gamepad.bRightTrigger << 7);
	#endif
}


void releaseallbuttons(void)
{
	int i;

	if (mousepresscallback) {
		if (mouseb & 1) mousepresscallback(1, 0);
		if (mouseb & 2) mousepresscallback(2, 0);
		if (mouseb & 4) mousepresscallback(3, 0);
		if (mouseb & 8) mousepresscallback(4, 0);
		if (mousewheel[0]>0) mousepresscallback(5,0);
		if (mousewheel[1]>0) mousepresscallback(6,0);
	}
	mousewheel[0]=mousewheel[1]=0;
	mouseb = 0;

	if (joypresscallback)
	{
		for (i=0;i<32;i++)
			if (joyb & (1<<i)) joypresscallback(i+1, 0);
	}
	
	joyb = joyblast = 0;

	for (i=0;i<256;i++) {
		//if (!keystatus[i]) continue;
		//if (OSD_HandleKey(i, 0) != 0) {
			OSD_HandleKey(i, 0);
			SetKey(i, 0);
			if (keypresscallback) keypresscallback(i, 0);
		//}
	}
}


//
// fetchkeynames() -- retrieves the names for all the keys on the keyboard
//
static void putkeyname(int vsc, int ex, int scan) {
	TCHAR tbuf[24];

	vsc <<= 16;
	vsc |= ex << 24;
	if (GetKeyNameText(vsc, tbuf, 24) == 0) return;
	CharToOemBuff(tbuf, keynames[scan], 24-1);

	//buildprintf("VSC %8x scan %-2x = %s\n", vsc, scan, keynames[scan]);
}

static void fetchkeynames(void)
{
	int scan, ex;
	unsigned i;

	memset(keynames,0,sizeof(keynames));
	for (i=0; i < 256; i++)
	{
		scan = wscantable[i];
		if (scan != 0)
			putkeyname(i, 0, scan);
		
		scan = wxscantable[i];
		if (scan != 0)
			putkeyname(i, 1, scan);
	}
}

const char *getkeyname(int num)
{
	if ((unsigned)num >= 256) return NULL;
	return keynames[num];
}

const char *getjoyname(int what, int num)
{
	static const char * axisnames[6] = {
		"Left Stick X",
		"Left Stick Y",
		"Right Stick X",
		"Right Stick Y",
		"Left Trigger",
		"Right Trigger",
	};
	static const char * buttonnames[15] = {
		"A",
		"B",
		"X",
		"Y",
		"Start",
		"Guide",
		"Back",
		"Left Thumb",
		"Right Thumb",
		"Left Shoulder",
		"Right Shoulder",
		"DPad Up",
		"DPad Down",
		"DPad Left",
		"DPad Right",
	};
	switch (what) {
		case 0:	// axis
			if ((unsigned)num > (unsigned)6) return NULL;
			return axisnames[num];

		case 1: // button
			if ((unsigned)num > (unsigned)15) return NULL;
			return buttonnames[num];

		default:
			return NULL;
	}
}


//-------------------------------------------------------------------------------------------------
//  VIDEO
//=================================================================================================

// DIB stuff
static HDC      hDCSection  = NULL;
static HBITMAP  hDIBSection = NULL;
static HPALETTE hPalette    = NULL;
static VOID    *lpPixels    = NULL;

static int setgammaramp(WORD gt[3][256]);
static int getgammaramp(WORD gt[3][256]);

//
// checkvideomode() -- makes sure the video mode passed is legal
//
int checkvideomode(int *x, int *y, int c, int fs, int forced)
{
	int i, nearest=-1, dx, dy, odx=9999, ody=9999;

	getvalidmodes();

#if USE_OPENGL
	if (c > 8 && nogl) return -1;
#else
	if (c > 8) return -1;
#endif

	// fix up the passed resolution values to be multiples of 8
	// and at least 320x200 or at most MAXXDIMxMAXYDIM
	if (*x < 320) *x = 320;
	if (*y < 200) *y = 200;
	if (*x > MAXXDIM) *x = MAXXDIM;
	if (*y > MAXYDIM) *y = MAXYDIM;
	*x &= 0xfffffff8l;

	for (i=0; i<validmodecnt; i++) {
		if (validmode[i].bpp != c) continue;
		if (validmode[i].fs != fs) continue;
		dx = klabs(validmode[i].xdim - *x);
		dy = klabs(validmode[i].ydim - *y);
		if (!(dx | dy)) { 	// perfect match
			nearest = i;
			break;
		}
		if ((dx <= odx) && (dy <= ody)) {
			nearest = i;
			odx = dx; ody = dy;
		}
	}

#ifdef ANY_WINDOWED_SIZE
	if (!forced && (fs&1) == 0 && (nearest < 0 || validmode[nearest].xdim!=*x || validmode[nearest].ydim!=*y)) {
		// check the colour depth is recognised at the very least
		for (i=0;i<validmodecnt;i++)
			if (validmode[i].bpp == c)
				return 0x7fffffffl;
		return -1;	// strange colour depth
	}
#endif

	if (nearest < 0) {
		// no mode that will match (eg. if no fullscreen modes)
		return -1;
	}

	*x = validmode[nearest].xdim;
	*y = validmode[nearest].ydim;

	return nearest;		// JBF 20031206: Returns the mode number
}

static void shutdownvideo(void)
{
#if USE_OPENGL
	if (frame) {
		free(frame);
		frame = NULL;
	}
	glbuild_delete_8bit_shader(&gl8bit);
	UninitOpenGL();
#endif
	UninitDIB();

	if (desktopmodeset) {
		ChangeDisplaySettings(NULL, 0);
		desktopmodeset = 0;
	}
}

//
// setvideomode() -- set the video mode
//
int setvideomode(int x, int y, int c, int fs)
{
	int i, modenum, refresh=-1;

	if ((fs == fullscreen) && (x == xres) && (y == yres) && (c == bpp) && !videomodereset) {
		OSD_ResizeDisplay(xres,yres);
		return 0;
	}

	modenum = checkvideomode(&x,&y,c,fs,0);
	if (modenum < 0) return -1;
	if (modenum != 0x7fffffff) {
		refresh = validmode[modenum].extra;
	}

	if (hWindow && gammabrightness) {
		setgammaramp(sysgamma);
		gammabrightness = 0;
	}

	shutdownvideo();

	buildprintf("Setting video mode %dx%d (%d-bit %s)\n",
			x,y,c, ((fs&1) ? "fullscreen" : "windowed"));

	if (!CreateAppWindow(x, y, c, fs, refresh)) return -1;

	if (!gammabrightness) {
		if (getgammaramp(sysgamma) >= 0) gammabrightness = 1;
		if (gammabrightness && setgamma(curgamma) < 0) gammabrightness = 0;
	}

	modechange=1;
	videomodereset = 0;
	//baselayer_onvideomodechange(c>8);
	
	if (appactive)
		grabmouse(1);
	
	return 0;
}


//
// getvalidmodes() -- figure out what video modes are available
//
#if 0
#define ADDMODE(x,y,c,f,n) if (validmodecnt<MAXVALIDMODES) { \
	validmode[validmodecnt].xdim=x; \
	validmode[validmodecnt].ydim=y; \
	validmode[validmodecnt].bpp=c; \
	validmode[validmodecnt].fs=f; \
	validmode[validmodecnt].extra=n; \
	validmodecnt++; \
	buildprintf("  - %dx%d %d-bit %s\n", x, y, c, (f&1)?"fullscreen":"windowed"); \
	}

#define CHECKL(w,h) if ((w < maxx) && (h < maxy))
#define CHECKLE(w,h) if ((w <= maxx) && (h <= maxy))

#if USE_OPENGL
static void cdsenummodes(void)
{
	DEVMODE dm;
	int i = 0, j = 0;

	struct { unsigned x,y,bpp,freq; } modes[MAXVALIDMODES];
	int nmodes=0;
	unsigned maxx = MAXXDIM, maxy = MAXYDIM;

	// Enumerate display modes.
	ZeroMemory(&dm,sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	while (nmodes < MAXVALIDMODES && EnumDisplaySettings(NULL, j, &dm)) {
		// Identify the same resolution and bit depth in the existing set.
		for (i=0;i<nmodes;i++) {
			if (modes[i].x == dm.dmPelsWidth
			 && modes[i].y == dm.dmPelsHeight
			 && modes[i].bpp == dm.dmBitsPerPel)
				break;
		}
		// A new mode, or a same format mode with a better refresh rate match.
		if ((i==nmodes) ||
		    (dm.dmDisplayFrequency <= maxrefreshfreq && dm.dmDisplayFrequency > modes[i].freq && maxrefreshfreq > 0) ||
		    (dm.dmDisplayFrequency > modes[i].freq && maxrefreshfreq == 0)) {
			if (i==nmodes) nmodes++;

			modes[i].x = dm.dmPelsWidth;
			modes[i].y = dm.dmPelsHeight;
			modes[i].bpp = dm.dmBitsPerPel;
			modes[i].freq = dm.dmDisplayFrequency;
		}

		j++;
		ZeroMemory(&dm,sizeof(DEVMODE));
		dm.dmSize = sizeof(DEVMODE);
	}

	// Add what was found to the list.
	for (i=0;i<nmodes;i++) {
		CHECKLE(modes[i].x, modes[i].y) {
			ADDMODE(modes[i].x, modes[i].y, modes[i].bpp, 1, modes[i].freq);
		}
	}
}
#endif


#endif

static int sortmodes(const struct validmode_t *a, const struct validmode_t *b)
{
	int x;

	if ((x = a->fs   - b->fs)   != 0) return x;
	if ((x = a->bpp  - b->bpp)  != 0) return x;
	if ((x = a->xdim - b->xdim) != 0) return x;
	if ((x = a->ydim - b->ydim) != 0) return x;

	return 0;
}

void getvalidmodes(void)
{
	DEVMODE d;	
	int i, j;

	if (!modeschecked)
	{
		validmodecnt = 0; // offer best possible video modes
		for (i = 0; EnumDisplaySettings(NULL, i, &d) && validmodecnt < MAXVALIDMODES; i++)
		{
			if (d.dmPelsWidth > MAXXDIM || d.dmPelsHeight > MAXYDIM) continue;
			//if (maxrefreshfreq && d.dmDisplayFrequency > maxrefreshfreq) continue;
			if (desktopbpp < d.dmBitsPerPel) continue;
			if (!fullscreen && (d.dmPelsWidth > desktopxdim || d.dmPelsHeight > desktopydim))
				continue;
			//if (fullscreen)

			j = validmodecnt;
			while(--j >= 0 && (validmode[j].xdim != d.dmPelsWidth || validmode[j].ydim != d.dmPelsHeight));
			
			if (j < 0) j = validmodecnt++;
			else if (d.dmBitsPerPel < validmode[j].bpp || d.dmDisplayFrequency < validmode[j].extra)
				continue;
			
			// update existing or add new mode
			validmode[j].extra	= d.dmDisplayFrequency;
			validmode[j].xdim	= d.dmPelsWidth;
			validmode[j].ydim 	= d.dmPelsHeight;
			validmode[j].bpp	= d.dmBitsPerPel;
			validmode[j].fs		= 1;
		}
		
		qsort((void*)validmode, validmodecnt, sizeof(struct validmode_t), (int(*)(const void*,const void*))sortmodes);
		modeschecked = 1;
	}
	

	
	/*static int defaultres[][2] = {
		{1920,1200},{1920,1080},{1600,1200},{1680,1050},{1600,900},{1400,1050},{1440,900},{1366,768},
		{1280,1024},{1280,960},{1280,800},{1280,720},{1152,864},{1024,768},{800,600},{640,480},
		{640,400},{512,384},{480,360},{400,300},{320,240},{320,200},{0,0}
	};
	int i, j, maxx=0, maxy=0;

	if (modeschecked) return;

	validmodecnt=0;
	buildputs("Detecting video modes:\n");

	// Fullscreen 8-bit modes: upsamples to the desktop mode.
	maxx = desktopxdim;
	maxy = desktopydim;
	for (i=0; defaultres[i][0]; i++) {
		CHECKLE(defaultres[i][0],defaultres[i][1]) {
			ADDMODE(defaultres[i][0], defaultres[i][1], 8, 1, -1);
		}
	}

#if USE_POLYMOST && USE_OPENGL
	// Fullscreen >8-bit modes.
	if (!nogl) cdsenummodes();
#endif

	// Windowed modes can't be bigger than the current desktop resolution.
	maxx = desktopxdim+1;
	maxy = desktopydim+1;

	// Windows 8-bit modes
	for (i=0; defaultres[i][0]; i++) {
		CHECKL(defaultres[i][0],defaultres[i][1]) {
			ADDMODE(defaultres[i][0], defaultres[i][1], 8, 0, -1);
		}
	}

#if USE_POLYMOST && USE_OPENGL
	// Windowed >8-bit modes
	if (!nogl) {
		for (i=0; defaultres[i][0]; i++) {
			CHECKL(defaultres[i][0],defaultres[i][1]) {
				ADDMODE(defaultres[i][0], defaultres[i][1], desktopbpp, 0, -1);
			}
		}
	}
#endif

	//qsort((void*)validmode, validmodecnt, sizeof(struct validmode_t), (int(*)(const void*,const void*))sortmodes);

	modeschecked=1;*/
}

//
// resetvideomode() -- resets the video system
//
void resetvideomode(void)
{
	videomodereset = 1;
	modeschecked = 0;
}


//
// begindrawing() -- locks the framebuffer for drawing
//
void begindrawing(void)
{
}


//
// enddrawing() -- unlocks the framebuffer
//
void enddrawing(void)
{
}


//
// showframe() -- update the display
//
void showframe(void)
{
	HRESULT result;
	char *p,*q;
	int i,j;

#if USE_OPENGL
	if (!nogl) {
		if (bpp == 8) {
			glbuild_update_8bit_frame(&gl8bit, frame, xres, yres, bytesperline);
			glbuild_draw_8bit_frame(&gl8bit);
		}

		wglfunc.wglSwapBuffers(hDCGLWindow);
		return;
	}
#endif

	{
		if ((xres == desktopxdim && yres == desktopydim) || !fullscreen) {
			BitBlt(hDCWindow, 0, 0, xres, yres, hDCSection, 0, 0, SRCCOPY);
		} else {
			int xpos, ypos, xscl, yscl;
			int desktopaspect = divscale16(desktopxdim, desktopydim);
			int frameaspect = divscale16(xres, yres);

			if (desktopaspect >= frameaspect) {
				// Desktop is at least as wide as the frame. We maximise frame height and centre on width.
				ypos = 0;
				yscl = desktopydim;
				xscl = mulscale16(desktopydim, frameaspect);
				xpos = (desktopxdim - xscl) >> 1;
			} else {
				// Desktop is narrower than the frame. We maximise frame width and centre on height.
				xpos = 0;
				xscl = desktopxdim;
				yscl = divscale16(desktopxdim, frameaspect);
				ypos = (desktopydim - yscl) >> 1;
			}

			StretchBlt(hDCWindow, xpos, ypos, xscl, yscl, hDCSection, 0, 0, xres, yres, SRCCOPY);
		}
	}
}


//
// setpalette() -- set palette values
//
int setpalette(int UNUSED(start), int UNUSED(num), unsigned char * UNUSED(dapal))
{
#if USE_OPENGL
	if (!nogl && bpp == 8) {
		glbuild_update_8bit_palette(&gl8bit, curpalettefaded);
		return 0;
	}
#endif
	if (hDCSection) {
		RGBQUAD rgb[256];
		int i;

		for (i = 0; i < 256; i++) {
			rgb[i].rgbBlue = curpalettefaded[i].b;
			rgb[i].rgbGreen = curpalettefaded[i].g;
			rgb[i].rgbRed = curpalettefaded[i].r;
			rgb[i].rgbReserved = 0;
		}

		SetDIBColorTable(hDCSection, 0, 256, rgb);
	}

	return 0;
}


//
// setgamma
//
static int setgammaramp(WORD gt[3][256])
{
	int i;
	i = SetDeviceGammaRamp(hDCWindow, gt) ? 0 : -1;
	return i;
}

int setgamma(float gamma)
{
	int i;
	WORD gt[3][256];

	if (!hWindow) return -1;

	gamma = 1.0 / gamma;
	for (i=0;i<256;i++) {
		gt[0][i] =
		gt[1][i] =
		gt[2][i] = (WORD)min(65535, max(0, (int)(pow((double)i / 256.0, gamma) * 65535.0 + 0.5)));
	}

	return setgammaramp(gt);
}

static int getgammaramp(WORD gt[3][256])
{
	int i;

	if (!hWindow) return -1;

	i = GetDeviceGammaRamp(hDCWindow, gt) ? 0 : -1;

	return i;
}


//
// UninitDIB() -- clean up the DIB renderer
//
static void UninitDIB(void)
{
	if (hPalette) {
		DeleteObject(hPalette);
		hPalette = NULL;
	}

	if (hDCSection) {
		DeleteDC(hDCSection);
		hDCSection = NULL;
	}

	if (hDIBSection) {
		DeleteObject(hDIBSection);
		hDIBSection = NULL;
	}
}


//
// SetupDIB() -- sets up DIB rendering
//
static int SetupDIB(int width, int height)
{
	struct binfo {
		BITMAPINFOHEADER header;
		RGBQUAD colours[256];
	} dibsect;
	int i, bpl;

	// create the new DIB section
	memset(&dibsect, 0, sizeof(dibsect));
	numpages = 1;	// KJS 20031225
	dibsect.header.biSize = sizeof(dibsect.header);
	dibsect.header.biWidth = width|1;	// Ken did this
	dibsect.header.biHeight = -height;
	dibsect.header.biPlanes = 1;
	dibsect.header.biBitCount = 8;
	dibsect.header.biCompression = BI_RGB;
	dibsect.header.biClrUsed = 256;
	dibsect.header.biClrImportant = 256;
	for (i=0; i<256; i++) {
		dibsect.colours[i].rgbBlue = curpalettefaded[i].b;
		dibsect.colours[i].rgbGreen = curpalettefaded[i].g;
		dibsect.colours[i].rgbRed = curpalettefaded[i].r;
	}

	hDIBSection = CreateDIBSection(hDCWindow, (BITMAPINFO *)&dibsect, DIB_RGB_COLORS, &lpPixels, NULL, 0);
	if (!hDIBSection || lpPixels == NULL) {
		UninitDIB();
		ShowErrorBox("Error creating DIB section");
		return TRUE;
	}

	memset(lpPixels, 0, (((width|1) + 4) & ~3)*height);

	// create a compatible memory DC
	hDCSection = CreateCompatibleDC(hDCWindow);
	if (!hDCSection) {
		UninitDIB();
		ShowErrorBox("Error creating compatible DC");
		return TRUE;
	}

	// select the DIB section into the memory DC
	if (!SelectObject(hDCSection, hDIBSection)) {
		UninitDIB();
		ShowErrorBox("Error creating compatible DC");
		return TRUE;
	}

	return FALSE;
}

#if USE_OPENGL

//
// loadgldriver -- loads an OpenGL DLL
//
int loadgldriver(const char *dll)
{
	if (hGLDLL) return 0;	// Already loaded

	if (!dll) {
		dll = "OPENGL32.DLL";
	}

	buildprintf("Loading %s\n", dll);

	hGLDLL = LoadLibrary(dll);
	if (!hGLDLL) return -1;

	return 0;
}

int unloadgldriver(void)
{
	if (!hGLDLL) return 0;
	FreeLibrary(hGLDLL);
	hGLDLL = NULL;
	return 0;
}

//
// getglprocaddress
//
void *getglprocaddress(const char *name, int ext)
{
	void *func = NULL;
	if (!hGLDLL) return NULL;
	if (ext && wglfunc.wglGetProcAddress) {
		func = wglfunc.wglGetProcAddress(name);
	}
	if (!func) {
		func = GetProcAddress(hGLDLL, name);
	}
	return func;
}


//
// UninitOpenGL() -- cleans up OpenGL rendering
//

static void UninitOpenGL(void)
{
	if (hGLRC) {
#if USE_POLYMOST
		polymost_glreset();
#endif
		if (!wglfunc.wglMakeCurrent(0,0)) { }
		if (!wglfunc.wglDeleteContext(hGLRC)) { }
		hGLRC = NULL;
	}
	if (hGLWindow) {
		if (hDCGLWindow) {
			ReleaseDC(hGLWindow, hDCGLWindow);
			hDCGLWindow = NULL;
		}

		DestroyWindow(hGLWindow);
		hGLWindow = NULL;
	}
}

// Enumerate the WGL interface extensions.
static void EnumWGLExts(HDC hdc)
{
	const GLchar *extstr;
	char *workstr, *workptr, *nextptr = NULL, *ext = NULL;
	int ack;

	wglfunc.wglGetExtensionsStringARB = getglprocaddress("wglGetExtensionsStringARB", 1);
	if (!wglfunc.wglGetExtensionsStringARB) {
		debugprintf("Note: OpenGL does not provide WGL_ARB_extensions_string extension.\n");
		return;
	}

	extstr = wglfunc.wglGetExtensionsStringARB(hdc);

	debugprintf("WGL extensions supported:\n");
	workstr = workptr = strdup(extstr);
	while ((ext = Bstrtoken(workptr, " ", &nextptr, 1)) != NULL) {
		if (!strcmp(ext, "WGL_ARB_pixel_format")) {
			wglfunc.wglChoosePixelFormatARB = getglprocaddress("wglChoosePixelFormatARB", 1);
			ack = !wglfunc.wglChoosePixelFormatARB ? '!' : '+';
		} else if (!strcmp(ext, "WGL_ARB_create_context")) {
			wglfunc.wglCreateContextAttribsARB = getglprocaddress("wglCreateContextAttribsARB", 1);
			ack = !wglfunc.wglCreateContextAttribsARB ? '!' : '+';
		} else if (!strcmp(ext, "WGL_ARB_create_context_profile")) {
			wglfunc.have_ARB_create_context_profile = 1;
			ack = '+';
		} else if (!strcmp(ext, "WGL_EXT_swap_control")) {
			wglfunc.wglSwapIntervalEXT = getglprocaddress("wglSwapIntervalEXT", 1);
			wglfunc.wglGetSwapIntervalEXT = getglprocaddress("wglGetSwapIntervalEXT", 1);
			ack = (!wglfunc.wglSwapIntervalEXT || !wglfunc.wglGetSwapIntervalEXT) ? '!' : '+';
		} else {
			ack = ' ';
		}
		debugprintf("  %s %c\n", ext, ack);
		workptr = NULL;
	}
	free(workstr);
}

//
// SetupOpenGL() -- sets up opengl rendering
//
static int SetupOpenGL(int width, int height, int bitspp, int cover)
{
	int err, pixelformat;

	// Step 1. Create a fake context with a safe pixel format descriptor.
	GLuint dummyPixelFormat;
	PIXELFORMATDESCRIPTOR dummyPfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,                             //Version Number
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, //Must Support these
		PFD_TYPE_RGBA,                 //Request An RGBA Format
		32,                            //Color Depth
		0,0,0,0,0,0,                   //Color Bits Ignored
		0,                             //No Alpha Buffer
		0,                             //Shift Bit Ignored
		0,                             //No Accumulation Buffer
		0,0,0,0,                       //Accumulation Bits Ignored
		24,                            //16/24/32 Z-Buffer depth
		8,                             //Stencil Buffer
		0,                             //No Auxiliary Buffer
		PFD_MAIN_PLANE,                //Main Drawing Layer
		0,                             //Reserved
		0,0,0                          //Layer Masks Ignored
	};
	HDC dummyhDC = 0;
	HGLRC dummyhGLRC = 0;
	const char *errmsg = NULL;

	dummyhGLwindow = CreateWindow(
			WINDOW_CLASS,
			"OpenGL Window",
			WS_CHILD,
			0,0,
			1,1,
			hWindow,
			(HMENU)0,
			hInstance,
			NULL);
	if (!dummyhGLwindow) {
		errmsg = "Error creating dummy OpenGL child window.";
		goto fail;
	}

	dummyhDC = GetDC(dummyhGLwindow);
	if (!dummyhDC) {
		errmsg = "Error getting dummy device context";
		goto fail;
	}

	dummyPixelFormat = ChoosePixelFormat(dummyhDC, &dummyPfd);
	if (!dummyPixelFormat) {
		errmsg = "Can't choose dummy pixel format";
		goto fail;
	}

	err = SetPixelFormat(dummyhDC, dummyPixelFormat, &dummyPfd);
	if (!err) {
		errmsg = "Can't set dummy pixel format";
		goto fail;
	}

	dummyhGLRC = wglfunc.wglCreateContext(dummyhDC);
	if (!dummyhGLRC) {
		errmsg = "Can't create dummy GL context";
		goto fail;
	}

	if (!wglfunc.wglMakeCurrent(dummyhDC, dummyhGLRC)) {
		errmsg = "Can't activate dummy GL context";
		goto fail;
	}

	// Step 2. Check the WGL extensions.
	EnumWGLExts(dummyhDC);

	// Step 3. Create the actual window we will use going forward.
	{
		int xpos, ypos, xscl, yscl;

		if (cover) {
			// The desktop resolution is set to the target. Fill the screen.
			xpos = ypos = 0;
			xscl = width;
			yscl = height;
		} else {
			// The desktop resolution remains the same and we stretch to fit.
			int desktopaspect = divscale16(desktopxdim, desktopydim);
			int frameaspect = divscale16(width, height);
			if (desktopaspect >= frameaspect) {
				// Desktop is at least as wide as the frame. We maximise frame height and centre on width.
				ypos = 0;
				yscl = desktopydim;
				xscl = mulscale16(desktopydim, frameaspect);
				xpos = (desktopxdim - xscl) >> 1;
			} else {
				// Desktop is narrower than the frame. We maximise frame width and centre on height.
				xpos = 0;
				xscl = desktopxdim;
				yscl = divscale16(desktopxdim, frameaspect);
				ypos = (desktopydim - yscl) >> 1;
			}
		}

		hGLWindow = CreateWindow(
				WINDOW_CLASS,
				"OpenGL Window",
				WS_CHILD|WS_VISIBLE,
				xpos, ypos,
				xscl, yscl,
				hWindow,
				(HMENU)0,
				hInstance,
				NULL);
		if (!hGLWindow) {
			errmsg = "Error creating OpenGL child window.";
			goto fail;
		}
	}

	hDCGLWindow = GetDC(hGLWindow);
	if (!hDCGLWindow) {
		errmsg = "Error getting device context.";
		goto fail;
	}

	// Step 3. Find and set a suitable pixel format.
	if (wglfunc.wglChoosePixelFormatARB) {
		UINT numformats;
		int pformatattribs[] = {
			WGL_DRAW_TO_WINDOW_ARB,	GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,     bitspp,
			WGL_DEPTH_BITS_ARB,     24,
			WGL_STENCIL_BITS_ARB,   0,
			WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
			0,
		};
		if (!wglfunc.wglChoosePixelFormatARB(hDCGLWindow, pformatattribs, NULL, 1, &pixelformat, &numformats)) {
			errmsg = "Can't choose pixel format.";
			goto fail;
		} else if (numformats < 1) {
			errmsg = "No suitable pixel format available.";
			goto fail;
		}
	} else {
		PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,                             //Version Number
			PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, //Must Support these
			PFD_TYPE_RGBA,                 //Request An RGBA Format
			bitspp,                        //Color Depth
			0,0,0,0,0,0,                   //Color Bits Ignored
			0,                             //No Alpha Buffer
			0,                             //Shift Bit Ignored
			0,                             //No Accumulation Buffer
			0,0,0,0,                       //Accumulation Bits Ignored
			24,                            //16/24/32 Z-Buffer depth
			0,                             //Stencil Buffer
			0,                             //No Auxiliary Buffer
			PFD_MAIN_PLANE,                //Main Drawing Layer
			0,                             //Reserved
			0,0,0                          //Layer Masks Ignored
		};
		pixelformat = ChoosePixelFormat(hDCGLWindow, &pfd);
		if (!pixelformat) {
			errmsg = "Can't choose pixel format";
			goto fail;
		}
	}

	DescribePixelFormat(hDCGLWindow, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &dummyPfd);
	err = SetPixelFormat(hDCGLWindow, pixelformat, &dummyPfd);
	if (!err) {
		errmsg = "Can't set pixel format.";
		goto fail;
	}

	// Step 4. Create a context with the needed profile.
	if (wglfunc.wglCreateContextAttribsARB) {
		int contextattribs[] = {
#if (USE_OPENGL == USE_GL3)
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#else
			WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
			WGL_CONTEXT_MINOR_VERSION_ARB, 1,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#endif
			0,
		};
		if (!wglfunc.have_ARB_create_context_profile) {
			contextattribs[4] = 0;	//WGL_CONTEXT_PROFILE_MASK_ARB
		}
		hGLRC = wglfunc.wglCreateContextAttribsARB(hDCGLWindow, 0, contextattribs);
		if (!hGLRC) {
			errmsg = "Can't create GL context.";
			goto fail;
		}
	} else {
		hGLRC = wglfunc.wglCreateContext(hDCGLWindow);
		if (!hGLRC) {
			errmsg = "Can't create GL context.";
			goto fail;
		}
	}

	// Scrap the dummy stuff.
	if (!wglfunc.wglMakeCurrent(NULL, NULL)) { }
	if (!wglfunc.wglDeleteContext(dummyhGLRC)) { }
	ReleaseDC(dummyhGLwindow, dummyhDC);
	DestroyWindow(dummyhGLwindow);
	dummyhGLwindow = NULL;
	dummyhGLRC = NULL;
	dummyhDC = NULL;

	if (!wglfunc.wglMakeCurrent(hDCGLWindow, hGLRC)) {
		errmsg = "Can't activate GL context";
		goto fail;
	}

	// Step 5. Done.
	switch (baselayer_setupopengl()) {
		case 0:
			break;
		case -1:
			errmsg = "Can't load required OpenGL function pointers.";
			goto fail;
		case -2:
			errmsg = "Minimum OpenGL requirements are not met.";
			goto fail;
		default:
			errmsg = "Other OpenGL initialisation error.";
			goto fail;
	}

	if (wglfunc.wglSwapIntervalEXT) {
		wglfunc.wglSwapIntervalEXT(glswapinterval);
	}
	numpages = 127;

	return FALSE;

fail:
	if (bpp > 8) {
		ShowErrorBox(errmsg);
	}
	shutdownvideo();

	if (!wglfunc.wglMakeCurrent(NULL, NULL)) { }

	if (hGLRC) {
		if (!wglfunc.wglDeleteContext(hGLRC)) { }
	}
	if (hGLWindow) {
		if (hDCGLWindow) {
			ReleaseDC(hGLWindow, hDCGLWindow);
		}
	}
	hDCGLWindow = NULL;
	hGLRC = NULL;
	hGLWindow = NULL;

	if (dummyhGLRC) {
		if (!wglfunc.wglDeleteContext(dummyhGLRC)) { }
	}
	if (dummyhGLwindow) {
		if (dummyhDC) {
			ReleaseDC(dummyhGLwindow, dummyhDC);
		}
		DestroyWindow(dummyhGLwindow);
	}

	return TRUE;
}

#endif	//USE_OPENGL

//
// CreateAppWindow() -- create the application window
//
static BOOL CreateAppWindow(int width, int height, int bitspp, int fs, int refresh)
{
	RECT rect;
	int w, h, x, y, stylebits = 0, stylebitsex = 0;
	HRESULT result;

	if (width == xres && height == yres && fs == fullscreen && bitspp == bpp && !videomodereset) return TRUE;
	if (hWindow)
		ShowWindow(hWindow, SW_HIDE);	// so Windows redraws what's behind if the window shrinks

	if (fs)
	{
		stylebitsex = 0;
		stylebits = WS_POPUP;
	}
	else
	{
		stylebitsex = 0;
		stylebits = (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
	}

	if (!hWindow)
	{
		hWindow = CreateWindowEx(stylebitsex, WINDOW_CLASS, apptitle, stylebits,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			320,
			200,
			NULL,
			NULL,
			hInstance,
			0);
			
		if (!hWindow)
		{
			ShowErrorBox("Unable to create window");
			return FALSE;
		}

		hDCWindow = GetDC(hWindow);
		if (!hDCWindow)
		{
			ShowErrorBox("Error getting device context");
			return FALSE;
		}
		
		#ifdef HAVE_START_WINDOW
		startwin_close();
		#endif
	}
	else
	{
		//SetWindowLong(hWindow,GWL_EXSTYLE,stylebitsex);
		SetWindowLong(hWindow,GWL_STYLE,stylebits);
	}

	// resize the window
	if (!fs)
	{
		rect.left = 0;
		rect.top = 0;
		rect.right = width-1;
		rect.bottom = height-1;
		AdjustWindowRect(&rect, stylebits, FALSE);

		w = (rect.right - rect.left);
		h = (rect.bottom - rect.top);
		x = (desktopxdim - w) / 2;
		y = (desktopydim - h) / 2;
	}
	else
	{
		x=y=0;
		w=desktopxdim;
		h=desktopydim;
	}
	
	SetWindowPos(hWindow, HWND_TOP, x, y, w, h, 0);

	UpdateAppWindowTitle();
	ShowWindow(hWindow, SW_SHOWNORMAL);
	SetForegroundWindow(hWindow);
	SetFocus(hWindow);

	if (bitspp == 8) {
		int i, j;

#if USE_OPENGL
		if (nogl)
		{
#endif
			// 8-bit software with no GL shader uses classic Windows DIB blitting.
			if (SetupDIB(width, height))
			{
				return FALSE;
			}

			frameplace = (intptr_t)lpPixels;
			bytesperline = (((width|1) + 4) & ~3);
#if USE_OPENGL
		}
		else
		{
			// Prepare the GLSL shader for 8-bit blitting.
			if (SetupOpenGL(width, height, bitspp, !fs))
			{
				// No luck. Write off OpenGL and try DIB.
				buildputs("OpenGL initialisation failed. Falling back to DIB mode.\n");
				nogl = 1;
				return CreateAppWindow(width, height, bitspp, fs, refresh);
			}

			bytesperline = (((width|1) + 4) & ~3);

			if (glbuild_prepare_8bit_shader(&gl8bit, width, height, bytesperline) < 0)
			{
				shutdownvideo();
				nogl = 1;
				return CreateAppWindow(width, height, bitspp, fs, refresh);
			}

			frame = (unsigned char *) malloc(bytesperline * height);
			if (!frame)
			{
				ShowErrorBox("Unable to allocate framebuffer\n");
				return FALSE;
			}

			frameplace = (intptr_t)frame;
		}
#endif

		imageSize = bytesperline*height;
		setvlinebpl(bytesperline);

		for(i=j=0; i<=height; i++) ylookup[i] = j, j += bytesperline;
		modechange=0;
		numpages = 1;
		
	}
	else
	{
#if USE_OPENGL
		if (fs)
		{
			DEVMODE dmScreenSettings;

			ZeroMemory(&dmScreenSettings, sizeof(DEVMODE));
			dmScreenSettings.dmSize = sizeof(DEVMODE);
			dmScreenSettings.dmPelsWidth = width;
			dmScreenSettings.dmPelsHeight = height;
			dmScreenSettings.dmBitsPerPel = bitspp;
			dmScreenSettings.dmFields = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
			if (refresh > 0)
			{
				dmScreenSettings.dmDisplayFrequency = refresh;
				dmScreenSettings.dmFields |= DM_DISPLAYFREQUENCY;
			}

			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				ShowErrorBox("Video mode not supported");
				return FALSE;
			}
			desktopmodeset = 1;
		}

		ShowWindow(hWindow, SW_SHOWNORMAL);
		SetForegroundWindow(hWindow);
		SetFocus(hWindow);

		if (SetupOpenGL(width, height, bitspp, !desktopmodeset))
		{
			shutdownvideo();
			nogl = 1;
			return CreateAppWindow(width, height, bitspp, fs, refresh);
		}

		frameplace = 0;
		bytesperline = 0;
		imageSize = 0;
#else
		return TRUE;
#endif
	}

	xres = width;
	yres = height;
	bpp = bitspp;
	fullscreen = fs;

	modechange = 1;
	OSD_ResizeDisplay(xres,yres);

	
	UpdateWindow(hWindow);
	UpdateWindowInfo(hWindow);
	
	return TRUE;
}


//
// DestroyAppWindow() -- destroys the application window
//
static void DestroyAppWindow(void)
{
	if (hWindow && gammabrightness) {
		setgammaramp(sysgamma);
		gammabrightness = 0;
	}

	shutdownvideo();

	if (hDCWindow) {
		ReleaseDC(hWindow, hDCWindow);
		hDCWindow = NULL;
	}

	if (hWindow) {
		DestroyWindow(hWindow);
		hWindow = NULL;
	}
}

//
// UpdateAppWindowTitle() -- sets the title of the application window
//
static void UpdateAppWindowTitle(void)
{
	char tmp[256+3+256+1];		//sizeof(wintitle) + " - " + sizeof(apptitle) + '\0'

	if (!hWindow)
		return;

	if (wintitle[0])
	{
		sprintf(tmp, "%s - %s", wintitle, apptitle);
		tmp[sizeof(tmp)-1] = 0;
		SetWindowText(hWindow, tmp);
	}
	else
	{
		SetWindowText(hWindow, apptitle);
	}
}






//-------------------------------------------------------------------------------------------------
//  MOSTLY STATIC INTERNAL WINDOWS THINGS
//=================================================================================================

//
// ShowErrorBox() -- shows an error message box
//
static void ShowErrorBox(const char *m)
{
	TCHAR msg[1024];

	wsprintf(msg, "%s: %s", m, GetWindowsErrorMsg(GetLastError()));
	MessageBox(0, msg, apptitle, MB_OK|MB_ICONSTOP);
}


//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under
//
static BOOL CheckWinVersion(void)
{
	OSVERSIONINFO osv;

	ZeroMemory(&osv, sizeof(osv));
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&osv)) return TRUE;	
	#if SUBSYS <= 401
		// At least Windows 98/95
		if (osv.dwPlatformId < VER_PLATFORM_WIN32_WINDOWS || osv.dwMajorVersion < 4)
			return TRUE;
	#elif SUBSYS <= 501
		// At least Windows XP
		if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT || (osv.dwMajorVersion < 5 && osv.dwMinorVersion < 1))
			return TRUE;
	#else
		// At least Windows Vista
		if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT || osv.dwMajorVersion < 6)
			return TRUE;
	#endif
		
	return FALSE;
}


static const int wscantable[256] = {
/*         x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF */
/* 0y */ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
/* 1y */ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
/* 2y */ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
/* 3y */ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
/* 4y */ 0x40, 0x41, 0x42, 0x43, 0x44, 0x59, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
/* 5y */ 0x50, 0x51, 0x52, 0x53, 0,    0,    0,    0x57, 0x58, 0,    0,    0,    0,    0,    0,    0,
/* 6y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 7y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 8y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 9y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Ay */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* By */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Cy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Dy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Ey */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Fy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

static const int wxscantable[256] = {
/*         x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF */
/* 0y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 1y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x9c, 0x9d, 0,    0,
/* 2y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 3y */ 0,    0,    0,    0,    0,    0xb5, 0,    0,    0xb8, 0,    0,    0,    0,    0xb8, 0,    0,
/* 4y */ 0,    0,    0,    0,    0,    0x45, 0,    0xc7, 0xc8, 0xc9, 0,    0xcb, 0,    0xcd, 0,    0xcf,
/* 5y */ 0xd0, 0xd1, 0xd2, 0xd3, 0,    0,    0,    0,    0,    0,    0,    0x5b, 0x5c, 0x5d, 0,    0,
/* 6y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 7y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 8y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 9y */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x9d, 0,    0,
/* Ay */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* By */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Cy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Dy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Ey */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* Fy */ 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

static void togglePriority(char active)
{
	if (backgroundidle)
		SetPriorityClass( GetCurrentProcess(),
			active ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS );
}

static char mouseInsideWindow()
{
	POINT c; GetCursorPos(&c);
	return (c.x >= windowrct.left && c.x <= windowrct.right
			&& c.y >= windowrct.top && c.y <= windowrct.bottom);
}

void SetNumLock( BOOL bState )
{
	int t = GetKeyState(VK_NUMLOCK);
	if( (bState && !(t & 1)) || (!bState && (t & 1)) )
	{
		// Simulate a key press
		 keybd_event( VK_NUMLOCK,
					  0x45,
					  KEYEVENTF_EXTENDEDKEY | 0,
					  0 );

		// Simulate a key release
		 keybd_event( VK_NUMLOCK,
					  0x45,
					  KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
					  0);
	}
}



//
// WndProcCallback() -- the Windows window callback
//
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// somehow these messages is not working
	// in the main switch when using OpenGL
	// and mouse is not grabbed
	if (appactive && !mousegrab)
	{
		POINT c;
		switch(uMsg)
		{
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				if (mouseInsideWindow())
				{
					grabmouse(1);
					return 0;
				}
				break;
		}
	}
	
#if USE_OPENGL
	if (hGLWindow && hWnd == hGLWindow) return DefWindowProc(hWnd,uMsg,wParam,lParam);
	if (dummyhGLwindow && hWnd == dummyhGLwindow) return DefWindowProc(hWnd,uMsg,wParam,lParam);
#endif
		
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			switch(wParam)
			{
				case SC_MONITORPOWER:
				case SC_SCREENSAVE:
				case SC_KEYMENU:
				case SC_HOTKEY:
					return 0;
			}
			break;
			
		case WM_ACTIVATEAPP:
			appactive = wParam;
			togglePriority(appactive);
			grabmouse(mouseInsideWindow() && appactive);
			return 0;
			
		case WM_ACTIVATE:
			appactive = (LOWORD(wParam) != WA_INACTIVE && HIWORD(wParam) == 0);
			grabmouse(mouseInsideWindow() && appactive);
			togglePriority(appactive);
			return 0;
			
		case WM_KILLFOCUS:
			memset(keystatus, 0, sizeof(keystatus));
			togglePriority(0);
			appactive = 0;
			grabmouse(0);
			return 0;
			
		case WM_SIZE:
			appactive = (wParam != SIZE_MAXHIDE && wParam != SIZE_MINIMIZED);
			togglePriority(appactive);
			return 0;
			
		case WM_DISPLAYCHANGE:
			// desktop settings changed so adjust our world-view accordingly
			desktopxdim = LOWORD(lParam);
			desktopydim = HIWORD(lParam);
			desktopbpp  = wParam;
			resetvideomode();
			getvalidmodes();
			break;
			
		case WM_PAINT:
		case WM_ERASEBKGND:
			break;

		#if MOUSETYPE == 0
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			if (mousegrab)
			{
				POINT c;
				GetCursorPos(&c);
				if (wParam & MK_LBUTTON) mouseb |= 0x01; else mouseb &= ~0x01;
				if (wParam & MK_RBUTTON) mouseb |= 0x02; else mouseb &= ~0x02;
				if (wParam & MK_MBUTTON) mouseb |= 0x04; else mouseb &= ~0x04;
				
				mousex += (c.x - windowcen.x);
				mousey += (c.y - windowcen.y);
			}
			return 0;
			#if SUBSYS > 400
				case WM_MOUSEWHEEL:
					if (mousegrab)
					{
						int i = (((short)HIWORD(wParam)) < 0) ? 1 : 0;
						mousewheel[i] = getticks();
						mouseb |= (16 << i);
					}
					return 0;
			#endif
		#elif MOUSETYPE == 2
		case WM_INPUT:
			if (mousegrab)
			{
				int but;
				RAWINPUT raw; UINT dwSize = sizeof(RAWINPUT);
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)&raw, &dwSize, sizeof(RAWINPUTHEADER));
				if (!mousegrab || raw.header.dwType != RIM_TYPEMOUSE)
					return 0;

				for (but = 0; but < 4; but++) // Sorry XBUTTON2, I didn't plan for you.
				{
					switch ((raw.data.mouse.usButtonFlags >> (but << 1)) & 3) {
						case 1:		// press
							mouseb |= (1 << but);
							if (!mousepresscallback) break;
							mousepresscallback(but, 1);
							break;
						case 2:		// release
							mouseb &= ~(1 << but);
							if (!mousepresscallback) break;
							mousepresscallback(but, 0);
							break;
						default:
							break;	// no change
					}
				}

				if (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
				{
					int direction = (short)raw.data.mouse.usButtonData < 0;	// 1 = down (-ve values), 0 = up

					// Repeated events before the fake button release timer
					// expires need to trigger a release and a new press.
					if (mousewheel[direction] > 0 && mousepresscallback)
						mousepresscallback(5 + direction, 0);

					mousewheel[direction] = getticks();
					mouseb |= (16 << direction);
					if (mousepresscallback)
						mousepresscallback(5 + direction, 1);
				}

				if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
				{
					static LONG absx = 0, absy = 0;
					mousex += raw.data.mouse.lLastX - absx;
					mousey += raw.data.mouse.lLastY - absy;
					
					absx = raw.data.mouse.lLastX;
					absy = raw.data.mouse.lLastY;
				}
				else
				{
					mousex += raw.data.mouse.lLastX;
					mousey += raw.data.mouse.lLastY;
				}
			}
			return 0;
		#endif
		case WM_MOVE:
			UpdateWindowInfo(hWnd);
			return 0;

		case WM_CLOSE:
			quitevent = (onquiteventcallback) ? onquiteventcallback() : 1;
			return 0;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			{
				int press = (lParam & 0x80000000l) == 0;
				int wscan = (lParam >> 16) & 0xff;
				int scan = 0;

				if (lParam & (1<<24))
				{
					scan = wxscantable[wscan];
				}
				else
				{
					scan = wscantable[wscan];
				}

				//buildprintf("VK %-2x VSC %8x scan %-2x = %s\n", wParam, (UINT)lParam, scan, keynames[scan]);

				if (scan == 0)
				{
					// Not a key we want, so give it to the OS to handle.
					break;
				}
				else if (scan == OSD_CaptureKey(-1))
				{
					if (press)
					{
						OSD_ShowDisplay(-1);
						eatosdinput = 1;
					}
				}
				else if (OSD_HandleKey(scan, press) != 0)
				{
					if (!keystatus[scan] || !press)
					{
						if (keypresscallback)
							keypresscallback(scan, press);
					}
					
					SetKey(scan, press);
				}
			}
			return 0;
		case WM_CHAR:
			if (appactive)
			{
				if (eatosdinput)
				{
					eatosdinput = 0;
				}
				else if (OSD_HandleChar((unsigned char)wParam))
				{
					if (((keyasciififoend+1)&(KEYFIFOSIZ-1)) != keyasciififoplc)
					{
						keyasciififo[keyasciififoend] = (unsigned char)wParam;
						keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
						//buildprintf("Char %d, %d-%d\n",wParam,keyasciififoplc,keyasciififoend);
					}
				}
			}
			return 0;
		case WM_HOTKEY:
			return 0;
		case WM_ENTERMENULOOP:
		case WM_ENTERSIZEMOVE:
			return 0;
		case WM_EXITMENULOOP:
		case WM_EXITSIZEMOVE:
			return 0;
		case WM_DESTROY:
			hWindow = 0;
			return 0;
		default:
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


//
// RegisterWindowClass() -- register the window class
//
static BOOL RegisterWindowClass(void)
{
	WNDCLASSEX wcx;

	if (window_class_registered) return FALSE;

	//buildputs("Registering window class\n");

	wcx.cbSize			= sizeof(wcx);
	wcx.style			= CS_OWNDC;
	wcx.lpfnWndProc		= WndProcCallback;
	wcx.cbClsExtra		= 0;
	wcx.cbWndExtra		= 0;
	wcx.hInstance		= hInstance;
	wcx.hIcon			= LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
							GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	wcx.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground 	= CreateSolidBrush(RGB(0,0,0));
	wcx.lpszMenuName 	= NULL;
	wcx.lpszClassName 	= WINDOW_CLASS;
	wcx.hIconSm			= LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
							GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	if (!RegisterClassEx(&wcx))
	{
		ShowErrorBox("Failed to register window class");
		return TRUE;
	}

	window_class_registered = TRUE;
	return FALSE;
}


//
// GetWindowsErrorMsg() -- gives a pointer to a static buffer containing the Windows error message
//
static LPTSTR GetWindowsErrorMsg(DWORD code)
{
	static TCHAR lpMsgBuf[1024];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)lpMsgBuf, 1024, NULL);

	return lpMsgBuf;
}

static const char *getwindowserrorstr(DWORD code)
{
	static char msg[1024];
	memset(msg, 0, sizeof(msg));
	OemToCharBuff(GetWindowsErrorMsg(code), msg, sizeof(msg)-1);
	return msg;
}
