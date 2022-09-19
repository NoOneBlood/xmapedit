#ifndef __XMPHUDLG_H
#define __XMPHUDLG_H
#include "common_game.h"

typedef struct DIALOG_ITEM *ITEM_PTR;
typedef BYTE ( *ITEM_PROC )(DIALOG_ITEM* dialog, ITEM_PTR, BYTE key );
struct DIALOG_ITEM {
	
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
	ITEM_PROC fieldHelpProc;
	char *readyLabel;
	int value;
	
};

enum
{
	CONTROL_END = 0,
	HEADER,
	LABEL,
	NUMBER,
	CHECKBOX,
	RADIOBUTTON,
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
NO		= 0x00,
DISABLED = 0x01,
};


/* struct DLG_MAP_INFO
{
	char* pathptr[1];
	char* authptr[1];
	unsigned int revision;
	
};


extern DLG_MAP_INFO gTest; */

extern DIALOG_ITEM dlgSprite[];
extern DIALOG_ITEM dlgXSprite[];
extern DIALOG_ITEM dlgWall[];
extern DIALOG_ITEM dlgXWall[];
extern DIALOG_ITEM dlgSector[];
extern DIALOG_ITEM dlgXSector[];
extern DIALOG_ITEM dlgXSectorFX[];
extern DIALOG_ITEM dlgMapInfo[];

// dialog helper function prototypes
BYTE GetNextUnusedID(DIALOG_ITEM *dialog, DIALOG_ITEM *control, BYTE key );
BYTE DoSectorFXDialog(DIALOG_ITEM *dialog, DIALOG_ITEM *control, BYTE key );
BYTE AuditSound(DIALOG_ITEM *dialog, DIALOG_ITEM *control, BYTE key );
BYTE pickItemTile(DIALOG_ITEM *dialog, DIALOG_ITEM *control, BYTE key);
BYTE pickEnemyTile(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key);
BYTE pickIniMessage(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key);
#endif