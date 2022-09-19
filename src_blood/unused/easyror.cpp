enum {
kLinkNeutral		= 0,
kLinkWater			= 1,
kLinkGoo			= 2,
kLinkStack			= 3,
kLinkMax			   ,
};

void sectGetEdgeWalls(int nSector, short* lw, short* rw, short* tw, short* bw)
{
	register int s, e;
	getSectorWalls(nSector, &s, &e);
	
	*lw = *rw = *tw = *bw = s;
	while(s <= e)
	{
		if (wall[s].x < wall[*lw].x) *lw = s;
		if (wall[s].x > wall[*rw].x) *rw = s;
		if (wall[s].y < wall[*tw].y) *tw = s;
		if (wall[s].y > wall[*bw].y) *bw = s;
		
		s++;
	}
}


// group, lower, upper
NAMED_TYPE gStackDB[kLinkMax][3] = {
	
	{ {kLinkNeutral, 	"Neutral"},	{kMarkerLowLink, 	gSpriteNames[kMarkerLowLink]},		{kMarkerUpLink, 	gSpriteNames[kMarkerUpLink]}, },
	{ {kLinkWater, 		"Water"},	{kMarkerLowWater, 	gSpriteNames[kMarkerLowWater]},		{kMarkerUpWater, 	gSpriteNames[kMarkerUpWater]}, },
	{ {kLinkGoo, 		"Goo"},		{kMarkerLowGoo, 	gSpriteNames[kMarkerLowGoo]},		{kMarkerUpGoo, 		gSpriteNames[kMarkerUpGoo]}, },
	{ {kLinkStack, 		"Stack"},	{kMarkerLowStack, 	gSpriteNames[kMarkerLowStack]},		{kMarkerUpStack, 	gSpriteNames[kMarkerUpStack]}, },

};

NAMED_TYPE gEasyRorErrors[] = {
	
	{-1, "Out of sprites"},
	{-2, "Marker is not inside of it's sector"},
	{-3, "Sector of marker is wrong"},
	{-10, "Second room is wrong or first room is not defined"},
	{-20, "First and second rooms are same"},
	{-30, "Failed to get new room ID"},
	{-999, NULL},
	
};

class XMPROR
{
	private:
	//----------------------------------------------
	signed int indexA			: 16;
	signed int typeA			: 16;
	unsigned int linkGroup		: 2;
	unsigned int seeThrough		: 1;
	public:
	//----------------------------------------------
	XMPROR() { Reset(); }
	int RoomGetID() 	{ return indexA; }
	int RoomGetType() 	{ return typeA;  }
	BOOL RoomPrepared()
	{
		switch (typeA) {
			case OBJ_FLOOR:
			case OBJ_CEILING:
				return (indexA < numsectors);
			case OBJ_WALL:
			case OBJ_MASKED:
				return (indexA < numwalls);	
		}
		
		return FALSE;
	}
	
	void RoomPrepare(int nType, int nIdx)
	{
		indexA = nIdx;
		typeA  = nType;
	}
	
	int RoomConnected(int nType, int nIndex, int* nMarkA, int* nMarkB)
	{
		register int i, j, k, n, low1, low2;
		spritetype *pMark1, *pMark2;
		*nMarkA = *nMarkB = -1;
		
		switch(nType) {
			case OBJ_FLOOR:
			case OBJ_CEILING:
				for (i = headspritesect[nIndex]; i >= 0; i = nextspritesect[i])
				{
					pMark1 =& sprite[i];
					if (pMark1->extra <= 0 || (low1 = LinkIsLower(pMark1->type)) < 0)
						continue;
					
					n = getDataOf(0, OBJ_SPRITE, i);
					for (j = 0; j < numsectors; j++)
					{
						if (j == nIndex)
							continue;
						
						for (k = headspritesect[j]; k >= 0; k = nextspritesect[k])
						{
							pMark2 =& sprite[k];
							if (pMark2->extra <= 0) continue;
							else if ((low2 = LinkIsLower(pMark2->type)) < 0 || low1 == low2) continue;
							else if (getDataOf(0, OBJ_SPRITE, k) == n)
							{
								*nMarkA = pMark1->index;
								*nMarkB = pMark2->index;
								return n;
							}
						}
					}
				}
				break;
		}
		
		return -1;
	}
	
