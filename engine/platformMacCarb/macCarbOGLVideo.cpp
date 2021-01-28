//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//
// Portions taken from OpenGL Full Screen.c sample from Apple Computer, Inc.
// (that's where many of the lead helper functions originated from, but code
//  has been significantly changed & revised.)
//-----------------------------------------------------------------------------
#define NO_REDEFINE_GL_FUNCS

#include "platformMacCarb/platformMacCarb.h"
#include "platformMacCarb/platformGL.h"
#include "platformMacCarb/maccarbOGLVideo.h"
#include "console/console.h"
#include "math/mPoint.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "core/fileStream.h"

// !!!!!!!TBD
// Note: Card Profiling code isn't doing anything.
// Note: Screen-res list building is hardcoded, and should tap DSp.
// Note: Gamma support.

#define USE_AGL_FULLSCREEN			1

// whether to enable DSpSetDebugMode
#define DSP_DEBUG_MODE				(defined(TORQUE_DEBUG))
// whether to only capture single monitor under gl fullscreen
#define ALLOW_SINGLEMON_CAPTURE		(defined(TORQUE_DEBUG))

// flip this if we want smooth gamma-fades when switching screenres or in/out of DSp.
// generally, it's less nice but easier to find probs if we just leave it off, even for release.
#define FADE_ON_SWITCH			0

#define CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON   1
#include <DrawSprocket/DrawSprocket.h>
#include <AGL/AGL.h>

AGLContext agl_ctx;

//-----------------------------------------------------------------------------------------
// prototypes and globals -- !!!!!!TBD - globals should mostly go away, into platState. 
//-----------------------------------------------------------------------------------------

void ReportError (char * strError);
OSStatus DSpDebugStr (OSStatus error);
GLenum aglDebugStr (void);

CGrafPtr SetupDSp (GDHandle *phGD, int width, int height, int bpp);
void ShutdownDSp (CGrafPtr pDSpPort);

AGLContext SetupAGL (AGLPixelFormat *inFmt, GDHandle hGD, AGLDrawable win, int bpp);
AGLContext SetupAGLFullScreen (AGLPixelFormat *inFmt, GDHandle hGD, int width, int height, int bpp);

const RGBColor rgbBlack = { 0x0000, 0x0000, 0x0000 };

// extern accessible global!
bool gDSpActive = false;

NumVersion gVersionDSp;
DSpContextAttributes gContextAttributes;
DSpContextReference gContext = 0;
int gCurrentFrequency = 0;

AGLDrawable gpDSpPort = NULL; // will be NULL for full screen under X
Rect gRectPort = {0, 0, 0, 0};
bool gDSpOwnsPort = false;
#define MAX_DEVICES		8	// Geez, this SHOULD be safe!
GDHandle gDeviceList[MAX_DEVICES];
int gNumDevices = 0;
DSpContextReference gBlockedContext[MAX_DEVICES];
int gNumBlockedContexts = 0;

#if defined(TORQUE_DEBUG)
bool gOutlineEnabled = false;

void (* glDrawElementsProcPtr) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) = glDrawElements;
void (* glDrawArraysProcPtr) (GLenum mode, GLint first, GLsizei count) = glDrawArrays;
void (* glNormDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) = glDrawElements;
void (* glNormDrawArrays) (GLenum mode, GLint first, GLsizei count) = glDrawArrays;      
#endif

// locals.
static AGLDrawable   drawable;
static AGLPixelFormat mainfmt;

//-----------------------------------------------------------------------------------------
// display errors -- for the moment, dumps to the console.  !!!!TBD
//-----------------------------------------------------------------------------------------
void ReportError (char * strError)
{
// !!!!!TBD - for now, dump all errors to console.
   Con::errorf(strError);
//   used to do a debug str.  or should do an alert/assert.
}


//-----------------------------------------------------------------------------------------
// translate DSp codes to strings.
//-----------------------------------------------------------------------------------------
OSStatus DSpDebugStr (OSStatus error)
{
   switch (error)
   {
      case noErr:
         break;
      case kDSpNotInitializedErr:
         ReportError ("DSp Error: Not initialized");
         break;
      case kDSpSystemSWTooOldErr:
         ReportError ("DSp Error: system Software too old");
         break;
      case kDSpInvalidContextErr:
         ReportError ("DSp Error: Invalid context");
         break;
      case kDSpInvalidAttributesErr:
         ReportError ("DSp Error: Invalid attributes");
         break;
      case kDSpContextAlreadyReservedErr:
         ReportError ("DSp Error: Context already reserved");
         break;
      case kDSpContextNotReservedErr:
         ReportError ("DSp Error: Context not reserved");
         break;
      case kDSpContextNotFoundErr:
         ReportError ("DSp Error: Context not found");
         break;
      case kDSpFrameRateNotReadyErr:
         ReportError ("DSp Error: Frame rate not ready");
         break;
      case kDSpConfirmSwitchWarning:
//         ReportError ("DSp Warning: Must confirm switch"); // removed since it is just a warning, add back for debugging
         return 0; // don't want to fail on this warning
         break;
      case kDSpInternalErr:
         ReportError ("DSp Error: Internal error");
         break;
      case kDSpStereoContextErr:
         ReportError ("DSp Error: Stereo context");
         break;
   }
   return error;
}


//-----------------------------------------------------------------------------------------
// if error dump agl errors to debugger string, return error
//-----------------------------------------------------------------------------------------
GLenum aglDebugStr (void)
{
   GLenum err = aglGetError();
   if (AGL_NO_ERROR != err)
      ReportError ((char *)aglErrorString(err));
   return err;
}

//-----------------------------------------------------------------------------------------
// if error dump agl errors to debugger string, return error
//-----------------------------------------------------------------------------------------
GLenum glDebugStr (void)
{
	GLenum err = glGetError();
	if (GL_NO_ERROR != err)
		ReportError ((char *)gluErrorString(err));
   return err;
}

#pragma mark -

//-----------------------------------------------------------------------------------------
// release any blocks held
//-----------------------------------------------------------------------------------------
void UnblockDSpContexts(void)
{
   int i;
   if (gNumBlockedContexts) // shouldn't happen, but...
   {
      for (i=0; i<gNumBlockedContexts; i++)
         DSpContext_Release(gBlockedContext[i]);
      gNumBlockedContexts = 0;
   }
}


//-----------------------------------------------------------------------------------------
// setup blocks on contexts on non-selected display device
//-----------------------------------------------------------------------------------------
DisplayIDType BlockDSpContexts(GDHandle hDevice)
{
   int i;
   DisplayIDType mainDid = 0;
   
   if (hDevice)
   {
      OSErr err = noErr;
      DisplayIDType did;
      DSpContextAttributes attrib;
      DSpContextReference dsp;
      for (i=0; i<gNumDevices; i++)
      {
         err = DMGetDisplayIDByGDevice(gDeviceList[i], &did, false);
         if(err) continue;
         if (gDeviceList[i]==hDevice) // skip our preferred device, but cache its did
         {
            mainDid = did;
            continue;
         }
         if (platState.osX==false) // since we don't reserve on OSX.
         {
            err = DSpGetFirstContext(did, &dsp);
            if (err) continue;
            err = DSpContext_GetAttributes(dsp, &attrib);
            if (err) continue;
            err = DSpContext_Reserve(dsp, &attrib);
            if (err) continue;
            // store the blocked-out context for cleanup later.
            gBlockedContext[gNumBlockedContexts] = dsp;
            gNumBlockedContexts++;
         }
      }
   }
   
   return(mainDid);
}

//-----------------------------------------------------------------------------------------
// Does the basic setup and initialization of DSp, returns false if any error state.
//-----------------------------------------------------------------------------------------
bool InitDSp(void)
{
   // check for DSp
   if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpStartup)
   {
      ReportError ("DSp not installed");
      return(false);
   }

   if (noErr != DSpDebugStr (DSpStartup()))
      return false;

   if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpGetVersion) 
   {
      ReportError ("DSp error when checking installed version number");
      return(false);
   }
   else
      gVersionDSp = DSpGetVersion ();

   if ((gVersionDSp.majorRev == 0x01) && (gVersionDSp.minorAndBugRev < 0x99))
   {
      // this version of DrawSprocket is not completely functional on Mac OS X
      if (platState.osX)
      {
	      ReportError ("DSp not properly functional -- upgrade to release build of OSX");
	      return(false);
	  }
   }

   // !!!!tbd assuming no error.
   DSpSetBlankingColor(&rgbBlack);

