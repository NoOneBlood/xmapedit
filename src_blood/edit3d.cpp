/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1996: Originally written by Peter Freese.
// Copyright (C) 2021: Updated by NoOne.
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
#include "xmpstub.h"
#include "misc.h"
#include "gameutil.h"
#include "trig.h"
#include "db.h"
#include "tile.h"
#include "screen.h"
#include "gui.h"
#include "keyboard.h"
#include "gfx.h"
#include "editor.h"
#include "tilefav.h"
#include "prefabs.h"
#include "grdshd.h"
#include "hglt.h"
#include "sectorfx.h"
#include "aadjust.h"
#include "preview.h"
#include "xmphud.h"
#include "xmpexplo.h"
#include "xmpconf.h"
#include "xmptools.h"
#include "xmpsky.h"
#include "xmpmisc.h"

/*******************************************************************************
									Defines
*******************************************************************************/
#define kMaxVisited			kMaxSectors + kMaxWalls	// set to larger of kMaxSectors and kMaxWalls


/*******************************************************************************
								Local Variables
*******************************************************************************/
static BOOL visited[kMaxVisited];

static ushort WallShadeFrac[kMaxWalls];
static ushort FloorShadeFrac[kMaxSectors];
static ushort CeilShadeFrac[kMaxSectors];
static int WallArea[kMaxWalls];
static int SectorArea[kMaxSectors];
static int WallNx[kMaxWalls], WallNy[kMaxWalls];
static int FloorNx[kMaxSectors], FloorNy[kMaxSectors], FloorNz[kMaxSectors];
static int CeilNx[kMaxSectors], CeilNy[kMaxSectors], CeilNz[kMaxSectors];


void SetCeilingZ( int nSector, int z )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	// don't allow to go through the floor
	z = ClipHigh(z, sector[nSector].floorz);

	for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		int zTop, zBot;
		spritetype *pSprite = &sprite[i];
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if ( zTop <= getceilzofslope(nSector, pSprite->x, pSprite->y) )
			pSprite->z += z - sector[nSector].ceilingz;
	}
	sector[nSector].ceilingz = z;
}


void SetFloorZ( int nSector, int z )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	// don't allow to go through the ceiling
	z = ClipLow(z, sector[nSector].ceilingz);

	for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i]) {

		int zTop, zBot;
		spritetype *pSprite = &sprite[i];
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if (zBot >= getflorzofslope(nSector, pSprite->x, pSprite->y))
			pSprite->z += z - sector[nSector].floorz;

	}
	sector[nSector].floorz = z;
}


void SetCeilingSlope( int nSector, int nSlope )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	sector[nSector].ceilingslope = (short)nSlope;

	if ( sector[nSector].ceilingslope == 0 )
		sector[nSector].ceilingstat &= ~kSectSloped;
	else
		sector[nSector].ceilingstat |= kSectSloped;

	for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		spritetype *pSprite = &sprite[i];
		int zTop, zBot;
		GetSpriteExtents(pSprite, &zTop, &zBot);
		int z = getceilzofslope(nSector, pSprite->x, pSprite->y);
		if ( zTop < z )
			sprite[i].z += z - zTop;
	}
}

void SetFloorSlope( int nSector, int nSlope )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	sector[nSector].floorslope = (short)nSlope;

	if ( sector[nSector].floorslope == 0 )
		sector[nSector].floorstat &= ~kSectSloped;
	else
		sector[nSector].floorstat |= kSectSloped;

	for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		spritetype *pSprite = &sprite[i];
		int zTop, zBot;
		GetSpriteExtents(pSprite, &zTop, &zBot);
		int z = getflorzofslope(nSector, pSprite->x, pSprite->y);
		if ( zBot > z )
			sprite[i].z += z - zBot;
	}
}

void SetSectorShadePhase( int nSector, int shadePhase )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	int nXSector = sector[nSector].extra;

	if (nXSector > 0)
	{
		XSECTOR *pXSector = &xsector[nXSector];
		pXSector->shadePhase = shadePhase;
	}
}

void SetSectorTheta( int nSector, int bobTheta )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	int nXSector = sector[nSector].extra;

	if (nXSector > 0)
	{
		XSECTOR *pXSector = &xsector[nXSector];
		pXSector->bobTheta = bobTheta;
	}
}


/***********************************************************************
 * IsSectorHighlight()
 *
 * Determines if a sector is in the sector highlight list
 **********************************************************************/
static BOOL IsSectorHighlight( int nSector )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	for (int i = 0; i < highlightsectorcnt; i++)
		if (highlightsector[i] == nSector)
			return TRUE;

	return FALSE;
}


inline int NextCCW( int nWall )
{
	dassert( wall[nWall].nextwall >= 0 );
	return wall[wall[nWall].nextwall].point2;
}


/*******************************************************************************
	FUNCTION:		GetWallZPeg()

	DESCRIPTION:	Calculate the z position that the wall texture is relative
					to.
*******************************************************************************/
inline int GetWallZPeg( int nWall )
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	int z;

	int nSector = sectorofwall(nWall);
	dassert(nSector >= 0 && nSector < kMaxSectors);

	int nNextSector = wall[nWall].nextsector;

	if (nNextSector == -1)
	{
		// one sided wall
		if ( wall[nWall].cstat & kWallOrgBottom )
			z = sector[nSector].floorz;
		else
			z = sector[nSector].ceilingz;
	}
	else
	{
		// two sided wall
		if ( wall[nWall].cstat & kWallOrgOutside )
			z = sector[nSector].ceilingz;
		else
		{
			// top step
			if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
				z = sector[nNextSector].ceilingz;
			// bottom step
			if (sector[nNextSector].floorz < sector[nSector].floorz)
				z = sector[nNextSector].floorz;
		}
	}
	return z;
}


static void AlignWalls( int nWall0, int z0, int nWall1, int z1, int nTile )
{
	dassert(nWall0 >= 0 && nWall0 < kMaxWalls);
	dassert(nWall1 >= 0 && nWall1 < kMaxWalls);

	//dprintf("¯%d", nWall1);

	// do the x alignment
	wall[nWall1].cstat &= ~kWallFlipMask;    // set to non-flip
	wall[nWall1].xpanning = (BYTE)((wall[nWall0].xpanning + (wall[nWall0].xrepeat << 3)) % tilesizx[nTile]);

	z1 = GetWallZPeg(nWall1);

	int n = picsiz[nTile] >> 4;
	if ( (1 << n) != tilesizy[nTile] )
		n++;

	wall[nWall1].yrepeat = wall[nWall0].yrepeat;
	wall[nWall1].ypanning = (BYTE)(wall[nWall0].ypanning + (((z1 - z0) * wall[nWall0].yrepeat) >> (n + 3)));
}


#define kMaxAlign	64
static void AutoAlignWalls( int nWall0, int ply = 0 )
{
	dassert(nWall0 >= 0 && nWall0 < kMaxWalls);

	int z0, z1;
	int nTile = wall[nWall0].picnum;
	int nWall1;
	int branch = 0;

	if (ply == kMaxAlign )
		return;

	if ( ply == 0 )
	{
		// clear visited bits
		memset(visited, FALSE, sizeof(visited));
		visited[nWall0] = TRUE;
	}

	z0 = GetWallZPeg(nWall0);

	nWall1 = wall[nWall0].point2;
	dassert(nWall1 >= 0 && nWall1 < kMaxWalls);

	// loop through walls at this vertex in CCW order
	while (1)
	{
		// break if this wall would connect us in a loop
		if ( visited[nWall1] )
			break;

		visited[nWall1] = TRUE;

		// break if reached back of left wall
		if ( wall[nWall1].nextwall == nWall0 )
			break;

		if ( wall[nWall1].picnum == nTile )
		{
			z1 = GetWallZPeg(nWall1);
			BOOL visible = FALSE;

			int nNextSector = wall[nWall1].nextsector;
			if ( nNextSector < 0 )
				visible = TRUE;
			else
			{
				// ignore two sided walls that have no visible face
				int nSector = wall[wall[nWall1].nextwall].nextsector;
				if ( getceilzofslope(nSector, wall[nWall1].x, wall[nWall1].y) <
					getceilzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y) )
					visible = TRUE;

				if ( getflorzofslope(nSector, wall[nWall1].x, wall[nWall1].y) >
					getflorzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y) )
					visible = TRUE;
			}

			if ( visible )
			{
				branch++;
				AlignWalls(nWall0, z0, nWall1, z1, nTile);

				int nNextWall = wall[nWall1].nextwall;

				// if wall was 1-sided, no need to recurse
				if ( nNextWall < 0 )
				{
					nWall0 = nWall1;
					z0 = GetWallZPeg(nWall0);
					nWall1 = wall[nWall0].point2;
					branch = 0;
					continue;
				}
				else
				{
					if ( wall[nWall1].cstat & kWallSwap && wall[nNextWall].picnum == nTile )
						AlignWalls(nWall0, z0, nNextWall, z1, nTile);
					AutoAlignWalls(nWall1, ply + 1);
				}
			}
		}

		if (wall[nWall1].nextwall < 0)
			break;

		nWall1 = wall[wall[nWall1].nextwall].point2;
	}

}


static void BuildStairsF( int nSector, int nStepHeight )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				SetFloorZ(j, sector[nSector].floorz - (nStepHeight << 8));
				BuildStairsF(j, nStepHeight);
			}
		}
	}
}


static void BuildStairsC( int nSector, int nStepHeight )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				SetCeilingZ(j, sector[nSector].ceilingz - (nStepHeight << 8));
				BuildStairsC(j, nStepHeight);
			}
		}
	}
}