	int RoomSetup(int nMarker, int nLinkType, int nRoomID)
	{
		int low, group; bool water, neutral;
		spritetype* pMark = &sprite[nMarker];
		if ((low = LinkIsLower(nLinkType)) < 0 || (group = LinkGetGroup(nLinkType)) < 0)
			return -1;
		
		pMark->type = nLinkType;
		adjSpriteByType(pMark);
		
		sectortype* pSect =& sector[pMark->sectnum];
		water = (group != kLinkNeutral && group != kLinkStack);
		neutral = (group == kLinkNeutral);
				
		if (low == 1)
		{
			PutSpriteOnCeiling(pMark, 0);
			
			if (!neutral)
			{
				if (water)
					xsector[GetXSector(pMark->sectnum)].underwater = 1;
				else if (pSect->extra > 0)
					xsector[pSect->extra].underwater = 0;
			}
			
			if (seeThrough) pSect->ceilingpicnum = 504;
			else if (pSect->ceilingpicnum == 504)
				pSect->ceilingpicnum = 0;
		}
		else
		{
			PutSpriteOnFloor(pMark, 0);
			
			if (!neutral && pSect->extra > 0)
				xsector[pSect->extra].underwater = 0;
			
			if (seeThrough) pSect->floorpicnum = 504;
			else if (pSect->floorpicnum == 504)
				pSect->floorpicnum = 0;
		}
		
		XSPRITE* pXMark = &xsprite[GetXSprite(nMarker)];
		pXMark->data1 = nRoomID;
		return 0;
	}
	
	void Reset()
	{
		indexA		= -1;
		typeA  		= -1;
		seeThrough	= 0;
		linkGroup 	= kLinkNeutral;
	}
	
	int LinkGetGroup(int nLinkType)
	{
		int i, j;
		NAMED_TYPE* pRow;
		
		i = LENGTH(gStackDB); 
		while(i--)
		{
			j = 3;
			pRow = gStackDB[i];
			while(--j)
			{
				if (pRow[j].id == nLinkType)
					return pRow[0].id;
			}
		}
		
		return -1;
	}
	
	int LinkGetType(int objType, int nGroup)
	{
		int i;
		NAMED_TYPE* pRow;
		i = LENGTH(gStackDB);
		
		while(i--)
		{
			pRow = gStackDB[i];
			if (pRow[0].id == nGroup)
				return pRow[1 + (objType == OBJ_FLOOR)].id;
		}
		
		return -1;
	}

	int LinkIsLower(int nLinkType)
	{
		int i;
		NAMED_TYPE* pRow;
		i = LENGTH(gStackDB); 
		while(i--)
		{
			pRow = gStackDB[i];
			if (pRow[1].id == nLinkType) return 1;
			else if (pRow[2].id == nLinkType)
				return 0;
		}
		
		return -1;
	}

	int CreateMarker(int nSector)
	{
		spritetype* pMark;
		
		short lw, rw, tw, bw;
		int retn, x1, y1, x2, y2;
		int nMark = InsertSprite(nSector, 0);		
		if (nMark < 0)
			return -1;
		
		sectGetEdgeWalls(nSector, &lw, &rw, &tw, &bw);
		
		avePointWall(lw, &x1, &y1);
		doWallCorrection(lw, &x1, &y1);

		pMark = &sprite[nMark];
		pMark->x = x1; pMark->y = y1;
		ChangeSpriteSect(nMark, nSector);
		
		if (inside(pMark->x, pMark->y, nSector) <= 0) retn = -2;
		else if (pMark->sectnum != nSector) retn = -3;
		else return nMark;
		
		DeleteSprite(nMark);
		return retn;
	}

	void DisconnectRooms(int nType, int nIndex)
	{
		int nMark1, nMark2;
		switch (nType) {
			case OBJ_FLOOR:
			case OBJ_CEILING:
				while(RoomConnected(nType, nIndex, &nMark1, &nMark2) >= 0)
				{
					if (nMark1 >= 0) DeleteSprite(nMark1);
					if (nMark2 >= 0) DeleteSprite(nMark2);
				}
				break;
		}
	}
	
