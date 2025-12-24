// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by NoOne

#include "build.h"
#include "pragmas.h"
#include "osd.h"
#include "cache1d.h"
#include "editor.h"

#include "baselayer.h"
#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif


#define TIMERINTSPERSECOND 120


static int synctics = 0, lockclock = 0;
int vel = 0, hvel = 0, svel = 0, angvel = 0, doubvel = 0;
int posx, posy, posz, horiz = 100;		short ang, cursectnum;
int startposx, startposy, startposz;	short startang, startsectnum;
int xdimgame = 640, ydimgame = 480, bppgame = 8;
int grponlymode = 0, forcesetup = 1;

int zlock = 0x7fffffff, zmode = 0, kensplayerheight = 32;
short defaultspritecstat = 0, asksave = 0, editstatus, searchit;
short searchsector, searchwall, searchindex, searchstat;     //search output
int searchx, searchy;                          //search input

short pointhighlight, linehighlight, highlightcnt;
short grid = 3, gridlock = 1;
int zoom = 768, numsprites, mapversion;

short highlight[MAXWALLS + MAXSPRITES];
short highlightsector[MAXSECTORS], highlightsectorcnt = -1;

////////////////
int mousxplc, mousyplc;
int boardWidth, boardHeight;
short temppicnum, tempcstat, templotag, temphitag, tempextra, tempxoffset, tempyoffset;
unsigned char tempshade, temppal, tempvis, tempxrepeat, tempyrepeat;
unsigned char somethingintab = 255;
int clipmovemask2d = 0, clipmovemask3d = 0;
char gPreviewMode = 0, gMapLoaded = 0;
int qsetmode = 0;
char gNoclip = 0;
int clipmoveboxtracenum = 3;
int xdim2d, ydim2d;
//////////////

void (*customtimerhandler)(void);



unsigned char changechar(unsigned char dachar, int dadir, unsigned char smooshyalign, unsigned char boundcheck);
void flipwalls(short numwalls, short newnumwalls);
void insertpoint(short linehighlight, int dax, int day);
void deletepoint(short point);
int deletesector(short sucksect);
int checksectorpointer(short i, short sectnum);
void fixrepeats(short i);
short whitelinescan(short dalinehighlight);
void copysector(short soursector, short destsector, short deststartwall, unsigned char copystat);

int movewalls(int start, int offs);
void updatenumsprites(void);


void clearkeys(void) { memset(keystatus,0,sizeof(keystatus)); }

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
	extern int qsetmode;

	if (qsetmode != 200) return OSDCMD_OK;

	resetvideomode();
	if (setgamemode(fullscreen,xdim,ydim,bpp))
		buildputs("restartvid: Reset failed...\n");

	return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
	int newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen;
	extern int qsetmode;

	if (qsetmode != 200) return OSDCMD_OK;

	if (parm->numparms < 1 || parm->numparms > 4) {
		return OSDCMD_SHOWHELP;
	}

	if (parm->numparms == 4) {
		// fs, res, bpp switch
		newfullscreen = (Batol(parm->parms[3]) != 0);
	}
	if (parm->numparms >= 3) {
		// res & bpp switch
		newbpp = Batol(parm->parms[2]);
	}
	if (parm->numparms >= 2) {
		// res switch
		newy = Batol(parm->parms[1]);
		newx = Batol(parm->parms[0]);
	}
	if (parm->numparms == 1) {
		// bpp switch
		newbpp = Batol(parm->parms[0]);
	}

	if (setgamemode(newfullscreen,newx,newy,newbpp))
		buildputs("vidmode: Mode change failed!\n");
	xdimgame = newx;
	ydimgame = newy;
	bppgame = newbpp;
	fullscreen = newfullscreen;
	return OSDCMD_OK;
}

