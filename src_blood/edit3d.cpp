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
#include "xmpmaped.h"
#include "tile.h"
#include "prefabs.h"
#include "grdshd.h"
#include "hglt.h"
#include "aadjust.h"
#include "preview.h"
#include "xmpexplo.h"
#include "xmpsky.h"
#include "xmpview.h"

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

// lower, upper
BYTE gStackDB[4][2] = {
	{kMarkerLowLink, 	kMarkerUpLink},
	{kMarkerLowWater, 	kMarkerUpWater},
	{kMarkerLowGoo,		kMarkerUpGoo},
	{kMarkerLowStack, 	kMarkerUpStack},
};

char* gTranslucLevNames[3] = {
	
	"not",
	"less",
	"mostly",
	
};



int NextSectorNeighborZ(int nSect, int nBaseZ, char topBot, char dir)
{
	int nNextZ = (dir) ? 0x7fffffff : 0x80000000;
	int t, s, e, nRetnZ = nBaseZ, nNextSect;
	int z[3];
	
	getSectorWalls(nSect, &s, &e);
	while(s <= e)
	{
		if ((nNextSect = wall[s++].nextsector) < 0) continue;
		else if (getSectorHeight(nNextSect) <= 0) continue;
		else if (topBot)
		{
			floorGetEdgeZ(nNextSect, &z[0], &z[2]);
			z[1] = sector[nNextSect].floorz;
		}
		else
		{
			ceilGetEdgeZ(nNextSect, &z[0], &z[2]);
			z[1] = sector[nNextSect].ceilingz;
		}
				
		t = 0;
		while(t < LENGTH(z))
		{
			if (dir)
			{
				if (z[t] > nBaseZ && z[t] < nNextZ)
					nRetnZ = nNextZ = z[t];
			}
			else if (z[t] < nBaseZ && z[t] > nNextZ)
			{
				nRetnZ = nNextZ = z[t];
			}
			
			t++;
		}
	}
	
	return nRetnZ;
}

