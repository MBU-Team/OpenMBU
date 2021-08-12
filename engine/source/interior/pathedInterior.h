//--------------------------------------------------------------------------
// PathedInterior.h: 
// 
// 
//--------------------------------------------------------------------------

#ifndef _H_PATHEDINTERIOR
#define _H_PATHEDINTERIOR


#include "interior/interior.h"
#include "game/gameBase.h"
#include "core/resManager.h"
#include "interior/interiorRes.h"

class PathManager;
class InteriorInstance;
class EditGeometry;
class EditInteriorResource;
class SFXProfile;
class SFXSource;

struct PathedInteriorData : public GameBaseData {
    typedef GameBaseData Parent;
public:
    enum Sounds {
        StartSound,
        SustainSound,
        StopSound,
        MaxSounds
    };
    SFXProfile* sound[MaxSounds];
    static void initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);
    bool preload(bool server, char errorBuffer[256]);
    PathedInteriorData();

    DECLARE_CONOBJECT(PathedInteriorData);
};

class PathedInterior : public GameBase
{
    typedef GameBase Parent;
    friend class InteriorInstance;
    friend class EditGeometry;
    friend class EditInteriorResource;

    PathedInteriorData* mDataBlock;

public:
    enum UpdateMasks {
        NewTargetMask = Parent::NextFreeMask,
        NewPositionMask = Parent::NextFreeMask << 1,
        NextFreeMask = Parent::NextFreeMask << 2,
    };

    enum
    {
        UpdateTicks = 32,
        UpdateTicksInc = 5,
    };

    struct TickState
    {
        F64 pathPosition;
        Box3F extrudedBox;
        Point3F velocity;
        Point3F worldPosition;
        S32 targetPos;
        F64 stopTime;
    };

private:

    U32 getPathKey();                            // only used on the server

    // Persist fields
protected:
    PathedInterior::TickState mSavedState;
    StringTableEntry         mName;
    S32                      mPathIndex;
    Vector<StringTableEntry> mTriggers;
    Point3F                  mOffset;
    Box3F mExtrudedBox;
    F64 mAdvanceTime;
    F64 mStopTime;
    U32 mNextNetUpdate;

    // Loaded resources and fields
protected:
    static PathedInterior* mClientPathedInteriors;
    static PathedInterior* mServerPathedInteriors;

    SFXSource* mSustainHandle;

    StringTableEntry           mInteriorResName;
    S32                        mInteriorResIndex;
    Resource<InteriorResource> mInteriorRes;
    Interior* mInterior;
    Vector<ColorI>             mVertexColorsNormal;
    Vector<ColorI>             mVertexColorsAlarm;

    U32 mLMHandle;

    MatrixF                    mBaseTransform;
    Point3F                    mBaseScale;

    U32                        mPathKey;         // only used on the client
    F64                        mCurrentPosition;
    S32                        mTargetPosition;
    Point3F                    mCurrentVelocity;

    PathedInterior* mNextPathedInterior;
    InteriorInstance* mDummyInst;

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state);
    void renderShadowVolumes(SceneState* state);

protected:
    bool onAdd();
    void onRemove();
    bool onSceneAdd(SceneGraph* graph);
    void onSceneRemove();

public:
    PathedInterior();
    ~PathedInterior();

    static PathedInterior* getPathedInteriors(NetObject* obj);

    PathedInterior* getNext() { return mNextPathedInterior; }

    PathManager* getPathManager() const;

    void pushTickState();
    void popTickState();
    void resetTickState(bool setT);

    void processTick(const Move* move);
    void interpolateTick(F32 delta);
    void setStopped();
    void resolvePathKey();

    bool onNewDataBlock(GameBaseData* dptr);
    bool  buildPolyListHelper(AbstractPolyList* list, const Box3F& wsBox, const SphereF& __formal, bool render);
    virtual bool  buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    virtual bool  buildRenderPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    bool            readPI(Stream&);
    bool            writePI(Stream&) const;
    PathedInterior* clone() const;

    DECLARE_CONOBJECT(PathedInterior);
    static void initPersistFields();
    void setPathPosition(S32 newPosition);
    void setTargetPosition(S32 targetPosition);
    void computeNextPathStep(F64 timeDelta);
    Box3F getExtrudedBox() { return mExtrudedBox; }
    Point3F getVelocity() const;
    void advance(F64 timeDelta);

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    void writePacketData(GameConnection* conn, BitStream* stream);
    void readPacketData(GameConnection* conn, BitStream* stream);

    void doSustainSound();

    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
};

#endif // _H_PATHEDINTERIOR

