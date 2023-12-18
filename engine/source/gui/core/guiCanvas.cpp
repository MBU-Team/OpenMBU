//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "platform/profiler.h"
#include "platform/event.h"
#include "platform/platform.h"
#include "gfx/gfxDevice.h"
#include "gui/core/guiTypes.h"
#include "gui/controls/guiConsole.h"
#include "gui/core/guiCanvas.h"
//#include "gui/editor/guiMenuBar.h"
#include "console/consoleTypes.h"
#include "gfx/screenshot.h"
#include "sim/sceneObject.h"
#include "gfx/gammaBuffer.h"

bool gEnableDatablockCanvasRepaint = false;

extern bool FakeXboxButtonEvent(const InputEvent* event, GuiControl* ctrl);

bool GuiCanvas::smForceMouse = false;

// We formerly kept all the GUI related IMPLEMENT_CONOBJECT macros here.
// Now they belong with their implementations. -- BJG

IMPLEMENT_CONOBJECT(GuiCanvas);


GuiCanvas* Canvas = NULL;
U32 gLastEventTime = 0;

ConsoleMethod(GuiCanvas, getContent, S32, 2, 2, "Get the GuiControl which is being used as the content.")
{
    GuiControl* ctrl = object->getContentControl();
    if (ctrl)
        return ctrl->getId();
    return -1;
}

ConsoleMethod(GuiCanvas, setContent, void, 3, 3, "(GuiControl ctrl)"
    "Set the content of the canvas.")
{
    object;
    argc;

    GuiControl* gui = NULL;
    if (argv[2][0])
    {
        if (!Sim::findObject(argv[2], gui))
        {
            Con::printf("%s(): Invalid control: %s", argv[0], argv[2]);
            return;
        }
    }

    //set the new content control
    Canvas->setContentControl(gui);
}

ConsoleMethod(GuiCanvas, pushDialog, void, 3, 4, "(GuiControl ctrl, int layer)")
{
    object;

    GuiControl* gui;

    if (!Sim::findObject(argv[2], gui))
    {
        Con::printf("%s(): Invalid control: %s", argv[0], argv[2]);
        return;
    }

    //find the layer
    S32 layer = 0;
    if (argc == 4)
        layer = dAtoi(argv[3]);

    //set the new content control
    Canvas->pushDialogControl(gui, layer);
}

ConsoleMethod(GuiCanvas, popDialog, void, 2, 3, "(GuiControl ctrl=NULL)")
{
    object;

    GuiControl* gui;
    if (argc == 3)
    {
        if (!Sim::findObject(argv[2], gui))
        {
            Con::printf("%s(): Invalid control: %s", argv[0], argv[2]);
            return;
        }
    }

    if (gui)
        Canvas->popDialogControl(gui);
    else
        Canvas->popDialogControl();
}

ConsoleMethod(GuiCanvas, popLayer, void, 2, 3, "(int layer)")
{
    object;

    S32 layer = 0;
    if (argc == 3)
        layer = dAtoi(argv[2]);

    Canvas->popDialogControl(layer);
}

ConsoleMethod(GuiCanvas, cursorOn, void, 2, 2, "")
{
    object;
    argc;
    argv;
    Canvas->setCursorON(true);
}

ConsoleMethod(GuiCanvas, cursorOff, void, 2, 2, "")
{
    object;
    argc;
    argv;
    Canvas->setCursorON(false);
}

ConsoleMethod(GuiCanvas, setCursor, void, 3, 3, "(bool visible)")
{
    object;
    argc;

    GuiCursor* curs = NULL;
    if (argv[2][0])
    {
        if (!Sim::findObject(argv[2], curs))
        {
            Con::printf("%s is not a valid cursor.", argv[2]);
            return;
        }
    }
    Canvas->setCursor(curs);
}

ConsoleMethod(GuiCanvas, renderFront, void, 3, 3, "(bool enable)")
{
    Canvas->setRenderFront(dAtob(argv[2]));
}

ConsoleMethod(GuiCanvas, showCursor, void, 2, 2, "")
{
    Canvas->showCursor(true);
}

ConsoleMethod(GuiCanvas, hideCursor, void, 2, 2, "")
{
    Canvas->showCursor(false);
}

ConsoleMethod(GuiCanvas, isCursorOn, bool, 2, 2, "")
{
    return Canvas->isCursorON();
}

ConsoleMethod(GuiCanvas, repaint, void, 2, 2, "Force canvas to redraw.")
{
    Canvas->paint();
}

ConsoleMethod(GuiCanvas, reset, void, 2, 2, "Reset the update regions for the canvas.")
{
    Canvas->resetUpdateRegions();
}

ConsoleMethod(GuiCanvas, getCursorPos, const char*, 2, 2, "Get the current position of the cursor.")
{
    Point2I pos = Canvas->getCursorPos();
    char* ret = Con::getReturnBuffer(32);
    dSprintf(ret, 32, "%d %d", pos.x, pos.y);
    return(ret);
}

ConsoleMethod(GuiCanvas, setCursorPos, void, 3, 4, "(Point2I pos)")
{
    Point2I pos(0, 0);
    if (argc == 4)
        pos.set(dAtoi(argv[2]), dAtoi(argv[3]));
    else
        dSscanf(argv[3], "%d %d", &pos.x, &pos.y);
    Canvas->setCursorPos(pos);
}

ConsoleMethod(GuiCanvas, setDefaultFirstResponder, void, 2, 2, "()")
{
    Canvas->setDefaultFirstResponder();
}

ConsoleMethod(GuiCanvas, getLastDeviceInst, S32, 2, 2, "()")
{
    // On Xbox Live this would return the device that sent the last input
    //return object->mLastEvent.deviceInst;

    return 0;
}

void GuiCanvas::consoleInit()
{
    Con::addVariable("$Canvas::forceMouse", TypeBool, &smForceMouse);
}

void GuiCanvas::initPersistFields()
{
    Con::addVariable("LastInputEventTime", TypeS32, &gLastEventTime);
    Con::addVariable("EnableDatablockCanvasRepaint", TypeBool, &gEnableDatablockCanvasRepaint);
}

