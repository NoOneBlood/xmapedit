/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// Game GUI based directory explorer and it's variations.
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
#include "gui.h"
#include "gfx.h"
#include "screen.h"
#include "xmpstub.h"
#include "xmpexplo.h"
#include "xmpmisc.h"
#include "enumstr.h"


#define kDirExpMaxFiles 1024 // per directory
#define kDirExpDefaultDirColor 0
#define kDirExpDefaultFileColor 1
#define kDirExpBakFileColor 6

short restable[][6] = {
	
	// ydim, dwidth, dheigh, butw,  rows,   cols
	{200,	300,	180,	69,		4,		5},		// 320p
	{480,	420,	400,	78,		5, 		16},	// 640p
	{600,	500,	500,	78,		6, 		21},	// 800p
	{768,	668,	668,	78,		8, 		29},	// 1024p
	//{1024,	924,	924,	79,		11,		42},	// 1280p

};

DIR_EXPLORER files[kDirExpMaxFiles];
char filepath[_MAX_PATH]; // path and selected filename
char title[256];

BYTE getColorByAttr(int attr) {
	
	if (attr & kDirExpFile) return kDirExpDefaultFileColor;
	if (attr & kDirExpFileBak) return kDirExpBakFileColor;
	return kDirExpDefaultDirColor;
}

void dirExpFormatTitle(char* titleArg, char* path, int dwidth) {
	
	int len1, len2;
	if (!path || !path[0])
	{
		sprintf(title, titleArg);
		return;
	}

	len1 = len2 = sprintf(buffer3, path);
	while( 1 )
	{
		sprintf(title, "%s - %s", buffer3, titleArg);
		if (gfxGetTextLen(title, pFont) >= dwidth - 20)
		{
			buffer3[--len1] = 0;
			continue;
		}
		
		break;
	}
	
	if (len1 != len2)
		sprintf(title, "%s... - %s", buffer3, titleArg);
	
	
	return;

}



BOOL compareExtension(char* ext, char* allow) {
	
	int i = 0;
	char *pExt = ext, *pAllow = allow, tmp[16];
	if (!pAllow) return TRUE;
	if (!pExt)   return FALSE;

	if (pExt[0] == '.')
		pExt =& ext[1];
	
	if (pAllow[0] == '.')
		pAllow =& allow[1];

	enumStrGetChar(i, tmp, pAllow, '.');
	while(enumStrGetChar(i++, tmp, NULL, '.'))
	{
		if (stricmp(tmp, pExt) == 0)
			return TRUE;
	}

	return FALSE;
}

int countSelected() {
	
	int i, j;
	for (i = 0, j = 0; i < kDirExpMaxFiles; i++)
	{
		if (files[i].sel) j++;
	}
	
	return j;
}

BOOL enumSelected(int* idx, char* out) {
	
	int i, start;
	char tmp[BMAX_PATH]; char *dir, *drv;
	start = (*idx < 0) ? 0 : *idx + 1;
	if (start >= kDirExpMaxFiles)
		return FALSE;
	
	i = sprintf(out, filepath);
	if (i && out[i - 1] != '\\') strcat(out, "\\");
	pathSplit2(out, tmp, &drv, &dir, NULL, NULL);
	_makepath(out, drv, dir, NULL, NULL);
	//sprintf(out, dir);

	for (i = start; i < kDirExpMaxFiles; i++)
	{
		if (!files[i].sel) continue;
		strcat(out, files[i].basename);
		strcat(out, files[i].ext);
		*idx = i;
		return TRUE;
	}
	
	return FALSE;
}

void dirExpSortItems() {
	
	// first dirs, then files
	DIR_EXPLORER dir[kDirExpMaxFiles];
	DIR_EXPLORER fil[kDirExpMaxFiles];
	
	int i, dirc = 0, filc = 0;
	for (i = 0; i < kDirExpMaxFiles; i++)
	{
		if (files[i].attr & kDirExpDir) dir[dirc++] = files[i];
		else if (files[i].attr > 0) fil[filc++] = files[i];
	}
	
	memcpy(&files[0], dir, sizeof(DIR_EXPLORER)*dirc);
	memcpy(&files[dirc], fil, sizeof(DIR_EXPLORER)*filc);
}