static void ShootRay(int x, int y, int z, short nSector, int dx, int dy, int dz, int nIntensity, int nReflect, int dist)
{
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	int hitx, hity, hitz;
	int x2, y2, z2;
	int dotProduct;
	int E;

	hitscan(x, y, z, nSector, dx, dy, dz << 4, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz,
		BLOCK_NONE);

	x2 = klabs(x - hitx) >> 4;
	y2 = klabs(y - hity) >> 4;
	z2 = klabs(z - hitz) >> 8;
	dist += ksqrt(x2 * x2 + y2 * y2 + z2 * z2);

	if (hitwall >= 0)
	{
		E = divscale16(nIntensity, ClipLow(dist + gLightBomb.rampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of wall
		E = divscale16(E, ClipLow(WallArea[hitwall], 1));

		int n = (wall[hitwall].shade << 16) | WallShadeFrac[hitwall];
		n -= E;
		wall[hitwall].shade = (schar)ClipLow(n >> 16, gLightBomb.maxBright);
		WallShadeFrac[hitwall] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLightBomb.reflections )
		{
			int Nx = WallNx[hitwall];
			int Ny = WallNy[hitwall];

			// dotProduct is cos of angle of intersection
			dotProduct = dmulscale16(dx, Nx, dy, Ny);
			if (dotProduct < 0)
				return;		// bogus intersection

			// reflect vector
			dx -= mulscale16(2 * dotProduct, Nx);
			dy -= mulscale16(2 * dotProduct, Ny);

			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;

			nIntensity -= mulscale16(nIntensity, gLightBomb.attenuation);

			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
	else if (hitsprite >= 0)
	{
	}
	else if (dz > 0)	// hit floor
	{
		E = divscale16(nIntensity, ClipLow(dist + gLightBomb.rampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of floor
		E = divscale16(E, ClipLow(SectorArea[hitsect], 1));

		int n = (sector[hitsect].floorshade << 16) | FloorShadeFrac[hitsect];
		n -= E;
		sector[hitsect].floorshade = (schar)ClipLow(n >> 16, gLightBomb.maxBright);
		FloorShadeFrac[hitsect] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLightBomb.reflections )
		{
			if ( sector[hitsect].floorstat & kSectSloped )
			{
				int Nx = FloorNx[hitsect];
				int Ny = FloorNy[hitsect];
				int Nz = FloorNz[hitsect];

				// dotProduct is cos of angle of intersection
				dotProduct = tmulscale16(dx, Nx, dy, Ny, dz, Nz);
				if (dotProduct < 0)
					return;		// bogus intersection

				dx -= mulscale16(2 * dotProduct, Nx);
				dy -= mulscale16(2 * dotProduct, Ny);
				dz -= mulscale16(2 * dotProduct, Nz);
			}
			else
				dz = -dz;

			nIntensity -= mulscale16(nIntensity, gLightBomb.attenuation);
			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;
			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
	else				// hit ceiling
	{
		E = divscale16(nIntensity, ClipLow(dist + gLightBomb.rampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of ceiling
		E = divscale16(E, ClipLow(SectorArea[hitsect], 1));

		int n = (sector[hitsect].ceilingshade << 16) | CeilShadeFrac[hitsect];
		n -= E;
		sector[hitsect].ceilingshade = (schar)ClipLow(n >> 16, gLightBomb.maxBright);
		CeilShadeFrac[hitsect] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLightBomb.reflections )
		{
			// reflect vector
			if ( sector[hitsect].ceilingstat & kSectSloped )
			{
				int Nx = CeilNx[hitsect];
				int Ny = CeilNy[hitsect];
				int Nz = CeilNz[hitsect];

				// dotProduct is cos of angle of intersection
				dotProduct = tmulscale16(dx, Nx, dy, Ny, dz, Nz);
				if (dotProduct < 0)
					return;		// bogus intersection

				dx -= mulscale16(2 * dotProduct, Nx);
				dy -= mulscale16(2 * dotProduct, Ny);
				dz -= mulscale16(2 * dotProduct, Nz);
			}
			else
				dz = -dz;

			nIntensity -= mulscale16(nIntensity, gLightBomb.attenuation);
			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;
			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
}

static void SetupLightBomb( void )
{
	sectortype *pSector;
	walltype *pWall;
	int nSector, nWall;

	pSector = &sector[0];
	for ( nSector = 0; nSector < numsectors; nSector++, pSector++ )
	{
		FloorShadeFrac[nSector] = 0;
		CeilShadeFrac[nSector] = 0;
		SectorArea[nSector] = AreaOfSector(pSector);

		pWall = &wall[pSector->wallptr];
		for (nWall = pSector->wallptr; nWall < pSector->wallptr + pSector->wallnum; nWall++, pWall++)
		{
			WallShadeFrac[nWall] = 0;

			int Nx, Ny;

			// calculate normal for wall
			Nx = (wall[pWall->point2].y - pWall->y) >> 4;
			Ny = -(wall[pWall->point2].x - pWall->x) >> 4;

			int length = ksqrt(Nx * Nx + Ny * Ny);

			WallNx[nWall] = divscale16(Nx, length);
			WallNy[nWall] = divscale16(Ny, length);

			int ceilZ, ceilZ1, ceilZ2, floorZ, floorZ1, floorZ2;
			getzsofslope(nSector, pWall->x, pWall->y, &ceilZ1, &floorZ1);
			getzsofslope(nSector, wall[pWall->point2].x, wall[pWall->point2].y, &ceilZ2, &floorZ2);
			ceilZ = (ceilZ1 + ceilZ2) >> 1;
			floorZ = (floorZ1 + floorZ2) >> 1;

			// calculate the area of the wall
			int height = floorZ - ceilZ;

			// red wall?
			if ( pWall->nextsector >= 0 )
			{
				height = 0;

				int nextCeilZ, nextCeilZ1, nextCeilZ2, nextFloorZ, nextFloorZ1, nextFloorZ2;
				getzsofslope(pWall->nextsector, pWall->x, pWall->y, &nextCeilZ1, &nextFloorZ1);
				getzsofslope(pWall->nextsector, wall[pWall->point2].x, wall[pWall->point2].y, &nextCeilZ2, &nextFloorZ2);
				nextCeilZ = (nextCeilZ1 + nextCeilZ2) >> 1;
				nextFloorZ = (nextFloorZ1 + nextFloorZ2) >> 1;

				// floor step up?
				if ( nextFloorZ < floorZ)
					height += floorZ - nextFloorZ;

				// ceiling step down?
				if ( nextCeilZ > ceilZ )
					height += nextCeilZ - ceilZ;
			}

			WallArea[nWall] = length * height >> 8;
		}

		FloorNx[nSector] = 0;
		FloorNy[nSector] = 0;
		FloorNz[nSector] = -0x10000;

		CeilNx[nSector] = 0;
		CeilNy[nSector] = 0;
		CeilNz[nSector] = 0x10000;
	}
}


static void LightBomb( int x, int y, int z, short nSector )
{
	int dx, dy, dz;
	for (int a = kAng90 - kAng60; a <= kAng90 + kAng60; a += kAng15)
	{
		for (int i = 0; i < kAng360; i += kAng360 / 256)
		{
			dx = mulscale30(Cos(i), Sin(a)) >> 16;
			dy = mulscale30(Sin(i), Sin(a)) >> 16;
			dz = Cos(a) >> 16;

			ShootRay(x, y, z, nSector, dx, dy, dz, gLightBomb.intensity, 0, 0);
		}
	}
}


void SetFirstWall( int nSector, int nWall )
{
	int start, length, shift;
	int i, j, k;
	walltype tempWall;

	// rotate the walls using the shift copy algorithm

	start = sector[nSector].wallptr;
	length = sector[nSector].wallnum;

	dassert(nWall >= start && nWall < start + length);
	shift = nWall - start;

	if (shift == 0)
		return;

	i = k = start;

	for (int n = length; n > 0; n--)
	{
		if (i == k)
			tempWall = wall[i];

		j = i + shift;
		while (j >= start + length)
			j -= length;

		if (j == k)
		{
			wall[i] = tempWall;
			i = ++k;
			continue;
		}
		wall[i] = wall[j];
		i = j;
	}

	for (i = start; i < start + length; i++)
	{
		if ( (wall[i].point2 -= shift) < start )
			wall[i].point2 += length;

		if ( wall[i].nextwall >= 0 )
			wall[wall[i].nextwall].nextwall = (short)i;
	}

	CleanUp();
}

static spritetype* InsertGameSprite( int nSector, int x, int y, int z, int nAngle, int group ) {

	int i = 0; short picnum = -1;
	if (!adjFillTilesArray(group) || (picnum = (short)tilePick(-1, -1, OBJ_CUSTOM, "Select game object")) < 0)
		return NULL;

	if ((i = adjIdxByTileInfo(picnum, adjCountSkips(picnum))) >= 0) {

		int nSprite = InsertSprite(nSector, kStatDecoration);
		updatenumsprites();

		spritetype *pSprite = &sprite[nSprite];

		pSprite->picnum = picnum;
		pSprite->type = autoData[i].type;

		AutoAdjustSprites();

		pSprite->x = x; pSprite->y = y; pSprite->z = z;
		pSprite->ang = (short) nAngle;

		if (autoData[i].exception)
			adjSetApperance(pSprite, i);

		clampSprite(pSprite);

		return pSprite;

	}

	return NULL;

}


static spritetype* InsertModernSpriteType(int nSector, int x, int y, int z, int nAngle) {

	int i = 0;
	static int length = 0;
	static NAMED_TYPE modernTypes[64];
	if (!length) {

		for (i = 0; i < spriteNamesLength; i++) {
			if (spriteNames[i].compt != MO) continue;
			modernTypes[length].id = spriteNames[i].id;
			modernTypes[length].name = spriteNames[i].name;
			length++;
		}

	}

	if (!length || (i = showButtons(modernTypes, length, "Modern types")) < mrUser)
		return NULL;

	int nSpr = InsertSprite(nSector, 0);
	spritetype* pSprite = &sprite[nSpr];
	sprite[nSpr].type = (short) (i - mrUser);
	sprite[nSpr].ang  = (short) nAngle;
	sprite[nSpr].x = x, sprite[nSpr].y = y;
	sprite[nSpr].z = z;
	updatenumsprites();
	GetXSprite(nSpr);

	adjSpriteByType(pSprite);

	// set a letter picnum if have nothing in autoData
	if (pSprite->picnum == 0) {

		pSprite->xrepeat = pSprite->yrepeat = 128;
		pSprite->picnum = (short)(4096 + (gSpriteNames[pSprite->type][0] - 32));

	}

	clampSprite(pSprite);
	return pSprite;

}



// modal results for InsertGameObject
enum {
	mrEnemy = mrUser,
	mrWeapon,
	mrAmmo,
	mrItem,
	mrHazard,
	mrMisc,
	mrMarker,
	mrFavesPut,
	mrModern,
	mrPrefabPut,
	mrPrefabAdd,
	mrPrefabRem,
};

/*******************************************************************************
	FUNCTION:		InsertGameObject()

	DESCRIPTION:	Displays a menu allowing user to insert a game-specific
					object. Only ornaments can be placed as wall sprites.

	PARAMETERS:

	NOTES:
*******************************************************************************/
int InsertGameObject( int where, int nSector, int x, int y, int z, int nAngle) {


	int i = 0, pfbAng = 0;
	char* filename = NULL;

	Window dialog(0, 0, 135, ydim, "Game objects");
	dialog.height = 112;

	dialog.Insert(new TextButton(4,   4, 60, 20,  "&Enemy",     mrEnemy));
	dialog.Insert(new TextButton( 66,  4, 60, 20,  "&Weapon",   mrWeapon ));
	dialog.Insert(new TextButton( 4,  26, 60, 20,  "&Ammo",     mrAmmo ));
	dialog.Insert(new TextButton( 66,  26, 60, 20,  "&Item",    mrItem ));
	dialog.Insert(new TextButton( 4,  48, 60, 20,  "&Hazard",   mrHazard ));
	dialog.Insert(new TextButton( 66, 48, 60, 20,  "&Misc",     mrMisc ));
	dialog.Insert(new TextButton( 4,  70, 60, 20,  "Mar&ker",   mrMarker ));
	dialog.Insert(new TextButton( 66, 70, 60, 20,  "&Faves",    mrFavesPut ));
	if (gMisc.showTypes > 1) {

		dialog.Insert(new TextButton( 4, 92, 122, 20,  "Modern &Types",	mrModern ));
		dialog.height+=22;

	}

	dialog.Insert(new Label(6, dialog.height - 8, "PREFABS. . . . . . . . . ."));

	dialog.height+=22;

	dialog.Insert(new TextButton(4,    dialog.height - 20, 40, 20,  "I&ns",     mrPrefabPut));
	dialog.Insert(new TextButton(44,   dialog.height - 20, 42, 20,  "Sa&ve",     mrPrefabAdd));
	dialog.Insert(new TextButton(86,   dialog.height - 20, 40, 20,  "&Del",     mrPrefabRem));

	dialog.height+=22;

	ShowModal(&dialog);

	spritetype* pSprite = NULL; nAngle = (nAngle + kAng180) & kAngMask;
	switch (dialog.endState) {
		case mrCancel:
			break;
		case mrEnemy:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpDude);
			break;
		case mrWeapon:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpWeapon | kOGrpAmmoMix);
			break;
		case mrAmmo:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpAmmo | kOGrpAmmoMix);
			break;
		case mrItem:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpItem);
			break;
		case mrHazard:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpHazard);
			break;
		case mrMisc:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpMisc);
			break;
		case mrMarker:
			pSprite = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpMarker);
			break;
		case mrFavesPut:
			if (gFavTilesC <= 0) Alert("There is no favorite tiles yet.");
			else pSprite = favTileInsert(where, nSector, x, y, z, (short) nAngle);
			break;
		case mrModern:
			pSprite = InsertModernSpriteType(nSector, x, y, z, nAngle);
			break;
		case mrPrefabPut:
			if ((filename = dirBrowse("Insert prefab", gPaths.prefabs, kPrefabFileExt)) == NULL) break;
			switch (pfbInsert(filename, where, nSector, x, y, z)) {
				default: scrSetMessage("The prefab inserted at x:%d, y:%d, z:%d.", x, y, z); return -1;
				case -1: Alert("Must insert in a sector.");   break;
				case -3: Alert("Wrong prefab file version."); break;
				case -4: Alert("Too many sprites!"); return -1;
				//case -5: Alert("This prefab is not designed for walls."); break;
				case -2:
				case  0: // zero sprites was created
					Alert("File \"%s\" is corrupted or not exist.");
					break;
			}
			break;
		case mrPrefabRem:
			while ( 1 ) {
				if ((filename = dirBrowse("Delete prefab", gPaths.prefabs, kPrefabFileExt)) == NULL) break;
				else if (Confirm("Delete file \"%s\"?", filename)) {
					if (pfbRemove(filename))
						scrSetMessage("File deleted.");
				}
			}
			break;
		case mrPrefabAdd:
			if ((i = hgltSprCount()) < kMinPrefabSprites || i >= kMaxPrefabSprites) {
				Alert("You must highlight from %d to %d sprites first.", kMinPrefabSprites, kMaxPrefabSprites);
				break;
			}
			i = 0;
			while ( 1 ) {

				if ((filename = dirBrowse("Save prefab", gPaths.prefabs, kPrefabFileExt, kDirExpTypeSave, 0)) == NULL) return -1;
				if (fileExists(filename) && !Confirm("Overwrite existing file?"))
					continue;

				// pick face side of the prefab (for walls)
				while ( 1 ) {

					switch (pfbAng = pfbDlgFaceAngDefine()) {
						case mrOk:
							Alert("Define face side of the prefab by selecting one of arrows.");
							continue;
						case mrCancel:
							break;
						default:
							if (pfbAng < mrUser) continue;
							else if (pfbAng - mrUser == kAng360 && !Confirm("Are you sure?")) continue;
							else pfbAng -= mrUser;
							i = 1;
							break;
					}

					break;
				}

				if (i > 0)
					break;

			}
			switch (pfbAdd(filename, pfbAng)) {
				case  0:
					Alert("Failed to save prefab. All sprites were wrong!");
					pfbRemove(filename);
					break;
				case -1:
					Alert("Failed to save prefab. Previous file exists!");
					break;
				default:
					scrSetMessage("Prefab saved.");
					break;
			}
			break;
	}

	if (pSprite == NULL)
		return -1;

	switch (pSprite->statnum) {
		case kStatItem:
		case kStatDude:
			pSprite->shade = -8;
			break;
	}

	// handle any special setups here
	switch(pSprite->type) {
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
		case kDudeSpiderMother:
		case kDudeBat:
			if (where != OBJ_CEILING || isSkySector(nSector, searchstat)) break;
			else if (pSprite->type == kDudeBat) pSprite->picnum = 1948; // make picnum ceiling bat
			else pSprite->cstat |= kSprFlipY; // invert ceiling spiders
			break;
		case kThingSpiderWeb:
		case kSwitchToggle:
		case kSwitchOneWay:
		case kSwitchCombo:
		case kSwitchPadlock:
		case kThingCrateFace:
		case kThingWoodBeam:
		case kThingWallCrack:
		case kThingMetalGrate:
		case kTrapMachinegun:
		case kTrapFlame:
		case kGenMissileFireball:
		case kGenMissileEctoSkull:
		case kTrapSawCircular:
		case kTrapPendulum:
		case kTrapGuillotine:
		case kTrapZapSwitchable:
			if (where == OBJ_WALL || where == OBJ_MASKED) break;
			pSprite->cstat |= kSprWall; // auto wall align
			break;
	}

	//gameObjectAdvancedEdit(OBJ_SPRITE, pSprite->index);
	return pSprite->index;
}



enum {
	mrStep = mrUser,
	mrShadePhase,
	mrTheta,
};

static void SectorShadePhase( int nSector, int dPhase )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				int nXSector = sector[nSector].extra;
				if (nXSector > 0)
				{
					dassert(nXSector < kMaxXSectors);
					XSECTOR *pXSector = &xsector[nXSector];

					int shadePhase = ((int)pXSector->shadePhase + dPhase) & kAngMask;
					SetSectorShadePhase(j, shadePhase);
					SectorShadePhase(j, dPhase);
				}
  			}
		}
	}
}

static void SectorTheta( int nSector, int dTheta )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				int nXSector = sector[nSector].extra;
				if (nXSector > 0)
				{
					dassert(nXSector < kMaxXSectors);
					XSECTOR *pXSector = &xsector[nXSector];

					int bobTheta = ((int)pXSector->bobTheta + dTheta) & kAngMask;
					SetSectorTheta(j, bobTheta);
					SectorTheta(j, dTheta);
				}
  			}
		}
	}
}

static BOOL F9Menu( void )
{
	BOOL bChanged = FALSE;

	Window dialog(0, 0, 180, 182, "Sector Tricks");

	dialog.Insert(new Label( 4,  8, "&Value:"));
	EditNumber *peNumber = new EditNumber(44, 4, 80, 16, 0);
	peNumber->hotKey = 'V';
	dialog.Insert(peNumber);	// value field

	dialog.Insert(new TextButton( 4,  24, 80, 20,  "&Step Height",	(int)mrStep ));
	dialog.Insert(new TextButton( 4,  44, 80, 20,  "&Light Phase",	(int)mrShadePhase ));
	dialog.Insert(new TextButton( 4,  64, 80, 20,  "&Z Phase",		(int)mrTheta ));

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrStep:
			switch (searchstat)
			{
				case OBJ_FLOOR:
					// clear visited bits
					memset(visited, FALSE, sizeof(visited));

					BuildStairsF(searchsector, peNumber->value);
					bChanged = TRUE;
					break;

				case OBJ_CEILING:
					// clear visited bits
					memset(visited, FALSE, sizeof(visited));

					BuildStairsC(searchsector, peNumber->value);
					bChanged = TRUE;
					break;
			}
			break;

		case mrShadePhase:
			// clear visited bits
			memset(visited, FALSE, sizeof(visited));

			SectorShadePhase(searchsector, peNumber->value);
			bChanged = TRUE;
			break;

		case mrTheta:
			// clear visited bits
			memset(visited, FALSE, sizeof(visited));

			SectorTheta(searchsector, peNumber->value);
			bChanged = TRUE;
			break;
	}

	return bChanged;
}


void PutSpriteOnCeiling( spritetype *pSprite, int offs)
{
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	dassert(pSprite->sectnum >= 0 && pSprite->sectnum < kMaxSectors);
	
	if ((pSprite->cstat & 48) == 48) // sloped sprites on sloped sectors...
	{ 
		if ((sector[pSprite->sectnum].ceilingstat & kSectSloped)
			&& sector[pSprite->sectnum].ceilingslope != 0)
				zTop = pSprite->z;
	}
	
	pSprite->z += getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zTop;
	pSprite->z += offs;

}


void PutSpriteOnFloor( spritetype *pSprite, int offs)
{
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	dassert(pSprite->sectnum >= 0 && pSprite->sectnum < kMaxSectors);
	
	if ((pSprite->cstat & 48) == 48) // sloped sprites on sloped sectors...
	{ 
		if ((sector[pSprite->sectnum].floorstat & kSectSloped)
			&& sector[pSprite->sectnum].floorslope != 0)
				zBot = pSprite->z;
	}
	
	pSprite->z += getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot;
	pSprite->z += offs;
}


