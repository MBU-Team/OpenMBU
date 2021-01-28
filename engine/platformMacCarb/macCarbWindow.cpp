//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformMacCarb/platformMacCarb.h"

#include "platform/types.h"
#include "platformMacCarb/platformGL.h"
#include "platform/platform.h"
#include "platform/platformVideo.h"
#include "platformMacCarb/maccarbOGLVideo.h"
#include "platform/event.h"
#include "console/console.h"
#include "platformMacCarb/maccarbConsole.h"
#include "platform/platformInput.h"
//#include "platformMacCarb/maccarbInput.h"
#include "platform/gameInterface.h"
#include "math/mRandom.h"
#include "core/fileStream.h"
#include "game/resource.h"
#include "platformMacCarb/maccarbFileio.h"
#include "core/fileio.h"
#include "platform/platform.h"

//-------------------------------------- Mac stuff

#if defined(TORQUE_OS_MAC_CARB)  //&& (CARBON_VERSION>=0x0120) // !!!TBD -- ASSUMES WE'RE ALWAYS RECENT CARBON LIB.

// !!!!TBD these could get eliminated if we're stable on the new event mgmt & event loop systems.
#define TARG_MACCARB_EVENTS          1
#define CARBEVENTS_RAEL              (1 && TARG_MACCARB_EVENTS)

static MRandomLCG sgPlatRandom;

bool carbonEventsReady = false;
bool carbEventsRAEL = false;

// for OSX window layering control.
//#include "CGWindowLevel.h" // only if we reference the consts or funcs directly...
#if defined(TORQUE_OS_MAC_OSX)
#include <Carbon/Carbon.h>
#include <DrawSprocket/DrawSprocket.h>
#else
#include <MacWindows.h>
#include <DrawSprocket.h>
#endif
WindowGroupRef masterGroup = NULL;

#else // OS8-legacy-stuff.

#include <DrawSprocket.h>

#endif // MAC_CARB ifdef

#include <AGL/agl.h>
#include <stdio.h>

#if USE_SIOUX
#include <SIOUX.h>
#include <Gestalt.h>
#endif

//-------------------------------------- Resource Includes
#include "dgl/gBitmap.h"
#include "dgl/gFont.h"

extern void createFontInit();
extern void createFontShutdown();

void installCarbonEventHandlers(void);
void removeCarbonEventHandlers(void);

EventHandlerRef gWinEventHandlerRef, gAppEventHandlerRef, gTextEventHandlerRef;

bool gWindowCreated = false;

MacCarbPlatState platState;

MacCarbPlatState::MacCarbPlatState()
{
   hDisplay    = NULL;
   appWindow   = NULL;
   
   quit      = false;

   ctx         = NULL;

   // start with something reasonable.
   desktopBitsPixel = 16;
   desktopWidth = 1024;
   desktopHeight = 768;

   osVersion = 0;
   osX = false;
   
   dStrcpy(appWindowTitle, "MacOS Torque Game Engine");
}

// THIS IS A GLOBAL! MUST BE CHANGED WHEN AFFECTING CURSOR VISIBILITY!
int cursorHidden = 0;

static bool windowLocked = false;

static U8 keyboardState[256];
static bool mouseButtonState[8];
static bool capsLockDown = false;
static S32 modifierKeys = 0;
static bool windowActive = true;

static Point2I lastCursorPos(0,0);
static Point2I windowSize;
//static bool sgDoubleByteEnabled = false;

static const char *getKeyName(S32 vkCode);

#define TICKS_FRONT    1L
#define TICKS_BACK     Platform::getBackgroundSleepTime()
static bool gBackgrounded = false;
static long gSleepTicks = 1L; // start with something safe...

#if defined(TORQUE_OS_MAC_CARB)
//--------------------------------------
// helper function to load fnptrs on OSX

//-------------------------------------------------------------------------------
// Function name:  LoadPrivateFrameworksBundle
// Summary:        Looks in the application's Frameworks folder for a framework,
//                  and loads it if it finds it.
//                  Adpoted from a forum post by David 'FenrirWolf' Grace.
//-------------------------------------------------------------------------------
bool LoadPrivateFrameworkBundle(CFStringRef framework, CFBundleRef *bundlePtr) {
	// Looks in the application bundle for a framework, and loads it if it finds it.
	
	CFURLRef   privFrameworksURL, bundleURL;
	if(!( privFrameworksURL = CFBundleCopyPrivateFrameworksURL( CFBundleGetMainBundle() )) ) {
		return false;
	}

	
	if(!( bundleURL = CFURLCreateCopyAppendingPathComponent(NULL, privFrameworksURL, framework, false)) ) {
		CFRelease(privFrameworksURL);
		return false;
	}
	
    *bundlePtr = CFBundleCreate(NULL, bundleURL);
	if (*bundlePtr && CFBundleLoadExecutable( *bundlePtr ) ) {
		//found it under the apps private framework
		CFRelease(privFrameworksURL);
		CFRelease(bundleURL);
		return true;
	}
    
    CFRelease(privFrameworksURL);
    CFRelease(bundleURL);
    if(*bundlePtr != NULL)
        CFRelease(*bundlePtr);
    return false;
}


// this function was adopted from Apple "CallMachOFramework" sample application
OSStatus LoadFrameworkBundle(CFStringRef framework, CFBundleRef *bundlePtr)
{
	const int	numLocs = 5;
	int maxLocs = numLocs - ((platState.osX)?0:1);
	short		vols[numLocs] = {kOnAppropriateDisk,
								kOnAppropriateDisk,
								kOnAppropriateDisk,
								kLocalDomain,
								kLocalDomain};
	OSType		folderType[numLocs] = {kPrivateFrameworksFolderType,
										kFrameworksFolderType,
										kApplicationSupportFolderType,
										kFrameworksFolderType,
										kDomainLibraryFolderType}; // OSX only searchpath.
	OSStatus    err;
	FSRef       frameworksFolderRef;
	CFURLRef   baseURL;
	CFURLRef   bundleURL;

	if (bundlePtr==NULL) return(-1);

	*bundlePtr = NULL;

    // Look in the application's Frameworks folder before looking in the system
	if( LoadPrivateFrameworkBundle(framework, bundlePtr) )
        return noErr ; // Yay!  We found it, so return with no error..
	// Otherwise, fall through to Torques usual bundle load logic
		

	baseURL = NULL;
	bundleURL = NULL;

	for (int i = 0; i<maxLocs; i++)
	{
	   err = FSFindFolder(vols[i], folderType[i], true, &frameworksFolderRef);
	   if (err == noErr) {
	      baseURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &frameworksFolderRef);
	      if (baseURL == NULL) {
	         err = coreFoundationUnknownErr;
	      }
	   }
	   if (err == noErr) {
	      bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorSystemDefault, baseURL, framework, false);
	      if (bundleURL == NULL) {
	         err = coreFoundationUnknownErr;
	      }
	   }
	   if (err == noErr) {
	      *bundlePtr = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
	      if (*bundlePtr == NULL) {
	         err = coreFoundationUnknownErr;
	      }
	   }
	   if (err == noErr) {
	      if ( ! CFBundleLoadExecutable( *bundlePtr ) ) {
	         err = coreFoundationUnknownErr;
	      }
	      else // GOT IT!
	      	break;
	   }
	}
	
   // Clean up.
   if (err != noErr && *bundlePtr != NULL)
   {
      CFRelease(*bundlePtr);
      *bundlePtr = NULL;
   }
   if (bundleURL != NULL)
      CFRelease(bundleURL);
   if (baseURL != NULL)
      CFRelease(baseURL);
   
   return err;
}


//--------------------------------------
// typedefs for the fns we need:
typedef CGDisplayErr (*CGGDWPPtr) (CGPoint p, CGDisplayCount m, CGDirectDisplayID *d, CGDisplayCount *c);
typedef CGRect (*CGDBPtr) (CGDirectDisplayID d);
typedef CGDisplayErr (*CGDMCTPPtr) (CGDirectDisplayID d, CGPoint p);

static CGGDWPPtr cgddGetDisplay = NULL;
static CGDBPtr cgddBounds = NULL;
static CGDMCTPPtr cgddTeleportMouse = NULL;

