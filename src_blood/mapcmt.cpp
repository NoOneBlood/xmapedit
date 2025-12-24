/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022: Originally written by NoOne.
// Xmapedit map commentary system for level designers.
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

#include "edit2d.h"
#include "aadjust.h"
#include "mapcmt.h"
#include "xmpmaped.h"

short cmthglt = -1;
MAP_COMMENT_MGR gCommentMgr;

MAP_COMMENT_MGR::MAP_COMMENT_MGR()
{
    deFonts[0] = kFontSmall;
    deFonts[1] = kFontNormal;
    deFonts[2] = kFontLarge;

    cuFonts[0] = 0;     cuFonts[1] = 2;
    cuFonts[2] = 5;     cuFonts[3] = 4;
    cuFonts[4] = 7;

    commentsCount   = 0;
    comments        = NULL;
    CRC             = 0;
}

int MAP_COMMENT_MGR::LoadFromIni(IniFile* pFile)
{
    char id[3], *tmp; int i = 0, j; MAP_COMMENT comment;
    if (pFile->GetKeyInt("Header", "Version", 0) != kCommentVersion) return -1;
    else CRC = pFile->GetKeyInt("Header", "MapCRC", kDefaultCRC);
    while ( 1 )
    {
        Clear(&comment);
        sprintf(id, "%03d", ++i);
        if (!pFile->SectionExists(id))
            break;

        tmp = pFile->GetKeyString(id, "Text001", "");
        if (!strlen(tmp))
            continue;

        strncpy(comment.text, tmp, kCommentMaxLength - 1);

        tmp = pFile->GetKeyString(id, "Position", "");
        comment.cx = enumStrGetInt(0, tmp,  ',', kNoTail);
        comment.cy = enumStrGetInt(1, NULL, ',', kNoTail);
        if (pFile->KeyExists(id, "Object"))
        {
            tmp = pFile->GetKeyString(id, "Object", "");
            comment.objType = enumStrGetInt(0, tmp, ',', -1);
            comment.objIdx  = enumStrGetInt(1, NULL, ',', -1);
            comment.tx = comment.cx;
            comment.ty = comment.cy;
        }
        else
        {
            comment.tx = enumStrGetInt(2, NULL, ',', kNoTail);
            comment.ty = enumStrGetInt(3, NULL, ',', kNoTail);
        }

        tmp = pFile->GetKeyString(id, "Appearance", "");
        comment.foreColor   = enumStrGetInt(0, tmp, ',', clr2std(kColorYellow));
        comment.backColor   = enumStrGetInt(1, NULL, ',', -1);
        comment.thickTail   = enumStrGetInt(2, NULL, ',', 0);
        comment.fontID      = enumStrGetInt(3, NULL, ',', kDefaultFont);
        
        if (!rngok(comment.fontID, 0, LENGTH(cuFonts)))
            comment.fontID = kDefaultFont;
        
        Format(&comments[Add(&comment)]);
    }

    return commentsCount;

}

int MAP_COMMENT_MGR::LoadFromIni(char* filename)
{
    char temp[_MAX_PATH];
    int retn = 0;

    sprintf(temp, filename);
    ChangeExtension(temp, kCommentExt);

    IniFile* pComment = new IniFile(temp);
    retn = LoadFromIni(pComment);
    delete(pComment);

    return retn;
}

