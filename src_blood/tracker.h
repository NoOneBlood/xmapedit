/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023: Originally written by NoOne.
// X-object tracker for xmapedit.
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

#ifndef _TRACKER_H_
#define _TRACKER_H_
#include "xmpmisc.h"
#include "xmp2dscr.h"
class CXTracker
{
    private:
        static unsigned char tx;
        static OBJECT_LIST *pList;
        static OBJECT src;
    public:
        static int Track(int nType, int nID, char send);
        static char HaveObjects();
        static void Draw(SCREEN2D* pScreen);
        static void Cleanup();
        static void Clear();
};
#endif