static OSStatus LoadCGDDFunctions()
{
   CFBundleRef cgBundle;
   OSStatus err = cfragNoSymbolErr;

   if (!platState.osX) return(err);

//   err = LoadFrameworkBundle(CFSTR("CoreGraphics.framework"), &cgBundle);
   err = LoadFrameworkBundle(CFSTR("ApplicationServices.framework"), &cgBundle);
   if (err!=noErr) return(err);
   
   cgddGetDisplay = (CGGDWPPtr) CFBundleGetFunctionPointerForName( cgBundle, CFSTR("CGGetDisplaysWithPoint") );
   if (cgddGetDisplay == NULL)
      err = cfragNoSymbolErr;
   cgddBounds = (CGDBPtr) CFBundleGetFunctionPointerForName( cgBundle, CFSTR("CGDisplayBounds") );
   if (cgddBounds == NULL)
      err = cfragNoSymbolErr;
   cgddTeleportMouse = (CGDMCTPPtr) CFBundleGetFunctionPointerForName( cgBundle, CFSTR("CGDisplayMoveCursorToPoint") );
   if (cgddTeleportMouse == NULL)
      err = cfragNoSymbolErr;
   
   return(err);
}
#endif //TARG_MACCARB


void Platform::AlertOK(const char *windowTitle, const char *message)
{
/*
   S16 hit;
   Str255 title, desc;
   str2p(windowTitle, title);
   str2p(message, desc);

   Input::deactivate();

   StandardAlert(0, title, desc, NULL, &hit);
*/
   S16 hit;
   AlertStdAlertParamRec param;
   Str255 title, desc;
   str2p(windowTitle, title);
   str2p(message, desc);
   param.movable = true;
   param.helpButton = false;
   param.filterProc = NULL;
   param.defaultText = (StringPtr)kAlertDefaultOKText;
   param.cancelText = NULL;
   param.otherText = NULL;
   param.defaultButton = kAlertStdAlertOKButton;
   param.cancelButton = 0;
   param.position = kWindowDefaultPosition;
   // BUG: when in full screen, the alert doesnt show. The best fix for this is new mac platf.
   if( Video::isFullScreen()) {
      Con::printf("Cannot display alerts in full screen mode. Dumping message to console...");
      Con::errorf("%s, %s",windowTitle,message);
      return;
   }
   
   removeCarbonEventHandlers();                 /// stop capturing all events
   ShowCursor();                                /// give user a mouse cursor with which to click buttons
   StandardAlert(0, title, desc, &param, &hit); /// show the alert box 
   installCarbonEventHandlers();                /// capture events again.
   HideCursor();                                /// hide system cursor, so we use torque's cursor again
   return; // (hit==kAlertStdAlertOKButton);
}

bool Platform::AlertOKCancel(const char *windowTitle, const char *message)
{
   S16 hit;
   AlertStdAlertParamRec param;
   Str255 title, desc;
   str2p(windowTitle, title);
   str2p(message, desc);
   param.movable = true;
   param.helpButton = false;
   param.filterProc = NULL;
   param.defaultText = (StringPtr)kAlertDefaultOKText;
   param.cancelText = (StringPtr)kAlertDefaultCancelText;
   param.otherText = NULL;
   param.defaultButton = kAlertStdAlertOKButton;
   param.cancelButton = kAlertStdAlertCancelButton;
   param.position = kWindowDefaultPosition;
   // BUG: when in full screen, the alert doesnt show. The best fix for this is new mac platf.
   if( Video::isFullScreen()) {
      Con::printf("Cannot display alerts in full screen mode. Dumping message to console...");
      Con::errorf("%s, %s",windowTitle,message);
      return true; // always return "OK".
   }
   
   removeCarbonEventHandlers();                 /// stop capturing all events
   ShowCursor();                                /// give user a mouse cursor with which to click buttons
   StandardAlert(0, title, desc, &param, &hit); /// show the alert box
   installCarbonEventHandlers();                /// capture events again.
   HideCursor();                                /// hide system cursor, so we use torque's cursor again
   return (hit==kAlertStdAlertOKButton);
}

bool Platform::AlertRetry(const char *windowTitle, const char *message)
{
   S16 hit;
   AlertStdAlertParamRec param;
   Str255 title, desc;
   Str255 retryStr;
   str2p(windowTitle, title);
   str2p(message, desc);
   str2p("Retry", retryStr);
   param.movable = true;
   param.helpButton = false;
   param.filterProc = NULL;
   param.defaultText = retryStr;
   param.cancelText = (StringPtr)kAlertDefaultCancelText;
   param.otherText = NULL;
   param.defaultButton = kAlertStdAlertOKButton;
   param.cancelButton = kAlertStdAlertCancelButton;
   param.position = kWindowDefaultPosition;
   // BUG: when in full screen, the alert doesnt show. The best fix for this is new mac platf.
   if( Video::isFullScreen()) {
      Con::printf("Cannot display alerts in full screen mode. Dumping message to console...");
      Con::errorf("%s, %s",windowTitle,message);
      return false; // always return 'cancel', as asserts use alertRetry, and constantly retrying would be bad.
   }
   
   removeCarbonEventHandlers();                 /// stop capturing all events
   ShowCursor();                                /// give user a mouse cursor with which to click buttons
   StandardAlert(0, title, desc, &param, &hit); /// show the alert box
   installCarbonEventHandlers();                /// capture events again.
   HideCursor();                                /// hide system cursor, so we use torque's cursor again
   return (hit==kAlertStdAlertOKButton);
}

//--------------------------------------
static void InitInput()
{
   dMemset( keyboardState, 0, 256 );
   dMemset( mouseButtonState, 0, sizeof( mouseButtonState ) );

   if (platState.osX)
   {
      OSStatus err = LoadCGDDFunctions();
   }
}

static Point& ClientToScreen( WindowPtr wnd, Point pt )
{
   static Point screen;
   screen = pt;
   
   GrafPtr savePort;
   GetPort( &savePort );
   SetPortWindowPort(platState.appWindow);
   LocalToGlobal( &screen );
   SetPort( savePort );
   
   return screen;
}


void GetWindowRect( WindowPtr wnd, RectI *pRect )
{
   // note - I should probably add a check for title bar, and borders to simulate Windows completely.
   if ( pRect && wnd ) 
   {
      GrafPtr port;
      Rect r;
      GetPort( &port );
      SetPortWindowPort(wnd);
      GetWindowPortBounds(wnd, &r);
      SetPort( port );
      
      pRect->point.x  = r.left;
      pRect->point.y  = r.top;
      pRect->extent.x = r.right - r.left;
      pRect->extent.y = r.bottom - r.top;
   } 
}


static void RecenterCapturedMouse(void)
{
   if (!platState.appWindow || !windowLocked)
      return;
      
   Rect r;
   Point wndCenter;
   GrafPtr savePort;
   GetPort( &savePort );
   SetPortWindowPort(platState.appWindow);
   GetWindowPortBounds(platState.appWindow, &r);
   wndCenter.h = r.left + (r.right-r.left)>>1;
   wndCenter.v = r.top + (r.bottom-r.top)>>1;
   LocalToGlobal( &wndCenter );

   if (!platState.osX)
   {
   }
#if defined(TORQUE_OS_MAC_CARB)
   else // on OSX, we use CGDD functions.
   if (cgddGetDisplay!=NULL) // then we have the funcs we need.
   {
      CGDirectDisplayID cgdid;
      CGDisplayCount cgdc;
      CGPoint cgp;
      cgp.x =  wndCenter.h;
      cgp.y =  wndCenter.v;
      cgddGetDisplay(cgp, 1, &cgdid, &cgdc);
      if (cgdc) // had better be non-zero!!!
      {
         CGRect cgr = cgddBounds(cgdid);
         cgp.x =  wndCenter.h - cgr.origin.x;
         cgp.y =  wndCenter.v - cgr.origin.y;
         cgddTeleportMouse(cgdid, cgp);
      }
   }
#endif
   
   SetPort( savePort );
}


static void GetMousePos(Point *pt)
{
   GrafPtr savePort;
   GetPort( &savePort );
   SetPortWindowPort( platState.appWindow );
   GetMouse(pt);
   SetPort( savePort );
}


//--------------------------------------
static bool HandleUpdate(WindowPtr w)
{
   if (w && w == platState.appWindow)
   {
#if TARG_MACCARB_EVENTS
      if (!carbonEventsReady)
#endif
      BeginUpdate(w);
      Game->refreshWindow();
#if TARG_MACCARB_EVENTS
      if (!carbonEventsReady)
#endif
      EndUpdate(w);
      return(true);
   }
   
   return(false);
}