int MAP_COMMENT_MGR::SaveToIni(IniFile* pFile)
{
    int i, j = 0;
    char id[16], tmp[256]; MAP_COMMENT* cmt;

    pFile->PutKeyInt("Header", "Version", kCommentVersion);
    if (CRC != kDefaultCRC)
        pFile->PutKeyInt("Header", "MapCRC",  CRC);

    for (i = 0; i < commentsCount; i++)
    {
        cmt =& comments[i];
        sprintf(id, "%03d", i+1);

        if (cmt->objIdx >= 0)
        {
            sprintf(tmp, "(%d, %d)", cmt->cx, cmt->cy);
            pFile->PutKeyString(id, "Position", tmp);

            sprintf(tmp, "(%d, %d)", cmt->objType, cmt->objIdx);
            pFile->PutKeyString(id, "Object", tmp);
        }
        else
        {
            if (cmt->tx == kNoTail) sprintf(tmp, "(%d, %d)", cmt->cx, cmt->cy);
            else sprintf(tmp, "(%d, %d, %d, %d)", cmt->cx, cmt->cy, cmt->tx, cmt->ty);
            pFile->PutKeyString(id, "Position", tmp);
        }

        pFile->PutKeyString(id, "Text001", cmt->text);

        sprintf(tmp, "(%d, %d, %d, %d)", cmt->foreColor, cmt->backColor, cmt->thickTail, cmt->fontID);
        pFile->PutKeyString(id, "Appearance", tmp);

    }

    if (i > 0)
        pFile->Save();

    return i;

}

int MAP_COMMENT_MGR::SaveToIni(char* filename)
{
    char temp[_MAX_PATH];
    int retn = 0;

    sprintf(temp, filename);
    ChangeExtension(temp, kCommentExt);
    if (fileExists(temp))
        unlink(temp);

    if (commentsCount > 0)  // save comments
    {
        IniFile* pFile = new IniFile(temp);
        retn = SaveToIni(pFile);
        delete pFile;
    }

    return retn;
}

int MAP_COMMENT_MGR::GetCRC()
{
    return CRC;
}

void MAP_COMMENT_MGR::SetCRC(int nCRC)
{
    CRC = nCRC;
}

BOOL MAP_COMMENT_MGR::CompareCRC(int nCRC)
{
    if (CRC == kDefaultCRC) return TRUE;
    return (CRC == nCRC);
}

void MAP_COMMENT_MGR::Clear(MAP_COMMENT* cmt)
{
    cmt->id = 0;
    cmt->cx = kNoTail;
    cmt->cy = kNoTail;
    cmt->tx = kNoTail;
    cmt->ty = kNoTail;

    memset(cmt->width, 0, sizeof(cmt->width));
    memset(cmt->heigh, 0, sizeof(cmt->heigh));

    cmt->objType = -1;
    cmt->objIdx  = -1;

    cmt->foreColor = clr2std(kColorYellow);
    cmt->backColor = -1;
    cmt->thickTail = 0;
    cmt->fontID    = 1;

    memset(cmt->text, 0, kCommentMaxLength);

}

int MAP_COMMENT_MGR::ClosestToPoint(int nTresh, int x, int y, int zoome)
{
    int i, d, closest, n = -1;
    int x1, y1, x2, y2;
    int wh, hg;

    closest = divscale14(nTresh, zoome);
    for (i = 0; i < commentsCount; i++)
    {
        MAP_COMMENT* cmt = &comments[i];
        FormatGetByZoom(cmt, zoome, &wh, &hg);
        hg = divscale14(ClipLow(hg, 20), zoome) >> 1;
        wh = divscale14(wh, zoome) >> 1;

        x1 = cmt->cx - wh;  x2 = cmt->cx + wh;
        y1 = cmt->cy - hg;  y2 = cmt->cy + hg;
        if (x >= x1 && x <= x2 && y >= y1 && y <= y2) return i; // body
        else if (cmt->tx != kNoTail && (d = approxDist(x - cmt->tx, y - cmt->ty)) < closest) // tail
        {
            closest = d;
            n = i | 0x4000;
        }
    }

    return n;
}



enum {
kMono           = 0x1,
kHColor         = 0x2,
kVColor         = 0x4,
};