#if DSP_DEBUG_MODE
   // !!!!tbd assuming no error.
   DSpSetDebugMode(TRUE);
#endif

   // !!!!tbd assuming no error.
   UnblockDSpContexts();
   
   return(true);
}

#if !defined(TORQUE_DEBUG)
//-----------------------------------------------------------------------------------------
// Creates a dummy AGL context, so that naughty objects that call OpenGL before the window
//  exists will not crash the game.
//  If for some reason we fail to get a default contet, assert -- something's very wrong.
//-----------------------------------------------------------------------------------------
void InitDummyAgl(void)
{
   short               i = 0;
   GLint               attrib[64];
   AGLPixelFormat    fmt;
   AGLContext        ctx;

   // clear the existing pixel format.
   if (mainfmt != NULL)
      aglDestroyPixelFormat(mainfmt);
   mainfmt = NULL;
   
   // clear the existing agl context
   if(platState.ctx != NULL)
      aglDestroyContext((AGLContext)platState.ctx);
   platState.ctx = NULL;
   agl_ctx = NULL;

   // set up an attribute array for the pixel format.
   attrib [i++] = AGL_RGBA;         // red green blue and alpha
   attrib [i++] = AGL_DOUBLEBUFFER; // double buffered
   attrib [i++] = AGL_ACCELERATED;  // hardware accelerated format
   attrib [i++] = AGL_NO_RECOVERY;  // don't fall back to a software renderer if out of video card resources
   attrib [i++] = AGL_DEPTH_SIZE;   // depth buffer
   attrib [i++] = 16;               // size for depth buffer - either 16 or 32.
   // BUG: bit depth is ignored for color buffer. fix and test.
   //   attrib [i++] = AGL_PIXEL_SIZE;
   //   attrib [i++] = bpp;
   attrib [i++] = AGL_NONE;            // terminate the list.

   // choose a pixel format that works for any current display device, and matches
   // the attributes above as closely as possible.
   fmt = aglChoosePixelFormat(NULL, 0, attrib); 
   aglDebugStr ();
   AssertFatal(fmt, "Could not find a valid default pixel format");

   // create an agl context. NULL = don't share resouces with any other context.
   ctx = aglCreateContext (fmt, NULL); 
   aglDebugStr ();
   AssertFatal(ctx, "Could not create a default agl context");
   
   // make gl calls go to our dummy context
   if (!aglSetCurrentContext (ctx))
   {
      aglDebugStr ();
      AssertFatal(false,"Could not set a default agl context as the current context.");
   }

   agl_ctx = ctx; // maintain up aglMacro.h context
   mainfmt = fmt;

}
#endif


//-----------------------------------------------------------------------------------------
// Runs through and builds a list of supported modes.
//-----------------------------------------------------------------------------------------
bool OpenGLDevice::enumDisplayModes (GDHandle hDevice)
{
   DSpContextAttributes attr;
   DSpContextReference ctxt;
   OSStatus err;
   int i;
   
   if (!InitDSp()) return(false);
   
   // first, lock out other display devices if we have a preferred device..
   DisplayIDType mainDid = BlockDSpContexts(hDevice);
 
   if (noErr == (err = DSpDebugStr (DSpGetFirstContext(mainDid, &ctxt)) ))
   while (ctxt)
   {
      // extract the attributes of this context, and if we don't have a matching
      // set on our list already, add it to our list.
      if (noErr != (err = DSpDebugStr (DSpContext_GetAttributes(ctxt, &attr)) )) // see what we actually found
      {
         ReportError ("DSpContext_GetAttributes() had an error.");
         break;
      }

      if (attr.displayWidth >= 640 && attr.displayHeight >= 480
      &&  (attr.displayBestDepth == 16 || attr.displayBestDepth == 32) )
      {
         // Only add this resolution if it is not already in the list:
         bool alreadyInList = false;
         for (i=0; i<mResolutionList.size(); i++)
         {
            if (attr.displayWidth == mResolutionList[i].w
            &&  attr.displayHeight == mResolutionList[i].h
            &&  attr.displayBestDepth == mResolutionList[i].bpp )
            {
               alreadyInList = true;
               break;
            }
         }

         if (!alreadyInList)
         {
            Resolution newRes( attr.displayWidth, attr.displayHeight, attr.displayBestDepth );
            mResolutionList.push_back( newRes );
         }
      }
      
      // go on to next context
      if (noErr != (err = DSpGetNextContext(ctxt, &ctxt) )) // don't do the DSpDebugStr here!
      {
         if (err == kDSpContextNotFoundErr) // that's okay
            err = noErr;
         else
         {
            DSpDebugStr (err);
            ReportError ("DSpGetNextContext() had an error.");
         }
	     break;
      }
   }
  
   // clear the blocked contexts now that we're done
   UnblockDSpContexts();

   if (noErr != err)
      return(false);
   
   return(true);
}


//-----------------------------------------------------------------------------------------
// Set up DSp screens, handles multi-monitor correctly
// side effect: sets both gpDSpWindow and gpPort
//-----------------------------------------------------------------------------------------
CGrafPtr SetupDSp (GDHandle *phGD, int width, int height, int bpp)
{
   GDHandle hDevice = NULL;
   DSpContextAttributes foundAttributes;
   DisplayIDType displayID;
   CGrafPtr pPort;
   int i;
   
   if (*phGD)
      hDevice = *phGD; // save it to look for.
   gDSpOwnsPort = false;
   gDSpActive = false;
   
   if (!InitDSp()) return(NULL);

   AssertFatal( bpp!=0, "SetupDSp\nBPP passed in was ZERO!" );
      
   // first, lock out other display devices if we have a preferred device..
   DisplayIDType mainDid = BlockDSpContexts(hDevice);
 
   // Note: DSp < 1.7.3 REQUIRES the back buffer attributes even if only one buffer is required
   dMemset(&gContextAttributes, 0, sizeof (DSpContextAttributes));
   gContextAttributes.displayWidth          = width;
   gContextAttributes.displayHeight         = height;
   gContextAttributes.colorNeeds            = kDSpColorNeeds_Require;
   gContextAttributes.displayBestDepth      = bpp;
   gContextAttributes.backBufferBestDepth   = bpp;
   gContextAttributes.displayDepthMask      = kDSpDepthMask_All;
   gContextAttributes.backBufferDepthMask   = kDSpDepthMask_All;
   gContextAttributes.pageCount             = 1;                        // only the front buffer is needed
   gContextAttributes.frequency             = 0; //try letting DSp pick.

   if (platState.osX && hDevice)
   { // use a different function.
      if (noErr != DSpDebugStr (DSpFindBestContextOnDisplayID(&gContextAttributes, &gContext, mainDid)))
   { // then try 60hz frequency.
         gContextAttributes.frequency           = 60<<16;
         if (noErr != DSpDebugStr (DSpFindBestContextOnDisplayID(&gContextAttributes, &gContext, mainDid)))
         {
            ReportError ("DSpFindBestContextOnDisplayID() had an error.");
            return NULL;
         }
      }
   }
   else
   if (noErr != DSpDebugStr (DSpFindBestContext(&gContextAttributes, &gContext)))
   { // then try 60hz frequency.
      gContextAttributes.frequency           = 60<<16;
      if (noErr != DSpDebugStr (DSpFindBestContext(&gContextAttributes, &gContext)))
      {
         ReportError ("DSpFindBestContext() had an error.");
         return NULL;
      }
   }

   if (noErr != DSpDebugStr (DSpContext_GetAttributes (gContext, &foundAttributes))) // see what we actually found
   {
      ReportError ("DSpContext_GetAttributes() had an error.");
      return NULL;
   }
   
   // clear the blocked contexts now that we're done locating ours.
   UnblockDSpContexts();

   // reset width and height to full screen and handle our own centering
   // HWA will not correctly center less than full screen size contexts
   gContextAttributes.displayWidth    = foundAttributes.displayWidth;
   gContextAttributes.displayHeight    = foundAttributes.displayHeight;
   gContextAttributes.pageCount      = 1;                           // only the front buffer is needed
   gContextAttributes.contextOptions   = 0 | kDSpContextOption_DontSyncVBL;   // no page flipping and no VBL sync needed

   if (noErr !=  DSpDebugStr (DSpContext_GetDisplayID(gContext, &displayID)))    // get our device for future use
   {
      ReportError ("DSpContext_GetDisplayID() had an error.");
      return NULL;
   }
   
   if (noErr !=  DMGetGDeviceByDisplayID (displayID, phGD, false)) // get GDHandle for ID'd device
   {
      ReportError ("DMGetGDeviceByDisplayID() had an error.");
      return NULL;
   }
   
   if (noErr !=  DSpDebugStr (DSpContext_Reserve ( gContext, &gContextAttributes))) // reserve our context
   {
      ReportError ("DSpContext_Reserve() had an error.");
      return NULL;
   }
   
   gCurrentFrequency = foundAttributes.frequency>>16;
#if FADE_ON_SWITCH
   DSpDebugStr (DSpContext_FadeGammaOut (NULL, NULL)); // fade display, remove for debug
#endif
   if (noErr != DSpDebugStr (DSpContext_SetState (gContext, kDSpContextState_Active))) // activate our context
   {
      ReportError ("DSpContext_SetState() had an error.");
      return NULL;
   }
	
   if (platState.osX && !((gVersionDSp.majorRev > 0x01) || ((gVersionDSp.majorRev == 0x01) && (gVersionDSp.minorAndBugRev >= 0x99))))// DSp should be supported in version after 1.98
   {
      ReportError ("Mac OS X with DSp < 1.99 does not support DrawSprocket for OpenGL full screen");
      return NULL;
   }
   else if (platState.osX) // DSp should be supported in versions 1.99 and later
   {
#if originally
/*
      // use DSp's front buffer on Mac OS X
      if (noErr != DSpDebugStr (DSpContext_GetFrontBuffer (gContext, &pPort)))
      {
         ReportError ("DSpContext_GetFrontBuffer() had an error.");
         return NULL;
      }
      // make sure to flag this!
      gDSpOwnsPort = true;
      // there is a problem in Mac OS X GM CoreGraphics that may not size the port pixmap correctly
      // this will check the vertical sizes and offset if required to fix the problem
      // this will not center ports that are smaller then a particular resolution
      {
         long deltaV, deltaH;
         Rect portBounds;
         PixMapHandle hPix = GetPortPixMap (pPort);
         Rect pixBounds = (**hPix).bounds;
         GetPortBounds (pPort, &portBounds);
         deltaV = (portBounds.bottom - portBounds.top) - (pixBounds.bottom - pixBounds.top) +
                  (portBounds.bottom - portBounds.top - height) / 2;
         deltaH = -(portBounds.right - portBounds.left - width) / 2;
         if (deltaV || deltaH)
         {
            GrafPtr pPortSave;
            GetPort (&pPortSave);
            SetPort ((GrafPtr)pPort);
            // set origin to account for CG offset and if requested drawable smaller than screen rez
            SetOrigin (deltaH, deltaV);
            SetPort (pPortSave);
         }
      }
      platState.appWindow = GetWindowFromPort(pPort);
#if FADE_ON_SWITCH
      DSpDebugStr (DSpContext_FadeGammaIn (NULL, NULL));
#endif
      return pPort;
*/
#endif
   }

   Con::printf( "Creating new Fullscreen window..." );
   WindowPtr pWindow = CreateOpenGLWindow(*phGD, width, height, true);
   if (pWindow == NULL)
      return(NULL); // !!!!TBD err msg.
   platState.appWindow = pWindow;
   pPort = GetWindowPort(pWindow);
      
#if FADE_ON_SWITCH
   DSpDebugStr (DSpContext_FadeGammaIn (NULL, NULL));
#endif
   
   gDSpActive = true;
   
   return pPort;
}


