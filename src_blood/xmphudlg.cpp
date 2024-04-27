
#include "common_game.h"
#include "db.h"
#include "preview.h"
#include "aadjust.h"
#include "xmphudlg.h"
#include "xmptools.h"
#include "xmpexplo.h"
#include "xmpmaped.h"
#include "tile.h"
#include "hglt.h"

#define kMaxMessages		32
#define kMaxMessageLength	64

// dialog helper function prototypes
static char helperGetNextUnusedID(DIALOG_ITEM *pRoot, DIALOG_ITEM *control, BYTE key);
static char helperDoSectorFXDialog(DIALOG_ITEM *pRoot, DIALOG_ITEM *control, BYTE key);
static char helperAuditSound(DIALOG_ITEM *pRoot, DIALOG_ITEM *control, BYTE key);
static char helperPickItemTile(DIALOG_ITEM *pRoot, DIALOG_ITEM *control, BYTE key);
static char helperPickEnemyTile(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key);
static char helperPickIniMessage(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key);

static int dlgCountItems(DIALOG_ITEM* pDlg)
{
	int c = 1;
	while(pDlg->type != CONTROL_END) { pDlg++, c++; };
	return c;
}

#pragma pack(push, 1)
struct DIALOG_INFO
{
	unsigned int objectType					: 8;
	DIALOG_ITEM* dialog;
	void (*objectToDialog)(DIALOG_HANDLER*, int);
	void (*dialogToObject)(DIALOG_HANDLER*, int);
	const char* pHintGroup;
	signed int objectTypeHintID				: 8;
};
#pragma pack(pop)

DIALOG_INFO* GetDialogInfo(DIALOG_ITEM* pDlg, int nID);