int fontPicker(int* fontList, int len, char* title, int nDefault = -1, char* sample = NULL, int flags = kMono | kVColor | kHColor)
{
    char sampleText[256];
    if (sample && strlen(sample)) strncpy(sampleText, sample, 255);
    else sprintf(sampleText, "The quick brown fox jumps fast...");
    int i, j, k, dx1, dy1, dx2, dy2, dw, dh, dwh, dhg;
    int widest = 0, tallest = 0, stlen; QFONT* font;
    int x, y, bh = 40;
    int* fonts = (int*)malloc(sizeof(int)*len);

    stlen = strlen(sampleText);
    for (i = j = 0; i < len; i++)
    {
        font = qFonts[fontList[i]];

        if (!font) continue;
        else if ((flags & kMono) && font->type == kFontTypeMono);
        else if ((flags & kHColor) && font->type == kFontTypeRasterHoriz);
        else if ((flags & kVColor) && font->type == kFontTypeRasterVert);
        else continue;

        fonts[j++] = fontList[i];
        if ((k = stlen*font->width) > widest)
            widest = k;
    }

    if ((len = j) <= 0)
    {
        free(fonts);
        return nDefault;
    }

    dw = ClipHigh(widest+14, xdim - 10);
    dh = ClipHigh((bh*len)+60, ydim - 10);

    Window dialog(0, 0, dw, dh, title);
    dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
    dialog.getSize(&dwh, &dhg);

    y = dy1;
    for (i = 0; i < len; i++)
    {
        j = fonts[i];
        if (!qFonts[j]) continue;
        TextButton* pFButton = new TextButton(dx1, y, widest, bh, sampleText, j + mrUser);
        pFButton->font = qFonts[j];
        dialog.Insert(pFButton);
        y+=bh;
    }

    TextButton* pDfButton = new TextButton(dx1, dy2-30, dx2-98, 30, "&Use default font", nDefault + mrUser);
    TextButton* pCnButton = new TextButton(dx2-90, dy2-30, 90, 30, "&Cancel", mrCancel);

    if (nDefault >= 0)
    {
        pDfButton->fontColor = kColorBlue;
    }
    else
    {
        pDfButton->fontColor = kColorDarkGray;
        pDfButton->canFocus = 0;
        pDfButton->disabled = 1;
    }

    pCnButton->fontColor = kColorRed;

    dialog.Insert(pDfButton);
    dialog.Insert(pCnButton);

    ShowModal(&dialog);
    free(fonts);

    if (dialog.endState - mrUser >= 0) return dialog.endState - mrUser;
    else if (dialog.endState == mrOk && dialog.focus) // find a button we are focusing on
    {
        Container* pCont = (Container*)dialog.focus;
        TextButton* pFocus = (TextButton*)pCont->focus;
        if (pFocus && pFocus->result >= mrUser)
            return pFocus->result - mrUser;
    }

    return -1;
}

