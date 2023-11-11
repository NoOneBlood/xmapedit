#ifndef __XMP2DSCR
#define __XMP2DSCR
#include "gfx.h"
#include "xmpmisc.h"
#include "screen.h"
#include "inifile.h"

enum
{
kClrBase			= 0,
kClrEdBase			= kClrBase,
kClrEdBgd			= kClrEdBase,
kClrEdGrd,
kClrEdCurPos,
kClrEdStartPos,
kClrEdHgltPointB,
kClrEdHgltPointF,
kClrEdHgltSectB,
kClrEdHgltSectF,
kClrEdSprCountUnder,
kClrEdCircleFill,
kClrEdCircleCenter,
kClrEdHgltSectBox,
kClrEdWalNew,
kClrEdFirstVtxBrd,
kClrEdFirstVtxBgd,
kClrEdWallAngled,
kClrEdCaptWalNewT,
kClrEdCaptWalNewB,
kClrEdCaptWalFT,
kClrEdCaptWalFB,
kClrEdCaptWalAT,
kClrEdCaptWalAB,
kClrEdExpRad,
kClrEdTracerPath,
kClrEdTracerSlide,
kClrEdTracerWarp,
kClrEdAmbRad2On,
kClrEdAmbRad1On,
kClrEdAmbRad2Off,
kClrEdAmbRad1Off,
kClrEdMax,
//------------------------------------------
kClrSectBase		= kClrEdMax,
kClrSectFillHovr	= kClrSectBase,
kClrSectFillHglt,
kClrSectFillMerg,
kClrPrvSectOff,
kClrPrvSectOn,
kClrPrvSectMotFx,
kClrCaptSectT,
kClrCaptSectB,
kClrTracerSec,
kClrSectMax			= kClrTracerSec,
//------------------------------------------
kClrWalBase,
kClrWalNext,
kClrWalHscn,
kClrWalMveF,
kClrWalMveR,
kClrCaptWalT,
kClrCaptWalB,
kClrVtxBord,
kClrVtxFill,
kClrTracerWal,
kClrWalMax			= kClrTracerWal,
//------------------------------------------
kClrSprBase,
kClrSprHscn,
kClrSprInvs,
kClrSprMveF,
kClrSprMveR,
kClrSprDude,
kClrSprItem,
kClrSprMark,
kClrSprMarkPath,
kClrSprMarkOwn,
kClrSprMarkWarp,
kClrSprStealMore,
kClrSprStealLess,
kClrSprMissile1,
kClrSprMissile2,
kClrSprAmbOff,
kClrSprAmbOn,
kClrSprPly1,
kClrSprPly2,
kClrSprPly3,
kClrSprPly4,
kClrSprPly5,
kClrSprPly6,
kClrSprPly7,
kClrSprPly8,
kClrCaptSprTHscn,
kClrCaptSprTMveF,
kClrCaptSprT,
kClrTracerSpr,
kClrSprMax		= kClrTracerSpr,
kClrMax,
};

