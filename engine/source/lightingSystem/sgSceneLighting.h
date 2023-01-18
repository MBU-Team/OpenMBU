//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGSCENELIGHTING_H_
#define _SGSCENELIGHTING_H_

#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifdef TORQUE_TERRAIN
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#endif
#ifndef _INTERIORINSTANCE_H_
#include "interior/interiorInstance.h"
#endif
#ifndef _SHADOWVOLUMEBSP_H_
#include "sceneGraph/shadowVolumeBSP.h"
#endif
#ifdef TORQUE_TERRAIN
#include "atlas/runtime/atlasInstance2.h"
#endif
#include "math/mathUtils.h"
#include "lightingSystem/sgScenePersist.h"
#include "lightingSystem/sgLightMap.h"


class SceneLighting : public SimObject
{
    typedef SimObject Parent;
public:
    S32 sgTimeTemp;
    S32 sgTimeTemp2;
    void sgNewEvent(U32 light, S32 object, U32 event);

    void sgLightingStartEvent();
    void sgLightingCompleteEvent();

    void sgTGEPassSetupEvent();
    void sgTGELightStartEvent(U32 light);
    void sgTGELightProcessEvent(U32 light, S32 object);
    void sgTGELightCompleteEvent(U32 light);
    void sgTGESetProgress(U32 light, S32 object);

    void sgSGPassSetupEvent();
    void sgSGObjectStartEvent(S32 object);
    void sgSGObjectProcessEvent(U32 light, S32 object);
    void sgSGObjectCompleteEvent(S32 object);
    void sgSGSetProgress(U32 light, S32 object);

    bool sgIsCorrectStaticObjectType(SceneObject* obj);

    // 'sg' prefix omitted to conform with existing 'addInterior' method...
    void addStatic(void* terrainproxy, ShadowVolumeBSP* shadowVolume,
        SceneObject* sceneobject, LightInfo* light, S32 level);


    // persist objects moved to 'sgScenePersist.h' for clarity...
    // everything below this line should be original code...

    U32 calcMissionCRC();

    bool verifyMissionInfo(PersistInfo::PersistChunk*);
    bool getMissionInfo(PersistInfo::PersistChunk*);

    bool loadPersistInfo(const char*);
    bool savePersistInfo(const char*);

    class ObjectProxy;
    class TerrainProxy;
    class InteriorProxy;

    enum {
        SHADOW_DETAIL = -1
    };

    void addInterior(ShadowVolumeBSP*, InteriorProxy&, LightInfo*, S32);
    ShadowVolumeBSP::SVPoly* buildInteriorPoly(ShadowVolumeBSP*, InteriorProxy&, Interior*, U32, LightInfo*, bool);

    //------------------------------------------------------------------------------
    /// Create a proxy for each object to store data.
    class ObjectProxy
    {
    public:
        SimObjectPtr<SceneObject>     mObj;
        U32                           mChunkCRC;

        ObjectProxy(SceneObject* obj) : mObj(obj) { mChunkCRC = 0; }
        virtual ~ObjectProxy() {}
        SceneObject* operator->() { return(mObj); }
        SceneObject* getObject() { return(mObj); }

        /// @name Lighting Interface
        /// @{
        virtual bool loadResources() { return(true); }
        virtual void init() {}
        virtual bool preLight(LightInfo*) { return(false); }
        virtual void light(LightInfo*) {}
        virtual void postLight(bool lastLight) {}
        /// @}

        /// @name Persistence
        ///
        /// We cache lighting information to cut down on load times.
        ///
        /// There are flags such as ForceAlways and LoadOnly which allow you
        /// to control this behaviour.
        /// @{
        bool calcValidation();
        bool isValidChunk(PersistInfo::PersistChunk*);

