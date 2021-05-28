//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXSTRUCTS_H_
#define _GFXSTRUCTS_H_

#include "gfx/gfxVertexColor.h"
#include "gfx/gfxEnums.h"
#include "math/mMath.h"
#include "platform/profiler.h"

#ifndef _REFBASE_H_
#include "core/refBase.h"
#endif

struct StateTracker
{
    StateTracker()
    {
        currentValue = newValue = 0;
        dirty = false;
    }

    U32 currentValue;
    U32 newValue;
    bool dirty;
};

struct TextureDirtyTracker
{
    TextureDirtyTracker()
    {
        stage = state = 0;
    }

    U32 stage;
    U32 state;
};

//-----------------------------------------------------------------------------

class GFXLightInfo
{
    Point3F position;
    ColorF ambient;
    ColorF diffuse;
    ColorF specular;
    VectorF spotDirection;
    F32 spotExponent;
    F32 spotCutoff;
    F32 constantAttenuation;
    F32 linearAttenuation;
    F32 quadraticAttenuation;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct GFXVideoMode
{
    Point2I resolution;
    U32 bitDepth;
    U32 refreshRate;
    bool fullScreen;
    bool borderless;

    inline bool operator == (GFXVideoMode& otherMode) const
    {
        if (otherMode.fullScreen != fullScreen)
            return false;
        if (otherMode.borderless != borderless)
            return false;
        if (otherMode.resolution != resolution)
            return false;
        if (otherMode.bitDepth != bitDepth)
            return false;
        if (otherMode.refreshRate != refreshRate)
            return false;

        return true;
    }
};

//-----------------------------------------------------------------------------
// GFXShaderFeatureData
//-----------------------------------------------------------------------------
struct GFXShaderFeatureData
{

    // WARNING - if the number of features here grows past 15 or so, then
    //    ShaderManager should implement a hash table or binary tree for lookup
    // WARNING - if this enum is modified - be sure to also change
    //    FeatureMgr::init() which maps features to this enum!
    enum
    {
        RTLighting = 0,   // realtime lighting
        TexAnim,
        BaseTex,
        DynamicLight,
        DynamicLightDual,
        SelfIllumination,
        LightMap,
        LightNormMap,
        BumpMap,
        DetailMap,
        ExposureX2,
        ExposureX4,
        EnvMap,
        CubeMap,
        // BumpCubeMap,
        // Refraction,
        PixSpecular,
        VertSpecular,
        Translucent,      // Not a feature with shader code behind it, but needed for pixSpecular.
        Visibility,
        Fog,              // keep fog last feature
        NumFeatures,
    };

    // lighting info - this will probably get a lot more complex
    bool useLightDir; // use light direction instead of position

    // General feature data for a pass or for other purposes.
    bool features[NumFeatures];

    // This is to give hints to shader creation code.  It contains
    //    all the features that are in a material stage instead of just
    //    the current pass.
    bool materialFeatures[NumFeatures];

public:

    GFXShaderFeatureData();
    U32 codify();

};


//-----------------------------------------------------------------------------
// GFX vertices
//-----------------------------------------------------------------------------
template<class T> inline U32 getGFXVertFlags(T* vertexPtr) { return 0; }


#define DEFINE_VERT(name,flags) struct name; \
            template<> inline U32 getGFXVertFlags(name *ptr) { return flags; }\
            struct name 


// Vertex types
DEFINE_VERT(GFXVertexP, GFXVertexFlagXYZ)
{
    Point3F point;
};

DEFINE_VERT(GFXVertexPT, GFXVertexFlagXYZ | GFXVertexFlagTextureCount1 | GFXVertexFlagUV0)
{
    Point3F point;
    Point2F texCoord;
};

DEFINE_VERT(GFXVertexPTT, GFXVertexFlagXYZ | GFXVertexFlagTextureCount2 | GFXVertexFlagUV0 | GFXVertexFlagUV1)
{
    Point3F point;
    Point2F texCoord1;
    Point2F texCoord2;
};

DEFINE_VERT(GFXVertexPTTT, GFXVertexFlagXYZ | GFXVertexFlagTextureCount3 | GFXVertexFlagUV0 | GFXVertexFlagUV1 | GFXVertexFlagUV2)
{
    Point3F point;
    Point2F texCoord1;
    Point2F texCoord2;
    Point2F texCoord3;
};

DEFINE_VERT(GFXVertexPC, GFXVertexFlagXYZ | GFXVertexFlagDiffuse)
{
    Point3F point;
    GFXVertexColor color;
};

DEFINE_VERT(GFXVertexPCT,
    GFXVertexFlagXYZ | GFXVertexFlagDiffuse | GFXVertexFlagTextureCount1 | GFXVertexFlagUV0)
{
    Point3F point;
    GFXVertexColor color;
    Point2F texCoord;
};

DEFINE_VERT(GFXVertexPN, GFXVertexFlagXYZ | GFXVertexFlagNormal)
{
    Point3F point;
    Point3F normal;
};

DEFINE_VERT(GFXVertexPNT,
    GFXVertexFlagXYZ | GFXVertexFlagNormal | GFXVertexFlagTextureCount1 | GFXVertexFlagUV0)
{
    Point3F point;
    Point3F normal;
    Point2F texCoord;
};

// BJGNOTE - this is a dup, remove me
DEFINE_VERT(GFXTerrainVert,
    GFXVertexFlagXYZ | GFXVertexFlagNormal |
    GFXVertexFlagTextureCount1 | GFXVertexFlagUV0)
{
    Point3F point;
    Point3F normal;
    Point2F texCoord0;
};

/// This is a special purpose vertex that we use with Atlas.
DEFINE_VERT(GFXAtlasVert,
    GFXVertexFlagXYZ | GFXVertexFlagUVW0 | GFXVertexFlagUV1 |
    GFXVertexFlagTextureCount2)
{
    Point3F point;
    Point3F morphCoord;
    Point2F texCoord;
};

DEFINE_VERT(GFXAtlasVert2,
    GFXVertexFlagXYZ | GFXVertexFlagNormal | GFXVertexFlagUV0 |
    GFXVertexFlagUV1 | GFXVertexFlagUVW2 | GFXVertexFlagTextureCount3)
{
    Point3F point;
    Point3F normal;
    Point2F texCoord;
    Point2F texMorphOffset;
    Point3F pointMorphOffset;
};

// This is a big mother vertex - 76 bytes, but it allows a lot of versatilty for
// shaders on interiors.  Ideally, verts should be 24-32 bytes in size.  This
// vertex could be dropped to a much nicer 64 bytes if the normal was removed.
// Without a normal, however, cubemaps and other effects are not possible.
// NOTE - the N value may always be identical to the normal value - something
// to look at - bramage
DEFINE_VERT(GFXVertexPNTTBN,
    GFXVertexFlagXYZ | GFXVertexFlagNormal | GFXVertexFlagTextureCount5 |
    GFXVertexFlagUV0 | GFXVertexFlagUV1 |
    GFXVertexFlagUVW2 | GFXVertexFlagUVW3 | GFXVertexFlagUVW4)
{
    Point3F point;
    Point3F normal;
    Point2F texCoord;
    Point2F texCoord2;

    // texture-space transform 3x3 matrix
    Point3F T;
    Point3F B;
    Point3F N;
};


//-----------------------------------------------------------------------------

struct GFXPrimitive
{
    GFXPrimitiveType type;

    U32 minIndex;       /// minimal value we will see in the indices
    U32 startIndex;     /// start of indices in buffer
    U32 numPrimitives;  /// how many prims to render
    U32 numVertices;    /// how many vertices... (used for locking, we lock from minIndex to minIndex + numVertices)

    GFXPrimitive()
    {
        dMemset(this, 0, sizeof(GFXPrimitive));
    }
};

class GFXPrimitiveBuffer : public RefBase
{
    friend class GFXPrimitiveBufferHandle;
    friend class GFXDevice;
public: //protected:
    U32 mIndexCount;
    U32 mPrimitiveCount;
    GFXBufferType mBufferType;
    GFXPrimitive* mPrimitiveArray;
    GFXDevice* mDevice;

#ifdef TORQUE_DEBUG
    // In debug builds we provide a TOC leak tracking system.
    static U32 smExtantPBCount;
    static GFXPrimitiveBuffer* smHead;
    static void dumpExtantPBs();