//-----------------------------------------------------------------------------------------
// clean up DSp
//-----------------------------------------------------------------------------------------
void ShutdownDSp (CGrafPtr pDSpPort)
{
#if FADE_ON_SWITCH
   DSpContext_FadeGammaOut(NULL, NULL);
#endif
   if ((NULL != pDSpPort))
   {
      // retreive the actual window in question...
      WindowPtr w = GetWindowFromPort(pDSpPort);
      if (w == platState.appWindow) // then clear our storage variable.
         platState.appWindow = NULL;
      if (!gDSpOwnsPort) // then we created the window, so we have to dispose of it.
      {
         if (platState.osX)
            ReleaseWindow(w);
         else
            DisposeWindow(w);
      }
   }
   
   DSpContext_SetState(gContext, kDSpContextState_Inactive);
#if FADE_ON_SWITCH
   DSpContext_FadeGammaIn(NULL, NULL);
#endif

//   ShowCursor(); // !!!!!!TBD - what the heck is this doing here??? this could screw up mouse/cursor management!!!

   DSpContext_Release(gContext);

   if (gNumBlockedContexts) // shouldn't happen, but...
   {
      for (int i=0; i<gNumBlockedContexts; i++)
         DSpContext_Release(gBlockedContext[i]);
      gNumBlockedContexts = 0;
   }

   DSpShutdown();
   
   gContext = NULL;
   gpDSpPort = NULL;
   gDSpOwnsPort = false;
   gDSpActive = false;
}

#pragma mark -

//-----------------------------------------------------------------------------------------
// OpenGL Setup, for any case where we already have a drawable (whether windowed or DSp-screen)
//-----------------------------------------------------------------------------------------
AGLContext SetupAGL (AGLPixelFormat *inFmt, GDHandle hGD, AGLDrawable drawable, int bpp)
{
// software renderer = AGL_RENDERER_ID + AGL_RENDERER_GENERIC_ID + AGL_ALL_RENDERERS
// any renderer = AGL_ALL_RENDERERS instead of AGL_ACCELERATED
// OpenGL compliant ATI renderer = AGL_RENDERER_ID + AGL_RENDERER_ATI_ID + AGL_ACCELERATED

   short               i = 0;
   GLint               attrib[64];
   AGLPixelFormat    fmt;
   AGLContext        ctx;

   if (*inFmt!=NULL)
      aglDestroyPixelFormat(*inFmt); // prev pixel format is no longer needed
   *inFmt = NULL;
   
   attrib [i++] = AGL_RGBA; // red green blue and alpha
   attrib [i++] = AGL_DOUBLEBUFFER; // double buffered
   attrib [i++] = AGL_ACCELERATED; // HWA pixel format only
   attrib [i++] = AGL_NO_RECOVERY; // HWA pixel format only
   attrib [i++] = AGL_DEPTH_SIZE; // MUST request a depth buffer.
   attrib [i++] = bpp>16?32:16; // !!!!!!!TBD this should be from a pref variable.
// !!!!!TBD -- do we need to have a pixelsize in here?
//   attrib [i++] = AGL_PIXEL_SIZE;
//   attrib [i++] = bpp;

//   attrib [i++] = AGL_ALL_RENDERERS; // choose even non-compliant renderers
//   attrib [i++] = AGL_RENDERER_ID; // choose only renderer type specified in next parameter
//   attrib [i++] = AGL_RENDERER_ATI_ID; // ATI renderer
//   attrib [i++] = AGL_RENDERER_GENERIC_ID; // generic renderer

   // terminate the list.
   attrib [i++] = AGL_NONE;

   if (hGD && gNumDevices>1)
      fmt = aglChoosePixelFormat (&hGD, 1, attrib); // get an appropriate pixel format
   else
      fmt = aglChoosePixelFormat(NULL, 0, attrib); // get an appropriate pixel format
   aglDebugStr ();
   if (NULL == fmt) 
   {
      ReportError("Could not find valid pixel format");
      return NULL;
   }

   ctx = aglCreateContext (fmt, NULL); // Create an AGL context
   aglDebugStr ();
   if (NULL == ctx)
   {
      ReportError ("Could not create context");
      return NULL;
   }
   
   if (!aglSetDrawable (ctx, drawable)) // attach the window to the context
   {
      ReportError ("SetDrawable failed");
      aglDebugStr ();
      return NULL;
   }


   if (!aglSetCurrentContext (ctx)) // make the context the current context
   {
      aglDebugStr ();
      aglSetDrawable (ctx, NULL);
      return NULL;
   }
#if !defined(TORQUE_DEBUG)
   agl_ctx = ctx; // maintain up aglMacro.h context
#endif
   *inFmt = fmt;
   return ctx;
}


