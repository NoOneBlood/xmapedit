/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021: Originally written by NoOne.
// A lite version of nnextsif adapted for xmapedit's preview mode feature.
// This file provides functionality for kModernCondition types which is part
// of nnexts.cpp. More info at http://cruo.bloodgame.ru/xxsystem
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

#ifndef __NNEXTSIF_H
#define __NNEXTSIF_H
#include "db.h"
#include "eventq.h"

void conditionsInit();
void conditionsTrackingAlloc();
void conditionsTrackingClear();
void conditionsTrackingProcess();
void useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT event);


#endif