int MAP_COMMENT_MGR::ShowDialog(int xpos1, int ypos1, int xpos2, int ypos2, int cmtID)
{
    char temp[256], title[64];
    int dw, dx1, dy1, dx2, dy2, dwh, dhg, i;
    int backCmtID = cmtID, foreColor, backColor, fontID;
    int x = 0, y = 0;

    BOOL standalone, arrow, thick, edit = (cmtID >= 0);
    MAP_COMMENT newcmt, *cmt = (edit) ? &comments[cmtID] : &newcmt;
    BYTE colors[256];

    memset(temp,   0, sizeof(temp));
    memset(colors, 1, sizeof(colors));

    if (edit)
    {
        sprintf(title, "Edit comment");
        standalone = (cmt->objType < 0 && cmt->objIdx < 0);
        arrow = (!standalone || cmt->tx != kNoTail);
        sprintf(temp, cmt->text);
        foreColor = cmt->foreColor;
        backColor = cmt->backColor;
        thick = cmt->thickTail;
        fontID = cmt->fontID;
    }
    else
    {
        sprintf(title, "Write a new comment");
        foreColor =  clr2std(kColorYellow);
        backColor = -1;
        standalone = (qsetmode != 200);
        arrow = TRUE;
        thick = FALSE;
        fontID = kDefaultFont;
    }

    dw = ClipHigh(630 + (xdim >> 3), xdim - 10);

    while ( 1 )
    {
        x = 0;
        Window dialog((xdim-dw)>>1, ydim-100, dw, 74, title);
        dialog.getEdges(&dx1, &dy1, &dx2, &dy2);
        dialog.getSize(&dwh, &dhg);

        EditText* pText = new EditText(dx1, dy1, dwh, 20, temp, 0x01);
        sprintf(pText->placeholder, "Enter comment text...");
        pText->maxlen = kCommentMaxLength;

        Panel* pButtons = new Panel(dx1, dy1+pText->height, dwh, 30);

        TextButton* pOk = new TextButton(x, 0, 60, 30, "&Confirm", mrOk);
        x+=pOk->width;

        TextButton* pFnButton = new TextButton(x, 0, 60, 30, "Font", 106);
        x+=pFnButton->width;

        TextButton* pFrButton = new TextButton(x, 0, 100, 20, "&Foreground", 100);
        Shape* pFrContr = new Shape(x+2, 22, 97, 6, 0);
        Shape* pFrShape = new Shape(x+3, 23, 95, 4, foreColor);
        x+=pFrButton->width;

        TextButton* pBgButton = new TextButton(x, 0, 100, 20, "&Background", 101);
        Shape* pBgContr = new Shape(x+2, 22, 97, 6, 0);
        Shape* pBgShape = new Shape(x+3, 23, 95, 4, (backColor >= 0) ? backColor : kColorBackground);
        x+=pBgButton->width;

        Checkbox* pStand  = new Checkbox(x+2, 3,   standalone, "&Standalone comment", 102);
        Checkbox* pArrow  = new Checkbox(x+2, 16,  arrow, "With &tracer", 103);
        Checkbox* pThick  = new Checkbox(x+92, 16, thick, "t&hick", 104);

        x = dwh-60;
        TextButton* pCancel = new TextButton(x, 0, 60, 30, "&Quit", mrCancel);
        x-=pCancel->width;

        TextButton* pDelete = new TextButton(x, 0, 60, 30, "&Delete", 105);
        x-=pDelete->width;

        pFnButton->font = qFonts[fontID];

        // no objects around to bind comment
        if (searchstat == 255 && ((edit && cmt->objType < 0) || !edit))
        {
            pStand->disabled = 1;
            standalone = 1;
        }
        else
        {
            pStand->disabled  = 0;
        }

        pOk->fontColor      = kColorBlue;
        pCancel->fontColor  = kColorRed;
        if (!edit)
        {
            pDelete->fontColor  = kColorDarkGray;
            pDelete->disabled   = 1;
            pDelete->canFocus   = 0;
        }
        else
        {
            pDelete->fontColor  = kColorMagenta;
        }

        if (!arrow)
            pThick->disabled = 1;

        pArrow->disabled = !standalone; // if binded, mark this checked always
        pArrow->canFocus = !pArrow->disabled;
        pStand->canFocus = !pStand->disabled;
        pThick->canFocus = !pThick->disabled;

        pButtons->Insert(pOk);
        pButtons->Insert(pFnButton);

        pButtons->Insert(pFrButton);
        pButtons->Insert(pFrContr);
        pButtons->Insert(pFrShape);

        pButtons->Insert(pBgButton);
        pButtons->Insert(pBgContr);
        pButtons->Insert(pBgShape);

        pButtons->Insert(pStand);
        pButtons->Insert(pArrow);
        pButtons->Insert(pThick);
        pButtons->Insert(pDelete);
        pButtons->Insert(pCancel);

        dialog.Insert(pText);
        dialog.Insert(pButtons);

        ShowModal(&dialog);

        if (pText->len)
            sprintf(temp, pText->string);

        switch (dialog.endState) {
            case mrOk:
                if (!strlen(pText->string))
                {
                    Alert("Comment empty.");
                    continue;
                }
                else
                {
                    if (!edit)
                    {
                        cmtID = Add(cmt);
                        cmt = &comments[cmtID];
                        Clear(cmt);

                        SetXYBody(cmtID, xpos1, ypos1);
                    }

                    cmt->id        = cmtID;
                    cmt->foreColor = foreColor;
                    cmt->backColor = backColor;
                    cmt->thickTail = thick;
                    cmt->fontID    = fontID;

                    sprintf(cmt->text, pText->string);
                    Format(cmt);

                    if (!standalone)
                    {
                        i = -1;
                        if (cmt->objType < 0 && (i = ShowBindMenu(cmtID, xpos2, ypos2)) == -2)
                        {
                            if (!edit)
                                Delete(cmt), cmt =& newcmt;

                            continue;
                        }

                        standalone = (cmt->objType < 0);
                    }

                    if (standalone)
                    {
                        if (cmt->objType >= 0)
                            Unbind(cmtID);

                        if (!arrow) SetXYTail(cmtID, kNoTail, kNoTail);
                        else if (cmt->tx == kNoTail)
                            SetXYTail(cmtID, cmt->cx+512, cmt->cy+512);
                    }
                }
                return 0;
            case 100:
                foreColor = colorPicker((BYTE*)colors, "Pick text color", foreColor);
                continue;
            case 101:
                backColor = colorPicker((BYTE*)colors, "Pick background color", -1, 0x01);
                continue;
            case 102:
                standalone = !standalone;
                if (!standalone) arrow = 1;
                continue;
            case 103:
                if (!standalone) arrow = 1;
                else arrow = !arrow;
                continue;
            case 104:
                thick = !thick;
                continue;
            case 105:
                if (!Confirm("Delete this comment?")) continue;
                Delete(cmt);
                return 0;
            case 106:
                if ((i = fontPicker(cuFonts, LENGTH(cuFonts), "Pick font", kDefaultFont)) < 0) continue;
                fontID = i;
                continue;
            case mrCancel:
                return -1;
        }

        break;
    }

    return 0;
}

