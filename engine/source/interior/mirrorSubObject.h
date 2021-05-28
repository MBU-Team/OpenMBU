//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

#ifndef _MIRRORSUBOBJECT_H_
#define _MIRRORSUBOBJECT_H_

#ifndef _INTERIORSUBOBJECT_H_
#include "interior/interiorSubObject.h"
#endif

#include "gfx/gfxTextureHandle.h"

class TextureHandle;

class MirrorSubObject : public InteriorSubObject
{
   typedef InteriorSubObject Parent;

  public:
   U32   mDetailLevel;
   U32   mZone;

   F32     mAlphaLevel;
   Point3F mCentroid;

   U32   surfaceCount;
   U32   surfaceStart;

  private:
   bool           mInitialized;
   GFXTexHandle*  mWhite;
   MatrixF        mReflectionMatrix;

   bool  isInitialized() const { return mInitialized; }
   void  setupTransforms();


   // ISO overrides
  protected:
   U32  getSubObjectKey() const;
   bool _readISO(Stream&);
   bool _writeISO(Stream&) const;

   // Render control.  A sub-object should return false from renderDetailDependant if
   //  it exists only at the level-0 detail level, ie, doors, elevators, etc., true
   //  if should only render at the interiors detail, ie, translucencies.
   //SubObjectRenderImage* getRenderImage(SceneState*, const Point3F&);
   bool                  renderDetailDependant() const;
   U32                   getZone() const;
   void                  noteTransformChange();

   InteriorSubObject*  clone(InteriorInstance*) const;

   // Rendering
  protected:
   void renderObject(SceneState*, SceneRenderImage*);
   void transformModelview(const U32, const MatrixF&, MatrixF*);
   void transformPosition(const U32, Point3F&);
   bool computeNewFrustum(const U32      portalIndex,
                          const F64*     oldFrustum,
                          const F64      nearPlane,
                          const F64      farPlane,
                          const RectI&   oldViewport,
                          F64*           newFrustum,
                          RectI&         newViewport,
                          const bool     flippedMatrix);
   void openPortal(const U32   portalIndex,
                   SceneState* pCurrState,
                   SceneState* pParentState);
   void closePortal(const U32   portalIndex,
                    SceneState* pCurrState,
                    SceneState* pParentState);
   void getWSPortalPlane(const U32 portalIndex, PlaneF*);


  public:
   MirrorSubObject();
   ~MirrorSubObject();

   DECLARE_CONOBJECT(MirrorSubObject);
   static void initPersistFields();
};

#endif // _H_MIRRORSUBOBJECT

