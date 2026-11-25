/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026: Originally written by by NoOne.
// Callbacks for various batch map processing
// functions.
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
#ifndef __MAPEDCALLABACK
#define __MAPEDCALLABACK

////////////////////////////////////////////////////
/***********************************************/
/** SPRITE FUNCTIONS                          **/
/***********************************************/
//////////////////

char sprPutOnCeil(spritetype *pSpr, int nOffs);
char sprPutOnFloor(spritetype *pSpr, int nOffs);
char sprZChg(spritetype *pSpr, int nVal);
char sprZChgNext(spritetype *pSpr, int nVal);
char sprCstatAdd(spritetype* pSpr, int nVal);
char sprCstatRem(spritetype* pSpr, int nVal);
char sprFlagsAdd(spritetype* pSpr, int nVal);
char sprFlagsRem(spritetype* pSpr, int nVal);
char sprSizeSet(spritetype* pSpr, int nVal);
char sprSizeXChg(spritetype* pSpr, int nVal);
char sprSizeYChg(spritetype* pSpr, int nVal);
char sprOffsXChg(spritetype* pSpr, int nVal);
char sprOffsYChg(spritetype* pSpr, int nVal);
char sprPosXChg(spritetype* pSpr, int nVal);
char sprPosYChg(spritetype* pSpr, int nVal);
char spr_Floor_OneSide_HandleFlip(spritetype* pSpr, int nVal);
char sprSlopeSet(spritetype* pSpr, int nVal);
char sprSlopeChg(spritetype* pSpr, int nVal);
char sprSlopeSetFloor(spritetype* pSpr, int);
char sprSlopeSetCeil(spritetype* pSpr, int);
char sprDetailSet(spritetype* pSpr, int nVal);
char sprClipdistSet(spritetype* pSpr, int nVal);
char sprPicnumSet(spritetype* pSpr, int nVal);
char sprStateSet(spritetype* pSpr, int nVal);
char sprFixSector(spritetype* pSpr, int flags = 0);
char sprDelete(spritetype* pSpr, int dummy = 0);
char sprPalSet(spritetype* pSpr, int value);
char sprShadeSet(spritetype* pSpr, int value);
char sprShadeChg(spritetype* pSpr, int step);
char sprClone(spritetype* pSpr, int);

////////////////////////////////////////////////////
/***********************************************/
/** WALL FUNCTIONS                            **/
/***********************************************/
//////////////////

char wallCstatAdd(int nWall, int nVal);
char wallCstatRem(int nWall, int nVal);
char wallCstatAdd_OneWay(int nWall, int nVal);
char wallCstatAdd_Block(int nWall, int nVal);
char wallCstatAdd_Mask(int nWall, int);
char wallPalSet(int nWall, int nVal);
char wallShadeSet(int nWall, int nVal);
char wallPicnumSet(int nWall, int nVal);
char wallPicnumMaskSet(int nWall, int nVal);
char wallPanXChg(int nWall, int nVal);
char wallPanYChg(int nWall, int nVal);
char wallPanSet(int nWall, int nVal);
char wallSizeSet(int nWall, int nVal);
char wallSizeXChg(int nWall, int nVal);
char wallSizeYChg(int nWall, int nVal);
char wallSizeReset(int nWall, int);
char wallSizeAuto(int nWall, int);
char wallGridSnap(int nWall, int nVal);
char wallStateSet(int nWall, int nVal);
char wallHitagSet(int nWall, int nVal);

////////////////////////////////////////////////////
/***********************************************/
/** FLOOR FUNCTIONS                           **/
/***********************************************/
//////////////////

char floorZSet(int nSect, int nVal);
char floorZChgNext(int nSect, int nVal);
char floorZChg(int nSect, int nVal);
char floorSlopeSet(int nSect, int nVal);
char floorSlopeChg(int nSect, int nVal);
char floorStatAdd(int nSect, int nVal);
char floorStatRem(int nSect, int nVal);
char floorShadeSet(int nSect, int nVal);
char floorPalSet(int nSect, int nVal);
char floorPicnumSet(int nSect, int nVal);
char floorPanSet(int nSect, int nVal);
char floorPanXChg(int nSect, int nVal);
char floorPanYChg(int nSect, int nVal);

////////////////////////////////////////////////////
/***********************************************/
/** CEILING FUNCTIONS                         **/
/***********************************************/
//////////////////

char ceilZSet(int nSect, int nVal);
char ceilZChgNext(int nSect, int nVal);
char ceilZChg(int nSect, int nVal);
char ceilSlopeSet(int nSect, int nVal);
char ceilSlopeChg(int nSect, int nVal);
char ceilStatAdd(int nSect, int nVal);
char ceilStatRem(int nSect, int nVal);
char ceilShadeSet(int nSect, int nVal);
char ceilPalSet(int nSect, int nVal);
char ceilPicnumSet(int nSect, int nVal);
char ceilPanSet(int nSect, int nVal);
char ceilPanXChg(int nSect, int nVal);
char ceilPanYChg(int nSect, int nVal);

////////////////////////////////////////////////////
/***********************************************/
/** SECTOR FUNCTIONS                          **/
/***********************************************/
//////////////////

char sectVisibilitySet(int nSect, int nVal);
char sectVisibilityChg(int nSect, int nVal);
char sectUnderwaterSet(int nSect, int nVal);
char sectOffsetZ(int nSect, int nVal);
char sectDelete(int nSect, int);
char sectFXSetShadeFlags(int nSect, int);
char sectFXFreqChg(int nSect, int nVal);
char sectFXPhaseChg(int nSect, int nVal);
char sectFXAmplitudeChg(int nSect, int nVal);
char sectFXWaveSet(int nSect, int nVal);
char sectStateSet(int nSect, int nVal);
char sectShowOrSetupZMotion(int nSect, int nVal);
char sectClone(int nSect, int);

#endif