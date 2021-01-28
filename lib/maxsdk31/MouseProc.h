/**********************************************************************
 *<
    FILE: MouseProc.h

    DESCRIPTION:  Declares DataEntryMouseProc class

    CREATED BY: Scott Morrison

    HISTORY: created 7 December, 1998

 *> Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

class DataEntryMouseProc : public MouseCallBack {
public:
    CoreExport DataEntryMouseProc(Object* pObj, int cursor, HINSTANCE hInst);

    CoreExport DataEntryMouseProc();

    // Called when a point is selected
    CoreExport virtual BOOL OnPointSelected()      {return TRUE; }
    // Called on every mouse move event
    CoreExport virtual void OnMouseMove(Point3& p) {}
    // Tells the system when to allow drawing in mutiple viewports
    CoreExport virtual BOOL AllowAnyViewport()     { return TRUE; }
    // Called when backspace is pressed
    CoreExport virtual void RemoveLastPoint()      {}
    // Called when the creation is finished
    CoreExport virtual int OnMouseAbort()          { return CREATE_ABORT; }
    // Says whether the mouse proc should perform redraws
    // When used in a CreateMouseCallBack, this should return FALSE
    CoreExport virtual BOOL PerformRedraw()        { return TRUE; }

    // These methods need to be implemented to get the offset line drawn

    // Tells the object to draw offset lines
    CoreExport virtual void SetUseConstructionLine(BOOL useLine) = 0;
    // Sets the endpoints of the line (0 is start, 1 is end)
    CoreExport virtual void SetConstructionLine(int i, Point3 p) = 0;
    
    // The mouse callback function
    CoreExport int proc(HWND hwnd, int msg, int point, int flags, IPoint2 m );
    
    friend class DataEntryBackspaceUser;
    
    CoreExport void ClearCreationParams();
    
    CoreExport void SetParams(HINSTANCE hInst, Object* pObj, int cursor);
    
private:
    Point3 GetPoint(IPoint2 m, HWND hWnd, ViewExp* pVpt);
    void SetOffsetBase(IPoint2 m, HWND hWnd, ViewExp* pVpt);
    BOOL GetNodeTM(Matrix3& tm);
    IPoint2 ProjectPointToViewport(ViewExp *pVpt, Point3 fp);
    IPoint2 AngleSnapPoint(Point3 in3, ViewExp* pVpt);

    // The inverse of the transform on the node (or viewport transform,
    // for creation mouse procs)
    Matrix3        mTM;

    // Indicates when to ignore upclicks close to down clicks
    BOOL           mDoNotDouble;
    // The resource id of the cursor to use
    int            mCursor;
    // The instance of the dll using the mouse proc
    HINSTANCE      mInstance;

    // State for off-construction plane creation
    Point3         mSnappedPoint;
    Matrix3        mOriginalViewTM;

    int            mPreviousFlags;

protected:
    // The object using the mouse proc
    Object*        mpObject;

    // The number of points selected so far.
    int            mMouseClick;
    // The 3D coordinates of the points, in the local coordinate system
    Tab<Point3>    mPoints;
    // The 2D points the user selected in the viewport.
    Tab<IPoint2>   mClickPoints;
    // TRUE when in the mode where we lift off the construction plane
    BOOL           mLiftOffCP;
    // The last window we had an event in
    HWND           mHwnd;
    IPoint2        mLastMovePoint;
};
