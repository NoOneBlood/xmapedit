#include "nnexts.h"
#include "preview.h"
#include "aadjust.h"
#include "xmpsnd.h"
#include "xmphudlg.h"
#include "xmptools.h"
#include "xmpexplo.h"
#include "xmpstub.h"
#include "xmpconf.h"
#include "xmpmisc.h"

#include "edit2d.h"


/* DLG_MAP_INFO gTest;
DIALOG_ITEM dlgMapInfo[] =
{
	{ NO,	1,		0,	0,	0,	HEADER,			"General info:" },
	{ NO,	1,		0,	1,	1,	LABEL,			"Filename....: %s", 0, 1, gTest.pathptr},
	{ NO,	1,		0,	2,	2,	LABEL,			"Author......: %s", 0, 1, gTest.authptr},
	{ NO,	1,		0,	3,	3,	NUMBER, 		"Revisions...: %d", 0, 13421722 },
}; */

DIALOG_ITEM dlgXSprite[] =
{
	{ NO,	1,		0,	0,	1,	LIST,			"Type %4d: %-18.18s", 0, 1023, gSpriteNames },
	{ NO,	1,		0,	1,	2,	NUMBER, 		"RX ID: %-4d", 0, 1023, NULL, GetNextUnusedID },
	{ NO,	1,		0,	2,	3,	NUMBER, 		"TX ID: %-4d", 0, 1023, NULL, GetNextUnusedID },
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
	{ NO,	1,		46, 7,	kSprDialogData1,	NUMBER, 	"Data1", -32767, 32767, NULL, AuditSound },
	{ NO,	1,		46,	8,	kSprDialogData2,	NUMBER, 	"Data2", -32767, 32767, NULL, AuditSound },
	{ NO,	1,		46,	9,	kSprDialogData3,	NUMBER, 	"Data3", -32767, 32767, NULL, AuditSound },
	{ NO,	1,		46,	10,	kSprDialogData4,	NUMBER, 	"Data4", -65535, 65535, NULL, AuditSound },

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
	{ NO,	1,		64,	9,	47,	NUMBER,			"Message:   %-3d", 0, 255, NULL, pickIniMessage },
	{ NO,	1,		64,	10,	48,	NUMBER,			"Drop item: %-3d", 0, kItemMax, NULL, pickItemTile },

	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgSprite[] =
{
	{ NO,	1,		0,	0,	0,	HEADER, 		"General:" },
	{ NO,	0,		0,	1,	1,	NUMBER,			"X-coordinate..: %d",		-kDefaultBoardSize, kDefaultBoardSize,	NULL,	NULL},
	{ NO,	0,		0,	2,	2,	NUMBER,			"Y-coordinate..: %d", 		-kDefaultBoardSize, kDefaultBoardSize,	NULL,	NULL},
	{ NO,	0,		0,	3,	3,	NUMBER,			"Z-coordinate..: %d", 		-13421721, 			13421722, 			NULL,	NULL},
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
	{ NO,	1,		0,	0,	1,	LIST, 		"Type %4d: %-18.18s", 0, 1023, gWallNames },
	{ NO,	1,		0,	1,	2,	NUMBER, 	"RX ID: %4d", 0, 1023, NULL, GetNextUnusedID },
	{ NO,	1,		0,	2,	3,	NUMBER, 	"TX ID: %4d", 0, 1023, NULL, GetNextUnusedID },
	{ NO,	1,		0,	3,	4,	LIST, 		"State %1d: %-3.3s", 0, 1, gBoolNames },
	{ NO,	1,		0,	4,	5,	LIST, 		"Cmd: %3d: %-18.18s", 0, 255, gCommandNames },

	{ NO,	1,		0,	5,	0,	HEADER, 	"Send when:" },
	{ NO,	1,		0,	6,	6,	CHECKBOX,	"going ON" },
	{ NO,	1,		0,	7,	7,	CHECKBOX,	"going OFF" },
	{ NO,	1,		0,	8,	8,	NUMBER, 	"busyTime = %4d", 0, 4095 },
	{ NO,	1,		0,	9,	9,	NUMBER, 	"waitTime = %4d", 0, 4095 },
	{ NO,	1,		0,	10,	10,	LIST,		"restState %1d: %-3.3s", 0, 1, gBoolNames },

	{ NO,	1,		30,	0,	0,	HEADER,		"Trigger On:" },
	{ NO,	1,		30,	1,	11,	CHECKBOX,	"Push" },
	{ NO,	1,		30,	2,	12,	CHECKBOX,	"Vector" },
	{ MO,	1,		30,	3,	13,	CHECKBOX,	"Touch" },


	{ NO,	1,		30,	5,	14,	LIST,		"Key: %1d %-7.7s", 0, 7, gKeyItemNames },
	{ NO,	1,		30,	7,	15,	NUMBER, 	"panX = %4d", -128, 127 },
	{ NO,	1,		30,	8,	16,	NUMBER, 	"panY = %4d", -128, 127 },
	{ NO,	1,		30,	9,	17,	CHECKBOX,	"always" },

	{ NO,	1,		48,	0,	0,	HEADER, 	"Trigger Flags:" },
	{ NO,	1,		48,	1,	18,	CHECKBOX, 	"Decoupled" },
	{ NO,	1,		48,	2,	19,	CHECKBOX,	"1-shot" },
	{ NO,	1,		48,	3,	20,	CHECKBOX, 	"Locked" },
	{ NO,	1,		48,	4,	21,	CHECKBOX,	"Interruptable" },
	{ NO,	1,		48,	5,	22,	CHECKBOX,	"Player only" },

	{ NO,	1,		48,	7,	0,	HEADER, 	"Data:         " },
	{ NO,	1,		48,	8,	kWallDialogData,	NUMBER, 	"Data", -65535, 65535, NULL, AuditSound },

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
	{ NO,	1,		0,	0,	1,	LIST, 		"Type %4d: %-16.16s", 0, 1023, gSectorNames },
	{ NO,	1,		0,	1,	2,	NUMBER, 	"RX ID: %4d", 0, 1023, NULL, GetNextUnusedID },
	{ NO,	1,		0,	2,	3,	NUMBER, 	"TX ID: %4d", 0, 1023, NULL, GetNextUnusedID },
	{ NO,	1,		0,	3,	4,	LIST, 		"State %1d: %-3.3s", 0, 1, gBoolNames },
	{ NO,	1,		0,	4,	5,	LIST, 		"Cmd: %3d: %-16.16s", 0, 255, gCommandNames },

	{ NO,	1,		0,	5,	0,	HEADER, 	"Trigger Flags:" },
	{ NO,	1,		0,	6,	6,	CHECKBOX, 	"Decoupled" },
	{ NO,	1,		0,	7,	7,	CHECKBOX,	"1-shot" },
	{ NO,	1,		0,	8,	8,	CHECKBOX, 	"Locked" },
	{ NO,	1,		0,	9,	9,	CHECKBOX, 	"Interruptable" },
	{ NO,	1,		0,	10, 10,	CHECKBOX,	"Player only" },

	{ NO,	1,		28,	0,	0,	HEADER, 	"OFF->ON:" },
	{ NO,	1,		28,	1,	11,	CHECKBOX,	"send at ON" },
	{ NO,	1,		28,	2,	12,	NUMBER, 	"busyTime   = %3d", 0, 255 },
	{ NO,	1,		28,	3,	13,	LIST, 		"Wave: %1d %-8.8s", 0, 3, gBusyNames },
	{ NO,	1,		28,	4,	14,	CHECKBOX, 	"" },
	{ NO,	1,		30,	4,	15,	NUMBER, 	"waitTime = %3d", 0, 255 },

	{ NO,	1,		28,	6,	0,	HEADER, 	"ON->OFF:" },
	{ NO,	1,		28,	7,	16,	CHECKBOX,	"send at OFF" },
	{ NO,	1,		28,	8,	17,	NUMBER, 	"busyTime   = %3d", 0, 255 },
	{ NO,	1,		28,	9,	18,	LIST, 		"Wave: %1d %-8.8s", 0, 3, gBusyNames },
	{ NO,	1,		28,	10,	19,	CHECKBOX, 	"" },
	{ NO,	1,		30,	10,	20,	NUMBER, 	"waitTime = %3d", 0, 255 },

	{ NO,	1,		45,	0,	0,	HEADER, 	"Trigger On:    " },
	{ NO,	1,		45,	1,	21,	CHECKBOX,	"Push" },
	{ NO,	1,		45,	2,	22,	CHECKBOX,	"WallPush" },
	//{	45,	2,	22,	CHECKBOX,	"Vector" },
	//{	45,	3,	23,	CHECKBOX,	"Reserved" },
	{ NO,	1,		45,	3,	23,	CHECKBOX,	"Enter" },
	{ NO,	1,		45,	4,	24,	CHECKBOX,	"Exit" },

	{ NO,	1,		45,	6,	0,	HEADER, 	"Sound:         " },
	{ NO,	1,		45,	7,	25,	NUMBER,		"Off->On : %5d", 0, 32768, NULL, AuditSound },
	{ NO,	1,		45,	8,	26,	NUMBER,		"Stopping: %5d", 0, 32768, NULL, AuditSound },
	{ NO,	1,		45,	9,	27,	NUMBER,		"On->Off : %5d", 0, 32768, NULL, AuditSound },
	{ NO,	1,		45,	10,	28,	NUMBER,		"Stopping: %5d", 0, 65535, NULL, AuditSound },

	{ NO,	1,		61,	0,	29,	LIST,		"Key:   %1d %-7.7s", 0, 7, gKeyItemNames },
	{ NO,	1,		61,	1,	30,	LIST,		"Depth: %1d %-7.7s", 0, 7, gDepthNames },
	{ NO,	1,		61,	3,	31,	CHECKBOX, 	"Underwater" },
	{ NO,	1,		61,	4,	32,	CHECKBOX, 	"Crush" },

	{ NO,	1,		61,	6,	0,	HEADER, 		"Data:         " },
	{ NO,	1,		61,	7,	kSectDialogData,	NUMBER,		"Data", -65535, 65535 },

	{ NO,	1,		61,	10,	34,	DIALOG,		"FX...",  0, 0, NULL, DoSectorFXDialog },

	{ NO,	1,		0,	0,	0,	CONTROL_END },
};

DIALOG_ITEM dlgSector[] =
{
	
	{ NO,	1,		0,	0,	0,	HEADER, 		"General:" },
	{ NO,	0,		0,	1,	1,	NUMBER,			"Total walls..: %d",		0,					kMaxWalls - 1,		NULL,	NULL},
	{ NO,	0,		0,	2,	2,	NUMBER,			"First wall...: %d",		0,					kMaxWalls - 1,		NULL,	NULL},
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


/* void getDialogSize(DIALOG_ITEM* dialog, int* wh, int* hg)
{
	int len; *wh = *hg = 0;
	for (DIALOG_ITEM *ptr = dialog; ptr->type != CONTROL_END; ptr++)
	{
		switch (ptr->type) {
			case HEADER:
			case DIALOG:
			case LABEL:
				break;
			default:
				controlSetReadyLabel(ptr, ptr->formatLabel);
				break;
		}
		
		len = strlen((ptr->readyLabel) ? ptr->readyLabel : ptr->formatLabel);
		*wh += (ptr->x << 3) + (len << 3);
		*hg += (ptr->y << 3) + 8;
	}
} */

BYTE GetNextUnusedID(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key ) {

	int i;
	switch (key) {
		case KEY_F10:
			if ((i = findUnusedChannel(dialog)) == -1) return key;
			control->value = i;
			return 0;
	}

	return key;
}


BYTE DoSectorFXDialog(DIALOG_ITEM*, DIALOG_ITEM*, BYTE key )
{
	switch ( key )
	{
		case KEY_ENTER:
			if ( EditDialog(dlgXSectorFX) )
				return KEY_ENTER;
			return 0;
	}

	return key;
}



BYTE AuditSound( DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key )
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
				if ( gSoundRes.Lookup(i, "SFX") != NULL )
				{
					control->value = i;
					break;
				}
			}
			return 0;
		case KEY_F10:
			if (dialog == dlgXWall)
			{
				switch (dialog->value) {
					case kWallGib:
						if (control->tabGroup != kWallDialogData) break;
						i = toolGibTool(control->value, OBJ_WALL);
						if (i < 0)
							return key;

						control->value = i;
						return 0;
				}
			}
			else if (dialog == dlgXSprite)
			{
				switch (dialog->value) {
					case kMarkerDudeSpawn:
						if (control->tabGroup < kSprDialogData1 || control->tabGroup > kSprDialogData4) break;
						pickEnemyTile(dialog, control, key);
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
						else control->value = findUnusedStack();
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

int pickTypeHelper(int nGroup, char* title)
{
	int retn = -1;
	
	if (adjFillTilesArray(nGroup))
	{
		int i, j;
		BYTE* scrSave = (BYTE*)Resource::Alloc(xdim*ydim);
		memcpy(scrSave, (void*)frameplace, xdim*ydim);
		
		if ((j = tilePick(-1, -1, OBJ_CUSTOM, title)) >= 0)
		{
			for (i = 0; i < autoDataLength; i++)
			{
				if (autoData[i].picnum != j) continue;
				retn = autoData[i].type;
				break;
			}
		}
		
		memcpy((void*)frameplace, scrSave, xdim*ydim);
		Resource::Free(scrSave);
	}
	
	return retn;
}

BYTE pickEnemyTile(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key) {

	int value;
	if (key == KEY_F10)
	{
		if ((value = pickTypeHelper(kOGrpDude, "Select enemy to spawn")) >= 0)
			control->value = value;
	}
	
	return key;

}

BYTE pickItemTile(DIALOG_ITEM* dialog, DIALOG_ITEM *control, BYTE key) {

	int value;
	if (key == KEY_F10)
	{
		if ((value = pickTypeHelper(kOGrpWeapon | kOGrpAmmo | kOGrpAmmoMix | kOGrpItem, "Select item")) >= 0)
			control->value = value;
	}
	
	return key;

}

#define kMaxMessages 32
#define kMaxMessageLength 64
BYTE pickIniMessage(DIALOG_ITEM*, DIALOG_ITEM *control, BYTE key) {

	if (key != KEY_F10)
		return key;

	NAMED_TYPE iniMessages[kMaxMessages];
	char messages[kMaxMessages][kMaxMessageLength], tmpbuf[256];
	IniFile* pEpisode = gPreview.pEpisode; char *tmp = NULL; char *iniFile = NULL;
	int i, j, len;

	BYTE* scrSave = (BYTE*)Resource::Alloc(xdim*ydim);
	memcpy(scrSave, (void*)frameplace, xdim*ydim);
	sprintf(tmpbuf, gPaths.episodeIni);

	while ( 1 )
	{
		iniFile = dirBrowse("Select INI File", tmpbuf, ".ini", kDirExpTypeOpen, kDirExpNone);
		if (iniFile == NULL)
			break;

		delete(pEpisode);
		pEpisode = new IniFile(iniFile);
		if (!pEpisode->SectionExists("Episode1"))
		{
			getFilename(iniFile, buffer);
			Alert("%s is not an episode INI.", buffer);
			continue;
		}

		getFilename(gPaths.maps, buffer);
		if (!pEpisode->SectionExists(buffer))
		{
			Alert("There is no section with map name %s.", buffer);
			continue;
		}

		memset(messages, 0, kMaxMessages*kMaxMessageLength);
		for (i = 1, j = 0; i < kMaxMessages; i++) {
			sprintf(buffer2, "message%d", i);
			if ((tmp = pEpisode->GetKeyString(buffer, buffer2, NULL)) == NULL)
				continue;

			len = strlen(tmp);
			sprintf(messages[i], "%0.44s%s", (len) ? tmp : "<empty message>", (len > 44) ? "..." : "");
			iniMessages[j].name = messages[i];
			iniMessages[j].id = i - 1;
			j++;
		}

		if (j == 0) Alert("No messages in %s found.", tmpbuf);
		else if ((i = showButtons(iniMessages, j, "Pick INI message")) >= mrUser)
		{

			i-=mrUser;
			control->value = i;
			sprintf(gPaths.episodeIni, tmpbuf);
			break;
		}

	}

	memcpy((void*)frameplace, scrSave, xdim*ydim);
	Resource::Free(scrSave);
	return key;

}