DIALOG_ITEM dlgXSprite[] =
{
	{ NO,	1,		0,	0,	1,	LIST,			"Type %4d: %-18.18s", 0, 1023, gSpriteNames },
	{ NO,	1,		0,	1,	2,	NUMBER, 		"RX ID: %-4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	2,	3,	NUMBER, 		"TX ID: %-4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	3,	4,	LIST, 			"State  %1d: %-3s", 0, 1, gBoolNames },
	{ NO,	1,		0,	4,	5,	LIST, 			"Cmd: %3d: %-16s", 0, 255, gCommandNames },

	{ NO,	1,		0,	5,	0,	HEADER, 		"Send when:" },
	{ NO,	1,		0,	6,	6,	CHECKBOX,		"going ON" },
	{ NO,	1,		0,	7,	7,	CHECKBOX,		"going OFF" },
	{ NO,	1,		0,	8,	8,	NUMBER, 		"busyTime = %-4d", 0, 4095 },
	{ NO,	1,		0,	9,	9,	NUMBER, 		"waitTime = %-4d", 0, 4095 },
	{ NO,	1,		0,	10,	10,	LIST,			"restState %1d: %-3s", 0, 1, gBoolNames },

	{ NO,	1,		30,	0,	0,	HEADER,			"Trigger On:" },
	{ NO,	1,		30,	1,	11,	CHECKBOX,		"Push" },
	{ NO,	1,		30,	2,	12,	CHECKBOX,		"Vector" },
	{ NO,	1,		30,	3,	13,	CHECKBOX,		"Impact" },
	{ NO,	1,		30,	4,	14,	CHECKBOX,		"Pickup" },
	{ NO,	1,		30,	5,	15,	CHECKBOX, 		"Touch" },
	{ NO,	1,		30,	6,	16,	CHECKBOX, 		"Proximity" },
	{ MO,	1,		30,	7,	17,	CHECKBOX, 		"Sees player" },
	{ MO,	1,		30,	8,	18,	CHECKBOX, 		"Screen" },
	{ MO,	1,		39,	8,	19,	CHECKBOX, 		"Aim" },

	{ NO,	1,		21,	9,	0,	HEADER,			"Launch 1 2 3 4 5 S B C T" },
	{ NO,	1,		28,	10,	20,	CHECKBOX, 		"" },
	{ NO,	1,		30,	10,	21,	CHECKBOX, 		"" },
	{ NO,	1,		32,	10,	22,	CHECKBOX, 		"" },
 	{ NO,	1,		34,	10,	23,	CHECKBOX, 		"" },
	{ NO,	1,		36,	10,	24,	CHECKBOX, 		"" },
	{ NO,	1,		38,	10,	25,	CHECKBOX, 		"" },
	{ NO,	1,		40,	10,	26,	CHECKBOX, 		"" },
	{ NO,	1,		42,	10,	27,	CHECKBOX, 		"" },
	{ NO,	1,		44,	10,	28,	CHECKBOX, 		"" },
	
	{ NO,	1,		46,	0,	0,	HEADER, 		"Trigger Flags:  " },
	{ NO,	1,		46,	1,	29,	CHECKBOX, 		"Decoupled" },
	{ NO,	1,		46,	2,	30,	CHECKBOX,		"1-shot" },
	{ NO,	1,		46,	3,	31,	CHECKBOX,		"Locked" },
	{ NO,	1,		46,	4,	32,	CHECKBOX,		"Interruptable" },
	{ NO,	1,		46,	5,	33,	CHECKBOX, 		"Player only" },

	{ NO,	1,		46,	6,	0,	HEADER, 		"Data:           " },
	{ NO,	1,		46, 7,	kSprDialogData1,	NUMBER, 	"Data1", -32767, 32767, NULL, helperAuditSound },
	{ NO,	1,		46,	8,	kSprDialogData2,	NUMBER, 	"Data2", -32767, 32767, NULL, helperAuditSound },
	{ NO,	1,		46,	9,	kSprDialogData3,	NUMBER, 	"Data3", -32767, 32767, NULL, helperAuditSound },
	{ NO,	1,		46,	10,	kSprDialogData4,	NUMBER, 	"Data4", -65535, 65535, NULL, helperAuditSound },

	{ NO,	1,		64,	0,	0,	HEADER, 		"Respawn:      " },
	{ NO,	1,		64,	1,	38,	LIST, 			"When %1d: %-6.6s", 0, 3, gRespawnNames },

	{ NO,	1,		64,	2,	0,	HEADER, 		"Dude P D B A S" },
	{ MO,	1,		69,	3,	39,	CHECKBOX,		"" },
	{ MO,	1,		71,	3,	40,	CHECKBOX, 		"" },
	{ MO,	1,		73,	3,	41,	CHECKBOX,		"" },
	{ MO,	1,		75,	3,	42,	CHECKBOX,		"" },
	{ MO,	1,		77,	3,	43,	CHECKBOX,		"" },

	{ NO,	1,		64,	5,	0,	HEADER, 		"Miscellaneous:" },
	{ NO,	1,		64,	6,	44,	LIST,			"Key:  %1d %-7.7s", 0, 7, gKeyItemNames },
	{ NO,	1,		64,	7,	45,	LIST,			"Wave: %1d %-7.7s", 0, 3, gBusyNames },
	{ NO,	1,		64,	8,	46,	NUMBER,			"Hi-tag: %-6d",  -32767, 32767, },
	{ NO,	1,		64,	9,	47,	NUMBER,			"Message:   %-3d", 0, 255, NULL, helperPickIniMessage },
	{ NO,	1,		64,	10,	48,	NUMBER,			"Drop item: %-3d", 0, kItemMax, NULL, helperPickItemTile },

	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgSprite[] =
{
	{ NO,	1,		0,	0,	0,	HEADER, 		"General:" },
	{ NO,	1,		0,	1,	1,	NUMBER,			"X-coordinate..: %d",		-kDefaultBoardSize, kDefaultBoardSize,	NULL,	NULL},
	{ NO,	1,		0,	2,	2,	NUMBER,			"Y-coordinate..: %d", 		-kDefaultBoardSize, kDefaultBoardSize,	NULL,	NULL},
	{ NO,	1,		0,	3,	3,	NUMBER,			"Z-coordinate..: %d", 		-13421721, 			13421722, 			NULL,	NULL},
	{ NO,	0,		0,	4,	4,	NUMBER,			"Sectnum.......: %d", 		0, 					kMaxSectors-1,		NULL,	NULL},
	{ NO,	1,		0,	5,	5,	NUMBER,			"Statnum.......: %d", 		0, 					kMaxStatus,			NULL,	NULL},
	{ NO,	1,		0,	6,	6,	NUMBER,			"Hi-tag........: %d",  		-32767, 			32768, 				NULL,	NULL},
	{ NO,	1,		0,	7,	7,	NUMBER,			"Lo-tag........: %d",  		-32767, 			32768, 				NULL,	NULL},
	{ NO,	1,		0,	8,	8,	NUMBER,			"Clipdist......: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	0,		0,	9,	9,	NUMBER,			"Extra.........: %d", 		-1, 				kMaxXSprites - 1,	NULL,	NULL},
	
	
	{ NO,	1,		24,	1,	0,	HEADER, 		"Appearance:" },
	{ NO,	1,		24,	2,	10,	NUMBER,			"Tilenum.......: %d", 		0, 					kMaxTiles - 1,		NULL,	NULL},
	{ NO,	1,		24,	3,	11,	NUMBER,			"Shade.........: %d", 		-127, 				63,					NULL,	NULL},
	{ NO,	1,		24,	4,	12,	NUMBER,			"Palookup......: %d", 		0, 					kPluMax - 1,		NULL,	NULL},
	{ NO,	1,		24,	5,	13,	NUMBER,			"X-repeat......: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		24,	6,	14,	NUMBER,			"Y-repeat......: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		24,	7,	15,	NUMBER,			"X-offset......: %d", 		0, 					127,				NULL,	NULL},
	{ NO,	1,		24,	8,	16,	NUMBER,			"Y-offset......: %d", 		0, 					127,				NULL,	NULL},
	{ NO,	1,		24,	9,	17,	NUMBER,			"Slope.........: %d", 		-32767, 			32768,				NULL,	NULL},
	
	{ NO,	1,		46,	1,	0,	HEADER, 		"Miscellaneous:" },
	{ NO,	1,		46,	2,	18,	NUMBER,			"Angle...........: %d",		-32767, 			32768,				NULL,	NULL},
	{ NO,	0,		46,	3,	19,	NUMBER,			"X-velocity......: %d", 	-13421721, 			13421728,			NULL,	NULL},
	{ NO,	0,		46,	4,	20,	NUMBER,			"Y-velocity......: %d", 	-13421721, 			13421728,			NULL,	NULL},
	{ NO,	0,		46,	5,	21,	NUMBER,			"Z-velocity......: %d", 	-13421721, 			13421728,			NULL,	NULL},
	{ NO,	0,		46,	6,	22,	NUMBER,			"Owner...........: %d", 	-1, 				32767,				NULL,	NULL},
	
/* 	{ NO,	1,		74,	1,	0,	HEADER, 		"Cstat flags:        " },
	{ NO,	1,		74,	2,	23,	RADIOBUTTON,	"Flor", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		81,	2,	24,	RADIOBUTTON,	"Wall", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		88,	2,	25,	RADIOBUTTON,	"Slop", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	3,	26,	CHECKBOX,		"Blocking", 				0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	4,	27,	CHECKBOX,		"Hitscan", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	5,	28,	CHECKBOX,		"One-Sided", 				0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	6,	29,	CHECKBOX,		"Invisible", 				0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	7,	30,	CHECKBOX,		"Center align",				0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	8,	31,	CHECKBOX,		"Flip-X", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		83,	8,	32,	CHECKBOX,		"Y", 						0, 						0,				NULL,	NULL},	
	{ NO,	1,		74,	9,	33,	CHECKBOX,		"Translucent", 				0, 						0,				NULL,	NULL},
	{ NO,	1,		88,	9,	34,	CHECKBOX,		"less", 					0, 						0,				NULL,	NULL},
	{ NO,	1,		74,	10,	35,	RADIOBUTTON,	"Move normal", 				0, 						0,				NULL,	NULL},
	{ NO,	1,		88,	10,	36,	RADIOBUTTON,	"reverse", 					0, 						0,				NULL,	NULL}, */
	
	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgXWall[] =
{
	{ NO,	1,		0,	0,	kWallType,			LIST, 		"Type %4d: %-18.18s", 0, 1023, gWallNames },
	{ NO,	1,		0,	1,	kWallRX,			NUMBER, 	"RX ID: %4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	2,	kWallTX,			NUMBER, 	"TX ID: %4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	3,	kWallState,			LIST, 		"State %1d: %-3.3s", 0, 1, gBoolNames },
	{ NO,	1,		0,	4,	kWallCmd,			LIST, 		"Cmd: %3d: %-18.18s", 0, 255, gCommandNames },

	{ NO,	1,		0,	5,	0,					HEADER, 	"Send when:" },
	{ NO,	1,		0,	6,	kWallGoingOn,		CHECKBOX,	"going ON" },
	{ NO,	1,		0,	7,	kWallGoingOff,		CHECKBOX,	"going OFF" },
	{ NO,	1,		0,	8,	kWallBusyTime,		NUMBER, 	"busyTime = %4d", 0, 4095 },
	{ NO,	1,		0,	9,	kWallWaitTime,		NUMBER, 	"waitTime = %4d", 0, 4095 },
	{ NO,	1,		0,	10,	kWallRestState,		LIST,		"restState %1d: %-3.3s", 0, 1, gBoolNames },

	{ NO,	1,		30,	0,	0,					HEADER,			"Trigger On:" },
	{ NO,	1,		30,	1,	kWallTrPush,		CHECKBOX,	"Push" },
	{ NO,	1,		30,	2,	kWallTrVector,		CHECKBOX,	"Vector" },
	{ MO,	1,		30,	3,	kWallTrTouch,		CHECKBOX,	"Touch" },


	{ NO,	1,		30,	5,	kWallKey,			LIST,		"Key: %1d %-7.7s", 0, 7, gKeyItemNames },
	{ NO,	1,		30,	7,	kWallPanX,			NUMBER, 	"panX = %4d", -128, 127 },
	{ NO,	1,		30,	8,	kWallPanY,			NUMBER, 	"panY = %4d", -128, 127 },
	{ NO,	1,		30,	9,	kWallPanAlways,		CHECKBOX,	"always" },

	{ NO,	1,		48,	0,	0,					HEADER, 	"Trigger Flags:" },
	{ NO,	1,		48,	1,	kWallDecoupled,		CHECKBOX, 	"Decoupled" },
	{ NO,	1,		48,	2,	kWallOnce,			CHECKBOX,	"1-shot" },
	{ NO,	1,		48,	3,	kWallLocked,		CHECKBOX, 	"Locked" },
	{ NO,	1,		48,	4,	kWallInterrupt,		CHECKBOX,	"Interruptable" },
	{ NO,	1,		48,	5,	kWallLockout,		CHECKBOX,	"Player only" },

	{ NO,	1,		48,	7,	0,					HEADER, 	"Data:         " },
	{ NO,	1,		48,	8,	kWallData,			NUMBER, 	"Data", -65535, 65535, NULL, helperAuditSound },

	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgWall[] =
{
	{ NO,	1,		0,	0,	0,	HEADER, 		"General:" },
	{ NO,	0,		0,	1,	1,	NUMBER,			"X-coordinate: %d",			-kDefaultBoardSize,		kDefaultBoardSize,	NULL,	NULL},
	{ NO,	0,		0,	2,	2,	NUMBER,			"Y-coordinate: %d",			-kDefaultBoardSize,		kDefaultBoardSize, 	NULL,	NULL},
	{ NO,	0,		0,	3,	3,	NUMBER,			"Point2......: %d", 		0, 						kMaxWalls - 1, 		NULL,	NULL},
	{ NO,	0,		0,	4,	4,	NUMBER,			"Sector......: %d", 		0,						kMaxSectors - 1, 	NULL,	NULL},
	{ NO,	0,		0,	5,	5,	NUMBER,			"Next wall...: %d", 		-1, 					kMaxWalls - 1,		NULL,	NULL},
	{ NO,	0,		0,	6,	6,	NUMBER,			"Next sector.: %d", 		-1, 					kMaxSectors - 1,	NULL,	NULL},
	{ NO,	1,		0,	7,	7,	NUMBER,			"Hi-tag......: %d",  		-32767, 				32768, 				NULL,	NULL},
	{ NO,	1,		0,	8,	8,	NUMBER,			"Lo-tag......: %d",  		-32767, 				32768, 				NULL,	NULL},
	{ NO,	0,		0,	9,	9,	NUMBER,			"Extra.......: %d", 		-1, 					kMaxXWalls - 1,		NULL,	NULL},
	
	{ NO,	1,		22,	1,	0,	HEADER, 		"Appearance:" },
	{ NO,	1,		22,	2,	10,	NUMBER,			"Tilenum.....: %d", 		0, 						kMaxTiles - 1,		NULL,	NULL},
	{ NO,	1,		22,	3,	11,	NUMBER,			"Mask tilenum: %d", 		0, 						kMaxTiles - 1,		NULL,	NULL},
	{ NO,	1,		22,	4,	12,	NUMBER,			"Shade.......: %d", 		-127, 					63,					NULL,	NULL},
	{ NO,	1,		22,	5,	13,	NUMBER,			"Palookup....: %d", 		0, 						kPluMax - 1,		NULL,	NULL},
	{ NO,	1,		22,	6,	14,	NUMBER,			"X-repeat....: %d", 		0, 						255,				NULL,	NULL},
	{ NO,	1,		22,	7,	15,	NUMBER,			"Y-repeat....: %d", 		0, 						255,				NULL,	NULL},
	{ NO,	1,		22,	8,	16,	NUMBER,			"X-panning...: %d", 		0, 						255,				NULL,	NULL},
	{ NO,	1,		22,	9,	17,	NUMBER,			"Y-panning...: %d", 		0, 						255,				NULL,	NULL},
		
/* 	{ NO,	1,		44,	1,	0,	HEADER, 		"Cstat settings:" },
	{ NO,	1,		44,	2,	18,	CHECKBOX,		"Blocking", 				0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	3,	19,	CHECKBOX,		"Bottom Swap", 				0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	4,	20,	CHECKBOX,		"Bottom Pegged", 			0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	5,	21,	CHECKBOX,		"Flip-X", 					0, 						0,					NULL,	NULL},
	{ NO,	1,		53,	5,	22,	CHECKBOX,		"Y", 						0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	6,	23,	CHECKBOX,		"Masked", 					0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	7,	24,	CHECKBOX,		"OneWay", 					0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	8,	25,	CHECKBOX,		"Hitscan", 					0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	9,	26,	CHECKBOX,		"Translucent", 				0, 						0,					NULL,	NULL},
	{ NO,	1,		58,	9,	27,	CHECKBOX,		"less", 					0, 						0,					NULL,	NULL},
	{ NO,	1,		44,	10,	28,	RADIOBUTTON,	"Move normal", 				0, 						0,					NULL,	NULL},
	{ NO,	1,		58,	10,	29,	RADIOBUTTON,	"reverse", 					0, 						0,					NULL,	NULL}, */
	
	//{ NO,	1,		70,	1,	0,	HEADER, 		"Miscellaneous:" },
	//{ NO,	1,		70,	2,	30,	NUMBER,			"Cstat (hex).....: 0x%08lx",	0, 					65535,				NULL,	NULL},
	//{ NO,	1,		70,	3,	31,	NUMBER,			"Angle (2048 deg): %d",			0, 					2047,				NULL,	NULL},
	//{ NO,	1,		70,	4,	32,	NUMBER,			"Length..........: %d", 		0, 					0x800000,			NULL,	NULL},
	//{ NO,	1,		70,	5,	33,	NUMBER,			"Pixel height....: %d", 		0, 					0x800000,			NULL,	NULL},

	
	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgXSectorFX[] =
{
	{ NO,	1,		0,	0,	0,	HEADER, 	"Lighting:" },
	{ NO,	1,		0,	1,	1,	LIST, 		"Wave: %1d %-9.9s", 0, 11, gWaveNames },
	{ NO,	1,		0,	2,	2,	NUMBER, 	"Amplitude: %+4d", -128, 127 },
	{ NO,	1,		0,	3,	3,	NUMBER, 	"Freq:    %3d", 0, 255 },
	{ NO,	1,		0,	4,	4,	NUMBER, 	"Phase:     %3d", 0, 255 },
	{ NO,	1,		0,	5,	5,	CHECKBOX,	"floor" },
	{ NO,	1,		0,	6,	6,	CHECKBOX,	"ceiling" },
	{ NO,	1,		0,	7,	7,	CHECKBOX,	"walls" },
	{ NO,	1,		0,	8,	8,	CHECKBOX,	"shadeAlways" },

	{ NO,	1,		20,	0,	0,	HEADER,		"More Lighting:" },
	{ NO,	1,		20,	1,	9,	CHECKBOX,	"Color Lights" },
	{ NO,	1,		20,	2,	10,	NUMBER,		"ceil  pal2 = %3d", 0, 15 },
	{ NO,	1,		20,	3,	11,	NUMBER,		"floor pal2 = %3d", 0, 15 },

	{ NO,	1,		40,	0,	0,	HEADER, 	"Motion FX:" },
	{ NO,	1,		40,	1,	12,	NUMBER, 	"Speed = %4d", 0, 255 },
	{ NO,	1,		40,	2,	13,	NUMBER, 	"Angle = %4d", 0, kAngMask },
	{ NO,	1,		40,	3,	14,	CHECKBOX,	"pan floor" },
	{ NO,	1,		40,	4,	15,	CHECKBOX,	"pan ceiling" },
	{ NO,	1,		40,	5,	16,	CHECKBOX,	"panAlways" },
	{ NO,	1,		40,	6,	17,	CHECKBOX,	"drag" },

	{ NO,	1,		40,	8,	18,	NUMBER,		"Wind vel: %4d", 0, 1023 },
	{ NO,	1,		40,	9,	19,	NUMBER,		"Wind ang: %4d", 0, kAngMask },
	{ NO,	1,		40,	10,	20,	CHECKBOX,	"Wind always" },

	{ NO,	1,		60,	0,	0,	HEADER, 	"Continuous motion:" },
	{ NO,	1,		60,	1,	21,	NUMBER,		"Z range: %3d", 0, 31 },
	{ NO,	1,		60,	2,	22,	NUMBER,		"Theta: %-4d", 0, 2047 },
	{ NO,	1,		60,	3,	23,	NUMBER,		"Speed: %-4d", -2048, 2047 },
	{ NO,	1,		60,	4,	24,	CHECKBOX,	"always" },
	{ NO,	1,		60,	5,	25,	CHECKBOX,	"bob floor" },
	{ NO,	1,		60,	6,	26,	CHECKBOX,	"bob ceiling" },
	{ NO,	1,		60,	7,	27,	CHECKBOX,	"rotate" },
	{ NO,	1,		60,	9,	28,	LIST,		"DamageType: %1d %-7.7s", 0, 6, gDamageNames},

	{ NO,	1,		0,	0,	0,	CONTROL_END },
};


DIALOG_ITEM dlgXSector[] =
{
	{ NO,	1,		0,	0,	kSectType,			LIST, 		"Type %4d: %-16.16s", 0, 1023, gSectorNames },
	{ NO,	1,		0,	1,	kSectRX,			NUMBER, 	"RX ID: %4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	2,	kSectTX,			NUMBER, 	"TX ID: %4d", 0, 1023, NULL, helperGetNextUnusedID },
	{ NO,	1,		0,	3,	kSectState,			LIST, 		"State %1d: %-3.3s", 0, 1, gBoolNames },
	{ NO,	1,		0,	4,	kSectCmd,			LIST, 		"Cmd: %3d: %-16.16s", 0, 255, gCommandNames },

	{ NO,	1,		0,	5,	0,					HEADER, 	"Trigger Flags:" },
	{ NO,	1,		0,	6,	kSectDecoupled,		CHECKBOX, 	"Decoupled" },
	{ NO,	1,		0,	7,	kSectOnce,			CHECKBOX,	"1-shot" },
	{ NO,	1,		0,	8,	kSectLocked,		CHECKBOX, 	"Locked" },
	{ NO,	1,		0,	9,	kSectInterrupt,		CHECKBOX, 	"Interruptable" },
	{ NO,	1,		0,	10, kSectLockout,		CHECKBOX,	"Player only" },

	{ NO,	1,		28,	0,	0,					HEADER, 	"OFF->ON:" },
	{ NO,	1,		28,	1,	kSectGoingOn,		CHECKBOX,	"send at ON" },
	{ NO,	1,		28,	2,	kSectBusyTimeOff,	NUMBER, 	"busyTime   = %3d", 0, 4095 },
	{ NO,	1,		28,	3,	kSectBusyWaveOff,	LIST, 		"Wave: %1d %-8.8s", 0, 3, gBusyNames },
	{ NO,	1,		28,	4,	kSectWaitOff,		CHECKBOX, 	"" },
	{ NO,	1,		30,	4,	kSectWaitTimeOff,	NUMBER, 	"waitTime = %3d", 0, 4095 },

	{ NO,	1,		28,	6,	0,					HEADER, 	"ON->OFF:" },
	{ NO,	1,		28,	7,	kSectGoingOff,		CHECKBOX,	"send at OFF" },
	{ NO,	1,		28,	8,	kSectBusyTimeOn,	NUMBER, 	"busyTime   = %3d", 0, 4095 },
	{ NO,	1,		28,	9,	kSectBusyWaveOn,	LIST, 		"Wave: %1d %-8.8s", 0, 3, gBusyNames },
	{ NO,	1,		28,	10,	kSectWaitOn,		CHECKBOX, 	"" },
	{ NO,	1,		30,	10,	kSectWaitTimeOn,	NUMBER, 	"waitTime = %3d", 0, 4095 },

	{ NO,	1,		47,	0,	0,					HEADER, 	"Trigger On:    " },
	{ NO,	1,		47,	1,	kSectTrPush,		CHECKBOX,	"Push" },
	{ NO,	1,		47,	2,	kSectTrWPush,		CHECKBOX,	"WallPush" },
	{ NO,	1,		47,	3,	kSectTrEnter,		CHECKBOX,	"Enter" },
	{ NO,	1,		47,	4,	kSectTrExit,		CHECKBOX,	"Exit" },

	{ NO,	1,		47,	6,	0,					HEADER, 	"Sound:         " },
	{ NO,	1,		47,	7,	kSectSndOffOn,		NUMBER,		"Off->On : %5d", 0, 32768, NULL, helperAuditSound },
	{ NO,	1,		47,	8,	kSectSndOffStop,	NUMBER,		"Stopping: %5d", 0, 32768, NULL, helperAuditSound },
	{ NO,	1,		47,	9,	kSectSndOnOff,		NUMBER,		"On->Off : %5d", 0, 32768, NULL, helperAuditSound },
	{ NO,	1,		47,	10,	kSectSndOnStop,		NUMBER,		"Stopping: %5d", 0, 65535, NULL, helperAuditSound },

	{ NO,	1,		65,	0,	kSectKey,			LIST,		"Key:   %1d %-7.7s", 0, 7, gKeyItemNames },
	{ NO,	1,		65,	1,	kSectDepth,			LIST,		"Depth: %1d %-7.7s", 0, 7, gDepthNames },
	{ NO,	1,		65,	3,	kSectUwater,		CHECKBOX, 	"Underwater" },
	{ NO,	1,		65,	4,	kSectCrush,			CHECKBOX, 	"Crush" },

	{ NO,	1,		65,	6,	0,					HEADER, 	"Data:         " },
	{ NO,	1,		65,	7,	kSectData,			NUMBER,		"Data", -32768, 32767 },
	{ NO,	1,		65,	10,	kSectFX,			DIALOG,		"FX...",  0, 0, NULL, helperDoSectorFXDialog },
	{ NO,	1,		0,	0,	0,					CONTROL_END },
};

DIALOG_ITEM dlgSector[] =
{
	{ NO,	1,		0,	0,	0,	HEADER, 		"General:" },
	{ NO,	0,		0,	1,	1,	NUMBER,			"Total walls..: %d",		0,					kMaxWalls - 1,		NULL,	NULL},
	{ NO,	1,		0,	2,	2,	NUMBER,			"First wall...: %d",		0,					kMaxWalls - 1,		NULL,	NULL},
	{ NO,	0,		0,	3,	3,	NUMBER,			"Total sprites: %d",		0,					kMaxSprites - 1,	NULL,	NULL},
	{ NO,	0,		0,	4,	4,	NUMBER,			"First sprite : %d",		-1,					kMaxSprites - 1,	NULL,	NULL},
	{ NO,	1,		0,	5,	5,	NUMBER,			"Hi-tag.......: %d",  		-32767, 			32768, 				NULL,	NULL},
	{ NO,	1,		0,	6,	6,	NUMBER,			"Lo-tag.......: %d",  		-32767, 			32768, 				NULL,	NULL},
	{ NO,	1,		0,	7,	7,	NUMBER,			"Visibility...: %d",  		0, 					239, 				NULL,	NULL},
	{ NO,	0,		0,	8,	8,	NUMBER,			"Extra........: %d",  		-1, 				kMaxXSectors - 1, 	NULL,	NULL},
	
	{ NO,	1,		22,	1,	0,	HEADER, 		"Ceiling settings:" },
	{ NO,	1,		22,	2,	9,	NUMBER,			"Tilenum......: %d", 		0, 					kMaxTiles - 1,		NULL,	NULL},
	{ NO,	1,		22,	3,	10,	NUMBER,			"Shade........: %d", 		-127, 				63,					NULL,	NULL},
	{ NO,	1,		22,	4,	11,	NUMBER,			"Palookup.....: %d", 		0, 					kPluMax - 1,		NULL,	NULL},
	{ NO,	1,		22,	5,	12,	NUMBER,			"X-panning....: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		22,	6,	13,	NUMBER,			"Y-panning....: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		22,	7,	14,	NUMBER,			"Slope........: %d", 		-32767, 			32768,				NULL,	NULL},
	{ NO,	1,		22,	8,	15,	NUMBER,			"Z-coordinate.: %d",		-13421721,			13421722,			NULL,	NULL},
	
	{ NO,	1,		46,	1,	0,	HEADER, 		"Floor settings:" },
	{ NO,	1,		46,	2,	16,	NUMBER,			"Tilenum......: %d", 		0, 					kMaxTiles - 1,		NULL,	NULL},
	{ NO,	1,		46,	3,	17,	NUMBER,			"Shade........: %d", 		-127, 				63,					NULL,	NULL},
	{ NO,	1,		46, 4,	18,	NUMBER,			"Palookup.....: %d", 		0, 					kPluMax - 1,		NULL,	NULL},
	{ NO,	1,		46,	5,	19,	NUMBER,			"X-panning....: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		46,	6,	20,	NUMBER,			"Y-panning....: %d", 		0, 					255,				NULL,	NULL},
	{ NO,	1,		46,	7,	21,	NUMBER,			"Slope........: %d", 		-32767, 			32768,				NULL,	NULL},
	{ NO,	1,		46,	8,	22,	NUMBER,			"Z-coordinate.: %d",		-13421721,			13421722,			NULL,	NULL},
	
/* 	{ NO,	1,		70,	0,	0,	HEADER, 		"Cstat flags     C F" },
	{ NO,	1,		70,	1,	0,	LABEL,			"Sloped..........", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	1,	23,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	1,	24,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	2,	0,	LABEL,			"Parallax........", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	2,	25,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	2,	26,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	3,	0,	LABEL,			"Expanded........", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	3,	27,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	3,	28,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	4,	0,	LABEL,			"Relative align..", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	4,	29,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	4,	30,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	5,	0,	LABEL,			"Flip-X..........:", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	5,	31,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	5,	32,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	6,	0,	LABEL,			"Flip-Y..........:", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	6,	33,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	6,	34,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	7,	0,	LABEL,			"Shade from......:", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	7,	35,	RADIOBUTTON,	"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	7,	36,	RADIOBUTTON,	"", 						0, 					0,					NULL,	NULL},
	
	{ NO,	1,		70,	8,	0,	LABEL,			"Masked..........:", 		0, 					0,					NULL,	NULL},
	{ NO,	1,		86,	8,	37,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL},
	{ NO,	1,		88,	8,	38,	CHECKBOX,		"", 						0, 					0,					NULL,	NULL}, */
	
	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

static DIALOG_INFO gDialogInfo[] =
{
	{ OBJ_SPRITE, 	dlgXSprite, 	dlgXSpriteToDialog, 	dlgDialogToXSprite,		gSearchStatNames[OBJ_SPRITE],	1 },
	{ OBJ_WALL, 	dlgXWall, 		dlgXWallToDialog, 		dlgDialogToXWall,		gSearchStatNames[OBJ_WALL],		kWallType, },
	{ OBJ_SECTOR, 	dlgXSector, 	dlgXSectorToDialog, 	dlgDialogToXSector,		gSearchStatNames[OBJ_SECTOR],	kSectType, },
	{ OBJ_SECTOR, 	dlgXSectorFX, 	dlgXSectorFXToDialog, 	dlgDialogToXSectorFX,	"SectorFX",						-1, },
};

DIALOG_INFO* GetDialogInfo(DIALOG_ITEM* pDlg, int nID = OBJ_NONE)
{
	int i = LENGTH(gDialogInfo);
	DIALOG_INFO* pEntry;
	
	while(--i >= 0)
	{
		pEntry = &gDialogInfo[i];
		if (pEntry->dialog == pDlg && (nID == OBJ_NONE || pEntry->objectType == nID))
			return pEntry;
	}
	
	return NULL;
}

int DIALOG_HANDLER::editStatus = 0;
IniFile* DIALOG_HANDLER::pHints = NULL;
DIALOG_HANDLER::DIALOG_HANDLER(MAPEDIT_HUD* pHud, DIALOG_ITEM* pItem)
{
	this->pFont		= pHud->GetFont(kHudFontCont);
	this->pDlg		= pItem;
	this->pHud		= pHud;
	
	DIALOG_ITEM* pCur = pDlg;
	while(pCur->type != CONTROL_END)
	{
		if (pCur->readyLabel)
			break; // dialog already initialized
		
		switch(pCur->type)
		{
			case HEADER:
				SetLabel(pCur, "%s", pCur->formatLabel);
				strupr(pCur->readyLabel);
				break;
			case LIST:
			case NUMBER:
				break;
			case LABEL:
				pCur->readyLabel = (pCur->names) ? NULL : pCur->formatLabel;
				break;
			default:
				pCur->readyLabel = pCur->formatLabel;
				break;
		}
		
		if (pCur->readyLabel)
			pCur->readyLabelLen = strlen(pCur->readyLabel);
		
		pCur++;
	}
	
}

void DIALOG_HANDLER::Paint()
{
	DIALOG_ITEM* pCur = pDlg;
	while(pCur->type != CONTROL_END)
	{
		Paint(pCur, 0);
		pCur++;
	}
}

void DIALOG_HANDLER::Paint(DIALOG_ITEM* pItem, char focus)
{
	unsigned char fc = clr2std(kColorLightCyan);
	short bc = -1;

	if (focus)
	{
		fc = clr2std(kColorYellow);
		bc = clr2std(kColorBlack);

		// show hints while editing only
		if (editStatus && pHints)
			DrawHints(pItem);
	}
	else if (pItem->type == ELT_SELECTOR)
	{
		if (pItem->selected)
			bc = clr2std(kColorMagenta);
	}
	else if (pItem->type == HEADER)
	{
		fc = clr2std(kColorWhite);
		bc = clr2std(kColorGreen);
	}
	else if (!pItem->enabled)
	{
		fc = clr2std(kColorGrey20);
	}
	else if (pItem->flags > 0 && gMisc.showTypes != pItem->flags)
	{
		fc = clr2std(kColorCyan);
	}
	
	switch (pItem->type)
	{
		case LABEL:
			if (pItem->names)
				SetLabel(pItem, pItem->formatLabel, pItem->names[pItem->value]);
			
			PrintText(pItem, fc, bc);
			break;
		case NUMBER:
			if (pDlg == dlgXSprite && irngok(pItem->tabGroup, kSprDialogData1, kSprDialogData4))
			{
				int nType = pDlg->value, maxRng = LENGTH(gSpriteDataNames);
				char** dataName = gSpriteDataNames[nType], *label;
				int longest = 5; int nData, i, j, k;
				
				nData = ((pItem->tabGroup - kSprDialogData4) + 4) - 1;
				if (!rngok(nType, 0, maxRng) || (label = dataName[nData]) == NULL)
					label = pItem->formatLabel;

				// special case
				if (label[0] == kPrefixSpecial)
				{
					DIALOG_ITEM* pCtrl;
					switch (nType)
					{
						case 500: // data names depends on CMD type
							for (pCtrl = pDlg; pCtrl->tabGroup != 5; pCtrl++);
							for (j = 0; j < LENGTH(pCtrlDataNames); j++, label = pItem->formatLabel)
							{
								if (pCtrlDataNames[j].var1 != pCtrl->value) continue;
								else if (!pCtrlDataNames[j].dataOvr[nData]) continue;
								else label = pCtrlDataNames[j].dataOvr[nData];
								break;
							}

							// get longest strlen from special array
							if (j == LENGTH(pCtrlDataNames)) break;
							for (k = 0; k < 4; k++)
							{
								if (!pCtrlDataNames[j].dataOvr[k]) continue;
								if ((i = strlen(pCtrlDataNames[j].dataOvr[k])) > longest)
									longest = i;
							}
							break;
					}
				}
				
				if (rngok(nType, 0, maxRng))
				{
					for (i = 0; i < 4; i++)
					{
						if (dataName[i] == NULL) continue;
						else if ((j = strlen(dataName[i])) > longest)
							longest = j;
					}
				}

				i = sprintf(buffer, "%-0.10s", label);
				if ((j = strlen(label)) < longest)
				{
					k = ClipRange(longest - j, 0, 5);
					memset(&buffer[i], 32, k);
					i+=k;
				}

				sprintf(&buffer[i], ": %-6d", pItem->value);
			}
			else if (pDlg == dlgXWall && pItem->tabGroup == kWallData)
			{
				// get wall type
				DIALOG_ITEM *wallType = pDlg;
				int maxRng = LENGTH(gWallDataNames)-1;
				char* label = NULL;
				
				if (!irngok(wallType->value, 0, maxRng) || (label = gWallDataNames[wallType->value][0]) == NULL)
				{
					label = pItem->formatLabel;
					if (!focus)
						fc = clr2std(kColorCyan);
				}

				sprintf(buffer, "%-0.11s: %-5d", label, pItem->value);
				
			}
			else if (pDlg == dlgXSector && pItem->tabGroup == kSectData)
			{
				// get sector type
				DIALOG_ITEM *sectType = pDlg;
				int maxRng = LENGTH(gSectorDataNames)-1;
				char* label = NULL;
				
				if (!irngok(sectType->value, 0, maxRng) || (label = gSectorDataNames[sectType->value][0]) == NULL)
				{
					label = pItem->formatLabel;
					if (!focus)
						fc = clr2std(kColorCyan);
				}

				sprintf(buffer, "%-0.11s: %-5d", label, pItem->value);
			}
			else
			{
				sprintf(buffer, pItem->formatLabel, pItem->value);
			}
			SetLabel(pItem, buffer);
			PrintText(pItem, fc, bc);
			break;
		case LIST:
			SetLabel(pItem, pItem->formatLabel, pItem->value,
				(pItem->names[pItem->value]) ? pItem->names[pItem->value] : "<unnamed>");
			PrintText(pItem, fc, bc);
			break;
		case CHECKBOX:
		case ELT_SELECTOR:
			DrawCheckbox(pItem, fc, bc);
			break;
		default:
			PrintText(pItem, fc, bc);
			break;
	}
}

char DIALOG_HANDLER::Edit()
{
	int x1, x2, x3, x4, y1, y2, y3, y4, wh, hg;
	char tmp[16], upd = true, *label;
	BYTE key, ctrl, shift, alt;
	DIALOG_ITEM *pItem, *pCur;
	
	int nVal, i;
	RESHANDLE hFile;
	MOUSE mouse;
	keyClear();

	if (editStatus++ < 1)
	{
		// started editing
		if (pHints == NULL) // load hints for edit dialog
		{
			if ((hFile = gGuiRes.Lookup(kIniEditDialogHints, "INI")) != NULL)
				pHints = new IniFile((BYTE*)gGuiRes.Load(hFile), hFile->size);
		}
		
		gfxFillBoxTrans(0, 0, xdim, ydim, clr2std(kColorCyan), kRSTransluc2);
	}
	
	
	pHud->content.GetCoords(&x1, &y1, &x2, &y2);
	
	mouse.ChangeCursor(kBitmapMouseCursor);
	wh = mouse.cursor.width; hg = mouse.cursor.height >> 1;
	mouse.RangeSet(x1, y1, x2+wh, y2+hg);
	mouse.VelocitySet(40, 35, false);
	mouse.wheelDelay = 14;
	
	// focus on first item
	pCur = FirstItem();
	dassert(pCur != NULL);
	
	mouse.X = x1 + (pCur->x * pFont->ls);
	mouse.Y = y1 + (pCur->y * pFont->lh);
	
	while ( 1 )
	{
		if (upd)
		{	
			pHud->DrawIt();
			
			Paint();
			Paint(pCur, true);
			mouse.Draw();
		}
		
		mouse.Read();
		keyGetHelper(&key, &ctrl, &shift, &alt);
		handleevents();
		updateClocks();
		showframe();
		
		upd = (key || mouse.buttons || mouse.dX2 || mouse.dY2);
		if (!upd)
			continue;
		
		if ((mouse.dY2 || mouse.dX2) && !keystatus[KEY_Q])
		{
			// find item to focus with mouse
			for (pItem = pDlg; pItem->type != CONTROL_END; pItem++)
			{
				if (CanAccess(pItem) && GetItemCoords(pItem, &x1, &y1, &x2, &y2))
				{
					if (irngok(mouse.X, x1, x2) && irngok(mouse.Y, y1, y2))
					{
						pCur = pItem;
						break;
					}
				}
			}
		}
		
		if (key)
		{
			if (pCur->pHelpFunc)
				key = pCur->pHelpFunc(pDlg, pCur, key);
		}
		else  if (mouse.buttons) // convert mouse buttons in keyboard scans
		{
			if (mouse.wheel && keystatus[KEY_Q])
			{
				key = (mouse.wheel < 0) ? KEY_LEFT : KEY_RIGHT;
				mouse.wheel = 0;
			}

			if ((mouse.press & 4) && pCur->pHelpFunc)
			{
				key = pCur->pHelpFunc(pDlg, pCur, KEY_F10);
				continue;
			}
		
			if (mouse.press & 2)
			{
				pCur->value = 0;
			}
			
			switch (pCur->type)
			{
				case CHECKBOX:
				case ELT_SELECTOR:
					if (mouse.press & 1) key = KEY_SPACE;
					break;
				case DIALOG:
					if (!(mouse.press & 1)) break;
					pCur->pHelpFunc(pDlg, pCur, KEY_ENTER);
					continue;
				case LIST:
				case NUMBER:
					if (mouse.wheel == -1)
					{
						if (ctrl || alt) key = KEY_PAGEUP;
						else if (shift) key = KEY_PADPLUS;
						else if (pCur->pHelpFunc != helperAuditSound) key = KEY_UP;
						else pCur->pHelpFunc(pDlg, pCur, KEY_UP); // force skip non-existing sounds
					}
					else if (mouse.wheel == 1)
					{
						if (ctrl || alt) key = KEY_PAGEDN;
						else if (shift) key = KEY_PADMINUS;
						else if (pCur->pHelpFunc != helperAuditSound) key = KEY_DOWN;
						else pCur->pHelpFunc(pDlg, pCur, KEY_DOWN); // force skip non-existing sounds
					}
					break;
			}
		}
		
		nVal = pCur->value;
		
		switch (key)
		{
			case KEY_ENTER:
			case KEY_PADENTER:
				BeepOk();
				// no break
			case KEY_ESC:
				keystatus[key] = 0;
				sndKillAllSounds();
				if (--editStatus <= 0 && pHints)
					DELETE_AND_NULL(pHints);
				return (key == KEY_ESC) ? 0 : 1;
			case KEY_TAB:
			case KEY_RIGHT:
				if (!shift)
				{
					if ((pCur = NextItem(pCur)) == NULL) pCur = FirstItem();
					continue;
				}
				// no break
			case KEY_LEFT:
				if ((pCur = PrevItem(pCur)) == NULL) pCur = LastItem();
				continue;
		}

		switch (pCur->type)
		{
			case NUMBER:
			case LIST:
				switch (key)
				{
					case KEY_DELETE:
					case KEY_SPACE:
						nVal = 0;
						break;
					case KEY_PAGEUP:
					case KEY_PADPLUS:
					case KEY_UP:
						if (pCur->type == NUMBER)
						{
							if (key == KEY_PAGEUP)
							{
								if (ctrl) nVal = pCur->maxValue;
								else nVal = IncBy(nVal, 10);
							}
							else
							{
								nVal++;
							}
							
							break;
						}
						else if (!ctrl)
						{
							do
							{
								nVal = IncBy(nVal, (key == KEY_PAGEUP) ? 5 : 1);
								nVal = ClipHigh(nVal, pCur->maxValue);
							}
							while (nVal < pCur->maxValue && pCur->names[nVal] == NULL);
							while (nVal >= pCur->minValue && pCur->names[nVal] == NULL)
								nVal--;
						}
						else
						{
							nVal = pCur->maxValue;
							while (nVal >= pCur->minValue && !pCur->names[nVal]) nVal--;
						}
						break;
					case KEY_PAGEDN:
					case KEY_PADMINUS:
					case KEY_DOWN:
						if (pCur->type == NUMBER)
						{
							if (key == KEY_PAGEDN)
							{
								if (ctrl) nVal = pCur->minValue;
								else nVal = DecBy(nVal, 10);
							}
							else
							{
								nVal--;
							}
						}
						else if (!ctrl)
						{
							do
							{
								nVal = DecBy(nVal, (key == KEY_PAGEDN) ? 5 : 1);
								nVal = ClipLow(nVal, pCur->minValue);
							}
							while (nVal >= pCur->minValue && pCur->names[nVal] == NULL);
							while (nVal < pCur->maxValue && pCur->names[nVal] == NULL)
								nVal++;
						}
						else
						{
							nVal = pCur->minValue;
							while (nVal <= pCur->maxValue && pCur->names[nVal] == NULL) nVal++;
						}
						break;
					case KEY_MINUS:
						if (pCur->minValue >= 0) break;
						else if (!nVal) nVal = -1;
						sprintf(tmp, "%d", (nVal > 0) ? -nVal : nVal);
						nVal = atoi(tmp);
						break;
					case KEY_PLUS:
						if (pCur->maxValue < 0) break;
						sprintf(tmp, "%d", abs(nVal));
						nVal = atoi(tmp);
						break;
					case KEY_BACKSPACE:
						if ((i = sprintf(tmp, "%d", nVal)) > 1) tmp[i-1] = 0;
						else tmp[0] = '\0';
						nVal = atoi(tmp);
						break;
					default:
						if (irngok(key, KEY_1, KEY_0) || irngok(key, KEY_PAD7, KEY_PAD0))
						{
							i = 0;
							i += sprintf(tmp, "%d", nVal);
							if (irngok(pCur->maxValue, 0, 9))
								i = 0;
							
							sprintf(&tmp[i], "%c", keyAscii[key]);
							nVal = atoi(tmp);
						}
						break;
				}
				pCur->value = ClipRange(nVal, pCur->minValue, pCur->maxValue);
				break;
			case CHECKBOX:
			case ELT_SELECTOR:
				switch (key)
				{
					case KEY_SPACE:
					case KEY_BACKSPACE:
					case KEY_DELETE:
						if (pCur->type == CHECKBOX) pCur->value = !pCur->value;
						else pCur->selected = !pCur->selected;
						break;
				}
				break;
		}
	}
}

DIALOG_ITEM* DIALOG_HANDLER::NextItem(DIALOG_ITEM* pItem)
{
	if (pItem->type != CONTROL_END)
	{
		while(pItem->type != CONTROL_END)
		{
			pItem++;
			if (CanAccess(pItem))
				return pItem;
		}
	}
	
	return NULL;
}

DIALOG_ITEM* DIALOG_HANDLER::PrevItem(DIALOG_ITEM* pItem)
{
	if (pItem != pDlg)
	{
		while(pItem != pDlg)
		{
			pItem--;
			if (CanAccess(pItem))
				return pItem;
		}
	}
	
	return NULL;
}

DIALOG_ITEM* DIALOG_HANDLER::FirstItem()
{
	DIALOG_ITEM* pItem = pDlg;
	while(pItem->type != CONTROL_END)
	{
		if (CanAccess(pItem))
			return pItem;
		
		pItem++;
	}
	
	return NULL;
}

DIALOG_ITEM* DIALOG_HANDLER::LastItem(DIALOG_ITEM* pItem)
{
	DIALOG_ITEM* pRetn = NULL;
	
	if (!pItem)
		pItem = pDlg;
	
	while(pItem->type != CONTROL_END)
	{
		if (CanAccess(pItem))
			pRetn = pItem;
		
		pItem++;
	}
	
	return pRetn;
}

void DIALOG_HANDLER::DrawHints(DIALOG_ITEM* pItem)
{
	if (!pDlg || !pItem || !pHints)
		return;
	
	char buf1[256], buf2[32], buf3[32], typeHint, *tmp = NULL;
	ROMFONT* pFont = pHud->GetFont(kHudFontStat);
	DIALOG_INFO* pInfo = GetDialogInfo(pDlg);
	int x1, x2, y1, y2, wh, i, l = 0;
	char bg = clr2std(kColorGrey27);
	char fg = clr2std(kColorGrey17);
	
	if (!pInfo || !pInfo->pHintGroup)
		return;
	
	typeHint = (pInfo->objectTypeHintID == pItem->tabGroup);
	sprintf(buf3, "%d", (typeHint) ? pDlg->value : pItem->tabGroup - 1);
	sprintf(buf1, (typeHint) ? "Type" : "Dialog");
	sprintf(buf2, "Hints.%s.%s", buf1, pInfo->pHintGroup);
	
	if (isempty(tmp = pHints->GetKeyString(buf2, buf3, NULL)))
	{
		sprintf(buf2, "Hints.%s.Shared", buf1);
		tmp = pHints->GetKeyString(buf2, buf3, "<no information found>");
	}
	
	gfxSetColor(bg);
	pHud->bottom.GetCoords(&x1, &y1, &x2, &y2);
	gfxFillBox(x1+1, y1+1, x2-1, y2-1);
	wh = x2-x1;
	
	i = wh/pFont->ls;
	if (pItem->selected)
		l += sprintf(&buf1[l], "*SEL* - ");
	
	if ((l += sprintf(&buf1[l], "%0.255s", tmp)) >= i)
		buf1[i] = 0, l = i;
	
	pHud->CenterText(&x1, &y1, x2-x1, l, y2-y1, pFont);
	printextShadow(x1, y1, fg, buf1, pFont);
}

void DIALOG_HANDLER::DrawCheckbox(DIALOG_ITEM* pItem, char fc, short bc)
{
	int x1, y1, x2, y2, x3, y3, x4, y4, a;
	if (!GetItemCoords(pItem, &x1, &y1, &x2, &y2))
		return;
	
	if (bc >= 0)
	{
		gfxSetColor((char)bc);
		gfxFillBox(x1, y1, x2, y2);
	}
	
	if (pFont->ls != pFont->lh)
	{
		a = (pFont->ls+pFont->lh)>>1;
		y3 = y1+((pFont->lh>>1)-(a>>1));
		y4 = y3+a-2;
		
		x3 = x1+((pFont->ls>>1)-(a>>1));
		x4 = x3+a-2;
	}
	else
	{
		x3 = x1+1;
		y3 = y1+1;
		x4 = x1+pFont->ls-1;
		y4 = y1+pFont->lh-1;
	}
	
	gfxSetColor(fc);
	if (pItem->selected)
	{
		gfxSetColor(clr2std(kColorMagenta));
		gfxFillBox(x3, y3, x4, y4);
		
		gfxSetColor(clr2std(kColorWhite));
		gfxLine(x3, y3, x4, y4);
		gfxLine(x4, y3, x3, y4);
	}
	else
	{
		if (pItem->enabled)
		{
			gfxSetColor(clr2std(kColorGrey27));
			gfxFillBox(x3, y3, x4, y4);
		}
		
		if (pItem->value)
		{
			if (pItem->enabled)
				gfxSetColor(clr2std(kColorGrey18));
			
			gfxLine(x3, y3, x4, y4);
			gfxLine(x4, y3, x3, y4);
		}
	}
	
	gfxSetColor(fc);
	gfxRect(x3, y3, x4, y4);
	printextShadow(x1+(pFont->ls<<1), y1, fc, pItem->readyLabel, pFont);
}

char DIALOG_HANDLER::GetItemCoords(DIALOG_ITEM* pItem, int* x1, int* y1, int* x2, int* y2)
{
	if (pItem->readyLabel)
	{
		*x1 = pHud->content.x1+(pItem->x * pFont->ls);
		*y1 = pHud->content.y1+(pItem->y * pFont->lh);
		*x2 = *x1 + (pItem->readyLabelLen * pFont->ls);
		*y2 = *y1 + pFont->lh;
		
		switch(pItem->type)
		{
			case CHECKBOX:
			case ELT_SELECTOR:
				*x2 = *x2 + pFont->ls;
				if (pItem->readyLabelLen)
					*x2 = *x2 + pFont->ls;
				break;
		}
		
		return 1;
	}
	
	return 0;
}

void DIALOG_HANDLER::PrintText(DIALOG_ITEM* pItem, char fc, short bc)
{
	int x1, y1, x2, y2;
	if (bc < 0)
	{
		x1 = pHud->content.x1+(pItem->x*pFont->ls);
		y1 = pHud->content.y1+(pItem->y*pFont->lh);
	}
	else
	{
		if (!GetItemCoords(pItem, &x1, &y1, &x2, &y2))
			return;
		
		gfxSetColor((char)bc);
		gfxFillBox(x1, y1, x2, y2);
	}

	printextShadow(x1, y1, fc, pItem->readyLabel, pFont);
}

void DIALOG_HANDLER::SetLabel(DIALOG_ITEM* pItem, char *__format, ...)
{
	if (pItem->readyLabel == pItem->formatLabel
		|| (!pItem->readyLabel && (pItem->readyLabel = (char*)malloc(128)) == NULL))
				return;
			
	va_list argptr;
	va_start(argptr, __format);
	pItem->readyLabelLen = vsprintf(pItem->readyLabel, __format, argptr);
	va_end(argptr);
}

void DIALOG_HANDLER::SetValue(int nGroup, int nValue)
{
	DIALOG_ITEM* pItem = pDlg;
	while(pItem->tabGroup != nGroup && pItem->type != CONTROL_END)
		pItem++;
	
	pItem->value = nValue;
}

int DIALOG_HANDLER::GetValue(int nGroup)
{
	DIALOG_ITEM* pItem = pDlg;
	while(pItem->tabGroup != nGroup && pItem->type != CONTROL_END)
		pItem++;
	
	return pItem->value;
}

void ShowSectorData(int nSector, BOOL xFirst, BOOL showDialog)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSect =& sector[nSector];
	char buf[256], *pBuf = buf;
	int i;
	
	pBuf += sprintf(pBuf, "Sector #%d: Height = %d, Area = %d", nSector, klabs(pSect->floorz-pSect->ceilingz), AreaOfSector(pSect));
	if (pSect->alignto)
		pBuf += sprintf(pBuf, ", Auto-align to wall = #%d", pSect->wallptr + pSect->alignto);
	
	if (pSect->extra > 0 && xFirst)
	{
		if (showDialog)
		{
			if (testXSectorForLighting(pSect->extra))
			{
				DIALOG_HANDLER dialog(&gMapedHud, dlgXSectorFX);
				dlgXSectorFXToDialog(&dialog, nSector);
				dialog.Paint();
			}
			else
			{
				DIALOG_HANDLER dialog(&gMapedHud, dlgXSector);
				dlgXSectorToDialog(&dialog, nSector);
				dialog.Paint();
			}
		}
		
		pBuf += sprintf(pBuf, ", Extra = %d, XRef = %d", pSect->extra, xsector[pSect->extra].reference);
	}
	else if (showDialog)
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgSector);
		dlgSectorToDialog(&dialog, nSector);
		dialog.Paint();
	}

	if (searchstat == OBJ_FLOOR)
		gMapedHud.SetTile(pSect->floorpicnum, pSect->floorpal, pSect->floorshade);
	else
		gMapedHud.SetTile(pSect->ceilingpicnum, pSect->ceilingpal, pSect->ceilingshade);
	
	gMapedHud.SetMsg(buf);
}

void ShowWallData(int nWall, BOOL xFirst, BOOL showDialog)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =& wall[nWall];
	int nSect = sectorofwall(nWall);
	char buf[256], *pBuf = buf;
	double nLen, t;
	int nAng, i;
	
	int x1, y1, x2, y2;
	getWallCoords(nWall, &x1, &y1, &x2, &y2);
	
	if (showDialog)
	{
		if (pWall->extra > 0 && xFirst)
		{
			DIALOG_HANDLER dialog(&gMapedHud, dlgXWall);
			dlgXWallToDialog(&dialog, nWall);
			dialog.Paint();
		}
		else
		{
			DIALOG_HANDLER dialog(&gMapedHud, dlgWall);
			dlgWallToDialog(&dialog, nWall);
			dialog.Paint();
		}
	}
		
	if (pWall->cstat & kWallOneWay)		 	pBuf += sprintf(pBuf, "One way wall");
	else if (pWall->cstat & kWallMasked)	pBuf += sprintf(pBuf, "Masked wall");
	else if (pWall->nextwall < 0) 		 	pBuf += sprintf(pBuf, "Solid wall");
	else 								 	pBuf += sprintf(pBuf, "Wall");
	
	nLen = exactDist(x2 - x1, y2 - y1);
	pBuf += sprintf(pBuf, " #%d: Length = %d", nWall, (int)nLen);
	if (grid)
	{
		double nGrid = (double)(nLen / (2048>>grid));
		if (modf(nGrid, &t))
		{
			pBuf += sprintf(pBuf, " (%2.1fG)", nGrid);
		}
		else
		{
			pBuf += sprintf(pBuf, " (%dG)", (int)nGrid);
		}
	}
	
	pBuf += sprintf(pBuf, ", Angle = %d", getangle(x2 - x1, y2 - y1) & kAngMask);
	
	if (pWall->extra > 0)
		pBuf += sprintf(pBuf, ", Extra = %d, XRef = %d", pWall->extra, xwall[pWall->extra].reference);

	sectortype* pSect = &sector[nSect];
	if (pSect->wallptr == nWall) pBuf += sprintf(pBuf, " [FIRST]");
	if (pSect->alignto && nWall - pSect->wallptr == pSect->alignto)
		pBuf += sprintf(pBuf, " [AUTO-ALIGN]");
	
	
	gMapedHud.SetTile((searchstat == OBJ_MASKED) ? pWall->overpicnum : pWall->picnum, pWall->pal, pWall->shade);
	gMapedHud.SetMsg(buf);
}

void ShowMapInfo(BOOL showDialog)
{
	gMapedHud.SetMsg(gMapInfoStr);
	if (!showDialog)
		return;

	char buf[64], *pBuf;
	int x1, y1, x2, y2, x3, y3, x4, y4, x, y;
	int nLongest = 0, i = LENGTH(gMapStatsNames), t;
	int nStat, nSkill, nPerc, nMode = gPreview.mode;
	int nCurVal, nTotVal;
	
	const char kColorNormalText 	= clr2std(kColorLightCyan);
	const char kColorHeaderText 	= clr2std(kColorWhite);
	const char kColorHeaderBack 	= clr2std(kColorGreen);
	const char kColorProgRectLine	= clr2std(kColorCyan);
	const char kColorProgPercText	= clr2std(19);
	const char kColorProgTextZero	= clr2std(20);
	const char kColorProgPercFull	= clr2std(kColorBlue);
	const char kColorProgPerc		= clr2std(kColorMagenta);
	
	
	gMapedHud.content.GetCoords(&x1, &y1, &x2, &y2);
	ROMFONT* pFont = gMapedHud.GetFont(kHudFontCont);
	int fw = pFont->ls, fh = pFont->lh;
	int nProgWidth = fw * 6;
	int nStep = fw * 13;
	
	while(--i >= 0) // get longest string
	{
		if ((t = strlen(gMapStatsNames[i])) > nLongest)
			nLongest = t;
	}
	
	t = sprintf(buf, "TOTAL SPRITES:")*fw;
	
	gfxSetColor(kColorHeaderBack);
	gfxFillBox(x1, y1, x1+t, y1+fh);
	printextShadow(x1, y1, kColorHeaderText, buf, pFont);
	
	y = y1+fh;
	for (i = 0; i < kMapStatMax; i++)
	{
		pBuf = buf;
		pBuf += sprintf(pBuf, "%s", gMapStatsNames[i]);
		t = nLongest-strlen(gMapStatsNames[i]);
		while(--t >= 0)
			*pBuf = '.', pBuf++;
		
		sprintf(pBuf, ".:%03d", ClipHigh(MapStats::Get(nMode, i), 999));
		printextShadow(x1, y, kColorNormalText, buf, pFont);
		y+=fh;
	}
	
	x = x1+(fw<<4), y = y1 + fh;
	for (nSkill = 0; nSkill < 5; nSkill++, x+=nStep, y = y1 + fh)
	{
		pBuf = buf;
		if (gPreview.difficulty >= 5 || nSkill == gPreview.difficulty)
			*pBuf = '*', pBuf++; // mark current difficulty
		
		sprintf(pBuf, "SKL%d", nSkill + 1);
		
		t = strlen(buf)*fw;
		gfxSetColor(kColorHeaderBack);
		gfxFillBox(x, y1, x + t, y1 + fh);
		printextShadow(x, y1, kColorHeaderText, buf, pFont);
		
		for (nStat = 0; nStat < kMapStatMax; nStat++)
		{
			x3 = x;		x4 = x3 + nProgWidth;
			y3 = y;		y4 = y3 + fh;

			gfxSetColor(kColorProgRectLine);
			gfxRect(x3, y3, x4, y4);
			
			nTotVal = MapStats::Get(nMode, nStat);
			nCurVal = MapStats::Get(nMode, nSkill, nStat);
			if ((nPerc = (100 * nCurVal)/ClipLow(nTotVal, 1)) > 0)
			{
				gfxSetColor((nPerc >= 100) ? kColorProgPercFull : kColorProgPerc);
				t = x3 + perc2val(nPerc, nProgWidth) - 1;
				while(t > x3)
					gfxVLine(t, y3+1, y4-1), t-=3;
				
				t = sprintf(buf, "%d%%", nPerc)*fw;
				printextShadow(x3 + ((nProgWidth>>1) - (t>>1)), y, kColorProgPercText, buf, pFont);
			}
			else
			{
				gfxSetColor(clr2std(30));
				
				t = x4 - 1;
				while(t > x3)
					gfxVLine(t, y3+1, y4-1), t-=3;
				
				t = sprintf(buf, "---")*fw;
				printextShadow(x3 + ((nProgWidth>>1) - (t>>1)), y, kColorProgTextZero, buf, pFont);
			}
			
			t = sprintf(buf, "%03d", ClipHigh(nCurVal, 999));
			printextShadow(x4+4, y, kColorNormalText, buf, pFont);
			y+=fh;
		}
	}
	
	i = LENGTH(gGameNames);
	while(--i >= 0)
	{
		if (gGameNames[i].id == nMode)
		{
			t = sprintf(buf, "Showing statistics for game mode: %s", gGameNames[i].name)*fw;
			printextShadow(x1+((x2-x1>>1)) - (t>>1), y2-fh, kColorNormalText, buf, pFont);
			break;
		}
	}
}

void ShowSpriteData(int nSprite, BOOL xFirst, BOOL showDialog)
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	spritetype* pSpr =& sprite[nSprite];
	char buf[256], *pBuf = buf;
	int nComment;
	
	if (isMarkerSprite(nSprite))
	{
		pBuf += sprintf(pBuf, "Marker #%d: ", nSprite);
		if (pSpr->ang)
		{
			pBuf += sprintf(pBuf, "rotate %s", (pSpr->ang < 0) ? "counter clockwise" : "clockwise");
			double nTimes = (double)(klabs(pSpr->ang))/kAng360;
			if (nTimes >= 1)
				pBuf += sprintf(pBuf, " around %2.1f times", nTimes);
		}
		else
		{
			pBuf += sprintf(pBuf, "no rotation");
		}
	}
	else
	{
		pBuf += sprintf(pBuf, "Sprite #%d: Statnum = %d", nSprite, pSpr->statnum);
	}
	
	if (pSpr->extra > 0 && xFirst)
	{
		if (showDialog)
		{
			DIALOG_HANDLER dialog(&gMapedHud, dlgXSprite);
			dlgXSpriteToDialog(&dialog, nSprite);
			dialog.Paint();
		}
		
		pBuf += sprintf(pBuf, ", Extra = %d, XRef = %d", pSpr->extra, xsprite[pSpr->extra].reference);
	}
	else if (showDialog)
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgSprite);
		dlgSpriteToDialog(&dialog, nSprite);
		dialog.Paint();
	}
	
	gMapedHud.SetTile(pSpr->picnum, pSpr->pal, pSpr->shade);
	gMapedHud.SetMsg(buf);
}

void EditSectorData(int nSector, BOOL xFirst)
{
	ShowSectorData(nSector, xFirst);
	if (xFirst)
	{
		GetXSector(nSector);
		
		DIALOG_HANDLER dialog(&gMapedHud, dlgXSector);
		dlgXSectorToDialog(&dialog, nSector);
		if (dialog.Edit())
			dlgDialogToXSector(&dialog, nSector);
	}
	else
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgSector);
		dlgSectorToDialog(&dialog, nSector);
		if (dialog.Edit())
			dlgDialogToSector(&dialog, nSector);
	}

	CleanUp();
	vel = svel = angvel = 0;
	ShowSectorData(nSector, TRUE);
}

