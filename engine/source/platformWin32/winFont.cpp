//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platformWin32/winFont.h"
#include "gfx/gNewFont.h"
#include "gfx/gFont.h"
#include "gfx/gBitmap.h"
#include "math/mRect.h"
#include "console/console.h"
#include "core/unicode.h"

static HDC fontHDC = NULL;
static HBITMAP fontBMP = NULL;

static U32 charsetMap[] =
{
    ANSI_CHARSET,
    SYMBOL_CHARSET,
    SHIFTJIS_CHARSET,
    HANGEUL_CHARSET,
    HANGUL_CHARSET,
    GB2312_CHARSET,
    CHINESEBIG5_CHARSET,
    OEM_CHARSET,
    JOHAB_CHARSET,
    HEBREW_CHARSET,
    ARABIC_CHARSET,
    GREEK_CHARSET,
    TURKISH_CHARSET,
    VIETNAMESE_CHARSET,
    THAI_CHARSET,
    EASTEUROPE_CHARSET,
    RUSSIAN_CHARSET,
    MAC_CHARSET,
    BALTIC_CHARSET,
};
#define NUMCHARSETMAP   (sizeof(charsetMap) / sizeof(U32))

void createFontInit(void);
void createFontShutdown(void);
//void CopyCharToBitmap(GBitmap* pDstBMP, HDC hSrcHDC, const RectI& r);

void createFontInit()
{
    fontHDC = CreateCompatibleDC(NULL);
    fontBMP = CreateCompatibleBitmap(fontHDC, 256, 256);
}

void createFontShutdown()
{
    DeleteObject(fontBMP);
    DeleteObject(fontHDC);
}

/*void CopyCharToBitmap(GBitmap* pDstBMP, HDC hSrcHDC, const RectI& r)
{
    for (S32 i = r.point.y; i < r.point.y + r.extent.y; i++)
    {
        for (S32 j = r.point.x; j < r.point.x + r.extent.x; j++)
        {
            COLORREF color = GetPixel(hSrcHDC, j, i);
            if (color)
                *pDstBMP->getAddress(j, i) = 255;
            else
                *pDstBMP->getAddress(j, i) = 0;
        }
    }
}*/

/*GOldFont* createFont(const char* name, dsize_t size, U32 charset *//* = TGE_ANSI_CHARSET *//*)
{
    if (!name)
        return NULL;
    if (size < 1)
        return NULL;

    if (charset > NUMCHARSETMAP)
        charset = TGE_ANSI_CHARSET;

#ifdef UNICODE
    UTF16 n[512];
    convertUTF8toUTF16((UTF8*)name, n, sizeof(n));
    HFONT hNewFont = CreateFont(size, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, n);
#else
    HFONT hNewFont = CreateFont(size, 0, 0, 0, 0, 0, 0, 0, charsetMap[charset], 0, 0, 0, 0, name);
#endif
    if (!hNewFont)
        return NULL;

    GOldFont* retFont = new GOldFont;
    static U8 scratchPad[65536];

    TEXTMETRIC textMetric;
    COLORREF backgroundColorRef = RGB(0, 0, 0);
    COLORREF foregroundColorRef = RGB(255, 255, 255);

    SelectObject(fontHDC, fontBMP);
    SelectObject(fontHDC, hNewFont);
    SetBkColor(fontHDC, backgroundColorRef);
    SetTextColor(fontHDC, foregroundColorRef);
    GetTextMetrics(fontHDC, &textMetric);
    MAT2 matrix;
    GLYPHMETRICS metrics;
    RectI clip;

    FIXED zero;
    zero.fract = 0;
    zero.value = 0;
    FIXED one;
    one.fract = 0;
    one.value = 1;

    matrix.eM11 = one;
    matrix.eM12 = zero;
    matrix.eM21 = zero;
    matrix.eM22 = one;
    S32 glyphCount = 0;

    for (S32 i = 32; i < 256; i++)
    {
#ifdef UNICODE
        UTF16 c[2];
        const UTF8 t[] = { (const UTF8)i, 0 };

        convertUTF8toUTF16(t, c, sizeof(c));
#endif

        if (GetGlyphOutline(
            fontHDC,	// handle of device context 
#ifdef UNICODE
            c[0],  // character to query 
#else
            i,	// character to query 
#endif
            GGO_GRAY8_BITMAP,	// format of data to return 
            &metrics,	// address of structure for metrics 
            sizeof(scratchPad),	// size of buffer for data 
            scratchPad,	// address of buffer for data 
            &matrix 	// address of transformation matrix structure  
        ) != GDI_ERROR)
        {
            glyphCount++;
            U32 rowStride = (metrics.gmBlackBoxX + 3) & ~3; // DWORD aligned
            U32 size = rowStride * metrics.gmBlackBoxY;
            for (U32 j = 0; j < size; j++)
            {
                U32 pad = U32(scratchPad[j]) << 2;
                if (pad > 255)
                    pad = 255;
                scratchPad[j] = pad;
            }
            S32 inc = metrics.gmCellIncX;
            if (inc < 0)
                inc = -inc;
            retFont->insertBitmap(i, scratchPad, rowStride, metrics.gmBlackBoxX, metrics.gmBlackBoxY, metrics.gmptGlyphOrigin.x, metrics.gmptGlyphOrigin.y, metrics.gmCellIncX);
        }
        else
        {
#ifdef UNICODE
            UTF16 b = i;
#else
            char b = i;
#endif
            SIZE size;
            GetTextExtentPoint32(fontHDC, &b, 1, &size);
            if (size.cx)
                retFont->insertBitmap(i, scratchPad, 0, 0, 0, 0, 0, size.cx);
        }
    }
    retFont->pack(textMetric.tmHeight, textMetric.tmAscent);
    //clean up local vars      
    DeleteObject(hNewFont);

    if (!glyphCount)
        Con::errorf(ConsoleLogEntry::General, "Error creating font: %s %d", name, size);

    return retFont;
}*/

