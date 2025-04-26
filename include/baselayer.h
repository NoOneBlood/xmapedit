// Base services interface declaration
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef __baselayer_h__
#define __baselayer_h__

#ifdef __cplusplus
extern "C" {
#endif

extern int _buildargc;
extern const char **_buildargv;
extern const char* helpfilename;
extern const char helpkey;

extern char quitevent, appactive;
extern char (*onquiteventcallback)(void);
extern char scanremap[256];



// NOTE: these are implemented in game-land so they may be overridden in game specific ways
extern int app_main(int argc, char const * const argv[]);
#ifdef HAVE_START_WINDOW
enum {
    STARTWIN_CANCEL = 0,
    STARTWIN_RUN = 1,
};
struct startwin_settings;
extern int startwin_open(void);
extern int startwin_close(void);
extern int startwin_puts(const char *);
extern int startwin_settitle(const char *);
extern int startwin_idle(void *);
extern int startwin_run(struct startwin_settings *);
#endif

// video
extern int xres, yres, bpp, fullscreen, bytesperline, imageSize;
extern char offscreenrendering;
extern intptr_t frameplace;

extern void (*baselayer_onvideomodechange)(int);

#if USE_OPENGL
typedef struct {
	int loaded;

	int majver;	// GL version
	int minver;
	int glslmajver;	// 0 = no support
	int glslminver;

	float maxanisotropy;
	char bgra;
	char clamptoedge;
	char texcomprdxt1;
	char texcomprdxt5;
	char texcompretc1;
	char texnpot;
	char multisample;
	char nvmultisamplehint;

	int multitex;
	int maxtexsize;
	int maxvertexattribs;
	char debugext;
} baselayer_glinfo;
extern baselayer_glinfo glinfo;
#endif

extern int inputdevices;

// keys
#define KEYFIFOSIZ 64
extern char keystatus[256], keyholdtime[256];
extern unsigned char keyfifo[KEYFIFOSIZ], keyhitfifo[KEYFIFOSIZ], keyasciififo[KEYFIFOSIZ];
extern unsigned char keyfifoplc, keyfifoend, keyhitfifoplc, keyhitfifoend;
extern unsigned char keyasciififoplc, keyasciififoend;

// mouse
extern int mousex, mousey, mouseb;

// joystick
extern int joyaxis[], joyb;
extern char joynumaxes, joynumbuttons;



int initsystem(void);
void uninitsystem(void);

void initputs(const char *);
void debugprintf(const char *,...) PRINTF_FORMAT(1, 2);

int handleevents(void);

typedef void (*KeyPressCallback)(int,int);
typedef void (*MousePressCallback)(int,int);
typedef void (*JoyPressCallback)(int,int);
int initinput(void);
void uninitinput(void);
void releaseallbuttons(void);
void setkeypresscallback(void (*callback)(int,int));
void setmousepresscallback(void (*callback)(int,int));
void setjoypresscallback(void (*callback)(int,int));
const char *getkeyname(int num);
const char *getjoyname(int what, int num);	// what: 0=axis, 1=button, 2=hat

unsigned char bgetchar(void);
int bkbhit(void);
void bflushchars(void);

int initmouse(void);
void uninitmouse(void);
void grabmouse(int a);
void readmousexy(int *x, int *y);
void readmousebstatus(int *b);

int inittimer(int);
void uninittimer(void);
void sampletimer(void);
unsigned int getticks(void);
unsigned int getusecticks(void);
int gettimerfreq(void);
void (*installusertimercallback(void (*callback)(void)))(void);

int checkvideomode(int *x, int *y, int c, int fs, int forced);
int setvideomode(int x, int y, int c, int fs);
void getvalidmodes(void);
void resetvideomode(void);

void begindrawing(void);
void enddrawing(void);
void showframe(void);

int setpalette(int start, int num, unsigned char *dapal);
int setgamma(float gamma);

int wm_msgbox(const char *name, const char *fmt, ...) PRINTF_FORMAT(2, 3);
int wm_ynbox(const char *name, const char *fmt, ...) PRINTF_FORMAT(2, 3);

// initialdir - the initial directory
// initialfile - the initial filename
// type - the file extension to choose (e.g. "map")
// foropen - boolean true, or false if for saving
// choice - the file chosen by the user to be free()'d when done
// Returns -1 if not supported, 0 if cancelled, 1 if accepted
int wm_filechooser(const char *initialdir, const char *initialfile, const char *type, int foropen, char **choice);

int wm_idle(void *);
void wm_setapptitle(const char *name);
void wm_setwindowtitle(const char *name);
void wm_showwindow();
void wm_hidewindow();
void wm_showhelp(const char* name, int nCommand);
void wm_remapkey(unsigned char key, unsigned char newkey);

#if USE_OPENGL
int loadgldriver(const char *driver);   // or NULL for platform default
void *getglprocaddress(const char *name, int ext);
int unloadgldriver(void);
#endif

// baselayer.c
int baselayer_init(void);
#if USE_OPENGL
int baselayer_setupopengl(void);
#endif

void makeasmwriteable(void);

#ifdef __cplusplus
}
#endif

#endif // __baselayer_h__