int MAP_COMMENT_MGR::Add(MAP_COMMENT* cmt)
{
    cmt->id  = commentsCount;
    comments = (MAP_COMMENT*)realloc(comments, sizeof(MAP_COMMENT)*(commentsCount+1));
    memcpy(&comments[commentsCount++], cmt, sizeof(MAP_COMMENT));
    return cmt->id;
}

int MAP_COMMENT_MGR::Delete(MAP_COMMENT* cmt)
{
    int i, j = cmt->id;
    for (i = 0; i < commentsCount; i++)
    {
        if (comments[i].id != j) continue;
        while (i < commentsCount - 1)
        {
            comments[i] = comments[i+1];
            comments[i].id--;
            i++;
        }
        break;
    }

    if (--commentsCount <= 0)
    {
        free(comments);
        comments = NULL;
    }
    else
    {
        comments = (MAP_COMMENT*)realloc(comments, sizeof(MAP_COMMENT)*commentsCount);
    }

    return commentsCount;

}

void MAP_COMMENT_MGR::DeleteAll()
{
    commentsCount = 0;
    if (comments)
    {
        free(comments);
        comments = NULL;
    }
}



void MAP_COMMENT_MGR::FormatGetByZoom(MAP_COMMENT* cmt, int zoome, int *wh, int *hg, int *font)
{
    if (cmthglt == cmt->id || (cmthglt & 0x3FFF) == cmt->id)
    {
        if (cmt->fontID != kDefaultFont)
        {
            if (font) *font = cmt->fontID;
            if (wh) *wh = cmt->width[0];
            if (hg) *hg = cmt->heigh[0];
        }
        else
        {
            if (font) *font = kFontNormal;
            if (wh) *wh = cmt->width[1];
            if (hg) *hg = cmt->heigh[1];
        }
    }
    else if (cmt->fontID != kDefaultFont)
    {
        if (font) *font = cmt->fontID;
        if (wh) *wh = cmt->width[0];
        if (hg) *hg = cmt->heigh[0];
    }
    else if (zoome <= 0x0100)
    {
        if (font) *font = kFontSmall;
        if (wh) *wh = cmt->width[0];
        if (hg) *hg = cmt->heigh[0];
    }
    else if (zoome >= 0x1000)
    {
        if (font) *font = kFontLarge;
        if (wh) *wh = cmt->width[2];
        if (hg) *hg = cmt->heigh[2];
    }
    else
    {
        if (font) *font = kFontNormal;
        if (wh) *wh = cmt->width[1];
        if (hg) *hg = cmt->heigh[1];
    }
}