void EditSectorLighting(int nSector)
{
	ShowSectorData(nSector, TRUE);
	GetXSector(nSector);

	DIALOG_HANDLER dialog(&gMapedHud, dlgXSectorFX);
	dlgXSectorFXToDialog(&dialog, nSector);
	if (dialog.Edit())
		dlgDialogToXSectorFX(&dialog, nSector);

	CleanUp();
	vel = svel = angvel = 0;
	ShowSectorData(nSector, TRUE);
}

void EditWallData(int nWall, BOOL xFirst)
{
	ShowWallData(nWall, xFirst);
	if (xFirst)
	{
		GetXWall(nWall);
		
		DIALOG_HANDLER dialog(&gMapedHud, dlgXWall);
		dlgXWallToDialog(&dialog, nWall);
		if (dialog.Edit())
			dlgDialogToXWall(&dialog, nWall);
	}
	else
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgWall);
		dlgWallToDialog(&dialog, nWall);
		if (dialog.Edit())
			dlgDialogToWall(&dialog, nWall);
	}
	
	CleanUp();
	vel = svel = angvel = 0;
	ShowWallData(nWall, TRUE);
}

void EditSpriteData(int nSprite, BOOL xFirst)
{
	ShowSpriteData(nSprite, xFirst);
	if (xFirst)
	{
		GetXSprite(nSprite);
		
		DIALOG_HANDLER dialog(&gMapedHud, dlgXSprite);
		dlgXSpriteToDialog(&dialog, nSprite);
		if (dialog.Edit())
			dlgDialogToXSprite(&dialog, nSprite);
	}
	else
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgSprite);
		dlgSpriteToDialog(&dialog, nSprite);
		if (dialog.Edit())
			dlgDialogToSprite(&dialog, nSprite);
	}
	
	CleanUp();
	AutoAdjustSprites();
	vel = svel = angvel = 0;
	ShowSpriteData(nSprite, xFirst);
}

