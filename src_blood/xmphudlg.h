#ifndef __XMPHUDLG_H
#define __XMPHUDLG_H
#include "common_game.h"

struct DIALOG_ITEM
{
	char flags;
	unsigned int enabled			: 1;
	unsigned int x					: 10;
	unsigned int y					: 10;
	unsigned int tabGroup			: 10;
	unsigned int type				: 8;
	char *formatLabel;
	int minValue;
	int maxValue;
	char **names;	
	char (*pHelpFunc)(DIALOG_ITEM* pRoot, DIALOG_ITEM* pItem, BYTE key);
	char *readyLabel;
	int value;
};

class DIALOG_HANDLER
{
	private:
		DIALOG_ITEM* pDlg;
		int nMaxGroup;
	public:
		DIALOG_HANDLER(DIALOG_ITEM* pItem);
		~DIALOG_HANDLER();
		void Paint(void);
		void Paint(DIALOG_ITEM* pItem, char focus);
		void PrintText(DIALOG_ITEM* pItem, char fc, char bc);
		void SetLabel(DIALOG_ITEM* pItem, char *__format, ...);
		void SetValue(int nGroup, int nValue);
		int  GetValue(int nGroup);
		
		DIALOG_ITEM *FindGroup(int nGroup, int dir = 0);
		char Edit(void);
};

enum
{
CONTROL_END 			= 0,
HEADER,
LABEL,
NUMBER,
CHECKBOX,
LIST,
DIALOG
};


enum {
kSprDialogData1 		= 34,
kSprDialogData2,
kSprDialogData3,
kSprDialogData4,
};

enum {
kWallDialogData			= 23,
};

enum {
kSectDialogData			= 33,
};

enum {
NO						= 0x00,
DISABLED				= 0x01,
};

extern DIALOG_ITEM dlgSprite[];
extern DIALOG_ITEM dlgXSprite[];
extern DIALOG_ITEM dlgWall[];
extern DIALOG_ITEM dlgXWall[];
extern DIALOG_ITEM dlgSector[];
extern DIALOG_ITEM dlgXSector[];
extern DIALOG_ITEM dlgXSectorFX[];
extern DIALOG_ITEM dlgMapInfo[];

void EditSectorData(int nSector, BOOL xFirst = TRUE);
void EditWallData(int nWall, BOOL xFirst = TRUE);
void EditSpriteData(int nSprite, BOOL xFirst = TRUE);
void EditSectorLighting(int nSector);
void ShowSectorData(int nSector, BOOL xFirst, BOOL dialog = TRUE);
void ShowWallData(int nWall, BOOL xFirst, BOOL dialog = TRUE);
void ShowSpriteData(int nSprite, BOOL xFirst, BOOL dialog = TRUE);
void ShowMapStatistics(void);

void dlgDialogToXSector(DIALOG_HANDLER* pHandle, int nSector);
void dlgXSectorToDialog(DIALOG_HANDLER* pHandle, int nSector);
void dlgDialogToSector(DIALOG_HANDLER* pHandle, int nSector);
void dlgSectorToDialog(DIALOG_HANDLER* pHandle, int nSector);

void dlgDialogToXSectorFX(DIALOG_HANDLER* pHandle, int nSector);
void dlgXSectorFXToDialog(DIALOG_HANDLER* pHandle, int nSector);

void dlgDialogToSprite(DIALOG_HANDLER* pHandle, int nSprite);
void dlgDialogToXSprite(DIALOG_HANDLER* pHandle, int nSprite);
void dlgXSpriteToDialog(DIALOG_HANDLER* pHandle, int nSprite);
void dlgSpriteToDialog(DIALOG_HANDLER* pHandle, int nSprite);

void dlgDialogToWall(DIALOG_HANDLER* pHandle, int nWall);
void dlgWallToDialog(DIALOG_HANDLER* pHandle, int nWall);
void dlgDialogToXWall(DIALOG_HANDLER* pHandle, int nWall);
void dlgXWallToDialog(DIALOG_HANDLER* pHandle, int nWall);
#endif