char* dirBrowse(char* titleArg, char* path, char* ext, BYTE dlgType, char flags) {

	dassert(path != NULL);
	BYTE read = 0x01; BOOL ok = FALSE, mext = (ext && strlen(ext) > 4);
	int len, chdcnt = 0, filescnt, dirscnt, perpage, start, pages, selcnt;
	int i, j, k, x, y, hotkey, dwidth, dheigh, butw, rows, cols, rowscnt, colscnt;
	int page = 0, itemid = -1, maxlen = 8, width = pFont->width*(maxlen + 2), buth = 20;
	char from[32] = "", tmp[_MAX_PATH], flags2 = flags, *drv, *dir, *fname, *fext;
	
	pathSplit2(path, tmp, &drv, &dir, &fname, &fext);
	if ((len = strlen(fext)) <= 0 && strlen(dir))
		fname[0] = 0;

	len = sprintf(filepath, (len) ? dir : (strlen(dir)) ? path : dir);
	if (len && filepath[len - 1] == '\\')
		filepath[len - 1] = 0, len--;
	
	if (len && !fileExists(filepath))
	{
		Alert("Path \"%s\" is not exists!", filepath);
		sprintf(filepath, ".\\");
	}


	// count sub dir level
	if (filepath[0])
	{
		k = strlen(filepath);
		for (i = 0, j = 0; i < k; i++)
			if (slash(filepath[i])) chdcnt++, j = i;
	
		if (j != i)
			chdcnt++;
	}

	// use resolution table to scale the dialog
	for (i = LENGTH(restable) - 1; i >= 0; i--)
	{
		if (ydim < restable[i][0]) continue;
		dwidth  = restable[i][1];
		dheigh  = restable[i][2];
		butw    = restable[i][3];
		rows    = restable[i][4];
		cols    = restable[i][5];
		perpage = rows*cols;
		break;
	}
	
	while ( 1 ) {

		if (read)
		{
			memset(files, 0, sizeof(files));
			dirscnt = filescnt = selcnt = read = 0;
			if ((len = dirBrowseGetFiles(filepath[0] ? filepath : (char*)".", ext, files, &dirscnt, &filescnt, flags2)) >= 0)
				dirExpSortItems();
			
			dirExpFormatTitle(titleArg, filepath, dwidth);
		}
		
		i = len/perpage, pages = (i*perpage < len) ? i + 1 : i;
		rowscnt = rows, colscnt = cols;
		start = page * perpage;

		Window dialog(0, 0, dwidth, dheigh, title); // create the dialog
		
		EditText* textEdit = NULL;
		if (!(flags & kDirExpMulti))
		{
			textEdit = new EditText(dwidth - width - 66, dheigh - 46, width, 18, "");
			sprintf(textEdit->placeholder, fname);
			
			textEdit->maxlen     = maxlen;
			textEdit->hotKey     = 'F';
		
			dialog.Insert(textEdit);
			dialog.Insert(new TextButton(dwidth - 64, dheigh - 48, 54, 20, (char*)((dlgType) ? "&Save" : "&Select"), mrOk));
		}
		else
		{
			x = dwidth - 48;
			TextButton* pSelNone = new TextButton(x, dheigh - 48, 40, 20, "&None", -11); x-=42;
			TextButton* pSelAll  = new TextButton(x, dheigh - 48, 40, 20, "&All",  -10);
			
			sprintf(buffer, "&Select");
			if (selcnt)
			{
				strcat(buffer, " ");
				sprintf(buffer2, "%d", selcnt);
				strcat(buffer, buffer2);
			}
			
			i = gfxGetLabelLen(buffer, pFont) + 20; x-=(i+2);
			TextButton* pSelNum  = new TextButton(x, dheigh - 48, i, 20, buffer, mrOk); x+=i;

			if (!selcnt)
			{
				pSelNum->disabled = pSelNone->disabled = TRUE;
				pSelNum->canFocus = pSelNone->canFocus = FALSE;
				pSelNum->fontColor = pSelNone->fontColor = 8;
			}

			dialog.Insert(pSelNum);
			dialog.Insert(pSelAll);
			dialog.Insert(pSelNone);
		}
		
		x = 4, y = 4, hotkey = 31;
		for (i = start; i < start + perpage; i++)
		{
			char* text = (mext) ? files[i].fullname : files[i].basename;
			TextButton* pButton   = new TextButton(x, y, butw, buth, (i >= len) ? (char*)"-" : text, mrUser + i);
			pButton->hotKey 	  = (char)ClipHigh(hotkey + 1, 40);
			pButton->font         = qFonts[(files[i].attr & kDirExpDir) ? 0 : 1];
			if (i >= len)
			{
				pButton->disabled  = TRUE;
				pButton->fontColor = 7;
				pButton->canFocus  = FALSE;
			}
			else
			{
				pButton->disabled  = FALSE;
				pButton->fontColor = getColorByAttr(files[i].attr);
			}
						
			if (files[i].sel)
			{
				pButton->pressed = TRUE;
				pButton->fontColor = (char)(pButton->fontColor ^ 8);
				dialog.Insert(new Shape(x-1, y+(buth>>1), butw+2, 2, gStdColor[0]));
			}
			
			dialog.Insert(pButton);
			if (--colscnt > 0) y += buth;
			else if (--rowscnt > 0) x += butw + 4, y = 4, colscnt = cols;
			else break;
		}

		x = 4; y += buth+4;
		sprintf(buffer3, "PAGE %d OF %d", page + 1, ClipLow(pages, 1));
		Label* pPages = new Label(x + 2, y, buffer3);
		pPages->font  = qFonts[1];

		char refresh[] = "Upd";
		i = gfxGetLabelLen(strupr(refresh), qFonts[3]) + 16;
		sprintf(buffer3, "%d DIRS %d FILES", dirscnt, filescnt);
		j = gfxGetTextLen(buffer3, qFonts[1]);
		
		Label* pItems = new Label(dwidth - j - i - 14, y, buffer3);
		pItems->font  = qFonts[1];
		
		int bw, bh;
		
		bh = 16;
		TextButton* pRefresh  = new TextButton(dwidth - i - 10, y - (buth >> 2), i, buth , refresh, -12);
		pRefresh->font = qFonts[3];
		pRefresh->fontColor = 2;
				
		y += qFonts[1]->height+7;
		bw = pBitmaps[2]->width+2;
		bh = pBitmaps[2]->height+2;
		BitButton2* prevPage = new BitButton2(x,  y, bw, bh,  pBitmaps[(len > perpage) ? 2 : 11], -1); x+=bw+4;
		BitButton2* nextPage = new BitButton2(x,  y, bw, bh,  pBitmaps[(len > perpage) ? 3 : 12], -2); x+=bw+4;
		if (len <= perpage)
			prevPage->disabled = nextPage->disabled = TRUE, prevPage->canFocus = nextPage->canFocus = FALSE;

		BitButton2* upDir = new BitButton2(x,  y, bw, bh,  pBitmaps[(chdcnt) ? 0 : 13], -4); x+=bw+4;
		if (!chdcnt)
			upDir->disabled = TRUE, upDir->canFocus = FALSE;
		
		dialog.Insert(pPages);
		dialog.Insert(pRefresh);
		dialog.Insert(pItems);
		dialog.Insert(prevPage);
		dialog.Insert(nextPage);
		dialog.Insert(upDir);
		
		if ((flags & kDirExpBak) && dlgType == kDirExpTypeOpen)
		{
			TextButton* pbBak = new TextButton(x, y, bw, bh, "BAK", -5); x+=bw+4;
			pbBak->font = qFonts[0];
			pbBak->fontColor = 1;
			dialog.Insert(pbBak);
		}
		
		ShowModal(&dialog);

		switch (dialog.endState) {
			case mrCancel: // esc
				break; 
			case -1: // prev page
				if (page-- <= 0) page = ClipLow(pages - 1, 0);
				continue;
			case -2: // next page
				if (++page >= pages) page = 0;
				continue;
			case -4: // up dir
				for (i = strlen(filepath) - 1; i >= 0; i--)
					if (slash(filepath[i])) break;
				
				filepath[(i > 0) ? i : 0] = 0;
				chdcnt = ClipLow(chdcnt - 1, 0);
				page = 0, read = 0x01;
				continue;
			case -5: // toggle showing BAK files on and off
				if (flags2 & kDirExpBak) flags2 &= ~kDirExpBak;
				else flags2 |= kDirExpBak;
				page = 0, read = 0x01;
				continue;
			case -10:	// select all
				selcnt = 0;
				for (i = 0; i < len; i++)
				{
					if (files[i].attr & kDirExpDir) continue;
					files[i].sel = 1;
					selcnt++;
				}
				continue;
			case -11: 	// select none
				for (i = 0; i < len; files[i++].sel = 0);
				selcnt = 0;
				continue;
			case -12:	// refresh
				read = 0x01;
				continue;
			case mrOk:
				if (textEdit && strlen(textEdit->string)) i = sprintf(from, textEdit->string), selcnt = 0;
				else if (textEdit && strlen(textEdit->placeholder)) i = sprintf(from, textEdit->placeholder);
				else if (!selcnt) continue;
				else
				{
					ok = TRUE;
					break;
				}

				if (dlgType == kDirExpTypeOpen)
				{
					for (itemid = 0; itemid < len; itemid++)
						if (stricmp(from, files[itemid].basename) == 0)
							break;
					
					if (fname)
						fname[0] = '\0';

					// file not found in the list
					if (itemid == len)
					{
						if (flags & kDirExpAllowRff) // lookup in rff (and root)
						{
							RESHANDLE hRes = NULL;
							if (ext) strcat(from, ext);
							if (fileExists(from, &hRes))
							{
								sprintf(filepath, from);
								ok = TRUE;
								break;
							}
						}
						
						Alert("File \"%s\" not found!", from);
						itemid = -1; from[0] = 0;
						continue;
					}
				}
				// no break
			default:
				if (dialog.endState >= mrUser) itemid = dialog.endState - mrUser;
				if ((itemid >= 0 && itemid < len) && (files[itemid].attr & kDirExpDir))
				{
					if (filepath[0]) catslash(filepath);
					strcat(filepath, files[itemid].basename);
					read = 0x01; itemid = -1, chdcnt++, page = 0;
					continue;
				}
				else if (itemid < 0 || itemid >= len)
				{
					switch (dlgType) {
						case kDirExpTypeSave:
							if (strlen(from))
							{
								if (filepath[0]) catslash(filepath);
								strcat(filepath, from);
								ChangeExtension(filepath, ext);
								ok = TRUE;
								break;
							}
							// no break
						default:
							continue;
					}
				}
				else if ((flags & kDirExpMulti))
				{
					files[itemid].sel^=1;
					if (files[itemid].sel) selcnt++; else selcnt--;
					continue;
				}
				else
				{
					if (!(flags & kDirExpMulti))
					{
						if (filepath[0]) catslash(filepath);
						strcat(filepath, files[itemid].basename);
						ChangeExtension(filepath, files[itemid].ext);
					}
					for (i = 0; i < len; files[i++].sel = 0);
					files[itemid].sel = 1;
					ok = TRUE;
				}
				break;

		}
		break;

	}
	
	if (!ok) 
		return NULL;
	
	// modify argument path
	//if (strlen(path) >= strlen(filepath))
		sprintf(path, filepath);
	return filepath;

}