void MAP_COMMENT_MGR::Format(MAP_COMMENT* cmt)
{
    #define kMaxWidth 50

    int i; QFONT* pFont;
    if (cmt->fontID == kDefaultFont)
    {
        for (i = 0; i < LENGTH(deFonts); i++)
        {
            pFont = qFonts[deFonts[i]];
            cmt->width[i] = gfxGetTextLen(cmt->text, pFont);
            cmt->heigh[i] = pFont->height;
        }
    }
    else
    {
        pFont = qFonts[cmt->fontID];
        cmt->width[0] = gfxGetTextLen(cmt->text, pFont);
        cmt->heigh[0] = pFont->height;
    }
}

void MAP_COMMENT_MGR::SetXYBody(int cmtID, int x, int y)
{
    comments[cmtID].cx = x;
    comments[cmtID].cy = y;
}

void MAP_COMMENT_MGR::SetXYTail(int cmtID, int x, int y)
{
    comments[cmtID].tx = x;
    comments[cmtID].ty = y;
}

void MAP_COMMENT_MGR::GetXYTail(int cmtID, int* x, int* y)
{
    *x = comments[cmtID].tx;
    *y = comments[cmtID].ty;
}

void MAP_COMMENT_MGR::RemoveTail(int cmtID)
{
    comments[cmtID].tx = kNoTail;
    comments[cmtID].ty = kNoTail;
}

void MAP_COMMENT_MGR::ResetAllTails()
{
    MAP_COMMENT* cmt; int i;
    for (i = 0; i < commentsCount; i++)
    {
        cmt = &comments[i];
        cmt->objType = cmt->objIdx = -1;
        if (cmt->tx != kNoTail)
        {
            //Alert("%d / %d", cmt->cx, cmt->ty);
            SetXYTail(i, cmt->cx+512, cmt->cy+512);
        }
    }
}

void MAP_COMMENT_MGR::Unbind(int cmtID)
{

    int objType, objIdx;
    MAP_COMMENT* cmt =&comments[cmtID];
    objType = cmt->objType, objIdx  = cmt->objIdx;

    cmt->objType = cmt->objIdx = -1;

    switch (objType) {
        case OBJ_SECTOR:
            if (objIdx < 0 || objIdx >= kMaxXSectors) break;
            else if (xsector[objIdx].reference < 0) break;
            else if (!obsoleteXObject(objType, objIdx)) break;
            dbDeleteXSector(objIdx);
            CleanUp();
            break;
        case OBJ_WALL:
            if (objIdx < 0 || objIdx >= kMaxXWalls) break;
            else if (xwall[objIdx].reference < 0) break;
            else if (!obsoleteXObject(objType, objIdx)) break;
            dbDeleteXWall(objIdx);
            CleanUp();
            break;
    }


}

int MAP_COMMENT_MGR::RebindMatching(int srcType, int srcIdx, int destType, int destIdx, BOOL once)
{
    int i, cnt; MAP_COMMENT* cmt;
    for (i = 0, cnt = 0; i < commentsCount; i++)
    {
        cmt = &comments[i];
        if (once && cmt->initRebind) continue;
        else if (cmt->objType != srcType || cmt->objIdx != srcIdx)
            continue;

        SetXYTail(i, cmt->cx, cmt->cy);
        cmt->objType = destType;
        cmt->objIdx  = destIdx;
        cmt->initRebind = 1;
        cnt++;
    }

    return cnt;
}