GuiCanvas::GuiCanvas()
{
    mBounds.set(0, 0, 640, 480);
    mAwake = true;
    mPixelsPerMickey = 1.0f;
    lastCursorON = false;
    cursorON = true;
    mShowCursor = true;
    rLastFrameTime = 0.0f;

    mMouseCapturedControl = NULL;
    mMouseControl = NULL;
    mMouseControlClicked = false;
    mMouseButtonDown = false;
    mMouseRightButtonDown = false;

    lastCursor = NULL;
    lastCursorPt.set(0, 0);
    cursorPt.set(0, 0);

    mLastMouseClickCount = 0;
    mLastMouseDownTime = 0;
    mPrevMouseTime = 0;
    defaultCursor = NULL;

    mCurrentlyProcessingLeftMousePress = false;
    mCurrentlyProcessingRightMousePress = false;

    mRenderFront = false;
}

GuiCanvas::~GuiCanvas()
{
    if (Canvas == this)
        Canvas = 0;
}

//------------------------------------------------------------------------------
void GuiCanvas::setCursor(GuiCursor* curs)
{
    defaultCursor = curs;
}

void GuiCanvas::setCursorON(bool onOff)
{
    cursorON = onOff;
    if (!cursorON)
        mMouseControl = NULL;
}

void GuiCanvas::addAcceleratorKey(GuiControl* ctrl, U32 index, U32 keyCode, U32 modifier)
{
    if (keyCode > 0 && ctrl)
    {
        AccKeyMap newMap;
        newMap.ctrl = ctrl;
        newMap.index = index;
        newMap.keyCode = keyCode;
        newMap.modifier = modifier;
        mAcceleratorMap.push_back(newMap);
    }
}

void GuiCanvas::tabNext(void)
{
    GuiControl* ctrl = static_cast<GuiControl*>(last());
    if (ctrl)
    {
        //save the old
        GuiControl* oldResponder = mFirstResponder;

        GuiControl* newResponder = ctrl->findNextTabable(mFirstResponder);
        if (!newResponder)
            newResponder = ctrl->findFirstTabable();

        if (newResponder && newResponder != oldResponder)
        {
            newResponder->setFirstResponder();

            if (oldResponder)
                oldResponder->onLoseFirstResponder();
        }
    }
}

void GuiCanvas::tabPrev(void)
{
    GuiControl* ctrl = static_cast<GuiControl*>(last());
    if (ctrl)
    {
        //save the old
        GuiControl* oldResponder = mFirstResponder;

        GuiControl* newResponder = ctrl->findPrevTabable(mFirstResponder);
        if (!newResponder)
            newResponder = ctrl->findLastTabable();

        if (newResponder && newResponder != oldResponder)
        {
            newResponder->setFirstResponder();

            if (oldResponder)
                oldResponder->onLoseFirstResponder();
        }
    }
}

void GuiCanvas::processMouseMoveEvent(const MouseMoveEvent* event)
{
    InputEvent iEvent;
    iEvent.deviceType = MouseDeviceType;
    iEvent.objType = SI_XAXIS;
    iEvent.fValue = event->xPos - cursorPt.x;
    iEvent.modifier = event->modifier;
    processInputEvent(&iEvent);
    iEvent.objType = SI_YAXIS;
    iEvent.fValue = event->yPos - cursorPt.y;
    processInputEvent(&iEvent);
}

