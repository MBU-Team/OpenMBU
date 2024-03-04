//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "terrain/terrData.h"
#include "core/fileStream.h"
#include "gfx/gBitmap.h"
#include "math/mRandom.h"
#include "editor/terrain/terraformer.h"

#include "core/dnet.h"
#include "game/shapeBase.h"
#include "game/gameConnection.h"

#include "editor/guiTerrPreviewCtrl.h"

IMPLEMENT_CONOBJECT(Terraformer);

S32* Heightfield::zoneOffset = NULL;
U32  Heightfield::instance = 0;

//------------------------------------------------------------------------------
Heightfield::Heightfield(U32 r, U32 sz)
{
    registerNumber = r;
    mask = sz - 1;
    shift = getBinLog2(sz);
    data = new F32[sz * sz];

    // initialize zone offsets if it has not been done
    if (instance++ == 0)
    {
        zoneOffset = new S32[sz * sz];

        // initialize Zone Offset lookup Table
        S32 array[3];
        array[0] = 0;
        array[1] = 1;
        array[2] = mask;

        S32* o = zoneOffset;
        for (S32 y = 0; y < 3; y++)
        {
            S32 y1 = array[y];
            S32 y1plus = (y1 + 1) & mask;
            S32 y1minus = (y1 - 1) & mask;
            for (S32 x = 0; x < 3; x++)
            {
                S32 x1 = array[x];
                S32 x1plus = (x1 + 1) & mask;
                S32 x1minus = (x1 - 1) & mask;
                S32 root = offset(x1, y1);

                *o++ = offset(x1minus, y1minus) - root;
                *o++ = offset(x1, y1minus) - root;
                *o++ = offset(x1plus, y1minus) - root;

                *o++ = offset(x1minus, y1) - root;
                *o++ = 0;
                *o++ = offset(x1plus, y1) - root;

                *o++ = offset(x1minus, y1plus) - root;
                *o++ = offset(x1, y1plus) - root;
                *o++ = offset(x1plus, y1plus) - root;
            }
        }
    }

}


Heightfield::~Heightfield()
{
    if (data)
        delete[] data;

    if (instance)
    {
        instance--;
        if (!instance)
            delete[] zoneOffset;
    }
}


Heightfield& Heightfield::operator=(const Heightfield& src)
{
    if (data != src.data)
        dMemcpy(data, src.data, sizeof(F32) * (1 << shift) * (1 << shift));
    return *this;
}



//------------------------------------------------------------------------------
Terraformer::Terraformer()
{
    setTerrainInfo(256, 8.0f, 100.f, 300.0f, 0.0f);
    setShift(Point2F(0, 0));
}


//------------------------------------------------------------------------------
Terraformer::~Terraformer()
{
    VectorPtr<Heightfield*>::iterator i;
    for (i = registerList.begin(); i != registerList.end(); i++)
        delete* i;
    for (i = scratchList.begin(); i != scratchList.end(); i++)
        delete* i;
}

//------------------------------------------------------------------------------
Heightfield* Terraformer::getRegister(U32 r)
{
    VectorPtr<Heightfield*>::iterator i;
    for (i = registerList.begin(); i != registerList.end(); i++)
        if ((*i)->registerNumber == r)
            break;
    if (i == registerList.end())
    {
        registerList.push_back(new Heightfield(r, blockSize));
        i = &registerList.last();
    }
    return *i;
}


Heightfield* Terraformer::getScratch(U32 r)
{
    VectorPtr<Heightfield*>::iterator i;
    for (i = scratchList.begin(); i != scratchList.end(); i++)
        if ((*i)->registerNumber == r)
            break;
    if (i == scratchList.end())
    {
        scratchList.push_back(new Heightfield(r, blockSize));
        i = &scratchList.last();
    }
    return *i;
}


//--------------------------------------
void Terraformer::setTerrainInfo(U32 size, F32 tileSize, F32 minHeight, F32 heightRange, F32 water)
{
    blockSize = getNextPow2(size);     // just to make sure clamp to power of 2
    blockMask = size - 1;
    worldTileSize = tileSize;
    worldHeight = heightRange;
    worldBaseHeight = minHeight;
    worldWater = water;
}


void Terraformer::setShift(const Point2F& shift)
{
    mShift.set(shift.x, shift.y);
}


//------------------------------------------------------------------------------
void Terraformer::getMinMax(U32 r, F32* fmin, F32* fmax)
{
    Heightfield* src = getRegister(r);
    if (!src)
        return;

    F32* p = src->data;
    *fmin = *p;
    *fmax = *p;
    for (S32 i = 0; i < (blockSize * blockSize); i++, p++)
    {
        if (*fmin > *p) *fmin = *p;
        if (*fmax < *p) *fmax = *p;
    }
}

//------------------------------------------------------------------------------
void Terraformer::clearRegister(U32 r)
{
    Heightfield* dst = getRegister(r);

    for (S32 y = 0; y < blockSize; y++)
        for (S32 x = 0; x < blockSize; x++)
            dst->val(x, y) = 0.0f;
}

