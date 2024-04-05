//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORINSTANCE_H_
#define _INTERIORINSTANCE_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _INTERIORRES_H_
#include "interior/interiorRes.h"
#endif
#ifndef _INTERIORLMMANAGER_H_
#include "interior/interiorLMManager.h"
#endif
#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _INTERIOR_H_
#include "interior.h"
#endif
#ifndef _REFLECTPLANE_H_
#include "gfx/reflectPlane.h"
#endif


class AbstractPolyList;
class InteriorSubObject;
class InteriorResTrigger;
class MaterialList;
class TextureObject;
class FloorPlan;
class Convex;
class SFXProfile;
class SFXEnvironment;

//--------------------------------------------------------------------------
class InteriorInstance : public SceneObject
{
    typedef SceneObject Parent;
    friend class SceneLighting;
    friend class FloorPlan;
    friend class PathedInterior;

public:
    InteriorInstance();
    ~InteriorInstance();


    S32 getSurfaceZone(U32 surfaceindex, Interior* detail);


    static void init();
    static void destroy();

    // Collision
public:
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
    virtual void setTransform(const MatrixF& mat);

    void buildConvex(const Box3F& box, Convex* convex);
private:
    Convex* mConvexList;

public:
    /// This returns true if the interior is in an alarm state. Alarm state
    /// will put different lighting into the interior and also possibly
    /// have an audio element also.
    bool inAlarmState() { return(mAlarmState); }

    /// This sets the alarm mode of the interior.
    /// @param   alarm   If true the interior will be in an alarm state next frame
    void setAlarmMode(const bool alarm);

public:
    /// @name Subobject access interface
    /// @{

    /// Returns the number of detail levels for an object
    U32 getNumDetailLevels();

    /// Gets the interior associated with a particular detail level
    /// @param   level   Detail level
    Interior* getDetailLevel(const U32 level);

    /// Sets the detail level to render manually
    /// @param   level   Detail level to force
    void setDetailLevel(S32 level = -1) { mForcedDetailLevel = level; }
    /// @}

    // Material management for overlays
public:

    /// Reloads material information if the interior skin changes
    void renewOverlays();

    /// Sets the interior skin to something different
    /// @param   newBase   New base skin
    void setSkinBase(const char* newBase);

    void addChildren();
    void addDoors(bool hide, Vector<U32>& usedTriggerIds);
    void addTriggers(Vector<U32>& usedTriggerIds);

public:
    static bool smDontRestrictOutside;
    static F32  smDetailModification;


    DECLARE_CONOBJECT(InteriorInstance);
    static void initPersistFields();
    static void consoleInit();

    /// Reads the lightmaps of the interior into the provided pointer
    /// @param   lightmaps   Lightmaps in the interior (out)
    bool readLightmaps(GBitmap**** lightmaps);

protected:
    bool onAdd();
    void onRemove();

    void inspectPreApply();
    void inspectPostApply();

    bool onSceneAdd(SceneGraph* graph);
    void onSceneRemove();
    U32  getPointZone(const Point3F& p);
    bool getOverlappingZones(SceneObject* obj, U32* zones, U32* numZones);

    U32  calcDetailLevel(SceneState*, const Point3F&);
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state, RenderInst* ri);
    bool scopeObject(const Point3F& rootPosition,
        const F32             rootDistance,
        bool* zoneScopeState);

private:

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);


    enum UpdateMaskBits {
        InitMask = BIT(0),
        TransformMask = BIT(1),
        AlarmMask = BIT(2),

        // Reserved for light updates (8 bits for now)
        _lightupdate0 = BIT(3),
        _lightupdate1 = BIT(4),
        _lightupdate2 = BIT(5),
        _lightupdate3 = BIT(6),
        _lightupdate4 = BIT(7),
        _lightupdate5 = BIT(8),
        _lightupdate6 = BIT(9),
        _lightupdate7 = BIT(10),

        SkinBaseMask = BIT(11),
        AudioMask = BIT(12),
        NextFreeMask = BIT(13)
    };
    enum Constants {
        LightUpdateBitStart = 3,
        LightUpdateBitEnd = 10
    };

private:
    StringTableEntry                     mInteriorFileName;     ///< File name of the interior this instance encapuslates
    U32                                  mInteriorFileHash;     ///< Hash for interior file name, used for sorting
    Resource<InteriorResource>           mInteriorRes;          ///< Interior managed by resource manager
    Vector<MaterialList*>                mMaterialMaps;         ///< Materials for this interior
    StringTableEntry                     mSkinBase;             ///< Skin for this interior

    bool                                 mShowTerrainInside;    ///< Enables or disables terrain showing through the interior
    SFXProfile* mAudioProfile;         ///< Audio profile
    SFXEnvironment* mAudioEnvironment;     ///< Audio environment
    S32                                  mForcedDetailLevel;    ///< Forced LOD, if -1 auto LOD
    U32                                  mCRC;                  ///< CRC for the interior

    LM_HANDLE                            mLMHandle;             ///< Handle to the light manager


public:

    /// Returns the Light Manager handle
    LM_HANDLE getLMHandle() { return(mLMHandle); }

    /// Returns the audio profile
    SFXProfile* getAudioProfile() { return(mAudioProfile); }

    /// Returns the audio environment
    SFXEnvironment* getAudioEnvironment() { return(mAudioEnvironment); }

    /// This is used to determine just how 'inside' a point is in an interior.
    /// This is used by the environmental audio code for audio properties and the
    /// function always returns true.
    /// @param   pos   Point to test
    /// @param   pScale   How inside is the point 0 = totally outside, 1 = totally inside (out)
    bool getPointInsideScale(const Point3F& pos, F32* pScale);   // ~0: outside -> 1: inside

    /// Returns the interior resource
    Resource<InteriorResource>& getResource() { return(mInteriorRes); } // SceneLighting::InteriorProxy interface

    /// Returns the CRC for validation
    U32 getCRC() { return(mCRC); }

    // Alarm state information
private:
    enum AlarmState {
        Normal = 0,
        Alarm = 1
    };

    bool mAlarmState;                   ///< Alarm state of the interior

private:

    /// Creates a transform based on an trigger area
    /// @param   trigger   Trigger to create a transform for
    /// @param   transform Transform generated (out)
    void createTriggerTransform(const InteriorResTrigger* trigger, MatrixF* transform);

    /// Reflection related
public:
    Vector< ReflectPlane >  mReflectPlanes;
    virtual void updateReflection();


    virtual Material* getMaterial(U32 material);
};



#endif //_INTERIORBLOCK_H_

