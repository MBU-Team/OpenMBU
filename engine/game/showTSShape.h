//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SHOWTSSHAPE_H_
#define _SHOWTSSHAPE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif

class ShowTSShape : public SceneObject
{
    typedef SceneObject Parent;
    typedef NetObject   GrandParent;

    enum { MaxThreads = 20 };

    TSShapeInstance* shapeInstance;
    char shapeNameBuffer[256];
    Vector<TSThread*> threads;
    Vector<bool> play;
    Vector<F32> scale;

    MatrixF mat;
    Point3F centerPos; // we want to be able to point here for orbit camera

    F32  timeScale;
    bool addGround;
    bool stayGrounded;

    // Rendering
protected:
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void prepBatchRender(SceneState* state);


public:
    ShowTSShape(const char* shapeName);
    ~ShowTSShape();
    bool onAdd();
    void onRemove();

    static ShowTSShape* currentShow;

    static Vector<char*> loadQueue;

    static void reset(); // if a new sequence is loaded, re-load all the shapes...
    void resetInstance();

    static void advanceTime(U32 delta);
    void advanceTimeInstance(U32 delta);

    static bool handleMovement(F32 leftSpeed, F32 rightSpeed,
        F32 forwardSpeed, F32 backwardSpeed,
        F32 upSpeed, F32 downSpeed,
        F32 delta);

    MatrixF getMatrix() { return mat; }
    void addGroundTransform(const MatrixF& ground);
    void setPosition(Point3F  p);
    void getFeetPosition(Point3F* p) { mat.getColumn(3, p); }
    void getCenterPosition(Point3F* p) { *p = centerPos; }

    void render();

    void orbitUs();

    //
    void newThread();
    void deleteThread(S32 th);

    S32 getThreadCount() { return threads.size(); }
    S32 getSequenceCount() { return shapeInstance->getShape()->sequences.size(); }
    const char* getSequenceName(S32 idx) { return shapeInstance->getShape()->getName(shapeInstance->getShape()->sequences[idx].nameIndex); }
    S32 getSequenceNum(S32 th) { return shapeInstance->getSequence(threads[th]); }

    void setSequence(S32 th, S32 seq) { shapeInstance->setSequence(threads[th], seq, shapeInstance->getPos(threads[th])); }
    void setSequence(S32 th, S32 seq, F32 pos) { shapeInstance->setSequence(threads[th], seq, pos); }
    void transitionToSequence(S32 th, S32 seq, F32 pos, F32 dur, bool play) { shapeInstance->transitionToSequence(threads[th], seq, pos, dur, play); }

    F32  getPos(S32 th) { return shapeInstance->getPos(threads[th]); }
    void setPos(S32 th, F32 pos) { if (th < threads.size()) shapeInstance->setPos(threads[th], pos); }

    void setPlay(S32 th, bool on) { if (th >= 0 && th < threads.size()) play[th] = on; }
    bool getPlay(S32 th) { return play[th]; }

    bool isInTransition(S32 th) { return shapeInstance->isInTransition(threads[th]); }
    bool isBlend(S32 seq) { return shapeInstance->getShape()->sequences[seq].isBlend(); }

    void setThreadScale(S32 th, F32 s) { if (th >= 0 && th < threads.size()) scale[th] = s; }
    F32  getThreadScale(S32 th) { return scale[th]; }

    S32 getNumDetails() { return shapeInstance->getShape()->details.size(); }
    S32 getCurrentDetail() { return shapeInstance->getCurrentDetail(); }
    void setCurrentDetail(S32 dl) { shapeInstance->setCurrentDetail(dl); }

    bool getAnimateRoot() { return addGround; }
    void setAnimateRoot(bool nv) { addGround = nv; }

    bool getStickToGround() { return stayGrounded; }
    void setStickToGround(bool nv) { stayGrounded = nv; }

    const char* getShapeName() { return shapeNameBuffer; }
    bool shapeLoaded() { return shapeInstance; }
    TSShapeInstance* getShapeInstance() { return shapeInstance; }
};

extern void ShowInit();

#endif
