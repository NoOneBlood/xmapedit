/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2019: Reverse engineered & edited by Nuke.YKT.
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
#pragma once

#pragma pack(push, 1)

enum DICTFLAGS {
    DICT_ID = 1,
    DICT_EXTERNAL = 2,
    DICT_LOAD = 4,
    DICT_LOCK = 8,
    DICT_CRYPT = 16,
    DICT_BUFFER = 32,
};

struct RFFHeader
{
    char sign[4];
    short version;
    short pad1;
    unsigned int offset;
    unsigned int filenum;
    int pad2[4];
};

struct DICTNODE_FILE
{
    char unused1[16];
    unsigned int offset;
    unsigned int size;
    char unused2[8];
    char flags;
    char type[3];
    char name[8];
    int id;
};

#pragma pack(pop)

struct CACHENODE
{
    void *ptr;
    CACHENODE *prev;
    CACHENODE *next;
    int lockCount;
};

struct DICTNODE : CACHENODE
{
    unsigned int offset;
    unsigned int size;
    char flags;
    //char type[3];
    //char name[8];
    char *type;
    char *name;
    char *path;
    char *buffer;
    unsigned int id;
};



class Resource
{
public:
    Resource(void);
    ~Resource(void);

    void Init(const char *filename, const char* external = NULL);
    static void Flush(CACHENODE *h);
    void Purge(void);
    DICTNODE **Probe(const char *fname, const char *type);
    DICTNODE **Probe(unsigned int id, const char *type);
    void Reindex(void);
    void Grow(void);
	void AddExternalResource(const char* pzPath, int id = 0, int flags = 0);
    void AddExternalResource(const char *name, const char *type, int id = 0, int flags = 0, const char* pzDirectory = NULL);
    void AddFromBuffer(const char* name, const char* type, char *data, int size, int id = 0, int flags = 0);
    static void *Alloc(int nSize);
    static void Free(void *p);
    DICTNODE *Lookup(const char *name, const char *type);
    DICTNODE *Lookup(unsigned int id, const char *type);
    void Read(DICTNODE *n);
    void Read(DICTNODE *n, void *p);
    void *Load(DICTNODE *h);
    void *Load(DICTNODE *h, void *p);
    void *Lock(DICTNODE *h);
    void Unlock(DICTNODE *h);
    void Crypt(void *p, int length, unsigned short key);
    static void AddMRU(CACHENODE *h);
	static void RemoveMRU(CACHENODE *h);
    int Size(DICTNODE*h) { return h->size; }
#if 0
    void FNAddFiles(fnlist_t *fnlist, const char *pattern);
#endif
    void PrecacheSounds(void);
    void PurgeCache(void);
    void RemoveNode(DICTNODE* pNode);

    DICTNODE *dict;
    DICTNODE **indexName;
    DICTNODE **indexId;
    unsigned int buffSize;
    unsigned int count;
    int handle;
    bool crypt;

#if USE_QHEAP
    static QHeap *heap;
#endif
    static CACHENODE purgeHead;
};

typedef DICTNODE *RESHANDLE;
