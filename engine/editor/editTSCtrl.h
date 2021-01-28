//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EDITTSCTRL_H_
#define _EDITTSCTRL_H_

#ifndef _GUITSCONTROL_H_
#include "gui/core/guiTSControl.h"
#endif

struct Gui3DMouseEvent : public GuiEvent
{
   Point3F     vec;
   Point3F     pos;
};

class EditManager;

class EditTSCtrl : public GuiTSCtrl
{
   private:
      typedef GuiTSCtrl Parent;

      // EditTSCtrl
      void make3DMouseEvent(Gui3DMouseEvent & gui3Devent, const GuiEvent &event);
      void getCursor(GuiCursor *&cursor, bool &showCursor, const GuiEvent &lastGuiEvent);
      void onMouseUp(const GuiEvent & event);
      void onMouseDown(const GuiEvent & event);
      void onMouseMove(const GuiEvent & event);
      void onMouseDragged(const GuiEvent & event);
      void onMouseEnter(const GuiEvent & event);
      void onMouseLeave(const GuiEvent & event);
      void onRightMouseDown(const GuiEvent & event);
      void onRightMouseUp(const GuiEvent & event);
      void onRightMouseDragged(const GuiEvent & event);
      bool onInputEvent(const InputEvent & event);

      virtual void updateGuiInfo() {};
      virtual void renderScene(const RectI &){};
      void renderMissionArea();

      // GuiTSCtrl
      void renderWorld(const RectI & updateRect);

   protected:
      EditManager * mEditManager;
      Gui3DMouseEvent mLastEvent;

   public:

      EditTSCtrl();
      ~EditTSCtrl();

      // SimObject
      bool onAdd();

      //
      bool        mRenderMissionArea;
      ColorI      mMissionAreaFillColor;
      ColorI      mMissionAreaFrameColor;

      //
      ColorI            mConsoleFrameColor;
      ColorI            mConsoleFillColor;
      S32               mConsoleSphereLevel;
      S32               mConsoleCircleSegments;
      S32               mConsoleLineWidth;

      static void initPersistFields();
      static void consoleInit();

      //
      bool              mConsoleRendering;
      bool              mRightMousePassThru;

      // all editors will share a camera
      static Point3F    smCamPos;
      static EulerF     smCamRot;
      static MatrixF    smCamMatrix;
      static F32        smVisibleDistance;

      // GuiTSCtrl
      bool processCameraQuery(CameraQuery * query);

      // guiControl
      virtual void onRender(Point2I offset, const RectI &updateRect);
      virtual void on3DMouseUp(const Gui3DMouseEvent &){};
      virtual void on3DMouseDown(const Gui3DMouseEvent &){};
      virtual void on3DMouseMove(const Gui3DMouseEvent &){};
      virtual void on3DMouseDragged(const Gui3DMouseEvent &){};
      virtual void on3DMouseEnter(const Gui3DMouseEvent &){};
      virtual void on3DMouseLeave(const Gui3DMouseEvent &){};
      virtual void on3DRightMouseDown(const Gui3DMouseEvent &){};
      virtual void on3DRightMouseUp(const Gui3DMouseEvent &){};
      virtual void on3DRightMouseDragged(const Gui3DMouseEvent &){};
      virtual void get3DCursor(GuiCursor *&cursor, bool &visible, const Gui3DMouseEvent &);

      DECLARE_CONOBJECT(EditTSCtrl);
};

#endif