void MAP_COMMENT_MGR::BindTo(int cmtID, int objType, int objIdx)
{
    dassert(cmtID >= 0 && cmtID < commentsCount);

    MAP_COMMENT* cmt = &comments[cmtID];
    cmt->objType = objType;

    // must use xobjects for now :(
    switch(objType) {
        case OBJ_WALL:
            objIdx = GetXWall(objIdx);
            break;
        case OBJ_SECTOR:
            objIdx = GetXSector(objIdx);
            break;
    }

    cmt->objIdx = objIdx;
    SetXYTail(cmtID, cmt->cx, cmt->cy);
}
int MAP_COMMENT_MGR::IsBind(int objType, int objIdx)
{
    int i; MAP_COMMENT* cmt;
    for (i = 0; i < commentsCount; i++)
    {
        cmt =& comments[i];
        if (cmt->objType == OBJ_SPRITE && sprite[cmt->objIdx].index == objIdx) return i;
        else if (cmt->objType == OBJ_WALL && cmt->objIdx == objIdx) return i;
        else if (cmt->objType == OBJ_SECTOR && cmt->objIdx == objIdx) return i;
    }

    return -1;
}

int MAP_COMMENT_MGR::IsBind(int cmtID, int* objIdx)
{
    MAP_COMMENT* cmt = &comments[cmtID];
    switch(cmt->objType) {
        case OBJ_SPRITE:
            if (objIdx) *objIdx = cmt->objIdx;
            break;
        case OBJ_WALL:
            if (objIdx) *objIdx = xwall[cmt->objIdx].reference;
            break;
        case OBJ_SECTOR:
            if (objIdx) *objIdx = xsector[cmt->objIdx].reference;
            break;
    }

    return cmt->objType;
}

void MAP_COMMENT_MGR::Cleanup() {

    int i, j, obj, idx; MAP_COMMENT* cmt;
    for (i = 0; i < commentsCount; i++)
    {
        cmt =& comments[i];
        obj = cmt->objType, idx = cmt->objIdx;

        switch (obj) {
            case OBJ_SPRITE:
                if (rngok(idx, 0, kMaxSprites) && sprite[idx].statnum < kMaxStatus) break;
                Unbind(cmt->id);
                break;
            case OBJ_WALL:
                if (rngok(idx, 0, kMaxXWalls) && xwall[idx].reference >= 0) break;
                Unbind(cmt->id);
                break;
            case OBJ_SECTOR:
                if (rngok(idx, 0, kMaxXSectors) && xsector[idx].reference >= 0) break;
                Unbind(cmt->id);
                break;
        }

        cmt->id = i;
    }

}