char helperGetNextUnusedID(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key )
{
	int i;
	switch (key) {
		case KEY_F10:
			if ((i = findUnusedChannel(dialog)) == -1) return key;
			control->value = i;
			return 0;
	}

	return key;
}

char helperDoSectorFXDialog(DIALOG_ITEM*, DIALOG_ITEM*, BYTE key)
{
	if (key == KEY_ENTER)
	{
		DIALOG_HANDLER dialog(&gMapedHud, dlgXSectorFX);
		dialog.Edit();
		return 0;
	}
	
	return key;
}

char helperAuditSound(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key)
{
	int i = 0;
	switch ( key )
	{
		case KEY_UP:
			for (i = control->value+1; i < 65535; i++)
			{
				if (gSoundRes.Lookup(i, "SFX") != NULL)
				{
					control->value = i;
					break;
				}
			}
			return 0;
		case KEY_DOWN:
			if (control->value == 1)
			{
				control->value = 0;
				return 0;
			}
			for (i = control->value-1; i > 0; i--)
			{
				if (gSoundRes.Lookup(i, "SFX") != NULL)
				{
					control->value = i;
					break;
				}
			}
			return 0;
		case KEY_F10:
			if (dialog == dlgXWall)
			{
				switch (dialog->value)
				{
					case kWallGib:
						if (control->tabGroup != kWallData) break;
						i = toolGibTool(control->value, OBJ_WALL);
						if (i < 0)
							return key;

						control->value = i;
						return 0;
					case kWallStack:
						if (control->tabGroup != kWallData) break;
						else if ((i = findUnusedStack()) >= 0) control->value = i;
						return 0;
				}
			}
			else if (dialog == dlgXSprite)
			{
				switch (dialog->value)
				{
					case kMarkerDudeSpawn:
						if (control->tabGroup < kSprDialogData1 || control->tabGroup > kSprDialogData4) break;
						helperPickEnemyTile(dialog, control, key);
						return 0;
					case kMarkerUpLink:
					case kMarkerUpWater:
					case kMarkerUpStack:
					case kMarkerUpGoo:
					case kMarkerLowLink:
					case kMarkerLowWater:
					case kMarkerLowStack:
					case kMarkerLowGoo:
						if (control->tabGroup != kSprDialogData1) break;
						else if ((i = findUnusedStack()) >= 0) control->value = i;
						return 0;
					case kMarkerPath:
						if (control->tabGroup < kSprDialogData1 || control->tabGroup > kSprDialogData2) break;
						else if ((i = findUnusedPath(dlgXSprite, control)) < 0) return key;
						control->value = i;
						return 0;
					case kModernRandomTX:
					case kModernSequentialTX:
						if (control->tabGroup < kSprDialogData1 || control->tabGroup > kSprDialogData4) break;
						else if ((i = findUnusedChannel(dlgXSprite)) == -1) return key;
						control->value = i;
						return 0;
					case kThingObjectGib:
					case kThingObjectExplode:
						if (control->tabGroup < kSprDialogData1 || control->tabGroup > kSprDialogData3) break;
						i = toolGibTool(control->value, OBJ_SPRITE);
						if (i < 0)
							return key;

						control->value = i;
						return 0;
					case kGenSound:
						for (DIALOG_ITEM *data4 = dialog; data4->type != CONTROL_END; data4++)
						{
							if (data4->tabGroup != kSprDialogData4) continue;
							i = data4->value;
							break;
						}
						break;
				}
			}


			playSound(control->value);
			return 0;
	}

	return key;
}

