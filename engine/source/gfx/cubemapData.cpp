#include "cubemapData.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxCubemap.h"
#include "core/bitStream.h"

IMPLEMENT_CONOBJECT(CubemapData);

//****************************************************************************
// Cubemap Data
//****************************************************************************


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
CubemapData::CubemapData()
{
    cubemap = NULL;
    dynamic = false;
    mPath[0] = '\0';

    dMemset(cubeFaceFile, 0, sizeof(cubeFaceFile));
}

CubemapData::~CubemapData()
{
    if (cubemap)
    {
        delete cubemap;
    }
}

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void CubemapData::initPersistFields()
{
    Parent::initPersistFields();

    addField("cubeFace", TypeString, Offset(cubeFaceFile, CubemapData), 6);
    addField("dynamic", TypeBool, Offset(dynamic, CubemapData));
}

//--------------------------------------------------------------------------
// onAdd
//--------------------------------------------------------------------------
bool CubemapData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    createMap();

    return true;
}

//--------------------------------------------------------------------------
// Create map - this is public so cubemaps can be used with materials
// in the show tool
//--------------------------------------------------------------------------
void CubemapData::createMap()
{
    if (!cubemap)
    {
        if (dynamic)
        {
            cubemap = GFX->createCubemap();
            cubemap->initDynamic(512);
        }
        else
        {
            bool initSuccess = true;

            for (U32 i = 0; i < 6; i++)
            {
                if (cubeFaceFile[i] && cubeFaceFile[i][0])
                {
                    StringTableEntry faceFile = cubeFaceFile[i];
                    if (mPath[0] != '\0' && faceFile[0] == '.')
                    {
                        // Append path
                        char fullFilename[128];
                        dStrncpy(fullFilename, mPath, dStrlen(mPath) + 1);
                        dStrcat(fullFilename, faceFile);
                        faceFile = StringTable->insert(fullFilename);
                    }

                    if (!cubeFace[i].set(faceFile, &GFXDefaultStaticDiffuseProfile))
                    {
                        Con::errorf("CubemapData::createMap - Failed to load texture '%s'", faceFile);
                        initSuccess = false;
                    }
                }
            }

            if (initSuccess)
            {
                cubemap = GFX->createCubemap();
                cubemap->initStatic(cubeFace);
            }
        }
    }
}