        virtual U32 getResourceCRC() = 0;
        virtual bool setPersistInfo(PersistInfo::PersistChunk*);
        virtual bool getPersistInfo(PersistInfo::PersistChunk*);
        /// @}
    };

#ifdef TORQUE_TERRAIN
    class AtlasChunkProxy : public ObjectProxy
    {
    public:
        Box3F sgChunkBox;
        AtlasInstance2* sgAtlas;
        AtlasResourceGeomTOC::StubType* sgStub;
        sgAtlasLightMap* sgLightMapData;

        AtlasChunkProxy(SceneObject* obj, AtlasResourceGeomTOC::StubType* stub) : ObjectProxy(obj)
        {
            sgAtlas = dynamic_cast<AtlasInstance2*>((SceneObject*)mObj);
            sgStub = stub;
            sgLightMapData = NULL;
            MathUtils::transformBoundingBox(sgStub->mBounds,
                sgAtlas->getRenderTransform(), sgChunkBox);

            AssertFatal((sgAtlas && sgStub), "Invalid atlas or stub instance.");
        }
        virtual ~AtlasChunkProxy()
        {
            if (sgLightMapData)
                delete sgLightMapData;
        }

        virtual bool preLight(LightInfo* light)
        {
            // this doesn't actually prevent the call to light...
            if (!sgRelightFilter::sgAllowLighting(sgChunkBox, false))
                return false;

            // not created yet?
            if (!sgLightMapData)
            {
                // need to calc this based on chunks and total size...
                F32 edgedivide = mPow(2.0f, F32(sgStub->level));
                U32 lmsize = F32(sgAtlas->lightMapSize) / edgedivide;
                Point2I size(lmsize, lmsize);
                sgLightMapData = new sgAtlasLightMap(
                    size.x, size.y, sgAtlas, sgStub);

                // setup light map data...
                sgStub->mBounds.getCenter(&sgLightMapData->sgWorldPosition);
                sgLightMapData->sgLightMapSVector = Point3F(
                    (sgStub->mBounds.max.x - sgStub->mBounds.min.x), 0, 0);
                sgLightMapData->sgLightMapTVector = Point3F(
                    0, (sgStub->mBounds.max.y - sgStub->mBounds.min.y), 0);

                sgLightMapData->sgLightMapSVector.x /= F32(size.x);
                sgLightMapData->sgLightMapTVector.y /= F32(size.y);

                // should apply scale to these!!!
                sgAtlas->getRenderTransform().mulP(sgLightMapData->sgWorldPosition);
                sgAtlas->getRenderTransform().mulV(sgLightMapData->sgLightMapSVector);
                sgAtlas->getRenderTransform().mulV(sgLightMapData->sgLightMapTVector);

                sgLightMapData->sgSetupLighting();
            }
            return true;
        }
        virtual void light(LightInfo* light)
        {
            // filtered out...
            if (!sgLightMapData)
                return;

            sgLightMapData->sgCalculateLighting(light);
        }
        virtual void postLight(bool lastlight)
        {
            // filtered out or not last...
            if (!sgLightMapData || !lastlight)
                return;

            GBitmap* lm = sgAtlas->lightMapTex.getBitmap();
            AssertFatal((lm), "Invalid light map.");

            Point2I offset = sgStub->pos;
            offset.x *= sgLightMapData->sgWidth;
            offset.y *= sgLightMapData->sgHeight;
            sgLightMapData->sgMergeLighting(lm, offset.x, offset.y);

            sgAtlas->lightMapDirty = true;

            delete sgLightMapData;
            sgLightMapData = NULL;
        }

        virtual U32 getResourceCRC() { return 1; }
        virtual bool setPersistInfo(PersistInfo::PersistChunk*) { return true; }
        virtual bool getPersistInfo(PersistInfo::PersistChunk*) { return true; }
    };