void RaiseSprite( spritetype *pSprite, int nStep )
{
	//pSprite->z = DecNext(pSprite->z, nStep);
	pSprite->z-=nStep;
}


void LowerSprite( spritetype *pSprite, int nStep )
{
	//pSprite->z = IncNext(pSprite->z, nStep);
	pSprite->z+=nStep;
}





void SetCeilingRelative( int nSector, int dz )
{
	SetCeilingZ(nSector, sector[nSector].ceilingz + dz);
}


void SetFloorRelative( int nSector, int dz )
{
	SetFloorZ(nSector, sector[nSector].floorz + dz);
}


void RaiseCeiling( int nSector, int nStep )
{
	SetCeilingZ(nSector, DecNext(sector[nSector].ceilingz, nStep));
}


void RaiseFloor( int nSector, int nStep )
{
	SetFloorZ(nSector, DecNext(sector[nSector].floorz, nStep));
}


void LowerCeiling( int nSector, int nStep )
{
	SetCeilingZ(nSector, IncNext(sector[nSector].ceilingz, nStep));
}


void LowerFloor( int nSector, int nStep )
{
	SetFloorZ(nSector, IncNext(sector[nSector].floorz, nStep));
}


void SetWave( int nSector, int nWave )
{
	int nXSector = sector[nSector].extra;
	if ( nXSector > 0 )
		xsector[nXSector].shadeWave = nWave;
}


static void ProcessHighlightSectors( HSECTORFUNC FloorFunc, int nData ) {

	int i = 0;
	if (gHgltc > 0) {

		// if only pointing in one of selected objects
		for (i = 0; i < kMaxHgltObjects; i++) {
			if (gHglt[i].type != OBJ_FLOOR && gHglt[i].type != OBJ_CEILING) continue;
			else if (gHglt[i].idx >= 0 && gHglt[i].idx == searchsector) {
				for (i = 0; i < kMaxHgltObjects; i++) {
					if (gHglt[i].type == OBJ_FLOOR || gHglt[i].type == OBJ_CEILING)
						FloorFunc(gHglt[i].idx, nData);
				}
				break;
			}
		}

	}

	if (IsSectorHighlight(searchsector)) {

		for (i = 0; i < highlightsectorcnt; i++)
			FloorFunc(highlightsector[i], nData);

	} else {

		FloorFunc(searchsector, nData);

	}

}

void processMouseLook3D(BOOL readMouse) {
	
	if (readMouse)
		gMouse.Read(gFrameClock);

	searchx = ClipRange(gMouse.X, 1, xdim - 2);
	searchy = ClipRange(gMouse.Y, 1, ydim - 2);
	if (!gMouse.look.mode)
		return;

	// free look
	int dx = abs(gMouse.dX2); int dy = abs(gMouse.dY2);
	searchx = xdim >> 1; searchy = ydim >> 1;
	gMouse.X = searchx; gMouse.Y = searchy;
	if (ctrl && keystatus[KEY_PAD5])
	{
		horiz = 100;
	}
	else
	{
		if (gMouse.look.dir & 0x0001)
		{
			if (gMouse.dY2 < 0) horiz = (gMouse.look.invert & 0x1) ? horiz - dy : horiz + dy;
			else if (gMouse.dY2 > 0) horiz = (gMouse.look.invert & 0x1) ? horiz + dy : horiz - dy;
			horiz = ClipRange(horiz, -gMouse.look.maxSlopeF, gMouse.look.maxSlope);
			
			if (zmode == 3 && !keystatus[KEY_A] && !keystatus[KEY_Z])
			{
				int mul = (shift) ? 3 : 2;
				if (keystatus[KEY_DOWN])
				{
					if (horiz < 100) posz -= (((100 - horiz) * gFrameTicks) << mul);
					else if (horiz > 100) posz += (((horiz - 100) * gFrameTicks) << mul);
				}
				
				if (keystatus[KEY_UP])
				{
					if (horiz < 100) posz += (((100 - horiz) * gFrameTicks) << mul);
					else if (horiz > 100) posz -= (((horiz - 100) * gFrameTicks) << mul);
				}
			}
		}
		
		if (gMouse.look.dir & 0x0002)
		{
			if (gMouse.dX2 < 0) ang = (short)((gMouse.look.invert & 0x2) ? ang + dx : ang - dx);
			else if (gMouse.dX2 > 0) ang = (short)((gMouse.look.invert & 0x2) ? ang - dx : ang + dx);
			ang = (short)(ang & kAngMask);
		}
	}
}

