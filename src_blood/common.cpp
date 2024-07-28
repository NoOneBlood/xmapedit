/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Originally written by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// Various functions and global definitions for XMAPEDIT
//
// This file is part of XMAPEDIT.
//
// XMAPEDIT is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License version 2
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
////////////////////////////////////////////////////////////////////////////////////
***********************************************************************************/

#include "common_game.h"
#include "gameutil.h"
#include "xmpconf.h"

PICANM* panm;
int gFrameClock = 0;
int gFrameTicks = 0;
int gFrame = 0;
int gFrameRate = 0;
int gGamma = 0;

Resource gSysRes;

static const char *_module;
static int _line;
unsigned int randSeed = 1;

int costable[2048];
BYTE OctantTable[8] = { 5, 6, 2, 1, 4, 7, 3, 0 };

#if USE_OPENGL
// test for glSwapInterval extension
char GL_swapIntervalTest()
{
	char tmp[16]; const char* tp = tmp;
	osdfuncparm_t arg;
	
	if (nogl)
		return 0;
	
	memset(&arg, 0, sizeof(arg));
	sprintf(tmp, "%d", glswapinterval);
	arg.numparms = 1;
	arg.parms = &tp;
	
	return (set_glswapinterval(&arg) == OSDCMD_OK);
}

int GL_swapInterval2Fps(int interval)
{
	int f = desktopfreq;
	if (f & 2)
		f++;
	
	dassert(interval > 0);
	return f / interval;
}

int GL_fps2SwapInterval(int fps)
{
	char tmp[16]; const char* tp = tmp;
	int val = 0, f = desktopfreq;
	osdfuncparm_t arg;
	
	if (f & 2)
		f++;

	switch(fps)
	{
		// no limit
		case -1:
			val = 0;
			break;	
		// display freq (v-sync)
		case -2:
			val = 1;
			break;
		// clip glswapinterval
		default:
			val = ClipLow(f / ClipLow(fps, 1), 1);
			break;
	}
	
	memset(&arg, 0, sizeof(arg));
	sprintf(tmp, "%d", val);
	arg.numparms = 1;
	arg.parms = &tp;

	set_glswapinterval(&arg);
	return glswapinterval;
}
#endif

void GetSpriteExtents(spritetype* pSpr, int *top, int *bot)
{
   int nTile, nSizeY, nYoff;

    *top = *bot = pSpr->z;
	if ((pSpr->cstat & kSprRelMask) == kSprFloor)
		return;
	
	nTile = pSpr->picnum; nSizeY = tilesizy[nTile];
	
	nYoff = panm[nTile].ycenter;
	if ((pSpr->cstat & kSprFlipY) && (pSpr->cstat & kSprRelMask) == kSprWall)
		nYoff = -nYoff;
	
	*bot = pSpr->z - ((nYoff * pSpr->yrepeat) << 2);
	if (pSpr->cstat & kSprOrigin)
	{
		*bot += ((nSizeY * pSpr->yrepeat) << 1);
		if (nSizeY & 1)
			*bot += (pSpr->yrepeat << 1);        //Odd yspans
	}

	*top = *bot - ((nSizeY * pSpr->yrepeat) << 2);
}

unsigned int qrand(void)
{
    if (randSeed&0x80000000)
        randSeed = ((randSeed<<1)^0x20000004)|0x1;
    else
        randSeed = randSeed<<1;
    return randSeed&0x7fff;
}

int GetOctant(int x, int y)
{
    int vc = klabs(x)-klabs(y);
    return OctantTable[7-(x<0)-(y<0)*2-(vc<0)*4];
}

void RotateVector(int *dx, int *dy, int nAngle)
{
    int ox = *dx;
    int oy = *dy;
    *dx = dmulscale30r(ox, Cos(nAngle), -oy, Sin(nAngle));
    *dy = dmulscale30r(ox, Sin(nAngle), oy, Cos(nAngle));
}

void RotatePoint(int *x, int *y, int nAngle, int ox, int oy)
{
    int dx = *x-ox;
    int dy = *y-oy;
    *x = ox+dmulscale30r(dx, Cos(nAngle), -dy, Sin(nAngle));
    *y = oy+dmulscale30r(dx, Sin(nAngle), dy, Cos(nAngle));
}

