//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SCENESTATE_H_
#define _SCENESTATE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class SceneObject;
class InteriorInstance;

//--------------------------------------------------------------------------
//-------------------------------------- SceneState
//

struct FogVolume;

/// The SceneState describes the state of the scene being rendered. It keeps track
/// of the information that objects need to render properly with regard to the
/// camera position, any fog information, viewing frustum, the global environment
/// map for reflections, viewable distance and portal information.
class SceneState
{
    friend class SceneGraph;

public:
    struct FogBand
    {
        bool isFog;
        float cap;
        float factor;
        ColorF color;
    };

    struct ZoneState {
        bool  render;
        F64   frustum[4];    ///< Winding order is:  left, right, bottom, top.
        RectI viewport;

        bool   clipPlanesValid;
        PlaneF clipPlanes[5];
    };

    struct InteriorListElem
    {
        InteriorInstance* obj;
        U32            stateKey;
        U32            startZone;
        U32            detailLevel;
        MatrixF* worldXform;
    };

    /// Sets up the clipping planes using the parameters from a ZoneState.
    ///
    /// @param   zone   ZoneState to initalize clipping to
    void setupClipPlanes(ZoneState& zone);

    /// Used to represent a portal which inserts a transformation into the scene.
    struct TransformPortal {
        SceneObject* owner;
        U32          portalIndex;
        U32          globalZone;
        Point3F      traverseStart;
        bool         flipCull;
    };

public:
    /// Constructor
    ///
    /// @see SceneGraph::renderScene
    SceneState(SceneState* parent,
        const U32      numZones,
        F64            left,
        F64            right,
        F64            bottom,
        F64            top,
        F64            nearPlane,
        F64            farPlane,
        RectI          viewport,
        const Point3F& camPos,
        const MatrixF& modelview,
        F32            fogDistance,
        F32            visibleDistance,
        ColorF         fogColor,
        U32            numFogVolumes,
        FogVolume* fogVolumes,
        F32            visFactor);

    ~SceneState();

    /// Sets the active portal.
    ///
    /// @param   owner Object which owns the portal (portalized object)
    /// @param   idx   Index of the portal in the list of portal planes
    void setPortal(SceneObject* owner, const U32 idx);

    /// Returns the camera position this SceneState is using.
    const Point3F& getCameraPosition() const { return mCamPosition; }

    /// Returns the minimum distance something must be from the camera to not be culled.
    F64            getNearPlane()      const { return mNearPlane; }

    /// Returns the maximum distance something can be from the camera to not be culled.
    F64            getFarPlane()       const { return mFarPlane; }

    /// Returns the base ZoneState.
    ///
    /// @see ZoneState
    const ZoneState& getBaseZoneState() const;

    /// Returns the base ZoneState as a non-const reference.
    ///
    /// @see getBaseZoneState
    ZoneState& getBaseZoneStateNC();

    /// Returns the ZoneState for a particular zone ID.
    ///
    /// @param   zoneId   ZoneId
    const ZoneState& getZoneState(const U32 zoneId) const;

    /// Returns the ZoneState for a particular zone ID as a non-const reference.
    ///
    /// @see getZoneState
    /// @param   zoneId   ZoneId
    ZoneState& getZoneStateNC(const U32 zoneId);

    /// Adds a new transform portal to the SceneState.
    ///
    /// @param   owner                 SceneObject owner of the portal (portalized object).
    /// @param   portalIndex           Index of the portal in the list of portal planes.
    /// @param   globalZone            Index of the zone this portal is in in the list of ZoneStates.
    /// @param   traversalStartPoint   Start point of the zone traversal.
    ///                                @see SceneGraph::buildSceneTree
    ///                                @see SceneGraph::findZone
    ///
    /// @param   flipCull              If true, the portal plane will be flipped
    void insertTransformPortal(SceneObject* owner, U32 portalIndex,
        U32 globalZone, const Point3F& traversalStartPoint,
        const bool flipCull);

    /// This enables terrain to be drawn inside interiors.
    void enableTerrainOverride();

    /// Returns true if terrain is allowed to be drawn inside interiors.
    bool isTerrainOverridden() const;

    /// Sorts the list of images, builds the translucency BSP tree,
    /// sets up the portal, then renders all images in the state.
    void renderCurrentImages();

    /// Returns true if the object in question is going to be rendered
    /// as opposed to being culled out.
    ///
    /// @param   obj   Object in question.
    bool isObjectRendered(const SceneObject* obj);

    /// Sets the viewport and projection up for the given zone.
    ///
    /// @param   zone   Zone for which we want to set up a viewport/projection.
    void setupZoneProjection(const U32 zone);

    /// Sets up the projection matrix based on the zone the specified object is in.
    ///
    /// @param   object   Object to use.
    void setupObjectProjection(const SceneObject* object);

    /// Sets up the projection matrix based on the base ZoneState.
    void setupBaseProjection();

    /// Returns the distance at which objects are no longer visible.
    F32 getVisibleDistance();

    /// Returns how far away fog is (ie, when objects will be fully fogged).
    F32 getFogDistance();

    /// Returns the color of the fog in the scene.
    ColorF getFogColor();

    /// Sets up all the fog volumes in the Scene.
    void setupFog();

    /// Returns true if the box specified is visible, taking fog into account.
    ///
    /// @param   dist   The length of the vector, projected on to the horizontal plane of the world, from the camera to the nearest point on the box
    /// @param   top   The maximum z-value of the box
    /// @param   bottom   The minimum z-value of the box
    bool isBoxFogVisible(F32 dist, F32 top, F32 bottom);

