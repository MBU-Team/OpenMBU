//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUICANVAS_H_
#define _GUICANVAS_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif
#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

/// A canvas on which rendering occurs.
///
///
/// @section GuiCanvas_contents What a GUICanvas Can Contain...
///
/// @subsection GuiCanvas_content_contentcontrol Content Control
/// A content control is the top level GuiControl for a screen. This GuiControl
/// will be the parent control for all other GuiControls on that particular
/// screen.
///
/// @subsection GuiCanvas_content_dialogs Dialogs
///
/// A dialog is essentially another screen, only it gets overlaid on top of the
/// current content control, and all input goes to the dialog. This is most akin
/// to the "Open File" dialog box found in most operating systems. When you
/// choose to open a file, and the "Open File" dialog pops up, you can no longer
/// send input to the application, and must complete or cancel the open file
/// request. Torque keeps track of layers of dialogs. The dialog with the highest
/// layer is on top and will get all the input, unless the dialog is
/// modeless, which is a profile option.
///
/// @see GuiControlProfile
///
/// @section GuiCanvas_dirty Dirty Rectangles
///
/// The GuiCanvas is based on dirty regions.
///
/// Every frame the canvas paints only the areas of the canvas that are 'dirty'
/// or need updating. In most cases, this only is the area under the mouse cursor.
/// This is why if you look in guiCanvas.cc the call to glClear is commented out.
/// If you want a really good idea of what exactly dirty regions are and how they
/// work, un-comment that glClear line in the renderFrame method of guiCanvas.cc
///
/// What you will see is a black screen, except in the dirty regions, where the
/// screen will be painted normally. If you are making an animated GuiControl
/// you need to add your control to the dirty areas of the canvas.
///
class GuiCanvas : public GuiControl
{

private:
    typedef GuiControl Parent;
    typedef SimObject Grandparent;

    /// @name Rendering
    /// @{
    RectI      mOldUpdateRects[2];
    RectI      mCurUpdateRect;
    F32        rLastFrameTime;
    /// @}

    /// @name Cursor Properties
    /// @{

    F32         mPixelsPerMickey; ///< This is the scale factor which relates mouse movement in pixels
                                  ///  to units in the GUI.
    bool        cursorON;
    bool        mShowCursor;
    bool        mRenderFront;
    Point2F     cursorPt;
    Point2I     lastCursorPt;
    GuiCursor* defaultCursor;
    GuiCursor* lastCursor;
    bool        lastCursorON;
    /// @}

    /// @name Mouse Input
    /// @{

    SimObjectPtr<GuiControl>   mMouseCapturedControl;  ///< All mouse events will go to this ctrl only
    SimObjectPtr<GuiControl>   mMouseControl;          ///< the control the mouse was last seen in unless some other on captured it
    bool                       mMouseControlClicked;   ///< whether the current ctrl has been clicked - used by helpctrl
    U32                        mPrevMouseTime;         ///< this determines how long the mouse has been in the same control
    U32                        mNextMouseTime;         ///< used for onMouseRepeat()
    U32                        mInitialMouseDelay;     ///< also used for onMouseRepeat()
    bool                       mMouseButtonDown;       ///< Flag to determine if the button is depressed
    bool                       mMouseRightButtonDown;  ///< bool to determine if the right button is depressed
    GuiEvent                   mLastEvent;

    U8                         mLastMouseClickCount;
    S32                        mLastMouseDownTime;
    bool                       mLeftMouseLast;
    bool                       mRightMouseLast;
    bool                       mCurrentlyProcessingLeftMousePress;
    bool                       mCurrentlyProcessingRightMousePress;
    static bool                smForceMouse;

    void findMouseControl(const GuiEvent& event);
    void refreshMouseControl();
    /// @}

    /// @name Keyboard Input
    /// @{

    GuiControl* keyboardControl;                     ///<  All keyboard events will go to this ctrl first
    U32          nextKeyTime;


    /// Accelerator key map
    struct AccKeyMap
    {
        GuiControl* ctrl;
        U32 index;
        U32 keyCode;
        U32 modifier;
    };
    Vector <AccKeyMap> mAcceleratorMap;

    /// @}

public:
    DECLARE_CONOBJECT(GuiCanvas);
    static void consoleInit();
    static void initPersistFields();

    GuiCanvas();
    ~GuiCanvas();

    /// @name Rendering methods
    ///
    /// @{

    /// Repaints the dirty regions of the canvas
    /// @param   preRenderOnly   If set to true, only the onPreRender methods of all the GuiControls will be called
    void renderFrame(bool preRenderOnly);

    /// Updates various reflective materials in the scenegraph
    void updateReflections();

    /// Repaints the entire canvas by calling resetUpdateRegions() and then renderFrame()
    void paint();

    /// Adds a dirty area to the canvas so it will be updated on the next frame
    /// @param   pos   Screen-coordinates of the upper-left hand corner of the dirty area
    /// @param   ext   Width/height of the dirty area
    void addUpdateRegion(Point2I pos, Point2I ext);

    /// Resets the update regions so that the next call to renderFrame will
    /// repaint the whole canvas
    void resetUpdateRegions();

    /// This builds a rectangle which encompasses all of the dirty regions to be
    /// repainted
    /// @param   updateUnion   (out) Rectangle which surrounds all dirty areas
    void buildUpdateUnion(RectI* updateUnion);