	int ConnectRooms(int typeB, int indexB)
	{
		bool aNew = true, bNew = true;
		int linkType, roomID = -1, retn = 0, a, b;
		if (typeB < 0 || indexB < 0 || !RoomPrepared())
			return -10;

		switch (typeA) {
			case OBJ_FLOOR:
			case OBJ_CEILING:
				if (typeB == typeA || (typeB != OBJ_FLOOR && typeB != OBJ_CEILING))
					return -20;
				
				roomID = RoomConnected(typeA, indexA, &a, &b);
				if (roomID < 0 && (roomID = findUnusedStack()) < 0)
					return -30;
				
				aNew = (a < 0);
				if (!aNew || (a = CreateMarker(indexA)) >= 0)
				{
					RoomSetup(a, LinkGetType(typeA, linkGroup), roomID);
					
					bNew = (b < 0);
					if (!bNew || (b = CreateMarker(indexB)) >= 0)
						RoomSetup(b, LinkGetType(typeB, linkGroup), roomID);
					else
						retn = b;
				}
				else
				{
					retn = a;
				}
				
				break;
			default:
				retn = -40;
				break;
		}
		
		return retn;
	}
	
	BOOL Dialog()
	{
		register int i = 0;
		NAMED_TYPE linkTypes[kLinkMax];
		while(i < LENGTH(linkTypes))
		{
			linkTypes[i].id		= gStackDB[i][0].id;
			linkTypes[i].name	= gStackDB[i][0].name;
			i++;
		}

		while( 1 )
		{
			if ((i = showButtons(linkTypes, LENGTH(linkTypes), "Select a ROR style...") - mrUser) < 0) return FALSE;
			else linkGroup = i;
			
			if ((i = YesNoCancel("Make it see-through?")) == mrCancel) continue;
			else seeThrough = i;
			
			break;
		}
		
		return TRUE;
	}
}
gEasyRor;

void easyRorToolHelper(XMPROR* pRor)
{
	char* errMsg;
	int bt, bi, at, ai, retn;
	bool in2d = (qsetmode != 200);
	getHighlightedObject();
	
	#define kMsgTime 128
	
	bi = searchsector;
	switch (searchstat) {
		default:
			if (!in2d || (bi = getSector()) < 0)
			{
				gMapedHud.SetMsgImp(kMsgTime, "Must point in floor or ceiling.");
				BeepFail();
				break;
			}
			searchstat = OBJ_FLOOR;
			// no break
		case OBJ_FLOOR:
		case OBJ_CEILING:
			if (!pRor->RoomPrepared())
			{
				pRor->RoomPrepare(searchstat, bi);
				gMapedHud.SetMsgImp(kMsgTime, "Select second sector to connect this one with.");
				BeepOk();
				break;
			}
			else if (pRor->RoomGetID() == bi)
			{
				gMapedHud.SetMsgImp(kMsgTime, "Easy ROR reset.");
				pRor->Reset();
				BeepFail();
				break;
			}
			
			bt = searchstat;
			if (pRor->RoomGetType() == bt)
				bt = (bt == OBJ_FLOOR) ? OBJ_CEILING : OBJ_FLOOR;
			
			at = pRor->RoomGetType(); ai = pRor->RoomGetID();
			if (Confirm("Connect %s #%d with %s #%d?", gSearchStatNames[at], ai, gSearchStatNames[bt], bi) && pRor->Dialog())
			{
				if ((retn = pRor->ConnectRooms(bt, bi)) < 0)
				{
					errMsg = retnCodeCheck(retn, gEasyRorErrors);
					Alert("Error #%d: %s", abs(retn), errMsg);
					BeepFail();
				}
				else
				{
					gMapedHud.SetMsgImp(kMsgTime, "Sectors connected.");
					BeepOk();
				}
			}
			
			pRor->Reset();
			CleanUpMisc();
	}
	
	return;
}