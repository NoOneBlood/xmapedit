/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2025: Adapted for XMAPEDIT by NoOne.
// Map editing key input processing for shared, 2D
// and 3D modes.
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
#ifndef __MAPEDPROC
#define __MAPEDPROC

typedef char (*INPUTPROC_FUNC)(char key, char ctrl, char shift, char alt);

struct INPUTPROC
{
	char key;
	INPUTPROC_FUNC pFunc;
};

#define PROC_FAIL		0x00
#define PROC_OK			0x01
#define PROC_UNDO_CMP	0x02
#define PROC_UNDO_ADD	0x04
#define PROC_UNDO_MASK	0x06
#define PROC_BEEP		0x08

#define PROC_OKUB		(PROC_OK | PROC_UNDO_CMP | PROC_BEEP)
#define PROC_OKU        (PROC_OK | PROC_UNDO_CMP)
#define PROC_FAILB		(PROC_FAIL | PROC_BEEP)
#define PROC_OKB		(PROC_OK | PROC_BEEP)


extern INPUTPROC gEditInputShared[256];
extern INPUTPROC gEditInput3D[256];
extern INPUTPROC gEditInput2D[256];
void editInputInit();

#endif