bool GuiCanvas::processInputEvent(const InputEvent* event)
{
    // First call the general input handler (on the extremely off-chance that it will be handled):
    if (mFirstResponder)
    {
        if (mFirstResponder->onInputEvent(*event))
            return(true);
    }

    GuiControl* responder = mFirstResponder;
    if (!responder)
    {
        if (objectList.size() <= 0)
            return false;
        responder = (GuiControl*)objectList[objectList.size() - 1];
        if (!responder)
            return false;
    }

    if (responder->mEventCtrl)
        responder = (GuiControl*)responder->mEventCtrl;

    if (event->deviceType == KeyboardDeviceType || event->deviceType == MouseDeviceType)
        gLastEventTime = Sim::getCurrentTime();

    if (event->deviceType == KeyboardDeviceType)
    {
        mLastEvent.ascii = event->ascii;
        mLastEvent.modifier = event->modifier;
        mLastEvent.keyCode = event->objInst;

        U32 eventModifier = event->modifier;
        if (eventModifier & SI_SHIFT)
            eventModifier |= SI_SHIFT;
        if (eventModifier & SI_CTRL)
            eventModifier |= SI_CTRL;
        if (eventModifier & SI_ALT)
            eventModifier |= SI_ALT;

        if (event->action == SI_MAKE)
        {
            //see if we should tab next/prev

            //see if we should now pass the event to the first responder
            if (responder)
            {
                if (FakeXboxButtonEvent(event, responder) || responder->onKeyDown(mLastEvent))
                    return true;
            }

            if (isCursorON() && (event->objInst == KEY_TAB))
            {
                if (size() > 0)
                {
                    if (event->modifier & SI_SHIFT)
                    {
                        tabPrev();
                        return true;
                    }
                    else if (event->modifier == 0)
                    {
                        tabNext();
                        return true;
                    }
                }
            }

            //if not handled, search for an accelerator
            for (U32 i = 0; i < mAcceleratorMap.size(); i++)
            {
                if ((U32)mAcceleratorMap[i].keyCode == (U32)event->objInst && (U32)mAcceleratorMap[i].modifier == eventModifier)
                {
                    mAcceleratorMap[i].ctrl->acceleratorKeyPress(mAcceleratorMap[i].index);
                    return true;
                }
            }
        }
        else if (event->action == SI_BREAK)
        {
            if (responder)
                if (FakeXboxButtonEvent(event, responder) || responder->onKeyUp(mLastEvent))
                    return true;

            //see if there's an accelerator
            for (U32 i = 0; i < mAcceleratorMap.size(); i++)
            {
                if ((U32)mAcceleratorMap[i].keyCode == (U32)event->objInst && (U32)mAcceleratorMap[i].modifier == eventModifier)
                {
                    mAcceleratorMap[i].ctrl->acceleratorKeyRelease(mAcceleratorMap[i].index);
                    return true;
                }
            }
        }
        else if (event->action == SI_REPEAT)
        {
            if (responder)
                responder->onKeyRepeat(mLastEvent);
            return true;
        }
    }
    else if (event->deviceType == MouseDeviceType && (cursorON || ((mCurrentlyProcessingLeftMousePress || mCurrentlyProcessingRightMousePress) && !smForceMouse)))
    {
        //copy the modifier into the new event
        mLastEvent.modifier = event->modifier;

        if (event->objType == SI_XAXIS || event->objType == SI_YAXIS)
        {
            bool moved = false;
            Point2I oldpt((S32)cursorPt.x, (S32)cursorPt.y);
            Point2F pt(cursorPt.x, cursorPt.y);

            if (event->objType == SI_XAXIS)
            {
                pt.x += (event->fValue * mPixelsPerMickey);
                cursorPt.x = getMax(0, getMin((S32)pt.x, mBounds.extent.x - 1));
                if (oldpt.x != S32(cursorPt.x))
                    moved = true;
            }
            else
            {
                pt.y += (event->fValue * mPixelsPerMickey);
                cursorPt.y = getMax(0, getMin((S32)pt.y, mBounds.extent.y - 1));
                if (oldpt.y != S32(cursorPt.y))
                    moved = true;
            }
            if (moved)
            {
                mLastEvent.mousePoint.x = S32(cursorPt.x);
                mLastEvent.mousePoint.y = S32(cursorPt.y);

                if (mMouseButtonDown)
                    rootMouseDragged(mLastEvent);
                else if (mMouseRightButtonDown)
                    rootRightMouseDragged(mLastEvent);
                else
                    rootMouseMove(mLastEvent);
            }
            return true;
        }
        else if (event->objType == SI_ZAXIS)
        {
            mLastEvent.mousePoint.x = S32(cursorPt.x);
            mLastEvent.mousePoint.y = S32(cursorPt.y);

            if (event->fValue < 0.0f)
                rootMouseWheelDown(mLastEvent);
            else
                rootMouseWheelUp(mLastEvent);
        }
        else if (event->objType == SI_BUTTON)
        {
            //copy the cursor point into the event
            mLastEvent.mousePoint.x = S32(cursorPt.x);
            mLastEvent.mousePoint.y = S32(cursorPt.y);

            if (event->objInst == KEY_BUTTON0) // left button
            {
                //see if button was pressed
                if (event->action == SI_MAKE)
                {
                    U32 curTime = Platform::getVirtualMilliseconds();
                    mNextMouseTime = curTime + mInitialMouseDelay;

                    //if the last button pressed was the left...
                    if (mLeftMouseLast)
                    {
                        //if it was within the double click time count the clicks
                        if (curTime - mLastMouseDownTime <= 500)
                            mLastMouseClickCount++;
                        else
                            mLastMouseClickCount = 1;
                    }
                    else
                    {
                        mLeftMouseLast = true;
                        mLastMouseClickCount = 1;
                    }

                    mLastMouseDownTime = curTime;
                    mLastEvent.mouseClickCount = mLastMouseClickCount;

                    rootMouseDown(mLastEvent);

                    mCurrentlyProcessingLeftMousePress = true;
                }
                //else button was released
                else
                {
                    mNextMouseTime = 0xFFFFFFFF;
                    rootMouseUp(mLastEvent);

                    mCurrentlyProcessingLeftMousePress = false;
                }
                return true;
            }
            else if (event->objInst == KEY_BUTTON1) // right button
            {
                if (event->action == SI_MAKE)
                {
                    U32 curTime = Platform::getVirtualMilliseconds();

                    //if the last button pressed was the right...
                    if (!mRightMouseLast)
                    {
                        //if it was within the double click time count the clicks
                        if (curTime - mLastMouseDownTime <= 50)
                            mLastMouseClickCount++;
                        else
                            mLastMouseClickCount = 1;
                    }
                    else
                    {
                        mRightMouseLast = false;
                        mLastMouseClickCount = 1;
                    }

                    mLastMouseDownTime = curTime;
                    mLastEvent.mouseClickCount = mLastMouseClickCount;

                    rootRightMouseDown(mLastEvent);

                    mCurrentlyProcessingRightMousePress = true;
                }
                else // it was a mouse up
                {
                    rootRightMouseUp(mLastEvent);

                    mCurrentlyProcessingRightMousePress = false;
                }
                return true;
            }
        }
    }
    else if (event->deviceType == XInputDeviceType)
    {
        // Do NOT PUT THE gLastEventTime change here, it will pick up the analog stick noise

        //copy the modifier into the new event
        mLastEvent.modifier = event->modifier;
        
        bool retval = false;

        if (event->objType == SI_BUTTON || event->objType == SI_POV)
        {
            if (event->action == SI_MAKE)
                retval = responder->onGamepadButtonPressed(event->objInst);
            else if (event->action == SI_BREAK)
                retval = responder->onGamepadButtonReleased(event->objInst);

            gLastEventTime = Sim::getCurrentTime();
            
            return retval;
        }
        else if ((event->objType == XI_THUMBLY || event->objType == XI_THUMBLX || 
            event->objType == XI_LEFT_TRIGGER || event->objType == XI_RIGHT_TRIGGER) &&
            event->action == SI_MOVE && event->deviceInst < 4)
        {
            F32 incomingValue = mFabs(event->fValue);
            F32 deadZone = 0.75f;//0.05f;
            F32 minClickTime = 500.f;
            F32 maxClickTime = 1000.f;
            static F32 xDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static F32 yDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static U32 xLastClickTime[] = { 0, 0, 0, 0 };
            static U32 yLastClickTime[] = { 0, 0, 0, 0 };

            // Each trigger also needs decay/click logic
            static F32 lDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static F32 rDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static U32 lLastClickTime[] = { 0, 0, 0, 0 };
            static U32 rLastClickTime[] = { 0, 0, 0, 0 };

            U32 curTime = Platform::getRealMilliseconds();
            F32* decay;
            U32* lastClickTime;

            // Determine the state for the respective analog value
            if (event->objType == XI_THUMBLX)
            {
                decay = &xDecay[event->deviceInst];
                lastClickTime = &xLastClickTime[event->deviceInst];
            }
            else if (event->objType == XI_THUMBLY)
            {
                decay = &yDecay[event->deviceInst];
                lastClickTime = &yLastClickTime[event->deviceInst];
            } 
            else if (event->objType == XI_LEFT_TRIGGER) 
            {
                decay = &lDecay[event->deviceInst];
                lastClickTime = &lLastClickTime[event->deviceInst];
            } 
            else if (event->objType == XI_RIGHT_TRIGGER) 
            {
                decay = &rDecay[event->deviceInst];
                lastClickTime = &rLastClickTime[event->deviceInst];
            }

            //bool down = event->fValue < 0.f;
            bool down = event->fValue > 0.f; // This is totally counter-intuitive, and it doesn't make sense
                                             // However, it was done to line up with the PC directX

            if (incomingValue < deadZone)
            {
                *decay = 1.0f;
                *lastClickTime = 0;
                return false;
            }

            // Rescales input between 0.0 and 1.0
            incomingValue = (incomingValue - deadZone) * (1.f / (1.f - deadZone));

            F32 clickTime = minClickTime + (maxClickTime - minClickTime) * (1.f - incomingValue);
            clickTime *= *decay;

            if (clickTime < curTime - *lastClickTime)
            {
                *decay *= 0.9;
                if (*decay < 0.2)
                    *decay = 0.2;

                *lastClickTime = curTime;

                if (event->objType == XI_THUMBLY)
                {
                    if (down)
                        retval = responder->onGamepadButtonPressed(XI_DPAD_DOWN);
                    else
                        retval = responder->onGamepadButtonPressed(XI_DPAD_UP);
                }
                else if (event->objType == XI_THUMBLX)
                {
                    if (down)
                        retval = responder->onGamepadButtonPressed(XI_DPAD_RIGHT);
                    else
                        retval = responder->onGamepadButtonPressed(XI_DPAD_LEFT);
                }
                else if (event->objType == XI_LEFT_TRIGGER)
                {
                    // Neither triggers care about the actual analog value, only if depressed
                    retval = responder->onGamepadButtonPressed(XI_LEFT_TRIGGER);  
                }
                else if (event->objType == XI_RIGHT_TRIGGER)
                {
                    retval = responder->onGamepadButtonPressed(XI_RIGHT_TRIGGER);  
                }
                gLastEventTime = curTime;
            }

            return retval;
        }
        else
        {
            //Con::warnf("Input fell outside current logic. Event type: %d", event->objType);
        }
    }

    return false;
}

