//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGOBJECTSHADOWS_H_
#define _SGOBJECTSHADOWS_H_

#include "shadowBase.h"
#include "lightingSystem/sgObjectBasedProjector.h"
#include "lightingSystem/sgHashMap.h"


struct sgShadowTextureEntryInfo
{
    GFXFormat sgFormat;
    Point2I sgSize;
    U32 sgCreateCount;
};


typedef hash_multimap<U32, ShadowBase*> sgShadowMultimap;
typedef hash_multimap<GFXTexHandle, sgShadowTextureEntryInfo> sgShadowTextureMultimap;


class sgObjectShadows
{
private:
    enum
    {
        sgosFirstEntryHash = 0
    };

    bool sgRegistered;
    U32 sgLastRenderTime;

    //bool sgEnable;
    //bool sgCanMove;
    //bool sgCanRTT;
    //bool sgCanSelfShadow;
    //U32 sgRequestedShadowSize;
    //U32 sgFrameSkip;
    //F32 sgMaxVisibleDistance;
    //F32 sgProjectionDistance;
    //F32 sgSphereAdjust;
    sgShadowMultimap sgShadows;

    // for non-multi-shadows...
    LightInfo sgSingleShadowSource;

    // returns an empty entry (use linkHigh to traverse the list)...
    // generally it's a good idea to expect some entries to be empty...
    sgShadowMultimap* sgGetFirstShadowEntry();
    ShadowBase* createNewShadow(SceneObject* parentObject, LightInfo* light, TSShapeInstance* shapeInstance);
    ShadowBase* sgFindShadow(SceneObject* parentobject,
        LightInfo* light, TSShapeInstance* shapeinstance);
    //void sgUpdateShadow(sgShadowProjector *shadow);
    //void sgUpdateShadows();
    void sgClearMap();
    dsize_t sgLightToHash(LightInfo* light) { return dsize_t(light); }
    LightInfo* sgHashToLight(dsize_t hash) { return (LightInfo*)hash; }

public:
    sgObjectShadows();
    ~sgObjectShadows();
    void sgRender(SceneObject* parentobject, TSShapeInstance* shapeinstance, F32 camdist);
    void sgCleanupUnused(U32 time);
    //void sgSetValues(bool enable, bool canmove, bool canrtt,
    //	bool selfshadow, U32 shadowsize, U32 frameskip,
    //	F32 maxvisibledist, F32 projectiondist, F32 adjust);
};


/**
 * Someone needs to make sure the resource
 * usage doesn't get out of hand...
 */
class sgObjectShadowMonitor
{
private:
    // I should be a map!!!
    static Vector<sgObjectShadows*> sgAllObjectShadows;

public:
    static void sgRegister(sgObjectShadows* shadows);
    static void sgUnregister(sgObjectShadows* shadows);
    static void sgCleanupUnused();
};

class sgShadowTextureCache
{
private:
    static sgShadowTextureMultimap sgShadowTextures;
    static sgShadowTextureMultimap* sgGetFirstEntry();

public:
    static void sgAcquire(GFXTexHandle& texture, Point2I size, U32 format);
    static void sgRelease(GFXTexHandle& texture);
    static void sgClear();
    static void sgCleanupUnused();
    static void sgPrintStats();
};


#endif//_SGOBJECTSHADOWS_H_
