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

#ifdef USE_QHEAP

struct HEAPNODE
{
    HEAPNODE *prev;
    HEAPNODE *next;
    int size;
    bool isFree;
    HEAPNODE *freePrev;
    HEAPNODE *freeNext;
};

class QHeap
{
public:
    QHeap(int heapSize);
    ~QHeap(void);

    void Check(void);
    void Debug(void);
    void *Alloc(int);
    int Free(void *p);

    void *heapPtr;
    HEAPNODE heap;
    HEAPNODE freeHeap;
    int size;
};

#endif