void trigInit(Resource &Res)
{
    int i;
	DICTNODE *pTable = Res.Lookup("cosine","dat");
	if (!pTable)				ThrowError("Cosine table not found");
	if (pTable->size != 2048)	ThrowError("Cosine table incorrect size");	
    memcpy(costable, Res.Load(pTable), pTable->size);
#if B_BIG_ENDIAN == 1
    for (i = 0; i < 512; i++)			costable[i] = B_LITTLE32(costable[i]);
#endif
    costable[512] = 0;
    for (i = 513; i <= 1024; i++)		costable[i] = -costable[1024-i];
    for (i = 1025; i < 2048; i++)		costable[i] = costable[2048 - i];
}

void ChangeExtension(char *pzFile, const char *pzExt)
{
	char ext[_MAX_PATH];
	int const nLength = strlen(pzFile);
    char *pDot = pzFile+nLength;
	int i = 0;

	if (pzExt[0] != '.')
		ext[0] = '.', i++;
	
	sprintf(&ext[i], pzExt);
    for (i = nLength-1; i >= 0; i--)
    {
        if (pzFile[i] == '/' || pzFile[i] == '\\') break;
        else if (pzFile[i] == '.')
        {
            pDot = pzFile+i;
            break;
        }
    }
    *pDot = '\0';
    strcat(pDot, ext);
}