//--------------------------------------
static void HandleActivate(WindowPtr w, bool winActive, bool appActive)
{
   if (w && w==platState.appWindow)
   {
      if (appActive)
      {
         ShowWindow(w);
         SelectWindow(w);
         BringToFront(w);
         
         cursorHidden = 0;
         Input::activate();
         
//         Video::reactivate();
      }
      else
      {
//         Video::deactivate();
         
         // if any input captured -- release all input states so we don't errantly keep them locked... 
         Input::deactivate();
         InitCursor(); // reset cursor state.
         cursorHidden = 0;
      }
      
      if (winActive)
      {
         ShowWindow(w);
         SelectWindow(w);
         BringToFront(w);

         cursorHidden = 0;
         Input::activate();
      }
      else
      {
         // if any input captured -- release all input states so we don't errantly keep them locked... 
         Input::deactivate();
         InitCursor(); // reset cursor state.
         cursorHidden = 0;
      }

      HandleUpdate(w);
   }
}

//--------------------------------------
void Platform::setWindowLocked(bool locked)
{
   bool noChange = (windowLocked==locked);
   
   if (!noChange)
   {
      windowLocked = locked;
   
      // do anything needed on state change.  like mouse recentering...
      if (windowLocked)
      {
         // reset the mouse cursor to center
         RecenterCapturedMouse();
         
         // need to do this for sanity.
         //HideMenuBar();
			///tbd this needs to funnel, so fullscreen & windowed share state tracking...
      }
      else
      {
         //ShowMenuBar();
			///tbd this needs to funnel, so fullscreen & windowed share state tracking...
      }
//      setMouseClipping();
   }
}


#if TARG_MACCARB_EVENTS
//--------------------------------------
// the following are the two event handlers
// we custom install.
//--------------------------------------

static bool inWinHandlerAlready = false;

//--------------------------------------
pascal OSStatus DoWinCarbonEventHandling(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
   if (inWinHandlerAlready)
      return(eventNotHandledErr);

   // still needed because we kludge propogate mouse events from the app handler
   inWinHandlerAlready = true;

   OSStatus result = noErr; // Function result
   
   UInt32 eventClass = GetEventClass(theEvent);
   UInt32 eventKind = GetEventKind(theEvent);
   
   InputEvent vev;

   // clear the event value. make sure to not clear the Event initialized fields!!!
   dMemset(((char*)&vev)+sizeof(Event), 0, sizeof(vev)-sizeof(Event));
   
   /* Your preprocessing here */

   // Do preprocessing

   // we used to only process if the next handler up the chain handled the event, or did not handle it.
   // we would only skip handling event if there was some error.
   // propogation has been moved down below.
   
   /* Your postprocessing here */
   result = noErr; // clear result state.
   UInt32 keyMods = 0;
   GetEventParameter (theEvent, kEventParamKeyModifiers, typeUInt32,
                     NULL, sizeof(UInt32), NULL, &keyMods);
   
   modifierKeys = 0;
   if (keyMods & shiftKey)          modifierKeys |= SI_LSHIFT;
   if (keyMods & rightShiftKey)     modifierKeys |= SI_RSHIFT;
   if (keyMods & cmdKey)            modifierKeys |= SI_LALT;
   if (keyMods & optionKey)         modifierKeys |= SI_MAC_LOPT;
   if (keyMods & rightOptionKey)    modifierKeys |= SI_MAC_ROPT;
   if (keyMods & controlKey)        modifierKeys |= SI_LCTRL;
   if (keyMods & rightControlKey)   modifierKeys |= SI_RCTRL;

   // Do postprocessing
   switch(eventClass)
   {
      case kEventClassMouse:
      {
         switch(eventKind)
         {
            case kEventMouseDown:
            case kEventMouseUp:
            {
               EventMouseButton whatButton;
               GetEventParameter (theEvent, kEventParamMouseButton, typeMouseButton,
                                  NULL, sizeof(EventMouseButton), NULL, &whatButton);

               vev.deviceType = MouseDeviceType;
               vev.objType    = SI_BUTTON;
               vev.objInst    = KEY_BUTTON0 + whatButton - 1; // we assume the KEY_BUTTON values are in order, in seq.
               vev.modifier   = modifierKeys;
               vev.ascii      = 0;
               vev.action     = (eventKind==kEventMouseDown)?SI_MAKE:SI_BREAK;
               vev.fValue     = (eventKind==kEventMouseDown)?1.0:0.0;
               Game->postEvent(vev);
               mouseButtonState[whatButton-1] = (eventKind==kEventMouseDown)?1:0;
               break;
            }

            case kEventMouseMoved:
            case kEventMouseDragged:
            {
               GrafPtr savePort;
               GetPort( &savePort );

               Point where;
               GetEventParameter (theEvent, kEventParamMouseLocation, typeQDPoint,
                                  NULL, sizeof(Point), NULL, &where);
               
               if (platState.appWindow)
               {
                  Rect r;
                  
                  // gee, almost forgot about this -- value is global, we want it window-local.
                  SetPortWindowPort(platState.appWindow);
                  GlobalToLocal(&where);
                  GetWindowPortBounds(platState.appWindow, &r);
                  SetPort(savePort);
               
                  // if mouse is in our window, make sure it is hidden.  if not, it should be visible.
                  bool inWindow = PtInRect(where, &r);
                  if (cursorHidden && !inWindow)
                  {
                     ShowCursor();
                     cursorHidden = 0;
                  }
                  else
                  if (!cursorHidden && inWindow)
                  {
                     HideCursor();
                     cursorHidden = 1;
                  }
               }

               // this is a hack workaround to manage the fact that the game wants delta values...
               static bool reset = true;
               static Point lastMouseLoc = {-11,-11}; // something unlikely...
               if (windowLocked && !platState.osX)
               if (!windowActive || gBackgrounded) //!!!!TBD windowActive/Locked should go false if bg.
               {
                  reset = true;
                  break; // out of this case.
               }

               static int wasLocked = 0;
               if (!windowLocked)
               { // then we want the raw, global mouse position.
                  MouseMoveEvent mme;
                  mme.xPos = where.h;     // horizontal position of cursor 
                  mme.yPos = where.v;     // vertical position of cursor 
                  mme.modifier = modifierKeys;
                  Game->postEvent(mme);
                  // if we care, we can check locked state flipping back.
                  // for now, just set flag.
                  wasLocked = 0;
               }
               else
               {
                  vev.deviceType = MouseDeviceType;
                  vev.objInst    = 0;
                  vev.modifier   = modifierKeys;
                  vev.ascii      = 0;
                  vev.action     = SI_MOVE;

#if defined(TORQUE_OS_MAC_OSX)
                  // we don't actually use the event data.
                  // we grab the raw delta from coregraphics.
                  CGMouseDelta mdx, mdy;
                  CGGetLastMouseDelta(&mdx, &mdy);
#else
                  // HACK for input control in-game when CFM on X, or when ISp temp disabled.
                  // There needs to be Input:: takeover of user input
                  // when the window is locked (gameplay)
                  int mdx, mdy;
                  if (platState.osX)
                  { // try to use carbon system for retrieving delta...
                     GetEventParameter (theEvent, kEventParamMouseDelta, typeQDPoint,
                                        NULL, sizeof(Point), NULL, &where);
                     mdx = where.h;
                     mdy = where.v;
                  }
                  else
                  { // try to determine delta ourselves, and not use carbon system...
                     // where already contains curr position...
                     if (reset)
                     {
                        reset = false;
                        lastMouseLoc = where;
                        break; // this case.
                     }
                  
                     mdx = where.h - lastMouseLoc.h;
                     mdy = where.v - lastMouseLoc.v;
                     lastMouseLoc = where;
                  }
#endif

                  if (wasLocked>1) // ignore first two events, so we skip errant behaviors...
                  { // post the event(s), possibly clamped a bit...
                     const int MAX_MOUSE_MOVE = 256;
                     if (mdx)
                     {
                        if (mdx > MAX_MOUSE_MOVE)
                           mdx = MAX_MOUSE_MOVE;
                        else if (mdx < -MAX_MOUSE_MOVE)
                           mdx = -MAX_MOUSE_MOVE;
                        vev.objType    = SI_XAXIS;
                        vev.fValue     = F32(mdx);
                        Game->postEvent(vev);
                     }
                     if (mdy)
                     {
                        if (mdy > MAX_MOUSE_MOVE)
                           mdy = MAX_MOUSE_MOVE;
                        else if (mdy < -MAX_MOUSE_MOVE)
                           mdy = -MAX_MOUSE_MOVE;
                        vev.objType    = SI_YAXIS;
                        vev.fValue     = F32(mdy);
                        Game->postEvent(vev);
                     }
                  }
                  else
                     wasLocked++; // else drop the events ON THE FLOOR.
                  
                  // reset the mouse cursor to center
                  RecenterCapturedMouse();
               }

               break;
            }

            case kEventMouseWheelMoved:
            {
               long dWheel = 0;
               GetEventParameter (theEvent, kEventParamMouseWheelDelta, typeLongInteger,
                                  NULL, sizeof(long), NULL, &dWheel);

               vev.deviceType = MouseDeviceType;
               vev.objType    = SI_ZAXIS;
               vev.objInst    = 0;
               vev.modifier   = 0; 
               vev.ascii      = 0; 
               vev.action     = SI_MOVE;
               vev.fValue     = dWheel; // !!!!TBD -- multiplication factor!!!!????
               Game->postEvent(vev);

               break;
            }

            default:
               result = eventNotHandledErr;
               break;
         }
         break;
      }

      case kEventClassWindow:
      {
         WindowRef theWind;
         GetEventParameter (theEvent, kEventParamDirectObject, typeWindowRef,
                             NULL, sizeof(WindowRef), NULL, &theWind);

         switch(eventKind)
         {
            case kEventWindowDrawContent:
            {
               // !!!!TBD.  Do we ned to bother checking theWind == platState.appWindow?
               Game->refreshWindow();
               break;
            }

            case kEventWindowActivated:
            case kEventWindowDeactivated:
            {
               // !!!!TBD.  Do we ned to bother checking theWind == platState.appWindow?
               HandleActivate(theWind, eventKind==kEventWindowActivated, !gBackgrounded);
               Game->refreshWindow();
               // mark that we didn't handle, so hopefully someone else will do the right thing.
               result = eventNotHandledErr;
               break;
            }

            default:
               result = eventNotHandledErr;
               break;
         }
         
         break;
      }

      default:
         result = eventNotHandledErr;
         break;
   }
   

#define PROPAGATE_EVENTS      1
#if PROPAGATE_EVENTS // whether to propagate...
   // Now propagate the event to the next handler (in this case the standard window handler)
   if (nextHandler)
   {
      result = CallNextEventHandler (nextHandler, theEvent);
   }
#endif


inWinHandlerAlready = false; // ending.

   return result; // Report result
}


