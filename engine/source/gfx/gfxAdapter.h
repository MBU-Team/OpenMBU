//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
//
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXAdapter_H_
#define _GFXAdapter_H_

#include "platform/types.h"
#include "gfx/gfxEnums.h"
#include "gfx/gfxStructs.h"

struct GFXAdapter
{
    GFXAdapterType mType;
    U32            mIndex;

    enum
    {
        MaxAdapterNameLen = 512,
    };

    char mName[MaxAdapterNameLen];

    /// List of available full-screen modes. Windows can be any size,
    /// so we do not enumerate them here.
    Vector<GFXVideoMode> mAvailableModes;

    /// Supported shader model. 0.f means none supported.
    F32 mShaderModel;

private:
    // Disallow copying to prevent mucking with our data above.
    GFXAdapter(const GFXAdapter&);

public:
    GFXAdapter()
    {
        mName[0] = 0;
        mShaderModel = 0.f;
        mIndex = 0;
    }

    ~GFXAdapter()
    {
        mAvailableModes.clear();
    }

    const char * getName() const { return mName; }
};

#endif // _GFXAdapter_H_
