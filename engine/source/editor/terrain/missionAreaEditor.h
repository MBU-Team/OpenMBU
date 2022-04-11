//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MISSIONAREAEDITOR_H_
#define _MISSIONAREAEDITOR_H_

#ifndef _GUIBITMAPCTRL_H_
#include "gui/controls/guiBitmapCtrl.h"
#endif
#ifndef _GUITYPES_H_
#include "gui/guiTypes.h"
#endif
#ifndef _MISSIONAREA_H_
#include "game/missionArea.h"
#endif

class GBitmap;
class TerrainBlock;

class MissionAreaEditor : public GuiBitmapCtrl
{
private:
    typedef GuiBitmapCtrl      Parent;

    SimObjectPtr<MissionArea>  mMissionArea;
    SimObjectPtr<TerrainBlock> mTerrainBlock;

    GBitmap* createTerrainBitmap();
    void onUpdate();

    void setControlObjPos(const Point2F& pos);
    bool clampArea(RectI& area);

    // --------------------------------------------------
    // conversion
    VectorF                    mScale;
    Point2F                    mCenterPos;

    Point2F worldToScreen(const Point2F&);
    Point2F screenToWorld(const Point2F&);

    void getScreenMissionArea(RectI& rect);
    void getScreenMissionArea(RectF& rect);

    void setupScreenTransform(const Point2I& offset);

    // --------------------------------------------------
    // mouse
    enum {
        DefaultCursor = 0,
        HandCursor,
        GrabCursor,
        VertResizeCursor,
        HorizResizeCursor,
        DiagRightResizeCursor,
        DiagLeftResizeCursor,
        NumCursors
    };

    bool grabCursors();
    GuiCursor* mCurrentCursor;
    GuiCursor* mCursors[NumCursors];
    void getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent);
    void setCursor(U32 cursor);

    S32                        mLastHitMode;
    Point2I                    mLastMousePoint;

    // --------------------------------------------------
    // mission area
    enum {
        NUT_SIZE = 3
    };

    enum {
        nothing = 0,
        sizingLeft = BIT(0),
        sizingRight = BIT(1),
        sizingTop = BIT(2),
        sizingBottom = BIT(3),
        moving = BIT(4)
    };

    void updateCursor(S32 hit);
    bool inNut(const Point2I& pt, S32 x, S32 y);
    S32 getSizingHitKnobs(const Point2I& pt, const RectI& box);
    void drawNut(const Point2I& nut);
    void drawNuts(RectI& box);

public:

    MissionAreaEditor();

    //
    bool missionAreaObjValid() { return(!mMissionArea.isNull()); }
    bool terrainObjValid() { return(!mTerrainBlock.isNull()); }

    TerrainBlock* getTerrainObj();

    const RectI& getArea();
    void setArea(const RectI& area);

    void updateTerrainBitmap();

    // GuiControl
    void parentResized(const Point2I& oldParentExtent, const Point2I& newParentExtent);
    void onRender(Point2I offset, const RectI& updateRect);
    bool onWake();
    void onSleep();

    void onMouseUp(const GuiEvent& event);
    void onMouseDown(const GuiEvent& event);
    void onMouseMove(const GuiEvent& event);
    void onMouseDragged(const GuiEvent& event);
    void onMouseEnter(const GuiEvent& event);
    void onMouseLeave(const GuiEvent& event);

    // SimObject
    bool onAdd();

    // field data..
    bool        mSquareBitmap;
    bool        mEnableEditing;
    bool        mRenderCamera;

    ColorI      mHandleFrameColor;
    ColorI      mHandleFillColor;
    ColorI      mDefaultObjectColor;
    ColorI      mWaterObjectColor;
    ColorI      mMissionBoundsColor;
    ColorI      mCameraColor;

    bool        mEnableMirroring;
    S32         mMirrorIndex;
    ColorI      mMirrorLineColor;
    ColorI      mMirrorArrowColor;

    static void initPersistFields();

    DECLARE_CONOBJECT(MissionAreaEditor);
};

#endif