//--------------------------------------
pascal OSStatus DoAppCarbonEventHandling(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
   OSStatus result = noErr; // Function result
   
   UInt32 eventClass = GetEventClass(theEvent);
   UInt32 eventKind = GetEventKind(theEvent);
   
   InputEvent vev;

   // clear the event value. make sure to not clear the Event initialized fields!!!
   dMemset(((char*)&vev)+sizeof(Event), 0, sizeof(vev)-sizeof(Event));
   
   /* Your preprocessing here */

#if PROPAGATE_EVENTS // whether to propagate...
   // Now propagate the event to the next handler (in this case the standard window handler)
   if (nextHandler)
      result = CallNextEventHandler (nextHandler, theEvent);
#endif
   if (result == noErr
   ||  result == eventNotHandledErr) // Did it succeed or was unhandled?
   {
      /* Your postprocessing here */
      result = noErr; // clear result state.

      // Do postprocessing
      switch(eventClass)
      {
         case kEventClassAppleEvent:
         {
            AEEventID aeType;
            GetEventParameter (theEvent, kEventParamAEEventID, typeType,
                                NULL, sizeof(OSType), NULL, &aeType);

            switch(aeType)
            {
               case kAEQuitApplication:
               {
                  Platform::postQuitMessage(0);
                  break;
               }

               default:
                  result = eventNotHandledErr;
 //                 AEProcessAppleEvent(&msg?????);
                  break;
            }
            
            break;
         }

         case kEventClassApplication:
         {
            switch(eventKind)
            {
               case kEventAppQuit:
               {
                  Platform::postQuitMessage(0);
                  break;
               }

               case kEventAppActivated:
               case kEventAppDeactivated:
               {
                  bool activating = (eventKind==kEventAppActivated);
                  
                  HandleActivate(platState.appWindow, activating, activating);
                  gBackgrounded = !activating;
                  if (gBackgrounded)
                     gSleepTicks = TICKS_BACK;
                  else
                     gSleepTicks = TICKS_FRONT;
/*                  
                  Cursor arrow; 
                  GetQDGlobalsArrow(&arrow);
                  if (gBackgrounded)   // just suspended
                     SetCursor(&arrow);
                  else                  // just resumed
                     SetCursor(&arrow);
*/                  
                  // mark it so that others will try to 'do the right thing' for us.
                  result = eventNotHandledErr;
                  
                  break;
               }

               default:
                  result = eventNotHandledErr;
                  break;
            }
            
            break;
         }
         
         case kEventClassMouse:
         {
            switch(eventKind)
            {
               case kEventMouseDown:
               case kEventMouseUp:
               case kEventMouseMoved:
               case kEventMouseDragged:
               case kEventMouseWheelMoved:
               {
                  // HACK HACK HACK HACK
                  // this should really be dispatched through the right
                  // dispatching mechanism to the main window...
                  if (!inWinHandlerAlready)
                     result = DoWinCarbonEventHandling(NULL, theEvent, userData);
                  break;
               }

               default:
                  result = eventNotHandledErr;
                  break;
            }
            break;
         }

         default:
            result = eventNotHandledErr;
            break;
      }

   }

   return result; // Report result
}
//--------------------------------------
#endif // targ_maccarb_events

//--------------------------------------
static Point lastPos = {-11,-11}; // something unlikely...
static void CheckCursorPos()
{
   static bool reset = true;

   if (!windowActive || gBackgrounded) //!!!!TBD windowActive/Locked should go false if bg.
   {
      reset = true;
      return;
   }
   
   InputEvent event;
   Point mousePos;
   GetMousePos(&mousePos);

   if (!windowLocked)
   {
      // since we're in Poll mode, we need to post a mousemove event for the cursor to be updated.
      MouseMoveEvent mme;
      mme.xPos = mousePos.h;     // horizontal position of cursor 
      mme.yPos = mousePos.v;     // vertical position of cursor 
      mme.modifier = modifierKeys;
      Game->postEvent(mme);
   }
   else
   {
      if (reset)
         reset = false;
      else
      if (mousePos.h!=lastPos.h
      &&  mousePos.v!=lastPos.v) // mouse moved.
      {
         if(mousePos.h != lastPos.h)
         {
            event.deviceInst  = 0;
            event.deviceType  = MouseDeviceType;
            event.objType     = SI_XAXIS;
            event.objInst     = 0;
            event.action      = SI_MOVE;
            event.modifier    = modifierKeys;
            event.ascii       = 0;
            event.fValue      = F32(mousePos.h - lastPos.h);
            Game->postEvent(event);
//            Con::printf( "EVENT: Mouse move (%.1f, 0.0).\n", event.fValue );
         }

         if(mousePos.v != lastPos.v)
         {
            event.deviceInst  = 0;
            event.deviceType  = MouseDeviceType;
            event.objType     = SI_YAXIS;
            event.objInst     = 0;
            event.action      = SI_MOVE;
            event.modifier    = modifierKeys;
            event.ascii       = 0;
            event.fValue      = F32(mousePos.v - lastPos.v);
            Game->postEvent(event);
//           Con::printf( "EVENT: Mouse move (0.0, %.1f).\n", event.fValue );
         }

      }

      lastPos = mousePos; // copy over last position.
   }
}

//--------------------------------------
#define TICK_WAIT_FOR_WNE        4
static bool ProcessMessages()
{
   EventRecord msg;
   bool done = false;
   static U32 lastTick = 0;
   U32 newTick = TickCount();

   if (platState.quit) return false;

      EventRef carbonEvent;
      EventTargetRef theTarget;
      OSStatus theErr;

      theTarget = GetEventDispatcherTarget();
      do
      {
         theErr = ReceiveNextEvent(0, NULL, kEventDurationNoWait, true, &carbonEvent);
         if (theErr == noErr && carbonEvent != NULL)
         {
            bool handled = false;

            // make a normal Mac event.
            // !!!!TBD

            // not sure if this is needed under OSX.
            // !!!!TBD.
            extern bool gDSpActive;
            if (gDSpActive)
            {
               // pass to DSp.
               Boolean dspdid;
               DSpProcessEvent(&msg, &dspdid);
               if (dspdid) handled = true;
            }

   // !!!!TBD
   //         if (!handled)
   //          handled = (gConsole && gConsole->handleEvent(&msg));
	
            if (!handled)
               theErr = SendEventToEventTarget(carbonEvent, theTarget);
            ReleaseEvent(carbonEvent);
         }
      }
      while (theErr == noErr);
   
   return true;
}


