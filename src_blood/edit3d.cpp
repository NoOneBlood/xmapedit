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
	else if (hitsect >= 0)
    {
        if (dz > 0)	// hit floor
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
}

void SetupLightBomb( void )
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

			int length = ClipLow(ksqrt(Nx * Nx + Ny * Ny), 1);

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


void LightBomb( int x, int y, int z, short nSector )
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
		if (pObj->type == OBJ_SPRITE)
        {
            buf.name = gSpriteNames[pObj->index];
            buf.id   = pObj->index;
            list.Add(&buf);
		}
        
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
    mrClean,
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

BOOL F9Menu( void )
{
    IDLIST list(1); XSECTOR model;
    int const nDefault = 9999999;
    int nValue, nTrick, i;
    int32_t* p;
    
    BOOL bChanged = FALSE;
    
    NAMED_TYPE tricks[] =
    {
        { mrStep,       "&Step Height" },
        { mrShadePhase, "&Light Phase" },
        { mrTheta,      "&Z Phase" },
        { mrClean,      "&Clean matching" },
    };
    
    memset(visited, FALSE, sizeof(visited));
    
    while( 1 )
    {
        switch(nTrick = showButtons(tricks, LENGTH(tricks), "Sector Tricks") - mrUser)
        {
            case mrStep:
            case mrShadePhase:
            case mrTheta:
                if ((nValue = GetNumberBox("Value", 0, nDefault)) != nDefault)
                {
                    switch(nTrick)
                    {
                        case mrStep:
                            switch (searchstat)
                            {
                                case OBJ_FLOOR:
                                    BuildStairsF(searchsector, nValue);
                                    bChanged = TRUE;
                                    break;
                                case OBJ_CEILING:
                                    BuildStairsC(searchsector, nValue);
                                    bChanged = TRUE;
                                    break;
                            }
                            break;
                        case mrShadePhase:
                            i = highlightsectorcnt;
                            while(--i >= 0) GetXSector(highlightsector[i]);

                            SectorShadePhase(searchsector, nValue);
                            bChanged = TRUE;
                            break;
                        case mrTheta:
                            i = highlightsectorcnt;
                            while(--i >= 0) GetXSector(highlightsector[i]);

                            SectorTheta(searchsector, nValue);
                            bChanged = TRUE;
                            break;
                    }
                    
                    break;
                }
                continue;
            case mrClean:
                if (sector[searchsector].extra > 0)
                {
                    memcpy(&model, &xsector[sector[searchsector].extra], sizeof(model));
                    list.Clear();
                    
                    i = numsectors;
                    while(--i >= 0)
                    {
                        if (sector[i].extra > 0)
                        {
                            model.reference = xsector[sector[i].extra].reference;
                            if (memcmp(&model, &xsector[sector[i].extra], sizeof(model)) == 0)
                                list.Add(sector[i].extra);
                        }
                    }
                    
                    if ((bChanged = Confirm("Clean %d x-sectors?", list.Length())) == TRUE)
                    {
                        for (p = list.First(); *p >= 0; p++) dbDeleteXSector(*p);
                        CleanUp();
                    }
                    
                    break;
                }
                Alert("Sector #%d is not x-sector!", searchsector);
                break;
        }
        
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


void ProcessHighlightSectors( HSECTORFUNC FloorFunc, int nData )
{

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

void processMouseLook3D()
{	
	int x1 = windowx1, y1 = windowy1;
	int x2 = windowx2, y2 = windowy2;
	int wh = x2-x1, hg = y2-y1;
	int dx, dy, sh;
	
	
	gMouse.Read();
	searchx = ClipRange(gMouse.X, x1, x2);
	searchy = ClipRange(gMouse.Y, y1, y2);
	if (!gMouseLook.mode)
		return;

	// free look
	searchx = x1+(wh>>1); searchy = y1+(hg>>1);
	gMouse.X = searchx; gMouse.Y = searchy;
	
	if (ctrl && keystatus[KEY_PAD5])
	{
		horiz = 100;
		return;
	}
	
	if (gMouseLook.dir & 0x1)
	{
		if (gMouse.dY2)
		{
			dy = gMouse.dY2;
			horiz = (gMouseLook.invert & 0x1) ? horiz + dy : horiz - dy;
			horiz = ClipRange(horiz, -gMouseLook.maxSlopeF, gMouseLook.maxSlope);
		}
		
		if (zmode == 3)
		{
			sh = (shift) ? 3 : 2;
			if (keystatus[KEY_UP])			posz += (((100 - horiz) * gFrameTicks) << sh);
			else if (keystatus[KEY_DOWN])	posz += (((horiz - 100) * gFrameTicks) << sh);
		}
	}
	
	if (gMouse.dX2 && (gMouseLook.dir & 0x2))
	{
		dx = gMouse.dX2;
		ang = ((gMouseLook.invert & 0x2) ? ang - dx : ang + dx);
		ang = (ang & kAngMask);
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



void ProcessInput3D( void )
{
	INPUTPROC* pInput = &gEditInput3D[key];
	short hitsect, hitwall, hitsprite;
	int x, y, z, r, hitx, hity, hitz;
	
	searchit = 2;

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

	processMouseLook3D();

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
			asksave = 1;
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

	if (pInput->pFunc)
	{
		r = pInput->pFunc(key, ctrl, shift, alt);
		
		if (r & PROC_OK)
		{
			switch(r & PROC_UNDO_MASK)
			{
				case PROC_UNDO_CMP: asksave = 1; break;
				case PROC_UNDO_ADD: asksave = 2; break;
			}
		}
		
		if (r & PROC_BEEP)
			Beep(r & PROC_OK);
        
        keyClear();
	}
}