
void vgaFont2qfn(unsigned char* table, int length, int charsiz) {
	
	QFONT font;
	dassert(charsiz*256 == length);
	
	int siz = charsiz*charsiz;
	memset(&font, 0, sizeof(font));
	font.sign[0] = 'F';
	font.sign[1] = 'N';
	font.sign[2] = 'T';
	font.sign[3] = '\x1a';
	font.ver = 0x0100;
	font.type = 0;
	font.tcolor = 255;
	font.width = charsiz;
	font.height = charsiz;
	font.charSpace = charsiz;
	font.baseline = charsiz;
	char* data = (char*)malloc(length);
	
	int i, j, k, m, c;
	for (c = 0; c < 256; c++)
	{
		QFONTCHAR* pInfo =& font.info[c];
		pInfo->offset = c * 8;
		
		for (i = 0; i < 8; i++)
		{
			int mask = 0x80;
			for (j = 0; j < 8; j++, mask >>= 1)
			{
				char data2 = table[c*8+i];
				if (data2 & mask)
				{
					k++;
					data[c*8+i] |= mask;
					gfxSetColor(clr2std(kColorWhite));
				}
			}
		}

		pInfo->w = charsiz;
		pInfo->h = charsiz;
		pInfo->ox = charsiz;
		pInfo->oy = charsiz;
	}
	
	font.size = k;

	int nHandle = open("TEST.QFN", O_BINARY|O_TRUNC|O_CREAT|O_WRONLY, S_IREAD|S_IWRITE);
	write(nHandle, &font, sizeof(font)-1);
	write(nHandle, data, length);
	close(nHandle);
	BeepOk();
	
	//keyGet();
	RESHANDLE zzz;
	fileExists("TEST.QFN", &zzz);
	
	QFONT* pFont = (QFONT*)gSysRes.Load(zzz);
	while(!keystatus[KEY_ESC])
	{
		gfxDrawText(20, 20, gStdColor[kColorWhite], "TeStiNg", pFont);
		handleevents();
		showframe();
	}
}


int sectPartsCount(int nSect) {
	
	int swal, ewal, first, i, retn = 0;
	getSectorWalls(nSect, &swal, &ewal);
	
	i = first = swal;
	while (i <= ewal)
	{
		if (wall[i++].point2 != first) continue;
		first = i;
		retn++;
	}
	
	return retn;
}

int getPrevWall(int nWall) {
	
	int swal, ewal;
	getSectorWalls(sectorofwall(nWall), &swal, &ewal);
	for (int i = swal; i <= ewal; i++)
	{
		if (wall[i].point2 == nWall)
			return i;
	}
	
	return -1;
}