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
#include "common_game.h"

IOBuffer::IOBuffer(int _nRemain, unsigned char *_pBuffer)
{
    dassert(_pBuffer != NULL);
    dassert(_nRemain >= 0);
    nRemain = nTotal = _nRemain;
    pBuffer =_pBuffer;

}


/// !!!
//  TO-DO: add more error control to this functions
void IOBuffer::read(void *pData, int nSize)
{
    if (nSize > nRemain)
        ThrowError("Read buffer overflow (nSize = %d, nRemain = %d)", nSize, nRemain);

    memcpy(pData, pBuffer, nSize);
    nRemain -= nSize;
    pBuffer += nSize;

}

void IOBuffer::write(void *pData, int nSize)
{
   if (nSize > nRemain)
       ThrowError("Write buffer overflow");

   memcpy(pBuffer, pData, nSize);
   nRemain -= nSize;
   pBuffer += nSize;

}

void IOBuffer::skip(int nSize)
{
    dassert(nSize >= 0);
    if (nSize > nRemain)
        ThrowError("Skip overflow");

    nRemain -= nSize;
    pBuffer += nSize;

}

int IOBuffer::seek(int nOffs, int seekType)
{
    int nTest;
    switch (seekType) {
        case SEEK_SET:
            nTest = ClipRange(nTotal - nRemain, 0, nTotal);
            nRemain = nTotal;
            pBuffer -= nTest;
            // no break
        case SEEK_CUR:
            skip(nOffs);
            break;
        case SEEK_END:
            skip(nRemain);
            nOffs = abs(nOffs);
            pBuffer -= nOffs;
            nRemain  = nOffs;
            break;
    }

    return tell();
}

int IOBuffer::tell()
{
    return abs(nRemain - nTotal);
}

void IOBuffer::rewind()
{
    seek(0, SEEK_SET);
}