bool FakeXboxButtonEvent(const InputEvent* event, GuiControl* ctrl)
{
    if (!event || !ctrl)
        return false;

    const char* retval = "";

    U16 key = event->objInst;
    U8 mod = event->modifier;

    if (event->action == SI_MAKE)
    {
        if (key == KEY_A || key == KEY_RETURN)
            return ctrl->onGamepadButtonPressed(XI_A);
        else if ((key == KEY_B || key == KEY_BACKSPACE) && !mod)
            return ctrl->onGamepadButtonPressed(XI_B);
        else if (key == KEY_X)
            return ctrl->onGamepadButtonPressed(XI_X);
        else if (key == KEY_Y)
            return ctrl->onGamepadButtonPressed(XI_Y);
        else if (key == KEY_S && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonPressed(XI_START);
        else if (key == KEY_B && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonPressed(XI_BACK);
        else if (key == KEY_L && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonPressed(XI_LEFT_SHOULDER);
        else if (key == KEY_R && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonPressed(XI_RIGHT_SHOULDER);
        else if (key == KEY_L && (mod & SI_CTRL) != 0)
            return ctrl->onGamepadButtonPressed(XI_LEFT_THUMB);
        else if (key == KEY_R && (mod & SI_CTRL) != 0)
            return ctrl->onGamepadButtonPressed(XI_RIGHT_THUMB);
        else if (key == KEY_L)
            return ctrl->onGamepadButtonPressed(XI_LEFT_TRIGGER);
        else if (key == KEY_R)
            return ctrl->onGamepadButtonPressed(XI_RIGHT_TRIGGER);
        else if (key == KEY_UP)
            return ctrl->onGamepadButtonPressed(XI_DPAD_UP);
        else if (key == KEY_LEFT)
            return ctrl->onGamepadButtonPressed(XI_DPAD_LEFT);
        else if (key == KEY_DOWN)
            return ctrl->onGamepadButtonPressed(XI_DPAD_DOWN);
        else if (key == KEY_RIGHT)
            return ctrl->onGamepadButtonPressed(XI_DPAD_RIGHT);
        else
            return false;
    }
    else
    {
        if (key == KEY_A || key == KEY_RETURN)
            return ctrl->onGamepadButtonReleased(XI_A);
        else if ((key == KEY_B || key == KEY_BACKSPACE) && !mod)
            return ctrl->onGamepadButtonReleased(XI_B);
        else if (key == KEY_X)
            return ctrl->onGamepadButtonReleased(XI_X);
        else if (key == KEY_Y)
            return ctrl->onGamepadButtonReleased(XI_Y);
        else if (key == KEY_S && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonReleased(XI_START);
        else if (key == KEY_B && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonReleased(XI_BACK);
        else if (key == KEY_L && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonReleased(XI_LEFT_SHOULDER);
        else if (key == KEY_R && (mod & SI_SHIFT) != 0)
            return ctrl->onGamepadButtonReleased(XI_RIGHT_SHOULDER);
        else if (key == KEY_L && (mod & SI_CTRL) != 0)
            return ctrl->onGamepadButtonReleased(XI_LEFT_THUMB);
        else if (key == KEY_R && (mod & SI_CTRL) != 0)
            return ctrl->onGamepadButtonReleased(XI_RIGHT_THUMB);
        else if (key == KEY_L)
            return ctrl->onGamepadButtonReleased(XI_LEFT_TRIGGER);
        else if (key == KEY_R)
            return ctrl->onGamepadButtonReleased(XI_RIGHT_TRIGGER);
        else if (key == KEY_UP)
            return ctrl->onGamepadButtonReleased(XI_DPAD_UP);
        else if (key == KEY_LEFT)
            return ctrl->onGamepadButtonReleased(XI_DPAD_LEFT);
        else if (key == KEY_DOWN)
            return ctrl->onGamepadButtonReleased(XI_DPAD_DOWN);
        else if (key == KEY_RIGHT)
            return ctrl->onGamepadButtonReleased(XI_DPAD_RIGHT);
        else
            return false;
    }

    return false;
}

bool GuiCanvas::MapRightMouseToXbox(bool release)
{
    GuiControl* responder = mFirstResponder;
    if (!responder)
    {
        if (objectList.size() <= 0)
            return false;
        responder = (GuiControl*)objectList[objectList.size() - 1];
        if (!responder)
            return false;
    }

    if (responder->mEventCtrl)
        responder = (GuiControl*)responder->mEventCtrl;

    if (responder)
    {
        if (release && responder->onGamepadButtonReleased(XI_B))
            return true;
        else if (!release && responder->onGamepadButtonPressed(XI_B))
            return true;
    }

    return false;
}

void GuiCanvas::rootMouseDown(const GuiEvent& event)
{
    mPrevMouseTime = Platform::getVirtualMilliseconds();
    mMouseButtonDown = true;

    //pass the event to the mouse locked control
    if (bool(mMouseCapturedControl) && mMouseCapturedControl->isCurrentUIMode())
        mMouseCapturedControl->onMouseDown(event);

    //else pass it to whoever is underneath the cursor
    else
    {
        iterator i;
        i = end();
        while (i != begin())
        {
            i--;
            GuiControl* ctrl = static_cast<GuiControl*>(*i);
            GuiControl* controlHit = ctrl->findHitControl(event.mousePoint);

            //Con::printf("GuiCanvas::rootMouseDown: ctrl = %s, controlHit = %s", ctrl->getName(), controlHit ? controlHit->getName() : "NULL");

            //see if the controlHit is a modeless dialog...
            if ((!controlHit->mActive) && (!controlHit->mProfile->mModal))
                continue;
            else
            {
                controlHit->onMouseDown(event);
                break;
            }
        }
    }
    if (bool(mMouseControl))
        mMouseControlClicked = true;
}

void GuiCanvas::findMouseControl(const GuiEvent& event)
{
    if (size() == 0)
    {
        mMouseControl = NULL;
        return;
    }
    GuiControl* controlHit = findHitControl(event.mousePoint);
    if (controlHit != static_cast<GuiControl*>(mMouseControl))
    {
        if (bool(mMouseControl))
            mMouseControl->onMouseLeave(event);
        mMouseControl = controlHit;
        mMouseControl->onMouseEnter(event);
    }
}

void GuiCanvas::refreshMouseControl()
{
    GuiEvent evt;
    evt.mousePoint.x = S32(cursorPt.x);
    evt.mousePoint.y = S32(cursorPt.y);
    findMouseControl(evt);
}

void GuiCanvas::rootMouseUp(const GuiEvent& event)
{
    mPrevMouseTime = Platform::getVirtualMilliseconds();
    mMouseButtonDown = false;

    //pass the event to the mouse locked control
    if (bool(mMouseCapturedControl) && mMouseCapturedControl->isCurrentUIMode())
        mMouseCapturedControl->onMouseUp(event);
    else
    {
        findMouseControl(event);

        //Con::printf("GuiCanvas::rootMouseUp: mMouseControl = %s", mMouseControl ? mMouseControl->getName() : "NULL");

        if (bool(mMouseControl) && mMouseControl->isCurrentUIMode())
            mMouseControl->onMouseUp(event);
    }
}

void GuiCanvas::checkLockMouseMove(const GuiEvent& event)
{
    GuiControl* controlHit = findHitControl(event.mousePoint);
    if (controlHit != mMouseControl)
    {
        if (mMouseControl == mMouseCapturedControl)
            mMouseCapturedControl->onMouseLeave(event);
        else if (controlHit == mMouseCapturedControl)
            mMouseCapturedControl->onMouseEnter(event);
        mMouseControl = controlHit;
    }
}

void GuiCanvas::rootMouseDragged(const GuiEvent& event)
{
    //pass the event to the mouse locked control
    if (bool(mMouseCapturedControl))
    {
        checkLockMouseMove(event);
        mMouseCapturedControl->onMouseDragged(event);
    }
    else
    {
        findMouseControl(event);
        if (bool(mMouseControl))
            mMouseControl->onMouseDragged(event);
    }
}

void GuiCanvas::rootMouseMove(const GuiEvent& event)
{
    if (bool(mMouseCapturedControl))
    {
        checkLockMouseMove(event);
        mMouseCapturedControl->onMouseMove(event);
    }
    else
    {
        findMouseControl(event);
        if (bool(mMouseControl))
            mMouseControl->onMouseMove(event);
    }
}

void GuiCanvas::rootRightMouseDown(const GuiEvent& event)
{
    mPrevMouseTime = Platform::getVirtualMilliseconds();
    mMouseRightButtonDown = true;

    if (MapRightMouseToXbox(false))
    {
        // Don't return because it breaks the editor!
        //return;
    }

    if (bool(mMouseCapturedControl))
        mMouseCapturedControl->onRightMouseDown(event);
    else
    {
        findMouseControl(event);

        if (bool(mMouseControl))
        {
            mMouseControl->onRightMouseDown(event);
        }
    }
}

void GuiCanvas::rootRightMouseUp(const GuiEvent& event)
{
    mPrevMouseTime = Platform::getVirtualMilliseconds();
    mMouseRightButtonDown = false;

    if (MapRightMouseToXbox(true))
    {
        // Don't return because it breaks the editor!
        //return;
    }

    if (bool(mMouseCapturedControl))
        mMouseCapturedControl->onRightMouseUp(event);
    else
    {
        findMouseControl(event);

        if (bool(mMouseControl))
            mMouseControl->onRightMouseUp(event);
    }
}

void GuiCanvas::rootRightMouseDragged(const GuiEvent& event)
{
    mPrevMouseTime = Platform::getVirtualMilliseconds();

    if (bool(mMouseCapturedControl))
    {
        checkLockMouseMove(event);
        mMouseCapturedControl->onRightMouseDragged(event);
    }
    else
    {
        findMouseControl(event);

        if (bool(mMouseControl))
            mMouseControl->onRightMouseDragged(event);
    }
}

void GuiCanvas::rootMouseWheelUp(const GuiEvent& event)
{
    if (bool(mMouseCapturedControl))
        mMouseCapturedControl->onMouseWheelUp(event);
    else
    {
        findMouseControl(event);

        if (bool(mMouseControl))
            mMouseControl->onMouseWheelUp(event);
    }
}

void GuiCanvas::rootMouseWheelDown(const GuiEvent& event)
{
    if (bool(mMouseCapturedControl))
        mMouseCapturedControl->onMouseWheelDown(event);
    else
    {
        findMouseControl(event);

        if (bool(mMouseControl))
            mMouseControl->onMouseWheelDown(event);
    }
}

void GuiCanvas::setContentControl(GuiControl* gui)
{
    if (!gui)
        return;

    bool lastDataBlockRepaint = gEnableDatablockCanvasRepaint;
    gEnableDatablockCanvasRepaint = false;

    //remove all dialogs on layer 0
    U32 index = 0;
    while (size() > index)
    {
        GuiControl* ctrl = static_cast<GuiControl*>((*this)[index]);
        if (ctrl == gui || ctrl->mLayer != 0)
            index++;

        removeObject(ctrl);
        Sim::getGuiGroup()->addObject(ctrl);
    }

    // lose the first responder from the old GUI
    GuiControl* oldResponder = mFirstResponder;
    mFirstResponder = gui->findFirstTabable();
    if (oldResponder && oldResponder != mFirstResponder)
        oldResponder->onLoseFirstResponder();

    //add the gui to the front
    if (!size() || gui != (*this)[0])
    {
        // automatically wakes objects in GuiControl::onWake
        addObject(gui);
        if (size() >= 2)
            reOrder(gui, *begin());
    }
    //refresh the entire gui
    resetUpdateRegions();

    //rebuild the accelerator map
    mAcceleratorMap.clear();

    for (iterator i = end(); i != begin(); )
    {
        i--;
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        ctrl->buildAcceleratorMap();

        if (ctrl->mProfile->mModal)
            break;
    }

    refreshMouseControl();

    gEnableDatablockCanvasRepaint = lastDataBlockRepaint;
}

GuiControl* GuiCanvas::getContentControl()
{
    if (size() > 0)
        return (GuiControl*)first();
    return NULL;
}

void GuiCanvas::pushDialogControl(GuiControl* gui, S32 layer)
{
    //add the gui
    gui->mLayer = layer;

    // GuiControl::addObject wakes the object
    bool wakedGui = !gui->isAwake();
    addObject(gui);

    //reorder it to the correct layer
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        if (ctrl->mLayer > gui->mLayer)
        {
            reOrder(gui, ctrl);
            break;
        }
    }

    //call the dialog push method
    gui->onDialogPush();

    //find the top most dialog
    GuiControl* topCtrl = static_cast<GuiControl*>(last());

    //save the old
    GuiControl* oldResponder = mFirstResponder;

    //find the first responder
    mFirstResponder = gui->findFirstTabable();

    if (oldResponder && oldResponder != mFirstResponder)
        oldResponder->onLoseFirstResponder();

    // call the 'onWake' method?
    //if(wakedGui)
    //   Con::executef(gui, 1, "onWake");

    //refresh the entire gui
    resetUpdateRegions();

    //rebuild the accelerator map
    mAcceleratorMap.clear();
    if (size() > 0)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(last());
        ctrl->buildAcceleratorMap();
    }
    refreshMouseControl();
}

void GuiCanvas::popDialogControl(GuiControl* gui)
{
    if (size() < 1)
        return;

    //first, find the dialog, and call the "onDialogPop()" method
    GuiControl* ctrl = NULL;
    if (gui)
    {
        //make sure the gui really exists on the stack
        iterator i;
        bool found = false;
        for (i = begin(); i != end(); i++)
        {
            GuiControl* check = static_cast<GuiControl*>(*i);
            if (check == gui)
            {
                ctrl = check;
                found = true;
            }
        }
        if (!found)
            return;
    }
    else
        ctrl = static_cast<GuiControl*>(last());

    //call the "on pop" function
    ctrl->onDialogPop();

    // sleep the object
    bool didSleep = ctrl->isAwake();

    //now pop the last child (will sleep if awake)
    removeObject(ctrl);

    // Save the old responder:
    GuiControl* oldResponder = mFirstResponder;

    Sim::getGuiGroup()->addObject(ctrl);

    if (size() > 0)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(last());
        mFirstResponder = ctrl->mFirstResponder;
    }
    else
    {
        mFirstResponder = NULL;
    }

    if (oldResponder && oldResponder != mFirstResponder)
        oldResponder->onLoseFirstResponder();

    //refresh the entire gui
    resetUpdateRegions();

    //rebuild the accelerator map
    mAcceleratorMap.clear();
    if (size() > 0)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(last());
        ctrl->buildAcceleratorMap();
    }
    refreshMouseControl();
}

void GuiCanvas::popDialogControl(S32 layer)
{
    if (size() < 1)
        return;

    GuiControl* ctrl = NULL;
    iterator i = end(); // find in z order (last to first)
    while (i != begin())
    {
        i--;
        ctrl = static_cast<GuiControl*>(*i);
        if (ctrl->mLayer == layer)
            break;
    }
    if (ctrl)
        popDialogControl(ctrl);
}

void GuiCanvas::mouseLock(GuiControl* lockingControl)
{
    if (bool(mMouseCapturedControl))
        return;
    mMouseCapturedControl = lockingControl;
    if (mMouseControl && mMouseControl != mMouseCapturedControl)
    {
        GuiEvent evt;
        evt.mousePoint.x = S32(cursorPt.x);
        evt.mousePoint.y = S32(cursorPt.y);

        mMouseControl->onMouseLeave(evt);
    }
}

void GuiCanvas::mouseUnlock(GuiControl* lockingControl)
{
    if (static_cast<GuiControl*>(mMouseCapturedControl) != lockingControl)
        return;

    GuiEvent evt;
    evt.mousePoint.x = S32(cursorPt.x);
    evt.mousePoint.y = S32(cursorPt.y);

    GuiControl* controlHit = findHitControl(evt.mousePoint);
    if (controlHit != mMouseCapturedControl)
    {
        mMouseControl = controlHit;
        mMouseControlClicked = false;
        if (bool(mMouseControl))
            mMouseControl->onMouseEnter(evt);
    }
    mMouseCapturedControl = NULL;
}

void GuiCanvas::paint()
{
    resetUpdateRegions();

    // inhibit explicit refreshes in the case we're swapped out
    if (GFX->allowRender())
        renderFrame(false);
}

//ColorI gCanvasClearColor(255, 0, 255); // TSE ///< For GFX->clear
ColorI gCanvasClearColor(42, 139, 190); // MBU Blue ///< For GFX->clear
//ColorI gCanvasClearColor(0, 0, 0); // Black ///< For GFX->clear

void GuiCanvas::renderFrame(bool preRenderOnly)
{
    bool disableSPMode = false;
    if (gRenderPreview && !gSPMode)
        disableSPMode = true;

    if (gRenderPreview)
        gSPMode = true;

    PROFILE_START(CanvasPreRender);

    // Set our window as the current render target so we can see outputs.
    GFX->setActiveRenderTarget(Platform::getWindowGFXTarget());

    if (!GFX->getActiveRenderTarget())
    {
        if (disableSPMode)
            gSPMode = false;
        PROFILE_END();
        return;
    }

#ifdef TORQUE_GFX_STATE_DEBUG
    GFX->getDebugStateManager()->startFrame();
#endif

    GFXTarget* renderTarget = GFX->getActiveRenderTarget();
    if (renderTarget == NULL)
    {
        if (disableSPMode)
            gSPMode = false;
        PROFILE_END();
        return;
    }

    //We always want to render to the back buffer now
    //if(mRenderFront)
    //   glDrawBuffer(GL_FRONT);
    //else
    //   glDrawBuffer(GL_BACK);

    //Point2I size = renderTarget->getSize();//GFX->getVideoMode().resolution;//Platform::getWindowSize();
    //Point2I size_old = renderTarget->getSize();//GFX->getVideoMode().resolution;//Platform::getWindowSize();
    Point2I size = GFX->getVideoMode().resolution;//Platform::getWindowSize();

    //Con::errorf("Size (%d,%d) - (%d,%d)", size_old.x, size_old.y, size.x, size.y);

    if (size.x == 0 || size.y == 0)
    {
        if (disableSPMode)
            gSPMode = false;
        PROFILE_END();
        return;
    }

    RectI screenRect(0, 0, size.x, size.y);
    mBounds = screenRect;

    //all bottom level controls should be the same dimensions as the canvas
    //this is necessary for passing mouse events accurately
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        AssertFatal(static_cast<GuiControl*>((*i))->isAwake(), "GuiCanvas::renderFrame: ctrl is not awake");
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        Point2I ext = ctrl->getExtent();
        Point2I pos = ctrl->getPosition();

        if (pos != screenRect.point || ext != screenRect.extent)
        {
            ctrl->resize(screenRect.point, screenRect.extent);
            resetUpdateRegions();
        }
    }

    //preRender (recursive) all controls
    preRender();
    PROFILE_END();
    if (preRenderOnly)
    {
        if (disableSPMode)
            gSPMode = false;
        return;
    }


    // for now, just always reset the update regions - this is a
    // fix for FSAA on ATI cards
    resetUpdateRegions();


    //draw the mouse, but not using tags...
    PROFILE_START(CanvasRenderControls);

    GuiCursor* mouseCursor = NULL;
    bool cursorVisible = true;

    if (bool(mMouseCapturedControl))
        mMouseCapturedControl->getCursor(mouseCursor, cursorVisible, mLastEvent);
    else if (bool(mMouseControl))
        mMouseControl->getCursor(mouseCursor, cursorVisible, mLastEvent);

    Point2I cursorPos((S32)cursorPt.x, (S32)cursorPt.y);
    if (!mouseCursor)
        mouseCursor = defaultCursor;

    if (lastCursorON && lastCursor)
    {
        Point2I spot = lastCursor->getHotSpot();
        Point2I cext = lastCursor->getExtent();
        Point2I pos = lastCursorPt - spot;
        addUpdateRegion(pos - Point2I(2, 2), Point2I(cext.x + 4, cext.y + 4));
    }
    if (cursorVisible && mouseCursor)
    {
        Point2I spot = mouseCursor->getHotSpot();
        Point2I cext = mouseCursor->getExtent();
        Point2I pos = cursorPos - spot;

        addUpdateRegion(pos - Point2I(2, 2), Point2I(cext.x + 4, cext.y + 4));
    }

    lastCursorON = cursorVisible;
    lastCursor = mouseCursor;
    lastCursorPt = cursorPos;

    // Begin GFX
    PROFILE_START(GFXBeginScene);

    GFX->setActiveDevice(0);
    GFX->beginScene();

    // ---------------------------------------
    // Gamma Buffer
    // ---------------------------------------

    GammaBuffer* gammaBuff = static_cast<GammaBuffer*>(Sim::findObject("GammaBufferData"));

    static bool initGamma = false;
    if (!initGamma && gammaBuff)
    {
        initGamma = true;
        gammaBuff->init();
    }

    RectI vp = GFX->getViewport();

    if (gammaBuff)
    {
        GFX->pushActiveRenderTarget();
        gammaBuff->setAsRenderTarget();
    }

    // ---------------------------------------

    // do this at beginning of frame
    updateReflections();

    // TO DISABLE DIRTY RECTS...

    GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, gCanvasClearColor, 1.0f, 0);
    resetUpdateRegions();

    PROFILE_END();
    RectI updateUnion;
    buildUpdateUnion(&updateUnion);
    if (updateUnion.intersect(screenRect))
    {
        //fill in with black first
        //glClearColor(0, 0, 0, 0);
        //glClear(GL_COLOR_BUFFER_BIT);

        //render the dialogs
        iterator i;
        for (i = begin(); i != end(); i++)
        {
            GuiControl* contentCtrl = static_cast<GuiControl*>(*i);
            GFX->setClipRect(updateUnion);
            GFX->setCullMode(GFXCullNone);
            contentCtrl->onRender(contentCtrl->getPosition(), updateUnion);
        }

        GFX->setClipRect(updateUnion);

        // Fill Black if no Dialogs
        if(this->size() == 0)
            GFX->clear( GFXClearTarget, ColorI(0,0,0,0), 1.0f, 0 );

        // TODO: Tooltip stuff here in TGEA 1.7.1

        GFX->setClipRect(updateUnion);

        //temp draw the mouse
        /*if (cursorON && mShowCursor && !mouseCursor)
        {
            GFX->drawRectFill(RectI(cursorPt.x, cursorPt.y, cursorPt.x + 2, cursorPt.y + 2), ColorI(255, 0, 0));
        }*/


        // CodeReview - Make sure our bitmap modulation is clear or else there's a black modulation
        // that ruins rendering of textures at startup.. This was done in mouseCursor
        // onRender and so at startup when it wasn't called the modulation was black, ruining
        // the loading screen display. This fixes the issue, but is it only masking a deeper issue
        // in GFX with regard to gui rendering? [5/3/2007 justind]
        GFX->clearBitmapModulation();

        //DEBUG
        //draw the help ctrl
        //if (helpCtrl)
        //{
        //   helpCtrl->render(srf);
        //}

        if (cursorON && mouseCursor && mShowCursor)
        {
            Point2I pos((S32)cursorPt.x, (S32)cursorPt.y);
            Point2I spot = mouseCursor->getHotSpot();

            pos -= spot;
            mouseCursor->render(pos);
        }
    }

    // ---------------------------------------
    // Gamma Buffer
    // ---------------------------------------
    if (gammaBuff)
    {
        GFX->popActiveRenderTarget();
        gammaBuff->copyToScreen(vp);
    }
    // ---------------------------------------

    // mPending is set when the console function "screenShot()" is called
    // this situation is necessary because it needs to take the screenshot
    // before the buffers swap
    if (gScreenShot != NULL && gScreenShot->mPending)
    {
        gScreenShot->captureStandard();
    }

    PROFILE_END();

    PROFILE_START(GFXEndScene);
    GFX->endScene();
    PROFILE_END();

    swapBuffers();

#ifdef TORQUE_GFX_STATE_DEBUG
    GFX->getDebugStateManager()->endFrame();
#endif

    if (disableSPMode)
        gSPMode = false;
}

void GuiCanvas::swapBuffers()
{
    //AssertISV(mPlatformWindow, "GuiCanvas::swapBuffers - no window present!");
    //if(!mPlatformWindow->isVisible())
    //    return;

    PROFILE_START(SwapBuffers);
    //mPlatformWindow->getGFXTarget()->present();
    GFXTarget* target = Platform::getWindowGFXTarget();
    if (target)
        Platform::getWindowGFXTarget()->present();
    PROFILE_END();
}

void GuiCanvas::buildUpdateUnion(RectI* updateUnion)
{
    *updateUnion = mOldUpdateRects[0];

    //the update region should encompass the oldUpdateRects, and the curUpdateRect
    Point2I upperL;
    Point2I lowerR;

    upperL.x = getMin(mOldUpdateRects[0].point.x, mOldUpdateRects[1].point.x);
    upperL.x = getMin(upperL.x, mCurUpdateRect.point.x);

    upperL.y = getMin(mOldUpdateRects[0].point.y, mOldUpdateRects[1].point.y);
    upperL.y = getMin(upperL.y, mCurUpdateRect.point.y);

    lowerR.x = getMax(mOldUpdateRects[0].point.x + mOldUpdateRects[0].extent.x, mOldUpdateRects[1].point.x + mOldUpdateRects[1].extent.x);
    lowerR.x = getMax(lowerR.x, mCurUpdateRect.point.x + mCurUpdateRect.extent.x);

    lowerR.y = getMax(mOldUpdateRects[0].point.y + mOldUpdateRects[0].extent.y, mOldUpdateRects[1].point.y + mOldUpdateRects[1].extent.y);
    lowerR.y = getMax(lowerR.y, mCurUpdateRect.point.y + mCurUpdateRect.extent.y);

    updateUnion->point = upperL;
    updateUnion->extent = lowerR - upperL;

    //shift the oldUpdateRects
    mOldUpdateRects[0] = mOldUpdateRects[1];
    mOldUpdateRects[1] = mCurUpdateRect;

    mCurUpdateRect.point.set(0, 0);
    mCurUpdateRect.extent.set(0, 0);
}

void GuiCanvas::addUpdateRegion(Point2I pos, Point2I ext)
{
    if (mCurUpdateRect.extent.x == 0)
    {
        mCurUpdateRect.point = pos;
        mCurUpdateRect.extent = ext;
    }
    else
    {
        Point2I upperL;
        upperL.x = getMin(mCurUpdateRect.point.x, pos.x);
        upperL.y = getMin(mCurUpdateRect.point.y, pos.y);
        Point2I lowerR;
        lowerR.x = getMax(mCurUpdateRect.point.x + mCurUpdateRect.extent.x, pos.x + ext.x);
        lowerR.y = getMax(mCurUpdateRect.point.y + mCurUpdateRect.extent.y, pos.y + ext.y);
        mCurUpdateRect.point = upperL;
        mCurUpdateRect.extent = lowerR - upperL;
    }
}

void GuiCanvas::resetUpdateRegions()
{
    //DEBUG - get surface width and height
    mOldUpdateRects[0].set(mBounds.point, mBounds.extent);
    mOldUpdateRects[1] = mOldUpdateRects[0];
    mCurUpdateRect = mOldUpdateRects[0];
}

void GuiCanvas::setFirstResponder(GuiControl* newResponder)
{
    GuiControl* oldResponder = mFirstResponder;
    Parent::setFirstResponder(newResponder);

    if (oldResponder && (oldResponder != mFirstResponder))
        oldResponder->onLoseFirstResponder();
}

void GuiCanvas::setDefaultFirstResponder()
{
    GuiControl** oldResponder = &mFirstResponder;
    if (objectList.size())
    {
        GuiControl* lastControl = (GuiControl*)objectList[objectList.size() - 1];
        GuiControl* old = *oldResponder;
        GuiControl* firstTabable = lastControl->findFirstTabable();
        *oldResponder = firstTabable;
        if (old && old != firstTabable)
            old->onLoseFirstResponder();
    }
    else if(*oldResponder) {
        (*oldResponder)->onLoseFirstResponder();
        *oldResponder = NULL;
    }
}

void GuiCanvas::updateReflections()
{
    PROFILE_START(GuiCanvas_updateReflections);

    static SimSet* reflectSet = NULL;

    if (!reflectSet)
    {
        reflectSet = dynamic_cast<SimSet*>(Sim::findObject("reflectiveSet"));
    }

    if (reflectSet)
    {
        GFX->setZEnable(true);

        // execute callback on reflective objects
        for (SimSetIterator itr(reflectSet); *itr; ++itr)
        {
            if (*itr)
            {
                SceneObject* obj = (SceneObject*)*itr;
                obj->updateReflection();
            }
        }
    }

    PROFILE_END();
}

void GuiCanvas::RefreshAndRepaint()
{
    if (gEnableDatablockCanvasRepaint)
    {
        Canvas->paint();
        Platform::process();
    }
}