extern char *defsfilename;	// set in bstub.c
int app_main(int argc, char const * const argv[])
{
	char ch, cmdsetup = 0;
    struct startwin_settings settings;
	int i, j, k, dark, light;

	pathsearchmode = PATHSEARCH_SYSTEM;		// unrestrict findfrompath so that full access to the filesystem can be had

	OSD_RegisterFunction("restartvid","restartvid: reinitialise the video mode",osdcmd_restartvid);
	OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: immediately change the video mode",osdcmd_vidmode);

	//wm_setapptitle("BUILD by Ken Silverman");

#ifdef RENDERTYPEWIN
	backgroundidle = 1;
#endif

	editstatus = 1;
	totalclock = 0;
   
	inittimer(TIMERINTSPERSECOND);
	//installusertimercallback((customtimerhandler) ? customtimerhandler : keytimerstuff);

#ifdef HAVE_START_WINDOW
		int startretval = STARTWIN_RUN;
		memset(&settings, 0, sizeof(settings));
		settings.fullscreen = fullscreen;
		settings.xdim2d = xdim2d;
		settings.ydim2d = ydim2d;
		settings.xdim3d = xdimgame;
		settings.ydim3d = ydimgame;
		settings.bpp3d = bppgame;
		settings.forcesetup = forcesetup;

	#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && (defined __APPLE__ || defined HAVE_GTK))
		if (i || forcesetup || cmdsetup)
		{
			if (quitevent) return 0;
			startretval = startwin_run(&settings);
			if (startretval == STARTWIN_CANCEL)
				return 0;
		}
	#endif

		fullscreen = settings.fullscreen;
		xdim2d = settings.xdim2d;
		ydim2d = settings.ydim2d;
		xdimgame = settings.xdim3d;
		ydimgame = settings.ydim3d;
		bppgame = settings.bpp3d;
		forcesetup = settings.forcesetup;
#endif

	
	if ((i = ExtInit(argc, argv)) < 0)
		return -1;

	while (!quitevent)
	{
		handleevents();
		OSD_DispatchQueued();
		
		ExtPreCheckKeys();

		drawrooms(posx,posy,posz,ang,horiz,cursectnum);
		ExtAnalyzeSprites();
		drawmasks();

		ExtCheckKeys();

		nextpage();
		synctics = totalclock - lockclock;
		lockclock += synctics;
	}

	ExtUnInit();
	uninitengine();
	return(0);
}

unsigned char changechar(unsigned char dachar, int dadir, unsigned char smooshyalign, unsigned char boundcheck)
{
	if (dadir < 0)
	{
		if ((dachar > 0) || (boundcheck == 0))
		{
			dachar--;
			if (smooshyalign > 0)
				dachar = (dachar&0xf8);
		}
	}
	else if (dadir > 0)
	{
		if ((dachar < 255) || (boundcheck == 0))
		{
			dachar++;
			if (smooshyalign > 0)
			{
				if (dachar >= 256-4) dachar = 255;
				else dachar = ((dachar+7)&0xf8);
			}
		}
	}
	return(dachar);
}

int adjustmark(int *xplc, int *yplc, short danumwalls)
{
	int i, dst, dist, dax, day, pointlockdist;

	if (danumwalls < 0)
		danumwalls = numwalls;

	pointlockdist = 0;
	if ((grid > 0) && (gridlock > 0))
		pointlockdist = (128>>grid);

	dist = pointlockdist;
	dax = *xplc;
	day = *yplc;
	for(i=0;i<danumwalls;i++)
	{
		dst = klabs((*xplc)-wall[i].x) + klabs((*yplc)-wall[i].y);
		if (dst < dist)
		{
			dist = dst;
			dax = wall[i].x;
			day = wall[i].y;
		}
	}
	if (dist == pointlockdist)
		if ((gridlock > 0) && (grid > 0))
		{
			dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
			day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
		}

	*xplc = dax;
	*yplc = day;
	return(0);
}



