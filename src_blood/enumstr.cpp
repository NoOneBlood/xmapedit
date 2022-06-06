/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A library of functions to split enumeration strings.
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

#include <stdio.h>
#include <string.h>
#include <io.h>
#include "misc.h"
#include "common_game.h"

int enumStrGetInt(int offset, char* astr, char expcr, int retn) {
	
	int i = 0, j = 0, start = 0;
	static char str[256], result[16];
	static int len = 0;

	if (astr != NULL)
	{
		memset(str, 0, sizeof(str));
		len = sprintf(str, "%0.255s", astr);
		dassert(len == strlen(astr));
		
		// space and special symbols removal
		for (i = 0; i < len; i++)
		{
			if (str[i] == expcr || str[i] == 43 || str[i] == 45 || (str[i] >= 48 && str[i] <= 57)) continue;
			for (j = i; j < len; j++) str[j] = str[j + 1];
			str[j - 1] = 0;
			len--;
		}
	}
	else
	{
		dassert(str[0] != 0);
	}
	
	if (len > 0)
	{
		if ((offset = ClipLow(offset, 0)) > 0)
		{
			// search for start
			for (i = 0; i < len; i++)
			{
				if (str[i] == expcr) start++;
				if (start != offset) continue;
				start = i + 1;
				break;
			}
			
			if (i == len || start >= len)
				return retn;
		}
		
		
		memset(result, 0, sizeof(result));
		for (i = start, j = 0; i < len; i++)
		{
			if (str[i] == expcr) break;
			else result[j++] = str[i];
			dassert(j < sizeof(result));
		}
		
		return atoi(result);
	}
	
	return retn;
}

char* enumStrGetChar(int offset, char* out, char* astr, char expcr, char* def)
{
	int i, j, start = 0;
	static char* ptr = NULL;
	static int len   = -1;
	
	out[0] = '\0';
	
	if (astr != NULL)
	{
		if (ptr)
			free(ptr);
		
		if ((len = strlen(astr)) > 0)
		{
			ptr = (char*)malloc(len);
			strncpy(ptr, astr, len);
		}
	}
	else
	{
		dassert(ptr != NULL);
	}
	
	if (len > 0)
	{
		if ((offset = ClipLow(offset, 0)) > 0)
		{
			// search for start
			for (i = 0; i < len; i++)
			{
				if (ptr[i] == expcr) start++;
				if (start != offset) continue;
				start = i + 1;
				break;
			}
			
			if (i >= len || start >= len)
				return def;
		}
		
		for (i = start, j = 0; i < len; i++)
		{
			if (ptr[i] == expcr) break;
			else out[j++] = ptr[i];
		}
		
		out[j] = '\0';
	}

	return (out[0]) ? out : def;
}