//--------------------------------------
void Platform::process()
{
   extern bool gMouseActive;
   if (!Input::isActive() || !gMouseActive)
#if TARG_MACCARB_EVENTS
   if (!carbonEventsReady) // we don't poll mouse if we have CarbonEvents working...
#endif
      CheckCursorPos();

/*
#if 0//TARG_MACCARB_EVENTS
// the moment we activate this code block, we lose all base WNE handling,
// so we'd need to convert to using a carbon timer or something to process the main loop.
   if (carbonEventsReady)
   {
      RunApplicationEventLoop();
   }
   else
#endif
*/
   if(!ProcessMessages())
   {
      // generate a quit event
      Event quitEvent;
      quitEvent.type = QuitEventType;
      
      Game->postEvent(quitEvent);
   }


   Input::process();
}

extern U32 calculateCRC(void * buffer, S32 len, U32 crcVal );

#if defined(TORQUE_DEBUG) || defined(INTERNAL_RELEASE)
static U32 stubCRC = 0;
#else
static U32 stubCRC = 0xEA63F56C;
#endif

//--------------------------------------
/*
static void InitWindowClass()
{
   WNDCLASS wc;
   dMemset(&wc, 0, sizeof(wc));
   
   wc.style         = CS_OWNDC;
   wc.lpfnWndProc   = WindowProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = winState.appInstance;
   wc.hIcon         = LoadIcon(winState.appInstance, MAKEINTRESOURCE(IDI_ICON2));
   wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
   wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
   wc.lpszMenuName  = 0;
   wc.lpszClassName = windowClassName;
   RegisterClass( &wc );
   
   // Curtain window class:
   wc.lpfnWndProc   = DefWindowProc;
   wc.hCursor       = NULL;
   wc.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
   wc.lpszClassName = "Curtain";
   RegisterClass( &wc );
}
*/

//--------------------------------------
static void GetDesktopState()
{
   // desired display was already set inside Video::Init.
   //platState.hDisplay = GetMainDevice();
   
   Rect r = (*(platState.hDisplay))->gdRect;
   platState.desktopWidth = r.right - r.left;
   platState.desktopHeight = r.bottom - r.top;
   platState.desktopBitsPixel = (*(*(platState.hDisplay))->gdPMap)->pixelSize;
}



//--------------------------------------
WindowPtr CreateOpenGLWindow( GDHandle hDevice, U32 width, U32 height, bool fullScreen )
{
   WindowPtr w = NULL;
   int offset = 144; // some nice starting offset.
   bool noConsoleOffset = true;
   
   // this rect should really be based off the device coords... !!!!!TBD   
   Rect   rect, dRect;
   // note: OpenGL is expecting a window so it can enumerate the devices it spans, 
   // center window in our context's gdevice
   dRect = (**hDevice).gdRect;
   if (fullScreen || noConsoleOffset)
   {
      // not sure all the casting is needed, but...
      rect.top  = (short) (dRect.top + (dRect.bottom - dRect.top) / 2);        // h center
      rect.top  -= (height / 2);
      rect.left  = (short) (dRect.left + (dRect.right - dRect.left) / 2);   // v center
      rect.left  -= (width / 2);
      rect.right = (short) (rect.left + width);
      rect.bottom = (short) (rect.top + height);
   }
   else
   {
      // do an offset window, based on device bounds.
      SetRect( &rect, dRect.left+offset+48, dRect.top+offset, dRect.left+width+offset+48, dRect.top+height+offset);
   }
   
   if (platState.osX)
   {
      OSStatus stat;
      WindowAttributes windAttr = 0L;
      WindowClass windClass = kDocumentWindowClass;
      if (fullScreen)
      {
         windAttr |= kWindowNoShadowAttribute; // no shadow when fullscreen window.
         windClass = kAltPlainWindowClass;//kMovableAlertWindowClass; //kPlainWindowClass; //kOverlayWindowClass;
         if (windClass==kOverlayWindowClass)
            windAttr |= kWindowOpaqueForEventsAttribute;
      }
      else
         windAttr |= kWindowCollapseBoxAttribute;
      
      stat = CreateNewWindow(windClass, windAttr, &rect, &w);
      if (stat!=noErr || w==NULL)
         return(NULL); // !!!! ASSERT?????

      if (fullScreen)
      { // move the thing into the proper level, in front of the blanking window.
         // get blanking window level.
         // create a new group if ours doesn't already exist
         if (masterGroup==NULL)
         {
            WindowGroupAttributes grpAttr = 0L; // no special atts to start.
            stat = CreateWindowGroup(grpAttr, &masterGroup);
            // get the window's current group
            // TBD!!!!!
            // walk up the parent hierarchy, to find the topmost group.
            // TBD!!!!!
            // set the parent of our group to the topmost group.
            // TBD!!!!!
         }
         // place window in group
         stat = SetWindowGroup(w, masterGroup);
         if (stat!=noErr)
         {
         	int errCode1 = stat;
         }
         // set window level to one higher than blanking window.
         // for the moment, we'll hack this.
         stat = SetWindowGroupLevel(masterGroup, 1010); // above screen saver...
         if (stat!=noErr)
         {
         	int errCode2 = stat;
         }
//#define kCGScreenSaverWindowLevel   CGWindowLevelForKey(kCGScreenSaverWindowLevelKey)   /* 1000 */
//#define kCGMaximumWindowLevel       CGWindowLevelForKey(kCGMaximumWindowLevelKey)   /* LONG_MAX - kCGNumReservedWindowLevels */
      }
   }
   else
   { // old WMgr func.  this will become outdated...
      w = NewCWindow(NULL, 
                     &rect,                                                    // bounding rect
                     str2p(platState.appWindowTitle),                         // window title
                     false,                                                   // is visible
                     fullScreen?kWindowPlainDialogProc:kWindowDocumentProc,   // window type
                     (WindowPtr) -1L,                                          // top most window
                     false,                                                    // has a close box
                     0L);                                                      // reference constant
   }
   
   if (w != NULL)
   {
      GrafPtr savegp;
      RGBColor savergb;
      Rect bounds;
      const RGBColor rgbBlack = { 0x0000, 0x0000, 0x0000 };

      ShowWindow(w);
      SelectWindow(w);
      BringToFront(w);

      // paint back ground black before fade in to avoid white background flash
      GetPort (&savegp);
      SetPortWindowPort (w);
      GetForeColor (&savergb);
      RGBForeColor (&rgbBlack);
      GetWindowPortBounds (w, &bounds);
      PaintRect (&bounds);
      RGBForeColor (&savergb);      // ensure color is reset for proper blitting
      SetPort (savegp);
   }
   
   return(w);
}

//--------------------------------------
WindowPtr CreateCurtain( U32 width, U32 height )
{
   WindowPtr w=NULL;
/*   
   w =  CreateWindow( 
      "Curtain",
      "",
      ( WS_POPUP | WS_MAXIMIZE | WS_VISIBLE ),
      0, 0,
      width, height,
      NULL, NULL,
      winState.appInstance,
      NULL );
*/
   return(w);
}


/*
//--------------------------------------
void CreatePixelFormat( PIXELFORMATDESCRIPTOR *pPFD, S32 colorBits, S32 depthBits, S32 stencilBits, bool stereo )
{
   PIXELFORMATDESCRIPTOR src = 
   {
      sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd
      1,                      // version number
      PFD_DRAW_TO_WINDOW |    // support window
      PFD_SUPPORT_OPENGL |    // support OpenGL
      PFD_DOUBLEBUFFER,       // double buffered
      PFD_TYPE_RGBA,          // RGBA type
      colorBits,              // color depth
      0, 0, 0, 0, 0, 0,       // color bits ignored
      0,                      // no alpha buffer
      0,                      // shift bit ignored
      0,                      // no accumulation buffer
      0, 0, 0, 0,             // accum bits ignored
      depthBits,              // z-buffer   
      stencilBits,            // stencil buffer
      0,                      // no auxiliary buffer
      PFD_MAIN_PLANE,         // main layer
      0,                      // reserved
      0, 0, 0                 // layer masks ignored
    };

   if ( stereo )
   {
      //ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
      src.dwFlags |= PFD_STEREO;
      //glConfig.stereoEnabled = true;
   }
   else
   {
      //glConfig.stereoEnabled = qfalse;
   }
   *pPFD = src;
}

//--------------------------------------
enum { MAX_PFDS = 256 };

S32 ChooseBestPixelFormat(HDC hDC, PIXELFORMATDESCRIPTOR *pPFD)
{
   PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
   S32 i;
   S32 bestMatch = 0;
   
   S32 maxPFD = qwglDescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[0]);
   if(maxPFD > MAX_PFDS)
      maxPFD = MAX_PFDS;

   bool accelerated = false;
   
   for(i = 1; i <= maxPFD; i++)
   {
      qwglDescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);

      // make sure this has hardware acceleration:
      if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) 
         continue;

      // verify pixel type
      if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
         continue;

      // verify proper flags
      if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) 
         continue;

      accelerated = !(pfds[i].dwFlags & PFD_GENERIC_FORMAT);

      //
      // selection criteria (in order of priority):
      // 
      //  PFD_STEREO
      //  colorBits
      //  depthBits
      //  stencilBits
      //
      if ( bestMatch )
      {
         // check stereo
         if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
         {
            bestMatch = i;
            continue;
         }
         
         if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
         {
            bestMatch = i;
            continue;
         }

         // check color
         if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
         {
            // prefer perfect match
            if ( pfds[i].cColorBits == pPFD->cColorBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
            {
               bestMatch = i;
               continue;
            }
         }

         // check depth
         if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
         {
            // prefer perfect match
            if ( pfds[i].cDepthBits == pPFD->cDepthBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
            {
               bestMatch = i;
               continue;
            }
         }

         // check stencil
         if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
         {
            // prefer perfect match
            if ( pfds[i].cStencilBits == pPFD->cStencilBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) && 
                ( pPFD->cStencilBits > 0 ) )
            {
               bestMatch = i;
               continue;
            }
         }
      }
      else
      {
         bestMatch = i;
      }
   }
   
   if ( !bestMatch )
      return 0;

   else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
   {
      // MCD
   }
   else
   {
      // ICD
   }

   *pPFD = pfds[bestMatch];

   return bestMatch;
}
*/