int helperPickTypeHelper(int nGroup, char* title)
{
	int retn = -1;
	scrSave();
	
	if (adjFillTilesArray(nGroup))
	{
		int i, j;
		if ((j = tilePick(-1, -1, OBJ_CUSTOM, title)) >= 0)
		{
			for (i = 0; i < autoDataLength; i++)
			{
				if (autoData[i].picnum != j) continue;
				retn = autoData[i].type;
				break;
			}
		}
	}
	
	scrRestore();
	return retn;
}

char helperPickEnemyTile(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key)
{
	int value;
	if (key == KEY_F10)
	{
		if ((value = helperPickTypeHelper(kOGrpDude, "Select enemy to spawn")) >= 0)
			control->value = value;
	}
	
	return key;
}

char helperPickItemTile(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key)
{
	int value;
	if (key == KEY_F10)
	{
		if ((value = helperPickTypeHelper(kOGrpWeapon | kOGrpAmmo | kOGrpAmmoMix | kOGrpItem, "Select item")) >= 0)
			control->value = value;
	}
	
	return key;
}

char helperPickIniMessage(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key)
{
	if (key != KEY_F10)
		return key;

	char messages[kMaxMessages][kMaxMessageLength], tmpbuf[BMAX_PATH];
	NAMED_TYPE iniMessages[kMaxMessages];
	char *pName, *pKey, *pVal;
	IniFile* pEpisode = NULL;
	int nID, nPrevNode;
	int i, n;

	sprintf(tmpbuf, gPaths.episodeIni);
	while ((pName = browseOpenFS(tmpbuf, ".INI")) != NULL)
	{
		if (pEpisode)
			delete(pEpisode);
		
		pEpisode = new IniFile(pName);
		if (!pEpisode->SectionExists("Episode1"))
		{
			getFilename(pName, buffer);
			Alert("%s is not an episode INI.", buffer);
			continue;
		}

		getFilename(gPaths.maps, buffer);
		if (!pEpisode->SectionExists(buffer))
		{
			Alert("No map section %s found!", buffer);
			continue;
		}

		nPrevNode = -1, i = 0;
		while(pEpisode->GetNextString(&pKey, &pVal, &nPrevNode, buffer))
		{
			if (pVal && isIdKeyword(pKey, "Message", &nID))
			{
				n = strlen(pVal);
				sprintf(messages[i], "%0.60s", (n) ? pVal : "<empty>");
				if (n >= 60)
					strcat(messages[i], "...");
				
				iniMessages[i].name = messages[i];
				iniMessages[i].id = nID;
				i++;
			}
		}
		
		if (i > 0)
		{
			if ((i = showButtons(iniMessages, i, "Pick message")) >= mrUser)
			{
				sprintf(gPaths.episodeIni, tmpbuf);
				if (gPreview.pEpisode)
					delete(gPreview.pEpisode);
				
				gPreview.pEpisode = new IniFile(gPaths.episodeIni);
				control->value = i - mrUser;
				break;
			}
		}
		else
		{
			Alert("No messages found in %s.", tmpbuf);
		}
	}
	
	if (pEpisode)
		delete(pEpisode);
	
	return key;

}