    class AtlasLightMapProxy : public ObjectProxy
    {
    private:
        typedef ObjectProxy Parent;
    public:
        AtlasInstance2* sgAtlas;
        AtlasLightMapProxy(SceneObject* obj) : ObjectProxy(obj)
        {
            sgAtlas = dynamic_cast<AtlasInstance2*>((SceneObject*)mObj);
            AssertFatal((sgAtlas), "Invalid atlas or stub instance.");
        }
        virtual U32 getResourceCRC() { return 1; }
        virtual bool setPersistInfo(PersistInfo::PersistChunk*);
        virtual bool getPersistInfo(PersistInfo::PersistChunk*);
    };
#endif

    class InteriorProxy : public ObjectProxy
    {
    private:
        typedef  ObjectProxy       Parent;
        bool isShadowedBy(InteriorProxy*);

    public:

        InteriorProxy(SceneObject* obj);
        ~InteriorProxy();
        InteriorInstance* operator->() { return(static_cast<InteriorInstance*>(static_cast<SceneObject*>(mObj))); }
        InteriorInstance* getObject() { return(static_cast<InteriorInstance*>(static_cast<SceneObject*>(mObj))); }

        // current light info
        ShadowVolumeBSP* mBoxShadowBSP;
        Vector<ShadowVolumeBSP::SVPoly*>    mLitBoxSurfaces;
        Vector<PlaneF>                      mOppositeBoxPlanes;
        Vector<PlaneF>                      mTerrainTestPlanes;


        struct sgSurfaceInfo
        {
            const Interior::Surface* sgSurface;
            U32 sgIndex;
            Interior* sgDetail;
            bool sgHasAlarm;
        };
        U32 sgCurrentSurfaceIndex;
        U32 sgSurfacesPerPass;
        InteriorInstance* sgInterior;
        Vector<LightInfo*> sgLights;
        Vector<sgSurfaceInfo> sgSurfaces;

        void sgAddLight(LightInfo* light, InteriorInstance* interior);
        //void sgLightUniversalPoint(LightInfo *light);
        void sgProcessSurface(const Interior::Surface& surface, U32 i, Interior* detail, bool hasAlarm);


        // lighting interface
        bool loadResources();
        void init();
        bool preLight(LightInfo*);
        void light(LightInfo*);
        void postLight(bool lastLight);

        // persist
        U32 getResourceCRC();
        bool setPersistInfo(PersistInfo::PersistChunk*);
        bool getPersistInfo(PersistInfo::PersistChunk*);
    };

#ifdef TORQUE_TERRAIN
    class TerrainProxy : public ObjectProxy
    {
    private:
        typedef  ObjectProxy    Parent;

        BitVector               mShadowMask;
        ShadowVolumeBSP* mShadowVolume;
        ColorF* mLightmap;


        ColorF* sgBakedLightmap;
        Vector<LightInfo*> sgLights;
        void sgAddUniversalPoint(LightInfo* light);
        void sgLightUniversalPoint(LightInfo* light, TerrainBlock* terrain);
        bool sgMarkStaticShadow(void* terrainproxy, SceneObject* sceneobject, LightInfo* light);
        void postLight(bool lastLight);


        void lightVector(LightInfo*);

        struct SquareStackNode
        {
            U8          mLevel;
            U16         mClipFlags;
            Point2I     mPos;
        };

        S32 testSquare(const Point3F&, const Point3F&, S32, F32, const Vector<PlaneF>&);
        bool markInteriorShadow(InteriorProxy*);

    public:

        TerrainProxy(SceneObject* obj);
        ~TerrainProxy();
        TerrainBlock* operator->() { return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj))); }
        TerrainBlock* getObject() { return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj))); }

        bool getShadowedSquares(const Vector<PlaneF>&, Vector<U16>&);

        // lighting
        void init();
        bool preLight(LightInfo*);
        void light(LightInfo*);

        // persist
        U32 getResourceCRC();
        bool setPersistInfo(PersistInfo::PersistChunk*);
        bool getPersistInfo(PersistInfo::PersistChunk*);
    };
