//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiTSControl.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneLighting.h"
#include "sceneGraph/sceneGraph.h"
#include "gfx/debugDraw.h"

IMPLEMENT_CONOBJECT(GuiTSCtrl);

U32 GuiTSCtrl::smFrameCount = 0;

GuiTSCtrl::GuiTSCtrl()
{
   mCameraZRot = 0;
   mForceFOV = 0;

   mSaveModelview.identity();
   mSaveProjection.identity();
}
void GuiTSCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("cameraZRot", TypeF32, Offset(mCameraZRot, GuiTSCtrl));
   addField("forceFOV",   TypeF32, Offset(mForceFOV,   GuiTSCtrl));
}

void GuiTSCtrl::consoleInit()
{
   Con::addVariable("$TSControl::frameCount", TypeS32, &smFrameCount);
}

void GuiTSCtrl::onPreRender()
{
   setUpdate();
}

bool GuiTSCtrl::processCameraQuery(CameraQuery *)
{
   return false;
}

void GuiTSCtrl::renderWorld(const RectI& /*updateRect*/)
{
}

bool GuiTSCtrl::project(const Point3F &pt, Point3F *dest)
{

   GFX->project( *dest, (Point3F&) pt, mSaveModelview, mSaveProjection, mSaveViewport );

   if( dest->z < 0.0 || dest->z > 1.0 )
   {
      return false;
   }

   return true;
}

bool GuiTSCtrl::unproject(const Point3F &pt, Point3F *dest)
{
   GFX->unProject( *dest, (Point3F&) pt, mSaveModelview, mSaveProjection, mSaveViewport );

   return true;
}

void GuiTSCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   if(!processCameraQuery(&mLastCameraQuery))
      return;

   if(mForceFOV != 0)
      mLastCameraQuery.fov = mDegToRad(mForceFOV);

   if(mCameraZRot)
   {
      MatrixF rotMat(EulerF(0, 0, mDegToRad(mCameraZRot)));
      mLastCameraQuery.cameraMatrix.mul(rotMat);
   }

   // set up the camera and viewport stuff:
   F32 wwidth = mLastCameraQuery.nearPlane * mTan(mLastCameraQuery.fov / 2);
   F32 wheight = F32(mBounds.extent.y) / F32(mBounds.extent.x) * wwidth;

   F32 hscale = wwidth * 2 / F32(mBounds.extent.x);
   F32 vscale = wheight * 2 / F32(mBounds.extent.y);

   F32 left = (updateRect.point.x - offset.x) * hscale - wwidth;
   F32 right = (updateRect.point.x + updateRect.extent.x - offset.x) * hscale - wwidth;
   F32 top = wheight - vscale * (updateRect.point.y - offset.y);
   F32 bottom = wheight - vscale * (updateRect.point.y + updateRect.extent.y - offset.y);


   GFX->setViewport( updateRect );
   GFX->setFrustum( left, right, bottom, top,
                    mLastCameraQuery.nearPlane, mLastCameraQuery.farPlane );

   getCurrentClientSceneGraph()->setVisibleDistance( mLastCameraQuery.farPlane );

   mLastCameraQuery.cameraMatrix.inverse();
   GFX->setWorldMatrix( mLastCameraQuery.cameraMatrix );

   mSaveProjection = GFX->getProjectionMatrix();
   mSaveModelview = GFX->getWorldMatrix();

   mSaveViewport = updateRect;


   // prepare the DRL system...
   drlSystem.sgPrepSystem(updateRect.point, updateRect.extent);

   renderWorld(updateRect);

   if(gDebugDraw) gDebugDraw->render();


   // render DRL system...
   drlSystem.sgRenderSystem();


   // screen fx...
   //sgDRLTest.sgPrepChain(mBounds.extent.x, mBounds.extent.y);
   //sgDRLTest.sgRenderChain();
   //sgDRLTest.sgRenderDRL();

   GFX->disableShaders();

   renderChildControls(offset, updateRect);
   smFrameCount++;
}

void GuiTSCtrl::onRemove()
{
   Parent::onRemove();
}
