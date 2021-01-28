#include "cubemapData.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxCubemap.h"
#include "core/bitStream.h"

IMPLEMENT_CONOBJECT( CubemapData );

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

   dMemset( cubeFaceFile, 0, sizeof( cubeFaceFile ) );
}

CubemapData::~CubemapData()
{
   if( cubemap )
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

   addField("cubeFace",    TypeFilename, Offset(cubeFaceFile, CubemapData),6);
   addField("dynamic",     TypeBool, Offset(dynamic, CubemapData));
}

//--------------------------------------------------------------------------
// onAdd
//--------------------------------------------------------------------------
bool CubemapData::onAdd()
{
   if( !Parent::onAdd() )
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
   if( !cubemap )
   {
      if( dynamic )
      {
         cubemap = GFX->createCubemap();
         cubemap->initDynamic( 512 );
      }
      else
      {
         bool initSuccess = true;

         for( U32 i=0; i<6; i++ )
         {
            if( cubeFaceFile[i] && cubeFaceFile[i][0] )
            {
               if(!cubeFace[i].set(cubeFaceFile[i], &GFXDefaultStaticDiffuseProfile ))
               {
                  Con::errorf("CubemapData::createMap - Failed to load texture '%s'", cubeFaceFile[i]);
                  initSuccess = false;
               }
            }
         }

         if( initSuccess )
         {
            cubemap = GFX->createCubemap();
            cubemap->initStatic( cubeFace );
         }
      }
   }
}
