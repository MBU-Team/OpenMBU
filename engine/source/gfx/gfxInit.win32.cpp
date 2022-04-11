//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
//
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include <d3d9.h>
#include <d3dx9.h>
#include "gfx/gfxInit.h"

// Platform specific API includes
//#include "gfx/D3D9/pc/gfxPCD3D9Device.h" // TEMP: re-add
#include "gfx/Null/gfxNullDevice.h"

#include "platformWin32/platformWin32.h"
#include "console/console.h"

Vector<GFXAdapter*> GFXInit::smAdapters;

//extern void enumerateD3D8Adapters(Vector<GFXAdapter*>& adaptorList);
//extern void createD3D8Instance(GFXDevice** device, U32 adapterIndex);

void GFXInit::enumerateAdapters()
{
    // Call each device class and have it report any adapters it supports.
    if(smAdapters.size())
    {
        // CodeReview Seems like this is ok to just ignore? [bjg, 5/19/07]
        //Con::warnf("GFXInit::enumerateAdapters - already have a populated adapter list, aborting re-analysis.");
        return;
    }

    //GFXPCD3D9Device::enumerateAdapters( GFXInit::smAdapters ); // TEMP: re-add
    GFXNullDevice::enumerateAdapters( GFXInit::smAdapters );
    //enumerateD3D8Adapters( GFXInit::smAdapters );
}

//-----------------------------------------------------------------------------

GFXDevice *GFXInit::createDevice( GFXAdapter *adapter )
{
    GFXDevice* temp = NULL;
    Con::printf("Attempting to create GFX device: %s", adapter->getName());
    switch (adapter->mType)
    {
        // TEMP: re-add
//        case Direct3D9 :
//            temp = GFXPCD3D9Device::createInstance( adapter->mIndex );
//            break;
//        case Direct3D8:
//            createD3D8Instance( &temp, adapter->mIndex );
//            break;
        case NullDevice :
            temp = GFXNullDevice::createInstance( adapter->mIndex );
            break;
        default :
        AssertFatal(false, "Bad adapter type");
            break;
    }

    if (temp)
    {
        Con::printf("Device created, setting adapter and enumerating modes");
        temp->setAdapter(*adapter);
        temp->enumerateVideoModes();
        temp->getVideoModeList();
    }
    else
        Con::errorf("Failed to create GFX device");

    GFX->setActiveDevice(0);
    GFXDevice::getDeviceEventSignal().trigger(GFXDevice::deCreate);

    return temp;
}

//---------------------------------------------------------------
void GFXInit::getAdapters(Vector<GFXAdapter*> *adapters)
{
    adapters->clear();
    for (U32 k = 0; k < smAdapters.size(); k++)
        adapters->push_back(smAdapters[k]);
}

GFXVideoMode GFXInit::getDesktopResolution()
{
    GFXVideoMode resVm;

    // Retrieve Resolution Information.
    resVm.bitDepth    = 32; //WindowManager->getDesktopBitDepth(); // TODO: Support this
    resVm.resolution  = Point2I(800, 600);//WindowManager->getDesktopResolution(); // TODO: Support this
    resVm.fullScreen  = false;
    resVm.refreshRate = 60;

    // Return results
    return resVm;
}