void dlgXWallToDialog(DIALOG_HANDLER* pHandle, int nWall)
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	XWALL *pXWall = &xwall[nXWall];

	pHandle->SetValue(kWallType,		wall[nWall].type);
	pHandle->SetValue(kWallRX,			pXWall->rxID);
	pHandle->SetValue(kWallTX,			pXWall->txID);
	pHandle->SetValue(kWallState,		pXWall->state);
	pHandle->SetValue(kWallCmd,			pXWall->command);

	pHandle->SetValue(kWallGoingOn,		pXWall->triggerOn);
	pHandle->SetValue(kWallGoingOff,	pXWall->triggerOff);
	pHandle->SetValue(kWallBusyTime,	pXWall->busyTime);
	pHandle->SetValue(kWallWaitTime,	pXWall->waitTime);
	pHandle->SetValue(kWallRestState,	pXWall->restState);

	pHandle->SetValue(kWallTrPush,		pXWall->triggerPush);
	pHandle->SetValue(kWallTrVector,	pXWall->triggerVector);
	pHandle->SetValue(kWallTrTouch,		pXWall->triggerTouch);

	pHandle->SetValue(kWallKey,			pXWall->key);
	pHandle->SetValue(kWallPanX,		pXWall->panXVel);
	pHandle->SetValue(kWallPanY,		pXWall->panYVel);
	pHandle->SetValue(kWallPanAlways,	pXWall->panAlways);

	pHandle->SetValue(kWallDecoupled,	pXWall->decoupled);
	pHandle->SetValue(kWallOnce,		pXWall->triggerOnce);
	pHandle->SetValue(kWallLocked,		pXWall->locked);
	pHandle->SetValue(kWallInterrupt,	pXWall->interruptable);
	pHandle->SetValue(kWallLockout,		pXWall->dudeLockout);

	pHandle->SetValue(kWallData,		pXWall->data);

}

void dlgDialogToXWall(DIALOG_HANDLER* pHandle, int nWall)
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	XWALL *pXWall = &xwall[nXWall];

	wall[nWall].type			= pHandle->GetValue(kWallType);
	pXWall->rxID				= pHandle->GetValue(kWallRX);
	pXWall->txID				= pHandle->GetValue(kWallTX);

	pXWall->state				= pHandle->GetValue(kWallState);
	pXWall->command				= pHandle->GetValue(kWallCmd);

	pXWall->triggerOn			= pHandle->GetValue(kWallGoingOn);
	pXWall->triggerOff			= pHandle->GetValue(kWallGoingOff);
	pXWall->busyTime			= pHandle->GetValue(kWallBusyTime);
	pXWall->waitTime			= pHandle->GetValue(kWallWaitTime);
	pXWall->restState			= pHandle->GetValue(kWallRestState);

	pXWall->triggerPush			= pHandle->GetValue(kWallTrPush);
	pXWall->triggerVector		= pHandle->GetValue(kWallTrVector);
	pXWall->triggerTouch		= pHandle->GetValue(kWallTrTouch);

	pXWall->key					= pHandle->GetValue(kWallKey);
	pXWall->panXVel				= pHandle->GetValue(kWallPanX);
	pXWall->panYVel				= pHandle->GetValue(kWallPanY);
	pXWall->panAlways			= pHandle->GetValue(kWallPanAlways);

	pXWall->decoupled			= pHandle->GetValue(kWallDecoupled);
	pXWall->triggerOnce			= pHandle->GetValue(kWallOnce);
	pXWall->locked				= pHandle->GetValue(kWallLocked);
	pXWall->interruptable		= pHandle->GetValue(kWallInterrupt);
	pXWall->dudeLockout			= pHandle->GetValue(kWallLockout);

	pXWall->data				= pHandle->GetValue(kWallData);
}

void dlgWallToDialog(DIALOG_HANDLER* pHandle, int nWall)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =&wall[nWall];

	pHandle->SetValue(1, pWall->x);
	pHandle->SetValue(2, pWall->y);
	pHandle->SetValue(3, pWall->point2);
	pHandle->SetValue(4, sectorofwall(nWall));
	pHandle->SetValue(5, pWall->nextwall);
	pHandle->SetValue(6, pWall->nextsector);
	pHandle->SetValue(7, pWall->hitag);
	pHandle->SetValue(8, pWall->lotag);
	pHandle->SetValue(9, pWall->extra);
	
	pHandle->SetValue(10, pWall->picnum);
	pHandle->SetValue(11, pWall->overpicnum);
	pHandle->SetValue(12, pWall->shade);
	pHandle->SetValue(13, pWall->pal);
	pHandle->SetValue(14, pWall->xrepeat);
	pHandle->SetValue(15, pWall->yrepeat);
	pHandle->SetValue(16, pWall->xpanning);
	pHandle->SetValue(17, pWall->ypanning);
	
/* 	pHandle->SetValue(18, pWall->cstat & kWallBlock);
	pHandle->SetValue(19, pWall->cstat & kWallSwap);
	pHandle->SetValue(20, pWall->cstat & kWallOrgBottom);
	pHandle->SetValue(21, pWall->cstat & kWallFlipX);
	pHandle->SetValue(22, pWall->cstat & kWallFlipY);
	pHandle->SetValue(23, pWall->cstat & kWallMasked);
	pHandle->SetValue(24, pWall->cstat & kWallOneWay);
	pHandle->SetValue(25, pWall->cstat & kWallHitscan);
	pHandle->SetValue(26, pWall->cstat & kWallTransluc2);
	pHandle->SetValue(27, pWall->cstat & kWallTranslucR);
	pHandle->SetValue(28, pWall->cstat & kWallMoveForward);
	pHandle->SetValue(29, pWall->cstat & kWallMoveReverse); */
		
	//pHandle->SetValue(30, GetWallAngle(nWall) & kAngMask);
	//pHandle->SetValue(31, getWallLength(nWall));
	//pHandle->SetValue(32, (pSect->floorz - pSect->ceilingz)>>8);


}

void dlgDialogToWall(DIALOG_HANDLER* pHandle, int nWall)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall =&wall[nWall];

	pWall->x 			= pHandle->GetValue(1);
	pWall->y 			= pHandle->GetValue(2);
	pWall->point2 		= pHandle->GetValue(3);
	// 4
	pWall->nextwall 	= pHandle->GetValue(5);
	pWall->nextwall 	= pHandle->GetValue(6);
	pWall->hitag 		= pHandle->GetValue(7);
	pWall->type 		= pHandle->GetValue(8);
	pWall->extra 		= pHandle->GetValue(9);
	
	pWall->picnum 		= pHandle->GetValue(10);
	pWall->overpicnum 	= pHandle->GetValue(11);
	pWall->shade 		= pHandle->GetValue(12);
	pWall->pal 			= pHandle->GetValue(13);
	pWall->xrepeat 		= pHandle->GetValue(14);
	pWall->yrepeat 		= pHandle->GetValue(15);
	pWall->xpanning 	= pHandle->GetValue(16);
	pWall->ypanning 	= pHandle->GetValue(17);
	
/* 	setCstat(pHandle->GetValue(18), &pWall->cstat, kWallBlock);
	setCstat(pHandle->GetValue(19), &pWall->cstat, kWallSwap);
	setCstat(pHandle->GetValue(20), &pWall->cstat, kWallOrgBottom);
	setCstat(pHandle->GetValue(21), &pWall->cstat, kWallFlipX);
	setCstat(pHandle->GetValue(22), &pWall->cstat, kWallFlipY);
	setCstat(pHandle->GetValue(23), &pWall->cstat, kWallMasked);
	setCstat(pHandle->GetValue(24), &pWall->cstat, kWallOneWay);
	setCstat(pHandle->GetValue(25), &pWall->cstat, kWallHitscan);
	setCstat(pHandle->GetValue(26), &pWall->cstat, kWallTransluc);
	setCstat(pHandle->GetValue(27), &pWall->cstat, kWallTransluc2);
	setCstat(pHandle->GetValue(28), &pWall->cstat, kWallMoveForward);
	setCstat(pHandle->GetValue(29), &pWall->cstat, kWallMoveReverse); */
}

void dlgSectorToDialog(DIALOG_HANDLER* pHandle, int nSector)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSect =& sector[nSector];
	int i, cnt = 0;
		
	pHandle->SetValue(1, pSect->wallnum);
	pHandle->SetValue(2, pSect->wallptr);	
	for (i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
	{
		if (sprite[i].statnum >= kMaxStatus) continue;
		cnt++;
	}
	
	pHandle->SetValue(3, cnt);
	pHandle->SetValue(4, headspritesect[nSector]);
	pHandle->SetValue(5, pSect->hitag);
	pHandle->SetValue(6, pSect->lotag);
	pHandle->SetValue(7, pSect->visibility);
	pHandle->SetValue(8, pSect->extra);
	
	pHandle->SetValue(9, pSect->ceilingpicnum);
	pHandle->SetValue(10, pSect->ceilingshade);
	pHandle->SetValue(11, pSect->ceilingpal);
	pHandle->SetValue(12, pSect->ceilingxpanning);
	pHandle->SetValue(13, pSect->ceilingypanning);
	pHandle->SetValue(14, pSect->ceilingheinum);
	pHandle->SetValue(15, pSect->ceilingz);
	
	pHandle->SetValue(16, pSect->floorpicnum);
	pHandle->SetValue(17, pSect->floorshade);
	pHandle->SetValue(18, pSect->floorpal);
	pHandle->SetValue(19, pSect->floorxpanning);
	pHandle->SetValue(20, pSect->floorypanning);
	pHandle->SetValue(21, pSect->floorheinum);
	pHandle->SetValue(22, pSect->floorz);
	
/* 	pHandle->SetValue(23, pSect->ceilingstat  & kSectSloped);
	pHandle->SetValue(24, pSect->floorstat    & kSectSloped);
	
	pHandle->SetValue(25, pSect->ceilingstat  & kSectParallax);
	pHandle->SetValue(26, pSect->floorstat    & kSectParallax);
	
	pHandle->SetValue(27, pSect->ceilingstat  & kSectExpand);
	pHandle->SetValue(28, pSect->floorstat    & kSectExpand);
	
	pHandle->SetValue(29, pSect->ceilingstat  & kSectRelAlign);
	pHandle->SetValue(30, pSect->floorstat    & kSectRelAlign);
	
	pHandle->SetValue(31, pSect->ceilingstat  & kSectFlipX);
	pHandle->SetValue(32, pSect->floorstat    & kSectFlipX);
	
	pHandle->SetValue(33, pSect->ceilingstat  & kSectFlipY);
	pHandle->SetValue(34, pSect->floorstat    & kSectFlipY);
	
	BOOL floorshade = (!(pSect->ceilingstat & kSectParallax) || (pSect->ceilingstat & kSectShadeFloor));
	pHandle->SetValue(35, !floorshade);
	pHandle->SetValue(36,  floorshade);
	
	pHandle->SetValue(37, pSect->ceilingstat & kSectMasked);
	pHandle->SetValue(38, pSect->floorstat   & kSectMasked); */
}

void dlgDialogToSector(DIALOG_HANDLER* pHandle, int nSector)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	
	int i, s, e;
	getSectorWalls(nSector, &s, &e);
	sectortype* pSect =& sector[nSector];
	
	// 1
	i = pHandle->GetValue(2);
	if (s != i && irngok(i, s, e))
		setFirstWall(nSector, i);
	// 3
	// 4
	pSect->hitag					= pHandle->GetValue(5);
	pSect->lotag 					= pHandle->GetValue(6);
	pSect->visibility				= pHandle->GetValue(7);
	
	pSect->ceilingpicnum			= ClipHigh(pHandle->GetValue(9), kMaxTiles-1);
	pSect->ceilingshade				= pHandle->GetValue(10);
	pSect->ceilingpal				= pHandle->GetValue(11);
	pSect->ceilingxpanning			= pHandle->GetValue(12);
	pSect->ceilingypanning			= pHandle->GetValue(13);
	
	SetCeilingSlope(nSector,		pHandle->GetValue(14));
	SetCeilingZ(nSector, 			pHandle->GetValue(15));
	
	pSect->floorpicnum				= ClipHigh(pHandle->GetValue(16), kMaxTiles-1);
	pSect->floorshade				= pHandle->GetValue(17);
	pSect->floorpal					= pHandle->GetValue(18);
	pSect->floorxpanning			= pHandle->GetValue(19);
	pSect->floorypanning			= pHandle->GetValue(20);
	
	SetFloorSlope(nSector,			pHandle->GetValue(21));
	SetFloorZ(nSector,				pHandle->GetValue(22));

	
	/* i = kSectSloped;
	setCstat(pHandle->GetValue(23), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(24), &pSect->floorstat, i);
	
	if (!(pSect->ceilingstat & i)) SetCeilingSlope(nSector, 0);
	if (!(pSect->floorstat & i)) SetFloorSlope(nSector, 0);
	
	i = kSectParallax;
	setCstat(pHandle->GetValue(25), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(26), &pSect->floorstat, i);
	
	i = kSectExpand;
	setCstat(pHandle->GetValue(27), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(28), &pSect->floorstat, i);
	
	i = kSectRelAlign;
	setCstat(pHandle->GetValue(29), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(30), &pSect->floorstat, i);
	
	i = kSectFlipX;
	setCstat(pHandle->GetValue(31), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(32), &pSect->floorstat, i);
	
	i = kSectFlipY;
	setCstat(pHandle->GetValue(33), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(34), &pSect->floorstat, i);
	
	if (pSect->ceilingstat & kSectParallax)
	{
		// 35 for ceiling
		setCstat(pHandle->GetValue(36), &pSect->floorstat, kSectShadeFloor);
	}
	else
	{
		pSect->floorstat &= ~kSectShadeFloor;
	}
	
	i = kSectMasked;
	setCstat(pHandle->GetValue(37), &pSect->ceilingstat, i);
	setCstat(pHandle->GetValue(38), &pSect->floorstat, i); */
}