//------------------------------------------------------------------------------
GBitmap* Terraformer::getScaledGreyscale(U32 r)
{
    enum { NUM_COLORS = 2 };
    ColorI land[NUM_COLORS] =
    {
       ColorI(0,0,0),
       ColorI(20,255,20)
    };

    ColorI water[NUM_COLORS] =
    {
       ColorI(0,0,0),
       ColorI(20,20,255)
    };


    Heightfield* src = getRegister(r);

    F32 fmin, fmax;
    getMinMax(r, &fmin, &fmax);
    F32 scale = (NUM_COLORS - 1) / (fmax - fmin);

    GBitmap* bitmap = new GBitmap(blockSize, blockSize, false);

    S32 y, x;
    U8* rgb = bitmap->getAddress(0, 0);
    for (y = blockSize - 1; y >= 0; y--)
    {
        for (x = 0; x < blockSize; x++)
        {
            ColorI c;
            F32 index = (src->val(wrap(x), wrap(y)) - fmin) * scale;
            if (index > worldWater)
            {  // above "water"
                S32 indexLo = (S32)mFloor(index);
                S32 indexHi = (S32)mCeil(index);
                index -= indexLo;
                c.interpolate(land[indexLo], land[indexHi], index);
            }
            else if (water != 0)
            {  // below "water"
                index /= worldWater;
                S32 indexLo = (S32)mFloor(index);
                S32 indexHi = (S32)mCeil(index);
                index -= indexLo;
                c.interpolate(water[indexLo], water[indexHi], index);
            }

            *rgb++ = c.red;
            *rgb++ = c.green;
            *rgb++ = c.blue;
        }
    }
    return(bitmap);
}


//------------------------------------------------------------------------------
GBitmap* Terraformer::getGreyscale(U32 r)
{
    Heightfield* src = getRegister(r);

    GBitmap* bitmap = new GBitmap(blockSize, blockSize, false);

    S32 y, x;
    U8* rgb = bitmap->getAddress(0, 0);
    for (y = blockSize - 1; y >= 0; y--)
    {
        for (x = 0; x < blockSize; x++)
        {
            S32 index = (S32)(src->val(wrap(x), wrap(y)) * 255.0f);
            index = getMax(getMin(index, 255), 0);

            *rgb++ = index;
            *rgb++ = index;
            *rgb++ = index;
        }
    }

    return(bitmap);
}



