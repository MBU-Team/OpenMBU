//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformGL.h"
#include "platformX86UNIX/platformX86UNIX.h"
#include "console/console.h"

#include <dlfcn.h>
#include <SDL/SDL.h>

// declare stub functions
//#define GL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_return stub_##fn_name fn_args{ fn_value }
//#include "platformX86UNIX/gl_func.h"
//#undef GL_FUNCTION

// point gl function pointers at stub functions
//#define GL_FUNCTION(fn_return,fn_name,fn_args, fn_value) fn_return (*fn_name)fn_args = stub_##fn_name;
//#include "platformX86UNIX/gl_func.h"
//#undef GL_FUNCTION

static void* dlHandle = NULL;

static bool isMissingFnOK( const char *name)
{
   bool ok = false;

   // JMQ: these are specific to torque's d3d->gl wrapper.  They are not used under linux.
   if (dStrcmp(name, "glAvailableVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glAllocateVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glLockVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glUnlockVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glSetVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glOffsetVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glFillVertexBufferEXT")==0)
      ok = true;
   else if (dStrcmp(name, "glFreeVertexBufferEXT")==0)
      ok = true;

   return ok;
}

static bool bindFunction( void *&fnAddress, const char *name )
{
   fnAddress = SDL_GL_GetProcAddress(name);
   if (fnAddress == NULL)
   {
      if (!isMissingFnOK(name))
         // weird, but just warn about it for now
         Con::errorf(" Missing OpenGL function: %s.  You may experience rendering problems.", name);
      //Con::warnf(" Missing OpenGL function: %s", name);
//       else
//       {
//          Con::errorf(" Error: Missing OpenGL function: %s", name);
//          return false;
//       }
   }
   return true;
}

static bool bindOpenGLFunctions()
{
   bool result = true;
   // point gl function pointers at dll functions, if possible
//#define GL_FUNCTION(fn_return, fn_name, fn_args, fn_value) result &= bindFunction( *(void**)&fn_name, #fn_name);
//#include "platformX86UNIX/gl_func.h"
//#undef GL_FUNCTION

   return result;
}

static void unbindOpenGLFunctions()
{
   // point gl function pointers at stub functions
//#define GL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_name = stub_##fn_name;
//#include "platformX86UNIX/gl_func.h"
//#undef GL_FUNCTION
}

namespace GLLoader
{

   bool OpenGLInit()
   {
      return OpenGLDLLInit();
   }

   void OpenGLShutdown()
   {
      OpenGLDLLShutdown();
   }

   bool OpenGLDLLInit()
   {
      OpenGLDLLShutdown();

      // load libGL.so
      if (SDL_GL_LoadLibrary("libGL.so") == -1 &&
         SDL_GL_LoadLibrary("libGL.so.1") == -1)
      {
         Con::errorf("Error loading GL library: %s", SDL_GetError());
         return false;
      }

      // bind functions
      if (!bindOpenGLFunctions())
      {
         Con::errorf("Error binding GL functions");
         OpenGLDLLShutdown();
         return false;
      }

      return true;
   }

   void OpenGLDLLShutdown()
   {
      // there is no way to tell SDL to unload the library
      if (dlHandle != NULL)
      {
         dlclose(dlHandle);
         dlHandle = NULL;
      }

      unbindOpenGLFunctions();
   }

}

GLState gGLState;

bool  gOpenGLDisablePT                   = false;
bool  gOpenGLDisableCVA                  = false;
bool  gOpenGLDisableTEC                  = false;
bool  gOpenGLDisableARBMT                = false;
bool  gOpenGLDisableFC                   = false;
bool  gOpenGLDisableTCompress            = false;
bool  gOpenGLNoEnvColor                  = false;
float gOpenGLGammaCorrection             = 0.5;
bool  gOpenGLNoDrawArraysAlpha           = false;

