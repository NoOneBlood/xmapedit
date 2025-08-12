//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

A very basic, slow and probably unsafe string parser.
Update or replace eventually.

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __XMPSTR_H
#define __XMPSTR_H

struct STRCUTSTYLE
{
    uint8_t cutSide;
    uint8_t insChar;
    uint8_t insRepeat;
};

int enumStr(int nOffs, const char* str, char* key, char* val);
int enumStr(int nOffs, const char* str, char* val);
char isfix(const char* str, char flags = 0x03);
void removeSpaces(char* str);
char isufix(const char* str);
char isarray(const char* str, int* nLen = NULL);
char isbool(const char* str);
char isperc(const char* str);
int btoi(const char* str);
char isempty(const char* str);
char isIdKeyword(const char* fullStr, const char* prefix, int* nID = NULL);
char parseRGBString(const char* str, unsigned char out[3]);
void strTrim(char* str, char side = 0x03);
void strTrim(char* str, char* list, char side = 0x03);
int strReplace(char* str, char cWhat, char cBy);
char strSubStr(char* str, char* s, char* e, char* o);
char strQuotPtr(char* str, char** qs, char** qe);
char strCut(char* str, char* o, int n, STRCUTSTYLE* s);

char* enumStrGetChar(int offset, char* out = NULL, char* astr = NULL, char expcr = ',', char* def = NULL);
int enumStrGetInt(int offset, char* astr = NULL, char expcr = ',', int retn = 0);
void pathSplit2(char *pzPath, char* buff, char** dsk, char** dir, char** fil, char** ext);
void pathCatSlash(char* pth, int l = -1);
void pathRemSlash(char* pth, int l = -1);
void removeExtension(char *str);
char removeQuotes(char* str);

char* getCurDir(char* pth, char* out, char addSlash = 0);
char* getFiletype(char* pth, char* out, char addDot = 1);
char* getRelPath(char* relto, char* targt);
char* getFilename(char* pth, char* out, char addExt = 0);
char* getPath(char* pth, char* out, char addSlash = 0);


inline char slash(char ch)          { return (ch == '\\' || ch == '/'); }
inline void catslash(char* str)     { strcat(str, "/"); }

#endif
