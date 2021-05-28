//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMVIDEO_H_
#define _PLATFORMVIDEO_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif

enum devices
{
    NO_DEVICE = -1,
    OPENGL_DEVICE,
    VOODOO2_DEVICE,
    N_DEVICES
};

struct Resolution;
class DisplayDevice;


class Video
{
private:
    static Vector<DisplayDevice*> smDeviceList;
    static DisplayDevice* smCurrentDevice;
    static bool smCritical;

public:
    static bool smNeedResurrect;

    static void init();                                // enumerate all the devices
    static void destroy();                             // clean up and shut down
    static bool installDevice(DisplayDevice* dev);
    static bool setDevice(const char* renderName, U32 width, U32 height, U32 bpp, bool fullScreen);   // set the current display device
    static bool setScreenMode(U32 width, U32 height, U32 bpp, bool fullScreen);
    static void deactivate();                          // deactivate current display device
    static void reactivate();                          // reactivate current display device
    static bool setResolution(U32 width, U32 height, U32 bpp);   // set the current resolution
    static bool toggleFullScreen();                    // toggle full screen mode
    static DisplayDevice* getDevice(const char* renderName);
    static const char* getDeviceList();                  // get a tab-separated list of all the installed display devices
    static const char* getResolutionList();            // get a tab-separated list of all the available resolutions for the current device
    static const char* getDriverInfo();                  // get info about the current display device driver
    static bool prevRes();                             // switch to the next smaller available resolution with the same bit depth
    static bool nextRes();                             // switch to the next larger available resolution with the same bit depth
    static Resolution getResolution();                 // get the current resolution
    //static GFXSurface *getSurface();                   // get a renderable surface (can return NULL if the window is minimized)
    static bool isFullScreen();                        // return the current screen state
    static void swapBuffers();                         // page flip
    static bool getGammaCorrection(F32& g);            // get gamma correction
    static bool setGammaCorrection(F32 g);             // set gamma correction
    static bool setVerticalSync(bool on);            // enable/disable vertical sync
};


struct Resolution
{
    U32 w, h, bpp;

    Resolution(U32 _w = 0, U32 _h = 0, U32 _bpp = 0)
    {
        w = _w;
        h = _h;
        bpp = _bpp;
    }

    bool operator==(const Resolution& otherRes) const
    {
        return ((w == otherRes.w) && (h == otherRes.h) && (bpp == otherRes.bpp));
    }

    void operator=(const Resolution& otherRes)
    {
        w = otherRes.w;
        h = otherRes.h;
        bpp = otherRes.bpp;
    }
};


class DisplayDevice
{
public:
    const char* mDeviceName;

protected:
    static Resolution    smCurrentRes;
    static bool          smIsFullScreen;

    Vector<Resolution>   mResolutionList;
    bool                 mFullScreenOnly;

public:
    DisplayDevice();

    virtual void initDevice() = 0;
    virtual bool activate(U32 width, U32 height, U32 bpp, bool fullScreen) = 0;
    virtual void shutdown() = 0;

    virtual bool setScreenMode(U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt = false, bool repaint = true) = 0;
    virtual bool setResolution(U32 width, U32 height, U32 bpp);
    virtual bool toggleFullScreen();
    virtual void swapBuffers() = 0;
    virtual const char* getDriverInfo() = 0;
    virtual bool getGammaCorrection(F32& g) = 0;
    virtual bool setGammaCorrection(F32 g) = 0;
    virtual bool setVerticalSync(bool on) = 0;

    bool prevRes();
    bool nextRes();
    const char* getResolutionList();
    bool isFullScreenOnly() { return(mFullScreenOnly); }

    static void       init();
    static Resolution getResolution();
    static bool       isFullScreen();
};


//------------------------------------------------------------------------------
inline bool DisplayDevice::setResolution(U32 width, U32 height, U32 bpp)
{
    return setScreenMode(width, height, bpp, smIsFullScreen);
}


//------------------------------------------------------------------------------
inline bool DisplayDevice::toggleFullScreen()
{
    return setScreenMode(smCurrentRes.w, smCurrentRes.h, smCurrentRes.bpp, !smIsFullScreen);
}


//------------------------------------------------------------------------------
inline Resolution DisplayDevice::getResolution()
{
    return smCurrentRes;
}


//------------------------------------------------------------------------------
inline bool DisplayDevice::isFullScreen()
{
    return smIsFullScreen;
}

#endif // _H_PLATFORMVIDEO
