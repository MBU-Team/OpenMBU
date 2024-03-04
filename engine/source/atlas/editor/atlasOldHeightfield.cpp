//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "atlas/editor/atlasOldHeightfield.h"
#include "core/resManager.h"
#include "gfx/gBitmap.h"
#include "core/fileStream.h"
#include "util/safeDelete.h"
#include "math/mPlane.h"

//------------------------------------------------------------------------

AtlasOldHeightfield::AtlasOldHeightfield(U32 sizeLog2, const F32 sampleSpacing, const F32 heightScale)
    : mFieldShift(sizeLog2)
{
    mHeight = new HeightType[realSize() * realSize()];
    mFieldShiftMinusOne = mFieldShift - 1;
    mSampleSpacing = sampleSpacing;
    mVerticalScale = heightScale;
    mNormal = NULL;
}

AtlasOldHeightfield::~AtlasOldHeightfield()
{
    SAFE_DELETE_ARRAY(mHeight);
    SAFE_DELETE_ARRAY(mNormal);
}

bool AtlasOldHeightfield::loadRawU16(Stream& s)
{
    // Read a raw heightfield from a file. It had best be right size!

    // First read the data, then blast it out.
    U16* tmp = new U16[realSize() * realSize()];

    if (!s.read(realSize() * realSize() * sizeof(U16), tmp))
    {
        Con::errorf("AtlasOldHeightfield::loadRaw16 - Failed to read data!");
        return false;
    }

    // TODO - deal with endianness.

    // Now, convolute it into our internal buffer...
    for (S32 i = 0; i < realSize(); i++)
    {
        for (S32 j = 0; j < realSize(); j++)
        {
            S32 a = tmp[i * realSize() + j];
            a = a - (BIT(15) - 1) - 1;

            AssertFatal(S32(a) == S16(a), "overflow in loadRawU16");
            sample(Point2I(i, j)) = a;
        }
    }

    // Clean up and done.
    delete[] tmp;

    return true;
}

void AtlasOldHeightfield::generateNormals()
{
    SAFE_DELETE_ARRAY(mNormal);
    mNormal = new Point3F[size() * size()];

    for (S32 i = 0; i < size(); i++)
    {
        for (S32 j = 0; j < size(); j++)
        {
            // We're really calculating the average normal for the SQUARE in
            // question. This is kind of a mythical beast but we'll do our
            // best to MAKE IT UP OMG!

            // Let's figure both halves and average it. This won't work out real
            // well but it's enough to get us going.

            const Point2I curPos(i, j);

            const Point3F a(0, 0, sampleScaled(curPos + Point2I(0, 0)));
            const Point3F b(0, 1, sampleScaled(curPos + Point2I(0, 1)));
            const Point3F c(1, 0, sampleScaled(curPos + Point2I(1, 0)));
            const Point3F d(1, 1, sampleScaled(curPos + Point2I(1, 1)));

            // My naming totally rawks.
            const PlaneF e(a, b, c);
            const PlaneF f(b, d, c);

            // Problem solving by casting.
            const Point3F g = e;
            const Point3F h = f;

            // Ok, average, normalize, store.
            Point3F norm = (g + h) / 2.f;
            norm.normalizeSafe();
            mNormal[i * size() + j] = norm;
        }
    }

}

bool AtlasOldHeightfield::write(Stream& s)
{
    return true;
}

bool AtlasOldHeightfield::read(Stream& s)
{
    return true;
}

bool AtlasOldHeightfield::saveJpeg(const char* filename)
{
    GBitmap tmp(realSize(), realSize(), false, GFXFormatA8);

    // Copy everything into the buffer.
    ColorI foo;
    for (S32 i = 0; i < realSize(); i++)
    {
        for (S32 j = 0; j < realSize(); j++)
        {
            const Point2I pos(i, j);

            foo.alpha = sample(pos) >> 8;
            tmp.setColor(pos.x, pos.y, foo);
        }
    }

    Stream* fs;

    if (!ResourceManager->openFileForWrite(fs, filename, File::Write))
    {
        Con::errorf("AtlasOldHeightfield::saveJpeg - failed to open output file!");
        return false;
    }

    tmp.writeJPEG(*fs);
    delete fs;

    return true;
}