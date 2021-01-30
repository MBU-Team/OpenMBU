//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WORLDEDITOR_H_
#define _WORLDEDITOR_H_

#ifndef _EDITTSCTRL_H_
#include "editor/editTSCtrl.h"
#endif
#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif
#ifndef _GTEXMANAGER_H_
//#include "dgl/gTexManager.h"
#endif

#include "gfx/gfxTextureHandle.h"

class Path;
class SceneObject;
class WorldEditor : public EditTSCtrl
{
    typedef EditTSCtrl Parent;

public:

    void ignoreObjClass(U32 argc, const char** argv);
    void clearIgnoreList();

    void undo();
    void redo();

    void clearSelection();
    void selectObject(const char* obj);
    void unselectObject(const char* obj);

    S32 getSelectionSize();
    S32 getSelectObject(S32 index);
    const char* getSelectionCentroid();

    void dropCurrentSelection();
    void deleteCurrentSelection();
    void copyCurrentSelection();
    bool canPasteSelection();

    const char* getMode();
    bool setMode(const char* mode);

    void addUndoState();
    void redirectConsole(S32 objID);

public:

    struct CollisionInfo
    {
        SceneObject* obj;
        Point3F           pos;
        VectorF           normal;
    };

    class Selection : public SimObject
    {
        typedef SimObject    Parent;

    private:

        Point3F        mCentroid;
        Point3F        mBoxCentroid;
        bool           mCentroidValid;
        SimObjectList  mObjectList;
        bool           mAutoSelect;

        void           updateCentroid();

    public:

        Selection();
        ~Selection();

        //
        U32 size() { return(mObjectList.size()); }
        SceneObject* operator[] (S32 index) { return((SceneObject*)mObjectList[index]); }

        bool objInSet(SceneObject*);

        bool addObject(SceneObject*);
        bool removeObject(SceneObject*);
        void clear();

        void onDeleteNotify(SimObject*);

        const Point3F& getCentroid();
        const Point3F& getBoxCentroid();

        void enableCollision();
        void disableCollision();

        //
        void autoSelect(bool b) { mAutoSelect = b; }
        void invalidateCentroid() { mCentroidValid = false; }

        //
        void offset(const Point3F&);
        void orient(const MatrixF&, const Point3F&);
        void rotate(const EulerF&, const Point3F&);
        void scale(const VectorF&);
    };

    //
    static SceneObject* getClientObj(SceneObject*);
    static void setClientObjInfo(SceneObject*, const MatrixF&, const VectorF&);
    static void updateClientTransforms(Selection&);

    // VERY basic undo functionality - only concerned with transform/scale/...
private:

    struct SelectionState
    {
        struct Entry
        {
            MatrixF     mMatrix;
            VectorF     mScale;

            // validation
            U32         mObjId;
            U32         mObjNumber;
        };

        Vector<Entry>  mEntries;

        SelectionState()
        {
            VECTOR_SET_ASSOCIATION(mEntries);
        }
    };

    SelectionState* createUndo(Selection&);
    void addUndo(Vector<SelectionState*>& list, SelectionState* sel);
    bool processUndo(Vector<SelectionState*>& src, Vector<SelectionState*>& dest);
    void clearUndo(Vector<SelectionState*>& list);

    Vector<SelectionState*>       mUndoList;
    Vector<SelectionState*>       mRedoList;

public:
    // someday get around to creating a growing memory stream...
    struct StreamedObject {
        U8 data[2048];
    };
    Vector<StreamedObject>              mStreamBufs;

    bool deleteSelection(Selection& sel);
    bool copySelection(Selection& sel);
    bool pasteSelection();
    void dropSelection(Selection& sel);

    // work off of mSelected
    void hideSelection(bool hide);
    void lockSelection(bool lock);

public:
    bool objClassIgnored(const SceneObject* obj);
    void renderObjectBox(SceneObject* obj, const ColorI& col);

private:
    SceneObject* getControlObject();
    bool collide(const Gui3DMouseEvent& event, CollisionInfo& info);

    // gfx methods
    //void renderObjectBox(SceneObject * obj, const ColorI & col);
    void renderObjectFace(SceneObject* obj, const VectorF& normal, const ColorI& col);
    void renderSelectionWorldBox(Selection& sel);

    void renderPlane(const Point3F& origin);
    void renderMousePopupInfo();
    void renderScreenObj(SceneObject* obj, Point2I sPos);

    void renderPaths(SimObject* obj);
    void renderSplinePath(Path* path);

    // axis gizmo methods...
    void calcAxisInfo();
    bool collideAxisGizmo(const Gui3DMouseEvent& event);
    void renderAxisGizmo();
    void renderAxisGizmoText();

    // axis gizmo state...
    Point3F     mAxisGizmoCenter;
    VectorF     mAxisGizmoVector[3];
    F32         mAxisGizmoProjLen;
    S32         mAxisGizmoSelAxis;
    bool        mUsingAxisGizmo;

    //
    Point3F snapPoint(const Point3F& pnt);
    bool                       mIsDirty;