// JMQTODO: really need a platform-shared version of this nastiness
/*bool QGL_EXT_Init( )
{
   // Load extensions...
   //
   const char* pExtString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
   gGLState.primMode = 0;

   // EXT_paletted_texture
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_EXT_paletted_texture") != NULL)
   {
      gGLState.suppPalettedTexture = true;
   }
   else
   {
      gGLState.suppPalettedTexture = false;
   }

   // EXT_compiled_vertex_array
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_EXT_compiled_vertex_array") != NULL)
   {
      gGLState.suppLockedArrays = true;
   }
   else
   {
      gGLState.suppLockedArrays = false;
   }

   // ARB_multitexture
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_ARB_multitexture") != NULL)
   {
      gGLState.suppARBMultitexture = true;
   } else
   {
      gGLState.suppARBMultitexture = false;
   }

   // NV_vertex_array_range
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_NV_vertex_array_range") != NULL) {
      gGLState.suppVertexArrayRange = true;
   } else {
      gGLState.suppVertexArrayRange = false;
   }

   // EXT_fog_coord
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_EXT_fog_coord") != NULL) {
      gGLState.suppFogCoord = true;
   } else {
      gGLState.suppFogCoord = false;
   }

   // ARB_texture_compression
   if (pExtString && dStrstr(pExtString,
          (const char*)"GL_ARB_texture_compression") != NULL) {
      gGLState.suppTextureCompression = true;
   } else {

      gGLState.suppTextureCompression = false;
   }

   // 3DFX_texture_compression_FXT1
   if (pExtString && dStrstr(pExtString, (const char*)"GL_3DFX_texture_compression_FXT1") != NULL)
      gGLState.suppFXT1 = true;
   else
      gGLState.suppFXT1 = false;

   // EXT_texture_compression_S3TC
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_texture_compression_s3tc") != NULL)
      gGLState.suppS3TC = true;
   else
      gGLState.suppS3TC = false;

   // JMQ: skipped WGL_3DFX_gamma_control

   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_vertex_buffer") != NULL)
   {
      gGLState.suppVertexBuffer = true;
   }
   else
   {
      gGLState.suppVertexBuffer = false;
   }

   // Binary states, i.e., no supporting functions
   // EXT_packed_pixels
   // EXT_texture_env_combine
   //
   // dhc note: a number of these can have multiple matching 'versions', private, ext, and arb.
   gGLState.suppPackedPixels      = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_packed_pixels") != NULL) : false;
   gGLState.suppTextureEnvCombine = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_combine") != NULL) : false;
   gGLState.suppEdgeClamp         = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_edge_clamp") != NULL) : false;
   gGLState.suppEdgeClamp        |= pExtString? (dStrstr(pExtString, (const char*)"GL_SGIS_texture_edge_clamp") != NULL) : false;
   gGLState.suppTexEnvAdd         = pExtString? (dStrstr(pExtString, (const char*)"GL_ARB_texture_env_add") != NULL) : false;
   gGLState.suppTexEnvAdd        |= pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_add") != NULL) : false;

   // Anisotropic filtering
   gGLState.suppTexAnisotropic    = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_filter_anisotropic") != NULL) : false;
   if (gGLState.suppTexAnisotropic)
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gGLState.maxAnisotropy);
   if (gGLState.suppARBMultitexture)
      glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gGLState.maxTextureUnits);
   else
      gGLState.maxTextureUnits = 1;

   // JMQ: vsync/swap interval skipped

   // NPatch/Truform support ========================================
#if ENABLE_NPATCH
   // !!!!!TBD implement full test.
   if (pExtString && dStrstr(pExtString, (const char*)GL_NPATCH_EXT_STRING) != NULL)
   {
      gGLState.suppNPatch = true;
      glGetIntegerv(GETINT_NPATCH_MAX_LEVEL, &gGLState.maxNPatchLevel);
      glNPatchSetInt = (PFNNPatchSetInt)qwglGetProcAddress(GL_NPATCH_SETINT_STRING);
   }
   else
   {
      gGLState.suppNPatch = false;
   }
#endif

   Con::printf("OpenGL Init: Enabled Extensions");
   if (gGLState.suppARBMultitexture)    Con::printf("  ARB_multitexture (Max Texture Units: %d)", gGLState.maxTextureUnits);
   if (gGLState.suppPalettedTexture)    Con::printf("  EXT_paletted_texture");
   if (gGLState.suppLockedArrays)       Con::printf("  EXT_compiled_vertex_array");
   if (gGLState.suppVertexArrayRange)   Con::printf("  NV_vertex_array_range");
   if (gGLState.suppTextureEnvCombine)  Con::printf("  EXT_texture_env_combine");
   if (gGLState.suppPackedPixels)       Con::printf("  EXT_packed_pixels");
   if (gGLState.suppFogCoord)           Con::printf("  EXT_fog_coord");
   if (gGLState.suppTextureCompression) Con::printf("  ARB_texture_compression");
   if (gGLState.suppS3TC)               Con::printf("  EXT_texture_compression_s3tc");
   if (gGLState.suppFXT1)               Con::printf("  3DFX_texture_compression_FXT1");
   if (gGLState.suppTexEnvAdd)          Con::printf("  (ARB|EXT)_texture_env_add");
   if (gGLState.suppTexAnisotropic)     Con::printf("  EXT_texture_filter_anisotropic (Max anisotropy: %f)", gGLState.maxAnisotropy);
   if (gGLState.suppSwapInterval)       Con::printf("  WGL_EXT_swap_control");
#if ENABLE_NPATCH
   if (gGLState.suppNPatch)             Con::printf("  NPatch tessellation");
#endif

   Con::warnf("OpenGL Init: Disabled Extensions");
   if (!gGLState.suppARBMultitexture)    Con::warnf("  ARB_multitexture");
   if (!gGLState.suppPalettedTexture)    Con::warnf("  EXT_paletted_texture");
   if (!gGLState.suppLockedArrays)       Con::warnf("  EXT_compiled_vertex_array");
   if (!gGLState.suppVertexArrayRange)   Con::warnf("  NV_vertex_array_range");
   if (!gGLState.suppTextureEnvCombine)  Con::warnf("  EXT_texture_env_combine");
   if (!gGLState.suppPackedPixels)       Con::warnf("  EXT_packed_pixels");
   if (!gGLState.suppFogCoord)           Con::warnf("  EXT_fog_coord");
   if (!gGLState.suppTextureCompression) Con::warnf("  ARB_texture_compression");
   if (!gGLState.suppS3TC)               Con::warnf("  EXT_texture_compression_s3tc");
   if (!gGLState.suppFXT1)               Con::warnf("  3DFX_texture_compression_FXT1");
   if (!gGLState.suppTexEnvAdd)          Con::warnf("  (ARB|EXT)_texture_env_add");
   if (!gGLState.suppTexAnisotropic)     Con::warnf("  EXT_texture_filter_anisotropic");
   if (!gGLState.suppSwapInterval)       Con::warnf("  WGL_EXT_swap_control");
#if ENABLE_NPATCH
   if (!gGLState.suppNPatch)             Con::warnf("  NPatch tessellation");
#endif
   Con::printf(" ");

   // Set some console variables:
   Con::setBoolVariable( "$FogCoordSupported", gGLState.suppFogCoord );
   Con::setBoolVariable( "$TextureCompressionSupported", gGLState.suppTextureCompression );
   Con::setBoolVariable( "$AnisotropySupported", gGLState.suppTexAnisotropic );
   Con::setBoolVariable( "$PalettedTextureSupported", gGLState.suppPalettedTexture );
   Con::setBoolVariable( "$SwapIntervalSupported", gGLState.suppSwapInterval );

   if (!gGLState.suppPalettedTexture && Con::getBoolVariable("$pref::OpenGL::forcePalettedTexture",false))
   {
      Con::setBoolVariable("$pref::OpenGL::forcePalettedTexture", false);
      Con::setBoolVariable("$pref::OpenGL::force16BitTexture", true);
   }

   return true;
}*/