//------------------------------------------------------------------------------
bool Terraformer::saveGreyscale(U32 r, const char* filename)
{
    GBitmap* bitmap = getScaledGreyscale(r);
    if (!bitmap)
        return false;

    FileStream sio;
    if (sio.open(filename, FileStream::Write))
    {
        bitmap->writePNG(sio);
        sio.close();

    }
    delete bitmap;
    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::loadGreyscale(U32 r, const char* filename)
{
    Heightfield* dst = getRegister(r);

    //   return ResourceManager->findFile(filename);
    GBitmap* bmp = (GBitmap*)ResourceManager->loadInstance(filename);
    if (!bmp)
        return false;

    for (S32 y = 0; y < blockSize; y++)
        for (S32 x = 0; x < blockSize; x++)
        {
            U8* rgb = bmp->getAddress(x, (blockSize - 1) - y);
            // compute the luminance of each RGB
            dst->val(x, y) = ((F32)rgb[0]) * (0.299f / 256.0f) +
                ((F32)rgb[1]) * (0.587f / 256.0f) +
                ((F32)rgb[2]) * (0.114f / 256.0f);
        }

    delete bmp;
    return true;
}

//------------------------------------------------------------------------------
bool Terraformer::saveHeightField(U32 r, const char* filename)
{
    Stream* stream;
    F32 fmin, fmax, f;
    Heightfield* dst = getRegister(r);

    getMinMax(r, &fmin, &fmax);

    F32 scale = blockSize / (fmax - fmin);
    if (ResourceManager->openFileForWrite(stream, filename))
    {
        for (S32 y = 0; y < blockSize; y++)
            for (S32 x = 0; x < blockSize; x++)
            {
                f = (dst->val(wrap((S32)(x + mShift.x)), wrap((S32)(y + mShift.y))) - fmin) * scale;
                U16 test = floatToFixed(f);
                stream->write(test);
            }
        delete stream;
    }

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::setTerrain(U32 r)
{
    Heightfield* src = getRegister(r);

    TerrainBlock* terrBlock = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (!terrBlock)
        return false;

    F32 omin, omax;
    getMinMax(r, &omin, &omax);

    F32 scale = worldHeight / (omax - omin);
    S32 sz = TerrainBlock::BlockSize;

    Point2F shift(mShift);
    shift *= 256.0 / (TerrainBlock::BlockSize * terrBlock->getSquareSize());

    S32 y;
    for (y = sz; y >= 0; y--)
    {
        S32 x;
        for (x = 0; x <= sz; x++)
        {
            F32 f = (src->val(wrap((S32)(x + shift.x)), wrap((S32)(y + shift.y))) - omin) * scale + worldBaseHeight;
            *(terrBlock->getHeightAddress(x, y)) = floatToFixed(f);
        }
    }
    /*   {
          Point3F a(0.5, 0.5, -0.5);
          terrBlock->buildGridMap();
          terrBlock->relight(ColorF(0.8,0.8,0.8), ColorF(0.35,0.35,0.35), a);
       } */

    Point3F pos = getCameraPosition();
    pos.z = 0.0f;
    setCameraPosition(pos);

    return (true);
}


//------------------------------------------------------------------------------
void Terraformer::setCameraPosition(const Point3F& pos)
{
    GameConnection* connection = GameConnection::getLocalClientConnection();
    Point3F position(pos);

    ShapeBase* camera = NULL;
    if (connection)
        camera = connection->getControlObject();

    if (!camera)
    {
        Con::warnf(ConsoleLogEntry::General, "Terraformer::setCameraPosition: could not get camera.");
        return;
    }

    // move it
    MatrixF mat = camera->getTransform();


    TerrainBlock* terrain = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (terrain)
    {
        F32 height;
        Point2F xy(position.x, position.y);

        terrain->getHeight(xy, &height);
        if ((position.z - height) < 2.0f)
            position.z = height + 10.0f;
    }

    mat.setColumn(3, position);
    camera->setTransform(mat);
}


//------------------------------------------------------------------------------
Point3F Terraformer::getCameraPosition()
{
    GameConnection* connection = GameConnection::getLocalClientConnection();
    Point3F current;
    current.set(0.0f, 0.0f, 0.0f);

    ShapeBase* camera = NULL;
    if (connection)
        camera = connection->getControlObject();

    if (!camera)
    {
        Con::warnf(ConsoleLogEntry::General, "Terraformer::getCameraPosition: could not get camera.");
        return current;
    }

    // move it
    MatrixF mat = camera->getTransform();
    mat.getColumn(3, &current);
    return current;
}


//------------------------------------------------------------------------------
bool Terraformer::scale(U32 r_src, U32 r_dst, F32 fmin, F32 fmax)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    F32 omin, omax;
    getMinMax(r_src, &omin, &omax);

    F32 scale = (fmax - fmin) / (omax - omin);
    for (S32 i = 0; i < (blockSize * blockSize); i++)
        dst->val(i) = ((src->val(i) - omin) * scale) + fmin;

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::smooth(U32 r_src, U32 r_dst, F32 factor, U32 iterations)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (iterations == 0)
    {
        *dst = *src;
        return true;
    }

    Heightfield* a = getScratch(0);
    Heightfield* b = getScratch(1);
    *a = *src;

    // factor of 0.0 = NO Smoothing
    // factor of 1.0 = MAX Smoothing
    F32 matrixM = 1.0f - getMax(0.0f, getMin(1.0f, factor));
    F32 matrixE = (1.0f - matrixM) * (1.0f / 12.0f) * 2.0f;
    F32 matrixC = matrixE * 0.5f;

    *a = *src;
    for (U32 i = 0; i < iterations; i++)
    {
        for (S32 y = 0; y < blockSize; y++)
        {
            for (S32 x = 0; x < blockSize; x++)
            {
                F32 array[9];
                a->block(x, y, array);
                //  0  1  2
                //  3 x,y 5
                //  6  7  8

                b->val(x, y) =
                    ((array[0] + array[2] + array[6] + array[8]) * matrixC) +
                    ((array[1] + array[3] + array[5] + array[7]) * matrixE) +
                    (array[4] * matrixM);
            }
        }
        Heightfield* tmp = a;
        a = b;
        b = tmp;
    }
    *dst = *a;

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::smoothWater(U32 r_src, U32 r_dst, F32 factor, U32 iterations)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (iterations == 0)
    {
        *dst = *src;
        return true;
    }

    Heightfield* a = getScratch(0);
    Heightfield* b = getScratch(1);
    *a = *src;

    F32 fmin, fmax;
    getMinMax(r_src, &fmin, &fmax);
    F32 water = (worldWater * (fmax - fmin)) + fmin;

    // factor of 0.0 = NO Smoothing
    // factor of 1.0 = MAX Smoothing
    F32 matrixM = 1.0f - getMax(0.0f, getMin(1.0f, factor));
    F32 matrixE = (1.0f - matrixM) * (1.0f / 12.0f) * 2.0f;
    F32 matrixC = matrixE * 0.5f;


    for (U32 i = 0; i < iterations; i++)
    {
        for (S32 y = 0; y < blockSize; y++)
        {
            for (S32 x = 0; x < blockSize; x++)
            {
                F32 value = a->val(x, y);
                if (value <= water)
                {
                    F32 array[9];
                    a->block(x, y, array);
                    //  0  1  2
                    //  3 x,y 5
                    //  6  7  8

                    b->val(x, y) =
                        ((array[0] + array[2] + array[6] + array[8]) * matrixC) +
                        ((array[1] + array[3] + array[5] + array[7]) * matrixE) +
                        (array[4] * matrixM);
                }
                else
                    b->val(x, y) = value;
            }
        }
        Heightfield* tmp = a;
        a = b;
        b = tmp;
    }
    *dst = *a;
    return true;
}


#define ridgeMask(m0, m1, m2, m3, m4, m5, m6, m7, m8) (m8 | (m7<<2) | (m6<<4) | (m5<<6) | (m4<<8) | (m3<<10) | (m2<<12) | (m1<<14) | (m0<<16) )

//------------------------------------------------------------------------------
bool Terraformer::smoothRidges(U32 r_src, U32 r_dst, F32 factor, U32 iterations, F32 threshold)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (iterations == 0)
    {
        *dst = *src;
        return true;
    }

    Heightfield* a = getScratch(0);
    Heightfield* b = getScratch(1);
    *a = *src;

    F32 fmin, fmax;
    getMinMax(r_src, &fmin, &fmax);
    threshold = mClampF(threshold, 0.0f, 1.0f);
    threshold = (fmax - fmin) * threshold;


    // factor of 0.0 = NO Smoothing
    // factor of 1.0 = MAX Smoothing
    F32 matrixM = 1.0f - getMax(0.0f, getMin(1.0f, factor));
    F32 matrixE = (1.0f - matrixM) * (1.0f / 12.0f) * 2.0f;
    F32 matrixC = matrixE * 0.5f;

    *a = *src;
    for (U32 i = 0; i < iterations; i++)
    {
        for (S32 y = 0; y < blockSize; y++)
        {
            for (S32 x = 0; x < blockSize; x++)
            {
                F32 array[9];
                a->block(x, y, array);
                //  0  1  2
                //  3 x,y 5
                //  6  7  8

                F32 center = array[4];
                F32 ave = (array[0] + array[1] + array[2] +
                    array[3] + array[5] +
                    array[6] + array[7] + array[8]) / 8.0f;

                // if this height deviates too much from its neighboors smooth it!
                if (mFabs(ave - center) > threshold)
                {
                    b->val(x, y) =
                        ((array[0] + array[2] + array[6] + array[8]) * matrixC) +
                        ((array[1] + array[3] + array[5] + array[7]) * matrixE) +
                        (center * matrixM);
                }
                else
                    b->val(x, y) = center;
            }
        }
        Heightfield* tmp = a;
        a = b;
        b = tmp;
    }
    *dst = *a;

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::blend(U32 r_srcA, U32 r_srcB, U32 r_dst, F32 factor, BlendOperation operation)
{
    Heightfield* srcA = getRegister(r_srcA);
    Heightfield* srcB = getRegister(r_srcB);
    Heightfield* dst = getRegister(r_dst);

    F32 fminA, fmaxA, fminB, fmaxB;
    getMinMax(r_srcA, &fminA, &fmaxA);
    getMinMax(r_srcB, &fminB, &fmaxB);

    F32 scaleA = blockSize / (fmaxA - fminA) * factor;
    F32 scaleB = blockSize / (fmaxB - fminB) * (1.0f - factor);

    for (S32 i = 0; i < (blockSize * blockSize); i++)
    {
        F32 a = (srcA->val(i) - fminA) * scaleA;
        F32 b = (srcB->val(i) - fminB) * scaleB;
        switch (operation)
        {
        case OperationSubtract:
            dst->val(i) = a - b;
            break;
        case OperationMax:
            dst->val(i) = getMax(a, b);
            break;
        case OperationMin:
            dst->val(i) = getMin(a, b);
        default:
        case OperationAdd:
            dst->val(i) = a + b;
            break;
        case OperationMultiply:
            dst->val(i) = a * b;
        }
    }
    return true;

}


//------------------------------------------------------------------------------
bool Terraformer::filter(U32 r_src, U32 r_dst, const Filter& filter)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    F32 omin, omax;
    getMinMax(r_src, &omin, &omax);
    scale(r_src, r_dst, 0.0f, 1.0f);

    for (S32 i = 0; i < (blockSize * blockSize); i++)
    {
        F32 h = dst->val(i);
        dst->val(i) = filter.getValue(h) * omax + omin;
    }
    return true;
}



