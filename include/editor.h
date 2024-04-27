// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

// This file has been modified from Ken Silverman's original release
// by NoOne

#ifndef __editor_h__
#define __editor_h__

#ifdef __cplusplus
extern "C" {
#endif

#define NUMBUILDKEYS 20

extern int qsetmode;
extern short searchsector, searchwall, searchindex, searchstat;
extern short searchwall2, searchwallcf;
extern int zmode, kensplayerheight;
extern short defaultspritecstat;
extern int posx, posy, posz, horiz;
extern short ang, cursectnum;
extern short ceilingheinum, floorheinum;
extern int zlock;
extern short editstatus, searchit;
extern int searchx, searchy;                          //search input



extern short temppicnum, tempcstat, templotag, temphitag, tempextra, tempxoffset, tempyoffset;
extern unsigned char tempshade, temppal, tempxrepeat, tempyrepeat;
extern unsigned char somethingintab;
extern char names[MAXTILES][25];

//extern int buildkeys[NUMBUILDKEYS];

extern int xdim2d, ydim2d, xdimgame, ydimgame, bppgame, forcesetup;

extern void (*customtimerhandler)(void);

struct startwin_settings {
    int fullscreen;
    int xdim2d, ydim2d;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
};

extern int ExtInit(int argc, char const * const argv[]);
extern void ExtUnInit(void);
extern void ExtPreCheckKeys(void);
extern void ExtAnalyzeSprites(void);
extern void ExtCheckKeys(void);
extern void ExtPreLoadMap(void);
extern void ExtLoadMap(const char *mapname);
extern void ExtPreSaveMap(void);
extern void ExtSaveMap(const char *mapname);


int loadsetup(const char *fn);	// from config.c
int writesetup(const char *fn);	// from config.c


#ifdef __cplusplus
}
#endif

#endif