void SetCeilingZ( int nSector, int z )
{
	int fs = sector[nSector].floorheinum;
	int cs = sector[nSector].ceilingheinum;
	int fz, cz;
	
	// don't allow to go through the floor
	if ((fs >= 0 && cs < 0) || (fs < 0 && cs >= 0) || (((fs < 0 && cs < 0) || (fs > 0 && cs > 0)) && fs != cs))
	{
		sectGetEdgeZ(nSector, &fz, &cz); // get most bottom and top Z
		z = ClipHigh(z, fz + (sector[nSector].ceilingz - cz));
	}
	else
	{
		z = ClipHigh(z, sector[nSector].floorz);
	}
	
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
	int fs = sector[nSector].floorheinum;
	int cs = sector[nSector].ceilingheinum;
	int fz, cz;
	
	// don't allow to go through the ceiling
	if ((fs >= 0 && cs < 0) || (fs < 0 && cs >= 0) || (((fs < 0 && cs < 0) || (fs > 0 && cs > 0)) && fs != cs))
	{
		sectGetEdgeZ(nSector, &fz, &cz); // get most bottom and top Z
		z = ClipLow(z, cz + (sector[nSector].floorz - fz));
	}
	else
	{
		z = ClipLow(z, sector[nSector].ceilingz);
	}
	
	for (int i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
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
	sector[nSector].ceilingslope = (short)nSlope;

	if (sector[nSector].ceilingslope == 0)
		sectCstatRem(nSector, kSectSloped, OBJ_CEILING);
	else
		sectCstatAdd(nSector, kSectSloped, OBJ_CEILING);

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
	sector[nSector].floorslope = (short)nSlope;

	if (sector[nSector].floorslope == 0)
		sectCstatRem(nSector, kSectSloped, OBJ_FLOOR);
	else
		sectCstatAdd(nSector, kSectSloped, OBJ_FLOOR);

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
	int nXSector = sector[nSector].extra;

	if (nXSector > 0)
	{
		XSECTOR *pXSector = &xsector[nXSector];
		pXSector->shadePhase = shadePhase;
	}
}

void SetSectorTheta( int nSector, int bobTheta )
{
	int nXSector = sector[nSector].extra;

	if (nXSector > 0)
	{
		XSECTOR *pXSector = &xsector[nXSector];
		pXSector->bobTheta = bobTheta;
	}
}



static void BuildStairsF( int nSector, int nStepHeight )
{
	int i, j;

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( sectInHglt(j) && !visited[j] )
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

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( sectInHglt(j) && !visited[j] )
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

static spritetype* InsertGameSprite( int nSector, int x, int y, int z, int nAngle, int group ) {

	int i = 0; short picnum = -1;
	if (!adjFillTilesArray(group) || (picnum = (short)tilePick(-1, -1, OBJ_CUSTOM, "Select game object")) < 0)
		return NULL;

	if ((i = adjIdxByTileInfo(picnum, adjCountSkips(picnum))) >= 0)
	{
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

int userItemsCount(VOIDLIST* pList = NULL)
{
	NAMED_TYPE buf; AUTODATA* pData;
	int i, c = 0;
	
	for (i = 0; i < autoDataLength; i++)
	{
		pData = &autoData[i];
		if ((pData->group & kOGrpItemUser) == 0)
			continue;
		
		buf.name = gSpriteNames[pData->type];
		buf.id   = pData->type;
		
		if (pList)
			pList->Add(&buf);
		
		c++;
	}
	
	return c;
}

static int qsSortByName(NAMED_TYPE* ref1, NAMED_TYPE* ref2)
{
	return  stricmp(ref1->name, ref2->name);
}

static spritetype* InsertUserItem(int nSector, int x, int y, int z, int nAngle)
{
	VOIDLIST list(sizeof(NAMED_TYPE));
	spritetype* pSpr; int nSpr, i;
	
	
	NAMED_TYPE* pEntry = NULL;
	if ((i = userItemsCount(&list)) <= 0)
		return NULL;
	
	list.Sort(qsSortByName);
	if ((i = showButtons((NAMED_TYPE*)list.First(), i, "User items")) < mrUser)
			return NULL;
	
	nSpr = InsertSprite(nSector, kStatItem);
	pSpr = &sprite[nSpr];
	
	pSpr->type	= (short) (i - mrUser);
	pSpr->ang	= (short) nAngle;
	
	pSpr->x = x;
	pSpr->y = y;
	pSpr->z = z;
	
	updatenumsprites();
	GetXSprite(nSpr);
	adjSpriteByType(pSpr);
	clampSprite(pSpr);
	return pSpr;
}

static spritetype* InsertModernSpriteType(int nSector, int x, int y, int z, int nAngle)
{
	VOIDLIST list(sizeof(NAMED_TYPE));
	OBJECT* pObj = gModernTypes.Ptr();
	spritetype* pSpr; 
	NAMED_TYPE buf;
	int nSpr, i;
	
	while(pObj->type != OBJ_NONE)
	{
		buf.name = gSpriteNames[pObj->index];
		buf.id   = pObj->index;
		list.Add(&buf);
		pObj++;
	}
	
	if (!list.Length()
		|| (i = showButtons((NAMED_TYPE*)list.First(), list.Length(), "Modern types")) < mrUser)
			return NULL;

	nSpr = InsertSprite(nSector, 0);
	pSpr = &sprite[nSpr];
	
	pSpr->type	= (short) (i - mrUser);
	pSpr->ang	= (short) nAngle;
	
	pSpr->x = x;
	pSpr->y = y;
	pSpr->z = z;
	
	updatenumsprites();
	GetXSprite(nSpr);
	adjSpriteByType(pSpr);

	// set a letter picnum if have nothing in autoData
	if (pSpr->picnum == 0)
	{
		pSpr->xrepeat = pSpr->yrepeat = 128;
		pSpr->picnum = (short)(4096 + (gSpriteNames[pSpr->type][0] - 32));
	}

	clampSprite(pSpr);
	return pSpr;

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
	mrUserItem,
	mrPrefabPut,
	mrPrefabAdd,
};

int InsertGameObject( int where, int nSector, int x, int y, int z, int nAngle) {


	int i, j, t, pfbAng = 0;
	char* filename = NULL;
	int pfbThumb = -1;
	int by = 92;

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
	
	TextButton* bUserItems = new TextButton( 4, 92, 122, 20,  "&User items", mrUserItem);
	dialog.Insert(bUserItems);
	dialog.height+=22;
	by+=22;
	
	if (userItemsCount() <= 0)
	{
		bUserItems->fontColor = kColorDarkGray;
		bUserItems->disabled = 1;
		bUserItems->canFocus = 0;
	}
	
	TextButton* bModernTypes = new TextButton( 4, by, 122, 20,  "Modern &Types", mrModern);
	dialog.Insert(bModernTypes);
	dialog.height+=22;
	by+=22;
	
	if (!gModernMap)
	{
		bModernTypes->fontColor = kColorDarkGray;
		bModernTypes->disabled = 1;
		bModernTypes->canFocus = 0;
	}
	

	dialog.Insert(new Label(6, dialog.height - 8, "PREFABS. . . . . . . . . ."));

	dialog.height+=22;

	dialog.Insert(new TextButton(4,    dialog.height - 20, 60, 20,  "I&nsert",	mrPrefabPut));
	dialog.Insert(new TextButton(66,   dialog.height - 20, 60, 20,  "Sa&ve",	mrPrefabAdd));

	dialog.height+=22;

	ShowModal(&dialog);

	spritetype* pSpr = NULL; nAngle = (nAngle + kAng180) & kAngMask;
	switch (dialog.endState) {
		case mrCancel:
			break;
		case mrEnemy:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpDude);
			break;
		case mrWeapon:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpWeapon | kOGrpAmmoMix);
			break;
		case mrAmmo:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpAmmo | kOGrpAmmoMix);
			break;
		case mrItem:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpItem);
			break;
		case mrHazard:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpHazard);
			break;
		case mrMisc:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpMisc);
			break;
		case mrMarker:
			pSpr = InsertGameSprite(nSector, x, y, z, nAngle, kOGrpMarker);
			break;
		case mrFavesPut:
			if (gFavTilesC <= 0) Alert("There is no favorite tiles yet.");
			else pSpr = favTileInsert(where, nSector, x, y, z, (short) nAngle);
			break;
		case mrModern:
			pSpr = InsertModernSpriteType(nSector, x, y, z, nAngle);
			break;
		case mrUserItem:
			pSpr = InsertUserItem(nSector, x, y, z, nAngle);
			break;
		case mrPrefabPut:
			if ((filename = browseOpenFS(gPaths.prefabs, kPrefabFileExt, "Insert prefab")) != NULL)
			{
				int objIdx;
				switch(where)
				{
					case OBJ_WALL:
					case OBJ_MASKED:
						objIdx = searchwall2;
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						objIdx = searchsector;
						break;
					default:
						objIdx = searchwall;
						break;
				}

				switch (pfbInsert(filename, where, objIdx, nSector, x, y, z, posz))
				{
					case -1: Alert("Must insert in a sector.");		break;
					case -3: Alert("Wrong file version.");			break;
					case -4: Alert("Too many sprites!");			break;
					case -2:
					case  0: // zero sprites was created
						Alert("File \"%s\" is corrupted.", filename);
						break;
					default:
						scrSetMessage("The prefab inserted at x:%d, y:%d, z:%d.", x, y, z);
						return -1;
				}
			}
			break;
		case mrPrefabAdd:
			if ((i = hgltSprCount()) < kMinPrefabSprites)
			{
				Alert("You must highlight at least %d sprites first.", kMinPrefabSprites);
				return -1;
			}
			
			while ( 1 )
			{
				if (pfbDlgOptions(&pfbAng, &pfbThumb) == mrOk)
				{
					if ((filename = browseSave(gPaths.prefabs, kPrefabFileExt)) == NULL)
						continue;
					
					if (fileExists(filename) && !Confirm("Overwrite existing file?"))
						continue;
					
					switch (pfbAdd(filename, pfbAng, pfbThumb))
					{
						case  0:
							Alert("Failed to save prefab. All sprites were wrong!");
							unlink(filename);
							break;
						case -1:
							Alert("Failed to save prefab. Previous file exists!");
							break;
						default:
							scrSetMessage("Prefab saved.");
							break;
					}
				}
				
				if (pfbThumb >= 0)
					tileFreeTile(pfbThumb);
				
				break;
			}
			return -1;
	}

	if (pSpr == NULL)
		return -1;

	switch (pSpr->statnum) {
		case kStatItem:
		case kStatDude:
			pSpr->shade = -8;
			break;
	}

	
	// handle any special setups here
	switch(pSpr->type) {
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
		case kDudeSpiderMother:
		case kDudeBat:
			if (where != OBJ_CEILING || isSkySector(nSector, searchstat)) break;
			else if (pSpr->type == kDudeBat) pSpr->picnum = 1948; // make picnum ceiling bat
			else pSpr->cstat |= kSprFlipY; // invert ceiling spiders
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
			pSpr->cstat |= kSprWall; // auto wall align
			break;
		case kMarkerPath:
			i = GetXSprite(pSpr->index);
			xsprite[i].data1 = ClipLow(findUnusedPath(), 0);
			xsprite[i].data2 = ClipLow(findUnusedPath(), 0);
			break;
		case kSoundAmbient:
			i = GetXSprite(pSpr->index);
			xsprite[i].state = 1;
			break;
		default:
			if (pSpr->type != kMarkerWarpDest && pSpr->type >= kMarkerLowLink && pSpr->type <= kMarkerLowGoo)
			{
				// inserting a ROR links - so create 2nd and define unused stack ID for each
				i = LENGTH(gStackDB);
				
				while(i--)
				{
					if (gStackDB[i][0] != pSpr->type && gStackDB[i][1] != pSpr->type) continue;
					else if ((j = InsertSprite(nSector, 0)) >= 0 && (t = findUnusedStack()) >= 0)
					{
						spritetype* pSpr2 = &sprite[j];
						memcpy(pSpr2, pSpr, sizeof(spritetype));
													
						pSpr2->index = j;
						pSpr2->type  = gStackDB[i][(pSpr->type == gStackDB[i][0])];
						pSpr2->y += 64; // move second one down a bit...
						pSpr2->extra = -1;
						
						xsprite[GetXSprite(j)].data1 = t;
						xsprite[GetXSprite(pSpr->index)].data1 = t;
						
						if ((i = adjIdxByType(pSpr2->type)) >= 0)
						{
							pSpr2->picnum	= autoData[i].picnum;
							pSpr2->xrepeat	= autoData[i].xrepeat;
							pSpr2->yrepeat	= autoData[i].yrepeat;
							pSpr2->pal		= autoData[i].plu;
						}
						
						clampSprite(pSpr2);
						CleanUp();
					}
					
					break;
				}
			}
			break;
	}
	
	return pSpr->index;
}

enum {
	mrStep = mrUser,
	mrShadePhase,
	mrTheta,
};