void dlgSpriteToDialog(DIALOG_HANDLER* pHandle, int nSprite)
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	spritetype* pSpr =&sprite[nSprite];
	
	pHandle->SetValue(1, pSpr->x);
	pHandle->SetValue(2, pSpr->y);
	pHandle->SetValue(3, pSpr->z);
	pHandle->SetValue(4, pSpr->sectnum);
	pHandle->SetValue(5, pSpr->statnum);
	pHandle->SetValue(6, pSpr->hitag);
	pHandle->SetValue(7, pSpr->lotag);
	pHandle->SetValue(8, pSpr->clipdist);
	pHandle->SetValue(9, pSpr->extra);
		
	pHandle->SetValue(10, pSpr->picnum);
	pHandle->SetValue(11, pSpr->shade);
	pHandle->SetValue(12, pSpr->pal);
	pHandle->SetValue(13, pSpr->xrepeat);
	pHandle->SetValue(14, pSpr->yrepeat);
	pHandle->SetValue(15, pSpr->xoffset);
	pHandle->SetValue(16, pSpr->yoffset);
	pHandle->SetValue(17, spriteGetSlope(nSprite));
	
	pHandle->SetValue(18, pSpr->ang);
	pHandle->SetValue(19, pSpr->xvel);
	pHandle->SetValue(20, pSpr->yvel);
	pHandle->SetValue(21, pSpr->zvel);
	pHandle->SetValue(22, pSpr->owner);
	
/* 	pHandle->SetValue(23, (pSpr->cstat & kSprRelMask) == kSprFloor);
	pHandle->SetValue(24, (pSpr->cstat & kSprRelMask) == kSprWall);
	pHandle->SetValue(25, (pSpr->cstat & kSprRelMask) == kSprVoxel);
	pHandle->SetValue(26, pSpr->cstat & kSprBlock);
	pHandle->SetValue(27, pSpr->cstat & kSprHitscan);
	pHandle->SetValue(28, pSpr->cstat & kSprOneSided);
	pHandle->SetValue(29, pSpr->cstat & kSprInvisible);
	pHandle->SetValue(30, pSpr->cstat & kSprOrigin);
	pHandle->SetValue(31, pSpr->cstat & kSprFlipX);
	pHandle->SetValue(32, pSpr->cstat & kSprFlipY);
	pHandle->SetValue(33, pSpr->cstat & kSprTransluc2);
	pHandle->SetValue(34, pSpr->cstat & kSprTranslucR);
	pHandle->SetValue(35, pSpr->cstat & kSprMoveForward);
	pHandle->SetValue(36, pSpr->cstat & kSprMoveReverse); */
}


void dlgXSpriteToDialog(DIALOG_HANDLER* pHandle, int nSprite)
{
	int nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	pHandle->SetValue(1, sprite[nSprite].type);
	pHandle->SetValue(2, pXSprite->rxID);
	pHandle->SetValue(3, pXSprite->txID);
	pHandle->SetValue(4, pXSprite->state);
	pHandle->SetValue(5, pXSprite->command);

	pHandle->SetValue(6, pXSprite->triggerOn);
	pHandle->SetValue(7, pXSprite->triggerOff);
	pHandle->SetValue(8, pXSprite->busyTime);
	pHandle->SetValue(9, pXSprite->waitTime);
	pHandle->SetValue(10, pXSprite->restState);

	pHandle->SetValue(11, pXSprite->triggerPush);
	pHandle->SetValue(12, pXSprite->triggerVector);
	pHandle->SetValue(13, pXSprite->triggerImpact);
	pHandle->SetValue(14, pXSprite->triggerPickup);
	pHandle->SetValue(15, pXSprite->triggerTouch);
	pHandle->SetValue(16, pXSprite->triggerProximity);
	pHandle->SetValue(17, pXSprite->triggerSight);

	pHandle->SetValue(18, (pXSprite->unused3 & 0x0001) ? 1 : 0);
	pHandle->SetValue(19, (pXSprite->unused3 & 0x0002) ? 1 : 0);

	//pHandle->SetValue(18, pXSprite->triggerReserved1);
	//pHandle->SetValue(19, pXSprite->triggerReserved2);

	pHandle->SetValue(20, !((pXSprite->lSkill >> 0) & 1));
	pHandle->SetValue(21, !((pXSprite->lSkill >> 1) & 1));
	pHandle->SetValue(22, !((pXSprite->lSkill >> 2) & 1));
	pHandle->SetValue(23, !((pXSprite->lSkill >> 3) & 1));
	pHandle->SetValue(24, !((pXSprite->lSkill >> 4) & 1));
	pHandle->SetValue(25, !pXSprite->lS);
	pHandle->SetValue(26, !pXSprite->lB);
	pHandle->SetValue(27, !pXSprite->lC);
	pHandle->SetValue(28, !pXSprite->lT);

	pHandle->SetValue(29, pXSprite->decoupled);
	pHandle->SetValue(30, pXSprite->triggerOnce);
	pHandle->SetValue(31, pXSprite->locked);
	pHandle->SetValue(32, pXSprite->interruptable);
	pHandle->SetValue(33, pXSprite->dudeLockout);

	pHandle->SetValue(kSprDialogData1, pXSprite->data1);
	pHandle->SetValue(kSprDialogData2, pXSprite->data2);
	pHandle->SetValue(kSprDialogData3, pXSprite->data3);
	pHandle->SetValue(kSprDialogData4, pXSprite->data4);


	pHandle->SetValue(38, pXSprite->respawn);
	pHandle->SetValue(39, pXSprite->dudeFlag4);
	pHandle->SetValue(40, pXSprite->dudeDeaf);
	pHandle->SetValue(41, pXSprite->dudeGuard);
	pHandle->SetValue(42, pXSprite->dudeAmbush);
	pHandle->SetValue(43, pXSprite->unused1); // used to set stealth flag for dude

	pHandle->SetValue(44, pXSprite->key);
	pHandle->SetValue(45, pXSprite->wave);
	pHandle->SetValue(46, sprite[nSprite].flags);
	pHandle->SetValue(47, pXSprite->lockMsg);
	pHandle->SetValue(48, pXSprite->dropItem);
}

void dlgDialogToXSprite(DIALOG_HANDLER* pHandle, int nSprite)
{
	int val, nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	sprite[nSprite].type		= (short)pHandle->GetValue(1);;
	pXSprite->rxID				= pHandle->GetValue(2);
	pXSprite->txID				= pHandle->GetValue(3);
	pXSprite->state				= pHandle->GetValue(4);
	pXSprite->command			= pHandle->GetValue(5);

	pXSprite->triggerOn			= pHandle->GetValue(6);
	pXSprite->triggerOff		= pHandle->GetValue(7);
	pXSprite->busyTime			= pHandle->GetValue(8);
	pXSprite->waitTime			= pHandle->GetValue(9);
	pXSprite->restState			= pHandle->GetValue(10);

	pXSprite->triggerPush		= pHandle->GetValue(11);
	pXSprite->triggerVector		= pHandle->GetValue(12);
	pXSprite->triggerImpact		= pHandle->GetValue(13);
	pXSprite->triggerPickup		= pHandle->GetValue(14);
 	pXSprite->triggerTouch		= pHandle->GetValue(15);
 	pXSprite->triggerProximity	= pHandle->GetValue(16);
	pXSprite->triggerSight		= pHandle->GetValue(17);
	if (pHandle->GetValue(18)) pXSprite->unused3 |= 0x0001;
	else pXSprite->unused3 &= ~0x0001;

 	if (pHandle->GetValue(19)) pXSprite->unused3 |= 0x0002;
	else pXSprite->unused3 &= ~0x0002;
	//pXSprite->triggerReserved1	= pHandle->GetValue(18);
	//pXSprite->triggerReserved2	= pHandle->GetValue(19);


 	pXSprite->lSkill			=
								(!pHandle->GetValue(20) << 0) |
								(!pHandle->GetValue(21) << 1) |
								(!pHandle->GetValue(22) << 2) |
								(!pHandle->GetValue(23) << 3) |
								(!pHandle->GetValue(24) << 4);
 	pXSprite->lS				= !pHandle->GetValue(25);
 	pXSprite->lB				= !pHandle->GetValue(26);
 	pXSprite->lC				= !pHandle->GetValue(27);
 	pXSprite->lT				= !pHandle->GetValue(28);

	pXSprite->decoupled			= pHandle->GetValue(29);
	pXSprite->triggerOnce		= pHandle->GetValue(30);
	pXSprite->locked			= pHandle->GetValue(31);
	pXSprite->interruptable		= pHandle->GetValue(32);
	pXSprite->dudeLockout		= pHandle->GetValue(33);

	pXSprite->data1				= pHandle->GetValue(kSprDialogData1);
	pXSprite->data2				= pHandle->GetValue(kSprDialogData2);
	pXSprite->data3				= pHandle->GetValue(kSprDialogData3);
	
	val							= pHandle->GetValue(kSprDialogData4);
	pXSprite->data4				= (val == -1) ? 65535 : val;

	pXSprite->respawn			= pHandle->GetValue(38);

	pXSprite->dudeFlag4			= pHandle->GetValue(39);
	pXSprite->dudeDeaf			= pHandle->GetValue(40);
	pXSprite->dudeGuard			= pHandle->GetValue(41);
	pXSprite->dudeAmbush		= pHandle->GetValue(42);
	pXSprite->unused1			= pHandle->GetValue(43);

	pXSprite->key				= pHandle->GetValue(44);
	pXSprite->wave				= pHandle->GetValue(45);
	sprite[nSprite].flags       = (short)pHandle->GetValue(46);
	pXSprite->lockMsg			= pHandle->GetValue(47);
	pXSprite->dropItem			= pHandle->GetValue(48);

}

void dlgDialogToSprite(DIALOG_HANDLER* pHandle, int nSprite)
{
	int i;
	dassert(nSprite > 0 && nSprite < kMaxSprites);
	spritetype *pSpr = &sprite[nSprite];
	
	pSpr->x 			= pHandle->GetValue(1);
	pSpr->y 			= pHandle->GetValue(2);
	pSpr->z 			= pHandle->GetValue(3);
	pSpr->sectnum 		= pHandle->GetValue(4);
	pSpr->statnum 		= pHandle->GetValue(5);
	pSpr->hitag 		= pHandle->GetValue(6);
	pSpr->type 			= pHandle->GetValue(7);
	pSpr->clipdist 		= pHandle->GetValue(8);
	pSpr->extra 		= pHandle->GetValue(9);
	
	pSpr->picnum 		= pHandle->GetValue(10);
	pSpr->shade 		= pHandle->GetValue(11);
	pSpr->pal 			= pHandle->GetValue(12);
	pSpr->xrepeat 		= pHandle->GetValue(13);
	pSpr->yrepeat 		= pHandle->GetValue(14);
	pSpr->xoffset 		= pHandle->GetValue(15);
	pSpr->yoffset 		= pHandle->GetValue(16);
	
	i = pHandle->GetValue(17);
	if (i && (pSpr->cstat & kSprRelMask) != kSprFloor)
		pSpr->cstat |= kSprFloor;

	spriteSetSlope(nSprite, i);
	
	pSpr->ang 			= pHandle->GetValue(18);
	pSpr->xvel 			= pHandle->GetValue(19);
	pSpr->yvel 			= pHandle->GetValue(20);
	pSpr->zvel 			= pHandle->GetValue(21);
	pSpr->owner 		= pHandle->GetValue(22);
	
	// 23
	// 24
	// 25
	
/* 	setCstat(pHandle->GetValue(26), &pSpr->cstat, kSprBlock);
	setCstat(pHandle->GetValue(27), &pSpr->cstat, kSprHitscan);
	setCstat(pHandle->GetValue(28), &pSpr->cstat, kSprOneSided);
	setCstat(pHandle->GetValue(29), &pSpr->cstat, kSprInvisible);
	setCstat(pHandle->GetValue(30), &pSpr->cstat, kSprOrigin);
	setCstat(pHandle->GetValue(31), &pSpr->cstat, kSprFlipX);
	setCstat(pHandle->GetValue(32), &pSpr->cstat, kSprFlipY);
	setCstat(pHandle->GetValue(33), &pSpr->cstat, kSprTransluc1);
	setCstat(pHandle->GetValue(34), &pSpr->cstat, kSprTranslucR);
	setCstat(pHandle->GetValue(35), &pSpr->cstat, kSprMoveForward);
	setCstat(pHandle->GetValue(36), &pSpr->cstat, kSprMoveReverse); */
}

