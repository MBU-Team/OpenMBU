//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORSUBOBJECT_H_
#define _INTERIORSUBOBJECT_H_

#ifndef _SCENESTATE_H_
#include "sceneGraph/sceneState.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

class InteriorInstance;

//class SubObjectRenderImage : public SceneRenderImage
//{
//  public:
//   U32 mDetailLevel;
//};

class InteriorSubObject : public SceneObject
{
   typedef SceneObject Parent;

  protected:
   InteriorInstance* mInteriorInstance;   // Should NOT be set by derived except in clone

  protected:
   enum SubObjectKeys {
      TranslucentSubObjectKey = 0,
      MirrorSubObjectKey      = 1
   };

   virtual U32  getSubObjectKey() const  = 0;
   virtual bool _readISO(Stream&);
   virtual bool _writeISO(Stream&) const;

   InteriorInstance* getInstance();
   const MatrixF&    getSOTransform() const;
   const Point3F&    getSOScale() const;

  public:
   InteriorSubObject();
   virtual ~InteriorSubObject();

   // Render control.  A sub-object should return false from renderDetailDependant if
   //  it exists only at the level-0 detail level, ie, doors, elevators, etc., true
   //  if should only render at the interiors detail, ie, translucencies.
   //virtual SubObjectRenderImage* getRenderImage(SceneState*, const Point3F& osPoint) = 0;
   virtual bool                  renderDetailDependant() const = 0;
   virtual U32                   getZone() const               = 0;

   virtual void                  noteTransformChange();
   virtual InteriorSubObject*    clone(InteriorInstance*) const = 0;

   static InteriorSubObject* readISO(Stream&);
   bool                      writeISO(Stream&) const;
};

#endif  // _H_INTERIORSUBOBJECT_