    /// This will swap the buffers at the end of renderFrame. It was added for canvas
    /// sub-classes in case they wanted to do some custom code before the buffer
    /// flip occured.
    virtual void swapBuffers();

    /// @}

    /// @name Canvas Content Management
    /// @{

    /// This sets the content control to something different
    /// @param   gui   New content control
    void setContentControl(GuiControl* gui);

    /// Returns the content control
    GuiControl* getContentControl();

    /// Adds a dialog control onto the stack of dialogs
    /// @param   gui   Dialog to add
    /// @param   layer   Layer to put dialog on
    void pushDialogControl(GuiControl* gui, S32 layer = 0);

    /// Removes a specific layer of dialogs
    /// @param   layer   Layer to pop off from
    void popDialogControl(S32 layer = 0);

    /// Removes a specific dialog control
    /// @param   gui   Dialog to remove from the dialog stack
    void popDialogControl(GuiControl* gui);
    ///@}

    /// This turns on/off front-buffer rendering
    /// @param   front   True if all rendering should be done to the front buffer
    void setRenderFront(bool front) { mRenderFront = front; }

    /// @name Cursor commands
    /// A cursor can be on, but not be shown. If a cursor is not on, than it does not
    /// process input.
    /// @{

    /// Sets the cursor for the canvas.
    /// @param   cursor   New cursor to use.
    void setCursor(GuiCursor* cursor);

    /// Returns true if the cursor is on.
    bool isCursorON() { return cursorON; }

    /// Turns the cursor on or off.
    /// @param   onOff   True if the cursor should be on.
    void setCursorON(bool onOff);

    /// Sets the position of the cursor
    /// @param   pt   Point, in screenspace for the cursor
    void setCursorPos(const Point2I& pt) { cursorPt.x = F32(pt.x); cursorPt.y = F32(pt.y); }

    /// Returns the point, in screenspace, at which the cursor is located.
    Point2I getCursorPos() { return Point2I(S32(cursorPt.x), S32(cursorPt.y)); }

    /// Enable/disable rendering of the cursor.
    /// @param   state    True if we should render cursor
    void showCursor(bool state) { mShowCursor = state; }

    /// Returns true if the cursor is being rendered.
    bool isCursorShown() { return(mShowCursor); }
    /// @}

    /// @name Input Processing
    /// @{

    /// Processes an input event
    /// @see InputEvent
    /// @param   event   Input event to process
    bool processInputEvent(const InputEvent* event);

    /// Processes a mouse movement event
    /// @see MouseMoveEvent
    /// @param   event   Mouse move event to process
    void processMouseMoveEvent(const MouseMoveEvent* event);
    /// @}

    /// @name Mouse Methods
    /// @{

    /// When a control gets the mouse lock this means that that control gets
    /// ALL mouse input and no other control recieves any input.
    /// @param   lockingControl   Control to lock mouse to
    void mouseLock(GuiControl* lockingControl);

    /// Unlocks the mouse from a control
    /// @param   lockingControl   Control to unlock from
    void mouseUnlock(GuiControl* lockingControl);

    /// Returns the control which the mouse is over
    GuiControl* getMouseControl() { return mMouseControl; }

    /// Returns the control which the mouse is locked to if any
    GuiControl* getMouseLockedControl() { return mMouseCapturedControl; }

    /// Returns true if the left mouse button is down
    bool mouseButtonDown(void) { return mMouseButtonDown; }

    /// Returns true if the right mouse button is down
    bool mouseRightButtonDown(void) { return mMouseRightButtonDown; }

    void checkLockMouseMove(const GuiEvent& event);
    /// @}

    /// @name Mouse input methods
    /// These events process the events before passing them down to the
    /// controls they effect. This allows for things such as the input
    /// locking and such.
    ///
    /// Each of these methods corosponds to the action in it's method name
    /// and processes the GuiEvent passd as a parameter
    /// @{
    void rootMouseUp(const GuiEvent& event);
    void rootMouseDown(const GuiEvent& event);
    void rootMouseMove(const GuiEvent& event);
    void rootMouseDragged(const GuiEvent& event);

    void rootRightMouseDown(const GuiEvent& event);
    void rootRightMouseUp(const GuiEvent& event);
    void rootRightMouseDragged(const GuiEvent& event);

    void rootMouseWheelUp(const GuiEvent& event);
    void rootMouseWheelDown(const GuiEvent& event);
    /// @}

    /// @name Keyboard input methods
    /// First responders
    ///
    /// A first responder is a the GuiControl which responds first to input events
    /// before passing them off for further processing.
    /// @{

    /// Moves the first responder to the next tabable controle
    void tabNext(void);

    /// Moves the first responder to the previous tabable control
    void tabPrev(void);

    /// Setups a keyboard accelerator which maps to a GuiControl.
    ///
    /// @param   ctrl       GuiControl to map to.
    /// @param   index
    /// @param   keyCode    Key code.
    /// @param   modifier   Shift, ctrl, etc.
    void addAcceleratorKey(GuiControl* ctrl, U32 index, U32 keyCode, U32 modifier);

    /// Sets the first responder.
    /// @param   firstResponder    Control to designate as first responder
    void setFirstResponder(GuiControl* firstResponder);
    /// @}

    void setDefaultFirstResponder();

    void RefreshAndRepaint();

private:
    bool MapRightMouseToXbox(bool release);
};

extern GuiCanvas* Canvas;
extern bool gEnableDatablockCanvasRepaint;

#endif
