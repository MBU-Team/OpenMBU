//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRAINEDITOR_H_
#define _TERRAINEDITOR_H_

#ifndef _EDITTSCTRL_H_
#include "editor/editTSCtrl.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif

//------------------------------------------------------------------------------

class GridInfo
{
public:
    Point2I                    mGridPos;
    TerrainBlock::Material     mMaterial;
    U8                         mMaterialAlpha[TerrainBlock::MaterialGroups];
    F32                        mHeight;
    U8                         mMaterialGroup;
    F32                        mWeight;
    F32                        mStartHeight;

    bool                       mPrimarySelect;
    bool                       mMaterialChanged;

    // hash table
    S32                        mNext;
    S32                        mPrev;
};

//------------------------------------------------------------------------------

class Selection : public Vector<GridInfo>
{
private:

    StringTableEntry     mName;
    BitSet32             mUndoFlags;

    // hash table
    S32 lookup(const Point2I& pos);
    void insert(GridInfo& info);
    U32 getHashIndex(const Point2I& pos);

    Vector<S32>          mHashLists;
    U32                  mHashListSize;

public:

    Selection();
    virtual ~Selection();

    void reset();
    bool add(GridInfo& info);
    bool getInfo(Point2I pos, GridInfo& info);
    bool setInfo(GridInfo& info);
    bool remove(const GridInfo& info);
    void setName(StringTableEntry name);
    StringTableEntry getName() { return(mName); }
    F32 getAvgHeight();
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class TerrainEditor;
class Brush : public Selection
{
protected:
    TerrainEditor* mTerrainEditor;
    Point2I           mSize;
    Point2I           mGridPos;

public:

    enum {
        MaxBrushDim = 40
    };

    Brush(TerrainEditor* editor);
    virtual ~Brush() {};

    //
    void setPosition(const Point3F& pos);
    void setPosition(const Point2I& pos);
    const Point2I& getPosition();

    void update();
    virtual void rebuild() = 0;

    Point2I getSize() { return(mSize); }
    virtual void setSize(const Point2I& size) { mSize = size; }
};

class BoxBrush : public Brush
{
public:
    BoxBrush(TerrainEditor* editor) : Brush(editor) {}
    void rebuild();
};

class EllipseBrush : public Brush
{
public:
    EllipseBrush(TerrainEditor* editor) : Brush(editor) {}
    void rebuild();
};

class SelectionBrush : public Brush
{
public:
    SelectionBrush(TerrainEditor* editor);
    void rebuild();
    void setSize(const Point2I&) {}
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct BaseMaterialInfo {
    StringTableEntry     mMaterialNames[TerrainBlock::MaterialGroups];
    U8                   mBaseMaterials[TerrainBlock::BlockSize * TerrainBlock::BlockSize];
};

class TerrainAction;
class TerrainEditor : public EditTSCtrl
{
    // XA: This methods where added to replace the friend consoleMethods.
public:
    void attachTerrain(TerrainBlock* terrBlock);

    void setBrushType(const char* type);
    void setBrushSize(S32 w, S32 h);
    const char* getBrushPos();
    void setBrushPos(Point2I pos);

    void setAction(const char* action);
    const char* getActionName(U32 index);
    const char* getCurrentAction();
    S32 getNumActions();
    void processAction(const char* sAction);

    void undo();
    void redo();

    void resetSelWeights(bool clear);
    void clearSelection();

    void buildMaterialMap();
    S32 getNumTextures();
    const char* getTextureName(S32 index);

    void markEmptySquares();
    void clearModifiedFlags();

    void mirrorTerrain(S32 mirrorIndex);

    void pushBaseMaterialInfo();
    void popBaseMaterialInfo();

    void setLoneBaseMaterial(const char* materialListBaseName);

private:
    typedef EditTSCtrl Parent;
    TerrainBlock* mTerrainBlock;
    Point2I  mGridUpdateMin;
    Point2I  mGridUpdateMax;
    U32 mMouseDownSeq;

    Point3F                    mMousePos;
    Brush* mMouseBrush;
    bool                       mRenderBrush;
    Point2I                    mBrushSize;
    Vector<TerrainAction*>    mActions;
    TerrainAction* mCurrentAction;
    bool                       mInAction;
    Selection                  mDefaultSel;
    bool                       mSelectionLocked;
    GuiCursor* mDefaultCursor;
    GuiCursor* mCurrentCursor;
    bool                       mCursorVisible;
    StringTableEntry           mPaintMaterial;

    Selection* mCurrentSel;

    //
    bool                       mRebuildEmpty;
    bool                       mRebuildTextures;
    void rebuild();