//-----------------------------------------------------------------------------------------
// OpenGL Setup
AGLContext SetupAGLFullScreen (AGLPixelFormat *inFmt, GDHandle hGD, int width, int height, int bpp)
{
   GLint         attrib[64];
   AGLPixelFormat    fmt;
   AGLContext        ctx;
   
   if (*inFmt!=NULL)
      aglDestroyPixelFormat(*inFmt); // prev pixel format is no longer needed
   *inFmt = NULL;

// different possible pixel format choices for different renderers 
// basics requirements are RGBA and double buffer
// OpenGL will select acclerated context if available

   short i = 0;
   attrib [i++] = AGL_RGBA; // red green blue and alpha
   attrib [i++] = AGL_DOUBLEBUFFER; // double buffered
   attrib [i++] = AGL_ACCELERATED; // HWA pixel format only
   attrib [i++] = AGL_NO_RECOVERY; // HWA pixel format only
   attrib [i++] = AGL_DEPTH_SIZE;
   attrib [i++] = bpp>16?32:16;

   attrib [i++] = AGL_FULLSCREEN;
   attrib [i++] = AGL_PIXEL_SIZE;
   attrib [i++] = bpp;

//   attrib [i++] = AGL_ALL_RENDERERS; // choose even non-compliant renderers
//   attrib [i++] = AGL_RENDERER_ID; // choose only renderer type specified in next parameter
//   attrib [i++] = AGL_RENDERER_ATI_ID; // ATI renderer
//   attrib [i++] = AGL_RENDERER_GENERIC_ID; // generic renderer

   attrib [i++] = AGL_NONE;

   if (hGD && gNumDevices>1)
      fmt = aglChoosePixelFormat (&hGD, 1, attrib); // get an appropriate pixel format
   else
      fmt = aglChoosePixelFormat(NULL, 0, attrib); // get an appropriate pixel format
   if (NULL == fmt) 
   {
      ReportError("Could not find valid pixel format");
      aglDebugStr ();
      return NULL;
   }

   ctx = aglCreateContext (fmt, NULL); // Create an AGL context
   if (NULL == ctx)
   {
      ReportError ("Could not create context");
      aglDebugStr ();
      return NULL;
   }

#ifdef ALLOW_SINGLEMON_CAPTURE
#ifndef AGL_FS_CAPTURE_SINGLE
#define AGL_FS_CAPTURE_SINGLE			255	/* enable the capture of only a single display for aglFullscreen */
#endif
	aglEnable(ctx, AGL_FS_CAPTURE_SINGLE);
#endif

static int hzrate = 0; //gCurrentFrequency; // should be a fairly safe rate on most displays.  we could use DSp to get rates, or DM maybe.

   if (!aglSetFullScreen (ctx, width, height, hzrate, 0)) // attach fulls screen device to the context
   {
      ReportError ("SetFullScreen(0Hz) failed");
      aglDebugStr ();
      
      hzrate = 60;  // safe mode.
      gCurrentFrequency = 60; // so we know what we set...
   if (!aglSetFullScreen (ctx, width, height, hzrate, 0)) // attach fulls screen device to the context
   {
         ReportError ("SetFullScreen(60Hz) failed");
      aglDebugStr ();
      return NULL;
   }
   }


   if (!aglSetCurrentContext (ctx)) // make the context the current context
   {
      ReportError ("SetCurrentContext failed");
      aglDebugStr ();
      aglSetDrawable (ctx, NULL); // turn off full screen
      return NULL;
   }
#if !defined(TORQUE_DEBUG)
   agl_ctx = ctx; // maintain up aglMacro.h context
#endif

//   aglDestroyPixelFormat(fmt); // pixel format is no longer needed
   *inFmt = fmt;

   return ctx;
}


#pragma mark -

//===============================================================================

Boolean sCanDoFullscreen = TRUE;

struct CardProfile
{
   const char *vendor;     // manufacturer
   const char *renderer;   // driver name

   bool safeMode;          // destroy rendering context for resolution change
   bool lockArray;         // allow compiled vertex arrays
   bool subImage;          // allow glTexSubImage*
   bool fogTexture;        // require bound texture for combine extension
   bool noEnvColor;        // no texture environment color
   bool clipHigh;          // clip high resolutions
   bool deleteContext;      // delete rendering context
   bool texCompress;         // allow texture compression
   bool interiorLock;      // lock arrays for Interior render
   bool skipFirstFog;      // skip first two-pass fogging (dumb 3Dfx hack)
   bool only16;            // inhibit 32-bit resolutions
   bool noArraysAlpha;   // don't use glDrawArrays with a GL_ALPHA texture

   const char *proFile;      // explicit profile of graphic settings
};

struct OSCardProfile
{
   const char *vendor;     // manufacturer
   const char *renderer;   // driver name
};

static Vector<CardProfile> sCardProfiles(__FILE__, __LINE__);
static Vector<OSCardProfile> sOSCardProfiles(__FILE__, __LINE__);

struct ProcessorProfile
{
    U16 clock;  // clock range max
    U16 adjust; // CPU adjust   
};

static U8 sNumProcessors = 4;
static ProcessorProfile sProcessorProfiles[] =
{ 
    {  400,  0 },
    {  600,  5 },
    {  800, 10 },
    { 1000, 15 },
};

struct SettingProfile
{
    U16 performance;        // metric range max
    const char *settings;   // default file
};    

static U8 sNumSettings = 3;
static SettingProfile sSettingProfiles[] =
{
    {  33, "LowProfile.cs" },
    {  66, "MediumProfile.cs" },
    { 100, "HighProfile.cs" }, 
};

//------------------------------------------------------------------------------

ConsoleFunction( addCardProfile, void, 16, 16, "(string vendor, string renderer, "
                                               " bool safeMode, bool lockArray, bool subImage, "
                                               " bool fogTexture, bool noEnvColor, bool clipHigh, "
                                               " bool deleteContext, bool texCompress, bool interiorLock, "
                                               " bool skipFirstFog, bool only16, bool noArraysAlpha, "
                                               " string proFile)")
{
   CardProfile profile;

   profile.vendor = dStrdup(argv[1]);
   profile.renderer = dStrdup(argv[2]);

   profile.safeMode = dAtob(argv[3]);   
   profile.lockArray = dAtob(argv[4]);
   profile.subImage = dAtob(argv[5]);
   profile.fogTexture = dAtob(argv[6]);
   profile.noEnvColor = dAtob(argv[7]);
   profile.clipHigh = dAtob(argv[8]);
   profile.deleteContext = dAtob(argv[9]);
   profile.texCompress = dAtob(argv[10]);
   profile.interiorLock = dAtob(argv[11]);
   profile.skipFirstFog = dAtob(argv[12]);
   profile.only16 = dAtob(argv[13]);
   profile.noArraysAlpha = dAtob(argv[14]);

   if (dStrcmp(argv[15],""))
      profile.proFile = dStrdup(argv[15]);
   else
      profile.proFile = NULL;

   sCardProfiles.push_back(profile);
}

ConsoleFunction( addOSCardPRofile, void, 6, 6, "(string vendor, string renderer, "
                                               " bool allowOpenGL, bool allowD3D, "
                                               " bool preferOpenGL) Mac builds ignore the last three flags.")
{
   OSCardProfile profile;

   profile.vendor = dStrdup(argv[1]);
   profile.renderer = dStrdup(argv[2]);

/*
   profile.allowOpenGL = dAtob(argv[3]);
   profile.allowD3D = dAtob(argv[4]);
   profile.preferOpenGL = dAtob(argv[5]);
*/

   sOSCardProfiles.push_back(profile);
}

static void clearCardProfiles()
{
   while (sCardProfiles.size())
   {
      dFree((char *) sCardProfiles.last().vendor);
      dFree((char *) sCardProfiles.last().renderer);

      dFree((char *) sCardProfiles.last().proFile);

      sCardProfiles.decrement();
   }
}

static void clearOSCardProfiles()
{
   while (sOSCardProfiles.size())
   {
      dFree((char *) sOSCardProfiles.last().vendor);
      dFree((char *) sOSCardProfiles.last().renderer);

      sOSCardProfiles.decrement();
   }
}

static void execScript(const char *scriptFile)
{
   // execute the script
   FileStream str;

   if (!str.open(scriptFile, FileStream::Read))
      return;

   U32 size = str.getStreamSize();
   char *script = new char[size + 1];

   str.read(size, script);
   str.close();

   script[size] = 0;
   Con::executef(2, "eval", script);
   delete[] script;
}