//--------------------------------------------------------------------------
bool Terraformer::shift(U32 r_src)
{
    Heightfield* src = getScratch(0);
    Heightfield* dst = getRegister(r_src);

    // early out if there is nothing to do
    if (mShift.x == 0 && mShift.y == 0)
        return true;

    *src = *dst;

    for (S32 y = 0; y < blockSize; y++)
        for (S32 x = 0; x < blockSize; x++)
            dst->val(x, y) = src->val(wrap((S32)(x + mShift.x)), wrap((S32)(y + mShift.y)));

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::sinus(U32 r, const Filter& filter, U32 seed)
{
    F32 invBlockSize = 1 / F32(blockSize);
    Heightfield* dst = getRegister(r);

    random.setSeed(seed);
    noise.setSeed(seed);

    U32 iterations = 31;
    for (U32 k = 0; k < blockSize * blockSize; k++)
        dst->val(k) = 0;

    for (S32 i = 0; i < iterations; i += 2)
    {
        F32 period = M_2PI * (i + 1) * invBlockSize;
        F32 scale = filter.getValue(i / F32(iterations - 1));
        F32 xOffset = random.randF() * M_2PI;
        F32 yOffset = random.randF() * M_2PI;

        F32 interval = i + 2;
        F32 sqInterval = invBlockSize * interval;

        for (S32 y = 0; y < blockSize; y++)
        {
            F32 cosy = mCos(y * period + yOffset);
            for (S32 x = 0; x < blockSize; x++)
            {
                F32 sinx = mSin(x * period + xOffset);
                dst->val(x, y) += scale * (sinx + cosy) * noise.getValue(x * sqInterval, y * sqInterval, (S32)interval);
            }
        }
    }
    return true;
}

bool Terraformer::setHeight(U32 r, F32 height)
{
    Heightfield* dst = getRegister(r);
    for (U32 i = 0; i < (blockSize * blockSize); i++)
        dst->val(i) = height;
    return(true);
}

bool Terraformer::terrainData(U32 r)
{
    if (blockSize != TerrainBlock::BlockSize)
    {
        Con::errorf(ConsoleLogEntry::General, "Terraformer::terrainData - invalid blocksize.");
        return(setHeight(r, 0.f));
    }

    Heightfield* dst = getRegister(r);

    TerrainBlock* terrBlock = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (!terrBlock)
    {
        Con::warnf(ConsoleLogEntry::General, "Terraformer::terrainData - TerrainBlock 'terrain' not found.");
        return(setHeight(r, 0.f));
    }

    //
    for (U32 y = 0; y < TerrainBlock::BlockSize; y++)
        for (U32 x = 0; x < TerrainBlock::BlockSize; x++)
            dst->val(x, y) = fixedToFloat(terrBlock->getHeight(x, y));

    return(true);
}

bool Terraformer::terrainFile(U32 r, const char* fileName)
{
    if (blockSize != TerrainBlock::BlockSize)
    {
        Con::errorf(ConsoleLogEntry::General, "Terraformer::terrainFile - invalid blocksize.");
        return(setHeight(r, 0.f));
    }

    Heightfield* dst = getRegister(r);

    Resource<TerrainFile> terrRes;

    terrRes = ResourceManager->load(fileName);
    if (!bool(terrRes))
    {
        Con::errorf(ConsoleLogEntry::General, "Terraformer::terrainFile - invalid terrain file '%s'.", fileName);
        return(setHeight(r, 0.f));
    }

    //
    for (U32 y = 0; y < TerrainBlock::BlockSize; y++)
        for (U32 x = 0; x < TerrainBlock::BlockSize; x++)
            dst->val(x, y) = fixedToFloat(terrRes->getHeight(x, y));

    return(true);
}

//------------------------------------------------------------------------------
bool Terraformer::fBm(U32 r, U32 interval, F32 roughness, F32 octave, U32 seed)
{
    Heightfield* dst = getRegister(r);

    noise.setSeed(seed);
    noise.fBm(dst, blockSize, interval, 1.0 - roughness, octave);

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::rigidMultiFractal(U32 r, U32 interval, F32 roughness, F32 octave, U32 seed)
{

    Heightfield* dst = getRegister(r);
    Heightfield* a = getScratch(0);

    noise.setSeed(seed);
    noise.rigidMultiFractal(dst, a, blockSize, interval, 1.0 - roughness, octave);

    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::canyonFractal(U32 r, U32 f, F32 v, U32 seed)
{
    Heightfield* dst = getRegister(r);
    noise.setSeed(seed);

    v *= 40; // just a magic number
    for (S32 y = 0; y < blockSize; y++)
    {
        F32 fy = (F32)y / (F32)blockSize;
        for (S32 x = 0; x < blockSize; x++)
        {
            F32 fx = (F32)x / (F32)blockSize;
            F32 t = noise.turbulence(fx, fy, 32) * v;
            dst->val(x, y) = mCos(fx * M_2PI * f + t);
        }
    }
    return true;
}


//--------------------------------------------------------------------------
bool Terraformer::turbulence(U32 r_src, U32 r_dst, F32 v, F32 r)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (v == 0.0f || r == 0.0f)
    {
        *dst = *src;
        return true;
    }

    v *= 20; // just a magic number
    F32 scale = 1.0f / (F32)blockSize;
    for (S32 y = 0; y < blockSize; y++)
    {
        F32 fy = (F32)y * scale;
        for (S32 x = 0; x < blockSize; x++)
        {
            F32 fx = (F32)x * scale;
            F32 t = noise.turbulence(fx, fy, 32) * v;
            F32 dx = r * mSin(t);
            F32 dy = r * mCos(t);
            dst->val(x, y) = src->val(wrap((S32)(x + dx)), wrap((S32)(y + dy)));
        }
    }
    return true;
}


//------------------------------------------------------------------------------
bool Terraformer::erodeThermal(U32 r_src, U32 r_dst, F32 slope, F32 materialLoss, U32 iterations)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (iterations == 0)
    {
        *dst = *src;
        return true;
    }


    F32 fmin, fmax;
    getMinMax(r_src, &fmin, &fmax);

    Heightfield* a = getScratch(0);
    Heightfield* b = getScratch(1);
    Heightfield* r = getScratch(2);
    *a = *src;

    F32 conservation = 1.0f - mClampF(materialLoss, 0.0f, 100.0f) / 100.0f;
    slope = mClampF(conservation, 0.0f, 89.0f);                  // clamp to 0-89 degrees

    F32 talusConst = mTan(mDegToRad(slope)) * worldTileSize; // in world units
    talusConst = talusConst * (fmax - fmin) / worldHeight;     // scale to current height units
    F32 p = 0.1f;

    for (U32 i = 0; i < iterations; i++)
    {
        // clear out the rubble accumulation field
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            r->val(i) = 0.0f;

        for (S32 y = 0; y < blockSize; y++)
        {
            for (S32 x = 0; x < blockSize; x++)
            {
                F32 totalDelta = 0.0f;
                F32* height = &a->val(x, y);
                F32* dstHeight = &r->val(x, y);

                // for each height look at the immediate surrounding heights
                // if any are higher than talusConst erode on me
                for (S32 y1 = y - 1; y1 <= y + 1; y1++)
                {
                    S32 ywrap = wrap(y1);
                    for (S32 x1 = x - 1; x1 <= x + 1; x1++)
                    {
                        if (x1 != x && y1 != y)
                        {
                            S32 adjOffset = a->offset(wrap(x1), ywrap);
                            F32 adjHeight = a->val(adjOffset);
                            F32 delta = adjHeight - *height;
                            if (delta > talusConst)
                            {
                                F32 rubble = p * (delta - talusConst);
                                r->val(adjOffset) -= rubble;
                                *dstHeight += rubble * conservation;
                            }
                        }
                    }
                }
            }
        }
        for (S32 k = 0; k < (blockSize * blockSize); k++)
            a->val(k) += r->val(k);
    }
    *dst = *a;
    return true;
}



//------------------------------------------------------------------------------
bool Terraformer::erodeHydraulic(U32 r_src, U32 r_dst, U32 iterations, const Filter& filter)
{
    filter;

    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    // early out if there is nothing to do
    if (iterations == 0)
    {
        *dst = *src;
        return true;
    }

    F32 fmin, fmax;
    getMinMax(r_src, &fmin, &fmax);


    // currently using SCRATCH_3 for debugging -- Rick
    U32* o = (U32*)getScratch(0)->data;
    Heightfield* a = getScratch(1);
    Heightfield* b = getScratch(2);
    Heightfield* c = getScratch(3);

    *a = *src;
    for (S32 k = 0; k < (blockSize * blockSize); k++)
        c->val(k) = 0.0f;

    for (int i = 0; i < iterations; i++)
    {
        b = a;

        for (S32 y = 0; y < blockSize; y++)
        {
            for (S32 x = 0; x < blockSize; x++)
            {
                U32 srcOffset = a->offset(x, y);
                F32 height = a->val(srcOffset);
                o[srcOffset] = srcOffset;
                for (S32 y1 = y - 1; y1 <= y + 1; y1++)
                {
                    F32 maxDelta = 0.0f;
                    S32 ywrap = wrap(y1);
                    for (S32 x1 = x - 1; x1 <= x + 1; x1++)
                    {
                        if (x1 != x && y1 != y)
                        {
                            U32 adjOffset = a->offset(wrap(x1), ywrap);
                            F32& adjHeight = a->val(adjOffset);
                            F32 delta = height - adjHeight;
                            if (x1 != x || y1 != y)
                                delta *= 1.414213562f;    // compensate for diagonals
                            if (delta > maxDelta)
                            {
                                maxDelta = delta;
                                o[srcOffset] = adjOffset;
                            }
                        }
                    }
                }
            }
        }
        for (S32 j = 0; j < (blockSize * blockSize); j++)
        {
            F32& s = a->val(j);
            F32& d = b->val(o[j]);
            F32 delta = s - d;
            if (delta > 0.0f)
            {
                F32 alt = (s - fmin) / (fmax - fmin);
                F32 amt = delta * (0.1f * (1.0f - alt));
                s -= amt;
                d += amt;
            }
        }
        // debug only
        for (S32 k = 0; k < (blockSize * blockSize); k++)
            c->val(k) += b->val(k) - a->val(k);

        Heightfield* tmp = a;
        a = b;
        b = tmp;
    }
    *dst = *b;
    //*dst = *c;

    return true;
}



// CONSOLE FN's
//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, setTerrainInfo, void, 7, 7, "(int blockSize, int tileSize, float minHeight, float heightRange, float waterPercent)")
{
    object->setTerrainInfo(dAtoi(argv[2]), dAtof(argv[3]), dAtof(argv[4]), dAtof(argv[5]), dAtof(argv[6]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, setShift, void, 4, 4, "(int x, int y)")
{
    Point2F shift(dAtof(argv[2]), dAtof(argv[3]));
    object->setShift(shift);
}


//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, generateSeed, S32, 2, 2, "")
{
    S32 n = Platform::getVirtualMilliseconds() * 57;
    n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return n;
}


//------------------------------------------------------------------------------

ConsoleMethod(Terraformer, saveGreyscale, bool, 4, 4, "(int register, string filename)")
{
    char filename[1024];
    Con::expandScriptFilename(filename, sizeof(filename), argv[3]);
    return object->saveGreyscale(dAtoi(argv[2]), filename);
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, loadGreyscale, bool, 4, 4, "(int register, string filename)")
{
    char filename[1024];
    Con::expandScriptFilename(filename, sizeof(filename), argv[3]);
    return object->loadGreyscale(dAtoi(argv[2]), filename);
}

//------------------------------------------------------------------------------

ConsoleMethod(Terraformer, saveHeightField, bool, 4, 4, "(int register, string filename)")
{
    char filename[1024];
    Con::expandScriptFilename(filename, sizeof(filename), argv[3]);
    return object->saveHeightField(dAtoi(argv[2]), filename);
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, setTerrain, bool, 3, 3, "(int register)")
{
    return object->setTerrain(dAtoi(argv[2]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, getCameraPosition, const char*, 2, 2, "Get the position of the camera.")
{
    argv;
    static char buffer[64];
    Point3F position = object->getCameraPosition();
    dSprintf(buffer, sizeof(buffer), "%f %f %f", position.x, position.y, position.z);
    return buffer;
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, setCameraPosition, void, 4, 5, "(float x, float y, float z=0)")
{
    F32 z = 0.0f;
    if (argc == 5)
        z = dAtof(argv[4]);
    Point3F pos(dAtof(argv[2]), dAtof(argv[3]), z);
    object->setCameraPosition(pos);
}


ConsoleMethod(Terraformer, terrainData, bool, 3, 3, "(int register)")
{
    return object->terrainData(dAtoi(argv[2]));
}

ConsoleMethod(Terraformer, terrainFile, bool, 4, 4, "(int register, string filename)")
{
    return object->terrainFile(dAtoi(argv[2]), argv[3]);
}
//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, scale, bool, 6, 6, "(int src_register, int dst_register, float min, float max)")
{
    return object->scale(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtof(argv[5]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, smooth, bool, 6, 6, "(int src_register, int dst_register, float factor, int iterations)")
{
    return object->smooth(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtoi(argv[5]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, smoothWater, bool, 6, 6, "(int src_register, int dst_register, float factor, int iterations)")
{
    return object->smoothWater(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtoi(argv[5]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, smoothRidges, bool, 6, 6, "(int src_register, int dst_register, float factor, int iterations)")
{
    return object->smoothRidges(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtoi(argv[5]), 0.01f);
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, filter, bool, 5, 5, "(int src_register, int dst_register, Filter arr)")
{
    Filter filter;
    filter.set(argc - 4, &argv[4]);
    return object->filter(dAtoi(argv[2]), dAtoi(argv[3]), filter);
}


//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, blend, bool, 7, 7, "(int srcA, int srcB, int dest_register, float factor, string operation)"
    "Perform a blending operation on the terrain.\n\n"
    "@param   srcA           First source for operation.\n"
    "@param   srcB           Second source for operation.\n"
    "@param   dest_register  Destination register for blend.\n"
    "@param   factor         Blending factor, from 0-1.\n"
    "@param   operation      One of: add, subtract, max, min, multiply. Default is add.")
{
    Terraformer::BlendOperation op = Terraformer::OperationAdd;
    if (dStricmp(argv[6], "add") == 0) op = Terraformer::OperationAdd;
    if (dStricmp(argv[6], "subtract") == 0) op = Terraformer::OperationSubtract;
    if (dStricmp(argv[6], "max") == 0) op = Terraformer::OperationMax;
    if (dStricmp(argv[6], "min") == 0) op = Terraformer::OperationMin;
    if (dStricmp(argv[6], "multiply") == 0) op = Terraformer::OperationMultiply;
    return object->blend(dAtoi(argv[2]), dAtoi(argv[3]), dAtoi(argv[4]), dAtof(argv[5]), op);
}


//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, turbulence, bool, 6, 6, "(int src_register, int dst_register, float factor, float radius)")
{
    return object->turbulence(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtof(argv[5]));
}


//------------------------------------------------------------------------------

ConsoleMethod(Terraformer, maskFBm, void, 9, 9, "(int dest_register, int frequency, float roughness, int seed, Filter arr, bool distort_factor, int distort_reg)")
{
    Filter filter;
    filter.set(1, &argv[6]);
    object->maskFBm(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtoi(argv[5]), filter, dAtob(argv[7]), dAtoi(argv[8]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, maskHeight, bool, 7, 7, "(int src_register, int dst_register, Filter arr, bool distort_factor, int distort_register)")
{
    Filter filter;
    filter.set(1, &argv[4]);
    return object->maskHeight(dAtoi(argv[2]), dAtoi(argv[3]), filter, dAtob(argv[5]), dAtoi(argv[6]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, maskSlope, bool, 7, 7, "(int src_register, int dst_register, Filter arr, bool distort_factor, int distort_register)")
{
    Filter filter;
    filter.set(1, &argv[4]);
    return object->maskSlope(dAtoi(argv[2]), dAtoi(argv[3]), filter, dAtob(argv[5]), dAtoi(argv[6]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, maskWater, bool, 6, 6, "(int src_register, int dst_register, bool distort_factor, int distort_reg)")
{
    return object->maskWater(dAtoi(argv[2]), dAtoi(argv[3]), dAtob(argv[4]), dAtoi(argv[5]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, mergeMasks, bool, 4, 4, "( src_array, int dst_register)")
{
    return object->mergeMasks(argv[2], dAtoi(argv[3]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, setMaterials, bool, 4, 4, "( src_array, material_array )")
{
    return object->setMaterials(argv[2], argv[3]);
}


//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, erodeHydraulic, bool, 6, 6, "(int src_register, int dst_register, int iterations, Filter arr )")
{
    Filter filter;
    filter.set(argc - 5, &argv[5]);
    return object->erodeHydraulic(dAtoi(argv[2]), dAtoi(argv[3]), dAtoi(argv[4]), filter);
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, erodeThermal, bool, 7, 7, "(int src_register, int dst_register, float slope, float materialLoss, int iterations )")
{
    return object->erodeThermal(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtof(argv[5]), dAtoi(argv[6]));
}


//--------------------------------------------------------------------------
ConsoleMethod(Terraformer, canyon, bool, 6, 6, "(int dest_register, int frequency, float turbulence, int seed)")
{
    return object->canyonFractal(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), dAtoi(argv[5]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, previewScaled, void, 4, 4, "(GuiTerrPreviewCtrl destination, int source)")
{
    GuiTerrPreviewCtrl* dcc;
    Sim::findObject(argv[2], dcc);

    //const GBitmap* bmp = object->getScaledGreyscale(dAtoi(argv[3]));

    //   dcc->setBitmap(TextureHandle("tfImage", bmp, true));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, preview, void, 4, 4, "(GuiTerrPreviewCtrl destination, int register)")
{
    GuiTerrPreviewCtrl* bmc;
    Sim::findObject(argv[2], bmc);

    //const GBitmap* bmp = object->getGreyscale(dAtoi(argv[3]));

    //   bmc->setBitmap(TextureHandle("tfImage", bmp, true));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, clearRegister, void, 3, 3, "(int r)")
{
    object->clearRegister(dAtoi(argv[2]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, fBm, void, 7, 7, "(int r, int freq, float roughness, string detail, int seed)"
    "Run an fBm pass.\n\n"
    "@param   roughness   From 0.0-1.0\n"
    "@param   detail      One of 'Very Low', 'Low', 'Normal', 'High', or 'Very High'")
{
    F32 octave = 3.0f;
    if (!dStricmp(argv[5], "Very Low")) octave = 1.0f;
    else if (!dStricmp(argv[5], "Low")) octave = 2.0f;
    else if (!dStricmp(argv[5], "Normal")) octave = 3.0f;
    else if (!dStricmp(argv[5], "High")) octave = 4.0f;
    else if (!dStricmp(argv[5], "Very High")) octave = 5.0f;
    object->fBm(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), octave, dAtoi(argv[6]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, sinus, void, 5, 5, "(int r, Filter a, int seed)")
{
    Filter filter;
    filter.set(1, &argv[3]);
    object->sinus(dAtoi(argv[2]), filter, dAtoi(argv[4]));
}

//------------------------------------------------------------------------------
ConsoleMethod(Terraformer, rigidMultiFractal, void, 7, 7, "(int r, int freq, float roughness, string detail, int seed)"
    "Run a rigid multi fractal pass.\n\n"
    "@param   roughness   From 0.0-1.0\n"
    "@param   detail      One of 'Very Low', 'Low', 'Normal', 'High', or 'Very High'")
{
    F32 octave = 3.0f;
    if (!dStricmp(argv[5], "Very Low")) octave = 1.0f;
    else if (!dStricmp(argv[5], "Low")) octave = 2.0f;
    else if (!dStricmp(argv[5], "Normal")) octave = 3.0f;
    else if (!dStricmp(argv[5], "High")) octave = 4.0f;
    else if (!dStricmp(argv[5], "Very High")) octave = 5.0f;
    object->rigidMultiFractal(dAtoi(argv[2]), dAtoi(argv[3]), dAtof(argv[4]), octave, dAtoi(argv[6]));
}

