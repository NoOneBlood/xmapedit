		case KEY_3:
			if (!in2d) return FALSE;
			i = makeSquareSector(sectorhighlight, mousxplc, mousyplc, 4);
			getSectorWalls(i, &swal, &ewal); hgltReset();
			while(swal <= ewal)
			{
				//if (wall[swal].nextwall >= 0)
					//hgltAdd(OBJ_WALL, wall[swal].nextwall);
				
				hglt2dAdd(OBJ_WALL, swal++);
			}


void flipwalls2(short start, short ofs)
{
	int i, j, nume, templong;

	nume = ofs-start;

	for(i=start;i<start+(nume>>1);i++)
	{
		j = start+ofs-i-1;
		templong = wall[i].x; wall[i].x = wall[j].x; wall[j].x = templong;
		templong = wall[i].y; wall[i].y = wall[j].y; wall[j].y = templong;
	}
}

int movewalls2(int start, int offs)
{
	int i;

	if (offs < 0)  //Delete
	{
		for(i=start;i<numwalls+offs;i++)
			Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
	}
	else if (offs > 0)  //Insert
	{
		for(i=numwalls+offs-1;i>=start+offs;i--)
			Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
	}
	numwalls += offs;
	for(i=0;i<numwalls;i++)
	{
		if (wall[i].nextwall >= start) wall[i].nextwall += offs;
		if (wall[i].point2 >= start) wall[i].point2 += offs;
	}
	return(0);
}

void resetWall(int nWall)
{
/* 	int p2 = wall[nWall].point2;
	int nw = wall[nWall].nextwall;
	int ns = wall[nWall].nextsector;
	int ex = wall[nWall].extra;
	int pc = wall[nWall].picnum;
	
	memset(&wall[nWall], 0, sizeof(walltype));
	
	wall[nWall].point2 = p2;
	wall[nWall].nextwall = nw;
	wall[nWall].nextsector = ns;
	wall[nWall].extra = ex;
	wall[nWall].picnum = pc; */
	
	wall[nWall].xrepeat = 8;
	wall[nWall].yrepeat = 8;
	
	fixrepeats(nWall);
}

void resetSector(int nSect)
{
	int wp = sector[nSect].wallptr;
	int wn = sector[nSect].wallnum;
	int ex = sector[nSect].extra;
	
	memset(&sector[nSect], 0, sizeof(sectortype));
	
	sector[nSect].floorz = kensplayerheight;
	sector[nSect].ceilingz = -kensplayerheight;
	sector[nSect].wallptr = wp;
	sector[nSect].wallnum = wn;
	sector[nSect].extra = ex;
	
	int swal, ewal;
	getSectorWalls(nSect, &swal, &ewal);
	while(swal <= ewal)
		resetWall(swal++);
	
}

void fixSectorOfSprites()
{
	int i, j, nSect;
	spritetype* pSpr;

	for (i = 0; i < numsectors; i++)
	{
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			pSpr =& sprite[j]; nSect = pSpr->sectnum;
			FindSector(pSpr->x, pSpr->y, &nSect);
			if (nSect >= 0 && nSect != pSpr->sectnum)
			{
				ChangeSpriteSect(j, nSect);
				i = ClipLow(i - 1, 0);
				break;
			}
		}
	}
}

int insertWalls(int nWhere, walltype* walls, int len)
{
	return 0;
	
}

int walls2Sector(int nSector, walltype* walls, int len, int flags)
{
	return 0;
}

int makeSquareSector(int nInside, int ax, int ay, int nGrids)
{
	walltype* pWall;
	int siz = (2048 >> grid) * (nGrids >> 1);
	int i, w, nWall, nSect, swal, ewal;
	
	if ((numwalls + 4 >= kMaxWalls) || (nInside >= 0 && numwalls + 8 >= kMaxWalls))
		return -1;
	else if (numsectors + 1 >= kMaxSectors)
		return -2;
	
	doGridCorrection(&ax, &ay, grid);
	
	nSect = numsectors;
	nWall = (nInside >= 0) ? sector[nInside].wallptr : numwalls;
	movewalls2(nWall, 4);
	w = nWall;
	
	// clock order
	pWall = &wall[w];
	pWall->x = ax-siz; pWall->y = ay-siz;
	pWall->nextwall = pWall->nextsector = -1;
	pWall->point2 = ++w;
	
	pWall = &wall[w];
	pWall->x = ax+siz; pWall->y = ay-siz;
	pWall->nextwall = pWall->nextsector = -1;
	pWall->point2 = ++w;
	
	pWall = &wall[w];
	pWall->x = ax+siz; pWall->y = ay+siz;
	pWall->nextwall = pWall->nextsector = -1;
	pWall->point2 = ++w;
	
	pWall = &wall[w];
	pWall->x = ax-siz; pWall->y = ay+siz;
	pWall->nextwall = pWall->nextsector = -1;
	pWall->point2 = nWall; 

	// new red sector inside another sector
	if (nInside >= 0)
	{
		sector[nInside].wallnum += 4;
		while(nInside++ < numsectors)
			sector[nInside].wallptr += 4;
		
		flipwalls2(nWall, nWall + 4);
		if (redSectorCanMake(nWall) > 0)
			redSectorMake(nWall);
	}
	// new white sector outside
	else
	{
		sector[nSect].wallptr = nWall;
		sector[nSect].wallnum = 4;
		
		resetSector(nSect);
		numsectors++;
		
		getSectorWalls(nSect, &swal, &ewal);
		while(swal <= ewal)
			checksectorpointer(swal++, nSect);
	}
		
	fixSectorOfSprites();
	
	return nSect;
}