    //
    bool                       mMouseDown;
    Selection                  mSelected;
    bool                       mUseVertMove;

    Selection                  mDragSelected;
    bool                       mDragSelect;
    RectI                      mDragRect;
    Point2I                    mDragStart;

    // modes for when dragging a selection
    enum {
        Move = 0,
        Rotate,
        Scale
    };

    //
    U32                        mCurrentMode;
    U32                        mDefaultMode;

    S32                        mRedirectID;

    CollisionInfo              mHitInfo;
    Point3F                    mHitOffset;
    SimObjectPtr<SceneObject>  mHitObject;
    Point2I                    mHitMousePos;
    Point3F                    mHitCentroid;
    EulerF                     mHitRotation;
    bool                       mMouseDragged;
    Gui3DMouseEvent            mLastMouseEvent;
    F32                        mLastRotation;

    //
    class ClassInfo
    {
    public:
        ~ClassInfo();

        struct Entry
        {
            StringTableEntry  mName;
            bool              mIgnoreCollision;
            GFXTexHandle      mDefaultHandle;
            GFXTexHandle      mSelectHandle;
            GFXTexHandle      mLockedHandle;
        };

        Vector<Entry*>       mEntries;
    };


    ClassInfo            mClassInfo;
    ClassInfo::Entry     mDefaultClassEntry;

    //bool objClassIgnored(const SceneObject * obj);
    ClassInfo::Entry* getClassEntry(StringTableEntry name);
    ClassInfo::Entry* getClassEntry(const SceneObject* obj);
    bool addClassEntry(ClassInfo::Entry* entry);

    // persist field data
public:

    enum {
        DropAtOrigin = 0,
        DropAtCamera,
        DropAtCameraWithRot,
        DropBelowCamera,
        DropAtScreenCenter,
        DropAtCentroid,
        DropToGround
    };

    bool              mPlanarMovement;
    S32               mUndoLimit;
    S32               mDropType;
    F32               mProjectDistance;
    bool              mBoundingBoxCollision;
    bool              mRenderPlane;
    bool              mRenderPlaneHashes;
    ColorI            mGridColor;
    F32               mPlaneDim;
    Point3F           mGridSize;
    bool              mRenderPopupBackground;
    ColorI            mPopupBackgroundColor;
    ColorI            mPopupTextColor;
    StringTableEntry  mSelectHandle;
    StringTableEntry  mDefaultHandle;
    StringTableEntry  mLockedHandle;
    ColorI            mObjectTextColor;
    bool               mObjectsUseBoxCenter;
    S32               mAxisGizmoMaxScreenLen;
    bool              mAxisGizmoActive;
    F32               mMouseMoveScale;
    F32               mMouseRotateScale;
    F32               mMouseScaleScale;
    F32               mMinScaleFactor;
    F32               mMaxScaleFactor;
    ColorI            mObjSelectColor;
    ColorI            mObjMouseOverSelectColor;
    ColorI            mObjMouseOverColor;
    bool              mShowMousePopupInfo;
    ColorI            mDragRectColor;
    bool              mRenderObjText;
    bool              mRenderObjHandle;
    StringTableEntry  mObjTextFormat;
    ColorI            mFaceSelectColor;
    bool              mRenderSelectionBox;
    ColorI            mSelectionBoxColor;
    bool              mSelectionLocked;
    bool              mSnapToGrid;
    bool              mSnapRotations;
    F32               mRotationSnap;
    bool              mToggleIgnoreList;
    bool              mRenderNav;
    bool              mNoMouseDrag;

private:
    // cursor constants
    enum {
        HandCursor = 0,
        RotateCursor,
        ScaleCursor,
        MoveCursor,
        ArrowCursor,
        DefaultCursor,

        //
        NumCursors
    };

    GuiCursor* mCursors[NumCursors];
    GuiCursor* mCurrentCursor;
    bool grabCursors();
    void setCursor(U32 cursor);
    void get3DCursor(GuiCursor*& cursor, bool& visible, const Gui3DMouseEvent& event);

public:

    WorldEditor();
    ~WorldEditor();

    // SimObject
    bool onAdd();
    void onEditorEnable();
    void setDirty() { mIsDirty = true; }

    // EditTSCtrl
    void on3DMouseMove(const Gui3DMouseEvent& event);
    void on3DMouseDown(const Gui3DMouseEvent& event);
    void on3DMouseUp(const Gui3DMouseEvent& event);
    void on3DMouseDragged(const Gui3DMouseEvent& event);
    void on3DMouseEnter(const Gui3DMouseEvent& event);
    void on3DMouseLeave(const Gui3DMouseEvent& event);
    void on3DRightMouseDown(const Gui3DMouseEvent& event);
    void on3DRightMouseUp(const Gui3DMouseEvent& event);

    void updateGuiInfo();

    //
    void renderScene(const RectI& updateRect);

    static void initPersistFields();

    DECLARE_CONOBJECT(WorldEditor);
};

#endif