    StringTableEntry mDebugCreationPath;
    GFXPrimitiveBuffer* mDebugNext;
    GFXPrimitiveBuffer* mDebugPrev;
#endif

    GFXPrimitiveBuffer(GFXDevice* device, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
    {
        mDevice = device;
        mIndexCount = indexCount;
        mPrimitiveCount = primitiveCount;
        mBufferType = bufferType;
        if (primitiveCount)
        {
            mPrimitiveArray = new GFXPrimitive[primitiveCount];
            dMemset((void*)mPrimitiveArray, 0, primitiveCount * sizeof(GFXPrimitive));
        }
        else
            mPrimitiveArray = NULL;

#ifdef TORQUE_DEBUG
        // Extant tracking.
        smExtantPBCount++;
        mDebugCreationPath = gProfiler->getProfilePath();
        mDebugNext = smHead;
        mDebugPrev = NULL;

        if (smHead)
        {
            AssertFatal(smHead->mDebugPrev == NULL, "GFXPrimitiveBuffer::GFXPrimitiveBuffer - found unexpected previous in current head!");
            smHead->mDebugPrev = this;
        }

        smHead = this;
#endif
    }

    virtual ~GFXPrimitiveBuffer()
    {
        if (mPrimitiveArray != NULL)
        {
            delete[] mPrimitiveArray;
            mPrimitiveArray = NULL;
        }

#ifdef TORQUE_DEBUG
        if (smHead == this)
            smHead = this->mDebugNext;

        if (mDebugNext)
            mDebugNext->mDebugPrev = mDebugPrev;

        if (mDebugPrev)
            mDebugPrev->mDebugNext = mDebugNext;

        mDebugPrev = mDebugNext = NULL;

        smExtantPBCount--;
#endif
    }

    virtual void lock(U16 indexStart, U16 indexEnd, U16** indexPtr) = 0; ///< locks this primitive buffer for writing into
    virtual void unlock() = 0; ///< unlocks this primitive buffer.
    virtual void prepare() = 0;  ///< prepares this primitive buffer for use on the device it was allocated on
};

class GFXPrimitiveBufferHandle : public RefPtr<GFXPrimitiveBuffer>
{
    typedef RefPtr<GFXPrimitiveBuffer> Parent;
public:
    enum Constants {
        MaxIndexCount = 65535,
    };
    GFXPrimitiveBufferHandle() {};
    GFXPrimitiveBufferHandle(GFXDevice* theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
    {
        set(theDevice, indexCount, primitiveCount, bufferType);
    }
    void set(GFXDevice* theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType);
    void lock(U16** indexBuffer, GFXPrimitive** primitiveBuffer = NULL, U32 indexStart = 0, U32 indexEnd = 0)
    {
        if (indexEnd == 0)
            indexEnd = getPointer()->mIndexCount;
        AssertFatal(indexStart < indexEnd&& indexEnd <= getPointer()->mIndexCount, "Out of range index lock!");
        getPointer()->lock(indexStart, indexEnd, indexBuffer);
        if (primitiveBuffer)
            *primitiveBuffer = getPointer()->mPrimitiveArray;
    }
    void unlock()
    {
        getPointer()->unlock();
    }
    void prepare()
    {
        getPointer()->prepare();
    }
    bool operator==(const GFXPrimitiveBufferHandle& buffer) const {
        return getPointer() == buffer.getPointer();
    }
    GFXPrimitiveBufferHandle& operator=(GFXPrimitiveBuffer* ptr)
    {
        RefObjectRef::set(ptr);
        return *this;
    }
};

/// Helper class to deal with OS specific debug functionality.
class GFXDebugMarker
{
private:
    ColorF mColor;
    const char* mName;

    bool mActive;

public:
    GFXDebugMarker(ColorF color, const char* name)
        : mColor(color), mName(name)
    {
        //
        mActive = false;
    }

    /// We automatically mark the marker as exited if we leave scope.
    ~GFXDebugMarker()
    {
        if (mActive)
            leave();
    }

    void enter();
    void leave();
};

#endif
