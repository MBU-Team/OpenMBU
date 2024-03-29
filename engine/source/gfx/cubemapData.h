#ifndef _CUBEMAPDATA_H_
#define _CUBEMAPDATA_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#include "gfx/gfxTextureHandle.h"

class GFXCubemap;

//**************************************************************************
// Cubemap data
//**************************************************************************
class CubemapData : public SimObject
{
    typedef SimObject Parent;

    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------
public:

    GFXCubemap* cubemap;
    StringTableEntry  cubeFaceFile[6];
    GFXTexHandle      cubeFace[6];
    bool              dynamic;
    char              mPath[256];

    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
public:
    CubemapData();
    ~CubemapData();

    bool onAdd();
    static void initPersistFields();

    void createMap();

    DECLARE_CONOBJECT(CubemapData);
};



#endif // CUBEMAPDATA

