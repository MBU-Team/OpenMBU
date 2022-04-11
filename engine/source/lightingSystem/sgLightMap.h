//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGLIGHTMAP_H_
#define _SGLIGHTMAP_H_

#include "lightingSystem/sgLighting.h"


class sgShadowObjects
{
public:
    static VectorPtr<SceneObject*> sgObjects;
    static void sgGetObjects(SceneObject* obj);
};

class sgColorMap
{
public:
    U32 sgWidth;
    U32 sgHeight;
    ColorF* sgData;
    sgColorMap(U32 width, U32 height)
    {
        sgWidth = width;
        sgHeight = height;
        sgData = new ColorF[(width * height)];
        dMemset(sgData, 0, (width * height * sizeof(ColorF)));
    }
    ~sgColorMap() { delete[] sgData; }
    void sgFillInLighting();
    void sgBlur();
    //void sgMerge(GBitmap *lightmap, U32 xoffset, U32 yoffset, bool normalblend);
};

/**
 * The base class for generating mission level or real-time light maps
 * for any sceneObject.  All actual work is performed in the descendent
 * class' sgLightMap::sgCalculateLighting method.
 */
class sgLightMap
{
public:
    U32 sgWidth;
    U32 sgHeight;
    /// The light map color buffer.
    sgColorMap* sgTexels;
    /// The light map vector buffer.
    sgColorMap* sgVectors;
public:
    /// Requests self shadowing (if available).
    bool sgSelfShadowing;
    /// Requests faster lighting calculations (if available).
    bool sgChooseSpeedOverQuality;
    /// The world space position that the texture space coord (0, 0) represents.
    Point3F sgWorldPosition;
    /// Defines the world space directional change
    /// corresponding to a change of (+1, 0) in the light map texture space.
    /// Similar to the tangent vector in dot3 bump mapping.
    Point3F sgLightMapSVector;
    /// Defines the world space directional change
    /// corresponding to a change of (0, +1) in the light map texture space.
    /// Similar to the binormal vector in dot3 bump mapping.
    Point3F sgLightMapTVector;
    sgLightMap(U32 width, U32 height)
    {
        sgWidth = width;
        sgHeight = height;
        sgSelfShadowing = SG_SELF_SHADOWING;
        sgChooseSpeedOverQuality = SG_CHOOSE_SPEED_OVER_QUALITY;
        sgTexels = new sgColorMap(width, height);
        sgVectors = new sgColorMap(width, height);
    }
    ~sgLightMap()
    {
        delete sgTexels;
        delete sgVectors;
    }
    /// Object specific light mapping calculations are done here.
    virtual void sgSetupLighting() {}// default...
    virtual void sgCalculateLighting(LightInfo* light) = 0;
protected:
    VectorPtr<SceneObject*> sgIntersectingObjects;
    void sgGetIntersectingObjects(const Box3F& surfacebox, const LightInfo* light);
};

/**
 * Used to generate light maps on interiors.  This class will
 * calculate one surface at a time (using sgPlanarLightMap::sgSurfaceIndex).
 */
class sgPlanarLightMap : public sgLightMap
{
private:
    struct sgLexel
    {
        Point2D lmPos;
        Point3F worldPos;
    };
    enum sgLightingPass
    {
        sglpInner = 0,
        sglpOuter,
        sglpCount
    };
    enum sgAdjacent
    {
        sgaTrue,
        sgaFalse,
        // I like this one - fuzzy logic in action "umm... maybe?"... :)
        sgaMaybe
    };
protected:
    /// Surface to generate light map.
    S32 sgSurfaceIndex;
    /// The surface normal.
    Point3F sgPlaneNormal;
    /// The interior instance (the same as
    /// sgLightMap::sgSceneObject, not sure why we need both).
    InteriorInstance* sgInteriorInstance;
    /// The current interior detail level (so all details can
    /// receive light maps).
    Interior* sgInteriorCurrentDetail;
    Box3F sgSurfaceBox;
    Vector<sgLexel> sgInnerLexels;
    Vector<sgLexel> sgOuterLexels;
public:
    sgPlanarLightMap(U32 width, U32 height, InteriorInstance* interiorinstance,
        Interior* currentdetail, S32 surfaceindex, Point3F normal)
        : sgLightMap(width, height)
    {
        sgDirty = false;
        sgSurfaceIndex = -1;
        sgInteriorInstance = interiorinstance;
        sgInteriorCurrentDetail = currentdetail;
        sgSurfaceIndex = surfaceindex;
        sgPlaneNormal = normal;
    }
    /// Transfer the light map to a GBitmap.
    void sgMergeLighting(GBitmap* lightmap, GBitmap* normalmap, U32 xoffset, U32 yoffset);
    /// See: sgLightMap::sgCalculateLighting.
    virtual void sgSetupLighting();
    virtual void sgCalculateLighting(LightInfo* light);
    bool sgIsDirty() { return sgDirty; }
protected:
    bool sgDirty;
    /// Try to avoid false shadows by ignoring direct neighbors.
    U32 sgAreAdjacent(U32 surface1, U32 surface2);
};

#ifdef TORQUE_TERRAIN
/**
 * Used to generate terrain light maps.
 */
class sgTerrainLightMap : public sgLightMap
{
public:
    TerrainBlock* sgTerrain;
    sgTerrainLightMap(U32 width, U32 height, TerrainBlock* terrain)
        : sgLightMap(width, height)
    {
        sgTerrain = terrain;
    }
    void sgMergeLighting(ColorF* lightmap);
    /// See: sgLightMap::sgGetBoundingBox.
    virtual void sgCalculateLighting(LightInfo* light);
};

class sgAtlasLightMap : public sgLightMap
{
public:
    Box3F sgChunkBox;
    AtlasInstance2* sgAtlas;
    AtlasResourceGeomTOC::StubType* sgStub;
    AtlasSurfaceQueryHelper sgAtlasSurfaceHelper;
    sgAtlasLightMap(U32 width, U32 height, AtlasInstance2* terrain,
        AtlasResourceGeomTOC::StubType* stub)
        : sgLightMap(width, height)
    {
        sgAtlas = terrain;
        sgStub = stub;
    }
    void sgMergeLighting(GBitmap* lightmap, U32 xoffset, U32 yoffset);
    /// See: sgLightMap::sgGetBoundingBox.
    virtual void sgSetupLighting();
    virtual void sgCalculateLighting(LightInfo* light);
};
#endif

#endif//_SGLIGHTMAP_H_