static void profileSystem(const char *vendor, const char *renderer)
{
   //Con::executef(2, "exec", "scripts/CardProfiles.cs");
   execScript("CardProfiles.cs");

   const char *os = NULL;
   char osProfiles[64];

#pragma message("todo: implement full profile checking")
   os = "MACCARB";
   
   if ( os != NULL )
   {
      dSprintf(osProfiles,64,"%s%sCardProfiles.cs","PPC",os);
      //Con::executef(2, "exec", osProfiles);
      execScript(osProfiles);
   }

   const char *proFile = NULL;
   U32 i;
   
   for (i = 0; i < sCardProfiles.size(); ++i)
      if (dStrstr(vendor, sCardProfiles[i].vendor) &&
          (!dStrcmp(sCardProfiles[i].renderer, "*") ||
           dStrstr(renderer, sCardProfiles[i].renderer)))
      {         
         Con::setBoolVariable("$pref::Video::safeModeOn", sCardProfiles[i].safeMode);
         Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", !sCardProfiles[i].lockArray);
         Con::setBoolVariable("$pref::OpenGL::disableSubImage", !sCardProfiles[i].subImage);
         Con::setBoolVariable("$pref::TS::fogTexture", sCardProfiles[i].fogTexture);
         Con::setBoolVariable("$pref::OpenGL::noEnvColor", sCardProfiles[i].noEnvColor);
         Con::setBoolVariable("$pref::Video::clipHigh", sCardProfiles[i].clipHigh);
         if (!sCardProfiles[i].deleteContext)
         {
               Con::setBoolVariable("$pref::Video::deleteContext", false);
/*
            // HACK: The Voodoo3/5 on W2K crash when deleting a rendering context
            // So we're not deleting it.
            // Oh, and the Voodoo3 returns a Banshee renderer string under W2K
            dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
            OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
            if ( GetVersionEx( &OSVersionInfo ) &&
                 OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
                 OSVersionInfo.dwMajorVersion == 5)
               Con::setBoolVariable("$pref::Video::deleteContext", false);
            else
               Con::setBoolVariable("$pref::Video::deleteContext", true);
*/
         }
         else
            Con::setBoolVariable("$pref::Video::deleteContext", true);
         Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", !sCardProfiles[i].texCompress);
         Con::setBoolVariable("$pref::Interior::lockArrays", sCardProfiles[i].interiorLock);
         Con::setBoolVariable("$pref::TS::skipFirstFog", sCardProfiles[i].skipFirstFog);
         Con::setBoolVariable("$pref::Video::only16", sCardProfiles[i].only16);
         Con::setBoolVariable("$pref::OpenGL::noDrawArraysAlpha", sCardProfiles[i].noArraysAlpha);

         proFile = sCardProfiles[i].proFile;

         break;   
      }

   // defaults
   U16 glProfile;

   if (!proFile)
   {
      // no driver GL profile -- make one via weighting GL extensions
      glProfile = 25;

      glProfile += gGLState.suppARBMultitexture * 25;
      glProfile += gGLState.suppLockedArrays * 15;
      glProfile += gGLState.suppVertexArrayRange * 10;
      glProfile += gGLState.suppTextureEnvCombine * 5;
      glProfile += gGLState.suppPackedPixels * 5;
      glProfile += gGLState.suppTextureCompression * 5;
      glProfile += gGLState.suppS3TC * 5;
      glProfile += gGLState.suppFXT1 * 5;

      Con::setBoolVariable("$pref::Video::safeModeOn", true);
      Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", false);
      Con::setBoolVariable("$pref::OpenGL::disableSubImage", false); 
      Con::setBoolVariable("$pref::TS::fogTexture", false);
      Con::setBoolVariable("$pref::OpenGL::noEnvColor", false);
      Con::setBoolVariable("$pref::Video::clipHigh", false);
      Con::setBoolVariable("$pref::Video::deleteContext", true);
      Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", false);
      Con::setBoolVariable("$pref::Interior::lockArrays", true);
      Con::setBoolVariable("$pref::TS::skipFirstFog", false);
      Con::setBoolVariable("$pref::Video::only16", false);
      Con::setBoolVariable("$pref::OpenGL::noDrawArraysAlpha", false);
   }

   Con::setVariable("$pref::Video::profiledVendor", vendor);
   Con::setVariable("$pref::Video::profiledRenderer", renderer);

   if (!Con::getBoolVariable("$DisableSystemProfiling") &&
       ( dStrcmp(vendor, Con::getVariable("$pref::Video::defaultsVendor")) ||
          dStrcmp(renderer, Con::getVariable("$pref::Video::defaultsRenderer")) ))
   {
      if (proFile)
      {
         char settings[64];

         dSprintf(settings,64,"%s.cs",proFile);
         //Con::executef(2, "exec", settings);
         execScript(settings);
      }
      else
      {
         U16 adjust;

         // match clock with profile
         for (i = 0; i < sNumProcessors; ++i)
         {
            adjust = sProcessorProfiles[i].adjust;

            if (Platform::SystemInfo.processor.mhz < sProcessorProfiles[i].clock) break;
         }

         const char *settings;

         // match performance metric with profile
         for (i = 0; i < sNumSettings; ++i)
         {
            settings = sSettingProfiles[i].settings;

            if (glProfile+adjust <= sSettingProfiles[i].performance) break;
         }

         //Con::executef(2, "exec", settings);
         execScript(settings);
      }

      bool match = false;

      for (i = 0; i < sOSCardProfiles.size(); ++i)
         if (dStrstr(vendor, sOSCardProfiles[i].vendor) &&
             (!dStrcmp(sOSCardProfiles[i].renderer, "*") ||
              dStrstr(renderer, sOSCardProfiles[i].renderer)))
         {

//            Con::setBoolVariable("$pref::Video::allowOpenGL", sOSCardProfiles[i].allowOpenGL);
//            Con::setBoolVariable("$pref::Video::allowD3D", sOSCardProfiles[i].allowD3D);
//            Con::setBoolVariable("$pref::Video::preferOpenGL", sOSCardProfiles[i].preferOpenGL);
         
            match = true;

            break;
         }

      if (!match)
      {
         Con::setBoolVariable("$pref::Video::allowOpenGL", true);
//         Con::setBoolVariable("$pref::Video::allowD3D", true);
         Con::setBoolVariable("$pref::Video::preferOpenGL", true);
      }

      Con::setVariable("$pref::Video::defaultsVendor", vendor);
      Con::setVariable("$pref::Video::defaultsRenderer", renderer);
   }

   // write out prefs
   gEvalState.globalVars.exportVariables("$pref::*", "prefs/ClientPrefs.cs", false);

   clearCardProfiles();
   clearOSCardProfiles();
}    

#pragma mark -

//------------------------------------------------------------------------------
OpenGLDevice::OpenGLDevice()
{
   initDevice();
}


//------------------------------------------------------------------------------
void OpenGLDevice::initDevice()
{
   // Set the device name:
   mDeviceName = "OpenGL";

   // Never unload a code module
   aglConfigure(AGL_RETAIN_RENDERERS, GL_TRUE);

#if defined(TORQUE_OS_MAC_OSX) && defined(AGL_TARGET_OS_MAC_OSX)
   {// carbon porting guide says we MUST do this in mach-o Carbon code...
      aglConfigure(AGL_TARGET_OS_MAC_OSX, GL_TRUE);
   }
#endif

   // Set some initial conditions:
   mResolutionList.clear();
// sCanDoFullscreen = FALSE;

   bool gotEm = enumDisplayModes(platState.hDisplay);
   if (!gotEm)
   {
      mResolutionList.clear();
      // fill it with some standard sizes for a machine, just to have something.
      Point sizes[4] = {{640,480},{800,600},{1024,768},{1280,1024}};
      for(int i=0; i<4; i++)
      {
         Resolution newRes16( sizes[i].h, sizes[i].v, 16 );
         Resolution newRes32( sizes[i].h, sizes[i].v, 32 );
         mResolutionList.push_back( newRes16 );
         mResolutionList.push_back( newRes32 );
      }
   }
}


