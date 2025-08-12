#ifndef __XMPHUDLG_H
#define __XMPHUDLG_H
#include "gfx.h"
#include "inifile.h"

struct MAPEDIT_HUD;

#pragma pack(push, 1)
struct DIALOG_ITEM
{
    char flags;
    unsigned int enabled            : 1;
    unsigned int x                  : 8;
    unsigned int y                  : 8;
    unsigned int tabGroup           : 10;
    unsigned int type               : 8;
    char *formatLabel;
    int minValue;
    int maxValue;
    char **names;
    char (*pHelpFunc)(DIALOG_ITEM* pRoot, DIALOG_ITEM* pItem, BYTE key);
    char *readyLabel;
    int readyLabelLen;
    int value;
    unsigned int selected           : 1;
};
#pragma pack(pop)

class DIALOG_HANDLER
{
    private:
        static IniFile* pHints;
        static int editStatus;
        DIALOG_ITEM *pDlg;
        MAPEDIT_HUD *pHud;
        ROMFONT* pFont;
    public:
        DIALOG_HANDLER(MAPEDIT_HUD* pHud, DIALOG_ITEM* pItem);
        ~DIALOG_HANDLER() { };
        void Paint(void);
        void Paint(DIALOG_ITEM* pItem, char focus);
        void PrintText(DIALOG_ITEM* pItem, char fc, short bc);
        void DrawCheckbox(DIALOG_ITEM* pItem, char fc, short bc);
        void DrawHints(DIALOG_ITEM* pItem);
        void SetLabel(DIALOG_ITEM* pItem, char *__format, ...);
        void SetValue(int nGroup, int nValue);
        int  GetValue(int nGroup);
        char GetItemCoords(DIALOG_ITEM* pItem, int* x1, int* y1, int* x2, int* y2);
        char Edit(void);

        DIALOG_ITEM* FirstItem();
        DIALOG_ITEM* NextItem(DIALOG_ITEM* pItem);
        DIALOG_ITEM* PrevItem(DIALOG_ITEM* pItem);
        DIALOG_ITEM* LastItem(DIALOG_ITEM* pItem = NULL);
        inline char CanAccess(DIALOG_ITEM* pItem) { return (pItem->tabGroup && pItem->enabled); }

};

enum
{
CONTROL_END             = 0,
HEADER,
LABEL,
NUMBER,
NUMBER_HEX,
CHECKBOX,
LIST,
DIALOG,
ELT_SELECTOR,
};


enum {
kSprDialogData1         = 34,
kSprDialogData2,
kSprDialogData3,
kSprDialogData4,
};

enum {
kWallType               = 1,
kWallRX,
kWallRx = kWallRX,
kWallTX,
kWallTx = kWallTX,
kWallState,
kWallCmd,
kWallGoingOn,
kWallGoingOff,
kWallBusyTime,
kWallWaitTime,
kWallRestState,
kWallTrPush,
kWallTrVector,
kWallTrTouch,
kWallKey,
kWallPanX,
kWallPanY,
kWallPanAlways,
kWallDecoupled,
kWallOnce,
kWallLocked,
kWallInterrupt,
kWallLockout,
kWallData           = 23,
};

enum {
kSectType               = 1,
kSectRX,
kSectRx = kWallRX,
kSectTX,
kSectTx = kWallTX,
kSectState,
kSectCmd,
kSectDecoupled,
kSectOnce,
kSectLocked,
kSectInterrupt,
kSectLockout,
kSectGoingOn,
kSectBusyTimeOff,
kSectBusyWaveOff,
kSectWaitOff,
kSectWaitTimeOff,

kSectGoingOff,
kSectBusyTimeOn,
kSectBusyWaveOn,
kSectWaitOn,
kSectWaitTimeOn,

kSectTrPush,
kSectTrWPush,
kSectTrEnter,
kSectTrExit,

kSectSndOffOn,
kSectSndOffStop,
kSectSndOnOff,
kSectSndOnStop,

kSectKey,
kSectDepth,
kSectUwater,
kSectCrush,
kSectData               = 33,
kSectFX,
};

enum {
NO                      = 0x00,
DISABLED                = 0x01,
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
void ShowMapInfo(BOOL showDialog);

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

int dlgCopyObjectSettings(DIALOG_ITEM* pDialog, int nType, int nSrc, int nDst, char hglt);
#endif