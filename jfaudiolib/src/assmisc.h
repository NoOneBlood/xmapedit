/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __ASSMISC_H
#define __ASSMISC_H

#ifdef __POWERPC__
#define LITTLE16(s) SWAP16(s)
#define LITTLE32(s) SWAP32(s)
static inline unsigned short SWAP16(unsigned short s)
{
	return (s >> 8) | (s << 8);
}
static inline unsigned int SWAP32(unsigned int s)
{
	return (s >> 24) | (s << 24) | ((s&0xff00) << 8) | ((s & 0xff0000) >> 8);
}
#else
#define LITTLE16(s) (s)
#define LITTLE32(s) (s)
#endif

#ifdef _MSC_VER
#define inline _inline
#define alloca _alloca
#endif

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef TRUE
#define TRUE  ( 1 == 1 )
#endif
#ifndef FALSE
#define FALSE ( !TRUE )
#endif

#endif