void ProcessKeys3D( void )
{
	short hitsect, hitwall, hitsprite;
	int startwall, endwall, nSector, nXSector, nXWall, nXSprite, zTop, zBot, gStep;
	int i, j, x, y, z, changedir, hitx, hity, hitz;
	int sect = -1; static int sectRLdelay = 4;
	searchit = 2;

	BYTE pad5  = keystatus[KEY_PAD5];
	if (gObjectLock.type >= 0)
	{
		if (totalclock < gObjectLock.time)
		{
			switch (searchstat = gObjectLock.type) {
				case OBJ_FLOOR:
				case OBJ_CEILING:
					searchsector = gObjectLock.idx;
					break;
				default:
					searchwall = gObjectLock.idx;
					break;
			}
		}
		else
		{
			gObjectLock.type = gObjectLock.idx = -1;
		}
	}

	processMove();
	processMouseLook3D(TRUE);

	if (!gPreviewMode && searchstat >= 0) {

		static short mhold = 0;
		static BOOL rfirst = FALSE;
		short mpress = (short)(~mhold & gMouse.buttons);
		static int ztofs = 0x80000000, zbofs = 0x80000000;
		static int ox = 0, oy = 0, oz = 0;

		if (mhold) searchit = 0;
		else if (gMouse.controls) {
			if (mhold) asksave = FALSE;
			else gMouse.speedReset = TRUE;
		}

		// highlight objects while *pressing* middle mouse
		if (mpress == 4)
		{
			if (searchstat != OBJ_SPRITE)
			{
				// highlight walls and sectors for gradient shading
				short highIdx = -1;
				short objIdx = (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING) ? searchsector : searchwall;
				if (gResetHighlight)
				{
					gResetHighlight = FALSE;
					if (ctrl) grshHgltObjects(-1);
					else grshUnhgltObjects(-1, TRUE);
				}

				// highlight
				if ((highIdx = grshHighlighted(searchstat, objIdx)) < 0)
				{
					if (gHgltc < kMaxHgltObjects)
					{
						if (!grshAddObjects((schar)searchstat, objIdx))
						{
							BeepFail();
						}
						else
						{
							grshHgltObjects(-1);
							scrSetMessage("%d objects highlighted", gHgltc);
							BeepOk();
						}
					}
					else
					{
						scrSetMessage("Max highlighted objects reached!");
						BeepFail();
					}
				// unhighlight
				}
				else
				{
					grshUnhgltObjects(highIdx, TRUE);
					scrSetMessage("%d objects highlighted", gHgltc);
					BeepFail();
				}
			}
			else if (!sprInHglt(searchwall))
			{
				hgltReset(kHgltSector);
				scrSetMessage("Sprite %d added in a highlight.", searchwall);
				hgltAdd(searchstat, searchwall);
				BeepOk();
			}
			else
			{
				scrSetMessage("Sprite %d removed from a highlight.", searchwall);
				hgltRemove(searchstat, searchwall);
				BeepFail();
			}

		// drag sprites while holding left mouse
		} else if ((mhold & 1) && (searchstat == OBJ_SPRITE && sprite[searchwall].statnum < kStatFree) && (gMouse.controls & 0x0001)) {

			gMouse.speedReset = FALSE;
			gMouse.speedX = ClipLow(gMouse.speedX >> 2, 10);
			gMouse.speedY = ClipLow(gMouse.speedY >> 2, 10);

			gHighSpr = searchwall;
			BOOL inHglt = sprInHglt(gHighSpr);
			spritetype* pSprite = &sprite[gHighSpr];

			if (ztofs == 0x80000000 || zbofs == 0x80000000)
				sprGetZOffsets((short)gHighSpr, &ztofs, &zbofs); // keep z-offsets

			hitsect = hitwall = hitsprite = -1;
			x = 16384; y = divscale14(searchx - xdim / 2, xdim / 2);
			RotateVector(&x, &y, ang);
			hitscan(posx, posy, posz, cursectnum, x, y,
				scaleZ(searchy, horiz), &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);

			x = hitx; y = hity; z = hitz;


			// only change z if right mouse button was hold before the left
			if (hitsect >= 0) {

				if (rfirst)
				{
					gMouse.speedX = 0;
					if (gMouse.dY2)
					{
						int t;
						if (inHglt)
						{
							int dax, day;
							hgltSprAvePoint(&dax, &day);
							t = approxDist(posx - dax, posy - day);
						}
						else
						{
							t = approxDist(posx - pSprite->x, posy - pSprite->y);
						}

						//int t = approxDist(posx - pSprite->x, posy - pSprite->y);
						int t2 = ClipHigh(16 * mulscale14(t, 0x100), 3072);
						z = gMouse.dY2 * t2;

						if (inHglt)
						{
							hgltSprChgXYZ(0, 0, z);
							hgltSprClamp(ztofs, zbofs, (gMouse.dY2 < 0) ? 0x01 : 0x02); // don't allow to go through floor or ceiling keeping the shape and offsets
						}
						else
						{
							pSprite->z += z;
							clampSprite(pSprite); // don't allow to put sprites in floors / ceilings
						}
						
						gMapedHud.SetMsgImp(16, "Moving %d sprites by z:%d", (inHglt) ? hgltSprCount() : 1, abs(z));
					}
				}
				else
				{
					dozCorrection(&z, (!shift) ? 0x3FF : 0x0FF);

					int nGrid = (gMouse.fixedGrid) ? gMouse.fixedGrid : grid;
					doGridCorrection(&x, &y, (shift) ? nGrid << 1 : nGrid);
					if (hitwall >= 0)
					{
						int wallSect = sectorofwall(hitwall);
						doWallCorrection(hitwall, &x, &y, 14);
						if (inside(x, y, wallSect) <= 0)
						{
							int nShift = 24, nx = x, ny = y;
							while(nShift > 0)
							{
								x = nx, y = ny;
								doWallCorrection(hitwall, &x, &y, nShift);
								if (inside(x, y, wallSect) <= 0)
								{
									nShift--;
									continue;
								}

								break;
							}
						}

						if (!inHglt)
						{
							if (mhold & 2) // auto angle sprite to wall while holding right mouse
								pSprite->ang = (short)((GetWallAngle(hitwall) + kAng90) & kAngMask);
						}
						// just rotate sprites
						else if (mpress & 2)
						{
							hgltSprRotate((shift) ? -256 : 256);
							ox = 0;
						}
					}
					// rotate sprites while pressing right mouse
					else if (mpress & 2)
					{
						if (inHglt)
						{
							hgltSprRotate((shift) ? -256 : 256);
							ox = 0;
						}
						else
						{
							short angs = (panm[pSprite->picnum].view == kSprViewFull5) ? 256 : 128;
							pSprite->ang = (short) ((!shift) ? (pSprite->ang + angs) : (pSprite->ang - angs) & kAngMask);
						}
					}

					if (ox != x || oy != y || oz != z)
					{
						if (inHglt)
						{
							int dax, day, daz;
							hgltSprAvePoint(&dax, &day, &daz);
							hgltSprChgXYZ(x - dax, y - day, z - daz);
							if (hitwall >= 0)
								hgltSprPutOnWall(hitwall, x, y);

							hgltSprClamp(ztofs, zbofs); // don't allow to go through floor or ceiling keeping the shape and offsets
						}
						else
						{
							pSprite->x = x;
							pSprite->y = y;
							pSprite->z = z;

							ChangeSpriteSect(gHighSpr, hitsect);
							clampSprite(pSprite); // don't allow to put sprites in floors / ceilings.
						}
					}

					ox = x; oy = y;	oz = z;
					gMapedHud.SetMsgImp(16, "Moving %d sprites at x:%d y:%d z:%d, sct:%d", (inHglt) ? hgltSprCount() : 1, x, y, z, hitsect);
				}
			}
		}
		else if (gHighSpr >= 0)
		{
			ox = oy = oz = 0;
			ztofs = zbofs = 0x80000000;
			gMouse.speedReset = TRUE;
			gHighSpr = -1;
		}
		
		if ((mhold == 1 || mhold == 2) && (gMouse.controls & 0x0002))
		{
			gMouse.speedReset = FALSE;
			gMouse.speedX = gMouse.speedY = 10;

			int step = -1, ztep = (shift) ? 256 : 1024, neigh = -1;
			schar dir = (mhold == 1) ? -1 : 1;

			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED:
				case OBJ_FLOOR:
				case OBJ_CEILING:
					if (!(totalclock < gTimers.sectRL || totalclock > gTimers.sectRL + sectRLdelay)) break;
					else if (searchstat == OBJ_WALL || searchstat == OBJ_MASKED)
					{
						TranslateWallToSector();
						if (searchsector < 0)
						{
							searchsector = (short)sectorofwall(searchwall);
							searchstat = OBJ_CEILING;
						}
					}
					sectRLdelay = 6; gTimers.sectRL = totalclock;
					switch (searchstat) {
						case OBJ_FLOOR:
							step = sector[searchsector].floorz;
							step = (mhold == 1) ? DecNext(step, ztep) : IncNext(step, ztep);
							if ((neigh = nextsectorneighborz(searchsector, sector[searchsector].floorz, 1, dir)) >= 0)
							{
								if (ctrl || abs(sector[neigh].floorz - sector[searchsector].floorz) <= ztep)
								{
									step = sector[neigh].floorz;
									sectRLdelay = 16;
									BeepOk();
								}
							}
							ProcessHighlightSectors(SetFloorZ, step);
							scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
							break;
						case OBJ_CEILING:
							step = sector[searchsector].ceilingz;
							step = (mhold == 1) ? DecNext(step, ztep) : IncNext(step, ztep);
							if ((neigh = nextsectorneighborz(searchsector, sector[searchsector].ceilingz, -1, dir)) >= 0)
							{
								if (ctrl || abs(sector[neigh].ceilingz - sector[searchsector].ceilingz) <= ztep)
								{
									step = sector[neigh].ceilingz;
									sectRLdelay = 16;
									BeepOk();
								}
							}
							ProcessHighlightSectors(SetCeilingZ, step);
							scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
							break;
					}
					if (gMisc.pan) AlignSlopes();
					break;
			}

		}
		else
		{
			sectRLdelay = 3;
			gTimers.sectRL = totalclock;
		}

		short omhold = mhold;
		mhold = gMouse.buttons;

		if (mhold & 3)
		{
			if ((omhold & 1) && !(omhold & 2)) rfirst = FALSE;
			else if ((omhold & 2) && !(omhold & 1))
				rfirst = TRUE;
		}
		else
		{
			rfirst = FALSE;
		}
	}
	
	// draw mouse
	gMouse.Draw();

	if (key == 0 || searchstat < 0) return;
	else if (gPreviewMode)
	{
		switch (key) {
			case KEY_ENTER:
				keystatus[KEY_ENTER] = 0;
				key = KEY_CAPSLOCK;
				break;
		}
	}
		
	switch (key) {
		case KEY_A:
		case KEY_Z:
			if (!alt) break;
			i = (key == KEY_A) ? 0x400 : -0x400;
			kensplayerheight = ClipRange(kensplayerheight + i, 0, 0x10000<<2);
			scrSetMessage("Camera height (gravity): %d", kensplayerheight >> 8);
			if (!gNoclip) zmode = 0;
			break;
		case KEY_CAPSLOCK:
			if (!(shift & 0x01)) break;
			gMouse.look.mode = (gMouse.look.mode) ? 0 : 3; //toggle mouse look on and off
			scrSetMessage("Mouse look is %s", onOff(gMouse.look.mode));
			BeepOk();
			break;
		case KEY_INSERT:
			switch (searchstat) {
				case OBJ_SPRITE:
					scrSetMessage("%d sprite(s) duplicated and stamped.", ClipLow(hgltSprCallFunc(sprClone), 1));
					gObjectLock.type = searchstat;
					gObjectLock.idx  = searchwall;
					gObjectLock.time = totalclock + 256;
					BeepOk();
					break;
				default:
					BeepFail();
					break;
			}
			break;
		case KEY_PAGEUP:
			gStep = 0x400;
			if (shift) gStep = 0x100;
			if (ctrl)  gStep >>=4;
			
			if ((searchstat == OBJ_WALL || searchstat == OBJ_MASKED) && (wall[searchwall].nextsector != -1))
				TranslateWallToSector();

			if (searchstat == OBJ_WALL)
				searchstat = OBJ_CEILING; // adjust ceilings when pointing at white walls

			if (ctrl && !shift)
			{
				switch (searchstat)
				{
					case OBJ_CEILING:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz,
								-1, -1);
							if (nNeighbor == -1)
								nNeighbor = searchsector;

							ProcessHighlightSectors(SetCeilingZ, sector[nNeighbor].ceilingz);
							scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						}
						break;
					case OBJ_FLOOR:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz,
								1, -1);
							if (nNeighbor == -1)
								nNeighbor = searchsector;

							ProcessHighlightSectors(SetFloorZ, sector[nNeighbor].floorz);
							scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						}
						break;
					case OBJ_SPRITE:
						hgltSprCallFunc(PutSpriteOnCeiling, 0);
						scrSetMessage("sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						break;
				}
			}
			else if (alt)
			{
				switch (searchstat)
				{
					case OBJ_CEILING:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						dzInPixels = GetNumberBox("height off floor", dzInPixels, dzInPixels) * 256;
						int dz = (sector[searchsector].floorz - dzInPixels) - sector[searchsector].ceilingz;
						ProcessHighlightSectors(SetCeilingRelative, dz);
						scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						break;
					}
					case OBJ_FLOOR:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						dzInPixels = GetNumberBox("height off ceiling", dzInPixels, dzInPixels) * 256;
						int dz = (sector[searchsector].ceilingz + dzInPixels) - sector[searchsector].floorz;
						ProcessHighlightSectors(SetFloorRelative, dz);
						scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						break;
					}
					case OBJ_SPRITE:
						if (!sprInHglt(searchwall)) break;
						hgltSprPutOnCeiling();
						scrSetMessage("%d sprite(s) put on ceiling keeping the shape.", hgltSprCount());
						break;

				}
			} else {
				switch (searchstat) {
					case OBJ_CEILING:
						ProcessHighlightSectors(RaiseCeiling, gStep);
						scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						break;

					case OBJ_FLOOR:
						ProcessHighlightSectors(RaiseFloor, gStep);
						scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						break;

					case OBJ_SPRITE:
						hgltSprCallFunc(RaiseSprite, gStep);
						scrSetMessage("sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						break;
				}
			}
			if ((searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR) && gMisc.pan) AlignSlopes();
			BeepOk();
			break;

		case KEY_PAGEDN:
			gStep = 0x400;
			if ( shift )
				gStep = 0x100;
			if (ctrl)
				gStep >>=4;

			if ((searchstat == OBJ_WALL || searchstat == OBJ_MASKED) && (wall[searchwall].nextsector != -1))
				TranslateWallToSector();

			if ( searchstat == OBJ_WALL )
				searchstat = OBJ_CEILING; // adjust ceilings when pointing at white walls

			if (ctrl && !shift) {
				switch (searchstat) {
					case OBJ_CEILING: {
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz,
								-1, 1);
							if (nNeighbor == -1)
								nNeighbor = searchsector;

							ProcessHighlightSectors(SetCeilingZ, sector[nNeighbor].ceilingz);
							scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						}
						break;
					case OBJ_FLOOR:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz,
								1, 1);
							if (nNeighbor == -1)
								nNeighbor = searchsector;

							ProcessHighlightSectors(SetFloorZ, sector[nNeighbor].floorz);
							scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						}
						break;
					case OBJ_SPRITE:
						hgltSprCallFunc(PutSpriteOnFloor, 0);
						scrSetMessage("sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						break;
				}
			}
			else if (alt)
			{
				switch (searchstat)
				{
					case OBJ_CEILING:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						dzInPixels = GetNumberBox("height off floor", dzInPixels, dzInPixels) * 256;
						int dz = (sector[searchsector].floorz - dzInPixels) - sector[searchsector].ceilingz;
						ProcessHighlightSectors(SetCeilingRelative, dz);
						scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						break;
					}
					case OBJ_FLOOR:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						dzInPixels = GetNumberBox("height off ceiling", dzInPixels, dzInPixels) * 256;
						int dz = (sector[searchsector].ceilingz + dzInPixels) - sector[searchsector].floorz;
						ProcessHighlightSectors(SetFloorRelative, dz);
						scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						break;
					}
					case OBJ_SPRITE:
						if (!sprInHglt(searchwall)) break;
						hgltSprPutOnFloor();
						scrSetMessage("%d sprite(s) put on floor keeping the shape.", hgltSprCount());
						break;

				}
			} else {
				switch (searchstat) {
					case OBJ_CEILING:
						ProcessHighlightSectors(LowerCeiling, gStep);
						scrSetMessage("sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						break;

					case OBJ_FLOOR:
						ProcessHighlightSectors(LowerFloor, gStep);
						scrSetMessage("sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						break;

					case OBJ_SPRITE:
						hgltSprCallFunc(LowerSprite, gStep);
						scrSetMessage("sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						break;
				}
			}
			if ((searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR) && gMisc.pan) AlignSlopes();
			BeepOk();
			break;
		case KEY_ENTER:
			if (somethingintab == 255)		// must have something to paste
			{
				scrSetMessage("There is nothing in clipboard.");
				BeepFail();
				break;
			}
			
			if (ctrl && alt)
			{
				
			}
			
			if (ctrl)
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						i = searchwall;
						do
						{
							if (shift)
							{
								wall[i].shade = tempshade;
								wall[i].pal = temppal;
							}
							else
							{
								wall[i].picnum = temppicnum;
								switch (somethingintab) {
									case OBJ_WALL:
									case OBJ_MASKED:
										wall[i].xrepeat  = tempxrepeat;
										wall[i].yrepeat  = tempyrepeat;
										wall[i].cstat    = tempcstat;
										break;
								}
								fixrepeats(i);
							}
							i = wall[i].point2;
						}
						while (i != searchwall);
						BeepOk();
						break;
					// paste to all parallax ceilings
					case OBJ_CEILING:
						for (i = 0; i < numsectors; i++)
						{
							if (isSkySector(i, searchstat))
							{
								sector[i].ceilingpicnum = temppicnum;
								sector[i].ceilingshade = tempshade;
								sector[i].ceilingpal = temppal;
								if (somethingintab == OBJ_CEILING || somethingintab == OBJ_FLOOR)
								{
									sector[i].ceilingxpanning = tempxrepeat;
									sector[i].ceilingypanning = tempyrepeat;
									sector[i].ceilingstat = (BYTE)(tempcstat | kSectParallax);
								}
							}
						}
						BeepOk();
						break;
					// paste to all parallax floors
					case OBJ_FLOOR:
						for (i = 0; i < numsectors; i++)
						{
							if (isSkySector(i, searchstat))
							{
								sector[i].floorpicnum = temppicnum;
								sector[i].floorshade = tempshade;
								sector[i].floorpal = temppal;
								if (somethingintab == OBJ_CEILING || somethingintab == OBJ_FLOOR)
								{
									sector[i].floorxpanning = tempxrepeat;
									sector[i].floorypanning = tempyrepeat;
									sector[i].floorstat = (BYTE)(tempcstat | kSectParallax);
								}
							}
						}
						BeepOk();
						break;
					default:
						BeepFail();
						break;
				}
				break;
			}

			if ( shift )	// paste shade and palette
			{
				switch (searchstat)
				{
					case OBJ_WALL:
						wall[searchwall].shade = tempshade;
						wall[searchwall].pal = temppal;
						break;
					case OBJ_CEILING:
						if ( IsSectorHighlight(searchsector) )
						{
							for(i = 0; i < highlightsectorcnt; i++)
							{
								sect = highlightsector[i];
								sector[sect].ceilingshade = tempshade;
								sector[sect].ceilingpal = temppal;
								sector[sect].visibility = tempvisibility;
							}
						}
						else
						{
							sector[searchsector].ceilingshade = tempshade;
							sector[searchsector].ceilingpal = temppal;
							sector[searchsector].visibility = tempvisibility;

							// if this sector is a parallaxed sky...
							if (isSkySector(searchsector, searchstat))
							{
								// propagate shade data on all parallaxed sky sectors
								for (i = 0; i < numsectors; i++)
									if (isSkySector(i, searchstat))
									{
										sector[i].ceilingshade = tempshade;
										sector[i].ceilingpal = temppal;
										sector[i].visibility = tempvisibility;
									}
							}
						}
						break;

					case OBJ_FLOOR:
						if (IsSectorHighlight(searchsector))
						{
							for(i = 0; i < highlightsectorcnt; i++)
							{
								sect = highlightsector[i];
								sector[sect].floorshade = tempshade;
								sector[sect].floorpal = temppal;
								sector[sect].visibility = tempvisibility;
							}
						}
						else
						{
							sector[searchsector].floorshade = tempshade;
							sector[searchsector].floorpal = temppal;
							sector[searchsector].visibility = tempvisibility;
						}
						break;
					case OBJ_SPRITE:
						sprite[searchwall].shade = tempshade;
						sprite[searchwall].pal = temppal;
						break;
					case OBJ_MASKED:
						wall[searchwall].shade = tempshade;
						wall[searchwall].pal = temppal;
						break;
				}
				BeepOk();
				break;
			}

			if (alt || keystatus[KEY_SPACE])	// copy extra structures
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						if (somethingintab == OBJ_WALL || somethingintab == OBJ_MASKED)
						{
							int oextra = wall[searchwall].extra;
							wall[searchwall].extra = tempextra;
							wall[searchwall].type = temptype;
							CleanUp();

							if (tempextra > 0) scrSetMessage("X-properties pasted.");
							else if (oextra > 0) scrSetMessage("X-properties cleared.");
							BeepOk();
						}
						else
						{
							scrSetMessage("Clipboard object is not a wall!");
							BeepFail();
						}
						break;
					case OBJ_CEILING:
					case OBJ_FLOOR:
						if (somethingintab == OBJ_CEILING || somethingintab == OBJ_FLOOR) {

							int oextra = sector[searchsector].extra;
							sector[searchsector].extra = tempextra;
							sector[searchsector].type = temptype;
							BeepOk();
							CleanUp();
							if (tempextra > 0) {

								int nIdx1, nIdx2;
								findSectorMarker(searchsector, &nIdx1, &nIdx2);

								if (nIdx1 >= 0) {
									sprite[nIdx1].x = wall[sector[searchsector].wallptr].x;
									sprite[nIdx1].y = wall[sector[searchsector].wallptr].y;
									ChangeSpriteSect(nIdx1, searchsector);
								}

								if (nIdx2 >= 0) {
									sprite[nIdx2].x = wall[sector[searchsector].wallptr].x;
									sprite[nIdx2].y = wall[sector[searchsector].wallptr].y;
									ChangeSpriteSect(nIdx2, searchsector);
								}


								CleanUp();
								scrSetMessage("X-properties pasted.");

							} else if (oextra > 0) {
								scrSetMessage("X-properties cleared.");
							}

						} else {
							scrSetMessage("Clipboard object is not a sector!");
							BeepFail();
						}
						break;
					case OBJ_SPRITE:
						if (somethingintab == OBJ_SPRITE) {

							int oextra = sprite[searchwall].extra;
							sprite[searchwall].type = temptype;
							sprite[searchwall].extra = tempextra;
							CleanUp();

							if (tempextra > 0) scrSetMessage("X-properties pasted.");
							else if (oextra > 0) scrSetMessage("X-properties cleared.");
							BeepOk();

						} else {
							scrSetMessage("Clipboard object is not a sprite!");
							BeepFail();
						}
						break;
				}
				break;
			}

			switch (searchstat)
			{
				case OBJ_WALL:
				case OBJ_MASKED:
					wall[searchwall].pal = temppal;
					wall[searchwall].shade = tempshade;
					switch (searchstat) {
						case OBJ_WALL:
							wall[searchwall].picnum = temppicnum;
							break;
						default:
							wall[searchwall].overpicnum = temppicnum;
							if (wall[searchwall].nextwall >= 0)
								wall[wall[searchwall].nextwall].overpicnum = temppicnum;
							break;
					}

					if (somethingintab == searchstat) {

						wall[searchwall].xrepeat = tempxrepeat;
						wall[searchwall].yrepeat = tempyrepeat;
						//wall[searchwall].xpanning = tempxoffset;
						//wall[searchwall].ypanning = tempyoffset;
						wall[searchwall].cstat = tempcstat;


						if (sectorofwall(tempidx) < 0 || getWallLength(tempidx) != getWallLength(searchwall))
							fixrepeats(searchwall);

					}
					break;
				case OBJ_CEILING:
					sector[searchsector].ceilingpicnum = temppicnum;
					sector[searchsector].ceilingshade = tempshade;
					sector[searchsector].ceilingpal = temppal;
					switch (somethingintab) {
						case OBJ_CEILING:
						case OBJ_FLOOR:
							sector[searchsector].visibility 		= tempvisibility;
							sector[searchsector].ceilingxpanning 	= tempxrepeat;
							sector[searchsector].ceilingypanning 	= tempyrepeat;
							int ostat = sector[searchsector].ceilingstat;
							sector[searchsector].ceilingstat = (BYTE)tempcstat;
							if ((ostat & kSectSloped) && !(sector[searchsector].ceilingstat & kSectSloped))
								sector[searchsector].ceilingstat |= kSectSloped;
							break;
					}
					if (isSkySector(searchsector, searchstat))
					{
						Sky::FixPan(searchsector, searchstat, FALSE);
						Sky::Setup(searchsector, searchstat, sector[searchsector].ceilingshade, sector[searchsector].ceilingpal, sector[searchsector].ceilingpicnum, -1, -1, FALSE);
					}
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorpicnum = temppicnum;
					sector[searchsector].floorshade = tempshade;
					sector[searchsector].floorpal = temppal;
					switch (somethingintab) {
						case OBJ_CEILING:
						case OBJ_FLOOR:
							sector[searchsector].visibility 	= tempvisibility;
							sector[searchsector].floorxpanning	= tempxrepeat;
							sector[searchsector].floorypanning	= tempyrepeat;
							int ostat = sector[searchsector].floorstat;
							sector[searchsector].floorstat = (BYTE)tempcstat;
							if ((ostat & kSectSloped) && !(sector[searchsector].floorstat & kSectSloped))
								sector[searchsector].floorstat |= kSectSloped;
							break;
					}
					if (isSkySector(searchsector, searchstat))
					{
						Sky::FixPan(searchsector, searchstat, FALSE);
						Sky::Setup(searchsector, searchstat, sector[searchsector].floorshade, sector[searchsector].floorpal, sector[searchsector].floorpicnum, -1, -1, FALSE);
					}
					break;
				case OBJ_SPRITE:
					sprite[searchwall].picnum = temppicnum;
					sprite[searchwall].shade = tempshade;
					sprite[searchwall].pal = temppal;
					if (somethingintab == searchstat)
					{
						sprite[searchwall].xrepeat = (char)ClipLow(tempxrepeat, 1);
						sprite[searchwall].yrepeat = (char)ClipLow(tempyrepeat, 1);
						//sprite[searchwall].xoffset = tempxoffset;
						//sprite[searchwall].yoffset = tempyoffset;
						sprite[searchwall].cstat = tempcstat;
						if ((sprite[searchwall].cstat & kSprRelMask) == kSprVoxel)
							spriteSetSlope(searchwall, tempslope);
						
					}
					clampSprite(&sprite[searchwall]);
					break;

			}
			BeepOk();
			break;

		case KEY_LBRACE:
			if (searchstat == OBJ_WALL || searchstat == OBJ_MASKED) TranslateWallToSector();
			if ((searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR) && sector[searchsector].alignto)
			{
				scrSetMessage("Sector must be non auto-aligned!");
				BeepFail();
				break;
			}
			
			gStep = (shift) ? 32 : 256;
			switch (searchstat) {
				case OBJ_CEILING:
					if (IsSectorHighlight(searchsector))
					{
						for (i = 0; i < highlightsectorcnt; i++)
							SetCeilingSlope(highlightsector[i], DecNext(sector[highlightsector[i]].ceilingslope, gStep));
						scrSetMessage("adjusted %i ceilings by %i", highlightsectorcnt, gStep);
					}
					else
					{
						SetCeilingSlope(searchsector, DecNext(sector[searchsector].ceilingslope, gStep));
						scrSetMessage("sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingslope);
					}
					BeepOk();
					break;
				case OBJ_FLOOR:
					if ( IsSectorHighlight(searchsector) ) {
						for(i = 0; i < highlightsectorcnt; i++)
							SetFloorSlope(highlightsector[i], DecNext(sector[highlightsector[i]].floorslope, gStep));
						scrSetMessage("adjusted %i floors by %i", highlightsectorcnt, gStep);
					} else {
						SetFloorSlope(searchsector, DecNext(sector[searchsector].floorslope, gStep));
						scrSetMessage("sector[%i].floorslope: %i", searchsector, sector[searchsector].floorslope);
					}
					BeepOk();
					break;
				case OBJ_SPRITE: {
					short cstat = (short)(sprite[searchwall].cstat & 48);
					if (cstat == 32 || cstat == 48) {
 						short newslope = 0, oldslope = spriteGetSlope(searchwall);
						if (ctrl) newslope = sector[sprite[searchwall].sectnum].floorslope;
						else if (alt) newslope = sector[sprite[searchwall].sectnum].ceilingslope;
						else newslope = (short)DecNext(oldslope, gStep);
						spriteSetSlope(searchwall, newslope);
						scrSetMessage("sprite[%i].slope: %i", searchwall, newslope);
						BeepOk();

					} else {

						scrSetMessage("Sprite #%d must be floor aligned!", searchwall);
						BeepFail();

					}
					break;
				}
			}
			break;
		case KEY_RBRACE:
			if (searchstat == OBJ_WALL || searchstat == OBJ_MASKED) TranslateWallToSector();
			if ((searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR) && sector[searchsector].alignto)
			{
				scrSetMessage("Sector must be non auto-aligned!");
				BeepFail();
				break;
			}
						
			if (alt)
			{
				short nextSector = wall[searchwall].nextsector;
				if (nextSector < 0) {
					BeepFail();
					break;
				}

				x = wall[searchwall].x;
				y = wall[searchwall].y;
				switch (searchstat) {
					case OBJ_CEILING:
						alignceilslope(searchsector, x, y, getceilzofslope(nextSector, x, y));
						BeepOk();
						break;
					case OBJ_FLOOR:
						alignflorslope(searchsector, x, y, getflorzofslope(nextSector, x, y));
						BeepOk();
						break;
					default:
						BeepFail();
						break;
				}
				break;
			}

			gStep = (shift) ? 32 : 256;
			switch (searchstat) {
				case OBJ_CEILING:
					if (IsSectorHighlight(searchsector)) {
						for (i = 0; i < highlightsectorcnt; i++)
							SetCeilingSlope(highlightsector[i], IncNext(sector[highlightsector[i]].ceilingslope, gStep));
						scrSetMessage("adjusted %i ceilings by %i", highlightsectorcnt, gStep);
					} else {
						SetCeilingSlope(searchsector, IncNext(sector[searchsector].ceilingslope, gStep));
						scrSetMessage("sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingslope);
					}
					BeepOk();
					break;
				case OBJ_FLOOR:
					if (IsSectorHighlight(searchsector)) {
						for(i = 0; i < highlightsectorcnt; i++)
							SetFloorSlope(highlightsector[i], IncNext(sector[highlightsector[i]].floorslope, gStep));
						scrSetMessage("adjusted %i floors by %i", highlightsectorcnt, gStep);
					} else {
						SetFloorSlope(searchsector, IncNext(sector[searchsector].floorslope, gStep));
						scrSetMessage("sector[%i].floorslope: %i", searchsector, sector[searchsector].floorslope);
					}
					BeepOk();
					break;
				case OBJ_SPRITE: {
					short cstat = (short)(sprite[searchwall].cstat & 48);
					if (cstat == 32 || cstat == 48) {
 						short newslope = 0;
						short oldslope = spriteGetSlope(searchwall);
						if (ctrl) newslope = sector[sprite[searchwall].sectnum].floorslope;
						else if (alt) newslope = sector[sprite[searchwall].sectnum].ceilingslope;
						else newslope = (short)IncNext(oldslope, gStep);
						spriteSetSlope(searchwall, newslope);
						scrSetMessage("sprite[%i].slope: %i", searchwall, newslope);
						BeepOk();

					} else {

						scrSetMessage("Sprite #%d must be floor aligned!", searchwall);
						BeepFail();

					}
					break;
				}
			}
			break;
		case KEY_COMMA:
			switch (searchstat)
			{
				case OBJ_SPRITE:
					gStep = shift ? 16 : 256;
					i = searchwall;
					sprite[i].ang = (short)(IncNext(sprite[i].ang, gStep) & kAngMask);
					scrSetMessage("sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
					BeepOk();
					break;

				default:
					BeepFail();
			}
			break;

		case KEY_PERIOD:
			switch (searchstat)
			{
				case OBJ_SPRITE:
					gStep = shift ? 16 : 256;
					i = searchwall;
					sprite[i].ang = (short)(DecNext(sprite[i].ang, gStep) & kAngMask);
					scrSetMessage("sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
					BeepOk();
					break;

				// search and fix panning to the right
				case OBJ_WALL:
				case OBJ_MASKED:
					AutoAlignWalls(searchwall);
					BeepOk();
					break;

				default:
					BeepFail();
			}
			break;

		case KEY_BACKSLASH:		// Reset slope to 0
			if (searchstat == OBJ_WALL || searchstat == OBJ_MASKED) TranslateWallToSector(); 
			if ((searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR) && sector[searchsector].alignto) {
				scrSetMessage("Sector must be non auto-aligned!");
				BeepFail();
				break;
			}

			switch (searchstat) {
				case OBJ_CEILING:
					sector[searchsector].ceilingslope = 0;
					sector[searchsector].ceilingstat &= ~kSectSloped;
					scrSetMessage("sector[%i] ceiling slope reset", searchsector);
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorslope = 0;
					sector[searchsector].floorstat &= ~kSectSloped;
					scrSetMessage("sector[%i] floor slope reset", searchsector);
					break;
				case OBJ_SPRITE: {
					short cstat = (short)(sprite[searchwall].cstat & 48);
					if (cstat == 32 || cstat == 48) {
						spriteSetSlope(searchwall, 0);
						scrSetMessage("sprite[%i] slope reset", searchwall);
					}
					break;
				}
			}
			BeepOk();
			break;
		case KEY_MINUS:
			// decrease sector visibility
			if (ctrl && alt)
			{
				if ( IsSectorHighlight(searchsector) )
				{
					for (i = 0; i < highlightsectorcnt; i++)
					{
						// higher numbers are less visibility
						sector[highlightsector[i]].visibility++;
					}
					scrSetMessage("highlighted sectors less visible");
				}
				else
				{
					// higher numbers are less visibility
					sector[searchsector].visibility++;
					scrSetMessage("Visibility: %i", sector[searchsector].visibility);
				}
				BeepOk();
				break;
			}

			// decrease lighting effect amplitude
			if ( ctrl )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].amplitude++;
				scrSetMessage("Amplitude: %i", xsector[nXSector].amplitude);
				BeepOk();
				break;
			}

			// decrease lighting effect phase
			if ( shift )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].shadePhase -= 6;
				scrSetMessage("Phase: %i", xsector[nXSector].shadePhase);
				BeepOk();
				break;
			}

			// adjust gamma level
			if ( keystatus[KEY_G] )
			{
				gGamma = ClipLow(gGamma - 1, 0);
				scrSetMessage("Gamma correction level %i", gGamma);
				scrSetGamma(gGamma);
				scrSetDac();
				break;
			}

			if ( keystatus[KEY_D] )
			{
				visibility = ClipHigh(visibility + 16, 4096);
				scrSetMessage("Global visibility %i", visibility);
				BeepOk();
				break;
			}
			break;

		case KEY_PLUS:
			// increase sector visibility
			if ( ctrl && alt )
			{
				if ( IsSectorHighlight(searchsector) )
				{
					for (i = 0; i < highlightsectorcnt; i++)
					{
						sector[highlightsector[i]].visibility--;
					}
					scrSetMessage("highlighted sectors more visible");
				}
				else
				{
					// lower numbers are more visibility
					sector[searchsector].visibility--;
					scrSetMessage("Visibility: %i", sector[searchsector].visibility);
				}
				BeepOk();
				break;
			}

			// increase lighting effect amplitude
			if ( ctrl )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].amplitude--;
				scrSetMessage("Amplitude: %i", xsector[nXSector].amplitude);
				BeepOk();
				break;
			}

			// increae lighting effect phase
			if ( shift )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].shadePhase += 6;
				scrSetMessage("Phase: %i", xsector[nXSector].shadePhase);
				BeepOk();
				break;
			}

			// adjust gamma level
			if ( keystatus[KEY_G] )
			{
				gGamma = ClipHigh(gGamma + 1, gGammaLevels - 1);
				scrSetMessage("Gamma correction level %i", gGamma);
				scrSetGamma(gGamma);
				scrSetDac();
				break;
			}

			if ( keystatus[KEY_D] )
			{

				visibility = ClipLow(visibility -16, 0);
				scrSetMessage("Global visibility %i", visibility);
				BeepOk();
				break;
			}
			break;
		case KEY_C:
			if (alt)	// change all tiles matching target to tab picnum
			{
				if (somethingintab != searchstat)
				{
					BeepFail();
					break;
				}

				switch (searchstat)
				{
					case OBJ_WALL:
						j = wall[searchwall].picnum;
						if ( TestBitString(show2dwall, searchwall) )	// highlighted?
						{
							for (int i = 0; i < highlightcnt; i++)
								if ( (highlight[i] & 0xC000) == 0 )
								{
									int nWall = highlight[ i ];
									if (wall[nWall].picnum == j)
									{
										if (wall[nWall].picnum != temppicnum)
											wall[nWall].picnum = temppicnum;
										else if (wall[nWall].pal != temppal)
											wall[nWall].pal = temppal;
									}
								}
						} else {
							for ( i = 0; i < numwalls; i++)
								if (wall[i].picnum == j)
								{
									if (wall[i].picnum != temppicnum)
										wall[i].picnum = temppicnum;
									else if (wall[i].pal != temppal)
										wall[i].pal = temppal;
								}
						}
						break;

					case OBJ_CEILING:
						j = sector[searchsector].ceilingpicnum;
						for (i = 0; i < numsectors; i++)
							if (sector[i].ceilingpicnum == j)
							{
								if (sector[i].ceilingpicnum != temppicnum)
									sector[i].ceilingpicnum = temppicnum;
								else if (sector[i].ceilingpal != temppal)
									sector[i].ceilingpal = temppal;
							}
						break;

					case OBJ_FLOOR:
						j = sector[searchsector].floorpicnum;
						for (i = 0; i < numsectors; i++)
							if (sector[i].floorpicnum == j)
							{
								if (sector[i].floorpicnum != temppicnum)
									sector[i].floorpicnum = temppicnum;
								else if (sector[i].floorpal != temppal)
									sector[i].floorpal = temppal;
							}
						break;

					case OBJ_SPRITE:
						j = sprite[searchwall].picnum;
						for (i = 0; i < kMaxSprites; i++)
							if (sprite[i].statnum < kMaxStatus && sprite[i].picnum == j)
							{
								sprite[i].picnum = temppicnum;
								sprite[i].type = temptype;
							}
						break;

					case OBJ_MASKED:
						j = wall[searchwall].overpicnum;
						for (i = 0; i < numwalls; i++)
							if (wall[i].overpicnum == j)
							{
								wall[i].overpicnum = temppicnum;
							}
						break;
				}
				BeepOk();
				break;
			}
			break;
		case KEY_E:	// E (expand)
			switch (searchstat) {
				default:
					BeepFail();
					break;
				case OBJ_CEILING:
					sector[searchsector].ceilingstat ^= kSectExpand;
					scrSetMessage("ceiling texture %s expanded", isNot(sector[searchsector].ceilingstat & kSectExpand));
					BeepOk();
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorstat ^= kSectExpand;
					scrSetMessage("floor texture %s expanded", isNot(sector[searchsector].floorstat & kSectExpand));
					BeepOk();
					break;
			}
			break;
		case KEY_F:
			if (ctrl) {
				gFogMode = !gFogMode;
				scrLoadPLUs();
				scrSetMessage("Fog mode: %s", gBoolNames[gFogMode]);
				BeepOk();

			} else if (alt) {
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						nSector = sectorofwall(searchwall);
						sector[searchsector].ceilingstat &= ~kSectFlipMask;
						sector[searchsector].ceilingstat |= kSectRelAlign;
						SetFirstWall(nSector, searchwall);
						BeepOk();
						break;
					case OBJ_CEILING:
						sector[searchsector].ceilingstat &= ~kSectFlipMask;
						sector[searchsector].ceilingstat |= kSectRelAlign;
						SetFirstWall(searchsector, sector[searchsector].wallptr + 1);
						BeepOk();
						break;
					case OBJ_FLOOR:
						sector[searchsector].floorstat &= ~kSectFlipMask;
						sector[searchsector].floorstat |= kSectRelAlign;
						SetFirstWall(searchsector, sector[searchsector].wallptr + 1);
						BeepOk();
						break;
					default:
						BeepFail();
				}
			} else {
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						switch((i = (wall[searchwall].cstat & kWallFlipMask))) {
							case 0: i = kWallFlipX; break;
							case kWallFlipX: i = kWallFlipX | kWallFlipY; break;
							case kWallFlipX | kWallFlipY: i = kWallFlipY; break;
							case kWallFlipY: i = 0;  break;
						}

						wall[searchwall].cstat &= ~kWallFlipMask;
						wall[searchwall].cstat |= (short)i;
						sprintf(buffer, "wall[%i]", searchwall);
						if (wall[searchwall].cstat & kWallFlipX)
							strcat(buffer," x-flipped");

						if (wall[searchwall].cstat & kWallFlipY) {
							if (wall[searchwall].cstat & kWallFlipX)
								strcat(buffer," and");
							strcat(buffer," y-flipped");
						}

						scrSetMessage(buffer);
						BeepOk();
						break;
					case OBJ_CEILING:
					case OBJ_FLOOR:
						if (searchstat == OBJ_CEILING) i = sector[searchsector].ceilingstat & kSectFlipMask;
						else i = sector[searchsector].floorstat & kSectFlipMask;
						switch (i) {
							case 0x00: i = 0x10; break;
							case 0x10: i = 0x30; break;
							case 0x30: i = 0x20; break;
							case 0x20: i = 0x04; break;
							case 0x04: i = 0x14; break;
							case 0x14: i = 0x34; break;
							case 0x34: i = 0x24; break;
							case 0x24: i = 0x00; break;
						}
						if (searchstat == OBJ_CEILING) {
							sector[searchsector].ceilingstat &= ~kSectFlipMask;
							sector[searchsector].ceilingstat |= (BYTE)i;
						} else {
							sector[searchsector].floorstat &= ~kSectFlipMask;
							sector[searchsector].floorstat |= (BYTE)i;
						}
						break;
					case OBJ_SPRITE:
						i = sprite[searchwall].cstat;

						// two-sided floor sprite?
						if ((i & kSprRelMask) == kSprFloor && !(i & kSprOneSided)) {

							// what the hell is this supposed to be doing?
							sprite[searchwall].cstat &= ~kSprFlipY;
							sprite[searchwall].cstat ^= kSprFlipX;

						} else {

							i = i & 0xC;
							switch(i) {
								case 0x0: i = 0x4; break;
								case 0x4: i = 0xC; break;
								case 0xC: i = 0x8; break;
								case 0x8: i = 0x0; break;
							}
							sprite[searchwall].cstat &= ~0xC;
							sprite[searchwall].cstat |= (short)i;

						}

						sprintf(buffer, "sprite[%i]", searchwall);
						if (sprite[searchwall].cstat & kSprFlipX)
							strcat(buffer," x-flipped");

						if (sprite[searchwall].cstat & kSprFlipY) {
							if (sprite[searchwall].cstat & kSprFlipX) strcat(buffer," and");
							strcat(buffer," y-flipped");
						}
						scrSetMessage(buffer);
						break;
				}
				BeepOk();
			}
			break;
		case KEY_G:
			if (shift || (!alt && gHighSpr >= 0))
			{
				if (!ctrl)
				{
					grid = (short)ClipLow(IncRotate(grid, 7), 1);
					scrSetMessage("Grid size: %d", grid);
				}
				else
				{
					gridlock = (gridlock) ? FALSE : TRUE;
					scrSetMessage("Gridlock is %s", gBoolNames[gridlock]);
				}
				
				BeepOk();
			}
			else if (alt)
			{
				// change global palette
				gMisc.palette = (BYTE)IncRotate(gMisc.palette, kPalMax);
				scrSetMessage("Screen palette #%d", gMisc.palette);
				scrSetPalette(gMisc.palette);
				BeepOk();
			}
			else
			{
				short hidx = -1;
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						hidx = grshHighlighted(searchstat, searchwall);
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						hidx = grshHighlighted(searchstat, searchsector);
						break;
					default:
						break;
				}

				if (hidx >= 0)
				{
					if (gHgltc > 1)
					{
						grshShadeWalls((ctrl && gResetHighlight) ? TRUE : FALSE);
					}
					else
					{
						scrSetMessage("Must highlight at least 2 objects!");
						BeepFail();
					}
				}
				else
				{
					scrSetMessage("Object is not highlighted!");
					BeepFail();
				}
			}
			break;
		case KEY_H:		// H (hitscan sensitivity)
			switch (searchstat)
			{
				case OBJ_FLOOR:
				case OBJ_CEILING:
					// no break
				case OBJ_WALL:
				case OBJ_MASKED:
					wallCstatToggle(searchwall, kWallHitscan, !shift);
					scrSetMessage("wall[%i] %s hitscan sensitive", searchwall, isNot(wall[searchwall].cstat & kWallHitscan));
					BeepOk();
					break;
				case OBJ_SPRITE:
					sprite[searchwall].cstat ^= kSprHitscan;
					scrSetMessage("sprite[%i] %s hitscan sensitive", searchwall, isNot(sprite[searchwall].cstat & kSprHitscan));
					BeepOk();
					break;
				default:
					BeepFail();
			}
			break;
		case KEY_L:
			switch (searchstat) {
				case OBJ_SPRITE: {
					GetSpriteExtents(&sprite[searchwall], &zTop, &zBot);
					ushort oldcstat = sprite[searchwall].cstat;
					sprite[searchwall].cstat &= ~kSprHitscan;
					SetupLightBomb();
					LightBomb(sprite[searchwall].x, sprite[searchwall].y, zTop, sprite[searchwall].sectnum);
					sprite[searchwall].cstat = oldcstat;
					BeepOk();
					break;
				}
				case OBJ_FLOOR:
				case OBJ_CEILING:
					Beep(Sky::ToggleFloorShade(searchsector, alt));
					break;
				default:
					BeepFail();
					break;
			}
			break;
		case KEY_O:
			switch (searchstat) {
				default:
					BeepFail();
					break;
				// O (top/bottom orientation - for doors)
				case OBJ_WALL:
				case OBJ_MASKED:
					wall[searchwall].cstat ^= kWallOrgBottom;
					if (wall[searchwall].nextwall == -1) scrSetMessage("Texture pegged at %s", (wall[searchwall].cstat & kWallOrgBottom) ? "bottom" : "top");
					else scrSetMessage("Texture pegged at %s", (wall[searchwall].cstat & kWallOrgOutside) ? "outside" : "inside");
					BeepOk();
					break;
				// O (ornament onto wall)
				case OBJ_SPRITE:
				{
					if (alt) {
						sprite[searchwall].cstat ^= kSprOrigin;
						scrSetMessage("sprite[%d] origin align is %s", searchwall, onOff((sprite[searchwall].cstat & kSprOrigin)));
						BeepOk();
						break;
					}
					if (!ctrl) break;
					else if (sprite[searchwall].z > sector[sprite[searchwall].sectnum].floorz) {
						scrSetMessage("sprite[%d].z is below floor", searchwall);
						BeepFail();
						break;
					}

					if (sprite[searchwall].z < sector[sprite[searchwall].sectnum].ceilingz) {
						scrSetMessage("sprite[%d].z is above ceiling", searchwall);
						BeepFail();
						break;
					}

					int hitType = HitScan(&sprite[searchwall], sprite[searchwall].z,Cos(ang) >> 16, Sin(ang) >> 16, 0, BLOCK_NONE, 0);
					if (hitType != OBJ_WALL && hitType != OBJ_MASKED) {
						BeepFail();
						break;
					}

					int nx, ny;
					GetWallNormal(gHitInfo.hitwall, &nx, &ny);
					sprite[searchwall].x = gHitInfo.hitx + (nx >> 14);
					sprite[searchwall].y = gHitInfo.hity + (ny >> 14);
					sprite[searchwall].z = gHitInfo.hitz;
					ChangeSpriteSect(searchwall, gHitInfo.hitsect);
					sprite[searchwall].ang = (short)((GetWallAngle(gHitInfo.hitwall) + kAng90) & kAngMask);
					BeepOk();
					break;
				}
			}
			break;
		case KEY_P: {
			if (ctrl)
			{
				int nTile = -1;
				switch (searchstat) {
					case OBJ_FLOOR:
						if (!isSkySector(searchsector, searchstat)) break;
						else nTile = sector[searchsector].floorpicnum;
						break;
					case OBJ_CEILING:
						if (!isSkySector(searchsector, searchstat)) break;
						else nTile = sector[searchsector].ceilingpicnum;
						break;
				}

				if (Beep(nTile >= 0))
				{
					if (alt)
					{
						Sky::ToggleBits(nTile);
					}
					else
					{
						parallaxtype = (char)IncRotate(parallaxtype, 3);
						scrSetMessage("Parallax type: %i", parallaxtype);
					}
				}
				else
				{
					scrSetMessage("Must aim in parallax sector.");
				}
				
				break;
			}
			
			if (alt || shift)
			{
				char nPlu;
				short nPic, nIdx, nStat = searchstat;
				
				i = 1;
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						nPic = (searchstat == OBJ_MASKED) ? wall[searchwall].overpicnum : wall[searchwall].picnum;
						nPlu = wall[searchwall].pal;
						nIdx = searchwall;
						if (shift) wall[searchwall].pal = getPLU(nPic, &nPlu, (BOOL)(shift & 0x01));
						else wall[searchwall].pal = nPlu = (char)GetNumberBox("Wall palookup", nPlu, nPlu);
						i = 0;
						break;
					case OBJ_CEILING:
						nPic = sector[searchsector].ceilingpicnum;
						nPlu = sector[searchsector].ceilingpal;
						nIdx = searchsector;
						nStat = OBJ_SECTOR;
						if (shift) sector[searchsector].ceilingpal = getPLU(nPic, &nPlu, (BOOL)(shift & 0x01));
						else sector[searchsector].ceilingpal = nPlu = (char)GetNumberBox("Ceiling palookup", nPlu, nPlu);
						if (isSkySector(searchsector, searchstat))
							i = Sky::SetPal(searchsector, searchstat, nPlu, FALSE);
						else
							i = 0;
						break;
					case OBJ_FLOOR:
						nPic = sector[searchsector].floorpicnum;
						nPlu = sector[searchsector].floorpal;
						nIdx = searchsector;
						nStat = OBJ_SECTOR;
						if (shift) sector[searchsector].floorpal = getPLU(nPic, &nPlu, (BOOL)(shift & 0x01));
						else sector[searchsector].floorpal = nPlu = (char)GetNumberBox("Floor palookup", nPlu, nPlu);
						if (isSkySector(searchsector, searchstat))
							i = Sky::SetPal(searchsector, searchstat, nPlu, FALSE);
						else
							i = 0;
						break;
					case OBJ_SPRITE:
						nPic = sprite[searchwall].picnum;
						nPlu = sprite[searchwall].pal;
						nIdx = searchwall;
						if (!shift && sprInHglt(searchwall))
						{
							sprintf(buffer, "Palookup for %d %s(s)", hgltSprCount(), gSearchStatNames[searchstat]);
							if ((i = GetNumberBox(buffer, sprite[searchwall].pal, -1)) >= 0)
							{
								hgltSprCallFunc(sprPalSet, i);
								scrSetMessage(buffer);
								BeepOk();
								i = 1;
								break;
							}
						}
						else if (shift) sprite[searchwall].pal = getPLU(nPic, &nPlu, (BOOL)(shift & 0x01));
						else sprite[searchwall].pal = nPlu = (char)GetNumberBox("Sprite palookup", nPlu, nPlu);
						i = 0;
						break;
				}
				if (i == 0)
				{
					buffer2[0] = '\0';
					i =  (int)isEffectivePLU(nPic, palookup[nPlu]);
					j =  sprintf(buffer, gSearchStatNames[searchstat]);
					if (nPlu > 1)
						sprintf(buffer2, "(%d%% efficiency)", i);
					
					scrSetMessage("%s[%d] palookup: #%d %s", strlwr(buffer), nIdx, nPlu, buffer2);
					Beep(nPlu <= 1 || i);
				}
				break;
			}
			
			switch (searchstat) {
				case OBJ_CEILING:
					sector[searchsector].ceilingstat ^= kSectParallax;
					scrSetMessage("sector[%i] ceiling %s parallaxed", searchsector, isNot(isSkySector(searchsector, searchstat)));
					if (isSkySector(searchsector, searchstat))
					{
						Sky::MakeSimilar(searchsector, searchstat, 0);
					}
					else sector[searchsector].floorstat &= ~kSectShadeFloor;	// clear forced floor shading bit
					BeepOk();
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorstat ^= kSectParallax;
					scrSetMessage("sector[%i] floor %s parallaxed", searchsector, isNot(isSkySector(searchsector, searchstat)));
					if (isSkySector(searchsector, searchstat))
					{
						Sky::MakeSimilar(searchsector, searchstat, 0);
					}
					BeepOk();
					break;
				default:
					BeepFail();
			}
			break;
		}
		case KEY_R:		// R (relative alignment, rotation)
			switch (searchstat) {
				case OBJ_CEILING:
					sector[searchsector].ceilingstat ^= kSectRelAlign;
					scrSetMessage("sector[%i] ceiling %s relative", searchsector, isNot(sector[searchsector].ceilingstat & kSectRelAlign));
					BeepOk();
					break;

				case OBJ_FLOOR:
					sector[searchsector].floorstat ^= kSectRelAlign;
					scrSetMessage("sector[%i] floor %s relative", searchsector, isNot(sector[searchsector].floorstat & kSectRelAlign));
					BeepOk();
					break;
				default:
					BeepFail();
			}
			break;

		case KEY_S:
			i = -1;
			hit2sector(&sect, &x, &y, &z, 0, BLOCK_MOVE | BLOCK_HITSCAN);
			if (sect < 0 || searchstat == OBJ_MASKED)
			{
				BeepFail();
				break;

			}
			else if (alt)
			{
				i = InsertGameObject(searchstat, sect, x, y, z, ang);
			}
			else if ((i = InsertSprite(searchsector, kStatDecoration)) >= 0)
			{
				int picnum = OBJ_SPRITE;
				switch (searchstat) {
					case OBJ_SPRITE:
						switch (sprite[searchwall].cstat & kSprRelMask) {
							case kSprFloor:
							case 48: // sloped
							case kSprWall:
								picnum = OBJ_FLATSPRITE;
								break;
						}
						break;
					case OBJ_WALL:
						picnum = OBJ_FLATSPRITE;
						break;
				}

				if (somethingintab == OBJ_SPRITE)
				{
					sprite[i].picnum = temppicnum;
					sprite[i].shade = tempshade;
					sprite[i].pal = temppal;
					sprite[i].xrepeat = tempxrepeat;
					sprite[i].yrepeat = tempyrepeat;
					sprite[i].xoffset = (char)tempxoffset;
					sprite[i].yoffset = (char)tempyoffset;
					sprite[i].cstat = (short)tempcstat;
					if ((sprite[i].cstat & kSprRelMask) == kSprVoxel)
						spriteSetSlope(i, tempslope);
				}
				else if ((picnum = tilePick(-1, -1, picnum)) >= 0)
				{
					sprite[i].shade  = -8;
					sprite[i].picnum = (short)picnum;
				}
				else
				{
					DeleteSprite(i);
					i = -1;
				}
			}

			updatenumsprites();

			if (i >= 0)
			{
				sprite[i].x = x;
				sprite[i].y = y;
				sprite[i].z = z;

				AutoAdjustSprites();

				switch (searchstat) {
					case OBJ_WALL:
						doWallCorrection(searchwall2, &sprite[i].x, &sprite[i].y);
						sprite[i].ang = (short)((GetWallAngle(searchwall2) + kAng90) & kAngMask);
						if ((sprite[i].cstat & kSprRelMask) != kSprWall)
						{
							sprite[i].cstat &= ~kSprRelMask;
							sprite[i].cstat |= kSprWall;
						}
						sprite[i].cstat |= kSprOneSided;
						break;
					case OBJ_SPRITE:
						sprite[i].x = sprite[searchwall].x;
						sprite[i].y = sprite[searchwall].y;
						ChangeSpriteSect(i, sprite[searchwall].sectnum);
						switch (sprite[searchwall].cstat & 48) {
							case 32:
							case 48:
								clampSprite(&sprite[i]);
								sprite[i].z = sprite[searchwall].z;
								clampSpriteZ(&sprite[i], sprite[searchwall].z, (posz > sprite[searchwall].z) ? 0x01 : 0x02);
								if (((sprite[i].cstat & 48) == 16) && (sprite[i].cstat & kSprOneSided))
									sprite[i].ang = (short)((ang + kAng180) & kAngMask);
								break;
							case 16:
								sprite[i].ang = (short)(sprite[searchwall].ang & kAngMask);
								sprite[i].cstat |= kSprWall;
								break;
						}
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						clampSprite(&sprite[i]);
						doGridCorrection(&sprite[i].x, &sprite[i].y, grid);
						if (((sprite[i].cstat & 48) == 16) && (sprite[i].cstat & kSprOneSided))
							sprite[i].ang = (short)((ang + kAng180) & kAngMask);
						break;
				}

				scrSetMessage("Sprite inserted.");
				BeepOk();
			}
			break;
		case KEY_T:
			switch (searchstat)
			{
				case OBJ_MASKED:
				{
					int level = 0;
					if ( wall[searchwall].cstat & kWallTransluc )
						level = 2;
					if ( wall[searchwall].cstat & kWallTranslucR )
						level = 1;

					level = IncRotate(level, 3);

					switch (level) {
						case 0: wall[searchwall].cstat &= ~kWallTransluc2; break;
						case 1:	wall[searchwall].cstat |= kWallTransluc2; break;
						case 2:
							wall[searchwall].cstat &= ~kWallTransluc2;
							wall[searchwall].cstat |= kWallTransluc;
							break;
					}

					if (wall[searchwall].nextwall >= 0)
					{
						wall[wall[searchwall].nextwall].cstat &= ~kWallTransluc2;
						wall[wall[searchwall].nextwall].cstat |= (short)(wall[searchwall].cstat & kWallTransluc2);
					}

					scrSetMessage("wall[%i] translucent type %d", searchwall, level);
					BeepOk();
					break;
				}

				case OBJ_SPRITE:
				{
					int level = 0;
					if ( sprite[searchwall].cstat & kSprTransluc1 )
						level = 2;
					if ( sprite[searchwall].cstat & kSprTranslucR )
						level = 1;

					level = IncRotate(level, 3);

					switch ( level )
					{
						case 0:
							sprite[searchwall].cstat &= ~kSprTransluc2;
							break;

						case 1:
							sprite[searchwall].cstat |= kSprTransluc2;
							break;

						case 2:
							sprite[searchwall].cstat &= ~kSprTransluc2;
							sprite[searchwall].cstat |= kSprTransluc1;
							break;
					}

					scrSetMessage("sprite[%i] translucent type %d", searchwall, level);
					BeepOk();
					break;
				}

				default:
					BeepFail();
					break;
			}
			break;

		case KEY_U: {
			int vis = 0;
			switch (searchstat) {
				case OBJ_FLOOR:
				case OBJ_CEILING:
					vis = sector[searchsector].visibility;
					break;
				case OBJ_WALL:
				case OBJ_MASKED:
					vis = sector[sectorofwall(searchwall)].visibility;
					break;
				case OBJ_SPRITE:
					vis = sector[sprite[searchwall].sectnum].visibility;
					break;
			}

			if (Confirm("Set visibility for all sectors?") && (vis = GetNumberBox("Visibility", vis, -1)) > -1) {
				vis = (char)ClipRange(vis, 0, 255);
				for (i = 0; i < numsectors; i++) sector[i].visibility = (char)vis;
				scrSetMessage("All sector visibility values set to %d", vis);
				BeepOk();
			}
		}
			break;
		case KEY_V: {

			vel = svel = angvel = 0;
			short opic = -1, npic = -1;
			if (shift && gFavTilesC <= 0)
			{
				Alert("There is no favorite tiles added yet.");
				BeepFail();
				break;
			}

			i = (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING) ? searchsector : searchwall;
			sprintf(buffer2, gSearchStatNames[searchstat]);
			sprintf(buffer, "%s #%d picnum", buffer2, i);

			switch (searchstat) {
				case OBJ_SPRITE:
					opic = sprite[searchwall].picnum;
					if ((sprite[searchwall].cstat & kSprRelMask) != kSprFace) searchstat = OBJ_FLATSPRITE;
					if (shift) npic = sprite[searchwall].picnum = (short)favoriteTileSelect(opic, opic, TRUE, searchstat);
					else npic = sprite[searchwall].picnum = (short)tilePick(opic, opic, searchstat, buffer);
					break;
				case OBJ_WALL:
					opic = wall[searchwall].picnum;
					if (shift) npic = wall[searchwall].picnum = (short)favoriteTileSelect(opic, opic, TRUE, searchstat);
					else npic = wall[searchwall].picnum = (short)tilePick(opic, opic, searchstat, buffer);
					break;
				case OBJ_MASKED:
					opic = wall[searchwall].overpicnum;
					if (shift) npic = wall[searchwall].overpicnum = (short)favoriteTileSelect(opic, opic, TRUE, searchstat);
					else npic = wall[searchwall].overpicnum = (short)tilePick(opic, opic, searchstat, buffer);

					if (wall[searchwall].nextwall >= 0) wall[wall[searchwall].nextwall].overpicnum = npic;
					break;
				case OBJ_CEILING:
				case OBJ_FLOOR:
					if (searchstat == OBJ_FLOOR)
					{
						opic = sector[searchsector].floorpicnum;
						if (shift) npic = (short)favoriteTileSelect(opic, opic, TRUE, searchstat);
						else npic = (short)tilePick(opic, opic, searchstat, buffer);
					}
					else
					{
						opic = sector[searchsector].ceilingpicnum;
						if (shift) npic = (short)favoriteTileSelect(opic, opic, TRUE, searchstat);
						else npic = (short)tilePick(opic, opic, searchstat, buffer);
					}
					
					if (isSkySector(searchsector, searchstat))
						Sky::SetPic(searchsector, searchstat, npic, !gMisc.diffSky);
					else if (searchstat == OBJ_FLOOR)
						sector[searchsector].floorpicnum = npic;
					else
					{
						sector[searchsector].ceilingpicnum = npic;
						sector[searchsector].floorstat &= ~kSectShadeFloor;	// clear forced floor shading bit
					}
					break;
			}

			if (opic == npic) break;
			BeepOk();
			break;
		}
		case KEY_W: {
			// change lighting waveform
			nXSector = GetXSector(searchsector);
			int nWave = xsector[nXSector].shadeWave;
			do {
				nWave = IncRotate(nWave, 12);
			} while ( gWaveNames[nWave] == NULL );
			ProcessHighlightSectors(SetWave, nWave);
			scrSetMessage(gWaveNames[nWave]);
			break;
		}
		case KEY_PADMINUS:
			gStep = (ctrl) ? 128 : 1;
			if (gHgltc > 0 && grshHighlighted(searchstat, (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING) ? searchsector : searchwall) >= 0)
			{
				grshUnhgltObjects(-1, FALSE);
				gResetHighlight = TRUE; // reset highlight to zero on next highlight attempt

				// check if at least one of highlighted objects gets reached min shade
				for (i = 0; i < kMaxHgltObjects; i++)
				{
					if (gHglt[i].shade >= 63)
						break;
				}

				if (i == kMaxHgltObjects)
				{
					// set shade relatively
					for (i = 0; i < kMaxHgltObjects; i++)
					{
						gHglt[i].shade = (schar)ClipHigh(gHglt[i].shade + gStep, 63);
						short idx = gHglt[i].idx;
						switch (gHglt[i].type) {
							case OBJ_WALL:
							case OBJ_MASKED:
								wall[idx].shade = gHglt[i].shade;
								break;
							case OBJ_FLOOR:
								sector[idx].floorshade = gHglt[i].shade;
								break;
							case OBJ_CEILING:
								sector[idx].ceilingshade = gHglt[i].shade;
								break;
						}
					}

					scrSetMessage("Relative shading (shade: +%d)", gStep);
				}
				else
				{
					scrSetMessage("One of objects reached max shade!");
					BeepFail();
					break;
				}
			}
			else if (IsSectorHighlight(searchsector))
			{
				for (i = 0; i < highlightsectorcnt; i++) {
					nSector = highlightsector[i];

					sector[nSector].ceilingshade = (schar)ClipHigh(sector[nSector].ceilingshade + gStep, 64 - 1);
					sector[nSector].floorshade = (schar)ClipHigh(sector[nSector].floorshade + gStep, 64 - 1);
					getSectorWalls(nSector, &startwall, &endwall);
					for (j = startwall; j <= endwall; j++)
						wall[j].shade = (schar)ClipHigh(wall[j].shade + gStep, 64 - 1);
				}
			}
			else
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						wall[searchwall].shade = (schar)ClipHigh(wall[searchwall].shade + gStep, 63);
						scrSetMessage("Shade: %i", wall[searchwall].shade);
						break;
					case OBJ_CEILING:
						sector[searchsector].ceilingshade = (schar)ClipHigh(sector[searchsector].ceilingshade + gStep, 63);
						scrSetMessage("Shade: %i", sector[searchsector].ceilingshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].ceilingshade, alt);
						break;
					case OBJ_FLOOR:
						sector[searchsector].floorshade = (schar)ClipHigh(sector[searchsector].floorshade + gStep, 63);
						scrSetMessage("Shade: %i", sector[searchsector].floorshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].floorshade, alt);
						break;
					case OBJ_SPRITE:
						if (!shift && sprInHglt(searchwall))
						{
							scrSetMessage("%d sprites darker by %d", hgltSprCallFunc(sprShadeIterate, gStep), gStep);
						}
						else
						{
							sprShadeIterate(&sprite[searchwall], gStep);
							scrSetMessage("Shade: %i", sprite[searchwall].shade);
						}
						break;
				}
			}
			BeepOk();
			break;
		case KEY_PADPLUS:
			gStep = (ctrl) ? 128 : 1;
			if (gHgltc > 0 && grshHighlighted(searchstat, (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING) ? searchsector : searchwall) >= 0) {

				grshUnhgltObjects(-1, FALSE);
				gResetHighlight = TRUE; // reset highlight to zero on next highlight attempt

				// check if at least one of highlighted objects gets reached max shade
				for (i = 0; i < kMaxHgltObjects; i++)
				{
					if (gHglt[i].shade < -127)
						break;
				}

				if (i == kMaxHgltObjects)
				{
					// set shade relatively
					for (i = 0; i < kMaxHgltObjects; i++) {

						gHglt[i].shade = (schar)ClipLow(gHglt[i].shade - gStep, -128);
						short idx = gHglt[i].idx;
						switch (gHglt[i].type) {
							case OBJ_WALL:
							case OBJ_MASKED:
								wall[idx].shade = gHglt[i].shade;
								break;
							case OBJ_FLOOR:
								sector[idx].floorshade = gHglt[i].shade;
								break;
							case OBJ_CEILING:
								sector[idx].ceilingshade = gHglt[i].shade;
								break;
						}
					}

					scrSetMessage("Relative brighting (shade: -%d)", gStep);
				}
				else
				{
					scrSetMessage("One of objects reached min shade!");
					BeepFail();
					break;
				}
			}
			else if (IsSectorHighlight(searchsector))
			{
				for( i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];
					sector[nSector].ceilingshade = (schar)ClipLow(sector[nSector].ceilingshade - gStep, -128);
					sector[nSector].floorshade = (schar)ClipLow(sector[nSector].floorshade - gStep, -128);
					getSectorWalls(nSector, &startwall, &endwall);
					for(j=startwall;j<=endwall;j++)
						wall[j].shade = (schar)ClipLow(wall[j].shade - 1, -128);
				}
			}
			else
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						wall[searchwall].shade = (schar)ClipLow(wall[searchwall].shade - gStep, -128);
						scrSetMessage("Shade: %i", wall[searchwall].shade);
						break;
					case OBJ_CEILING:
						sector[searchsector].ceilingshade = (schar)ClipLow(sector[searchsector].ceilingshade - gStep, -128);
						scrSetMessage("Shade: %i", sector[searchsector].ceilingshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].ceilingshade, alt);
						break;
					case OBJ_FLOOR:
						sector[searchsector].floorshade = (schar)ClipLow(sector[searchsector].floorshade - gStep, -128);
						scrSetMessage("Shade: %i", sector[searchsector].floorshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].floorshade, alt);
						break;
					case OBJ_SPRITE:
						if (!shift && sprInHglt(searchwall))
						{
							scrSetMessage("%d sprites brighter by %d", hgltSprCallFunc(sprShadeIterate, -gStep), gStep);
						}
						else
						{
							sprShadeIterate(&sprite[searchwall], -gStep);
							scrSetMessage("Shade: %i", sprite[searchwall].shade);
						}
						break;
				}
			}
			BeepOk();
			break;

		case KEY_PAD0:
			// set the shade of something to 0 brightness
			if ( IsSectorHighlight(searchsector) )
			{
				for( i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];

					sector[nSector].ceilingshade = 0;
					sector[nSector].floorshade = 0;
					getSectorWalls(nSector, &startwall, &endwall);
					for(j=startwall;j<=endwall;j++)
						wall[j].shade = 0;
				}
			}
			else
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED: 
						wall[searchwall].shade = 0; 
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						if (searchstat == OBJ_CEILING)
							sector[searchsector].ceilingshade = 0;
						else
							sector[searchsector].floorshade = 0;
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, 0, alt);
						break;
					case OBJ_SPRITE:
						if (!shift) hgltSprCallFunc(sprShadeSet, 0);
						else sprite[searchwall].shade = 0;
						break;
				}
			}
			scrSetMessage("Shade reset.");
			BeepOk();
			break;
		case KEY_PAD5:	// Reset horiz or panning and repeat
			if (!ctrl)
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						wall[searchwall].xpanning = wall[searchwall].ypanning = 0;
						wall[searchwall].xrepeat = wall[searchwall].yrepeat = 8;
						fixrepeats(searchwall);
						scrSetMessage("wall[%i] pan/repeat reset", searchwall);
						break;
					case OBJ_CEILING:
						sector[searchsector].ceilingxpanning = sector[searchsector].ceilingypanning = 0;
						if (isSkySector(searchsector, searchstat))
						{
							Sky::SetPan(searchsector, OBJ_CEILING, 0, 0, alt);
						}
						else
							scrSetMessage("sector[%i] ceiling pan reset", searchsector);
						break;
					case OBJ_FLOOR:
						sector[searchsector].floorxpanning = sector[searchsector].floorypanning = 0;
						if (isSkySector(searchsector, searchstat))
						{
							Sky::SetPan(searchsector, OBJ_FLOOR, 0, 0, alt);
						}
						else
							scrSetMessage("sector[%i] floor pan reset", searchsector);
						break;
					case OBJ_SPRITE:
						sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
						scrSetMessage("sprite[%i]. pan/repeat reset", searchwall);
						break;
				}
			}
			else
			{
				horiz = 100;
			}
			BeepOk();
			break;
		case KEY_PAD7:
		case KEY_PAD9: {
			changedir = (key == KEY_PAD7) ? 1 : -1;
			BOOL bCoarse = (!shift) ? TRUE : FALSE, fix = FALSE;
			gStep = (bCoarse) ? 8 : 1;
			walltype* curWall = NULL;
			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED:
					curWall = (!ctrl) ? getCorrectWall(searchwall) : &wall[searchwall];
					if (ctrl)
					{
						// fix wrong panning direction when wall x-flipped and/or bottom swapped.
						if ((curWall->nextwall >= 0 && (wall[curWall->nextwall].cstat & kWallSwap)
							&& (wall[curWall->nextwall].cstat & kWallFlipX)) || (curWall->cstat & kWallFlipX))
								changedir = (key == KEY_PAD7) ? -1 : 1;
						
						curWall->xpanning = changechar(curWall->xpanning, changedir, bCoarse, 0);
						curWall->ypanning = changechar(curWall->ypanning, changedir, bCoarse, 0);
						scrSetMessage("wall %i xpanning: %i ypanning: %i", searchwall, curWall->xpanning, curWall->ypanning);
					}
					else
					{
						curWall->yrepeat = changechar(curWall->yrepeat, changedir, bCoarse, 1);
						curWall->xrepeat = changechar(curWall->xrepeat, changedir, bCoarse, 1);
						scrSetMessage("wall %i xrepeat: %i yrepeat: %i", searchwall, curWall->xrepeat, curWall->yrepeat );
					}
					break;
				case OBJ_SPRITE:
					if (alt)
					{
						x = sprite[searchwall].x, y = sprite[searchwall].y;
						gStep = (bCoarse) ? 8 : 1;
						sprite[searchwall].x = (changedir == -1) ? x + gStep : x - gStep;
						sprite[searchwall].y = (changedir == -1) ? y + gStep : y - gStep;
						scrSetMessage("sprite %i xpos: %i ypos: %i", searchwall, sprite[searchwall].x, sprite[searchwall].y);
					}
					else  if (ctrl)
					{
						if ((sprite[searchwall].cstat & 48) == 48)
							break;
						
						sprite[searchwall].xoffset = changechar(sprite[searchwall].xoffset, changedir, bCoarse, 0);
						sprite[searchwall].yoffset = changechar(sprite[searchwall].yoffset, changedir, bCoarse, 0);
						scrSetMessage("sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
					}
					else
					{
						i = sprite[searchwall].xrepeat;
						j = changechar(sprite[searchwall].xrepeat, -changedir, bCoarse, 1);
						if (key == KEY_PAD9 && j < i) sprite[searchwall].xrepeat = 255;
						else if (key == KEY_PAD7 && (j > i || !j)) sprite[searchwall].xrepeat = 1;
						else sprite[searchwall].xrepeat = j;
						
						i = sprite[searchwall].yrepeat;
						j = changechar(sprite[searchwall].yrepeat, -changedir, bCoarse, 1);
						if (key == KEY_PAD9 && j < i) sprite[searchwall].yrepeat = 255;
						else if (key == KEY_PAD7 && (j > i || !j)) sprite[searchwall].yrepeat = 1;
						else sprite[searchwall].yrepeat = j;
						
						scrSetMessage("sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
					}
					break;
				case OBJ_FLOOR:
					if (isSkySector(searchsector, searchstat)) break;
					sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning, changedir, bCoarse, 0);
					sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning, changedir, bCoarse, 0);
					scrSetMessage("sector %i floor xpan: %i ypan: %i", searchsector, sector[searchsector].floorxpanning, sector[searchsector].floorypanning );
					break;
				case OBJ_CEILING:
					if (isSkySector(searchsector, searchstat)) break;
					sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning, changedir, bCoarse, 0);
					sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning, changedir, bCoarse, 0);
					scrSetMessage("sector %i ceil xpan: %i ypan: %i", searchsector,sector[searchsector].ceilingxpanning, sector[searchsector].ceilingypanning);
					break;
			}
			BeepOk();
			break;
		}
		case KEY_PADLEFT:
		case KEY_PADRIGHT: {
			changedir = (key == KEY_PADLEFT) ? 1 : -1;
			BOOL bCoarse = (!shift) ? TRUE : FALSE;
			walltype* curWall = NULL;
			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED:
					curWall = (!ctrl) ? getCorrectWall(searchwall) : &wall[searchwall];
					if (ctrl)
					{
						// fix wrong panning direction when wall x-flipped and/or bottom swapped.
						if ((curWall->nextwall >= 0 && (wall[curWall->nextwall].cstat & kWallSwap)
							&& (wall[curWall->nextwall].cstat & kWallFlipX)) || (curWall->cstat & kWallFlipX))
								changedir = (key == KEY_PADLEFT) ? -1 : 1;
						
						curWall->xpanning = changechar(curWall->xpanning, changedir, bCoarse, 0);
						scrSetMessage("wall %i xpanning: %i ypanning: %i", searchwall, curWall->xpanning, curWall->ypanning);
					}
					else
					{
						curWall->xrepeat = changechar(curWall->xrepeat, changedir, bCoarse, 1);
						scrSetMessage("wall %i xrepeat: %i yrepeat: %i", searchwall, curWall->xrepeat, curWall->yrepeat );
					}
					break;
				case OBJ_SPRITE:
					if (alt) {
						x = sprite[searchwall].x;
						gStep = (bCoarse) ? 8 : 1; sprite[searchwall].x = (changedir == -1) ? x + gStep : x - gStep;
						scrSetMessage("sprite %i xpos: %i ypos: %i", searchwall, sprite[searchwall].x, sprite[searchwall].y);
					} else if (ctrl) {
						if ((sprite[searchwall].cstat & 48) == 48)
							break;
						// fix wrong offset direction when sprite x-flipped.
						if (sprite[searchwall].cstat & kSprFlipX) changedir = (key == KEY_PADLEFT) ? -1 : 1;
						sprite[searchwall].xoffset = changechar(sprite[searchwall].xoffset, changedir, bCoarse, 0);
						scrSetMessage("sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
					} else {
						sprite[searchwall].xrepeat = (BYTE)ClipLow(changechar(sprite[searchwall].xrepeat, -changedir, bCoarse, 1), 4);
						scrSetMessage("sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
					}
					break;
				case OBJ_CEILING:
					sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning, changedir, bCoarse, 0);
					if (isSkySector(searchsector, searchstat))
					{
						// fix panning first
						Sky::FixPan(searchsector, searchstat,  alt);
						Beep(Sky::Rotate(key == KEY_PADRIGHT));
					}
					else
						scrSetMessage("sector %i ceil xpan: %i ypan: %i", searchsector,sector[searchsector].ceilingxpanning, sector[searchsector].ceilingypanning);
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning, changedir, bCoarse, 0);
					if (isSkySector(searchsector, searchstat))
					{
						// fix panning first
						Sky::FixPan(searchsector, searchstat,  alt);
						Beep(Sky::Rotate(key == KEY_PADRIGHT));
					}
					else
						scrSetMessage("sector %i floor xpan: %i ypan: %i", searchsector, sector[searchsector].floorxpanning, sector[searchsector].floorypanning );
					break;
			}
			BeepOk();
			break;
		}
		case KEY_PADUP:
		case KEY_PADDOWN: {
			changedir = (key == KEY_PADUP) ? -1 : 1;
			BOOL bCoarse = (!shift) ? TRUE : FALSE;
			walltype* curWall = NULL;
			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED:
					curWall = (!ctrl) ? getCorrectWall(searchwall) : &wall[searchwall];
					if (ctrl) {
						curWall->ypanning = changechar(curWall->ypanning, changedir, bCoarse, 0);
						scrSetMessage("wall %i xpanning: %i ypanning: %i", searchwall, curWall->xpanning, curWall->ypanning );
					} else {
						curWall->yrepeat = changechar(curWall->yrepeat, changedir, bCoarse, 1);
						scrSetMessage("wall %i xrepeat: %i yrepeat: %i", searchwall, curWall->xrepeat, curWall->yrepeat );
					}
					break;
				case OBJ_SPRITE:
					if (alt) {
						y = sprite[searchwall].y;
						gStep = (bCoarse) ? 8 : 1; sprite[searchwall].y = (changedir == -1) ? y + gStep : y - gStep;
						scrSetMessage("sprite %i xpos: %i ypos: %i", searchwall, sprite[searchwall].x, sprite[searchwall].y);
					} else if (ctrl) {
						if ((sprite[searchwall].cstat & 48) == 48)
							break;
						sprite[searchwall].yoffset = changechar(sprite[searchwall].yoffset, changedir, bCoarse, 0);
						scrSetMessage("sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
					} else {
						sprite[searchwall].yrepeat = (BYTE)ClipLow(changechar(sprite[searchwall].yrepeat, -changedir, bCoarse, 1), 4);
						scrSetMessage("sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
					}
					break;
				case OBJ_CEILING:
					sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning, changedir, bCoarse, 0);
					if (isSkySector(searchsector, searchstat))
					{
						Sky::SetPan(searchsector, searchstat, 0, sector[searchsector].ceilingypanning, alt);
					}
					else
						scrSetMessage("sector %i ceil xpan: %i ypan: %i", searchsector,sector[searchsector].ceilingxpanning, sector[searchsector].ceilingypanning);
					break;
				case OBJ_FLOOR:
					sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning, changedir, bCoarse, 0);
					if (isSkySector(searchsector, searchstat))
					{
						Sky::SetPan(searchsector, searchstat, 0, sector[searchsector].floorypanning, alt);
					}
					else
						scrSetMessage("sector %i floor xpan: %i ypan: %i", searchsector, sector[searchsector].floorxpanning, sector[searchsector].floorypanning );
					break;
			}
			BeepOk();
			break;
		}
		case KEY_PADENTER:
			if (gHgltc > 0) grshUnhgltObjects(-1, TRUE);
			vel = svel = angvel = hvel = doubvel = 0;
			overheadeditor();
			keyClear();
			break;
	 	case KEY_F2:
			// toggle xstructure state
			switch (searchstat)
			{
				case OBJ_WALL:
				case OBJ_MASKED:
					nXWall = wall[searchwall].extra;
					if (nXWall > 0)
					{
						xwall[nXWall].state ^= 1;
						xwall[nXWall].busy = xwall[nXWall].state << 16;
						scrSetMessage(gBoolNames[xwall[nXWall].state]);
						BeepOk();
					}
					else
						BeepFail();
					break;

				case OBJ_CEILING:
				case OBJ_FLOOR:
					nXSector = sector[searchsector].extra;
					if (nXSector > 0)
					{
						xsector[nXSector].state ^= 1;
						xsector[nXSector].busy = xsector[nXSector].state << 16;
						scrSetMessage(gBoolNames[xsector[nXSector].state]);
						BeepOk();
					}
					else
						BeepFail();
					break;

				case OBJ_SPRITE:
					nXSprite = sprite[searchwall].extra;
					if (nXSprite > 0)
					{
						xsprite[nXSprite].state ^= 1;
						xsprite[nXSprite].busy = xsprite[nXSprite].state << 16;
						scrSetMessage(gBoolNames[xsprite[nXSprite].state]);
						BeepOk();
					}
					else
						BeepFail();
					break;
			}
			break;

	 	case KEY_F3:
			nSector = searchsector;
			if ( alt )
			{
				// capture off positions for sector ceiling and floor height
				switch (searchstat)
				{
					case OBJ_WALL:
						if (wall[searchwall].nextsector != -1)
							nSector = wall[searchwall].nextsector;
						// fall through to nXSector processing

					case OBJ_CEILING:
					case OBJ_FLOOR:
						nXSector = sector[nSector].extra;
						if ( nXSector > 0 )
						{
							xsector[nXSector].offFloorZ = sector[nSector].floorz;
							xsector[nXSector].offCeilZ = sector[nSector].ceilingz;
							scrSetMessage("SET offFloorZ= %i  offCeilZ= %i", xsector[nXSector].offFloorZ, xsector[nXSector].offCeilZ );
							BeepOk();
						} else {
							scrSetMessage("Sector type must first be set in 2D mode.");
							BeepFail();
						}
						break;
				}
				break;

			}

			// set sector height to captured off positions
			switch (searchstat)
			{
				case OBJ_WALL:
					if (wall[searchwall].nextsector != -1)
						nSector = wall[searchwall].nextsector;
					// fall through to nXSector processing

				case OBJ_CEILING:
				case OBJ_FLOOR:
					nXSector = sector[nSector].extra;
					if ( nXSector > 0 )
					{
						switch( sector[nSector].type )
						{
							case kSectorZMotion:
							case kSectorRotate:
							case kSectorRotateMarked:
							case kSectorSlide:
							case kSectorSlideMarked:
								sector[nSector].floorz = xsector[nXSector].offFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].offCeilZ;
								scrSetMessage("SET floorz= %i  ceilingz= %i", sector[nSector].floorz, sector[nSector].ceilingz );
								BeepOk();
								break;
						}
					} else {
						scrSetMessage("Sector type must first be set in 2D mode.");
						BeepFail();
					}
					break;
			}
			break;

	 	case KEY_F4:
			nSector = searchsector;
			if ( alt )
			{
				// capture on positions for sector ceiling and floor height
				switch (searchstat)
				{
					case OBJ_WALL:
						if (wall[searchwall].nextsector != -1)
							nSector = wall[searchwall].nextsector;
						// fall through to nXSector processing

					case OBJ_CEILING:
					case OBJ_FLOOR:
						nXSector = sector[nSector].extra;
						if ( nXSector > 0 )
						{
							xsector[nXSector].onFloorZ = sector[nSector].floorz;
							xsector[nXSector].onCeilZ = sector[nSector].ceilingz;
							scrSetMessage("SET onFloorZ= %i  onCeilZ= %i", xsector[nXSector].onFloorZ, xsector[nXSector].onCeilZ );
							BeepOk();
						} else {
							scrSetMessage("Sector type must first be set in 2D mode.");
							BeepFail();
						}
						break;
				}
				break;
			}
			// set sector height to captured on positions
			switch (searchstat)
			{
				case OBJ_WALL:
					if (wall[searchwall].nextsector != -1)
						nSector = wall[searchwall].nextsector;
					// fall through to nXSector processing

				case OBJ_CEILING:
				case OBJ_FLOOR:
					nXSector = sector[nSector].extra;
					if ( nXSector > 0 )
					{
						switch( sector[nSector].type )
						{
							case kSectorZMotion:
							case kSectorRotate:
							case kSectorRotateMarked:
							case kSectorSlide:
							case kSectorSlideMarked:
								sector[nSector].floorz = xsector[nXSector].onFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].onCeilZ;
								scrSetMessage("SET floorz= %i  ceilingz= %i", sector[nSector].floorz, sector[nSector].ceilingz );
								BeepOk();
								break;
						}
					} else {
						scrSetMessage("Sector type must first be set in 2D mode.");
						BeepFail();
					}
					break;
			}
			break;

		case KEY_F9:
			F9Menu();
			break;
		case KEY_F11:
			scrSetMessage("Global panning and slope auto-align is %s", gBoolNames[gMisc.pan^=1]);
			BeepOk();
			break;
		case KEY_2:		// 2 (bottom wall swapping)
			switch (searchstat) {
				case OBJ_WALL:
				case OBJ_MASKED: {
					walltype* pWall = &wall[searchwall]; walltype* pWall2 = NULL;
					if (pWall->nextwall >= 0) {

						pWall2 = getCorrectWall(searchwall);
						if (pWall2->nextwall == searchwall)
							pWall = pWall2;

						pWall->cstat ^= kWallSwap;
						scrSetMessage("wall[%i] bottom swap flag is %s", searchwall, onOff(pWall->cstat & kWallSwap));
						BeepOk();

					} else {

						scrSetMessage("wall[%i] must be red!", searchwall);
						BeepFail();

					}
					break;
				}
				default:
					BeepFail();
			}
			break;
	}
}