//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRAFORMER_H_
#define _TERRAFORMER_H_

#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#ifndef _GBITMAP_H_
#include "gfx/gBitmap.h"
#endif
#ifndef _MRANDOM_H_
#include "math/mRandom.h"
#endif
#ifndef _TERRAFORMER_NOISE_H_
#include "editor/terrain/terraformerNoise.h"
#endif
#ifndef _GUIFILTERCTRL_H_
#include "gui/editor/guiFilterCtrl.h"
#endif


struct Heightfield
{
    static S32* zoneOffset;
    static U32 instance;

    U32 registerNumber;
    U32 mask;
    U32 shift;
    union
    {
        F32* data;
        S32* dataS32;
    };
    Heightfield(U32 r, U32 sz);
    ~Heightfield();

    Heightfield& operator=(const Heightfield& src);
    S32  offset(S32 x, S32 y);
    F32& val(S32 x, S32 y);
    F32& val(S32 index);
    void block(S32 x, S32 y, F32* a);

    S32& valS32(S32 x, S32 y);
    S32& valS32(S32 index);

};



class Terraformer : public SimObject
{
public:
    typedef SimObject Parent;
    enum BlendOperation
    {
        OperationAdd,
        OperationSubtract,
        OperationMax,
        OperationMin,
        OperationMultiply,
    };

private:
    VectorPtr<Heightfield*> registerList;
    VectorPtr<Heightfield*> scratchList;
    MRandomR250 random;

    Noise2D noise;

    // used by wrap, val inlines
    U32 blockMask;
    U32 blockSize;
    F32 worldTileSize;
    F32 worldBaseHeight;
    F32 worldHeight;
    F32 worldWater;
    Point2F mShift;

    S32 wrap(S32 p);

    Heightfield* getRegister(U32 r);
    Heightfield* getScratch(U32 r);
    void getMinMax(U32 r, F32* fmin, F32* fmax);

public:
    Terraformer();
    ~Terraformer();
    DECLARE_CONOBJECT(Terraformer);

    void setSize(U32 n);
    void setTerrainInfo(U32 blockSize, F32 tileSize, F32 minHeight, F32 heightRange, F32 worldWater);
    void setShift(const Point2F& shift);
    void clearRegister(U32 r);

    // I/O operations
    GBitmap* getScaledGreyscale(U32 r);
    GBitmap* getGreyscale(U32 r);
    bool saveGreyscale(U32 r, const char* filename);
    bool loadGreyscale(U32 r, const char* filename);
    bool saveHeightField(U32 r, const char* filename);
    bool setTerrain(U32 r);
    void setCameraPosition(const Point3F& pos);
    Point3F getCameraPosition();


    // Mathmatical operations
    bool scale(U32 r_src, U32 r_dst, F32 fmin, F32 fmax);
    bool smooth(U32 r_src, U32 r_dst, F32 factor, U32 iterations);
    bool smoothWater(U32 r_src, U32 r_dst, F32 factor, U32 iterations);
    bool smoothRidges(U32 r_src, U32 r_dst, F32 factor, U32 iterations, F32 threshold);
    bool blend(U32 r_srcA, U32 r_srcB, U32 r_dst, F32 factor, BlendOperation operation);
    bool filter(U32 r_src, U32 r_dst, const Filter& filter);
    bool turbulence(U32 r_src, U32 r_dst, F32 v, F32 r);
    bool shift(U32 r_src);

    // Generation operations
    bool setHeight(U32 r, F32 height);
    bool terrainData(U32 r);
    bool terrainFile(U32 r, const char* terrFile);
    bool fBm(U32 r, U32 interval, F32 roughness, F32 detail, U32 seed);
    bool rigidMultiFractal(U32 r, U32 interval, F32 roughness, F32 detail, U32 seed);
    bool canyonFractal(U32 r, U32 f, F32 v, U32 seed);
    bool sinus(U32 r, const Filter& filter, U32 seed);

    // effects
    bool erodeHydraulic(U32 r_src, U32 r_dst, U32 iterations, const Filter& filter);
    bool erodeThermal(U32 r_src, U32 r_dst, F32 slope, F32 materialLoss, U32 iterations);

    // Texturing operations
    bool maskFBm(U32 r_dst, U32 interval, F32 roughness, U32 seed, const Filter& filter, bool distort, U32 r_distort);
    bool maskHeight(U32 r_src, U32 r_dst, const Filter& filter, bool distort, U32 r_distort);
    bool maskSlope(U32 r_src, U32 r_dst, const Filter& filter, bool distort, U32 r_distort);
    bool maskWater(U32 r_src, U32 r_dst, bool distort, U32 r_distort);

    bool mergeMasks(const char* r_src, U32 r_dst);
    bool setMaterials(const char* r_src, const char* materials);
};


//--------------------------------------
inline S32 Heightfield::offset(S32 x, S32 y)
{
    return (y << shift) + x;
}


//--------------------------------------
inline F32& Heightfield::val(S32 i)
{
    return (data[i]);
}

//--------------------------------------
inline F32& Heightfield::val(S32 x, S32 y)
{
    return data[(y << shift) + x];
}


//--------------------------------------
inline S32& Heightfield::valS32(S32 i)
{
    return (dataS32[i]);
}

//--------------------------------------
inline S32& Heightfield::valS32(S32 x, S32 y)
{
    return dataS32[(y << shift) + x];
}

//--------------------------------------
inline S32 Terraformer::wrap(S32 p)
{  // simply wrap the coordinate
    return (p & blockMask);
}

//--------------------------------------
inline void Heightfield::block(S32 x, S32 y, F32* a)
{
    // zone = 0-8
    //
    // 0 1 2
    // 3 4 5
    // 5 6 8
    //

    U32 zone = 0;
    if (x > 0)
    {
        zone++;
        if ((U32)x >= mask)
            zone++;
    }
    if (y > 0)
    {
        zone += 3;
        if ((U32)y >= mask)
            zone += 3;
    }

    F32* d = &data[(y << shift) + x];
    S32* p = &zoneOffset[zone * 9];

    a[0] = d[p[0]];
    a[1] = d[p[1]];
    a[2] = d[p[2]];

    a[3] = d[p[3]];
    a[4] = d[p[4]];
    a[5] = d[p[5]];

    a[6] = d[p[6]];
    a[7] = d[p[7]];
    a[8] = d[p[8]];
}


#endif //_H_TERRAFORMER_