static void SectorShadePhase( int nSector, int dPhase )
{
	int i, j;
	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( sectInHglt(j) && !visited[j] )
			{
				int nXSector = sector[nSector].extra;
				if (nXSector > 0)
				{
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
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( sectInHglt(j) && !visited[j] )
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
	if (sprInHglt(pSprite->index))
		pSprite->z-=nStep;
	else
		pSprite->z = DecNext(pSprite->z, nStep);
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

void OffsetRoomZ(int nSector, int dz)
{
	int i;
	sectortype* pSect = &sector[nSector];
	pSect->ceilingz += dz;
	pSect->floorz += dz;
	
	if (pSect->extra > 0)
	{
		XSECTOR* pXSect = &xsector[pSect->extra];
		pXSect->onFloorZ += dz;
		pXSect->offFloorZ += dz;
		pXSect->onCeilZ += dz;
		pXSect->offCeilZ += dz;
		
	}
	
	for (i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
	{
		spritetype* pSpr = &sprite[i];
		pSpr->z += dz;
	}
}


void SetWave( int nSector, int nWave )
{
	int nXSector = sector[nSector].extra;
	if ( nXSector > 0 )
		xsector[nXSector].shadeWave = nWave;
}


static void ProcessHighlightSectors( HSECTORFUNC FloorFunc, int nData ) {

	int i = 0;
	if (gListGrd.Length())
	{
		// if only pointing in one of selected objects
		OBJECT* pDb = gListGrd.Ptr();
		while(pDb->type != OBJ_NONE)
		{
			switch(pDb->type)
			{
				case OBJ_FLOOR:
				case OBJ_CEILING:
					if (!sectInHglt(pDb->index))
					{
						if (!i)
						{
							if (pDb->index == searchsector)
							{
								pDb = gListGrd.Ptr(); i = 1;
								continue;
							}
						}
						else
						{
							FloorFunc(pDb->index, nData);
						}
					}
					break;
			}
			
			pDb++;
		}
	}

	if (sectInHglt(searchsector))
	{
		for (i = 0; i < highlightsectorcnt; i++)
			FloorFunc(highlightsector[i], nData);
	}
	else if (i == 0)
	{
		FloorFunc(searchsector, nData);
	}

}

void processMouseLook3D(BOOL readMouse) {
	
	if (readMouse)
		gMouse.Read();
	
	int x1 = windowx1, y1 = windowy1;
	int x2 = windowx2, y2 = windowy2;
	int wh = x2-x1, hg = y2-y1;
	
	searchx = ClipRange(gMouse.X, x1+1, x2-2);
	searchy = ClipRange(gMouse.Y, y1+1, y2-2);
	if (!gMouseLook.mode)
		return;

	// free look
	int dx = abs(gMouse.dX2); int dy = abs(gMouse.dY2);
	searchx = x1+(wh>>1); searchy = y1+(hg>>1);
	gMouse.X = searchx; gMouse.Y = searchy;
	if (ctrl && keystatus[KEY_PAD5])
	{
		horiz = 100;
	}
	else
	{
		if (gMouseLook.dir & 0x1)
		{
			if (gMouse.dY2 < 0) horiz = (gMouseLook.invert & 0x1) ? horiz - dy : horiz + dy;
			else if (gMouse.dY2 > 0) horiz = (gMouseLook.invert & 0x1) ? horiz + dy : horiz - dy;
			horiz = ClipRange(horiz, -gMouseLook.maxSlopeF, gMouseLook.maxSlope);
			
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
		
		if (gMouseLook.dir & 0x2)
		{
			if (gMouse.dX2 < 0) ang = (short)((gMouseLook.invert & 0x2) ? ang + dx : ang - dx);
			else if (gMouse.dX2 > 0) ang = (short)((gMouseLook.invert & 0x2) ? ang - dx : ang + dx);
			ang = (short)(ang & kAngMask);
		}
	}
}

NAMED_TYPE gSpriteTextErrors[] =
{
	{-1, "Sector not found"},
	{-2, "Could not print text with selected font"},
	{-999, NULL},
};

struct TILEFONT
{
	unsigned int offset			: 8;
	unsigned int start			: 16;
	unsigned int end			: 16;
};

TILEFONT gTileFont[] =
{
	{ 65, 	3808, 	3833 },
	{ 32, 	4096, 	4191 },
	{ 32, 	4192, 	4283 },
	{ 65, 	4321, 	4379 },
	{ 32, 	4384, 	4475 },
	{ 32, 	4480, 	4574 },
	{ 48, 	2190, 	2200 },
	{ 48, 	2210, 	2220 },
	{ 48, 	2230, 	2240 },
	{ 48, 	2240, 	2250 },
	{ 48, 	2250, 	2260 },
};

int spriteText(const char* text, TILEFONT* pFont, unsigned char nSize, int nSpace, char nAlign)
{
	spritetype* pSpr; POSOFFS pos(0, 0, 0, 0);
	int nSpr, nSect, nTile, x1, y1, x2, y2;
	int nWidth = 0, i, zt, zb, t;
	int nAng;
	
	short cstat = kSprOneSided | kSprHitscan;
	short hsc = -1, hw = -1, hsp;
	const char* p = text;	
	
	camHitscan(&hsc, &hw, &hsp, &pos.x, &pos.y, &pos.z, 0);
	if (hsc < 0)
		return -1;
	
	nSect = hsc;
	if (hw < 0)
	{
		cstat |= kSprFloor;
		ceilGetEdgeZ(nSect, &zb, &zt);
		
		if (irngok(pos.z, zb, zt))
		{
			cstat |= (kSprFlipY | kSprFlipX);
			pos.a  = nAng = (ang + kAng180) & kAngMask;
		}
		else
		{
			pos.a = (ang + kAng180) & kAngMask;
			nAng  = ang;
		}
	}
	else
	{
		cstat |= kSprWall;
		pos.a  = nAng = (GetWallAngle(hw) + kAng90) & kAngMask;
		pos.Forward(4);
	}
	
	hgltReset(kHgltPoint);
	
	while(*p)
	{
		t = nSpace;
		switch(*p)
		{
			case ' ':
			case '\t':
				t <<= 1;
				break;
			default:
				nTile = pFont->start + *p - pFont->offset;
				if (!rngok(nTile, pFont->start, pFont->end) || !tilesizx[nTile])
					nTile = pFont->start + toupper(*p) - pFont->offset;
				
				if (rngok(nTile, pFont->start, pFont->end) && tilesizx[nTile])
				{
					if ((nSpr = InsertSprite(nSect, 0)) < 0)
						break;
					
					pSpr 				= &sprite[nSpr];
					pSpr->picnum 		= nTile;
					pSpr->cstat 	   |= cstat;
					pSpr->ang			= nAng;
					
					pSpr->xrepeat		= nSize;
					pSpr->yrepeat		= nSize;
					pSpr->shade			= -32;
					
					pSpr->x 			= pos.x;
					pSpr->y 			= pos.y;
					pSpr->z 			= pos.z;
					
					hgltAdd(OBJ_SPRITE, nSpr);
					GetSpriteExtents(pSpr, &x1, &y1, &x2, &y2);
					t += exactDist(x1 - x2, y1 - y2);
				}
				break;
		}
		
		pos.Right(t);
		nWidth+=t;
		p++;
	}
	
	for (i = 0; i < highlightcnt; i++)
	{
		pSpr = &sprite[highlight[i] & 0x3FFF];
		switch(nAlign & 0x03)
		{
			case 0x03:
				offsetPos(-(nWidth>>1), 0, 0, pos.a, &pSpr->x, &pSpr->y, NULL);
				break;
			case 0x01:
				offsetPos(-nWidth, 0, 0, pos.a, &pSpr->x, &pSpr->y, NULL);
				break;
		}
		
		if (FindSector(pSpr->x, pSpr->y, pSpr->z, &nSect))
			ChangeSpriteSect(pSpr->index, nSect);
		
		if((pSpr->cstat & kSprRelMask) >= kSprFloor)
		{
			getzsofslope(pSpr->sectnum, pSpr->x, pSpr->y, &zt, &zb);
			if (pSpr->cstat & kSprFlipY) pSpr->z = zt;
			else pSpr->z = zb;
		}
		else
		{
			clampSprite(pSpr);
		}
	}
	
	return (highlightcnt > 0) ? highlightcnt : -2;
}

char dlgSpriteText()
{
	static TILEFONT userFont = {48, 0, (unsigned int)gMaxTiles};
	static int nFont = 1;
	TILEFONT* pFont;
	
	static int nSize = 64, nSpace = 32;
	char *errMsg, fontName[16], temp[256] = "\0";
	int dw, dx1, dy1, dx2, dy2, dwh, dhg, i, t;
	int nTile, nRetn = mrCancel;
	int x, y;
	
	if (nFont >= 0)
	{
		sprintf(fontName, "[%d]", nFont+1);
		pFont =& gTileFont[nFont];
	}
	else
	{
		sprintf(fontName, "[USER]");
		pFont = &userFont;
	}
	
	nTile = pFont->start+1;
	dw = ClipHigh(630 + (xdim >> 3), xdim - 10);
	scrSave();
	
	while( 1 )
	{
		x = 0, y = 0;
		Window dialog((xdim-dw)>>1, ydim-100, dw, 68, "Sprite text");
		dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
		dialog.getSize(&dwh, &dhg);
		
		EditText* pTextE = new EditText(dx1, dy1, dwh, 20, temp, 0x01);
		sprintf(pTextE->placeholder, "Enter text...");
		pTextE->maxlen = 255;

		Panel* pButtons = new Panel(dx1, dy1+pTextE->height, dwh, 26);
		
		TextButton* pOk = new TextButton(x, 0, 60, 26, "&Confirm", mrOk);
		x+=pOk->width;
		
		TextButton* pFnButton = new TextButton(x, 0, 60, 26, "Font", 106);
		x+=pFnButton->width+4;
		
		Label* pFontNumL = new Label(x, 10, fontName);
		x+=gfxGetTextLen(pFontNumL->string, pFontNumL->font)+16;

		Label* pFontSizeL = new Label(x, 10, "Size:");
		x+=gfxGetTextLen(pFontSizeL->string, pFontSizeL->font)+6;
		EditNumber* pFontSizeE	= new EditNumber(x, 7, 32, 16, nSize, '\0', 1, 255);
		x+=pFontSizeE->width+8;
		
		Label* pCharSpaceL = new Label(x, 10, "Space:");
		x+=gfxGetTextLen(pCharSpaceL->string, pCharSpaceL->font)+6;
		EditNumber* pCharSpaceE	= new EditNumber(x, 7, 32, 16, nSpace, '\0', 0, 255);
		x = dwh-60;
		
		TextButton* pCancel 	= new TextButton(x, 0, 60, 26, "&Quit", mrCancel);
		x-=pCancel->width;
		
		pOk->fontColor 			= kColorBlue;
		pCancel->fontColor 		= kColorRed;
		pFontNumL->fontColor 	= kColorMagenta;
		pFontSizeL->fontColor 	= kColorDarkGray;
		pCharSpaceL->fontColor 	= kColorDarkGray;
		
		pButtons->Insert(pOk);
		pButtons->Insert(pFnButton);
		pButtons->Insert(pFontNumL);
		pButtons->Insert(pFontSizeL);
		pButtons->Insert(pFontSizeE);
		pButtons->Insert(pCharSpaceL);
		pButtons->Insert(pCharSpaceE);
		
		pButtons->Insert(pCancel);
		
		dialog.Insert(pTextE);
		dialog.Insert(pButtons);
		
		ShowModal(&dialog);
		sprintf(temp, pTextE->string);
		nSize = pFontSizeE->value;
		nSpace = pCharSpaceE->value;
		
		nRetn = dialog.endState;
		
		switch(nRetn)
		{
			case mrOk:
				if (!pTextE->string[0]) continue;
				if ((i = spriteText(pTextE->string, pFont, nSize, nSpace, 0x03)) < 0)
				{
					if ((errMsg = retnCodeCheck(i, gSpriteTextErrors)) != NULL)
						Alert(errMsg);
					
					continue;
				}
				break;
			case 106:
				tileIndexCount = 0;
				for (i = 0; i < LENGTH(gTileFont); i++)
				{
					pFont =& gTileFont[i];
					for(t = pFont->start; t < pFont->end; t++)
						tileIndex[tileIndexCount++] = t;
				}
				
				t = 0;
				while(tileIndexCount < gMaxTiles)
					tileIndex[tileIndexCount++] = t++;
				
				if ((nTile = tilePick(nTile, -1, OBJ_CUSTOM, "Select font")) >= 0)
				{
					nFont = LENGTH(gTileFont);
					while(--nFont >= 0)
					{
						pFont =& gTileFont[nFont];
						if (rngok(nTile, pFont->start, pFont->end))
							break;
					}
					
					if (nFont >= 0)
					{
						sprintf(fontName, "[%d]", nFont+1);
					}
					else
					{
						sprintf(fontName, "[USER]");
						userFont.start = nTile;
						pFont = &userFont;
					}
				}
				scrRestore(0);
				continue;
		}
		
		break;
	}
	
	scrRestore(1);
	return (nRetn != mrCancel);
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
			switch (gObjectLock.type)
			{
				case OBJ_FLOOR:
				case OBJ_CEILING:
					searchsector = gObjectLock.idx;
					break;
				default:
					searchwall = gObjectLock.idx;
					break;
			}
			
			searchindex = gObjectLock.idx;
			searchstat = gObjectLock.type;
		}
		else
		{
			gObjectLock.type = gObjectLock.idx = -1;
		}
	}

	processMouseLook3D(TRUE);

	if (!gPreviewMode && searchstat >= 0)
	{
		static short mhold = 0;
		static BOOL rfirst = FALSE;
		short mpress = (short)(~mhold & gMouse.buttons);
		static int ztofs = 0x80000000, zbofs = 0x80000000;
		static int ox = 0, oy = 0, oz = 0;

		if (mhold) searchit = 0;
		else if (gMousePrefs.controls)
		{
			if (mhold) asksave = FALSE;
			else gMouse.velRst = true;
		}

		// highlight objects while *pressing* middle mouse
		if (mpress == 4)
		{
			if (searchstat != OBJ_SPRITE)
			{
				// highlight walls and sectors for gradient shading
				if (Beep(!gListGrd.Exists(searchstat, searchindex)))
				{
					gListGrd.Add(searchstat, searchindex);
				}
				else
				{
					gListGrd.Remove(searchstat, searchindex);
				}
				
				scrSetMessage("%d objects highlighted", gListGrd.Length());
			}
			else
			{
				if (Beep(!sprInHglt(searchwall)))
				{
					hgltReset(kHgltSector);
					hgltAdd(searchstat, searchwall);
				}
				else
				{
					hgltRemove(searchstat, searchwall);
				}
				
				scrSetMessage("%d sprites highlighted", hgltSprCount());
			}
		
		} // drag sprites while holding left mouse
		else if ((mhold & 1) && (searchstat == OBJ_SPRITE && sprite[searchwall].statnum < kStatFree) && (gMousePrefs.controls & 0x0001))
		{
			if (gMouseLook.mode || rfirst)
				gMouse.VelocitySet(ClipLow(gMouse.velX >> 2, 10), ClipLow(gMouse.velY >> 2, 10), false);
			
			gHighSpr = searchwall;
			BOOL inHglt = sprInHglt(gHighSpr);
			spritetype* pSprite = &sprite[gHighSpr];

			if (ztofs == 0x80000000 || zbofs == 0x80000000)
				sprGetZOffsets((short)gHighSpr, &ztofs, &zbofs); // keep z-offsets

			camHitscan(&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
			x = hitx; y = hity; z = hitz;


			// only change z if right mouse button was hold before the left
			if (hitsect >= 0) {

				if (rfirst)
				{
					gMouse.velX = 0;
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

					int nGrid = (gMousePrefs.fixedGrid) ? gMousePrefs.fixedGrid : grid;
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
			gMouse.VelocitySet(-1, -1, true);
			gHighSpr = -1;
		}
		
		if ((mhold == 1 || mhold == 2) && (gMousePrefs.controls & 0x0002))
		{
			gMouse.VelocitySet(10, 10, false);

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
							if ((neigh = NextSectorNeighborZ(searchsector, sector[searchsector].floorz, 1, dir)) >= 0)
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
							if ((neigh = NextSectorNeighborZ(searchsector, sector[searchsector].ceilingz, -1, dir)) >= 0)
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
			gMouseLook.mode = (gMouseLook.mode) ? 0 : 3; //toggle mouse look on and off
			scrSetMessage("Mouse look is %s", onOff(gMouseLook.mode));
			BeepOk();
			break;
		case KEY_INSERT:
			switch (searchstat)
			{
				case OBJ_SPRITE:
					i = ClipLow(hgltSprCallFunc(sprClone), 1);
					scrSetMessage("%d sprite(s) duplicated and stamped.", i);
					if (i > 1)
					{
						spritetype* pSprA = &sprite[searchwall];
						for (i = 0; i < highlightcnt; i++)
						{
							if ((highlight[i] & 0xC000) == 0)
								continue;
							
							j = highlight[i] & 0x3FFF;
							if (sprite[j].index != searchwall)
							{
								spritetype* pSprB = &sprite[j];
								if (pSprA->x == pSprB->x && pSprA->y == pSprB->y && pSprA->z == pSprB->z)
								{
									// give user some time to drag out new sprites
									gObjectLock.type = searchstat;
									gObjectLock.idx  = pSprB->index;
									gObjectLock.time = totalclock + 256;
									gMapedHud.SetMsgImp(256, "Locked on %s #%d", gSearchStatNames[searchstat], pSprB->index);
									break;
								}
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
		case KEY_PAGEDN:
		case KEY_PAGEUP:
			if (searchstat == OBJ_WALL || searchstat == OBJ_MASKED)
			{
				if (wall[searchwall].nextsector != -1)
					TranslateWallToSector();
				
				// still a wall...
				if (searchstat == OBJ_WALL)
				{
					// adjust ceilings when pointing at white walls
					searchindex = searchsector;
					searchstat = OBJ_CEILING;
				}
			}
			
			z = 0x80000000;
			j = (key == KEY_PAGEUP);
			gStep = (shift) ? 0x100 : 0x400;
			

			if (ctrl && alt && (searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR))
			{
				if (j)
					gStep = -gStep;
				
				i = 1;
				if (sectInHglt(searchindex)) i = hgltSectCallFunc(sectChgZ, gStep);
				else sectChgZ(searchindex, gStep);
				
				if (gMisc.pan)
					AlignSlopes();
				
				scrSetMessage("Z-offset %d sectors by %d", i, gStep);
				BeepOk();
				break;
			}
			
			if (ctrl)
				gStep >>= 4;
			
			if (ctrl && !shift)
			{
				switch (searchstat)
				{
					case OBJ_CEILING:
						if (j) z = NextSectorNeighborZ(searchindex, sector[searchindex].ceilingz, 0, 0);
						else z = NextSectorNeighborZ(searchindex, sector[searchindex].ceilingz, 0, 1);
						ProcessHighlightSectors(SetCeilingZ, z);
						break;
					case OBJ_FLOOR:
						if (j) z = NextSectorNeighborZ(searchindex, sector[searchindex].floorz, 1, 0);
						else z = NextSectorNeighborZ(searchindex, sector[searchindex].floorz, 1, 1);
						ProcessHighlightSectors(SetFloorZ, z);
						break;
					case OBJ_SPRITE:
						hgltSprCallFunc((j) ? PutSpriteOnCeiling : PutSpriteOnFloor, 0);
						z = sprite[searchindex].z;
						break;
				}
			}
			else if (alt)
			{
				switch (searchstat)
				{
					case OBJ_CEILING:
					{
						z = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						z = GetNumberBox("height off floor", z, z) * 256;
						z = (sector[searchsector].floorz - z) - sector[searchsector].ceilingz;
						ProcessHighlightSectors(SetCeilingRelative, z);
						z = sector[searchsector].ceilingz;
						break;
					}
					case OBJ_FLOOR:
					{
						z = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						z = GetNumberBox("height off ceiling", z, z) * 256;
						z = (sector[searchsector].ceilingz + z) - sector[searchsector].floorz;
						ProcessHighlightSectors(SetFloorRelative, z);
						z = sector[searchsector].floorz;
						break;
					}
					case OBJ_SPRITE:
						if (sprInHglt(searchindex))
						{
							if (j)
							{
								sprintf(buffer, "%s", gSearchStatNames[OBJ_CEILING]);
								hgltSprPutOnCeiling();
							}
							else
							{
								sprintf(buffer, "%s", gSearchStatNames[OBJ_FLOOR]);
								hgltSprPutOnFloor();
							}
							
							scrSetMessage("%d sprite(s) put on %s keeping the shape", hgltSprCount(), buffer);
							BeepOk();
						}
						break;
				}
			}
			else
			{
				switch(searchstat)
				{
					case OBJ_CEILING:
						ProcessHighlightSectors((j) ? RaiseCeiling : LowerCeiling, gStep);
						z = sector[searchindex].ceilingz;
						break;
					case OBJ_FLOOR:
						ProcessHighlightSectors((j) ? RaiseFloor : LowerFloor, gStep);
						z = sector[searchindex].floorz;
						break;
					case OBJ_SPRITE:
						hgltSprCallFunc((j) ? RaiseSprite : LowerSprite, gStep);
						z = sprite[searchindex].z;
						break;
				}
			}
			
			if (z != 0x80000000)
			{
				scrSetMessage("%s #%d Z: %d", gSearchStatNames[searchstat], searchindex, z);
				if (gMisc.pan)
				{
					if (searchstat == OBJ_CEILING || searchstat == OBJ_FLOOR)
						AlignSlopes();
				}
				
				BeepOk();
			}
			break;
		case KEY_ENTER:
			if (somethingintab == 255)		// must have something to paste
			{
				scrSetMessage("There is nothing in clipboard.");
				BeepFail();
				break;
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
						if ( sectInHglt(searchsector) )
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
						if (sectInHglt(searchsector))
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

					if (somethingintab == searchstat)
					{
						int oCstat = wall[searchwall].cstat;
						wall[searchwall].xrepeat	= tempxrepeat;
						wall[searchwall].yrepeat	= tempyrepeat;
						wall[searchwall].cstat		= tempcstat;
						
						if ((oCstat & kWallSwap) && !(tempcstat & kWallSwap))
							wall[searchwall].cstat |= kWallSwap;
						
						TranslateWallToSector();
						if (sector[searchsector].type > 0)
						{
							if (oCstat & kWallMoveMask)
							{
								wall[searchwall].cstat &= ~(kWallMoveForward | kWallMoveReverse);
								if (oCstat & kWallMoveForward)
									wall[searchwall].cstat |= kWallMoveForward;
								else
									wall[searchwall].cstat |= kWallMoveReverse;
							}
						}
						
						if (sectorofwall(tempidx) < 0 || getWallLength(tempidx) != getWallLength(searchwall))
							fixrepeats(searchwall);
					}
					break;
				case OBJ_CEILING:
					sector[searchsector].ceilingpicnum = temppicnum;
					sector[searchsector].ceilingshade = tempshade;
					sector[searchsector].ceilingpal = temppal;
					switch (somethingintab)
					{
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
					if (sectInHglt(searchsector))
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
					if ( sectInHglt(searchsector) ) {
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
					if (sectInHglt(searchsector)) {
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
					if (sectInHglt(searchsector)) {
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
		case KEY_PLUS:
			gStep = (key == KEY_MINUS) ? 1 : -1 ;
			sprintf(buffer, (gStep < 0) ? "more" : "less");
			
			// iterate sector visibility (higher numbers are less)
			if (ctrl && alt)
			{
				if (sectInHglt(searchsector))
				{
					i = hgltSectCallFunc((HSECTORFUNC2*)sectChgVisibility, gStep);
					scrSetMessage("%d sectors are %s visible", i, buffer);
				}
				else
				{
					sectChgVisibility(searchsector, gStep);
					scrSetMessage("sector[%d] visibility: %i (%s)", searchsector, sector[searchsector].visibility, buffer);
				}
			}
			else if (keystatus[KEY_D])
			{
				gStep = mulscale10((key == KEY_MINUS) ? 16 : -16, numpalookups<<5);
				visibility = ClipRange(visibility + gStep, 0, kMaxVisibility);
				scrSetMessage("Global visibility %d (%s)", visibility, buffer);
			}
			else if (ctrl || shift)
			{
				nXSector = GetXSector(searchsector);
				
				if (ctrl) // iterate lighting effect amplitude
				{
					xsector[nXSector].amplitude  += gStep;
					scrSetMessage("Amplitude: %d", xsector[nXSector].amplitude);
				}
				else // iterate lighting effect phase
				{
					
					xsector[nXSector].shadePhase += gStep;
					scrSetMessage("Phase: %d", xsector[nXSector].shadePhase);
				}
				
				BeepOk();
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
						if ( TestBitString(hgltwall, searchwall) )	// highlighted?
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
			}
			BeepOk();
			break;
		case KEY_E:	// E (expand)
			switch (searchstat)
			{
				case OBJ_FLOOR:
				case OBJ_CEILING:
					sectCstatToggle(searchsector, kSectExpand, searchstat);
					scrSetMessage("%s texture %s expanded", gSearchStatNames[searchstat], isNot(sectCstatGet(searchsector, searchstat) & kSectExpand));
					BeepOk();
					break;
				default:
					BeepFail();
					break;
			}
			break;
		case KEY_F:
			if (alt)
			{
				switch (searchstat)
				{
					case OBJ_WALL:
					case OBJ_MASKED:
						nSector = sectorofwall(searchwall);
						sectCstatRem(searchsector, kSectFlipMask, OBJ_CEILING);
						sectCstatAdd(searchsector, kSectRelAlign, OBJ_CEILING);
						setFirstWall(nSector, searchwall);
						BeepOk();
						break;
					case OBJ_CEILING:
					case OBJ_FLOOR:
						sectCstatRem(searchsector, kSectFlipMask, searchstat);
						sectCstatAdd(searchsector, kSectRelAlign, searchstat);
						setFirstWall(searchsector, sector[searchsector].wallptr + 1);
						BeepOk();
						break;
					default:
						BeepFail();
						break;
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
						switch (i)
						{
							case 0x00: i = 0x10; break;
							case 0x10: i = 0x30; break;
							case 0x30: i = 0x20; break;
							case 0x20: i = 0x04; break;
							case 0x04: i = 0x14; break;
							case 0x14: i = 0x34; break;
							case 0x34: i = 0x24; break;
							case 0x24: i = 0x00; break;
						}
						
						sectCstatRem(searchsector, kSectFlipMask, searchstat);
						sectCstatAdd(searchsector, (BYTE)i, searchstat);
						break;
					case OBJ_SPRITE:
					{
						int zTopOld, zTopNew;
						GetSpriteExtents(&sprite[searchwall], &zTopOld, &i);
						i = sprite[searchwall].cstat;
						
						// two-sided floor sprite?
						if ((i & kSprRelMask) == kSprFloor && !(i & kSprOneSided))
						{
							// what the hell is this supposed to be doing?
							sprite[searchwall].cstat &= ~kSprFlipY;
							sprite[searchwall].cstat ^= kSprFlipX;
						}
						else
						{
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
						GetSpriteExtents(&sprite[searchwall], &zTopNew, &i);
						sprite[searchwall].z += (zTopOld-zTopNew); // compensate Z (useful for wall sprites)
						break;
					}
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
				if (gListGrd.Exists(searchstat, searchindex))
				{
					if (gListGrd.Length() > 1)
					{
						grshShadeWalls(ctrl);
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
			switch (searchstat)
			{
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
					if (ctrl)
					{
						if (sprite[searchwall].z > sector[sprite[searchwall].sectnum].floorz)
						{
							scrSetMessage("%s[%d].z is below floor", gSearchStatNames[searchstat], searchwall);
							BeepFail();
							break;
						}

						if (sprite[searchwall].z < sector[sprite[searchwall].sectnum].ceilingz)
						{
							scrSetMessage("%s[%d].z is above ceiling", gSearchStatNames[searchstat], searchwall);
							BeepFail();
							break;
						}
						
						int nAng = ang;
						if (alt)
							nAng = sprite[searchwall].ang + kAng180;
						
						int nx, ny;
						int hitType = HitScan(&sprite[searchwall], sprite[searchwall].z,Cos(nAng) >> 16, Sin(nAng) >> 16, 0, BLOCK_NONE, 0);
						switch(hitType)
						{
							case OBJ_WALL:
							case OBJ_MASKED:
								GetWallNormal(gHitInfo.hitwall, &nx, &ny);
								sprite[searchwall].x = gHitInfo.hitx + (nx >> 14);
								sprite[searchwall].y = gHitInfo.hity + (ny >> 14);
								sprite[searchwall].z = gHitInfo.hitz;
								ChangeSpriteSect(searchwall, gHitInfo.hitsect);
								sprite[searchwall].ang = (short)((GetWallAngle(gHitInfo.hitwall) + kAng90) & kAngMask);
								scrSetMessage("%s ornamented onto wall %d\n", gSearchStatNames[searchstat], gHitInfo.hitwall);
								BeepOk();
								break;
							default:
								BeepFail();
								break;
						}
					}
					else if (alt)
					{
						sprite[searchwall].cstat ^= kSprOrigin;
						scrSetMessage("%s[%d] origin align is %s", gSearchStatNames[searchstat], searchwall, onOff((sprite[searchwall].cstat & kSprOrigin)));
						BeepOk();
						break;
					}
					break;
			}
			break;
		case KEY_P:
			if (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING)
			{
				if (ctrl && shift)
				{
					if (isSkySector(searchsector, searchstat))
					{
						Sky::Disable(searchsector, searchstat, alt);
						BeepOk();
						break;
					}
					
					BeepFail();
					break;
				}
				
				if (alt)
				{
					sectCstatToggle(searchsector, kSectParallax, searchstat);
					if (isSkySector(searchsector, searchstat)) Sky::MakeSimilar(searchsector, searchstat, 0);
					else if (searchstat == OBJ_CEILING)			sectCstatRem(searchsector, kSectShadeFloor, searchstat);
					scrSetMessage("%s[%d] %s parallaxed", gSearchStatNames[searchstat], searchsector, isNot(isSkySector(searchsector, searchstat)));
					BeepOk();
					break;
				}
			}
			{
				char title[32];
				char nPlu		= getPluOf(searchstat,		searchindex);
				short nPic		= getPicOf(searchstat, 		searchindex);
				short nShade	= getShadeOf(searchstat,	searchindex);
				sprintf(title, "%s #%d palookup", gSearchStatNames[searchstat], searchindex);
				
				if (!shift)
				{
					i = nPlu;
					if (searchstat == OBJ_SPRITE)
						nShade = viewSpriteShade(nShade, nPic, sprite[searchindex].sectnum);
					
					if ((nPlu = pluPick(nPic, nShade, nPlu, title)) == i)
						break;
				}
				else
				{
					nPlu = nextEffectivePlu(nPic, 0, nPlu, (shift & 0x01));
				}
				
				if (searchstat == OBJ_FLOOR || searchstat == OBJ_CEILING)
				{
					if (isSkySector(searchindex, searchstat))
					{
						Sky::SetPal(searchindex, searchstat, nPlu, ctrl);
						break;
					}
				}

				
				i = isEffectivePLU(nPic, palookup[nPlu]);
				buffer[0] = '\0';
				if (nPlu > 1)
					sprintf(buffer, "(%d efficiency)", i);
				
				scrSetMessage("%s: #%d %s", strlwr(title), nPlu, buffer);
				if (searchstat == OBJ_SPRITE && sprInHglt(searchindex) && !shift)
					hgltSprCallFunc(sprPalSet, nPlu);
				else 
					setPluOf(nPlu, searchstat, searchindex);
				
				Beep(nPlu < 2 || i);
			}
			break;
		case KEY_R:		// R (relative alignment, rotation)
			switch (searchstat) {
				case OBJ_CEILING:
				case OBJ_FLOOR:
					if (!isSkySector(searchsector, searchstat))
					{
						i = sectCstatToggle(searchsector, kSectRelAlign, searchstat);
						scrSetMessage("%s[%d] %s relative", gSearchStatNames[searchstat], searchsector, isNot(i & kSectRelAlign));
						BeepOk();
						break;
					}
					// no break
				default:
					BeepFail();
			}
			break;
		case KEY_S:
			if (searchstat == OBJ_SPRITE)
			{
				if ((sprite[searchwall].cstat & kSprRelMask) == kSprFace)
				{
					BeepFail();
					break;
				}
			}
			
			if (camHitscan(&hitsect, &hitwall, &hitsprite, &x, &y, &z, 0) < 0)
			{
				BeepFail();
				break;
			}
			
			i = -1;
			if (!alt)
			{
				int nPic = OBJ_SPRITE;
				if (somethingintab != OBJ_SPRITE)
				{
					switch(searchstat)
					{
						case OBJ_SPRITE:
							switch (sprite[searchwall].cstat & kSprRelMask)
							{
								case kSprFloor:
								case kSprSloped:
								case kSprWall:
									nPic = OBJ_FLATSPRITE;
									break;
							}
							break;
						case OBJ_WALL:
							nPic = OBJ_FLATSPRITE;
							break;
					}
					
					if ((nPic = tilePick(-1, -1, nPic)) < 0)
					{
						BeepFail();
						break;
					}
				}
				
				if ((i = InsertSprite(hitsect, kStatDecoration)) >= 0)
				{
					if (somethingintab == OBJ_SPRITE)
					{
						sprite[i].picnum	= temppicnum;
						sprite[i].shade		= tempshade;
						sprite[i].pal		= temppal;
						sprite[i].xrepeat	= tempxrepeat;
						sprite[i].yrepeat	= tempyrepeat;
						sprite[i].xoffset	= (char)tempxoffset;
						sprite[i].yoffset	= (char)tempyoffset;
						sprite[i].cstat		= (short)tempcstat;
						if ((sprite[i].cstat & kSprRelMask) == kSprSloped)
							spriteSetSlope(i, tempslope);
					}
					else
					{
						sprite[i].picnum = (short)nPic;
						sprite[i].shade  = -8;
					}
				}
			}
			else
			{
				i = InsertGameObject(searchstat, hitsect, x, y, z, ang);
			}
			
			if (Beep(i >= 0))
			{
				sprite[i].x = x;
				sprite[i].y = y;
				sprite[i].z = z;

				AutoAdjustSprites();

				switch (searchstat)
				{
					case OBJ_WALL:
					case OBJ_MASKED:
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
						switch (sprite[searchwall].cstat & kSprRelMask)
						{
							case kSprSloped:
							case kSprFloor:
								sprite[i].z = sprite[searchwall].z;
								clampSpriteZ(&sprite[i], sprite[searchwall].z,
										(posz > sprite[searchwall].z) ? 0x01 : 0x02);
								
								if (((sprite[i].cstat & kSprRelMask) == kSprWall)
									&& (sprite[i].cstat & kSprOneSided))
											sprite[i].ang = (short)((ang + kAng180) & kAngMask);
								break;
							case kSprWall:
								sprite[i].ang = sprite[searchwall].ang;
								sprite[i].cstat |= kSprWall;
								break;
						}
						break;
					case OBJ_FLOOR:
					case OBJ_CEILING:
						clampSprite(&sprite[i]);
						doGridCorrection(&sprite[i].x, &sprite[i].y, grid);
						
						if (((sprite[i].cstat & kSprRelMask) == kSprWall)
							&& (sprite[i].cstat & kSprOneSided))
								sprite[i].ang = (short)((ang + kAng180) & kAngMask);
						break;
				}

				scrSetMessage("Sprite inserted.");
			}
			break;
		case KEY_T:
			buffer[0] = 0;
			switch (searchstat) {
				case OBJ_MASKED:
					i = 0;
					sprintf(buffer, "%d", searchwall);
					if (wall[searchwall].cstat & kWallTransluc)  i = 2;
					if (wall[searchwall].cstat & kWallTranslucR) i = 1;
					switch (i = IncRotate(i, 3)) {
						case 0:
							wallCstatRem(searchwall, kWallTransluc2, !shift);
							break;
						case 1:
							wallCstatAdd(searchwall, kWallTransluc2, !shift);
							break;
						case 2:
							wallCstatRem(searchwall, kWallTransluc2, !shift);
							wallCstatAdd(searchwall, kWallTransluc,  !shift);
							break;
					}
					break;
				case OBJ_SPRITE:
					i = 0;
					sprintf(buffer, "%d", searchwall);
					if (sprite[searchwall].cstat & kSprTransluc1) i = 2;
					if (sprite[searchwall].cstat & kSprTranslucR) i = 1;
					switch (i = IncRotate(i, 3)) {
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
					break;
				case OBJ_FLOOR:
				case OBJ_CEILING:
					sprintf(buffer, "%d", searchsector);
					j = sectCstatGet(searchsector, searchstat);
					switch (j & kSectTranslucR) {
						case 0: // first set it just masked
							sectCstatAdd(searchsector, kSectMasked, searchstat);
							scrSetMessage("%s[%s] is masked", gSearchStatNames[searchstat], buffer);
							buffer[0] = '\0';
							BeepOk();
							break;
						case kSectMasked:
							sectCstatAdd(searchsector, kSectTransluc, searchstat); // less
							i = 1;
							break;
						case kSectTranslucR:
							sectCstatAdd(searchsector, kSectTranslucR, searchstat); // mostly
							sectCstatRem(searchsector, kSectMasked, searchstat);
							i = 2;
							break;
						case kSectTransluc:
							sectCstatRem(searchsector, kSectTranslucR, searchstat); // none
							scrSetMessage("%s[%s] is not masked nor translucent", gSearchStatNames[searchstat], buffer);
							buffer[0] = '\0';
							BeepFail();
							break;
					}
					break;
			}
			if (buffer[0])
			{
				scrSetMessage("%s[%s] %s tanslucent", gSearchStatNames[searchstat], buffer, gTranslucLevNames[i]);
				Beep(i);
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
			sprintf(buffer, "%s #%d picnum", gSearchStatNames[searchstat], i);

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
			if (opic != npic) BeepOk();
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
			if (gListGrd.Exists(searchstat, searchindex))
			{
				OBJECT* pFirst = gListGrd.Ptr();
				OBJECT* pDb = pFirst;
				
				// check if at least one of highlighted objects gets reached min shade
				while(pDb->type != OBJ_NONE)
				{
					if (getShadeOf(pDb->type, pDb->index) >= NUMPALOOKUPS(1))
						break;
					
					pDb++;
				}
				
				if (pDb->type == OBJ_NONE)
				{
					pDb = pFirst;
					
					// set shade relatively
					while(pDb->type != OBJ_NONE)
					{
						iterShadeOf(gStep, pDb->type, pDb->index);
						pDb++;
					}
					
					scrSetMessage("Relative shading (shade: +%d) for %d objects", gStep, gListGrd.Length());
				}
				else
				{
					scrSetMessage("One of objects reached max shade!");
					BeepFail();
					break;
				}
			}
			else if (sectInHglt(searchsector))
			{
				for (i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];
					iterShadeOf(gStep, OBJ_CEILING, nSector);
					iterShadeOf(gStep, OBJ_FLOOR, nSector);
					
					getSectorWalls(nSector, &startwall, &endwall);
					for (j = startwall; j <= endwall; j++)
						iterShadeOf(gStep, OBJ_WALL, j);
				}
			}
			else
			{
				switch (searchstat)
				{
					case OBJ_WALL:
					case OBJ_MASKED:
						iterShadeOf(gStep, searchstat, searchwall);
						scrSetMessage("Shade: %i", wall[searchwall].shade);
						break;
					case OBJ_CEILING:
						iterShadeOf(gStep, searchstat, searchsector);
						scrSetMessage("Shade: %i", sector[searchsector].ceilingshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].ceilingshade, alt);
						break;
					case OBJ_FLOOR:
						iterShadeOf(gStep, searchstat, searchsector);
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
			gStep = (ctrl) ? -128 : -1;
			if (gListGrd.Exists(searchstat, searchindex))
			{
				OBJECT* pFirst = gListGrd.Ptr();
				OBJECT* pDb = pFirst;
				
				// check if at least one of highlighted objects gets reached max shade
				while(pDb->type != OBJ_NONE)
				{
					if (getShadeOf(pDb->type, pDb->index) <= -128)
						break;
					
					pDb++;
				}
				
				if (pDb->type == OBJ_NONE)
				{
					pDb = pFirst;
					
					// set shade relatively
					while(pDb->type != OBJ_NONE)
					{
						iterShadeOf(gStep, pDb->type, pDb->index);
						pDb++;
					}
					
					scrSetMessage("Relative brighting (shade: -%d) for %d objects", gStep, gListGrd.Length());
				}
				else
				{
					scrSetMessage("One of objects reached min shade!");
					BeepFail();
					break;
				}
			}
			else if (sectInHglt(searchsector))
			{
				for( i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];
					iterShadeOf(gStep, OBJ_CEILING, nSector);
					iterShadeOf(gStep, OBJ_FLOOR, nSector);
					
					getSectorWalls(nSector, &startwall, &endwall);
					for (j = startwall; j <= endwall; j++)
						iterShadeOf(gStep, OBJ_WALL, j);
				}
			}
			else
			{
				switch (searchstat) {
					case OBJ_WALL:
					case OBJ_MASKED:
						iterShadeOf(gStep, searchstat, searchwall);
						scrSetMessage("Shade: %i", wall[searchwall].shade);
						break;
					case OBJ_CEILING:
						iterShadeOf(gStep, searchstat, searchsector);
						scrSetMessage("Shade: %i", sector[searchsector].ceilingshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].ceilingshade, alt);
						break;
					case OBJ_FLOOR:
						iterShadeOf(gStep, searchstat, searchsector);
						scrSetMessage("Shade: %i", sector[searchsector].floorshade);
						if (isSkySector(searchsector, searchstat))
							Sky::SetShade(searchsector, searchstat, sector[searchsector].floorshade, alt);
						break;
					case OBJ_SPRITE:
						if (!shift && sprInHglt(searchwall))
						{
							scrSetMessage("%d sprites brighter by %d", hgltSprCallFunc(sprShadeIterate, gStep), gStep);
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

		case KEY_PAD0:
			// set the shade of something to 0 brightness
			if ( sectInHglt(searchsector) )
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
	 	case KEY_F2:
			if (alt)
			{
				// reverse door position
				switch(searchstat)
				{
					case OBJ_SPRITE:
						BeepFail();
						break;
					case OBJ_WALL:
					case OBJ_MASKED:
						TranslateWallToSector();
						// no break
					case OBJ_FLOOR:
					case OBJ_CEILING:
						if ((i = reverseSectorPosition(searchsector)) < 0)
						{
							char* errMsg = retnCodeCheck(i, gReverseSectorErrors);
							if (errMsg)
								Alert(errMsg);
							
							BeepFail();
						}
						BeepOk();
						break;
					
				}
				break;
			}
			
			// toggle xstructure state
			switch (searchstat)
			{
				case OBJ_WALL:
				case OBJ_MASKED:
					nXWall = wall[searchwall].extra;
					if (Beep(nXWall > 0))
					{
						xwall[nXWall].state ^= 1;
						xwall[nXWall].busy = xwall[nXWall].state << 16;
						scrSetMessage(gBoolNames[xwall[nXWall].state]);
					}
					break;
				case OBJ_CEILING:
				case OBJ_FLOOR:
					nXSector = sector[searchsector].extra;
					if (Beep(nXSector > 0))
					{
						xsector[nXSector].state ^= 1;
						xsector[nXSector].busy = xsector[nXSector].state << 16;
						scrSetMessage(gBoolNames[xsector[nXSector].state]);
					}
					break;
				case OBJ_SPRITE:
					nXSprite = sprite[searchwall].extra;
					if (Beep(nXSprite > 0))
					{
						xsprite[nXSprite].state ^= 1;
						xsprite[nXSprite].busy = xsprite[nXSprite].state << 16;
						scrSetMessage(gBoolNames[xsprite[nXSprite].state]);
					}
					break;
			}
			break;
	 	case KEY_F3:
		case KEY_F4:
			switch (searchstat) {
				case OBJ_WALL:
					TranslateWallToSector();
					// no break
				case OBJ_CEILING:
				case OBJ_FLOOR:
					nSector = searchsector;
					switch (sector[nSector].type) {
						default:
							scrSetMessage("Sector type #%d does not support z-motion!", sector[nSector].type);
							BeepFail();
							break;
						case kSectorZMotionSprite:
							if (!alt)
							{
								scrSetMessage("Preview unavailable for %s!", gSectorNames[sector[nSector].type]);
								BeepFail();
								break; // you can't preview because of sprites...
							}
						case kSectorZMotion:
						case kSectorRotate:
						case kSectorRotateMarked:
						case kSectorSlide:
						case kSectorSlideMarked:
							nXSector = GetXSector(nSector);
							i = sprintf(buffer, (alt) ? "Capture" : "Show");
							if (key == KEY_F3)
							{
								if (alt)
								{
									xsector[nXSector].offFloorZ = sector[nSector].floorz;
									xsector[nXSector].offCeilZ  = sector[nSector].ceilingz;
								}
								
								sector[nSector].floorz   = xsector[nXSector].offFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].offCeilZ;
							}
							else
							{
								if (alt)
								{
									xsector[nXSector].onFloorZ = sector[nSector].floorz;
									xsector[nXSector].onCeilZ  = sector[nSector].ceilingz;
								}
								
								sector[nSector].floorz   = xsector[nXSector].onFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].onCeilZ;
							}
							
							i += sprintf(&buffer[i], " %s", onOff(key == KEY_F4));
							scrSetMessage("%s floorZ=%d, ceilingZ=%d", strupr(buffer), sector[nSector].floorz, sector[nSector].ceilingz);
							BeepOk();
							break;
					}
					break;
				default:
					BeepFail();
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
			if (Beep((searchstat == OBJ_WALL || searchstat == OBJ_MASKED) && wall[searchwall].nextwall >= 0))
			{
				walltype* pWall  = &wall[searchwall];
				walltype* pWall2 = getCorrectWall(searchwall);
				if (pWall2->nextwall == searchwall)
					pWall = pWall2;
				
				pWall->cstat ^= kWallSwap;
				scrSetMessage("wall[%d] bottom swap flag is %s", searchwall, onOff(pWall->cstat & kWallSwap));
			}
			break;
	}
}