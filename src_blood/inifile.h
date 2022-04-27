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
#pragma once
#include "compat.h"

struct ININODE
{
    ININODE *next;
    char name[1];
};

// 161 bytes
class IniFile
{
public:
    IniFile(char *fileName);
    IniFile(void *res, char *saveName);
    ~IniFile();

    void Save(void);
    bool FindSection(char *section);
    bool SectionExists(char *section);
    bool FindKey(char *key);
    void AddSection(char *section);
    void AddKeyString(char *key, char *value);
    void ChangeKeyString(char *key, char *value);
    bool KeyExists(char *section, char *key);
    void PutKeyString(char *section, char *key, char *value);
    char* GetKeyString(char *section, char *key, char *defaultValue);
    void PutKeyInt(char *section, char *key, const int value);
    int GetKeyInt(char *section, char *key, int defaultValue);
    void PutKeyHex(char *section, char *key, int value);
    int GetKeyHex(char *section, char *key, int defaultValue);
    bool GetKeyBool(char *section, char *key, int defaultValue);
    void RemoveKey(char *section, char *key);
    void RemoveSection(char *section);
	bool GetNextString(char* out, char** key, char** value, ININODE** prev, char *sSection = NULL);

private:
    ININODE head;
    ININODE *curNode;
    ININODE *anotherNode;

    char *_13;
    char fileName[BMAX_PATH]; // watcom maxpath
    void LoadRes(void *, char*);
    void Load();
};