//--------------------------------------
const Point2I &Platform::getWindowSize()
{
   return windowSize;
}


//--------------------------------------
void Platform::setWindowSize( U32 newWidth, U32 newHeight )
{
   windowSize.set( newWidth, newHeight );
}


//--------------------------------------
// !!!!!TBD!!!!! what should this do on the Mac.
void Platform::minimizeWindow()
{
}


//--------------------------------------
static void InitWindow(const Point2I &initialSize)
{
   windowSize = initialSize;   
}


//--------------------------------------
static void InitOpenGL()
{
   DisplayDevice::init();

   // Get the video settings from the prefs.
   const char* resString;
   U32 height, width, bpp;
   char *tempBuf, *s;
   bool fullScreen = Con::getBoolVariable( "$pref::Video::fullScreen" );
   // properly grab fullscreen vs windowed resolution.   
   if (fullScreen)
      resString = Con::getVariable( "$pref::Video::resolution" );
   else
      resString = Con::getVariable( "$pref::Video::windowedRes" );

   tempBuf = new char[dStrlen( resString ) + 1];
   dStrcpy( tempBuf, resString );

   s = dStrtok( tempBuf, " x\0" );
   width = ( s ? dAtoi( s ) : (windowSize.x > 0 ? windowSize.x : 640) );  //DAW: Added check for windowSize

   s = dStrtok( NULL, " x\0" );
   height = ( s ? dAtoi( s ) : (windowSize.y > 0 ? windowSize.y : 480) );  //DAW: Added check for windowSize

   if (fullScreen)
   {
      s = dStrtok( NULL, "\0" );
      bpp = ( s ? dAtoi( s ) : 16 );
   }
   else
      bpp = platState.desktopBitsPixel;

   delete [] tempBuf;

   if ( !Video::setDevice( Con::getVariable( "$pref::Video::displayDevice" ), width, height, bpp, fullScreen ) )
   {
      // Next, try the default OpenGL device:
      if ( !Video::setDevice( "OpenGL", width, height, bpp, fullScreen ) )
      {
           AssertFatal( false, "Could not find a compatible display device!" );
           return;
      }
   }
}


//--------------------------------------
ConsoleFunction( getDesktopResolution, const char*, 1, 1, "getDesktopResolution()" )
{
   argc; argv;
   char buffer[256];
   dSprintf( buffer, sizeof( buffer ), "%d %d %d", platState.desktopWidth, platState.desktopHeight, platState.desktopBitsPixel );
   char* returnString = Con::getReturnBuffer( dStrlen( buffer ) + 1 );
   dStrcpy( returnString, buffer );
   return( returnString ); 
}


//--------------------------------------
void Platform::init()
{
   // Set the platform variable for the scripts
   Con::setVariable( "$platform", "macos" );

   MacConsole::create();
   if ( !MacConsole::isEnabled() )
      Input::init();
   InitInput();   // as internal windowing input handles different stuff from Input object.

// not sure why this is here on mac, but so be it.
   DisplayDevice *dev = NULL;
   Con::printf( "Video Init:" );
   Video::init();
   dev = OpenGLDevice::create();
   // now safe to grab the right desktop size.
   GetDesktopState();
   // and now we can install the device.
   if ( Video::installDevice(dev) )
      Con::printf( "   Accelerated OpenGL display device detected." );
   else
      Con::printf( "   Accelerated OpenGL display device not detected." );
   Con::printf( "" );

//   sgDoubleByteEnabled = true; // !!!!! this right?
//   sgQueueEvents = true;
}

//--------------------------------------
void Platform::shutdown()
{
//   sgQueueEvents = false;

//   if(gMutexHandle)
//      CloseHandle(gMutexHandle);
   setWindowLocked( false );
   Video::destroy();
   Input::destroy();
   MacConsole::destroy();
}


//--------------------------------------
static U32 lastTimeTick;
static S32 passargc = 0;
static char **passargv = NULL;
static S32 appReturn = 0;

//--------------------------------------
static S32 run()
{
   createFontInit();
   windowSize.set(0,0);
   
   lastTimeTick = Platform::getRealMilliseconds();
   
   int ret = Game->main(passargc, (const char **)passargv);

   createFontShutdown();

   return ret;
}

//--------------------------------------
void Platform::initWindow(const Point2I &initialSize, const char *name)
{
/* // on the mac, video is currently initalized earlier...
   DisplayDevice *dev = NULL;
   Con::printf( "Video Init:" );
   Video::init();
   dev = OpenGLDevice::create();
   // now safe to grab the right desktop size.
   GetDesktopState();
   // and now we can install the device.
   if ( Video::installDevice(dev) )
      Con::printf( "   Accelerated OpenGL display device detected." );
   else
      Con::printf( "   Accelerated OpenGL display device not detected." );
   Con::printf( "" );
*/

   dSprintf(platState.appWindowTitle, sizeof(platState.appWindowTitle), name);
   InitWindow(initialSize); // this doesn't create the window -- gl subcode it.
   InitOpenGL();
  
   if (platState.appWindow)
   {
      gWindowCreated = true; // if we get in here, we really have a secondary window, not just a console...
#if TARG_MACCARB_EVENTS
      // install handlers to the given window.
      EventTargetRef winTarg = GetWindowEventTarget(platState.appWindow);
#define DEFAULT_HANDLERS      1
#if DEFAULT_HANDLERS
      InstallStandardEventHandler(winTarg);
      InstallStandardEventHandler(GetApplicationEventTarget());
#endif
   installCarbonEventHandlers();
#endif
   }
}


#if CARBEVENTS_RAEL
//--------------------------------------
pascal void DoRAELLoop(EventLoopTimerRef theTimer, void *userData)
{
   appReturn = run();
   QuitApplicationEventLoop();
}
#endif