int MAP_COMMENT_MGR::ShowBindMenu(int cmtID, int xpos, int ypos)
{
    short sect = -1;
    int objType[3], objIdx[3], i, j = 0;
    int point = -1, line = -1;

    char tmp[] = "Bind it to %s #%d";
    NAMED_TYPE menu[6]; char names[5][32];
    MAP_COMMENT* cmt =& comments[cmtID];

    if (qsetmode != 200)
    {
        point = getpointhighlight(gMisc.hgltTreshold, xpos, ypos, zoom);
        line  = getlinehighlight(gMisc.hgltTreshold, xpos, ypos, zoom);
        updatesector(xpos, ypos, &sect);
    }
    else
    {
        sect = searchsector;
        switch (searchstat) {
            case OBJ_WALL:
            case OBJ_MASKED:
                line  = searchwall2;
                break;
            case OBJ_SPRITE:
                point = searchwall | 0x4000;
                break;
        }
    }

    if (point >= 0 && (point & 0xc000) != 0)
    {
        i = point & 0x3FFF;
        if (IsBind(OBJ_SPRITE, i) != cmtID)
        {
            sprintf(names[j], tmp, gSearchStatNames[OBJ_SPRITE], i);
            menu[j].name = names[j]; menu[j].id = j;
            objType[j] = OBJ_SPRITE;
            objIdx[j] = i;
            j++;
        }
    }

    if (line >= 0 || (point >= 0 && (point & 0xc000) == 0))
    {
        i = (line >= 0) ? line : point;
        if (IsBind(OBJ_WALL, wall[i].extra) != cmtID)
        {
            sprintf(names[j], tmp, gSearchStatNames[OBJ_WALL], i);
            menu[j].name = names[j]; menu[j].id = j;
            objType[j] = OBJ_WALL;
            objIdx[j] = i;
            j++;
        }
    }

    if (sect >= 0)
    {
        i = sect;
        if (IsBind(OBJ_SECTOR, sector[i].extra) != cmtID)
        {
            sprintf(names[j], tmp, gSearchStatNames[OBJ_SECTOR], i);
            menu[j].name = names[j]; menu[j].id = j;
            objType[j] = OBJ_SECTOR;
            objIdx[j] = i;
            j++;
        }
    }

    if (cmt->objType >= 0)
    {
        menu[j].name = names[j]; menu[j].id = 100;
        sprintf(names[j], "&Unbind comment");
        j++;
    }
    else if (cmt->tx != kNoTail)
    {
        menu[j].name = names[j]; menu[j].id = 100;
        sprintf(names[j], "&Remove tracer");
        j++;
    }

    menu[j].name = names[j]; menu[j].id = 200;
    sprintf(names[j], "&Cancel");
    j++;

    if (j > 1)
    {
        if ((j = showButtons(menu, j, "Select an object to bind comment...") - mrUser) >= 0)
        {
            switch (j) {
                case 100:
                    if (cmt->objType < 0) RemoveTail(cmtID);
                    else Unbind(cmtID);
                    return -1;
                case 200:
                    break;
                default:
                    if (cmt->objType >= 0) Unbind(cmtID);
                    BindTo(cmtID, objType[j], objIdx[j]);
                    return 0;
            }
        }
    }

    return -2;
}


void MAP_COMMENT_MGR::Draw(SCREEN2D* pScr)
{
    MAP_COMMENT* cmt;
    int i, x1, y1, x2, y2, objType, objIdx;
    int fontID, foreColor, backColor, wh, hg;
    int nZoom = pScr->data.zoom;

    for (i = 0; i < commentsCount; i++)
    {
        cmt = &comments[i];
        FormatGetByZoom(cmt, nZoom, &wh, &hg, &fontID);

        x1 = x2 = pScr->cscalex(cmt->cx);
        y1 = y2 = pScr->cscaley(cmt->cy);

        backColor   = cmt->backColor;
        foreColor   = cmt->foreColor;

        objType     = cmt->objType;
        objIdx      = cmt->objIdx;

        if ((cmthglt == i || (cmthglt & 0x3FFF) == i))
            foreColor = fade();

        if (cmt->tx != kNoTail)
        {
            if (objType >= 0)
                UpdateTailCoords(cmt);

            x2 = pScr->cscalex(cmt->tx);
            y2 = pScr->cscaley(cmt->ty);
            if (y2 > y1)
            {
                y1+=hg>>1;
                y2-=hg>>1;
            }

            pScr->DrawArrow(x1, y1, x2, y2, foreColor, nZoom >> 2, (objType < 0) ? 130 : 0, cmt->thickTail, kPatDotted);
        }

        pScr->CaptionPrint(cmt->text, x1, y1, 2, foreColor, backColor, (backColor < 0), qFonts[fontID]);
    }
}

void MAP_COMMENT_MGR::UpdateTailCoords(MAP_COMMENT* cmt)
{
    switch (cmt->objType) {
        case OBJ_WALL:
            avePointWall(xwall[cmt->objIdx].reference, &cmt->tx, &cmt->ty);
            break;
        case OBJ_SPRITE:
            cmt->tx = sprite[cmt->objIdx].x;
            cmt->ty = sprite[cmt->objIdx].y;
            break;
        case OBJ_SECTOR:
            avePointSector(xsector[cmt->objIdx].reference, &cmt->tx, &cmt->ty);
            break;
    }

}