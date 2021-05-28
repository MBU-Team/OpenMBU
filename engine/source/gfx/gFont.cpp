//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "gFont.h"
#include "core/fileStream.h"
#include "gfx/gfxDevice.h"
#include "console/console.h"

/*S32 GOldFont::smSheetIdCount = 0;
static U32 smPadding = 0;

//GFX_ImplementTextureProfile(GFXFontTextureProfile,
//                            GFXTextureProfile::DiffuseMap, 
//                            GFXTextureProfile::PreserveSize |
//                            GFXTextureProfile::Static, 
//                            GFXTextureProfile::None);

ConsoleFunction(generateFont, void, 5, 5, "(sysFaceName, newFaceName, size, padding)")
{
    GOldFont::create(argv[1], dAtoi(argv[3]), Con::getVariable("$GUI::fontCacheDirectory"), argv[2], dAtoi(argv[4]));
}

ResourceInstance* constructFont(Stream& stream)
{
    GOldFont* ret = new GOldFont;
    if (!ret->read(stream))
    {
        delete ret;
        return NULL;
    }

    return ret;
}
const U32 GOldFont::csm_fileVersion = 1;

Resource<GOldFont> GOldFont::create(const char* faceName, dsize_t size, const char* cacheDirectory, const char* newFaceName, U32 padding)
{
    smPadding = padding;
    U32 i;
    char buf[256];
    if (!newFaceName)
        newFaceName = faceName;
    dSprintf(buf, sizeof(buf), "%s/%s_%d.gf2", cacheDirectory, newFaceName, size);
    char bitmapNameBuf[256];

    Resource<GOldFont> ret = ResourceManager->load(buf);
    if (bool(ret))
    {
        if (ret->mTextureSheets)
            // already generated sheets, simply return font
            return ret;
        ret->mTextureSheets = new GFXTexHandle[ret->mNumSheets];
        for (i = 0; i < ret->mNumSheets; i++)
        {
            dSprintf(bitmapNameBuf, sizeof(bitmapNameBuf), "%s/%s_%d_%d", cacheDirectory, newFaceName, size, i);
            //gNoCompression = true;
            //ret->mTextureSheets[i] = GFX->allocTexture(bitmapNameBuf, GFXTextureType_Normal, false, true);
            ret->mTextureSheets[i].set(bitmapNameBuf, &GFXFontTextureProfile);
            //gNoCompression = false;
            if (!bool(ret->mTextureSheets[i]))
                break;
        }
        if (i == ret->mNumSheets)
            return ret;
        // else, missing a bitmap...should never happen (either gf2 & bitmaps are present or neither)
        // AssertFatal in debug, fall through and generate bitmaps in release
        AssertFatal(0, "Missing font png file");
    }

    GOldFont* resFont = createFont(faceName, size);
    if (resFont == NULL || resFont->mBitmap == NULL)
    {
        AssertISV(dStricmp(faceName, "Arial") != 0, "Error, The Arial Font must always be available!");

        // Need to handle this case better.  For now, let's just return a font that we're
        //  positive exists, in the correct size...
        return create("Arial", size, cacheDirectory);
    }

    FileStream stream;
    if (ResourceManager->openFileForWrite(stream, buf))
    {
        resFont->write(stream);
        stream.close();
    }
    else
        return NULL;

    // chop font bitmap into the sheets...
    GBitmap* bmp = resFont->mBitmap;
    Vector<GBitmap*> bitmaps;
    for (U32 i = 0; i < resFont->mNumSheets; i++)
        bitmaps.push_back(new GBitmap(256, resFont->mSheetSizes[i], false, bmp->getFormat()));
    for (U32 i = 0; i < resFont->mCharInfoList.size(); i++)
    {
        GOldFont::CharInfo& ci = resFont->mCharInfoList[i];
        if (ci.bitmapIndex == -1)
            continue;
        RectI srcRect(ci.xPos, 0, ci.width, ci.height);
        Point2I dstPoint(ci.xOffset, ci.yOffset);
        S32 topOfChar = S32(resFont->mBaseLine + resFont->mPadding) - ci.yOrigin;
        if (topOfChar < 0)
            topOfChar = 0;
        srcRect.point.y = topOfChar;

        //bitmaps[ci.bitmapIndex]->preserveSize = true;
        bitmaps[ci.bitmapIndex]->copyRect(bmp, srcRect, dstPoint);
    }
    for (U32 i = 0; i < resFont->mNumSheets; i++)
    {
        dSprintf(bitmapNameBuf, sizeof(bitmapNameBuf), "%s/%s_%d_%d.png", cacheDirectory, newFaceName, size, i);
        if (ResourceManager->openFileForWrite(stream, bitmapNameBuf))
        {
            bitmaps[i]->writePNG(stream);
            stream.close();
        }
        delete bitmaps[i];
    }

    // font sheets now saved off, now call this method again to load font resource with sheets
    delete resFont;
    return create(faceName, size, cacheDirectory);
}

GOldFont::GOldFont()
{
    VECTOR_SET_ASSOCIATION(mCharInfoList);

    for (U32 i = 0; i < 256; i++)
        mRemapTable[i] = -1;

    mTextureSheets = NULL;
    mBitmap = NULL;
}

GOldFont::~GOldFont()
{
    delete[] mTextureSheets;
    mTextureSheets = NULL;
    delete mBitmap;
}

void GOldFont::insertBitmap(U16 index, U8* src, U32 stride, U32 width, U32 height, S32 xOrigin, S32 yOrigin, S32 xIncrement)
{
    CharInfo c;
    c.bitmapIndex = -1;
    c.xOffset = 0;
    c.yOffset = 0;
    c.width = width + smPadding * 2;
    c.height = height + smPadding * 2;

    c.xOrigin = xOrigin + smPadding;
    c.yOrigin = yOrigin + smPadding;
    c.xIncrement = xIncrement;

    if (c.width != 0 && c.height != 0) {
        c.bitmapData = new U8[c.width * c.height];

        for (U32 y = 0; S32(y) < c.height; y++)
        {
            U32 x;
            for (x = 0; x < c.width; x++)
                c.bitmapData[y * c.width + x] = 0;
        }
        for (U32 y = 0; S32(y) < height; y++)
        {
            U32 x;
            for (x = 0; x < width; x++)
                c.bitmapData[(y + smPadding) * c.width + x + smPadding] = src[y * stride + x];
        }
        mRemapTable[index] = mCharInfoList.size();
        mCharInfoList.push_back(c);
    }
    else {
        c.bitmapData = NULL;
        mRemapTable[index] = mCharInfoList.size();
        mCharInfoList.push_back(c);
    }
}

static S32 QSORT_CALLBACK CharInfoCompare(const void* a, const void* b)
{
    S32 ha = (*((GOldFont::CharInfo**)a))->height;
    S32 hb = (*((GOldFont::CharInfo**)b))->height;

    return hb - ha;
}

void GOldFont::pack(U32 inFontHeight, U32 inBaseLine)
{
    mFontHeight = inFontHeight;
    mBaseLine = inBaseLine;

    // pack all the bitmap data into sheets.
    Vector<CharInfo*> vec;

    U32 size = mCharInfoList.size();
    U32 i;

    U16 bigBitmapX = 0;
    for (i = 0; i < size; i++)
    {
        CharInfo* ch = &mCharInfoList[i];
        ch->xPos = bigBitmapX;
        bigBitmapX += ch->width;
        vec.push_back(ch);
    }

    dQsort(vec.address(), size, sizeof(CharInfo*), CharInfoCompare);
    // sorted by height

    Point2I curSheetSize(256, 256);

    S32 curY = 0;
    S32 curX = 0;
    S32 curLnHeight = 0;
    S32 maxHeight = 0;
    for (i = 0; i < size; i++)
    {
        CharInfo* ci = vec[i];
        if (ci->height > maxHeight)
            maxHeight = ci->height;

        if (curX + ci->width > curSheetSize.x)
        {
            curY += curLnHeight;
            curX = 0;
            curLnHeight = 0;
        }
        if (curY + ci->height > curSheetSize.y)
        {
            mSheetSizes.push_back(curSheetSize.y);
            curX = 0;
            curY = 0;
            curLnHeight = 0;
        }
        if (ci->height > curLnHeight)
            curLnHeight = ci->height;
        ci->bitmapIndex = mSheetSizes.size();
        ci->xOffset = curX;
        ci->yOffset = curY;
        curX += ci->width;
    }
    curY += curLnHeight;

    if (curY < 64)
        curSheetSize.y = 64;
    else if (curY < 128)
        curSheetSize.y = 128;

    mSheetSizes.push_back(curSheetSize.y);

    mNumSheets = mSheetSizes.size();
    mPadding = smPadding;

    delete[] mTextureSheets;
    mTextureSheets = new GFXTexHandle[mNumSheets];

    if (inFontHeight + smPadding * 2 > maxHeight)
        maxHeight = inFontHeight + smPadding * 2;
    mBitmap = new GBitmap(bigBitmapX, maxHeight, false, GFXFormatA8);
    //mBitmap->preserveSize = true;
    S32 x, y;
    for (y = 0; y < maxHeight; y++)
    {
        U8* ptr = (U8*)mBitmap->getAddress(0, y);
        for (x = 0; x < bigBitmapX; x++)
            *ptr++ = 0;
    }

    for (i = 0; i < size; i++)
    {
        CharInfo* ci = vec[i];
        S32 topOfChar = S32(inBaseLine + smPadding - ci->yOrigin);
        if (topOfChar < 0)
            topOfChar = 0;
        if (topOfChar + ci->height > maxHeight)
            topOfChar = maxHeight - ci->height;

        for (y = 0; y < ci->height; y++)
            for (x = 0; x < ci->width; x++)
                *mBitmap->getAddress(x + ci->xPos, y + topOfChar) =
                ci->bitmapData[y * ci->width + x];
        delete[] ci->bitmapData;
    }
}

GFXTexHandle GOldFont::getTextureHandle(S32 index) const
{
    return mTextureSheets[index];
}


U32 GOldFont::getStrWidth(const char* in_pString) const
{
    AssertFatal(in_pString != NULL, "GOldFont::getStrWidth: String is NULL, height is undefined");
    // If we ain't running debug...
    if (in_pString == NULL)
        return 0;

    return getStrNWidth(in_pString, dStrlen(in_pString));
}

U32 GOldFont::getStrWidthPrecise(const char* in_pString) const
{
    AssertFatal(in_pString != NULL, "GOldFont::getStrWidth: String is NULL, height is undefined");
    // If we ain't running debug...
    if (in_pString == NULL)
        return 0;

    return getStrNWidthPrecise(in_pString, dStrlen(in_pString));
}

//-----------------------------------------------------------------------------
U32 GOldFont::getStrNWidth(const char* str, dsize_t n) const
{
    AssertFatal(str != NULL, "GOldFont::getStrNWidth: String is NULL");

    if (str == NULL)
        return(0);

    U32 totWidth = 0;
    const char* curChar;
    const char* endStr;
    for (curChar = str, endStr = str + n; curChar < endStr; curChar++)
    {
        if (isValidChar(*curChar))
        {
            const CharInfo& rChar = getCharInfo(*curChar);
            totWidth += rChar.xIncrement;
        }
        else if (*curChar == '\t')
        {
            const CharInfo& rChar = getCharInfo(' ');
            totWidth += rChar.xIncrement * TabWidthInSpaces;
        }
    }

    return(totWidth);
}

U32 GOldFont::getStrNWidthPrecise(const char* str, dsize_t n) const
{
    AssertFatal(str != NULL, "GOldFont::getStrNWidth: String is NULL");

    if (str == NULL)
        return(0);

    U32 totWidth = 0;
    const char* curChar;
    const char* endStr;
    for (curChar = str, endStr = str + n; curChar < endStr; curChar++)
    {
        if (isValidChar(*curChar))
        {
            const CharInfo& rChar = getCharInfo(*curChar);
            totWidth += rChar.xIncrement;
        }
        else if (*curChar == '\t')
        {
            const CharInfo& rChar = getCharInfo(' ');
            totWidth += rChar.xIncrement * TabWidthInSpaces;
        }
    }

    if (n != 0) {
        // Need to check the last char to see if it has some slop...
        char endChar = str[n - 1];
        if (isValidChar(endChar)) {
            const CharInfo& rChar = getCharInfo(endChar);
            if (rChar.width > rChar.xIncrement)
                totWidth += (rChar.width - rChar.xIncrement);
        }
    }

    return(totWidth);
}

U32 GOldFont::getBreakPos(const char* string, dsize_t slen, U32 width, bool breakOnWhitespace)
{
    U32 ret = 0;
    U32 lastws = 0;
    while (ret < slen)
    {
        char c = string[ret];
        if (c == '\t')
            c = ' ';
        if (!isValidChar(c))
        {
            ret++;
            continue;
        }
        if (c == ' ')
            lastws = ret + 1;
        const CharInfo& rChar = getCharInfo(c);
        if (rChar.width > width || rChar.xIncrement > width)
        {
            if (lastws && breakOnWhitespace)
                return lastws;
            return ret;
        }
        width -= rChar.xIncrement;

        ret++;
    }
    return ret;
}

void GOldFont::wrapString(const char* txt, U32 lineWidth, Vector<U32>& startLineOffset, Vector<U32>& lineLen)
{
    startLineOffset.clear();
    lineLen.clear();

    if (!txt || !txt[0] || lineWidth < getCharWidth('W')) //make sure the line width is greater then a single character
        return;

    U32 len = dStrlen(txt);

    U32 startLine;

    for (U32 i = 0; i < len;)
    {
        startLine = i;
        startLineOffset.push_back(startLine);

        // loop until the string is too large
        bool needsNewLine = false;
        U32 lineStrWidth = 0;
        for (; i < len; i++)
        {
            if (isValidChar(txt[i]))
            {
                lineStrWidth += getCharInfo(txt[i]).xIncrement;
                if (txt[i] == '\n' || lineStrWidth > lineWidth)
                {
                    needsNewLine = true;
                    break;
                }
            }
        }

        if (!needsNewLine)
        {
            // we are done!
            lineLen.push_back(i - startLine);
            return;
        }

        // now determine where to put the newline
        // else we need to backtrack until we find a either space character
        // or \\ character to break up the line.
        S32 j;
        for (j = i - 1; j >= startLine; j--)
        {
            if (dIsspace(txt[j]))
                break;
        }

        if (j < startLine)
        {
            // the line consists of a single word!
            // So, just break up the word
            j = i - 1;
        }
        lineLen.push_back(j - startLine);
        i = j;

        // now we need to increment through any space characters at the
        // beginning of the next line
        for (i++; i < len; i++)
        {
            if (!dIsspace(txt[i]) || txt[i] == '\n')
                break;
        }
    }
}

//------------------------------------------------------------------------------
//-------------------------------------- Persist functionality
//
static const U32 csm_fileVersion = 1;

bool GOldFont::read(Stream& io_rStream)
{
    // Handle versioning
    U32 version;
    io_rStream.read(&version);
    if (version != csm_fileVersion)
        return false;

    // Read Font Information
    io_rStream.read(&mFontHeight);
    io_rStream.read(&mBaseLine);

    U32 size = 0;
    io_rStream.read(&size);
    mCharInfoList.setSize(size);
    U32 i;
    for (i = 0; i < size; i++)
    {
        CharInfo* ci = &mCharInfoList[i];
        io_rStream.read(&ci->bitmapIndex);
        io_rStream.read(&ci->xPos);
        io_rStream.read(&ci->xOffset);
        io_rStream.read(&ci->yOffset);
        io_rStream.read(&ci->width);
        io_rStream.read(&ci->height);
        io_rStream.read(&ci->xOrigin);
        io_rStream.read(&ci->yOrigin);
        io_rStream.read(&ci->xIncrement);
    }
    io_rStream.read(&mNumSheets);
    for (i = 0; i < mNumSheets; i++)
    {
        io_rStream.read(&size);
        mSheetSizes.push_back(size);
    }
    // Read character remap table
    for (i = 0; i < 256; i++)
        io_rStream.read(&mRemapTable[i]);
    io_rStream.read(&mPadding);

    return (io_rStream.getStatus() == Stream::Ok);
}

bool GOldFont::write(Stream& stream) const
{
    // Handle versioning
    stream.write(csm_fileVersion);

    // Write Font Information
    stream.write(mFontHeight);
    stream.write(mBaseLine);

    stream.write(U32(mCharInfoList.size()));
    U32 i;
    for (i = 0; i < mCharInfoList.size(); i++)
    {
        const CharInfo* ci = &mCharInfoList[i];
        stream.write(ci->bitmapIndex);
        stream.write(ci->xPos);
        stream.write(ci->xOffset);
        stream.write(ci->yOffset);
        stream.write(ci->width);
        stream.write(ci->height);
        stream.write(ci->xOrigin);
        stream.write(ci->yOrigin);
        stream.write(ci->xIncrement);
    }
    stream.write(mNumSheets);
    for (i = 0; i < mNumSheets; i++)
        stream.write(mSheetSizes[i]);
    for (i = 0; i < 256; i++)
        stream.write(mRemapTable[i]);
    stream.write(mPadding);

    return (stream.getStatus() == Stream::Ok);
}
*/
