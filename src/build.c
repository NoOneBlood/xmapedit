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

#define updatecrc16(crc,dat) (crc = (((crc<<8)&65535)^crctable[((((unsigned short)crc)>>8)&65535)^dat]))
static int crctable[256];
static char kensig[24];

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
extern unsigned char textfont[128][8];

////////////////
int mousxplc, mousyplc;
int boardWidth, boardHeight;
short temppicnum, tempcstat, templotag, temphitag, tempextra, tempxoffset, tempyoffset;
unsigned char tempshade, temppal, tempvis, tempxrepeat, tempyrepeat;
unsigned char somethingintab = 255;
int clipmovemask2d = 0, clipmovemask3d = 0;
char gPreviewMode = 0, gMapLoaded = 0;
int qsetmode = 0;
short joinsector[2];
char gSectorDrawing = 1, gNoclip = 0;
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
short loopinside(int x, int y, short startwall);
short whitelinescan(short dalinehighlight);
void copysector(short soursector, short destsector, short deststartwall, unsigned char copystat);
void overheadeditor(void);

void fixspritesectors(void);
int movewalls(int start, int offs);
int loadnames(void);
void updatenumsprites(void);
void initcrc(void);


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

	Bstrcpy(kensig,"BUILD by Ken Silverman");
	initcrc();

	if (qsetmode != 200)
	{
		overheadeditor();
		keystatus[0x9c] = 0;
	}
	
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

