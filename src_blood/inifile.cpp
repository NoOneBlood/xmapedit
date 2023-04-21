/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
// Copyright (C) 2021: Additional changes by NoOne.
// Note: This module is based on the sirlemonhead's work
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
#include "inifile.h"
#include "xmpmisc.h"
#include "gui.h"


IniFile::IniFile(char *fileName)
{
    head.next = &head;
    strcpy(this->fileName, fileName);
    Load();
}

IniFile::IniFile(BYTE *res, int nLength, char* saveName)
{
    memset(this->fileName, 0, sizeof(this->fileName));	
	BYTE* pResNew = (BYTE*)malloc(nLength + 1);
	int i, j, c;
	
	
	head.next = &head;
	if (saveName)
		strcpy(this->fileName, saveName);
	
	if (pResNew)
	{
		memset(pResNew, 0, nLength + 1);
		for (i = 0, j = 0; i < nLength; i++)
		{
			c = res[i];
			if (c == '\r' || c == '\n')
			{
				pResNew[j++] = '\n';

				if (c == '\r')
				{
					if (i + 1 < nLength && res[i + 1] == '\n')
						i++;
				}
			}
			else
			{
				pResNew[j++] = c;
			}
		}
		
		LoadRes(pResNew);
		free(pResNew);
		return;
	}
	
    LoadRes(res);
}

void IniFile::LoadRes(void *res)
{
	char buffer[256];
	char *ch, *start;
	int lineLen;

    curNode = &head;

    while (ResReadLine(buffer, sizeof(buffer), &res) != 0)
    {
		if ((ch = strchr(buffer,'\r')) != NULL) *ch = '\0';
		if ((ch = strchr(buffer,'\n')) != NULL) *ch = '\0';
        
		start = buffer;
		
		// ignore leading spaces
		while (isspace(*start))
			start++;
		
		// ignore ending spaces
		lineLen = strlen(start);
		while(lineLen > 0 && isspace(start[lineLen - 1]))
			start[--lineLen] = '\0'; 
		

        curNode->next = (ININODE*)malloc(lineLen + sizeof(ININODE));
        dassert(curNode->next != NULL);

        anotherNode = curNode;
        curNode = curNode->next;

        strcpy(curNode->name, start);

        /*
            check for:
            ; - comment line. continue and grab a new line  (59)
            [ - start of section marker                     (91)
            ] - end of section marker                       (93)
            = - key and value seperator                     (61)
        */

        switch (*start) {
			case '\0':
			case ';': // comment line
				break;
			case '[':
				if (strchr(start, ']')) break;
				free(curNode);
				curNode = anotherNode;
				continue;
			default:
				if (strchr(start, '=') <= start)
				{
					free(curNode);
					curNode = anotherNode;
				}
				break;
        }
    }

    curNode->next = &head;
}

void IniFile::Load()
{	
	curNode = &head;
	BYTE *pAllocFile, *pFile; int hFile, len = 0;
	if ((hFile = open(fileName, O_RDONLY|O_BINARY, S_IREAD|S_IWRITE)) >= 0)
	{
		len = filelength(hFile);
		pAllocFile = (BYTE*)malloc(len+1);
		memset(pAllocFile, 0, len+1);
		pFile = pAllocFile;
		
		if (len > 0)
			read(hFile, pAllocFile, len);
		
		close(hFile);
		LoadRes(pFile);
		free(pAllocFile);
	}
	else
	{
		 curNode->next = &head;
	}
}

void IniFile::Save(void)
{
    if (!strlen(fileName))
		return;
	
	int hFile; char buffer[256];
	fileAttrSetWrite(fileName);
	hFile = open(fileName, O_CREAT | O_WRONLY | O_TEXT | O_TRUNC, S_IREAD | S_IWRITE);
	dassert(hFile >= 0);
    
	curNode = head.next;
    while (curNode != &head)
    {
        sprintf(buffer, "%s\n", curNode->name);
        write(hFile, buffer, strlen(buffer));
        curNode = curNode->next;
    }
    
	close(hFile);
}

bool IniFile::FindSection(char *section)
{
    char buffer[256];
    curNode = anotherNode = &head;
    if (section)
    {
        sprintf(buffer, "[%s]", section);
        do
        {
            anotherNode = curNode;
            curNode = curNode->next;
            if (curNode == &head)
                return false;
        }
		while(stricmp(curNode->name, buffer) != 0);
    }
    return true;
}

bool IniFile::SectionExists(char *section)
{
    return FindSection(section);
}

bool IniFile::FindKey(char *key)
{
    anotherNode = curNode;
    curNode = curNode->next;
    while (curNode != &head)
    {
        char c = curNode->name[0];

        if (c == ';' || c == '\0') {
            anotherNode = curNode;
            curNode = curNode->next;
            continue;
        }

        if (c == '[') {
            return 0;
        }

        char *pEqual = strchr(curNode->name, '=');
        char *pEqualStart = pEqual;
        dassert(pEqual != NULL);

        // remove whitespace
        while (isspace(*(pEqual - 1))) {
            pEqual--;
        }

        c = *pEqual;
        *pEqual = '\0';
		
		// make it no sensetive to case
        if (stricmp(key, curNode->name) == 0)
        {
            // strings match
            *pEqual = c;
            _13 = ++pEqualStart;
            while (isspace(*_13)) {
                _13++;
            }

            return true;
        }
        *pEqual = c;
        anotherNode = curNode;
        curNode = curNode->next;
    }
    
    return false;
}