//////////////////////////////////////////////////////////////////////////
// WinFont class
//////////////////////////////////////////////////////////////////////////

PlatformFont* createPlatformFont(const char* name, U32 size, U32 charset /* = TGE_ANSI_CHARSET */)
{
    PlatformFont* retFont = new WinFont;

    if (retFont->create(name, size, charset))
        return retFont;

    delete retFont;
    return NULL;
}

WinFont::WinFont() : mFont(NULL)
{
}

WinFont::~WinFont()
{
    if (mFont)
    {
        DeleteObject(mFont);
    }
}

bool WinFont::create(const char* name, U32 size, U32 charset /* = TGE_ANSI_CHARSET */)
{
    if (name == NULL || size < 1)
        return false;

    if (charset > NUMCHARSETMAP)
        charset = TGE_ANSI_CHARSET;

#ifdef UNICODE
    UTF16 n[512];
    convertUTF8toUTF16((UTF8*)name, n, sizeof(n));
    mFont = CreateFont(size, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, 0, PROOF_QUALITY, 0, n);
#else
    mFont = CreateFont(size, 0, 0, 0, 0, 0, 0, 0, charsetMap[charset], OUT_TT_PRECIS, 0, PROOF_QUALITY, 0, name);
#endif
    if (mFont == NULL)
        return false;

    SelectObject(fontHDC, fontBMP);
    SelectObject(fontHDC, mFont);
    GetTextMetrics(fontHDC, &mTextMetric);

    return true;
}

bool WinFont::isValidChar(const UTF16 ch) const
{
    return ch != 0 /* && (ch >= mTextMetric.tmFirstChar && ch <= mTextMetric.tmLastChar)*/;
}

bool WinFont::isValidChar(const UTF8* str) const
{
    return isValidChar(oneUTF8toUTF32(str));
}


PlatformFont::CharInfo& WinFont::getCharInfo(const UTF16 ch) const
{
    static PlatformFont::CharInfo c;

    dMemset(&c, 0, sizeof(c));
    c.bitmapIndex = -1;

    static U8 scratchPad[65536];

    COLORREF backgroundColorRef = RGB(0, 0, 0);
    COLORREF foregroundColorRef = RGB(255, 255, 255);
    SelectObject(fontHDC, fontBMP);
    SelectObject(fontHDC, mFont);
    SetBkColor(fontHDC, backgroundColorRef);
    SetTextColor(fontHDC, foregroundColorRef);

    MAT2 matrix;
    GLYPHMETRICS metrics;
    RectI clip;

    FIXED zero;
    zero.fract = 0;
    zero.value = 0;
    FIXED one;
    one.fract = 0;
    one.value = 1;

    matrix.eM11 = one;
    matrix.eM12 = zero;
    matrix.eM21 = zero;
    matrix.eM22 = one;


    if (GetGlyphOutline(
        fontHDC,	// handle of device context 
        ch,	// character to query 
        GGO_GRAY8_BITMAP,	// format of data to return 
        &metrics,	// address of structure for metrics 
        sizeof(scratchPad),	// size of buffer for data 
        scratchPad,	// address of buffer for data 
        &matrix 	// address of transformation matrix structure  
    ) != GDI_ERROR)
    {
        U32 rowStride = (metrics.gmBlackBoxX + 3) & ~3; // DWORD aligned
        U32 size = rowStride * metrics.gmBlackBoxY;
        for (U32 j = 0; j < size; j++)
        {
            U32 pad = U32(scratchPad[j]) << 2;
            if (pad > 255)
                pad = 255;
            scratchPad[j] = pad;
        }
        S32 inc = metrics.gmCellIncX;
        if (inc < 0)
            inc = -inc;

        c.xOffset = 0;
        c.yOffset = 0;
        c.width = metrics.gmBlackBoxX;
        c.height = metrics.gmBlackBoxY;
        c.xOrigin = metrics.gmptGlyphOrigin.x;
        c.yOrigin = metrics.gmptGlyphOrigin.y;
        c.xIncrement = metrics.gmCellIncX;

        c.bitmapData = new U8[c.width * c.height];
        for (U32 y = 0; S32(y) < c.height; y++)
        {
            U32 x;
            for (x = 0; x < c.width; x++)
                c.bitmapData[y * c.width + x] = scratchPad[y * rowStride + x];
        }
    }
    else
    {
        SIZE size;
        GetTextExtentPoint32W(fontHDC, &ch, 1, &size);
        if (size.cx)
        {
            c.xIncrement = size.cx;
            c.bitmapIndex = 0;
        }
    }

    return c;
}

PlatformFont::CharInfo& WinFont::getCharInfo(const UTF8* str) const
{
    return getCharInfo(oneUTF8toUTF32(str));
}
