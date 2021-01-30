//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITERRPREVIEWCTRL_H_
#define _GUITERRPREVIEWCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif
#ifndef _GUITSCONTROL_H_
#include "gui/core/guiTSControl.h"
#endif

class GuiTerrPreviewCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;
    //   TextureHandle mTextureHandle;
    Point2F mRoot;
    Point2F mOrigin;
    Point2F mWorldScreenCenter;
    Point2F mCamera;
    F32     mTerrainSize;

    Point2F& wrap(const Point2F& p);
    Point2F& worldToTexture(const Point2F& p);
    Point2F& worldToCtrl(const Point2F& p);


public:
    //creation methods
    DECLARE_CONOBJECT(GuiTerrPreviewCtrl);
    GuiTerrPreviewCtrl();
    static void initPersistFields();

    //Parental methods
    bool onWake();
    void onSleep();

    //   void setBitmap(const GFXTexHandlehandle);

    void reset();
    void setRoot();
    void setRoot(const Point2F& root);
    void setOrigin(const Point2F& origin);
    const Point2F& getRoot() { return mRoot; }
    const Point2F& getOrigin() { return mOrigin; }

    //void setValue(const Point2F *center, const Point2F *camera);
    //const char *getScriptValue();
    void onPreRender();
    void onRender(Point2I offset, const RectI& updateRect);
};


#endif
