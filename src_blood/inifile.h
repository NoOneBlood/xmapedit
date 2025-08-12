/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
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
#ifndef _INIFILE_H
#define _INIFILE_H
#include "compat.h"
#pragma pack(push, 1)
struct ININODE
{
    char    type;
    char *hiWord;
    char *loWord;
};
#pragma pack(pop)

enum
{
    INI_NORMAL      = 0x00,
    INI_SKIPCM      = 0x01,
    INI_SKIPZR      = 0x02,
};

class IniFile
{
    private:
        ININODE* node;
        int numgroups;
        int numnodes;
    public:
        char filename[BMAX_PATH];
        //---------------------------------------------------------------
        IniFile(unsigned char* pRaw, int nLength, char flags = INI_NORMAL);
        IniFile(char* fileName, char flags = INI_NORMAL);
        ~IniFile();
        //---------------------------------------------------------------
        void Init();
        void Load(unsigned char* pRaw, int nLength, char flags = INI_NORMAL);
        char Save(char* saveName = NULL);
        //---------------------------------------------------------------
        int  NodeAdd(ININODE* pNode, int nPos);
        int  NodeAddEmpty(int nPos);
        void NodeSetWord(char** wordPtr, char* string);
        void NodeRemove(int nID);
        char NodeComment(int nID, char hashChr);
        //---------------------------------------------------------------
        int  SectionFind(char* name);
        int  SectionAdd(char* section);
        void SectionRemove(char* section);
        //---------------------------------------------------------------
        int  KeyFind(char* section, char* key);
        int  KeyAdd(char* section, char* hiWord, char* loWord);
        int  KeyAddNew(char* section, char* hiWord, char* loWord);
        void KeyRemove(char* section, char* hiWord);
        //---------------------------------------------------------------
        char* GetKeyString(char* section, char* key, char* pRetn = NULL);
        int   GetKeyInt(char* section, char* key, const int32_t nRetn = -1);
        char  GetNextString(char* out, char** pKey, char** pVal, int* prevNode, char *section = NULL);
        char  GetNextString(char** pKey, char** pVal, int* prevNode, char *section = NULL);
        int   GetNextSection(char** section);
        //---------------------------------------------------------------
        char  PutKeyInt(char* section, char* hiWord, const int nVal);
        char  PutKeyHex(char* section, char* hiWord, const int nVal);
        //---------------------------------------------------------------
        inline char PutKeyString(char* section, char* hiWord, char* loWord = NULL)  { return (KeyAdd(section, hiWord, loWord) >= 0); }
        inline char GetKeyBool(char *section, char *key, int32_t nRetn)             { return (GetKeyInt(section, key, nRetn) != 0); }
        inline int  GetKeyHex(char *section, char *key, int32_t nRetn)              { return GetKeyInt(section, key, 0); }
        inline char KeyExists(char* section, char* key)                             { return (KeyFind(section, key) >= 0); }
        inline char SectionExists(char* name)                                       { return (SectionFind(name) >= 0); }
        inline void RemoveSection(char* name)                                       { SectionRemove(name); }
};
#endif