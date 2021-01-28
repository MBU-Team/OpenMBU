//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SCENEROOT_H_
#define _SCENEROOT_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

/// Root of scene graph.
class SceneRoot : public SceneObject
{
   typedef SceneObject Parent;

  protected:
   bool onSceneAdd(SceneGraph *graph);
   void onSceneRemove();

   bool getOverlappingZones(SceneObject *obj, U32 *zones, U32 *numZones);

   bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone,
                        const bool modifyBaseZoneState);

   bool scopeObject(const Point3F&        rootPosition,
                    const F32             rootDistance,
                    bool*                 zoneScopeState);

  public:
   SceneRoot();
   ~SceneRoot();
};

extern SceneRoot* gClientSceneRoot;     ///< Client's scene graph root.
extern SceneRoot* gServerSceneRoot;     ///< Server's scene graph root.

#endif //_SCENEROOT_H_
