//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIPLAYERVIEW_H_
#define _GUIPLAYERVIEW_H_

#ifndef _GUITSCONTROL_H_
#include "gui/core/guiTSControl.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif
#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _CAMERA_H_
#include "game/camera.h"
#endif

class TSThread;

class GuiPlayerView : public GuiTSCtrl
{
   private:
      typedef GuiTSCtrl Parent;

   enum
   {
      run = 0,
      hi,
      anim1,
      anim2,
      anim3,
      anim4,
      anim5
   };

   protected:
      enum MouseState
      {
         None,
         Rotating,
         Zooming
      };

      MouseState  mMouseState;

      TSShapeInstance*  mModel;
      TSShapeInstance*  mWeapon;
      U32   mSkinTag;

      Point3F  mCameraPos;
      MatrixF  mCameraMatrix;
      EulerF   mCameraRot;
      Point3F  mOrbitPos;
      F32      mMinOrbitDist;
      F32      mOrbitDist;
      S32      wNode;
      S32      pNode;

      TSThread *runThread;
      S32      lastRenderTime;
      S32      mAnimationSeq;

      Point2I  mLastMousePoint;

   public:
      DECLARE_CONOBJECT( GuiPlayerView );
      GuiPlayerView();
      ~GuiPlayerView();

      bool onWake();

      void onMouseEnter(const GuiEvent &event);
      void onMouseLeave(const GuiEvent &event);
      void onMouseDown( const GuiEvent &event );
      void onMouseUp( const GuiEvent &event );
      void onMouseDragged( const GuiEvent &event );
      void onRightMouseDown( const GuiEvent &event );
      void onRightMouseUp( const GuiEvent &event );
      void onRightMouseDragged( const GuiEvent &event );

      void setPlayerModel( const char* shape, const char* skin );
      void setPlayerSeq( S32 index );
      void getWeaponTransform( MatrixF *mat );

      bool processCameraQuery( CameraQuery *query );
      void renderWorld( const RectI &updateRect );
};

#endif // _GUI_PLAYERVIEW_H