#endif

    typedef Vector<ObjectProxy*>  ObjectProxyList;

    ObjectProxyList            mSceneObjects;
    ObjectProxyList            mLitObjects;

    LightInfoList              mLights;

    SceneLighting();
    ~SceneLighting();

    enum Flags {
        ForceAlways = BIT(0),   ///< Regenerate the scene lighting no matter what.
        ForceWritable = BIT(1),   ///< Regenerate the scene lighting only if we can write to the lighting cache files.
        LoadOnly = BIT(2),   ///< Just load cached lighting data.
    };
    static bool lightScene(const char*, BitSet32 flags = 0);
    static bool isLighting();

    S32                        mStartTime;
    char                       mFileName[1024];
    static bool                smUseVertexLighting;

    bool light(BitSet32);
    void completed(bool success);
    void processEvent(U32 light, S32 object);
    void processCache();

    // inlined
    bool isAtlas(SceneObject*);

#ifdef TORQUE_TERRAIN
    bool isTerrain(SceneObject*);
#endif
    bool isInterior(SceneObject*);
};

class sgSceneLightingProcessEvent : public SimEvent
{
private:
    U32 sgLightIndex;
    S32 sgObjectIndex;
    U32 sgEvent;

public:
    enum sgEventTypes
    {
        sgLightingStartEventType,
        sgLightingCompleteEventType,

        sgSGPassSetupEventType,
        sgSGObjectStartEventType,
        sgSGObjectCompleteEventType,
        sgSGObjectProcessEventType,

        sgTGEPassSetupEventType,
        sgTGELightStartEventType,
        sgTGELightCompleteEventType,
        sgTGELightProcessEventType
    };

    sgSceneLightingProcessEvent(U32 lightIndex, S32 objectIndex, U32 event)
    {
        sgLightIndex = lightIndex;
        sgObjectIndex = objectIndex;
        sgEvent = event;
    }
    void process(SimObject* object)
    {
        AssertFatal(object, "SceneLightingProcessEvent:: null event object!");
        if (!object)
            return;

        SceneLighting* sl = static_cast<SceneLighting*>(object);
        switch (sgEvent)
        {
        case sgLightingStartEventType:
            sl->sgLightingStartEvent();
            break;
        case sgLightingCompleteEventType:
            sl->sgLightingCompleteEvent();
            break;

        case sgTGEPassSetupEventType:
            sl->sgTGEPassSetupEvent();
            break;
        case sgTGELightStartEventType:
            sl->sgTGELightStartEvent(sgLightIndex);
            break;
        case sgTGELightProcessEventType:
            sl->sgTGELightProcessEvent(sgLightIndex, sgObjectIndex);
            break;
        case sgTGELightCompleteEventType:
            sl->sgTGELightCompleteEvent(sgLightIndex);
            break;

        case sgSGPassSetupEventType:
            sl->sgSGPassSetupEvent();
            break;
        case sgSGObjectStartEventType:
            sl->sgSGObjectStartEvent(sgObjectIndex);
            break;
        case sgSGObjectProcessEventType:
            sl->sgSGObjectProcessEvent(sgLightIndex, sgObjectIndex);
            break;
        case sgSGObjectCompleteEventType:
            sl->sgSGObjectCompleteEvent(sgObjectIndex);
            break;

        default:
            return;
        };
    };
};



//------------------------------------------------------------------------------

inline bool SceneLighting::isAtlas(SceneObject* obj)
{
    return obj && ((obj->getTypeMask() & AtlasObjectType) != 0);
}

#ifdef TORQUE_TERRAIN
inline bool SceneLighting::isTerrain(SceneObject* obj)
{
    return obj && ((obj->getTypeMask() & TerrainObjectType) != 0);
}
#endif

inline bool SceneLighting::isInterior(SceneObject* obj)
{
    return obj && ((obj->getTypeMask() & InteriorObjectType) != 0);
}


#endif//_SGSCENELIGHTING_H_