    void addUndo(Vector<Selection*>& list, Selection* sel);
    bool processUndo(Vector<Selection*>& src, Vector<Selection*>& dest);
    void clearUndo(Vector<Selection*>& list);

    U32                        mUndoLimit;
    Selection* mUndoSel;

    Vector<Selection*>         mUndoList;
    Vector<Selection*>         mRedoList;

    Vector<BaseMaterialInfo*>  mBaseMaterialInfos;
    bool mIsDirty; // dirty flag for writing terrain.
    bool mIsMissionDirty; // dirty flag for writing mission.
public:

    TerrainEditor();
    ~TerrainEditor();

    // conversion functions
    bool gridToWorld(const Point2I& gPos, Point3F& wPos);
    bool worldToGrid(const Point3F& wPos, Point2I& gPos);
    bool gridToCenter(const Point2I& gPos, Point2I& cPos);

    bool getGridInfo(const Point3F& wPos, GridInfo& info);
    bool getGridInfo(const Point2I& gPos, GridInfo& info);
    void setGridInfo(const GridInfo& info);
    void setGridInfoHeight(const GridInfo& info);
    void gridUpdateComplete();
    void materialUpdateComplete();
    void processActionTick(U32 sequence);

    bool collide(const Gui3DMouseEvent& event, Point3F& pos);
    void lockSelection(bool lock) { mSelectionLocked = lock; };

    Selection* getUndoSel() { return(mUndoSel); }
    Selection* getCurrentSel() { return(mCurrentSel); }
    void setCurrentSel(Selection* sel) { mCurrentSel = sel; }
    void resetCurrentSel() { mCurrentSel = &mDefaultSel; }

    S32 getPaintMaterial();

    Point2I getBrushSize() { return(mBrushSize); }
    TerrainBlock* getTerrainBlock() { AssertFatal(mTerrainBlock != NULL, "No terrain block"); return(mTerrainBlock); }
    TerrainBlock* getClientTerrain();
    bool terrainBlockValid() { return(mTerrainBlock ? true : false); }
    void setCursor(GuiCursor* cursor);
    void getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent);
    void setDirty() { mIsDirty = true; }
    void setMissionDirty() { mIsMissionDirty = true; }

    TerrainAction* lookupAction(const char* name);

private:


    // terrain interface functions
    F32 getGridHeight(const Point2I& gPos);
    void setGridHeight(const Point2I& gPos, const F32 height);

    TerrainBlock::Material getGridMaterial(const Point2I& gPos);
    void setGridMaterial(const Point2I& gPos, const TerrainBlock::Material& material);

    U8 getGridMaterialGroup(const Point2I& gPos);
    void setGridMaterialGroup(const Point2I& gPos, U8 group);

    //
    void updateBrush(Brush& brush, const Point2I& gPos);

    //
    Point3F getMousePos() { return(mMousePos); };

    //
    void renderSelection(const Selection& sel, const ColorF& inColorFull, const ColorF& inColorNone, const ColorF& outColorFull, const ColorF& outColorNone, bool renderFill, bool renderFrame);
    void renderBorder();

public:

    // persist field data - these are dynamic
    bool                 mRenderBorder;
    F32                  mBorderHeight;
    ColorI               mBorderFillColor;
    ColorI               mBorderFrameColor;
    bool                 mBorderLineMode;
    bool                 mSelectionHidden;
    bool                 mEnableSoftBrushes;
    bool                 mRenderVertexSelection;
    bool                 mProcessUsesBrush;

    //
    F32                  mAdjustHeightVal;
    F32                  mSetHeightVal;
    F32                  mScaleVal;
    F32                  mSmoothFactor;
    S32                  mMaterialGroup;
    F32                  mSoftSelectRadius;
    StringTableEntry     mSoftSelectFilter;
    StringTableEntry     mSoftSelectDefaultFilter;
    F32                  mAdjustHeightMouseScale;

public:

    // SimObject
    bool onAdd();
    void onDeleteNotify(SimObject* object);

    static void initPersistFields();

    // EditTSCtrl
    void on3DMouseUp(const Gui3DMouseEvent& event);
    void on3DMouseDown(const Gui3DMouseEvent& event);
    void on3DMouseMove(const Gui3DMouseEvent& event);
    void on3DMouseDragged(const Gui3DMouseEvent& event);
    void updateGuiInfo();
    void renderScene(const RectI& updateRect);

    DECLARE_CONOBJECT(TerrainEditor);
};

inline void TerrainEditor::setGridInfoHeight(const GridInfo& info)
{
    setGridHeight(info.mGridPos, info.mHeight);
}

#endif