//------------------------------------------------------------------------------
bool OpenGLDevice::activate( U32 width, U32 height, U32 bpp, bool fullScreen )
{
   Con::printf( "Activating the OpenGL display device..." );

   bool needResurrect = false;

#pragma message("todo: need to error check on return from agl funcs.")

  // If the rendering context exists, delete it:
/*
   if (platState.ctx)
   {
      Con::printf( "Killing the texture manager..." );
      Game->textureKill();
      needResurrect = true;

      Con::printf( "Making the rendering context not current..." );
      aglSetCurrentContext(NULL);
      aglSetDrawable(platState.ctx, NULL);

      Con::printf( "Deleting the rendering context..." );
      aglDestroyContext(platState.ctx);
      platState.ctx = NULL;
   }
*/
   static bool onceAlready = false;
   bool profiled = false;

   // Set the resolution:
   if ( !setScreenMode( width, height, bpp, ( fullScreen || mFullScreenOnly ), true, false ) )
      return false;

   // Get original gamma ramp
//   mRestoreGamma = GetDeviceGammaRamp(platState.appDC, mOriginalRamp);

   // Output some driver info to the console:
   const char* vendorString   = (const char*) glGetString( GL_VENDOR );
   const char* rendererString = (const char*) glGetString( GL_RENDERER );
   const char* versionString  = (const char*) glGetString( GL_VERSION );
   Con::printf( "OpenGL driver information:" );
   if ( vendorString )
      Con::printf( "  Vendor: %s", vendorString );
   if ( rendererString )
      Con::printf( "  Renderer: %s", rendererString );
   if ( versionString )
      Con::printf( "  Version: %s", versionString );

   if ( needResurrect )
   {
      // Reload the textures:
      Con::printf( "Resurrecting the texture manager..." );
      Game->textureResurrect();
   }

   //Now that we have video setup, and window established, we do any initialization/checking
   // of OpenGL interfaces necessary to setup the rendering system.
   GL_EXT_Init();

   Con::setVariable( "$pref::Video::displayDevice", mDeviceName );

/* !!!!!!TBD LATER!!!!!!!!
   // only do this for the first session
   if (!profiled &&
       !Con::getBoolVariable("$DisableSystemProfiling") &&
       (   dStrcmp(vendorString, Con::getVariable("$pref::Video::profiledVendor")) ||
          dStrcmp(rendererString, Con::getVariable("$pref::Video::profiledRenderer")) ))
   {
      profileSystem(vendorString, rendererString);
      profiled = true;
   }

   if (profiled)
   {      
      U32 width, height, bpp;

      if (Con::getBoolVariable("$pref::Video::clipHigh", false))
         for (S32 i = mResolutionList.size()-1; i >= 0; --i)
            if (mResolutionList[i].w > 1152 || mResolutionList[i].h > 864)
               mResolutionList.erase(i);

      if (Con::getBoolVariable("$pref::Video::only16", false))
         for (S32 i = mResolutionList.size()-1; i >= 0; --i)
            if (mResolutionList[i].bpp == 32)
               mResolutionList.erase(i);
      
      dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &width, &height, &bpp);
      setScreenMode(width, height, bpp,
                    Con::getBoolVariable("$pref::Video::fullScreen", true), false, false);
   }
*/

   // Do this here because we now know about the extensions:
//   if ( gGLState.suppSwapInterval )
      setVerticalSync( !Con::getBoolVariable( "$pref::Video::disableVerticalSync" ) );
   Con::setBoolVariable("$pref::OpenGL::allowTexGen", true);

   return true;
}


//------------------------------------------------------------------------------
// returns TRUE if textures need resurrecting in future...
//------------------------------------------------------------------------------
bool OpenGLDevice::GLCleanupHelper()
{
   bool needResurrect = false;
   
   Con::printf( "Cleaning up the display device..." );

   // !!!!!TBD
   // NOTE THAT A LARGE CHUNK OF THIS CODE IS SHARED WITH shutdown(), AND THUS SHOULD BE >REALLY< SHARED CODE.
   // Delete the rendering context:
   if (platState.ctx)
   {
      if (!Video::smNeedResurrect) // as that flag means we already ditched textures...
      {
         Con::printf( "Killing the texture manager..." );
         Game->textureKill();
         needResurrect = true;
      }

      Con::printf( "Making sure the rendering context is not current..." );
      aglSetCurrentContext(NULL);
#if !defined(TORQUE_DEBUG)
   agl_ctx = NULL; // maintain up aglMacro.h context
#endif

      aglSetDrawable((AGLContext)platState.ctx, NULL);
      if (0) // !!!!TBD error check
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode\naglSetDrawable( ctx, NULL ) failed!" );
         return false;
      }

      Con::printf( "Deleting the rendering context..." );
      // !!!! TBD WHY IS THIS CHECK HERE??? IS IT VALID ON MAC?? WHY NOT IN ACTIVATE???
//    if ( Con::getBoolVariable("$pref::Video::deleteContext",true) )
         aglDestroyContext((AGLContext)platState.ctx);
      if (0) // !!!!TBD error check
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode\naglDestroyContext failed!" );
         return false;
      }

      // make sure it is null...
      platState.ctx = NULL;
   }

// !!!!!TBD HANDLE GAMMA
//      if (mRestoreGamma)
//         SetDeviceGammaRamp(platState.appDC, mOriginalRamp);

   if (gpDSpPort) // == smIsFullScreen...
   {
      Con::printf("Shutting down DrawSprocket and restoring Desktop (%dx%dx%d)...", platState.desktopWidth, platState.desktopHeight, platState.desktopBitsPixel );
      ShutdownDSp(gpDSpPort);
   }

   // on the mac, if the window is still around for any reason, dispose of it now.
   // DestroyGL or ShutdownDSp will have nuked it already if they created it...
   if ( platState.appWindow )
   {
      Con::printf( "Destroying the window..." );
      if (platState.osX)
         ReleaseWindow(platState.appWindow);
      else
         DisposeWindow(platState.appWindow);
      platState.appWindow = NULL;
   }
   
   // clear some Res state, so we don't think we're set up already.
   smCurrentRes.w = 0;
   smCurrentRes.h = 0;
   
   return(needResurrect);
}


//------------------------------------------------------------------------------
void OpenGLDevice::shutdown()
{
   OSStatus err = noErr;
   
   Con::printf( "Shutting down the OpenGL display device..." );

   // call our new universal helper function...
   GLCleanupHelper();

   // should be safe to always call.
   ShowMenuBar();
}