//--------------------------------------
S32 main(S32 argc, const char **argv)
{
   bool optOutFromCmdline = false;
    
   // set up starting values.
   passargc = argc;
   passargv = (char**)argv;
  
   // Get rid of the -psn argument
   for(int j=0; j<passargc; j++)
      if(!strncmp(passargv[j], "-psn", 4))
	     passargv[j] = "";
		 
   InitCursor();
   
   FlushEvents( everyEvent, 0 );
   SetEventMask(everyEvent);

   // save away OS version info into platState.
   if (Gestalt(gestaltSystemVersion, (SInt32 *) &(platState.osVersion)) == noErr)
   {
      platState.osX = false;
      if (platState.osVersion >= 0x01000)
         platState.osX = true;
   }

   // Update the current working directory.
   Platform::getWorkingDirectory();

   // this is yucky, but the easiest way to ignore the cmdline text file:
   #define KEYISDOWN(key) ((((unsigned char *)currKeyState)[key>>3] >> (key & 7)) & 1)
   KeyMap currKeyState;
   GetKeys(currKeyState);
   if (KEYISDOWN(0x38)) // check shift key -- actually LShift.
      optOutFromCmdline = true;

   // mac does not support command line arguments.  well, OSX does, but not easily for the average user...
   // it may be possible to get the comment field from the file
   // see DesktopManager and DTGetComment
   // BUT, for the moment, try to open a file "maccmdline.txt" and use first line.
   #define MAX_CMDLINE  2048
   static char cmdline[2048]; // make it 2K just in case...
   static const char *cmdargv[32]; // 32 max commands?
   if (!optOutFromCmdline)
   if (1) // used to check and only override if there were no args.  now, we merge them.
   {
      File cmdfile;
      S32 cmdargc=0;
      U32 cmdlen;
      File::Status cmderr;
      cmderr = cmdfile.open("maccmdline.txt", cmdfile.Read);
      if (cmderr == File::Ok)
      {
         cmderr = cmdfile.read(MAX_CMDLINE-1, cmdline, &cmdlen);
         if ((cmderr == File::Ok || cmderr == File::EOS) && cmdlen>1) // if empty file, don't bother...
         {
            cmdline[cmdlen] = 0; // null terminate.
            cmdlen++; // add the null to len.
            int i = 0;
            
            // first, reset the max length if there's a CR or LF.
            while (i<cmdlen)
            {
               if (cmdline[i]=='\n' || cmdline[i]=='\r')
               {
                  cmdline[i] = 0; // null terminate.
                  cmdlen = i+1; // set new length.
                  break;
               }
               i++;
            }
            
            // now build args.
            // arg[0] should be exe name/path.
            // on OSX, already there.  under 10.1, looks like there's a psn serial number as arg 2.  pain in the ass.
            // on OS9, we could put in the real thing, but I toss in a fake string for now.  !!!!TBD
            // !!!!TBD.  mac can't call getWorkingDir here, as it's not yet inited properly.

			    // Exclude the magic -psn process id parameter
				if(strncmp(argv[0], "-psn", 4))
			      cmdargv[cmdargc++] = argv[0];

            // arg[1] will be the first thing in the cmdline, if anything is there...
            cmdargv[cmdargc] = cmdline;

            i = 0;
            while (i<cmdlen)
            {
               if (cmdline[i]==0 || cmdline[i]==' ') // on nulls or spaces.
               {
                  cmdline[i] = 0;
                  cmdargc++; // we hit a command, so inc the count.
                  // set up for next command
                  cmdargv[cmdargc] = &(cmdline[i+1]); // start at char past null.
               }
               i++;
            }
            
            // now, tack on any other parameters from the real commandline.
            for (i=1; i<argc; i++)
            {
			    // Exclude the magic -psn process id parameter
				if(strncmp(argv[i], "-psn", 4))
					cmdargv[cmdargc++] = argv[i];
            }
            
//            if (cmdargc>1) // since app path always there... // for now, always used.
            { // copy/set the vars.
               passargc = cmdargc;
               passargv = (char**)cmdargv;
            }
         }
         cmdfile.close();
      }
   }

   appReturn = 0;
      
#if TARG_MACCARB_EVENTS
#if CARBEVENTS_RAEL
   carbEventsRAEL = true;
#endif

   if (carbEventsRAEL)
   {
      EventLoopTimerRef theTimer;
      // Install a one-shot timer to run the game, then call RAEL to install
      // the default application handler (which can't be called directly).
      InstallEventLoopTimer(GetCurrentEventLoop(), 0, 0, NewEventLoopTimerUPP(DoRAELLoop), NULL, &theTimer);
      RunApplicationEventLoop();
   }
   else
#endif
      appReturn = run(); // use the statics/globals rather than actually passing as params.
   
   InitCursor(); // don't leave it in a screwy state...

   return(appReturn);
}

//--------------------------------------
void TimeManager::process()
{
   U32 curTime = Platform::getRealMilliseconds(); // GTC returns Milliseconds, FYI.
   TimeEvent event;
   event.elapsedTime = curTime - lastTimeTick;
   if(event.elapsedTime > sgTimeManagerProcessInterval)
   {
      lastTimeTick = curTime;
      Game->postEvent(event);
   }
}


//--------------------------------------
F32 Platform::getRandom()
{
   return sgPlatRandom.randF();
}

//--------------------------------------
// Web browser function:
//--------------------------------------
bool Platform::openWebBrowser( const char* webAddress )
{
   OSStatus err;
   CFURLRef url = CFURLCreateWithBytes(NULL,(UInt8*)webAddress,dStrlen(webAddress),kCFStringEncodingASCII,NULL);
   err = LSOpenCFURLRef(url,NULL);
   CFRelease(url);

   return(err==noErr);
}

//--------------------------------------
void Platform::enableKeyboardTranslation(void)
{
   ActivateTSMDocument(platState.tsmDoc);
   platState.tsmActive=true;
   Con::printf("tsm doc should be active");
}

//--------------------------------------
void Platform::disableKeyboardTranslation(void)
{
   DeactivateTSMDocument(platState.tsmDoc);
   platState.tsmActive=false;
   Con::printf("tsm doc Deactivated");
}