void dlgXSectorToDialog(DIALOG_HANDLER* pHandle, int nSector)
{
	int nXSector = sector[nSector].extra;
	int i, j;
	
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	pHandle->SetValue(kSectType,		sector[nSector].type);

	pHandle->SetValue(kSectRX,			pXSector->rxID);
	pHandle->SetValue(kSectTX,			pXSector->txID);
	pHandle->SetValue(kSectState,		pXSector->state);
	pHandle->SetValue(kSectCmd,			pXSector->command);

	pHandle->SetValue(kSectDecoupled,	pXSector->decoupled);
	pHandle->SetValue(kSectOnce,		pXSector->triggerOnce);
	pHandle->SetValue(kSectLocked,		pXSector->locked);
	pHandle->SetValue(kSectInterrupt,	pXSector->interruptable);
	pHandle->SetValue(kSectLockout,		pXSector->dudeLockout);

	pHandle->SetValue(kSectGoingOn,		pXSector->triggerOn);
	pHandle->SetValue(kSectBusyTimeOff, pXSector->busyTimeA);
	pHandle->SetValue(kSectBusyWaveOff, pXSector->busyWaveA);
	pHandle->SetValue(kSectWaitOff, 	pXSector->reTriggerA);
	pHandle->SetValue(kSectWaitTimeOff,	pXSector->waitTimeA);

	pHandle->SetValue(kSectGoingOff,	pXSector->triggerOff);
	pHandle->SetValue(kSectBusyTimeOn,	pXSector->busyTimeB);
	pHandle->SetValue(kSectBusyWaveOn,	pXSector->busyWaveB);
	pHandle->SetValue(kSectWaitOn,		pXSector->reTriggerB);
	pHandle->SetValue(kSectWaitTimeOn,	pXSector->waitTimeB);

	pHandle->SetValue(kSectTrPush,		pXSector->triggerPush);
	pHandle->SetValue(kSectTrWPush,		pXSector->triggerWallPush);
	pHandle->SetValue(kSectTrEnter,		pXSector->triggerEnter);
	pHandle->SetValue(kSectTrExit,		pXSector->triggerExit);

	// get values of first found setor sfx sprite
	for(i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		if (sprite[i].type == kSoundSector && sprite[i].extra > 0)
		{
			for (j = 0; j < 4; j++)
				pHandle->SetValue(kSectSndOffOn + j, getDataOf(j, OBJ_SPRITE, i));
			
			break;
		}
	}

	// nothing found? set all zeros
	if (i == -1)
	{
		for (i = 0; i < 4; i++)
			pHandle->SetValue(kSectSndOffOn + i, 0);
	}

	pHandle->SetValue(kSectKey,		pXSector->key);
	pHandle->SetValue(kSectDepth,	pXSector->Depth);
	pHandle->SetValue(kSectUwater,	pXSector->underwater);
	pHandle->SetValue(kSectCrush,	pXSector->crush);

	pHandle->SetValue(kSectData,	pXSector->data);
	
	
	DIALOG_HANDLER dialog(&gMapedHud, dlgXSectorFX);
	dlgXSectorFXToDialog(&dialog, nSector);
	
}


void dlgDialogToXSector(DIALOG_HANDLER* pHandle, int nSector)
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	sector[nSector].type 		= pHandle->GetValue(kSectType);
	pXSector->rxID 				= pHandle->GetValue(kSectRX);
	pXSector->txID 				= pHandle->GetValue(kSectTX);
	pXSector->state 			= pHandle->GetValue(kSectState);
	pXSector->command 			= pHandle->GetValue(kSectCmd);

	pXSector->decoupled			= pHandle->GetValue(kSectDecoupled);
	pXSector->triggerOnce		= pHandle->GetValue(kSectOnce);
	pXSector->locked			= pHandle->GetValue(kSectLocked);
	pXSector->interruptable		= pHandle->GetValue(kSectInterrupt);
	pXSector->dudeLockout		= pHandle->GetValue(kSectLockout);

	pXSector->triggerOn  		= pHandle->GetValue(kSectGoingOn);
	pXSector->busyTimeA			= pHandle->GetValue(kSectBusyTimeOff);
	pXSector->busyWaveA			= pHandle->GetValue(kSectBusyWaveOff);
	pXSector->reTriggerA		= pHandle->GetValue(kSectWaitOff);
	pXSector->waitTimeA			= pHandle->GetValue(kSectWaitTimeOff);

	pXSector->triggerOff 		= pHandle->GetValue(kSectGoingOff);
	pXSector->busyTimeB			= pHandle->GetValue(kSectBusyTimeOn);
	pXSector->busyWaveB			= pHandle->GetValue(kSectBusyWaveOn);
	pXSector->reTriggerB		= pHandle->GetValue(kSectWaitOn);
	pXSector->waitTimeB			= pHandle->GetValue(kSectWaitTimeOn);

	pXSector->triggerPush		= pHandle->GetValue(kSectTrPush);
	pXSector->triggerWallPush	= pHandle->GetValue(kSectTrWPush);
	pXSector->triggerEnter		= pHandle->GetValue(kSectTrEnter);
	pXSector->triggerExit		= pHandle->GetValue(kSectTrExit);

	int i = 0;
	short cstat = 0;
	int x = kHiddenSpriteLoc, y = kHiddenSpriteLoc, z = sector[nSector].floorz;

	// delete all the sector sfx sprites in this sector
	for (i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
	{
		if (sprite[i].statnum >= kStatFree || sprite[i].type != kSoundSector)
			continue;

		// save info of first found sprite
		if (x == kHiddenSpriteLoc)
		{

			x = sprite[i].x;
			y = sprite[i].y;
			z = sprite[i].z;
			cstat = sprite[i].cstat;
		}

		DeleteSprite(i);
		i = headspritesect[nSector];
	}

	// create new sector sfx sprite if values are not zero
	for (i = 0; i < 4; i++)
	{
		if (pHandle->GetValue(kSectSndOffOn + i) <= 0)
			continue;

		// set new coords if there was no old sprite found
		if (x == kHiddenSpriteLoc)
			avePointSector(nSector, &x, &y);

		int nSprite = InsertSprite(nSector, 0);
		dassert(nSprite >= 0 && nSprite < kMaxSprites);

		sprite[nSprite].x = x;
		sprite[nSprite].y = y;
		sprite[nSprite].z = z;

		ChangeSpriteSect(nSprite, nSector);


		spritetype* pSprite = &sprite[nSprite];
		int nXSprite = GetXSprite(nSprite);
		dassert(nXSprite >= 0 && nXSprite < kMaxXSprites && nXSprite == pSprite->extra);
		pSprite->type = kSoundSector;
		adjSpriteByType(pSprite);

		for (i = 0; i < 4; i++)
			setDataValueOfObject(OBJ_SPRITE, pSprite->index, i + 1, pHandle->GetValue(kSectSndOffOn + i));

		if (cstat != 0) pSprite->cstat = cstat;
		else if (sector[nSector].type > kSectorTeleport && sector[nSector].type <= kSectorRotate)
			pSprite->cstat |= kSprMoveForward; // if sector is movable, set kinetic movement stat

		break;
	}

	pXSector->key				= pHandle->GetValue(kSectKey);
	pXSector->Depth				= pHandle->GetValue(kSectDepth);
	pXSector->underwater		= pHandle->GetValue(kSectUwater);
	pXSector->crush				= pHandle->GetValue(kSectCrush);

	pXSector->data				= pHandle->GetValue(kSectData);
	
	DIALOG_HANDLER dialog(&gMapedHud, dlgXSectorFX);
	dlgDialogToXSectorFX(&dialog, nSector);
}

void dlgXSectorFXToDialog(DIALOG_HANDLER* pHandle, int nSector)
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];
	
	pHandle->SetValue(1, pXSector->shadeWave);
	pHandle->SetValue(2, pXSector->amplitude);
	pHandle->SetValue(3, pXSector->shadeFreq);
	pHandle->SetValue(4, pXSector->shadePhase);
	pHandle->SetValue(5, pXSector->shadeFloor);
	pHandle->SetValue(6, pXSector->shadeCeiling);
	pHandle->SetValue(7, pXSector->shadeWalls);
	pHandle->SetValue(8, pXSector->shadeAlways);

	pHandle->SetValue(9, pXSector->coloredLights);
	pHandle->SetValue(10, pXSector->ceilpal2);
	pHandle->SetValue(11, pXSector->floorpal2);

	pHandle->SetValue(12, pXSector->panVel);
	pHandle->SetValue(13, pXSector->panAngle);
	pHandle->SetValue(14, pXSector->panFloor);
	pHandle->SetValue(15, pXSector->panCeiling);
	pHandle->SetValue(16, pXSector->panAlways);
	pHandle->SetValue(17, pXSector->drag);

	pHandle->SetValue(18, pXSector->windVel);
	pHandle->SetValue(19, pXSector->windAng);
	pHandle->SetValue(20, pXSector->windAlways);

	pHandle->SetValue(21, pXSector->bobZRange);
	pHandle->SetValue(22, pXSector->bobTheta);
	pHandle->SetValue(23, pXSector->bobSpeed);
	pHandle->SetValue(24, pXSector->bobAlways);
	pHandle->SetValue(25, pXSector->bobFloor);
	pHandle->SetValue(26, pXSector->bobCeiling);
	pHandle->SetValue(27, pXSector->bobRotate);
	pHandle->SetValue(28, pXSector->damageType);
}

void dlgDialogToXSectorFX(DIALOG_HANDLER* pHandle, int nSector)
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];
	
	pXSector->shadeWave			= pHandle->GetValue(1);
	pXSector->amplitude     	= pHandle->GetValue(2);
	pXSector->shadeFreq        	= pHandle->GetValue(3);
	pXSector->shadePhase		= pHandle->GetValue(4);
	pXSector->shadeFloor    	= pHandle->GetValue(5);
	pXSector->shadeCeiling  	= pHandle->GetValue(6);
	pXSector->shadeWalls    	= pHandle->GetValue(7);
	pXSector->shadeAlways   	= pHandle->GetValue(8);

	pXSector->coloredLights		= pHandle->GetValue(9);
	pXSector->ceilpal2			= pHandle->GetValue(10);
	pXSector->floorpal2			= pHandle->GetValue(11);

	pXSector->panVel			= pHandle->GetValue(12);
	pXSector->panAngle			= pHandle->GetValue(13);
	pXSector->panFloor			= pHandle->GetValue(14);
	pXSector->panCeiling		= pHandle->GetValue(15);
	pXSector->panAlways			= pHandle->GetValue(16);
	pXSector->drag				= pHandle->GetValue(17);

	pXSector->windVel			= pHandle->GetValue(18);
	pXSector->windAng			= pHandle->GetValue(19);
	pXSector->windAlways		= pHandle->GetValue(20);

	pXSector->bobZRange			= pHandle->GetValue(21);
	pXSector->bobTheta			= pHandle->GetValue(22);
	pXSector->bobSpeed			= pHandle->GetValue(23);
	pXSector->bobAlways			= pHandle->GetValue(24);
	pXSector->bobFloor			= pHandle->GetValue(25);
	pXSector->bobCeiling		= pHandle->GetValue(26);
	pXSector->bobRotate			= pHandle->GetValue(27);
	pXSector->damageType		= pHandle->GetValue(28);
}



int dlgCopyObjectSettings(DIALOG_ITEM* pDialog, int nType, int nSrc, int nDst, char hglt)
{
	DIALOG_ITEM *pDlg, *pCpy, *pDst, *et, *ed, *es;
	int nSiz, i = LENGTH(gDialogInfo), n = 0; char ok = 0;
	DIALOG_HANDLER *hSrc, *hDst; DIALOG_INFO* pEntry;
	OBJECT_LIST objects; OBJECT* pObj;
		
	while(--i >= 0)
	{
		pEntry = &gDialogInfo[i];
		if (pEntry->dialog == pDialog && pEntry->objectType == nType)
			break;
	}
	
	if (i < 0)
		return -1;
	
	pDlg = pEntry->dialog;			// source dialog
	nSiz = (dlgCountItems(pDlg)+1) * sizeof(DIALOG_ITEM);
	
	pCpy = (DIALOG_ITEM*)malloc(nSiz);	// original dialog
	dassert(pCpy != NULL);
	
	pDst = (DIALOG_ITEM*)malloc(nSiz);	// will be changed on comparison
	dassert(pDst != NULL);
	
	switch(nType)
	{
		case OBJ_SPRITE:	GetXSprite(nSrc);	break;
		case OBJ_WALL:		GetXWall(nSrc);	  	break;
		case OBJ_SECTOR:	GetXSector(nSrc);	break;
	}
	
	// nasty trick to add checkboxes in the dialog
	// but keep the formatted labels of the source
	// object
	
	hSrc = new DIALOG_HANDLER(&gMapedHud, (DIALOG_ITEM*)pDlg);
	pEntry->objectToDialog(hSrc, nSrc);
	hSrc->Paint(); // this forces format of readyLabel which is what we need
	
	// backup the source so we can use values later
	memcpy(pCpy, pDlg, nSiz);
	
	et = pDlg;
	while(et->type != CONTROL_END)
	{
		switch(et->type)
		{
			case ELT_SELECTOR:
			case CHECKBOX:
				// just make it to be an element selector
				et->type = ELT_SELECTOR;
				break;
			case HEADER:
			case LABEL:
				break;
			case DIALOG:
				// nested dialogs is not currently
				// supported so make it invisible
				if (et->readyLabel)
				{
					if (et->readyLabel != et->formatLabel)
						free(et->readyLabel);
					
					et->formatLabel = "";
					et->readyLabel = et->formatLabel;
				}
				et->enabled = 0;
				break;
			default:
				if (et->readyLabel)
				{
					// make it to be an element selector and use
					// the formatted text as a normal
					// label
					
					et->type = ELT_SELECTOR;
					et->formatLabel = (char*)malloc(strlen(et->readyLabel)+1);
					dassert(et->formatLabel != NULL);
					sprintf(et->formatLabel, "%0.26s", et->readyLabel);
					et->value = 0;
				}
				break;
		}
		
		et->flags = NO;
		et++;
	}

	gMapedHud.SetMsg("Select properties for copying from %s #%d:", gSearchStatNames[nType], nSrc);
	gMapedHud.DrawIt();
	
	if ((ok = hSrc->Edit()) != 0)
	{
		if (hglt)
		{
			if (nDst < 0 || Confirm("%s #%d in a highlight! Copy properties to the rest of objects?", gSearchStatNames[nType], nDst))
				hglt2List(&objects, 0, kHgltSector|kHgltPoint);
		}
		
		if (nDst >= 0 && !objects.Exists(nType, nDst))
			objects.Add(nType, nDst);

		for (pObj = objects.Ptr(); pObj->type != OBJ_NONE; pObj++)
		{
			if (pObj->type != pEntry->objectType)
				continue;
			
			switch(pObj->type)
			{
				case OBJ_SPRITE:	GetXSprite(pObj->index);	break;
				case OBJ_WALL:		GetXWall(pObj->index);	  	break;
				case OBJ_SECTOR:	GetXSector(pObj->index);	break;
			}
			
			memcpy(pDst, pCpy, nSiz);
			hDst = new DIALOG_HANDLER(&gMapedHud, pDst);
			pEntry->objectToDialog(hDst, pObj->index);
			
			et = pDlg, ed = pDst, es = pCpy;
			while(et->type != CONTROL_END)
			{
				// was it selected in clipboard dialog?
				if (et->selected)
					ed->value = es->value; // source -> destination
				
				et++, ed++, es++;
			}
			
			pEntry->dialogToObject(hDst, pObj->index);
			delete(hDst);
			n++;
		}
		
		AutoAdjustSprites();
		CleanUp();
	}
	else
	{
		n = -1;
	}
	
	et = pDlg; es = pCpy;
	while(et->type != CONTROL_END)
	{
		if (et->type == ELT_SELECTOR && et->readyLabel)
			free(et->formatLabel); // don't forget to free this
		
		es->selected = et->selected; // save selected state in source
		et++, es++;
	}
	
	// restore source dialog
	memcpy(pDlg, pCpy, nSiz);
	free(pCpy); free(pDst);
	return n;
}