//------------------------------------------------------------------------------
// This is the real workhorse function of the DisplayDevice...
//
static bool wasWindowed = true;
bool OpenGLDevice::setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint )
{
   // SANITY CHECK, AS THE FRICKING SCRIPTS PASS IN BAD VALUES.  LIKE ZERO.
   if (!bpp)
      bpp = platState.desktopBitsPixel;

   WindowPtr curtain = NULL;
   char errorMessage[256];
   bool newFullScreen = fullScreen;
   bool safeModeOn = Con::getBoolVariable( "$pref::Video::safeModeOn" );
   bool needResurrect = false;
   Resolution newRes( width, height, bpp );
   Resolution oldRes = smCurrentRes;
   OSStatus err;

   // !!!!!TBD
   // if no values changing, return immediately.
   // this is sort-of an optimization, but also useful to prevent thrashing (which the
   // current graphic startup would do otherwise...)
   if (smCurrentRes.w == width && smCurrentRes.h == height && smCurrentRes.bpp == bpp && smIsFullScreen == newFullScreen)
      return(true);

   if ( !newFullScreen && mFullScreenOnly )
   {
      Con::warnf( ConsoleLogEntry::General, "OpenGLDevice::setScreenMode - device or desktop color depth does not allow windowed mode!" );
      newFullScreen = true;
   }



#pragma message("todo: how to handle picking available resolutions?")

// !!!!!!!TBD NOT SURE THAT WOULD DO THE RIGHT THING FOR MAC IN ALL CASES!
// THEREFORE, ALWAYS GO DO SHUTDOWN CODE!!!
   needResurrect = GLCleanupHelper();

   // MAC DOESN'T NEED A CURTAIN -- prefers to use gamma fade out/in...
//   curtain = CreateCurtain( newRes.w, newRes.h );

   Con::printf( ">> Attempting to change display settings to %s %dx%dx%d...", newFullScreen?"fullscreen":"windowed", newRes.w, newRes.h, newRes.bpp );

   if ( newFullScreen )
      smIsFullScreen = true;
   else if ( smIsFullScreen )
      smIsFullScreen = false;
   // WRITE TO PREFS
   Con::setBoolVariable( "$pref::Video::fullScreen", smIsFullScreen );

   // set our preferred device
   // default is the main device (which should be the one at [0,0]).
   GDHandle currDev = GetMainDevice();
   int devicenum = Con::getIntVariable("$pref::Video::monitorNum", 0);
   if (devicenum)
   { // then find the right device.
      if (devicenum>gNumDevices || devicenum<1)
      {
         // well, then give them default device
         devicenum = 0;
         Con::setIntVariable("$pref::Video::monitorNum", devicenum);
      }
      else
         currDev = gDeviceList[devicenum-1];
   }
   platState.hDisplay = currDev;

// in case we fail, put back values for the moment, as we munged them in Cleanup.
   smCurrentRes = oldRes;
   bool aglReady = false;
   if (newFullScreen)
   {
#if USE_AGL_FULLSCREEN
      if (platState.osX) // under osx we can try SetaglFullScreen
      {
         short display = devicenum; // !!!! IS THIS CORRECT? !!!!TBD -- need to define a GDHandle to display # mapping
         if (platState.ctx = SetupAGLFullScreen (&mainfmt, platState.hDisplay, newRes.w, newRes.h, newRes.bpp)) // Setup the OpenGL context
         {
            aglReady = true;
            SetRect (&gRectPort, 0, 0, newRes.w, newRes.h); // l, t, r, b
            Con::printf("Successfully set up AGLFullscreen support [%d x %d x %d : %dHz]", newRes.w, newRes.h, newRes.bpp, gCurrentFrequency);
         }
         else
         {
            Con::printf("Failed to set up AGLFullscreen support.");
         }
      }
#endif

      if (!aglReady) // not ready yet.  try normal SetupAGL.
      {
         gpDSpPort = SetupDSp (&(platState.hDisplay), newRes.w, newRes.h, newRes.bpp); // Setup DSp for OpenGL
         if (gpDSpPort==NULL) // oops
         {
            Con::warnf("Failed to establish DrawSprocket monitor control.");
            return(false);
         }

         if (platState.ctx = SetupAGL(&mainfmt, platState.hDisplay, gpDSpPort, newRes.bpp)) // Setup the OpenGL context
         {
            GetPortBounds (gpDSpPort, &gRectPort);
            if (platState.hDisplay)
               Con::printf("Successfully set up AGL support with DSp Fullscreen [%d x %d x %d : %dHz]", gRectPort.right - gRectPort.left, gRectPort.bottom - gRectPort.top, (**(**platState.hDisplay).gdPMap).pixelSize, gCurrentFrequency);
            else
               Con::printf("Successfully set up GL support [%d x %d]", gRectPort.right - gRectPort.left, gRectPort.bottom - gRectPort.top);
         }
         else
         {
            Con::printf("Failed to set up AGL context support.");
            return(false);
         }
      }
   }

   //if we haven't yet created/assigned a plat window, need to do so now.
   if (!platState.appWindow )
   {
      platState.appWindow = CreateOpenGLWindow( platState.hDisplay, newRes.w, newRes.h, false );
      if ( !platState.appWindow )
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode - Failed to create a new window!" );
         return false;
      }
      else
         Con::printf( "Created new %swindow for GL [%d x %d].", newFullScreen ? "Fullscreen " : "", newRes.w, newRes.h );
   }

   if (!newFullScreen)
   {
      CGrafPtr port = GetWindowPort(platState.appWindow);
      AGLDrawable drawto = port; // just to be clear.
      GetPortBounds (port, &gRectPort);
      if (NULL == (platState.ctx = SetupAGL(&mainfmt, platState.hDisplay, drawto, newRes.bpp))) // Setup the OpenGL context
      {
         Con::warnf("Failed to set up AGL windowed support.");
         return false;
      }
      Con::printf("Successfully set up AGL windowed support [%d x %d]", gRectPort.right - gRectPort.left, gRectPort.bottom - gRectPort.top);
   }

//PRINT BACK THE FORMAT WE GOT!!!! !!!!TBD
//qwglDescribePixelFormat( platState.appDC, chosenFormat, sizeof( pfd ), &pfd );
//Con::printf( "Pixel format set:" );
//Con::printf( "  %d color bits, %d depth bits, %d stencil bits", platState.fmt.cColorBits, pfd.cDepthBits, pfd.cStencilBits );

   // validate that this function is available in CFM on OSX...
   if ((Ptr) kUnresolvedCFragSymbolAddress != (Ptr) glPixelStorei) 
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if ( needResurrect )
   {
      // Reload the textures:
      Con::printf( "Resurrecting the texture manager..." );
      Game->textureResurrect();
   }

//   if ( gGLState.suppSwapInterval ) // mac always supports this, basically.
      setVerticalSync( !Con::getBoolVariable( "$pref::Video::disableVerticalSync" ) );

   smCurrentRes = newRes;
   // !!!TBD
   // this is kinda redundant in the mac schema...
   Platform::setWindowSize( newRes.w, newRes.h );
   
   // WRITE TO PREFS
   char tempBuf[15];
   if (smIsFullScreen)
   {
	   dSprintf( tempBuf, sizeof( tempBuf ), "%d %d %d", smCurrentRes.w, smCurrentRes.h, smCurrentRes.bpp );
	   Con::setVariable( "$pref::Video::resolution", tempBuf );
	   if (wasWindowed)
		   HideMenuBar();
	   wasWindowed = false;
   }
   else // windowed
   {
	   dSprintf( tempBuf, sizeof( tempBuf ), "%d %d", smCurrentRes.w, smCurrentRes.h );
	   Con::setVariable( "$pref::Video::windowedRes", tempBuf );
	   wasWindowed = true;
	   ShowMenuBar(); // should be safe to always call...
   }

//   if ( curtain )
//      DisposeWindow( curtain );

   if ( repaint )
      Con::evaluate( "resetCanvas();" );

   return true;
}


//------------------------------------------------------------------------------
void OpenGLDevice::swapBuffers()
{
   // TBD!!!!! should this be setting to platState.ctx
   // in future, support multiple contexts?
#if later //!!!
   //dhc - also adding in a sanity check here.
   AssertISV((AGLContext)platState.ctx, "OpenGLDevice::swapBuffers -- No GL context in place!");
#endif
   if (platState.ctx)
      aglSwapBuffers(aglGetCurrentContext()); 
#if defined(TORQUE_DEBUG)
   if(gOutlineEnabled)
      glClear(GL_COLOR_BUFFER_BIT);
#endif
}  


//------------------------------------------------------------------------------
const char* OpenGLDevice::getDriverInfo()
{
   // Output some driver info to the console:
   const char* vendorString   = (const char*) glGetString( GL_VENDOR );
   const char* rendererString = (const char*) glGetString( GL_RENDERER );
   const char* versionString  = (const char*) glGetString( GL_VERSION );
   const char* extensionsString = (const char*) glGetString( GL_EXTENSIONS );
   
   U32 bufferLen = ( vendorString ? dStrlen( vendorString ) : 0 )
                 + ( rendererString ? dStrlen( rendererString ) : 0 )
                 + ( versionString  ? dStrlen( versionString ) : 0 )
                 + ( extensionsString ? dStrlen( extensionsString ) : 0 )
                 + 4;

   char* returnString = Con::getReturnBuffer( bufferLen );
   dSprintf( returnString, bufferLen, "%s\t%s\t%s\t%s", 
         ( vendorString ? vendorString : "" ),
         ( rendererString ? rendererString : "" ),
         ( versionString ? versionString : "" ),
         ( extensionsString ? extensionsString : "" ) );
   
   return( returnString );   
}


//------------------------------------------------------------------------------
bool OpenGLDevice::getGammaCorrection(F32 &g)
{
   U16 ramp[256*3];

#pragma message("todo: gamma")
/*
   if (!GetDeviceGammaRamp(platState.appDC, ramp))
      return false;
*/

   F32 csum = 0.0;
   U32 ccount = 0;

   for (U16 i = 0; i < 256; ++i)
   {
      if (i != 0 && ramp[i] != 0 && ramp[i] != 65535)
      {
         F64 b = (F64) i/256.0;
         F64 a = (F64) ramp[i]/65535.0;
         F32 c = (F32) (mLog(a)/mLog(b));
         
         csum += c;
         ++ccount;  
      }
   }
   g = csum/ccount;

   return true;         
}    

//------------------------------------------------------------------------------
bool OpenGLDevice::setGammaCorrection(F32 g)
{
   U16 ramp[256*3];

   for (U16 i = 0; i < 256; ++i)
      ramp[i] = mPow((F32) i/256.0f, g) * 65535.0f;
   dMemcpy(&ramp[256],ramp,256*sizeof(U16));
   dMemcpy(&ramp[512],ramp,256*sizeof(U16));      

   bool t = false;
#pragma message("todo: gamma")
//   t = SetDeviceGammaRamp(platState.appDC, ramp);
   return t;
}

