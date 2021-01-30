//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFONT_H_
#define _GFONT_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMFONT_H_
#include "platform/platformFont.h"
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

extern ResourceInstance* constructNewFont(Stream& stream);

GFX_DeclareTextureProfile(GFXFontTextureProfile);

class GFont : public ResourceInstance
{
    friend ResourceInstance* constructNewFont(Stream& stream);

    static const U32 csm_fileVersion;
    static S32 smSheetIdCount;

    // Enumerations and structs available to everyone...
public:

    enum Constants
    {
        TabWidthInSpaces = 3,
        TextureSheetSize = 256,
    };


    // Enumerations and structures available to derived classes
private:
    PlatformFont* mPlatformFont;
    Vector<GFXTexHandle>mTextureSheets;

    S32 mCurX;
    S32 mCurY;
    S32 mCurSheet;

    StringTableEntry mGFTFile;
    StringTableEntry mFaceName;
    U32 mSize;
    U32 mCharSet;

    U32 mHeight;
    U32 mBaseline;
    U32 mAscent;
    U32 mDescent;

    Vector<PlatformFont::CharInfo>  mCharInfoList;       // - List of character info structures, must
                                           //    be accessed through the getCharInfo(U32)
                                           //    function to account for remapping...
    S32             mRemapTable[65536];    // - Index remapping
public:
    GFont();
    virtual ~GFont();

protected:
    bool loadCharInfo(const UTF16 ch);
    void addBitmap(PlatformFont::CharInfo& charInfo);
    void addSheet(void);
    void assignSheet(S32 sheetNum, GBitmap* bmp);

    void* mMutex;

public:
    static Resource<GFont> create(const char* faceName, U32 size, const char* cacheDirectory, U32 charset = TGE_ANSI_CHARSET);

    GFXTexHandle getTextureHandle(S32 index) const
    {
        return mTextureSheets[index];
    }

    const PlatformFont::CharInfo& getCharInfo(const UTF16 in_charIndex);

    U32  getCharHeight(const UTF16 in_charIndex);
    U32  getCharWidth(const UTF16 in_charIndex);
    U32  getCharXIncrement(const UTF16 in_charIndex);

    bool isValidChar(const UTF16 in_charIndex);

    const U32 getHeight() const { return mHeight; }
    const U32 getBaseline() const { return mBaseline; }
    const U32 getAscent() const { return mAscent; }
    const U32 getDescent() const { return mDescent; }

    U32 getBreakPos(const UTF8* string, U32 strlen, U32 width, bool breakOnWhitespace);

    /// These are the preferred width functions.
    U32 getStrNWidth(const UTF16*, U32 n);
    U32 getStrNWidthPrecise(const UTF16*, U32 n);

    /// These UTF8 versions of the width functions will be deprecated, please avoid them.
    U32 getStrWidth(const UTF8*);   // Note: ignores c/r
    U32 getStrNWidth(const UTF8*, U32 n);
    U32 getStrWidthPrecise(const UTF8*);   // Note: ignores c/r
    U32 getStrNWidthPrecise(const UTF8*, U32 n);
    void wrapString(const UTF8* string, U32 width, Vector<U32>& startLineOffset, Vector<U32>& lineLen);

    /// Dump information about this font to the console.
    void dumpInfo();

    /// Export to an image strip for image processing.
    void exportStrip(const char* fileName, U32 padding, U32 kerning);

    /// Import an image strip generated with exportStrip, make sure parameters match!
    void importStrip(const char* fileName, U32 padding, U32 kerning);

    /// Query as to presence of platform font. If absent, we cannot generate more
    /// chars!
    const bool hasPlatformFont() const
    {
        return mPlatformFont;
    }

    /// Query to determine if we should use add or modulate (as A8 textures
    /// are treated as having 0 for RGB).
    bool isAlphaOnly()
    {
        return mTextureSheets[0]->getBitmap()->getFormat() == GFXFormatA8;
    }

    /// Get the filename for a cached font.
    static void getFontCacheFilename(const char* faceName, U32 faceSize, U32 buffLen, char* outBuff);

    bool read(Stream& io_rStream);
    bool write(Stream& io_rStream);

    /// Override existing platform font if any with a new one from an external
    /// source. This is primarily used in font processing tools to enable
    /// trickery (ie, putting characters from multiple fonts in a single
    /// GFT) and should be used with caution!
    void forcePlatformFont(PlatformFont* pf)
    {
        mPlatformFont = pf;
    }
};

inline U32 GFont::getCharXIncrement(const UTF16 in_charIndex)
{
    const PlatformFont::CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.xIncrement;
}

inline U32 GFont::getCharWidth(const UTF16 in_charIndex)
{
    const PlatformFont::CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.width;
}

inline U32 GFont::getCharHeight(const UTF16 in_charIndex)
{
    const PlatformFont::CharInfo& rChar = getCharInfo(in_charIndex);
    return rChar.height;
}

inline bool GFont::isValidChar(const UTF16 in_charIndex)
{
    if (mRemapTable[in_charIndex] != -1)
        return true;

    if (mPlatformFont)
        return mPlatformFont->isValidChar(in_charIndex);

    return false;
}


#endif //_GFONT_H_