void IniFile::AddSection(char *section)
{
    char buffer[256];

    if (anotherNode != &head)
    {
        ININODE *newNode = (ININODE*)malloc(sizeof(ININODE));
        dassert(newNode != NULL);

        newNode->name[0] = 0;
        newNode->next = anotherNode->next;
        anotherNode->next = newNode;
        anotherNode = newNode;
    }

    sprintf(buffer, "[%s]", section);
    ININODE *newNode = (ININODE*)malloc(strlen(buffer) + sizeof(ININODE));
    dassert(newNode != NULL);

    strcpy(newNode->name, buffer);

    newNode->next = anotherNode->next;
    anotherNode->next = newNode;
    anotherNode = newNode;
}

void IniFile::AddKeyString(char *key, char *value)
{
    char buffer[256];

    sprintf(buffer, "%s=%s", key, value);

    ININODE *newNode = (ININODE*)malloc(strlen(buffer) + sizeof(ININODE));
    dassert(newNode != NULL);

    strcpy(newNode->name, buffer);

    newNode->next = anotherNode->next;
    anotherNode->next = newNode;
    curNode = newNode;
}

void IniFile::ChangeKeyString(char *key, char *value)
{
    char buffer[256];

    sprintf(buffer, "%s=%s", key, value);

    ININODE *newNode = (ININODE*)realloc(curNode, strlen(buffer) + sizeof(ININODE));
    dassert(newNode != NULL);

    strcpy(newNode->name, buffer);

    anotherNode->next = newNode;
}

bool IniFile::KeyExists(char *section, char *key)
{
    if (FindSection(section) && FindKey(key))
        return true;

    return false;
}

void IniFile::PutKeyString(char *section, char *key, char *value)
{
    if (FindSection(section))
    {
        if (FindKey(key))
        {
            ChangeKeyString(key, value);
            return;
        }
    }
    else
    {
        AddSection(section);
    }

    AddKeyString(key, value);
}

char* IniFile::GetKeyString(char *section, char *key, char *defaultValue)
{
    if (FindSection(section) && FindKey(key))
        return _13;
    return defaultValue;
}

void IniFile::PutKeyInt(char *section, char *key, int value)
{
    char buffer[256];

    // convert int to string
    sprintf(buffer,"%d",value);

    PutKeyString(section, key, buffer);
}

int IniFile::GetKeyInt(char *section, char *key, int defaultValue)
{
    if (FindSection(section) && FindKey(key))
    {
        // convert string to int int
        return strtol(_13, NULL, 0);
    }
    return defaultValue;
}

void IniFile::PutKeyHex(char *section, char *key, int value)
{
    char buffer[256] = "0x";

    // convert int to string
    sprintf(buffer,"%x",value);

    PutKeyString(section, key, buffer);
}

int IniFile::GetKeyHex(char *section, char *key, int defaultValue)
{
    return GetKeyInt(section, key, defaultValue);
}

bool IniFile::GetKeyBool(char *section, char *key, int defaultValue)
{
    return (bool)GetKeyInt(section, key, defaultValue);
}

void IniFile::RemoveKey(char *section, char *key)
{
    if (FindSection(section) && FindKey(key))
    {
        anotherNode->next = curNode->next;
        free(curNode);
        curNode = anotherNode->next;
    }
}

void IniFile::RemoveSection(char *section)
{
    if (FindSection(section))
    {
        anotherNode = curNode;

        curNode = curNode->next;

        while (curNode != &head)
        {
            if (curNode->name[0] == '[') {
                return;
            }

            anotherNode->next = curNode->next;
            free(curNode);
            curNode = anotherNode->next;
        }
    }
}

IniFile::~IniFile()
{
	curNode = head.next;
	if (fileExists(fileName))
		fileAttrSetWrite(fileName);
	
    while (curNode != &head)
    {
        anotherNode = curNode;
        curNode = curNode->next;
        free(anotherNode);
    }
}

bool IniFile::GetNextString(char* out, char** key, char** value, ININODE** prev, char *sSection) {

	*key = NULL, *value = NULL;
	int len, i, j;

	curNode = anotherNode = &head;
	if (*prev == NULL)
	{
		if (sSection != NULL && !FindSection(sSection))
			return FALSE;
		
		curNode = curNode->next;
	} 
	else
	{
		curNode = *prev;
	}

	// true while next node available
	while (curNode != &head)
	{
		anotherNode = curNode;
		
		*key = NULL, *value = NULL;
		if (*prev == NULL || curNode == *prev)
		{
			*prev = anotherNode->next;
			switch (curNode->name[0])
			{
				case ';':
				case '\0':
				case '\r':
				case '\n':
					continue;
				case '[':
					if (sSection == NULL) continue;
					*prev = NULL;
					return FALSE;
			}
			
			if (out == NULL) out = curNode->name;
			else sprintf(out, curNode->name);
			
			if ((len = strlen(out)) > 0)
			{
				i = 0;
				do { i++; } while(i < len && out[i] != '=');
				if (i < len) // found '=' - split key and value
				{
					j = i, out[i] = '\0';
					while(++i < len && isspace(out[i])); // ignore spaces after '='
					*value = (i < len) ? &out[i] : NULL;

					i = j;
					while(--i > 0 && isspace(out[i])) out[i] = '\0'; // ignore spaces before '='
					*key = (i >= 0) ? out : NULL;
				}
				else // '=' is not found - allow to use key as value
				{
					while(--i > 0 && isspace(out[i])) out[i] = '\0';
					*key = ((i >= 0) ? out : NULL), value = key;
				}
			}
			
			return TRUE;
		}

		curNode = anotherNode->next;
	}
	
	*prev = NULL;
	return FALSE;
	
}