bool FileLoad(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;
	if ((hFile = open(fname, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) < 0)
		return FALSE;
	
	n = read(hFile, buffer, size);
	close(hFile);

	return (n == size);
}

bool FileSave(char *fname, void *buffer, unsigned int size)
{
	int hFile, n;
	if ((hFile = open(fname, O_CREAT|O_WRONLY|O_BINARY|O_TRUNC, S_IREAD|S_IWRITE)) < 0)
		return FALSE;

	n = write(hFile, buffer, size);
	close(hFile);

	return (n == size);
}

char fileExists(char* filename, RESHANDLE* rffItem)
{
	char retn = 0;
	if (!filename) return retn;
	else if (access(filename, F_OK) >= 0) // filename on the disk?
		retn |= 0x0001;
	
	if (rffItem)
	{
		int len, i;
		char tmp[BMAX_PATH]; char *fname = NULL, *fext = NULL;
		pathSplit2(filename, tmp, NULL, NULL, &fname, &fext);
		
		if (!fname || !fext) return retn;
		else if ((len = strlen(fext)) > 0)
		{
			if (fext[0] == '.')
				fext =& fext[1];
			
			if ((*rffItem = gSysRes.Lookup(fname, fext)) != NULL) // filename in RFF?
			{
				retn |= 0x0002;
			}
			else
			{
				// fileID in RFF?
				len = strlen(fname);
				for (i = 0; i < len; i++) {
					if (fname[i] < 48 || fname[i] > 57)
						break;
				}
				
				if (i == len && (*rffItem = gSysRes.Lookup(atoi(fname), fext)) != NULL)
					retn |= 0x0004;
			}
		}
	}
	
	return retn;
}

char isDir(char* pPath)
{
	struct stat s;
	return (stat(pPath, &s) == 0 && (s.st_mode & _S_IFDIR) != 0);
}

char isFile(char* pPath)
{
	struct stat s;
	return (stat(pPath, &s) == 0 && (s.st_mode & _S_IFDIR) == 0);
}

int fileLoadHelper(char* filepath, BYTE** out, int* loadFrom)
{
	RESHANDLE pFile;
	int i, hFile, nSize = 0;
	dassert(out != NULL && *out == NULL);
	if (loadFrom)
		*loadFrom = -1;
	
	
	if ((i = fileExists(filepath, &pFile)) == 0)
		return -1;
	
	// file on the disk is the priority
	if ((i & 0x01) && (hFile = open(filepath, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) >= 0)
	{
		if ((nSize = filelength(hFile)) > 0 && (*out = (BYTE*)malloc(nSize)) != NULL)
		{
			read(hFile, *out, nSize);
			if (loadFrom)
				*loadFrom = 0x01;
		}
		
		close(hFile);
	}
	// load file from the RFF then
	else if (pFile)
	{
		if ((nSize = gSysRes.Size(pFile)) > 0 && (*out = (BYTE*)malloc(nSize)) != NULL)
		{
			memcpy(*out, (BYTE*)gSysRes.Load(pFile), nSize);
			if (loadFrom)
				*loadFrom = 0x02;
		}
	}
	
	return (*out) ? nSize : -2;
}

SCANDIRENT* dirScan(char* path, char* filter, int* numfiles, int* numdirs, char flags)
{
	BDIR* pDir; Bdirent* pFile;
	SCANDIRENT* db = NULL, tmp;
	char *ext, *p, buf[BMAX_PATH];
	char newpath[BMAX_PATH];
	
	int i, t;
	
	if (numdirs)	*numdirs = 0;
	if (numfiles)	*numfiles = 0;
	
	if (!isempty(filter))
	{
		if (filter[0] == '.')
			filter = &filter[1];
		
		enumStrGetChar(0, buf, (char*)filter, '.', NULL);
	}

	if (isempty(path))
		path = ".";
	
	if (flags & 0x01)
	{
		strcpy(newpath, path); pathCatSlash(newpath);
		_fullpath(newpath, newpath, BMAX_PATH);
		path = newpath;
	}
	
	t = 0;
	if ((pDir = Bopendir(path)) != NULL)
	{
		while ((pFile = Breaddir(pDir)) != NULL)
		{
			memset(&tmp, 0, sizeof(tmp));
			
			if (pFile->mode & BS_IFDIR)
			{
				if (!numdirs) continue;
				if (strcmp(pFile->name, "..") == 0)	continue;
				if (strcmp(pFile->name, ".")  == 0)	continue;
				*numdirs = *numdirs + 1;
			}
			else if (numfiles)
			{
				i = 0; ext = NULL;
				pathSplit2(pFile->name, tmp.name, NULL, NULL, NULL, &ext);
				if (!isempty(filter))
				{
					while((p = enumStrGetChar(i++, buf, NULL, '.')) != NULL)
					{
						if (ext[0] == '.') ext = &ext[1];
						if (stricmp(ext, buf) == 0) break;
					}
					
					if (!p)
						continue;
				}
				
				if (!isempty(ext))
					strcpy(tmp.type, ext);

				*numfiles = *numfiles + 1;
			}
			else
			{
				continue;
			}
			
			if (flags & 0x01)
			{
				strcpy(tmp.full, path);
				strcat(tmp.full, pFile->name);
			}
			
			strcpy(tmp.name, pFile->name);
			tmp.flags = pFile->mode;
			tmp.mtime = pFile->mtime;
			
			db = (SCANDIRENT*)realloc(db, sizeof(SCANDIRENT) * (t + 1));
			dassert(db != NULL);
			
			memcpy(&db[t++], &tmp, sizeof(SCANDIRENT));
		}
		
		Bclosedir(pDir);
	}
	
	return db;
}

SCANDIRENT* driveScan(int* numdrives)
{
	SCANDIRENT *drives = NULL, *pDrv;
	char *drv, *drp;
	int i = 0;
	
	drv = drp = Bgetsystemdrives();
	*numdrives = 0;
	
	if (drv)
	{
		while(*drp)
		{
			drives = (SCANDIRENT*)realloc(drives, sizeof(SCANDIRENT)*(i+1));
			dassert(drives != NULL);
			
			pDrv = &drives[i];
			memset(pDrv, 0, sizeof(SCANDIRENT));
			strcpy(pDrv->full, drp); strcpy(pDrv->name, drp);
			drp+=strlen(drp)+1;
			i++;
		}
		
		*numdrives = i;
		free(drv);
	}
	
	return drives;
}

char dirRemoveRecursive(char* path)
{
	BDIR* pDir; Bdirent* pFile;
	static char cwd[BMAX_PATH];
	
	if (chdir(path) == 0)
	{
		if ((pDir = Bopendir(".")) != NULL)
		{
			while ((pFile = Breaddir(pDir)) != NULL && chmod(pFile->name, S_IWRITE) == 0)
			{
				if (pFile->mode & BS_IFDIR)
				{
					if (strcmp(pFile->name, "..") == 0)	continue;
					if (strcmp(pFile->name, ".")  == 0)	continue;
					if (!dirRemoveRecursive(pFile->name))
						break;
				}
				else if (unlink(pFile->name) != 0)
					break;
			}
			
			Bclosedir(pDir);
		}
		
		return (getcwd(cwd, sizeof(cwd)) && chdir("../") == 0 && rmdir(cwd) == 0);
	}
	
	return 0;
}

char makeBackup(char* filename)
{
	if (!fileExists(filename))
		return TRUE;
	
	char temp[_MAX_PATH];
	sprintf(temp, filename);
	ChangeExtension(temp, ".bak");
	if (fileExists(temp))
		unlink(temp);
	
	return (rename(filename, temp) == 0);
}

char fileDelete(char* file)
{
	if (!fileExists(file)) return TRUE;
	fileAttrSetWrite(file);
	unlink(file);
	
	return (!fileExists(file));
}

void swapValues(int *nSrc, int *nDest)
{
	int nTmp;
	nTmp = *nSrc, *nSrc = *nDest, *nDest = nTmp;
}

int revertValue(int nVal)
{
	if (nVal < 0) return klabs(nVal);
	else if (nVal > 0) return -nVal;
	else return nVal;
}

void offsetPos(int oX, int oY, int oZ, int nAng, int* x, int* y, int* z)
{
    // left, right
    if (oX)
    {
        if (x) *x -= mulscale30(oX, Cos(nAng + kAng90));
        if (y) *y -= mulscale30(oX, Sin(nAng + kAng90));
    }

    // forward, backward
    if (oY)
    {
        if (x) *x += mulscale30r(Cos(nAng), oY);
        if (y) *y += mulscale30r(Sin(nAng), oY);
    }

    // top, bottom
    if (oZ && z)
        *z += oZ;

}

unsigned char mostUsedByte(unsigned char* bytes, int l, short nExcept)
{
	if (bytes == NULL || *bytes == NULL)
		return 0;
	
	int count[256], i, j = 0, m = 0;
	memset(count, 0, sizeof(count));
	
	while(--l >= 0)
	{
		if (nExcept < 0 || bytes[l] != nExcept)
			count[bytes[l]]++;
	}
	
	for (i = 0; i < 256; i++)
	{
		if (count[i] >= m)
		{
			m = count[i];
			j = i;
		}
	}
	
	return (unsigned char)j;
	
}

void BeepOk(void)
{
	if (!gMisc.beep) return;
	gBeep.Play(kBeepOk);
}


void BeepFail(void)
{
	if (!gMisc.beep) return;
	gBeep.Play(kBeepFail);
}

char Beep(char cond)
{
	if (cond) BeepOk();
	else BeepFail();
	return cond;
}

void doGridCorrection(int* x, int* y, int nGrid)
{
	if (!irngok(nGrid, 1, 11))
		return;
	
	if (x) *x = (*x+(1024 >> nGrid)) & (0xffffffff<<(11-nGrid));
	if (y) *y = (*y+(1024 >> nGrid)) & (0xffffffff<<(11-nGrid));
}

void doGridCorrection(int* x, int nGrid)
{
	doGridCorrection(x, NULL, nGrid);
}

void doWallCorrection(int nWall, int* x, int* y, int shift)
{
	int nx, ny;
	GetWallNormal(nWall, &nx, &ny);
	*x = *x + (nx >> shift);
	*y = *y + (ny >> shift);
}

BYTE countBestColor(PALETTE in, int r, int g, int b, int wR, int wG, int wB)
{
	int i, dr, dg, db;
	int dist, matchDist = 0x7FFFFFFF, match;
	
	for (i = 0; i < 256; i++)
	{
		dist = 0;
		dg = (int)in[i].g - g;
		dist += wG * dg * dg;
		if (dist >= matchDist)
			continue;

		dr = (int)in[i].r - r;
		dist += wR * dr * dr;
		if (dist >= matchDist)
			continue;

		db = (int)in[i].b - b;
		dist += wB * db * db;
		if (dist >= matchDist)
			continue;

		matchDist = dist;
		match = i;

		if (dist == 0)
			break;
	}

	return (BYTE)match;
}


void dozCorrection(int* z, int zStep)
{
	*z = *z & ~zStep;
}

void _SetErrorLoc(const char *pzFile, int nLine)
{
    _module = pzFile;
    _line = nLine;
}

void _ThrowError(const char *pzFormat, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, pzFormat);
    vsprintf(buffer, pzFormat, args);
    buildprintf("%s(%i): %s\n", _module, _line, buffer);
    wm_msgbox("Error", "%s(%i): %s\n", _module, _line, buffer);

    fflush(NULL);
    exit(0);
}


void __dassert(const char * pzExpr, const char * pzFile, int nLine)
{
    buildprintf("Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);
    wm_msgbox("Assertion failed", "Assertion failed: %s in file %s at line %i\n", pzExpr, pzFile, nLine);

    fflush(NULL);
    exit(0);
}
