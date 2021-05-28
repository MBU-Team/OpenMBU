//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneRoot.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"

SceneRoot* gClientSceneRoot = NULL;
SceneRoot* gServerSceneRoot = NULL;
SceneRoot* gSPModeSceneRoot = NULL;

SceneRoot::SceneRoot()
{
    mObjBox.min.set(-1e10, -1e10, -1e10);
    mObjBox.max.set(1e10, 1e10, 1e10);
    resetWorldBox();
}

SceneRoot::~SceneRoot()
{

}

bool SceneRoot::onSceneAdd(SceneGraph* pGraph)
{
    // _Cannot_ call the parent here.  Must handle this ourselves so we can keep out of
    //  the zone graph...
 //   if (Parent::onSceneAdd(pGraph) == false)
 //      return false;

    mSceneManager = pGraph;
    mSceneManager->registerZones(this, 1);
    AssertFatal(mZoneRangeStart == 0, "error, sceneroot must be first scene object zone manager!");

    return true;
}

void SceneRoot::onSceneRemove()
{
    AssertFatal(mZoneRangeStart == 0, "error, sceneroot must be first scene object zone manager!");
    mSceneManager->unregisterZones(this);
    mZoneRangeStart = 0xFFFFFFFF;
    mSceneManager = NULL;

    // _Cannot_ call the parent here.  Must handle this ourselves so we can keep out of
    //  the zone graph...
 //   Parent::onSceneRemove();
}

bool SceneRoot::getOverlappingZones(SceneObject*, U32* zones, U32* numZones)
{
    // If we are here, we always return the global zone.
    zones[0] = 0;
    *numZones = 1;

    return false;
}

bool SceneRoot::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32,
    const bool modifyBaseZoneState)
{
    AssertFatal(modifyBaseZoneState == true, "error, should never be called unless in the upward traversal!");
    AssertFatal(isLastState(state, stateKey) == false, "Error, should have been colored black in order to prevent double calls!");
    setLastState(state, stateKey);

    // We don't return a render image, or any portals, but we do setup the zone 0
    //  rendering parameters.  We simply copy them from what is in the states baseZoneState
    //  structure, and mark the zone as rendered.
    dMemcpy(state->getZoneStateNC(0).frustum, state->getBaseZoneState().frustum, sizeof(state->getZoneStateNC(0).frustum));
    state->getZoneStateNC(0).viewport = state->getBaseZoneState().viewport;
    state->getZoneStateNC(0).render = true;

    return false;
}

bool SceneRoot::scopeObject(const Point3F&        /*rootPosition*/,
    const F32             /*rootDistance*/,
    bool* zoneScopeState)
{
    zoneScopeState[0] = true;
    return false;
}