//------------------------------------------------------------------------------
bool OpenGLDevice::setVerticalSync( bool on )
{
   // Apple has an internal vsync flag:
   if (platState.ctx)
   {
      return(GL_FALSE != aglSetInteger((AGLContext)platState.ctx, AGL_SWAP_INTERVAL, &(GLint)on));
   }
   return(false);
}

//------------------------------------------------------------------------------
DisplayDevice* OpenGLDevice::create()
{
   bool result = true;
   bool fullScreenOnly = false;

   // validate OpenGL existance/linkage here for now.
   if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) glBegin
   ||  (Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat)
   {
      ReportError ("OpenGL not installed");
      return NULL;
   }   
   
   if (!InitDSp())
      return(NULL);

#if !defined(TORQUE_DEBUG)
   // set up a dummy default agl context
   // this will be replaced later with the window's context, but
   // since we use aglMacro.h in release builds, we need agl_ctx to be valid at any time.
   InitDummyAgl();
#endif


   // go find all the screen devices, in device order.
   GDHandle hDevice; 
   gNumDevices = 0;
   // always at least one screen device...
   hDevice = DMGetFirstScreenDevice (true);                        // check number of screens
   gDeviceList[gNumDevices++] = hDevice;
   while(hDevice = DMGetNextScreenDevice (hDevice, true))
	   gDeviceList[gNumDevices++] = hDevice;

   // set our preferred device
   // default is the main device (which should be the one at [0,0]).
   GDHandle currDev = GetMainDevice();
   int devicenum = Con::getIntVariable("$pref::Video::monitorNum", 0);
   if (devicenum)
   { // then find the right device.
      if (devicenum>gNumDevices || devicenum<1)
      {
         // well, then give them default device
         devicenum = 0;
         Con::setIntVariable("$pref::Video::monitorNum", devicenum);
      }
      else
         currDev = gDeviceList[devicenum-1];
   }
   platState.hDisplay = currDev;

#pragma message("todo: ::create() full gl acceleration/availability test")

   if ( result )
   {
      OpenGLDevice* newOGLDevice = new OpenGLDevice();
      if ( newOGLDevice )
      {
         newOGLDevice->mFullScreenOnly = fullScreenOnly;
         return (DisplayDevice*) newOGLDevice;
      }
      else 
         return NULL;
   }
   else
      return NULL;
}

#if defined(TORQUE_DEBUG)
static U32 getIndex(GLenum type, const void *indices, U32 i)
{
   if(type == GL_UNSIGNED_BYTE)
      return ((U8 *) indices)[i];
   else if(type == GL_UNSIGNED_SHORT)
      return ((U16 *) indices)[i];
   else
      return ((U32 *) indices)[i];
}

void glOutlineDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{ 
   if(mode == GL_POLYGON)
      mode = GL_LINE_LOOP;

   if(mode == GL_POINTS || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP || mode == GL_LINES)   
      glDrawElements( mode, count, type, indices );
   else
   {
      glBegin(GL_LINES);
      if(mode == GL_TRIANGLE_STRIP)
      {
         U32 i;
         for(i = 0; i < count - 1; i++)
         {
            glArrayElement(getIndex(type, indices, i));
            glArrayElement(getIndex(type, indices, i + 1));
            if(i + 2 != count)
            {
               glArrayElement(getIndex(type, indices, i));
               glArrayElement(getIndex(type, indices, i + 2));
            }
         }
      }
      else if(mode == GL_TRIANGLE_FAN)
      {
         for(U32 i = 1; i < count; i ++)
         {
            glArrayElement(getIndex(type, indices, 0));
            glArrayElement(getIndex(type, indices, i));
            if(i != count - 1)
            {
               glArrayElement(getIndex(type, indices, i));
               glArrayElement(getIndex(type, indices, i + 1));
            }
         }
      }
      else if(mode == GL_TRIANGLES)
      {
         for(U32 i = 3; i <= count; i += 3)
         {
            glArrayElement(getIndex(type, indices, i - 3));
            glArrayElement(getIndex(type, indices, i - 2));
            glArrayElement(getIndex(type, indices, i - 2));
            glArrayElement(getIndex(type, indices, i - 1));
            glArrayElement(getIndex(type, indices, i - 3));
            glArrayElement(getIndex(type, indices, i - 1));
         }
      }
      else if(mode == GL_QUADS)
      {
         for(U32 i = 4; i <= count; i += 4)
         {
            glArrayElement(getIndex(type, indices, i - 4));
            glArrayElement(getIndex(type, indices, i - 3));
            glArrayElement(getIndex(type, indices, i - 3));
            glArrayElement(getIndex(type, indices, i - 2));
            glArrayElement(getIndex(type, indices, i - 2));
            glArrayElement(getIndex(type, indices, i - 1));
            glArrayElement(getIndex(type, indices, i - 4));
            glArrayElement(getIndex(type, indices, i - 1));
         }
      }
      else if(mode == GL_QUAD_STRIP)
      {
         if(count < 4)
            return;
         glArrayElement(getIndex(type, indices, 0));
         glArrayElement(getIndex(type, indices, 1));
         for(U32 i = 4; i <= count; i += 2)
         {
            glArrayElement(getIndex(type, indices, i - 4));
            glArrayElement(getIndex(type, indices, i - 2));

            glArrayElement(getIndex(type, indices, i - 3));
            glArrayElement(getIndex(type, indices, i - 1));

            glArrayElement(getIndex(type, indices, i - 2));
            glArrayElement(getIndex(type, indices, i - 1));
         }
      }
      glEnd();
   }
}

void glOutlineDrawArrays(GLenum mode, GLint first, GLsizei count) 
{ 
   if(mode == GL_POLYGON)
      mode = GL_LINE_LOOP;

   if(mode == GL_POINTS || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP || mode == GL_LINES)   
      glDrawArrays( mode, first, count );
   else
   {
      glBegin(GL_LINES);
      if(mode == GL_TRIANGLE_STRIP)
      {
         U32 i;
         for(i = 0; i < count - 1; i++) 
         {
            glArrayElement(first + i);
            glArrayElement(first + i + 1);
            if(i + 2 != count)
            {
               glArrayElement(first + i);
               glArrayElement(first + i + 2);
            }
         }
      }
      else if(mode == GL_TRIANGLE_FAN)
      {
         for(U32 i = 1; i < count; i ++)
         {
            glArrayElement(first);
            glArrayElement(first + i);
            if(i != count - 1)
            {
               glArrayElement(first + i);
               glArrayElement(first + i + 1);
            }
         }
      }
      else if(mode == GL_TRIANGLES)
      {
         for(U32 i = 3; i <= count; i += 3)
         {
            glArrayElement(first + i - 3);
            glArrayElement(first + i - 2);
            glArrayElement(first + i - 2);
            glArrayElement(first + i - 1);
            glArrayElement(first + i - 3);
            glArrayElement(first + i - 1);
         }
      }
      else if(mode == GL_QUADS)
      {
         for(U32 i = 4; i <= count; i += 4)
         {
            glArrayElement(first + i - 4);
            glArrayElement(first + i - 3);
            glArrayElement(first + i - 3);
            glArrayElement(first + i - 2);
            glArrayElement(first + i - 2);
            glArrayElement(first + i - 1);
            glArrayElement(first + i - 4);
            glArrayElement(first + i - 1);
         }
      }
      else if(mode == GL_QUAD_STRIP)
      {
         if(count < 4)
            return;
         glArrayElement(first + 0);
         glArrayElement(first + 1);
         for(U32 i = 4; i <= count; i += 2)
         {
            glArrayElement(first + i - 4);
            glArrayElement(first + i - 2);

            glArrayElement(first + i - 3);
            glArrayElement(first + i - 1);

            glArrayElement(first + i - 2);
            glArrayElement(first + i - 1);
         }
      }
      glEnd();
   }
}


ConsoleFunction(GLEnableOutline,void,2,2,"GLEnableOutline( true | false ) - sets whether to render wireframe") 
{
   gOutlineEnabled = dAtob(argv[1]);
   if(gOutlineEnabled)
   {
      glDrawElementsProcPtr = glOutlineDrawElements;
      glDrawArraysProcPtr = glOutlineDrawArrays;
      Con::printf("swapped to outline mode funcs");
   } else {
      glDrawElementsProcPtr = glDrawElements;
      glDrawArraysProcPtr = glDrawArrays;
      Con::printf("swapped to normal funcs");
   }
}
#endif
