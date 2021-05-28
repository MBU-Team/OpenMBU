//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GOLDFONT_H_
#define _GOLDFONT_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GBITMAP_H_
#include "gfx/gBitmap.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureHandle.h"

extern ResourceInstance* constructFont(Stream& stream);

/*GFX_DeclareTextureProfile(GFXFontTextureProfile);

class GOldFont : public ResourceInstance
{
    static const U32 csm_fileVersion;
    static S32 smSheetIdCount;

    // Enumerations and structs available to everyone...
public:
    struct CharInfo {
        S16 bitmapIndex;    // Note: -1 indicates character is NOT to be
                            //  rendered, i.e., \n, \r, etc.
        U16 xPos;           // Position of character in single sheet
                            // Fonts are stored as a single bitmap, but
                            // then are packed into sheets on load
        U8  xOffset;        // x offset into bitmap sheet
        U8  yOffset;        // y offset into bitmap sheet
        U8  width;          // width of character (pixels)
        U8  height;         // height of character (pixels)
        S8  xOrigin;
        S8  yOrigin;
        S8  xIncrement;
        U8* bitmapData;    // temp storage for bitmap data
    };
    enum Constants {
        TabWidthInSpaces = 3
    };


    // Enumerations and structures available to derived classes
private:
    Vector<U32> mSheetSizes;
    U32 mNumSheets;
    U32 mPadding;
    GBitmap* mBitmap;
    GFXTexHandle* mTextureSheets;

    U32 mFontHeight;   // ascent + descent of the font
    U32 mBaseLine;     // ascent of the font (pixels above the baseline of any character in the font)

    Vector<CharInfo>  mCharInfoList;      // - List of character info structures, must
                                           //    be accessed through the getCharInfo(U32)
                                           //    function to account for remapping...
    S16             mRemapTable[256];    // - Index remapping

    S16 getActualIndex(const U8 in_charIndex) const;

public:
    GOldFont();
    virtual ~GOldFont();

    // Queries about this font
public:
    GFXTexHandle getTextureHandle(S32 index) const;
    U32   getCharHeight(const U8 in_charIndex) const;
    U32   getCharWidth(const U8 in_charIndex)  const;
    U32   getCharXIncrement(const U8 in_charIndex)  const;

    bool            isValidChar(const U8 in_charIndex) const;
    const CharInfo& getCharInfo(const U8 in_charIndex) const;

    U32 getSheetCount() const { return mNumSheets; };

    // Rendering assistance functions...
public:
    U32 getBreakPos(const char* string, U32 strlen, U32 width, bool breakOnWhitespace);

    U32 getStrWidth(const char*)  const;   // Note: ignores c/r
    U32 getStrNWidth(const char*, U32 n) const;
    U32 getStrWidthPrecise(const char*)  const;   // Note: ignores c/r
    U32 getStrNWidthPrecise(const char*, U32 n) const;
    void wrapString(const char* string, U32 width, Vector<U32>& startLineOffset, Vector<U32>& lineLen);

    bool read(Stream& io_rStream);
    bool write(Stream& io_rStream) const;

    U32 getPadding()  const { return mPadding; }
    U32 getHeight()   const { return mFontHeight; };
    U32 getBaseline() const { return mBaseLine; };
    U32 getAscent()   const { return mBaseLine; };
    U32 getDescent()  const { return mFontHeight - mBaseLine; };

    void insertBitmap(U16 index, U8* src, U32 stride, U32 width, U32 height, S32 xOrigin, S32 yOrigin, S32 xIncrement);
    void pack(U32 fontHeight, U32 baseLine);

    static Resource<GOldFont> create(const char* faceName, dsize_t size, const char* cacheDirectory, const char* newFaceName = NULL, U32 padding = 0);
};

inline bool GOldFont::isValidChar(const U8 in_charIndex) const
{
    return mRemapTable[in_charIndex] != -1;
}

inline S16 GOldFont::getActualIndex(const U8 in_charIndex) const
{
    AssertFatal(isValidChar(in_charIndex) == true,
        avar("GOldFont::getActualIndex: invalid character: 0x%x",
            in_charIndex));

    return mRemapTable[in_charIndex];
}

inline const GOldFont::CharInfo& GOldFont::getCharInfo(const U8 in_charIndex) const
{
    S16 remap = getActualIndex(in_charIndex);
    AssertFatal(remap != -1, "No remap info for this character");

    return mCharInfoList[remap];
}

inline U32 GOldFont::getCharXIncrement(const U8 in_charIndex) const
{
    const CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.xIncrement;
}

inline U32 GOldFont::getCharWidth(const U8 in_charIndex) const
{
    const CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.width;
}

inline U32 GOldFont::getCharHeight(const U8 in_charIndex) const
{
    const CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.height;
}*/

#endif //_GOLDFONT_H_