class SCREEN2D
{
	public:
		char colors[kClrMax][2];
		// ---------------------------------------------------------------------
		spritetype* pSpr; XSPRITE* pXSpr;
		QFONT* pCaptFont;
		QFONT* pEditFont;
		// ---------------------------------------------------------------------
		unsigned int HOVER		: 1;
		unsigned int BLINK		: 1;
		unsigned int THICK		: 1;
		unsigned int color		: 8;
		unsigned int vertexSize	: 8;
		unsigned int gridSize	: 32;
		signed   int x1, x2, x3;
		signed   int y1, y2, y3;
		// ---------------------------------------------------------------------
		struct
		{
			int wwh, whg;
			int wx1, wy1;
			int wx2, wy2;
			int wcx, wcy;
		}
		view;
		struct
		{
			unsigned int useTransluc		: 1;
			unsigned int ambRadius			: 4;
			unsigned int ambRadiusHover		: 1;
			unsigned int showSprites		: 4;
			unsigned int showFloorBorders	: 1;
			unsigned int showVertex			: 1;
			unsigned int showMap			: 4;
			unsigned int showHighlight		: 1;
			unsigned int showTags			: 3;
			unsigned int showFeatures		: 1;
		}
		prefs;
		struct
		{
			unsigned int sprUnderCursor		: 16;
			unsigned int grid				: 4;
			unsigned int ang 				: 12;
			int camx, camy;
			int zoom;
		}
		data;
		SCREEN2D() { memset(colors, 255, sizeof(colors)); }
		// ---------------------------------------------------------------------
		void SetView(int x1, int y1, int x2, int y2);
		// ---------------------------------------------------------------------
		void ColorInit(IniFile* pIni);
		void ColorFill(char which, char color, int nStart);
		char ColorGet(char which, char hover = 0);
		char GetWallColor(int nWall, char hover);
		char GetSpriteColor(spritetype* pSpr, char hover);
		// ---------------------------------------------------------------------
		void ScreenDraw(void);
		void ScreenClear(void);
		// ---------------------------------------------------------------------
		void DrawGrid(void);
		void DrawLine(int x1, int y1, int x2, int y2, char c, char b, int drawPat = kPatNormal);
		void DrawLine(int x1, int y1, int x2, int y2, char nColor);
		void DrawAngLine(int nScale, int nAng, int x1, int y1, char c, char b = false);
		void DrawIconFaceSpr_SMALL(int x, int y, char c);
		void DrawIconFaceSpr_NORMAL(int x, int y, char c);
		void DrawIconSpr(spritetype* pSpr, int x, int y, char c);
		void DrawIconMarker(int x, int y, char c);
		void DrawIconCross(int x, int y, char c, char s = 2);
		void DrawIconCross(int x, int y, int nAng, char c, char s = 2);
		void DrawCircle(int x, int y, int r, char c, char b = 0, int drawPat = kPatNormal);
		void DrawRect(int x1, int y1, int x2, int y2, char c, char b = 0, int drawPat = kPatNormal);
		void DrawSquare(int x, int y, char bc, int s);
		void DrawVertex(int x, int y, char bc, char fc, char s = 4);
		void DrawArrow(int xS, int yS, int xD, int yD, char c, int nZoom, int nAng = kAng30, char b = 0, int pat1 = kPatNormal, int pat2 = kPatNormal);
		void DrawArrow(int x, int y, char c, int nAng, int nZoom, int nSiz = 80, char b = false);
		// ---------------------------------------------------------------------
		void DrawSprite(spritetype* pSprite);
		void DrawSpritePathMarker(void);
		void DrawSpriteMarker(void);
		void DrawSpriteAmbient(void);
		void DrawSpriteStealth(void);
		void DrawSpriteFX(void);
		void DrawSpriteExplosion(void);
		void DrawSpriteProjectile(void);
		void DrawSpritePlayer(void);
		void DrawSpriteCommon(void);
		// ---------------------------------------------------------------------
		void DrawWall(int nWall);
		void DrawWallMidPoint(int nWall, char c, char s = 5, char thick = 0, char which = 0x01);
		void MarkWall(int nWall, int nMarkType);
		// ---------------------------------------------------------------------
		const char* CaptionGet(int nType, int nID);
		void CaptionPrint(const char* text, int cx, int cy, char pd, char fc, short bc, char shadow, QFONT* pFont);
		void CaptionPrintSector(int nSect, char hover, QFONT* pFont);
		void CaptionPrintSprite(int nSpr, char hover, QFONT* pFont);
		void CaptionPrintWall(int nWall, char hover, QFONT* pFont);
		void CaptionPrintWallEdit(int nWall, char fc, char bc, QFONT* pFont);
		// ---------------------------------------------------------------------
		void FillSector(int nSect, char c, char nStep = 2, int drawPat = kPatNormal);
		void FillPolygon(LINE2D* coords, int nCount, char c, char nStep = 2, int drawPat = kPatNormal);
		// ---------------------------------------------------------------------
		void ShowMap(char flags = 0x04);
		// ---------------------------------------------------------------------
		void GetPoint(int scrX, int scrY, int* posX, int* posY, char clamp);
		void GetPoint(int scrX, int scrY, int* posX, int* posY);
		void GetPoint(int* x, int* y);
		// ---------------------------------------------------------------------
		void LayerOpen(void);
		void LayerShow(char flags = 0);
		void LayerShowAndClose(char flags = 0);
		void LayerClose(void);
		// ---------------------------------------------------------------------
		char InHighlight(int x, int y);
		void ScaleAngLine(int scale, int ang, int* x, int* y);
		char OnScreen(int x1, int y1, int x2, int y2);
		inline char OnScreen(int x, int y, int s)	{ return OnScreen(x-s, y-s, x+s, y+s); }
		inline int cscalex(int a)				 	{ return view.wcx + mulscale14(a - data.camx, data.zoom); }
		inline int cscaley(int a)				 	{ return view.wcy + mulscale14(a - data.camy, data.zoom); }
};

extern SCREEN2D gScreen2D;
#endif