    /// Returns a value between 0.0 and 1.0 which indicates how obscured a point is by haze and fog.
    ///
    /// @param   dist     Length of vector, projected onto horizontal world plane, from camera to the point.
    /// @param   deltaZ   Z-offset of the camera.
    F32 getHazeAndFog(float dist, float deltaZ);

    /// Returns a value between 0.0 and 1.0 which indicates how obscured a point is by fog only.
    ///
    /// @param   dist     Length of vector, projected onto horizontal world plane, from camera to the point.
    /// @param   deltaZ   Z-offset of the camera.
    F32 getFog(float dist, float deltaZ);

    /// Returns a value between 0.0 and 1.0 which indicates how obscured a point is by fog only.
    ///
    /// @param   dist     Length of vector, projected onto horizontal world plane, from camera to the point.
    /// @param   deltaZ   Z-offset of the camera.
    /// @param   volKey   Index of a particular band of volumetric fog.
    F32 getFog(float dist, float deltaZ, S32 volKey);

    /// Returns all fogs at a point.
    ///
    /// @param   dist     Length of vector, projected onto horizontal world plane, from camera to the point.
    /// @param   deltaZ   Z-offset of the camera.
    /// @param   array    Array of fog colors at the point.
    /// @param   numFogs  Number of fogs in the array.
    void getFogs(float dist, float deltaZ, ColorF* array, U32& numFogs);

    /// Returns a value between 0.0 and 1.0 which indicates how obscured a point is by haze only.
    ///
    /// @param   dist     Length of vector, projected onto horizontal world plane, from camera to the point.
    F32 getHaze(F32 dist);

private:
    Vector<ZoneState>         mZoneStates;             ///< Collection of ZoneStates in the scene.

    /// Builds the BSP tree of translucent images.
    void buildTranslucentBSP();

    bool mTerrainOverride;                             ///< If true, terrain is allowed to render inside interiors

    Vector<SceneState*>       mSubsidiaries;           ///< Transform portals which have been processed by the scene traversal process
                                                       ///
                                                       ///  @note Closely related.  Transform portals are turned into sorted mSubsidiaries
                                                       ///        by the traversal process...

    Vector<TransformPortal>   mTransformPortals;       ///< Collection of TransformPortals

    Vector<FogBand> mPosFogBands;                      ///< Fog bands above the world plane
    Vector<FogBand> mNegFogBands;                      ///< Fog bands below the world plane

    ZoneState mBaseZoneState;                          ///< ZoneState of the base zone of the scene
    Point3F   mCamPosition;                            ///< Camera position in this state

    F64       mNearPlane;                              ///< Minimum distance an object must be from the camera to get rendered
    F64       mFarPlane;                               ///< Maximum distance an object can be from the camera to get rendered

    F32       mVisFactor;                              ///< Visibility factor of the scene, used to modify fog

    SceneState* mParent;                               ///< This is used for portals, if this is not NULL, then this SceneState belongs to a portal, and not the main SceneState

    SceneObject* mPortalOwner;                         ///< SceneObject which owns the current portal
    U32          mPortalIndex;                         ///< Index the current portal is in the list of portals

    ColorF   mFogColor;                                ///< Distance based, not volumetric, fog color
    F32 mFogDistance;                                  ///< Distance to distance-fog
    F32 mVisibleDistance;                              ///< Visible distance of the scene
    F32 mFogScale;                                     ///< Fog scale of the distance based fog

    U32 mNumFogVolumes;                                ///< Number of fog volumes in the scene
    FogVolume* mFogVolumes;                            ///< Pointer to the array of fog volumes in the scene

    Vector< InteriorListElem > mInteriorList;


public:
    bool    mFlipCull;                                 ///< If true the portal clipping plane will be reversed
    MatrixF mModelview;                                ///< Modelview matrix this scene is based off of

    /// Returns the fog bands above the world plane
    Vector<FogBand>* getPosFogBands() { return &mPosFogBands; }

    /// Returns the fog bands below the world planes
    Vector<FogBand>* getNegFogBands() { return &mNegFogBands; }

    void insertInterior(InteriorListElem& elem)
    {
        mInteriorList.push_back(elem);
    }
};

inline F32 SceneState::getHaze(F32 dist)
{
    if (dist <= mFogDistance)
        return 0;

    if (dist > mVisibleDistance)
        return 1.0;

    F32 distFactor = (dist - mFogDistance) * mFogScale - 1.0f;
    return 1.0f - distFactor * distFactor;
}

inline F32 SceneState::getVisibleDistance()
{
    return mVisibleDistance;
}

inline F32 SceneState::getFogDistance()
{
    return mFogDistance;
}

inline ColorF SceneState::getFogColor()
{
    return mFogColor;
}

inline const SceneState::ZoneState& SceneState::getBaseZoneState() const
{
    return mBaseZoneState;
}

inline SceneState::ZoneState& SceneState::getBaseZoneStateNC()
{
    return mBaseZoneState;
}

inline const SceneState::ZoneState& SceneState::getZoneState(const U32 zoneId) const
{
    AssertFatal(zoneId < (U32)mZoneStates.size(), "Error, out of bounds zone!");
    return mZoneStates[zoneId];
}

inline SceneState::ZoneState& SceneState::getZoneStateNC(const U32 zoneId)
{
    AssertFatal(zoneId < (U32)mZoneStates.size(), "Error, out of bounds zone!");
    return mZoneStates[zoneId];
}

inline void SceneState::enableTerrainOverride()
{
    mTerrainOverride = true;
}

inline bool SceneState::isTerrainOverridden() const
{
    return mTerrainOverride;
}


#endif  // _H_SCENESTATE_