int clockdir(short wallstart)   //Returns: 0 is CW, 1 is CCW
{
	short i, themin;
	int minx, templong, x0, x1, x2, y0, y1, y2;

	minx = 0x7fffffff;
	themin = -1;
	i = wallstart-1;
	do
	{
		i++;
		if (wall[wall[i].point2].x < minx)
		{
			minx = wall[wall[i].point2].x;
			themin = i;
		}
	}
	while ((wall[i].point2 != wallstart) && (i < MAXWALLS));

	x0 = wall[themin].x;
	y0 = wall[themin].y;
	x1 = wall[wall[themin].point2].x;
	y1 = wall[wall[themin].point2].y;
	x2 = wall[wall[wall[themin].point2].point2].x;
	y2 = wall[wall[wall[themin].point2].point2].y;

	if ((y1 >= y2) && (y1 <= y0)) return(0);
	if ((y1 >= y0) && (y1 <= y2)) return(1);

	templong = (x0-x1)*(y2-y1) - (x2-x1)*(y0-y1);
	if (templong < 0)
		return(0);
	else
		return(1);
}

void flipwalls(short numwalls, short newnumwalls)
{
	int i, j, nume, templong;

	nume = newnumwalls-numwalls;

	for(i=numwalls;i<numwalls+(nume>>1);i++)
	{
		j = numwalls+newnumwalls-i-1;
		templong = wall[i].x; wall[i].x = wall[j].x; wall[j].x = templong;
		templong = wall[i].y; wall[i].y = wall[j].y; wall[j].y = templong;
	}
}

void insertpoint(short linehighlight, int dax, int day)
{
	short sucksect;
	int i, j, k;

	j = linehighlight;
	sucksect = sectorofwall((short)j);

	sector[sucksect].wallnum++;
	for(i=sucksect+1;i<numsectors;i++)
		sector[i].wallptr++;

	movewalls((int)j+1,+1L);
	Bmemcpy(&wall[j+1],&wall[j],sizeof(walltype));

	wall[j].point2 = j+1;
	wall[j+1].x = dax;
	wall[j+1].y = day;
	fixrepeats((short)j);
	fixrepeats((short)j+1);

	if (wall[j].nextwall >= 0)
	{
		k = wall[j].nextwall;

		sucksect = sectorofwall((short)k);

		sector[sucksect].wallnum++;
		for(i=sucksect+1;i<numsectors;i++)
			sector[i].wallptr++;

		movewalls((int)k+1,+1L);
		Bmemcpy(&wall[k+1],&wall[k],sizeof(walltype));

		wall[k].point2 = k+1;
		wall[k+1].x = dax;
		wall[k+1].y = day;
		fixrepeats((short)k);
		fixrepeats((short)k+1);

		j = wall[k].nextwall;
		wall[j].nextwall = k+1;
		wall[j+1].nextwall = k;
		wall[k].nextwall = j+1;
		wall[k+1].nextwall = j;
	}
}

void deletepoint(short point)
{
	int i, j, k, sucksect;

	sucksect = sectorofwall(point);

	sector[sucksect].wallnum--;
	for(i=sucksect+1;i<numsectors;i++)
		sector[i].wallptr--;

	j = lastwall(point);
	k = wall[point].point2;
	wall[j].point2 = k;

	if (wall[j].nextwall >= 0)
	{
		wall[wall[j].nextwall].nextwall = -1;
		wall[wall[j].nextwall].nextsector = -1;
	}
	if (wall[point].nextwall >= 0)
	{
		wall[wall[point].nextwall].nextwall = -1;
		wall[wall[point].nextwall].nextsector = -1;
	}
	movewalls((int)point,-1L);

	checksectorpointer((short)j,(short)sucksect);
}

int deletesector(short sucksect)
{
	int i, j, k, nextk, startwall, endwall;

	while (headspritesect[sucksect] >= 0)
		deletesprite(headspritesect[sucksect]);
	updatenumsprites();

	startwall = sector[sucksect].wallptr;
	endwall = startwall + sector[sucksect].wallnum - 1;
	j = sector[sucksect].wallnum;

	for(i=sucksect;i<numsectors-1;i++)
	{
		k = headspritesect[i+1];
		while (k != -1)
		{
			nextk = nextspritesect[k];
			changespritesect((short)k,(short)i);
			k = nextk;
		}

		Bmemcpy(&sector[i],&sector[i+1],sizeof(sectortype));
		sector[i].wallptr -= j;
	}
	numsectors--;

	j = endwall-startwall+1;
	for (i=startwall;i<=endwall;i++)
		if (wall[i].nextwall != -1)
		{
			wall[wall[i].nextwall].nextwall = -1;
			wall[wall[i].nextwall].nextsector = -1;
		}
	movewalls(startwall,-j);
	for(i=0;i<numwalls;i++)
		if (wall[i].nextwall >= startwall)
			wall[i].nextsector--;
	return(0);
}

