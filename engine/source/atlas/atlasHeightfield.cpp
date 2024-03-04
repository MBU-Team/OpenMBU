//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasHeightfield.h"
#include "core/resManager.h"
#include "gfx/gBitmap.h"
#include "core/fileStream.h"
#include "util/safeDelete.h"
#include "math/mPlane.h"

//------------------------------------------------------------------------

AtlasHeightfield::AtlasHeightfield(U32 sizeLog2, const F32 sampleSpacing, const F32 heightScale)
    : mFieldShift(sizeLog2)
{
    mHeight = new HeightType[realSize() * realSize()];
    mFieldShiftMinusOne = mFieldShift - 1;
    mSampleSpacing = sampleSpacing;
    mVerticalScale = heightScale;
    mNormal = NULL;
}

AtlasHeightfield::~AtlasHeightfield()
{
    SAFE_DELETE_ARRAY(mHeight);
    SAFE_DELETE_ARRAY(mNormal);
}

bool AtlasHeightfield::loadRawU16(Stream& s)
{
    // Read a raw heightfield from a file. It had best be right size!

    // First read the data, then blast it out.
    U16* tmp = new U16[realSize() * realSize()];

    if (!s.read(realSize() * realSize() * sizeof(U16), tmp))
    {
        Con::errorf("AtlasHeightfield::loadRaw16 - Failed to read data!");
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

void AtlasHeightfield::generateNormals()
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

bool AtlasHeightfield::write(Stream& s)
{
    return true;
}

bool AtlasHeightfield::read(Stream& s)
{
    return true;
}

bool AtlasHeightfield::saveJpeg(const char* filename)
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
        Con::errorf("AtlasHeightfield::saveJpeg - failed to open output file!");
        return false;
    }

    tmp.writeJPEG(*fs);
    delete fs;

    return true;
}

ConsoleFunction(testCHF, void, 1, 1, "() Benchmark the AtlasHeightfield query speed.")
{
    AtlasHeightfield chf(getBinLog2(2048), 8.f, 0.03125f);

    // Load it!
    Stream* s = ResourceManager->openStream("demo/data/terrains/test_16_2049.raw");
    if (!s)
    {
        Con::errorf("testCHF - failed to load!");
        return;
    }

    chf.loadRawU16(*s);
    ResourceManager->closeStream(s);

    // Ok, now let's do several thousand queries and see what we get!
    U32 start = Platform::getRealMilliseconds();

    U32 width = 2046;

    U32 total = 0;

    for (S32 i = 1; i < width; i++)
    {
        for (S32 j = 1; j < width; j++)
        {
            // Now do a 2x2 sample at the specified location.
            U16 a, b, c, d;
            U16 a1, b1, c1, d1;

            Point2I tmp(
                mClamp(Platform::getRandom() * width, 1, width - 1),
                mClamp(Platform::getRandom() * width, 1, width - 1)
            );

            a = chf.sample(tmp + Point2I(0, 0));
            b = chf.sample(tmp + Point2I(0, 1));
            c = chf.sample(tmp + Point2I(1, 0));
            d = chf.sample(tmp + Point2I(1, 1));

            a1 = chf.sample(tmp + Point2I(0, 0));
            b1 = chf.sample(tmp + Point2I(0, -1));
            c1 = chf.sample(tmp + Point2I(-1, 0));
            d1 = chf.sample(tmp + Point2I(-1, -1));

            total += a + b + c + d + a1 + b1 + c1 + d1 / 1000;
        }
    }

    U32 end = Platform::getRealMilliseconds();

    U32 queryCount = width * width * 8;
    U32 time = end - start;
    Con::printf(" Did %d queries in %dms. Approx %f queries/ms. Total=%d", queryCount, time, F32(queryCount) / F32(time), total);
}