//--------------------------------------
void handleRawKeyEvent(EventRef theEvent, InputEvent &torqueEvent)
{
   // --------- modifiers ---------
   UInt32 keyMods = 0;
   GetEventParameter (theEvent, kEventParamKeyModifiers, typeUInt32,
                     NULL, sizeof(UInt32), NULL, &keyMods);
   
   U8 modifierKeys = 0;
   if (keyMods & shiftKey)          modifierKeys |= SI_LSHIFT;
   if (keyMods & rightShiftKey)     modifierKeys |= SI_RSHIFT;
   if (keyMods & cmdKey)            modifierKeys |= SI_LALT;
   if (keyMods & optionKey)         modifierKeys |= SI_MAC_LOPT;
   if (keyMods & rightOptionKey)    modifierKeys |= SI_MAC_ROPT;
   if (keyMods & controlKey)        modifierKeys |= SI_LCTRL;
   if (keyMods & rightControlKey)   modifierKeys |= SI_RCTRL;
   // --
   
   // ------- keycode & keychar -----
   UInt32   keyCode = 0;
   char     keyChar = 0;
   
   GetEventParameter (theEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
   GetEventParameter (theEvent, kEventParamKeyMacCharCodes, typeChar,NULL, sizeof(char), NULL, &keyChar);
   // -- 

   // ------- action & value ( make, break, repeat ) -----
   U8 action;
   float fvalue;
   U32 eventKind = GetEventKind(theEvent);
   switch(eventKind)
   {
      case kEventRawKeyDown:
         action = SI_MAKE;
         fvalue = 1.0f;
         break;
      case kEventRawKeyUp:
         action = SI_BREAK;
         fvalue = 0.0f;
         break;
      case kEventRawKeyRepeat:
         action = SI_REPEAT;
         fvalue = 1.0f;
         break;
      default:
         AssertISV(false, "Unhandled keyboard event kind!");
   }
   // --
   
   //---- fill the event ----
   torqueEvent.deviceType  = KeyboardDeviceType;
   torqueEvent.deviceInst  = 0;
   torqueEvent.objType     = SI_KEY;
   torqueEvent.objInst     = TranslateOSKeyCode(keyCode);
   torqueEvent.modifier    = modifierKeys;
   torqueEvent.ascii       = 0;
   torqueEvent.action      = action;
   torqueEvent.fValue      = fvalue;

}

OSStatus handleTextEvent(EventRef theEvent)
{
   UniChar  *text = NULL;
   UInt32   textLen;
   U32      charCount = 0;

   EventRef rawEvent;
   InputEvent rawTorqueEvent;
   rawTorqueEvent.objInst  = 0;
   rawTorqueEvent.modifier = 0;
   rawTorqueEvent.action   = SI_MAKE;
   rawTorqueEvent.fValue   = 1.0f;
   
   U32 eventKind = GetEventKind(theEvent);
    switch(eventKind)
   {
      case kEventTextInputUnicodeForKeyEvent:
      {
         // get the input string from the text input event
         // first get the number of bytes required
         GetEventParameter( theEvent, kEventParamTextInputSendText, typeUnicodeText, NULL, 0, &textLen, NULL);
         charCount = textLen / sizeof(UniChar);
         text = new UniChar[charCount];

         // now that we've allocated space, get the buffer of text from the input method or keyboard.
         GetEventParameter( theEvent, kEventParamTextInputSendText, typeUnicodeText, NULL, textLen, NULL, text);
         
         // now trap the raw event.
         OSStatus err = 
         GetEventParameter( theEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(EventRef), NULL, &rawEvent);
         if( err==noErr) 
         {
             handleRawKeyEvent(rawEvent,rawTorqueEvent);
         }
         
         break;
      }
      default:  // if we somehow get the wrong kind of event, let someone else handle it.
      {
         return eventNotHandledErr;
      }
   }
   
   
   // make torque events to enter that string.
   InputEvent torqueEvent;
   torqueEvent.deviceType  = KeyboardDeviceType;
   torqueEvent.deviceInst  = 0;
   torqueEvent.objType     = SI_KEY;
   torqueEvent.objInst     = rawTorqueEvent.objInst;
   torqueEvent.modifier    = rawTorqueEvent.modifier;
   torqueEvent.action      = rawTorqueEvent.action;
   torqueEvent.fValue      = rawTorqueEvent.fValue;

   for( int i=0; i < charCount; i++)
   {
      torqueEvent.ascii = text[i];
      Game->postEvent(torqueEvent);
   }

   return noErr;
}

//--------------------------------------
OSStatus DoTextInputCarbonEventHandling(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
   // be sure we have the right event type for this handler.
   U32 eventClass = GetEventClass(theEvent);

   switch(eventClass)
   {
      case kEventClassTextInput:
      {
         return handleTextEvent(theEvent);
         break;
      }
      case kEventClassKeyboard:
      {
         InputEvent keyEvent;
         handleRawKeyEvent( theEvent, keyEvent);

         // This is hackish and requires explanation: 
         // We don't get key-up events as text events, only key-down and key-repeat events. 
         // So we explicitly post key-up events, even if text translation is active.
         if(platState.tsmActive && keyEvent.action != SI_BREAK)
            return eventNotHandledErr;
         
         Game->postEvent(keyEvent);
         return noErr;
         break;
     }
      default:
         return eventNotHandledErr;
   }
   
}


//--------------------------------------
void installCarbonEventHandlers()
{
      EventTargetRef winTarg = GetWindowEventTarget(platState.appWindow);
      // set up the events we want to be notified of for our event handler.
      const int NUM_WIN_EVENT_TYPES = 5 + 3; // mouse+wind
      static EventTypeSpec winEventTypes[NUM_WIN_EVENT_TYPES];
      static EventHandlerUPP winHandlerUPP = NULL;

      const int NUM_APP_EVENT_TYPES = 1 + 3 + 5; // AE+app+dupmouse
      static EventTypeSpec appEventTypes[NUM_APP_EVENT_TYPES];
      static EventHandlerUPP appHandlerUPP = NULL;
      
      const int NUM_TEXT_EVENT_TYPES = 1 + 3; // text + kb
      static EventTypeSpec textEventTypes[NUM_TEXT_EVENT_TYPES];
      static EventHandlerUPP textHandlerUPP = NULL;

      int c = 0;

      if(textHandlerUPP==NULL)
      {
         c = 0;
         textEventTypes[c].eventClass = kEventClassTextInput;
         textEventTypes[c++].eventKind = kEventTextInputUnicodeForKeyEvent;
         // we would split these off to another Do... handler, but I want all keyboard interaction
         // to go through a single control point for the moment.  -paxorr
         textEventTypes[c].eventClass = kEventClassKeyboard;
         textEventTypes[c++].eventKind = kEventRawKeyDown;
         textEventTypes[c].eventClass = kEventClassKeyboard;
         textEventTypes[c++].eventKind = kEventRawKeyUp;
         textEventTypes[c].eventClass = kEventClassKeyboard;
         textEventTypes[c++].eventKind = kEventRawKeyRepeat;

         
         textHandlerUPP = NewEventHandlerUPP(DoTextInputCarbonEventHandling);
      }


      if (winHandlerUPP==NULL)
      {
         c = 0;
//         winEventTypes[c].eventClass = kEventClassKeyboard;
//         winEventTypes[c++].eventKind = kEventRawKeyDown;
//         winEventTypes[c].eventClass = kEventClassKeyboard;
//         winEventTypes[c++].eventKind = kEventRawKeyUp;
//         winEventTypes[c].eventClass = kEventClassKeyboard;
//         winEventTypes[c++].eventKind = kEventRawKeyRepeat;
   // don't do this until we decide we really need it to get raw modifier keys...
   //      eventTypes[c].eventClass = kEventClassKeyboard;
   //      eventTypes[c++].eventKind = kEventRawKeyModifiersChanged;

         winEventTypes[c].eventClass = kEventClassMouse;
         winEventTypes[c++].eventKind = kEventMouseDown;
         winEventTypes[c].eventClass = kEventClassMouse;
         winEventTypes[c++].eventKind = kEventMouseUp;
         winEventTypes[c].eventClass = kEventClassMouse;
         winEventTypes[c++].eventKind = kEventMouseMoved;
         winEventTypes[c].eventClass = kEventClassMouse;
         winEventTypes[c++].eventKind = kEventMouseDragged;
         winEventTypes[c].eventClass = kEventClassMouse;
         winEventTypes[c++].eventKind = kEventMouseWheelMoved;

   // don't need this unless we want special handling.
         winEventTypes[c].eventClass = kEventClassWindow;
         winEventTypes[c++].eventKind = kEventWindowDrawContent;
         winEventTypes[c].eventClass = kEventClassWindow;
         winEventTypes[c++].eventKind = kEventWindowActivated;
         winEventTypes[c].eventClass = kEventClassWindow;
         winEventTypes[c++].eventKind = kEventWindowDeactivated;

         winHandlerUPP = NewEventHandlerUPP(DoWinCarbonEventHandling);
      }
      
      if (appHandlerUPP==NULL)
      {
         c = 0;
         appEventTypes[c].eventClass = kEventClassAppleEvent;
         appEventTypes[c++].eventKind = kEventAppleEvent; // it's always an appleevent

         appEventTypes[c].eventClass = kEventClassApplication;
         appEventTypes[c++].eventKind = kEventAppQuit;
         appEventTypes[c].eventClass = kEventClassApplication;
         appEventTypes[c++].eventKind = kEventAppActivated;
         appEventTypes[c].eventClass = kEventClassApplication;
         appEventTypes[c++].eventKind = kEventAppDeactivated;
         
         // since if in windowed mode, these can all occur outside window bounds, we need to
         // force-propagate the msg.
//         appEventTypes[c].eventClass = kEventClassKeyboard;
//         appEventTypes[c++].eventKind = kEventRawKeyDown;
//         appEventTypes[c].eventClass = kEventClassKeyboard;
//         appEventTypes[c++].eventKind = kEventRawKeyUp;
//         appEventTypes[c].eventClass = kEventClassKeyboard;
//         appEventTypes[c++].eventKind = kEventRawKeyRepeat;
   // don't do this until we decide we really need it to get raw modifier keys...
   //      eventTypes[c].eventClass = kEventClassKeyboard;
   //      eventTypes[c++].eventKind = kEventRawKeyModifiersChanged;

         appEventTypes[c].eventClass = kEventClassMouse;
         appEventTypes[c++].eventKind = kEventMouseDown;
         appEventTypes[c].eventClass = kEventClassMouse;
         appEventTypes[c++].eventKind = kEventMouseUp;
         appEventTypes[c].eventClass = kEventClassMouse;
         appEventTypes[c++].eventKind = kEventMouseMoved;
         appEventTypes[c].eventClass = kEventClassMouse;
         appEventTypes[c++].eventKind = kEventMouseDragged;
         appEventTypes[c].eventClass = kEventClassMouse;
         appEventTypes[c++].eventKind = kEventMouseWheelMoved;

         appHandlerUPP = NewEventHandlerUPP(DoAppCarbonEventHandling);
      }
      
      // this installs into the window -- window doesn't always get all events, such as App events
      InstallEventHandler (winTarg, winHandlerUPP, NUM_WIN_EVENT_TYPES, winEventTypes, NULL, &gWinEventHandlerRef);
      // this installs at the app level.
      InstallEventHandler (GetApplicationEventTarget(), appHandlerUPP, NUM_APP_EVENT_TYPES, appEventTypes, NULL, &gAppEventHandlerRef);
      // this installs a handler for unicode text input
      InstallEventHandler (GetApplicationEventTarget(), textHandlerUPP, NUM_TEXT_EVENT_TYPES, textEventTypes, NULL, &gTextEventHandlerRef);
      
      carbonEventsReady = true;
}

void removeCarbonEventHandlers()
{
   RemoveEventHandler(gWinEventHandlerRef);
   RemoveEventHandler(gAppEventHandlerRef);
   RemoveEventHandler(gTextEventHandlerRef);
}

ConsoleFunction(testFloatingWindowLevel,void,1,1,"")
{
   Con::printf(" Sheilding window level is %x",CGShieldingWindowLevel());
   SetWindowGroupLevel( GetWindowGroupOfClass(kUtilityWindowClass), 1031);
   SetWindowGroupLevel( GetWindowGroupOfClass(kFloatingWindowClass), 1030);   
   SetWindowGroupLevel( GetWindowGroupOfClass(kAlertWindowClass), 1031);
}