int movewalls(int start, int offs)
{
	int i;

	if (offs < 0)  //Delete
	{
		for(i=start;i<numwalls+offs;i++)
			Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
	}
	else if (offs > 0)  //Insert
	{
		for(i=numwalls+offs-1;i>=start+offs;i--)
			Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
	}
	numwalls += offs;
	for(i=0;i<numwalls;i++)
	{
		if (wall[i].nextwall >= start) wall[i].nextwall += offs;
		if (wall[i].point2 >= start) wall[i].point2 += offs;
	}
	return(0);
}

int checksectorpointer(short i, short sectnum)
{
	int j, k, startwall, endwall, x1, y1, x2, y2;

	x1 = wall[i].x;
	y1 = wall[i].y;
	x2 = wall[wall[i].point2].x;
	y2 = wall[wall[i].point2].y;

	if (wall[i].nextwall >= 0)          //Check for early exit
	{
		k = wall[i].nextwall;
		if ((wall[k].x == x2) && (wall[k].y == y2))
			if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
				return(0);
	}

	wall[i].nextsector = -1;
	wall[i].nextwall = -1;
	for(j=0;j<numsectors;j++)
	{
		startwall = sector[j].wallptr;
		endwall = startwall + sector[j].wallnum - 1;
		for(k=startwall;k<=endwall;k++)
		{
			if ((wall[k].x == x2) && (wall[k].y == y2))
				if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
					if (j != sectnum)
					{
						wall[i].nextsector = j;
						wall[i].nextwall = k;
						wall[k].nextsector = sectnum;
						wall[k].nextwall = i;
					}
		}
	}
	return(0);
}

void fixrepeats(short i)
{
	int dax, day, dist;

	dax = wall[wall[i].point2].x-wall[i].x;
	day = wall[wall[i].point2].y-wall[i].y;
	dist = ksqrt(dax*dax+day*day);
	dax = wall[i].xrepeat; day = wall[i].yrepeat;
	wall[i].xrepeat = (unsigned char)min(max(mulscale10(dist,day),1),255);
}

short whitelinescan(short dalinehighlight)
{
	int i, j, k;
	short sucksect, newnumwalls;

	sucksect = sectorofwall(dalinehighlight);

	Bmemcpy(&sector[numsectors],&sector[sucksect],sizeof(sectortype));
	sector[numsectors].wallptr = numwalls;
	sector[numsectors].wallnum = 0;
	i = dalinehighlight;
	newnumwalls = numwalls;
	do
	{
		j = lastwall((short)i);
		if (wall[j].nextwall >= 0)
		{
			j = wall[j].point2;
			for(k=0;k<numwalls;k++)
			{
				if (wall[wall[k].point2].x == wall[j].x)
					if (wall[wall[k].point2].y == wall[j].y)
						if (wall[k].nextwall == -1)
						{
							j = k;
							break;
						}
			}
		}

		Bmemcpy(&wall[newnumwalls],&wall[i],sizeof(walltype));

		wall[newnumwalls].nextwall = j;
		wall[newnumwalls].nextsector = sectorofwall((short)j);

		newnumwalls++;
		sector[numsectors].wallnum++;

		i = j;
	}
	while (i != dalinehighlight);

	for(i=numwalls;i<newnumwalls-1;i++)
		wall[i].point2 = i+1;
	wall[newnumwalls-1].point2 = numwalls;

	if (clockdir(numwalls) == 1)
		return(-1);
	else
		return(newnumwalls);
}

void updatenumsprites(void)
{
	int i;

	numsprites = 0;
	for(i=0;i<MAXSPRITES;i++)
		if (sprite[i].statnum < MAXSTATUS)
			numsprites++;
}

/*
 * vim:ts=4:
 */

