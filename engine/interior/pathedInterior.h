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
#include "audio/audio.h"

class InteriorInstance;
class EditGeometry;
class EditInteriorResource;

struct PathedInteriorData : public GameBaseData {
    typedef GameBaseData Parent;
public:
    enum Sounds {
        StartSound,
        SustainSound,
        StopSound,
        MaxSounds
    };
    AudioProfile* sound[MaxSounds];
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
private:

    U32 getPathKey();                            // only used on the server

    // Persist fields
protected:
    StringTableEntry         mName;
    S32                      mPathIndex;
    Vector<StringTableEntry> mTriggers;
    Point3F                  mOffset;
    Box3F mExtrudedBox;
    bool mStopped;

    // Loaded resources and fields
protected:
    static PathedInterior* mClientPathedInteriors;

    AUDIOHANDLE mSustainHandle;

    StringTableEntry           mInteriorResName;
    S32                        mInteriorResIndex;
    Resource<InteriorResource> mInteriorRes;
    Interior* mInterior;
    Vector<ColorI>             mVertexColorsNormal;
    Vector<ColorI>             mVertexColorsAlarm;

    MatrixF                    mBaseTransform;
    Point3F                    mBaseScale;

    U32                        mPathKey;         // only used on the client
    F64                        mCurrentPosition;
    S32                        mTargetPosition;
    Point3F                    mCurrentVelocity;

    PathedInterior* mNextClientPI;

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

    PathedInterior* getNext() { return mNextClientPI; }

    static PathedInterior* getClientPathedInteriors() { return mClientPathedInteriors; }

    void processTick(const Move* move);
    void setStopped() { mStopped = true; }
    void resolvePathKey();

    bool onNewDataBlock(GameBaseData* dptr);
    bool  buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);
    bool            readPI(Stream&);
    bool            writePI(Stream&) const;
    PathedInterior* clone() const;

    DECLARE_CONOBJECT(PathedInterior);
    static void initPersistFields();
    void setPathPosition(S32 newPosition);
    void setTargetPosition(S32 targetPosition);
    void computeNextPathStep(U32 timeDelta);
    Box3F getExtrudedBox() { return mExtrudedBox; }
    Point3F getVelocity();
    void advance(F64 timeDelta);

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};

#endif // _H_PATHEDINTERIOR

