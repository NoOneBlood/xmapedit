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

char* enumStrGetChar(int offset, char* out = NULL, char* astr = NULL, char expcr = ',', char* def = NULL);
int enumStrGetInt(int offset, char* astr = NULL, char expcr = ',', int retn = 0);
#endif