int newnumwalls;
void overheadeditor(void)
{
	int i, j, k, m=0, firstx=0, firsty=0, oposz, col, ch;
	int templong, templong1, templong2;
	int startwall, endwall, dax, day, daz, x1, y1, x2, y2, x3, y3;
	short suckwall=0, sucksect, newnumsectors, split=0, bad;
	short splitsect=0, danumwalls, secondstartwall;
	short splitstartwall=0, splitendwall, loopnum;
	joinsector[0] = joinsector[1] = -1;
	
	/////
	// NoOne:
	// This is all WIP. Trying to get rid off this function
	// Already rewrote most of it functionality in edit2d.cpp and xmpstub.cpp
	//////////////////////////////////////////////////////////

	qsetmodeany(xdim2d,ydim2d);
	oposz = posz;

	highlightcnt = -1;

		//White out all bordering lines of grab that are
		//not highlighted on both sides
	for(i=highlightsectorcnt-1;i>=0;i--)
	{
		startwall = sector[highlightsector[i]].wallptr;
		endwall = startwall + sector[highlightsector[i]].wallnum;
		for(j=startwall;j<endwall;j++)
		{
			if (wall[j].nextwall >= 0)
			{
				for(k=highlightsectorcnt-1;k>=0;k--)
					if (highlightsector[k] == wall[j].nextsector)
						break;
				if (k < 0)
				{
					wall[wall[j].nextwall].nextwall = -1;
					wall[wall[j].nextwall].nextsector = -1;
					wall[j].nextwall = -1;
					wall[j].nextsector = -1;
				}
			}
		}
	}

	for(i=0;i<(MAXWALLS>>3);i++)   //Clear all highlights
		show2dwall[i] = 0;
	for(i=0;i<(MAXSPRITES>>3);i++)
		show2dsprite[i] = 0;

	newnumwalls = -1;
	joinsector[0] = -1;
	keystatus[0x9c] = 0;
	
	while ((keystatus[0x9c]>>1) == 0 && qsetmode != 200 && !quitevent)
	{
		handleevents();
		OSD_DispatchQueued();


		if (newnumwalls >= numwalls)
		{
			dax = mousxplc;
			day = mousyplc;
			adjustmark(&dax,&day,newnumwalls);
			wall[newnumwalls].x = dax;
			wall[newnumwalls].y = day;
		}
		
		
		

		//templong = numwalls;
		//numwalls = newnumwalls;
		//if (numwalls < 0) numwalls = templong;

		begindrawing();	//{{{

		//printcoords16(posx,posy,ang);

		//numwalls = templong;
		
		ExtCheckKeys();
		OSD_Draw();
		
		if (!gPreviewMode)
		{
			bad = (gSectorDrawing && keystatus[0x39] > 0);  //Gotta do this to save lots of 3 spaces!
			
			if (bad > 0)   //Space bar test
			{
				keystatus[0x39] = 0;
				adjustmark(&mousxplc,&mousyplc,newnumwalls);
				if (checkautoinsert(mousxplc,mousyplc,newnumwalls) == 1)
				{
					printmessage16("You must insert a point there first.");
					bad = 0;
				}
			}

			if (bad > 0)  //Space
			{
				if ((newnumwalls < numwalls) && (numwalls < MAXWALLS-1))
				{
					firstx = mousxplc, firsty = mousyplc;  //Make first point
					newnumwalls = numwalls;
					suckwall = -1;
					split = 0;

					//clearbufbyte(&wall[newnumwalls],sizeof(walltype),0L);
					memset(&wall[newnumwalls],0,sizeof(walltype));
					wall[newnumwalls].extra = -1;

					wall[newnumwalls].x = mousxplc;
					wall[newnumwalls].y = mousyplc;
					wall[newnumwalls].nextsector = -1;
					wall[newnumwalls].nextwall = -1;
					for(i=0;i<numwalls;i++)
						if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
							suckwall = i;
					wall[newnumwalls].point2 = newnumwalls+1;
					printmessage16("Sector drawing started.");
					newnumwalls++;
				}
				else
				{  //if not back to first point
					if ((firstx != mousxplc) || (firsty != mousyplc))  //nextpoint
					{
						j = 0;
						for(i=numwalls;i<newnumwalls;i++)
							if ((mousxplc == wall[i].x) && (mousyplc == wall[i].y))
								j = 1;
						if (j == 0)
						{
								//check if starting to split a sector
							if (newnumwalls == numwalls+1)
							{
								dax = ((wall[numwalls].x+mousxplc)>>1);
								day = ((wall[numwalls].y+mousyplc)>>1);
								for(i=0;i<numsectors;i++)
									if (inside(dax,day,i) == 1)
									{    //check if first point at point of sector
										m = -1;
										startwall = sector[i].wallptr;
										endwall = startwall + sector[i].wallnum - 1;
										for(k=startwall;k<=endwall;k++)
											if (wall[k].x == wall[numwalls].x)
												if (wall[k].y == wall[numwalls].y)
												{
													m = k;
													break;
												}
										if (m >= 0)
											if ((wall[wall[k].point2].x != mousxplc) || (wall[wall[k].point2].y != mousyplc))
												if ((wall[lastwall((short)k)].x != mousxplc) || (wall[lastwall((short)k)].y != mousyplc))
												{
													split = 1;
													splitsect = i;
													splitstartwall = m;
													break;
												}
									}
							}

								//make new point

							//make sure not drawing over old red line
							bad = 0;
							for(i=0;i<numwalls;i++)
							{
								if (wall[i].nextwall >= 0)
								{
									if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
										if ((wall[wall[i].point2].x == wall[newnumwalls-1].x) && (wall[wall[i].point2].y == wall[newnumwalls-1].y))
											bad = 1;
									if ((wall[i].x == wall[newnumwalls-1].x) && (wall[i].y == wall[newnumwalls-1].y))
										if ((wall[wall[i].point2].x == mousxplc) && (wall[wall[i].point2].y == mousyplc))
											bad = 1;
								}
							}

							if (bad == 0)
							{
								//clearbufbyte(&wall[newnumwalls],sizeof(walltype),0L);
								memset(&wall[newnumwalls],0,sizeof(walltype));
								wall[newnumwalls].extra = -1;

								wall[newnumwalls].x = mousxplc;
								wall[newnumwalls].y = mousyplc;
								wall[newnumwalls].nextsector = -1;
								wall[newnumwalls].nextwall = -1;
								for(i=0;i<numwalls;i++)
									if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
										suckwall = i;
								wall[newnumwalls].point2 = newnumwalls+1;
								newnumwalls++;
							}
							else
							{
								printmessage16("You can't draw new lines over red lines.");
							}
						}
					}

						//if not split and back to first point
					if ((split == 0) && (firstx == mousxplc) && (firsty == mousyplc) && (newnumwalls >= numwalls+3))
					{
						wall[newnumwalls-1].point2 = numwalls;

						if (suckwall == -1)  //if no connections to other sectors
						{
							k = -1;
							for(i=0;i<numsectors;i++)
								if (inside(firstx,firsty,i) == 1)
									k = i;
							if (k == -1)   //if not inside another sector either
							{              //add island sector
								if (clockdir(numwalls) == 1)
									flipwalls(numwalls,newnumwalls);

								//clearbufbyte(&sector[numsectors],sizeof(sectortype),0L);
								memset(&sector[numsectors],0,sizeof(sectortype));
								sector[numsectors].extra = -1;

								sector[numsectors].wallptr = numwalls;
								sector[numsectors].wallnum = newnumwalls-numwalls;
								sector[numsectors].ceilingz = -(32<<8);
								sector[numsectors].floorz = (32<<8);
								for(i=numwalls;i<newnumwalls;i++)
								{
									wall[i].cstat = 0;
									wall[i].shade = 0;
									wall[i].yrepeat = 8;
									fixrepeats((short)i);
									wall[i].picnum = 0;
									wall[i].overpicnum = 0;
									wall[i].nextsector = -1;
									wall[i].nextwall = -1;
								}
								headspritesect[numsectors] = -1;
								numsectors++;
							}
							else       //else add loop to sector
							{
								if (clockdir(numwalls) == 0)
									flipwalls(numwalls,newnumwalls);

								j = newnumwalls-numwalls;

								sector[k].wallnum += j;
								for(i=k+1;i<numsectors;i++)
									sector[i].wallptr += j;
								suckwall = sector[k].wallptr;

								for(i=0;i<numwalls;i++)
								{
									if (wall[i].nextwall >= suckwall)
										wall[i].nextwall += j;
									if (wall[i].point2 >= suckwall)
										wall[i].point2 += j;
								}

								for(i=newnumwalls-1;i>=suckwall;i--)
									Bmemcpy(&wall[i+j],&wall[i],sizeof(walltype));
								for(i=0;i<j;i++)
									Bmemcpy(&wall[i+suckwall],&wall[i+newnumwalls],sizeof(walltype));

								for(i=suckwall;i<suckwall+j;i++)
								{
									wall[i].point2 += (suckwall-numwalls);

									wall[i].cstat = wall[suckwall+j].cstat;
									wall[i].shade = wall[suckwall+j].shade;
									wall[i].yrepeat = wall[suckwall+j].yrepeat;
									fixrepeats((short)i);
									wall[i].picnum = wall[suckwall+j].picnum;
									wall[i].overpicnum = wall[suckwall+j].overpicnum;

									wall[i].nextsector = -1;
									wall[i].nextwall = -1;
								}
							}
						}
						else
						{
							  //add new sector with connections
							if (clockdir(numwalls) == 1)
								flipwalls(numwalls,newnumwalls);

							//clearbufbyte(&sector[numsectors],sizeof(sectortype),0L);
							memset(&sector[numsectors],0,sizeof(sectortype));
							sector[numsectors].extra = -1;

							sector[numsectors].wallptr = numwalls;
							sector[numsectors].wallnum = newnumwalls-numwalls;
							sucksect = sectorofwall(suckwall);
							sector[numsectors].ceilingstat = sector[sucksect].ceilingstat;
							sector[numsectors].floorstat = sector[sucksect].floorstat;
							sector[numsectors].ceilingxpanning = sector[sucksect].ceilingxpanning;
							sector[numsectors].floorxpanning = sector[sucksect].floorxpanning;
							sector[numsectors].ceilingshade = sector[sucksect].ceilingshade;
							sector[numsectors].floorshade = sector[sucksect].floorshade;
							sector[numsectors].ceilingz = sector[sucksect].ceilingz;
							sector[numsectors].floorz = sector[sucksect].floorz;
							sector[numsectors].ceilingpicnum = sector[sucksect].ceilingpicnum;
							sector[numsectors].floorpicnum = sector[sucksect].floorpicnum;
							sector[numsectors].ceilingheinum = sector[sucksect].ceilingheinum;
							sector[numsectors].floorheinum = sector[sucksect].floorheinum;
							for(i=numwalls;i<newnumwalls;i++)
							{
								wall[i].cstat = wall[suckwall].cstat;
								wall[i].shade = wall[suckwall].shade;
								wall[i].yrepeat = wall[suckwall].yrepeat;
								fixrepeats((short)i);
								wall[i].picnum = wall[suckwall].picnum;
								wall[i].overpicnum = wall[suckwall].overpicnum;
								checksectorpointer((short)i,(short)numsectors);
							}
							headspritesect[numsectors] = -1;
							numsectors++;
						}
						numwalls = newnumwalls;
						newnumwalls = -1;
						asksave = 1;
					}
					if (split == 1)
					{
							 //split sector
						startwall = sector[splitsect].wallptr;
						endwall = startwall + sector[splitsect].wallnum - 1;
						for(k=startwall;k<=endwall;k++)
							if (wall[k].x == wall[newnumwalls-1].x)
								if (wall[k].y == wall[newnumwalls-1].y)
								{
									bad = 0;
									if (loopnumofsector(splitsect,splitstartwall) != loopnumofsector(splitsect,(short)k))
										bad = 1;

									if (bad == 0)
									{
										//SPLIT IT!
										//Split splitsect given: startwall,
										//   new points: numwalls to newnumwalls-2

										splitendwall = k;
										newnumwalls--;  //first fix up the new walls
										for(i=numwalls;i<newnumwalls;i++)
										{
											wall[i].cstat = wall[startwall].cstat;
											wall[i].shade = wall[startwall].shade;
											wall[i].yrepeat = wall[startwall].yrepeat;
											fixrepeats((short)i);
											wall[i].picnum = wall[startwall].picnum;
											wall[i].overpicnum = wall[startwall].overpicnum;

											wall[i].nextwall = -1;
											wall[i].nextsector = -1;
											wall[i].point2 = i+1;
										}

										danumwalls = newnumwalls;  //where to add more walls
										m = splitendwall;          //copy rest of loop next
										while (m != splitstartwall)
										{
											Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
											m = wall[m].point2;
										}
										wall[danumwalls-1].point2 = numwalls;

											//Add other loops for 1st sector
										loopnum = loopnumofsector(splitsect,splitstartwall);
										i = loopnum;
										for(j=startwall;j<=endwall;j++)
										{
											k = loopnumofsector(splitsect,(short)j);
											if ((k != i) && (k != loopnum))
											{
												i = k;
												if (loopinside(wall[j].x,wall[j].y,numwalls) == 1)
												{
													m = j;          //copy loop
													k = danumwalls;
													do
													{
														Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
														wall[danumwalls].point2 = danumwalls+1;
														danumwalls++;
														m = wall[m].point2;
													}
													while (m != j);
													wall[danumwalls-1].point2 = k;
												}
											}
										}

										secondstartwall = danumwalls;
											//copy split points for other sector backwards
										for(j=newnumwalls;j>numwalls;j--)
										{
											Bmemcpy(&wall[danumwalls],&wall[j],sizeof(walltype));
											wall[danumwalls].nextwall = -1;
											wall[danumwalls].nextsector = -1;
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
										}
										m = splitstartwall;     //copy rest of loop next
										while (m != splitendwall)
										{
											Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
											m = wall[m].point2;
										}
										wall[danumwalls-1].point2 = secondstartwall;

											//Add other loops for 2nd sector
										loopnum = loopnumofsector(splitsect,splitstartwall);
										i = loopnum;
										for(j=startwall;j<=endwall;j++)
										{
											k = loopnumofsector(splitsect,(short)j);
											if ((k != i) && (k != loopnum))
											{
												i = k;
												if (loopinside(wall[j].x,wall[j].y,secondstartwall) == 1)
												{
													m = j;          //copy loop
													k = danumwalls;
													do
													{
														Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
														wall[danumwalls].point2 = danumwalls+1;
														danumwalls++;
														m = wall[m].point2;
													}
													while (m != j);
													wall[danumwalls-1].point2 = k;
												}
											}
										}

											//fix all next pointers on old sector line
										for(j=numwalls;j<danumwalls;j++)
										{
											if (wall[j].nextwall >= 0)
											{
												wall[wall[j].nextwall].nextwall = j;
												if (j < secondstartwall)
													wall[wall[j].nextwall].nextsector = numsectors;
												else
													wall[wall[j].nextwall].nextsector = numsectors+1;
											}
										}
											//set all next pointers on split
										for(j=numwalls;j<newnumwalls;j++)
										{
											m = secondstartwall+(newnumwalls-1-j);
											wall[j].nextwall = m;
											wall[j].nextsector = numsectors+1;
											wall[m].nextwall = j;
											wall[m].nextsector = numsectors;
										}
											//copy sector attributes & fix wall pointers
										Bmemcpy(&sector[numsectors],&sector[splitsect],sizeof(sectortype));
										Bmemcpy(&sector[numsectors+1],&sector[splitsect],sizeof(sectortype));
										sector[numsectors].wallptr = numwalls;
										sector[numsectors].wallnum = secondstartwall-numwalls;
										sector[numsectors+1].wallptr = secondstartwall;
										sector[numsectors+1].wallnum = danumwalls-secondstartwall;

											//fix sprites
										j = headspritesect[splitsect];
										while (j != -1)
										{
											k = nextspritesect[j];
											if (loopinside(sprite[j].x,sprite[j].y,numwalls) == 1)
												changespritesect(j,numsectors);
											//else if (loopinside(sprite[j].x,sprite[j].y,secondstartwall) == 1)
											else  //Make sure no sprites get left out & deleted!
												changespritesect(j,numsectors+1);
											j = k;
										}

										numsectors+=2;

											//Back of number of walls of new sector for later
										k = danumwalls-numwalls;

											//clear out old sector's next pointers for clean deletesector
										numwalls = danumwalls;
										for(j=startwall;j<=endwall;j++)
										{
											wall[j].nextwall = -1;
											wall[j].nextsector = -1;
										}
										deletesector(splitsect);

											//Check pointers
										for(j=numwalls-k;j<numwalls;j++)
										{
											if (wall[j].nextwall >= 0)
												checksectorpointer(wall[j].nextwall,wall[j].nextsector);
											checksectorpointer((short)j,sectorofwall((short)j));
										}

											//k now safe to use as temp

										for(m=numsectors-2;m<numsectors;m++)
										{
											j = headspritesect[m];
											while (j != -1)
											{
												k = nextspritesect[j];
												setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
												j = k;
											}
										}

										newnumwalls = -1;
										printmessage16("Sector split.");
										break;
									}
									else
									{
											//Sector split - actually loop joining

										splitendwall = k;
										newnumwalls--;  //first fix up the new walls
										for(i=numwalls;i<newnumwalls;i++)
										{
											wall[i].cstat = wall[startwall].cstat;
											wall[i].shade = wall[startwall].shade;
											wall[i].yrepeat = wall[startwall].yrepeat;
											fixrepeats((short)i);
											wall[i].picnum = wall[startwall].picnum;
											wall[i].overpicnum = wall[startwall].overpicnum;

											wall[i].nextwall = -1;
											wall[i].nextsector = -1;
											wall[i].point2 = i+1;
										}

										danumwalls = newnumwalls;  //where to add more walls
										m = splitendwall;          //copy rest of loop next
										do
										{
											Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
											m = wall[m].point2;
										} while (m != splitendwall);

										//copy split points for other sector backwards
										for(j=newnumwalls;j>numwalls;j--)
										{
											Bmemcpy(&wall[danumwalls],&wall[j],sizeof(walltype));
											wall[danumwalls].nextwall = -1;
											wall[danumwalls].nextsector = -1;
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
										}

										m = splitstartwall;     //copy rest of loop next
										do
										{
											Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
											wall[danumwalls].point2 = danumwalls+1;
											danumwalls++;
											m = wall[m].point2;
										} while (m != splitstartwall);
										wall[danumwalls-1].point2 = numwalls;

											//Add other loops to sector
										loopnum = loopnumofsector(splitsect,splitstartwall);
										i = loopnum;
										for(j=startwall;j<=endwall;j++)
										{
											k = loopnumofsector(splitsect,(short)j);
											if ((k != i) && (k != loopnumofsector(splitsect,splitstartwall)) && (k != loopnumofsector(splitsect,splitendwall)))
											{
												i = k;
												m = j; k = danumwalls;     //copy loop
												do
												{
													Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
													wall[danumwalls].point2 = danumwalls+1;
													danumwalls++;
													m = wall[m].point2;
												} while (m != j);
												wall[danumwalls-1].point2 = k;
											}
										}

											//fix all next pointers on old sector line
										for(j=numwalls;j<danumwalls;j++)
										{
											if (wall[j].nextwall >= 0)
											{
												wall[wall[j].nextwall].nextwall = j;
												wall[wall[j].nextwall].nextsector = numsectors;
											}
										}

											//copy sector attributes & fix wall pointers
										Bmemcpy(&sector[numsectors],&sector[splitsect],sizeof(sectortype));
										sector[numsectors].wallptr = numwalls;
										sector[numsectors].wallnum = danumwalls-numwalls;

											//fix sprites
										j = headspritesect[splitsect];
										while (j != -1)
										{
											k = nextspritesect[j];
											changespritesect(j,numsectors);
											j = k;
										}

										numsectors++;

											//Back of number of walls of new sector for later
										k = danumwalls-numwalls;

											//clear out old sector's next pointers for clean deletesector
										numwalls = danumwalls;
										for(j=startwall;j<=endwall;j++)
										{
											wall[j].nextwall = -1;
											wall[j].nextsector = -1;
										}
										deletesector(splitsect);

											//Check pointers
										for(j=numwalls-k;j<numwalls;j++)
										{
											if (wall[j].nextwall >= 0)
												checksectorpointer(wall[j].nextwall,wall[j].nextsector);
											checksectorpointer((short)j,numsectors-1);
										}

										newnumwalls = -1;
										printmessage16("Loops joined.");
										break;
									}
								}
					}
				}
			}

			if ((keystatus[0x0e] > 0) && (newnumwalls >= numwalls)) //Backspace
			{
				if (newnumwalls > numwalls)
				{
					newnumwalls--;
					asksave = 1;
					keystatus[0x0e] = 0;
				}
				if (newnumwalls == numwalls)
				{
					newnumwalls = -1;
					asksave = 1;
					keystatus[0x0e] = 0;
				}
			}

		}
		
		showframe();
		synctics = totalclock-lockclock;
		lockclock += synctics;

		if (keystatus[0x9c] > 0)
		{
			updatesector(posx,posy,&cursectnum);
			if (gNoclip || cursectnum >= 0) keystatus[0x9c] = 2;
			else
				printmessage16("Arrow must be inside a sector before entering 3D mode.");
		}

		//nextpage();
	}

	for(i=0;i<highlightsectorcnt;i++)
	{
		startwall = sector[highlightsector[i]].wallptr;
		endwall = startwall+sector[highlightsector[i]].wallnum-1;
		for(j=startwall;j<=endwall;j++)
		{
			if (wall[j].nextwall >= 0)
				checksectorpointer(wall[j].nextwall,wall[j].nextsector);
			checksectorpointer((short)j,highlightsector[i]);
		}
	}

	fixspritesectors();

	if (setgamemode(fullscreen,xdimgame,ydimgame,bppgame) < 0)
	{
		ExtUnInit();
		uninitinput();
		uninittimer();
		uninitsystem();
		printf("%d * %d not supported in this graphics mode\n",xdim,ydim);
		exit(0);
	}

	//posz = oposz;
	//searchx = scale(searchx,xdimgame,xdim2d);
	//searchy = scale(searchy,ydimgame,ydim2d-STATUS2DSIZ);
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

int checkautoinsert(int dax, int day, short danumwalls)
{
	int i, x1, y1, x2, y2;

	if (danumwalls < 0)
		danumwalls = numwalls;
	for(i=0;i<danumwalls;i++)       // Check if a point should be inserted
	{
		x1 = wall[i].x;
		y1 = wall[i].y;
		x2 = wall[wall[i].point2].x;
		y2 = wall[wall[i].point2].y;

		if ((x1 != dax) || (y1 != day))
			if ((x2 != dax) || (y2 != day))
				if (((x1 <= dax) && (dax <= x2)) || ((x2 <= dax) && (dax <= x1)))
					if (((y1 <= day) && (day <= y2)) || ((y2 <= day) && (day <= y1)))
						if ((dax-x1)*(y2-y1) == (day-y1)*(x2-x1))
							return(1);          //insertpoint((short)i,dax,day);
	}
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

void fixspritesectors(void)
{
	int i, j, dax, day, daz;

	for(i=numsectors-1;i>=0;i--)
		if ((sector[i].wallnum <= 0) || (sector[i].wallptr >= numwalls))
			deletesector((short)i);

	for(i=0;i<MAXSPRITES;i++)
		if (sprite[i].statnum < MAXSTATUS)
		{
			dax = sprite[i].x;
			day = sprite[i].y;
			if (inside(dax,day,sprite[i].sectnum) != 1)
			{
				daz = ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);

				for(j=0;j<numsectors;j++)
					if (inside(dax,day,(short)j) == 1)
						if (sprite[i].z >= getceilzofslope(j,dax,day))
							if (sprite[i].z-daz <= getflorzofslope(j,dax,day))
							{
								changespritesect((short)i,(short)j);
								break;
							}
			}
		}
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

short loopinside(int x, int y, short startwall)
{
	int x1, y1, x2, y2, templong;
	short i, cnt;

	cnt = clockdir(startwall);
	i = startwall;
	do
	{
		x1 = wall[i].x; x2 = wall[wall[i].point2].x;
		if ((x1 >= x) || (x2 >= x))
		{
			y1 = wall[i].y; y2 = wall[wall[i].point2].y;
			if (y1 > y2)
			{
				templong = x1, x1 = x2, x2 = templong;
				templong = y1, y1 = y2, y2 = templong;
			}
			if ((y1 <= y) && (y2 > y))
				if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
					cnt ^= 1;
		}
		i = wall[i].point2;
	}
	while (i != startwall);
	return(cnt);
}

int numloopsofsector(short sectnum)
{
	int i, numloops, startwall, endwall;

	numloops = 0;
	startwall = sector[sectnum].wallptr;
	endwall = startwall + sector[sectnum].wallnum;
	for(i=startwall;i<endwall;i++)
		if (wall[i].point2 < i) numloops++;
	return(numloops);
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

void copysector(short soursector, short destsector, short deststartwall, unsigned char copystat)
{
	short j, k, m, newnumwalls, startwall, endwall;

	newnumwalls = deststartwall;  //erase existing sector fragments

		//duplicate walls
	startwall = sector[soursector].wallptr;
	endwall = startwall + sector[soursector].wallnum;
	for(j=startwall;j<endwall;j++)
	{
		Bmemcpy(&wall[newnumwalls],&wall[j],sizeof(walltype));
		wall[newnumwalls].point2 += deststartwall-startwall;
		if (wall[newnumwalls].nextwall >= 0)
		{
			wall[newnumwalls].nextwall += deststartwall-startwall;
			wall[newnumwalls].nextsector += destsector-soursector;
		}
		newnumwalls++;
	}

	//for(j=deststartwall;j<newnumwalls;j++)
	//{
	//   if (wall[j].nextwall >= 0)
	//      checksectorpointer(wall[j].nextwall,wall[j].nextsector);
	//   checksectorpointer((short)j,destsector);
	//}

	if (newnumwalls > deststartwall)
	{
			//duplicate sectors
		Bmemcpy(&sector[destsector],&sector[soursector],sizeof(sectortype));
		sector[destsector].wallptr = deststartwall;
		sector[destsector].wallnum = newnumwalls-deststartwall;

		if (copystat == 1)
		{
				//duplicate sprites
			j = headspritesect[soursector];
			while (j >= 0)
			{
				k = nextspritesect[j];

				m = insertsprite(destsector,sprite[j].statnum);
				Bmemcpy(&sprite[m],&sprite[j],sizeof(spritetype));
				sprite[m].sectnum = destsector;   //Don't let memcpy overwrite sector!
				sprite[m].xvel = m;
				j = k;
			}
		}

	}
}

void printmessage16(char name[82])
{
	if (printmessage16_replace)
	{
		printmessage16_replace(name);
		return;
	}
}


void initcrc(void)
{
	int i, j, k, a;

	for(j=0;j<256;j++)      //Calculate CRC table
	{
		k = (j<<8); a = 0;
		for(i=7;i>=0;i--)
		{
			if (((k^a)&0x8000) > 0)
				a = ((a<<1)&65535) ^ 0x1021;   //0x1021 = genpoly
			else
				a = ((a<<1)&65535);
			k = ((k<<1)&65535);
		}
		crctable[j] = (a&65535);
	}
}


/*
 * vim:ts=4:
 */