int dirBrowseGetFiles(char* path, char* reqExt, DIR_EXPLORER* out, int* dirs, int* files, char flags) {

	char tmp[BMAX_PATH];
	BDIR* pDir = NULL; Bdirent* pFile = NULL; char *dsk, *fil, *dir, *ext, *tExt;
	int total = 0, dirscnt = 0, filescnt = 0, bakType = -1;
	BYTE attr;
	
	dassert(reqExt != NULL);
	dassert(path != NULL);

	sprintf(buffer3, reqExt); tExt = buffer3;
	if (tExt[0] == '.')
		tExt =& tExt[1];

	if ((pDir = Bopendir(path)) != NULL)
	{
		while ((pFile = Breaddir(pDir)) != NULL)
		{
			attr = 0;	
			pathSplit2(pFile->name, tmp, &dsk, &dir, &fil, &ext);
			if (pFile->mode & BS_IFDIR)
			{
				if (!dirs || pFile->name[0] == '.') continue;
				attr = kDirExpDir;
				dirscnt++;
			}
			else
			{
				if (!files) continue;
				else if (compareExtension(ext, reqExt)) attr = kDirExpFile;
				else if (!(flags & kDirExpBak) || stricmp(ext, ".bak") != 0) continue;
				else
				{
					
					// !!!
					// try to recognize BAK format
					_makepath(buffer2, NULL, path, fil, ext);
					if ((bakType = scanBakFile(buffer2)) < 0) continue;
					else if (stricmp(gExtNames[bakType], tExt) != 0) continue;
					else attr = kDirExpFileBak;
				}

				filescnt++;
			}
			
			if (out)
			{
				/// !!!
				out[total].attr = attr;
				sprintf(out[total].fullname, "%0.14s", pFile->name); 
				sprintf(out[total].basename, "%0.9s", fil);
				sprintf(out[total].ext, ext);
			}
			
			if (++total >= kDirExpMaxFiles - 1)
				break;
		}
		
		Bclosedir(pDir);
	}
	
	if (dirs)	*dirs = dirscnt;
	if (files)	*files = filescnt;
	return dirscnt + filescnt;
}