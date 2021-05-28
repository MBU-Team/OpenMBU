//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Many OSX frameworks include OpenTransport, and OT's new operator conflicts 
// with our redefinition of 'new', so we have to pre-include platformMacCarb.h,
// which contains the workaround.
#include "platformMacCarb/platformMacCarb.h"

#include "platform/types.h"

#include "PlatformMacCarb/platformGL.h"
#include "dgl/dgl.h"
#include "console/console.h"

#include <AGL/agl.h>

GLState gGLState;

bool hwIsaRadeon = false; // for detecting Radeon-brand (R, R8500, R7500, R7200, etc...) boards.
bool hwIsaR200 = false; // for detecting R200 (R8500, R8800, etc...) or later boards.

bool gOpenGLDisablePT          = false;
bool gOpenGLDisableCVA         = false;
bool gOpenGLDisableTEC         = false;
bool gOpenGLDisableARBMT       = false;
bool gOpenGLDisableFC          = true; //false;
bool gOpenGLDisableTCompress   = false;
bool gOpenGLNoEnvColor         = false;
float gOpenGLGammaCorrection   = 0.5;
bool gOpenGLNoDrawArraysAlpha  = false;

// !!!!!TBD -- What to do about missing functions??  are they in 1.3???
#define HANDLE_MISSING_EXTS		1
#if HANDLE_MISSING_EXTS // until we decide how to actually implement these...

/*
GLboolean mglAvailVB() {   return(false); }
GLint mglAllocVB(GLsizei size, GLint format, GLboolean preserve) {   return(0); }
void* mglLockVB(GLint handle, GLsizei size) {   return(NULL); }
void mglUnlockVB(GLint handle) {}
void mglSetVB(GLint handle) {}
void mglOffsetVB(GLint handle, GLuint offset) {}
void mglFillVB(GLint handle, GLint first, GLsizei count) {}
void mglFreeVB(GLint handle) {}
void mglFogCP(GLenum type, GLsizei stride, const GLvoid *pointer) {}
void mglColorTable(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table) {}
*/
// maybe we don't need the extern C?
#if GL_EXTERN_C
extern "C"
{
#endif
GLboolean glAvailableVertexBufferEXT() {   return(false); }
GLint glAllocateVertexBufferEXT(GLsizei size, GLint format, GLboolean preserve) {   return(0); }
void* glLockVertexBufferEXT(GLint handle, GLsizei size) {   return(NULL); }
void glUnlockVertexBufferEXT(GLint handle) {}
void glSetVertexBufferEXT(GLint handle) {}
void glOffsetVertexBufferEXT(GLint handle, GLuint offset) {}
void glFillVertexBufferEXT(GLint handle, GLint first, GLsizei count) {}
void glFreeVertexBufferEXT(GLint handle) {}

// if the colortable stuff isn't declared, or we're building in PB but against < GL1.3, stub the colortable fn.
#if !defined(GL_EXT_fog_coord) || !defined(__APPLE__) || !defined(GL_VERSION_1_3)
void glFogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *pointer) {}
#endif

// if the colortable stuff isn't declared, or we're building in PB but against < GL1.3, stub the colortable fn.
#if !defined(GL_EXT_paletted_texture) || !defined(__APPLE__) || !defined(GL_VERSION_1_3)
void glColorTableEXT(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table) {}
#endif
#if GL_EXTERN_C
}
#endif
#endif // HANDLE_MISSING_EXTS


// Load extensions.  !!!TBD rename the darn function already.
bool GL_EXT_Init( )
{
   OSStatus err = 0;

   // we only really need two of the strings.
   const char* pExtString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
   const char* pRendString = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

   // pre-clear the structure!!!
   dMemset(&gGLState, 0, sizeof(gGLState));

   // early hardware detection.  // !!!TBD update for new boards.
   hwIsaRadeon = false;
   hwIsaR200 = false;
   if (pRendString && dStrstr(pRendString, (const char*)"ATI ") != NULL)
   { // we know we're on an ATI renderer at least.  now, which one??
      // first, check for 8500 variations
      if (dStrstr(pRendString, (const char*)"ATI R-") != NULL // that way, we'll catch R-300...
      ||  dStrstr(pRendString, (const char*)"ATI R200") != NULL
      ||  dStrstr(pRendString, (const char*)"ATI Radeon 8") != NULL) // catch 8500, 8800, etc.
      {
         hwIsaR200 = true;
         hwIsaRadeon = true; // if is an R200, it's >= Radeon.
      }
      else // wasn't an R200+, let's just look for Radeon
      if (dStrstr(pRendString, (const char*)"ATI Radeon"))
      {
         hwIsaRadeon = true;
      }
   }
   
   // EXT_paletted_texture ========================================
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_paletted_texture") != NULL)
      gGLState.suppPalettedTexture = true;
   else
      gGLState.suppPalettedTexture = false;
   
   // EXT_compiled_vertex_array ========================================
   gGLState.suppLockedArrays = false;
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_compiled_vertex_array") != NULL)
      gGLState.suppLockedArrays = true;

   // ARB_multitexture ========================================
/*
      glActiveTextureARB       = NULL;
      glClientActiveTextureARB = NULL;
      glMultiTexCoord2fARB     = NULL;
      glMultiTexCoord2fvARB    = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"GL_ARB_multitexture") != NULL)
      gGLState.suppARBMultitexture = true;
   else
      gGLState.suppARBMultitexture = false;
   
   // EXT_blend_color
   if(pExtString && dStrstr(pExtString, (const char*)"GL_EXT_blend_color") != NULL)
      gGLState.suppEXTblendcolor = true;
   else
      gGLState.suppEXTblendcolor = false;

   // EXT_blend_minmax
   if(pExtString && dStrstr(pExtString, (const char*)"GL_EXT_blend_minmax") != NULL)
      gGLState.suppEXTblendminmax = true;
   else
      gGLState.suppEXTblendminmax = false;

   // NV_vertex_array_range ========================================
/*
      glVertexArrayRangeNV      = dllVertexArrayRangeNV      = NULL;
      glFlushVertexArrayRangeNV = dllFlushVertexArrayRangeNV = NULL;
*/
/*
   wglAllocateMemoryNV       = NULL;
   wglFreeMemoryNV           = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"GL_NV_vertex_array_range") != NULL)
      gGLState.suppVertexArrayRange = true;
   else
      gGLState.suppVertexArrayRange = false;
   

   // EXT_fog_coord ========================================
/*
      glFogCoordfEXT       = NULL;
      glFogCoordPointerEXT = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_fog_coord") != NULL)
      gGLState.suppFogCoord = true;
   else
      gGLState.suppFogCoord = false;
   
   // ARB_texture_compression ========================================
/*
      glCompressedTexImage3DARB    = NULL;
      glCompressedTexImage2DARB    = NULL;
      glCompressedTexImage1DARB    = NULL;
      glCompressedTexSubImage3DARB = NULL;
      glCompressedTexSubImage2DARB = NULL;
      glCompressedTexSubImage1DARB = NULL;
      glGetCompressedTexImageARB   = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"GL_ARB_texture_compression") != NULL)
      gGLState.suppTextureCompression = true;
   else
      gGLState.suppTextureCompression = false;
   

   // 3DFX_texture_compression_FXT1 ========================================
   if (pExtString && dStrstr(pExtString, (const char*)"GL_3DFX_texture_compression_FXT1") != NULL)
      gGLState.suppFXT1 = true;
   else
      gGLState.suppFXT1 = false;


   // EXT_texture_compression_S3TC ========================================
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_texture_compression_s3tc") != NULL)
      gGLState.suppS3TC = true;
   else
      gGLState.suppS3TC = false;


   // WGL_3DFX_gamma_control ========================================
/*
      qwglGetDeviceGammaRamp3DFX = NULL;
      qwglSetDeviceGammaRamp3DFX = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"WGL_3DFX_gamma_control" ) != NULL)
   { 
//      qwglGetDeviceGammaRamp3DFX = (qwglGetDeviceGammaRamp3DFX_t) qwglGetProcAddress( "wglGetDeviceGammaRamp3DFX" ); 
//     qwglSetDeviceGammaRamp3DFX = (qwglSetDeviceGammaRamp3DFX_t) qwglGetProcAddress( "wglSetDeviceGammaRamp3DFX" );
   }
   else
   {
   }


   // EXT_vertex_buffer ========================================
/*
      glAvailableVertexBufferEXT   = NULL;
      glAllocateVertexBufferEXT   = NULL;
      glLockVertexBufferEXT      = NULL;
      glUnlockVertexBufferEXT      = NULL;
      glSetVertexBufferEXT         = NULL;
      glOffsetVertexBufferEXT      = NULL;
      glFillVertexBufferEXT      = NULL;
      glFreeVertexBufferEXT      = NULL;
*/
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_vertex_buffer") != NULL)
   {
      gGLState.suppVertexBuffer = true;
      AssertWarn(gGLState.suppVertexBuffer == false, "We're assuming no vertex bufffer support on Mac for now!");
   }
   else
      gGLState.suppVertexBuffer = false;

   // Binary states, i.e., no supporting functions  ========================================
   // NOTE:
   // Some of these have multiple representations, via EXT and|or ARB and|or NV and|or SGIS ... etc.
   // Check all relative versions.
   
   gGLState.suppPackedPixels      = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_packed_pixels") != NULL) : false;
   gGLState.suppPackedPixels     |= pExtString? (dStrstr(pExtString, (const char*)"GL_APPLE_packed_pixel") != NULL) : false;

   gGLState.suppTextureEnvCombine = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_combine") != NULL) : false;
   gGLState.suppTextureEnvCombine|= pExtString? (dStrstr(pExtString, (const char*)"GL_ARB_texture_env_combine") != NULL) : false;

   gGLState.suppEdgeClamp         = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_edge_clamp") != NULL) : false;
   // whoops.  there's another things to check for.
   gGLState.suppEdgeClamp        |= pExtString? (dStrstr(pExtString, (const char*)"GL_SGIS_texture_edge_clamp") != NULL) : false;
   gGLState.suppEdgeClamp        |= pExtString? (dStrstr(pExtString, (const char*)"GL_ARB_texture_border_clamp") != NULL) : false;

   gGLState.suppTexEnvAdd         = pExtString? (dStrstr(pExtString, (const char*)"GL_ARB_texture_env_add") != NULL) : false;
   gGLState.suppTexEnvAdd        |= pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_add") != NULL) : false;

   // Anisotropic filtering ========================================
   gGLState.suppTexAnisotropic    = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_filter_anisotropic") != NULL) : false;
   if (gGLState.suppTexAnisotropic)
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gGLState.maxAnisotropy);


   // Texture combine units  ========================================
   if (gGLState.suppARBMultitexture)
      glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gGLState.maxTextureUnits);
   else
      gGLState.maxTextureUnits = 1;


   // Swap interval ========================================
//   if (pExtString && dStrstr(pExtString, (const char*)"WGL_EXT_swap_control") != NULL)
//      gGLState.suppSwapInterval = true; //( qwglSwapIntervalEXT != NULL );
//   else
// Mac inherently supports a swap interval AGL-set-integer extension.
      gGLState.suppSwapInterval = true;


   // FSAA support, currently Radeon++/ATI only.
   gGLState.maxFSAASamples = 0;
   if (hwIsaRadeon)
   {
      // default to off.
      gGLState.maxFSAASamples = 1;
   }

   // console out all the extensions...
   Con::printf("OpenGL Init: Enabled Extensions");
   if (gGLState.suppARBMultitexture)    Con::printf("  ARB_multitexture (Max Texture Units: %d)", gGLState.maxTextureUnits);
   if (gGLState.suppEXTblendcolor)      Con::printf("  EXT_blend_color");
   if (gGLState.suppEXTblendminmax)     Con::printf("  EXT_blend_minmax");
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
   if (gGLState.maxFSAASamples)         Con::printf("  ATI_FSAA");

   Con::warnf("OpenGL Init: Disabled Extensions");
   if (!gGLState.suppARBMultitexture)    Con::warnf("  ARB_multitexture");
   if (!gGLState.suppEXTblendcolor)      Con::warnf("  EXT_blend_color");
   if (!gGLState.suppEXTblendminmax)     Con::warnf("  EXT_blend_minmax");
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
   if (!gGLState.maxFSAASamples)         Con::warnf("  ATI_FSAA");
   Con::printf("");

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

   if (gGLState.maxFSAASamples>1)
   {
      gFSAASamples = Con::getIntVariable("$pref::OpenGL::numFSAASamples", gGLState.maxFSAASamples<<1);
      dglSetFSAASamples(gFSAASamples);
   }

   return true;
}

// this is currently only for Radeon++ ATI boards
// pass in 1 for normal/no-AA, 2 for 2x-AA, 4 for 4x-AA.
void dglSetFSAASamples(GLint aasamp)
{
   if (gGLState.maxFSAASamples<2) return;
   if (!hwIsaRadeon) return; // sanity check.

   // for the moment, don't assume values passed in are capped min/max.
   if (aasamp<1) aasamp = 1;
   else
   if (aasamp==3) aasamp = 2;
   else
   if (aasamp>4) aasamp = 4;

   // !!!!TBD method for OSX invocation.
   aglSetInteger((AGLContext)platState.ctx, AGLSETINT_ATI_FSAA_SAMPLES, (const long*)&aasamp);
   
   Con::printf(">>Number of FSAA samples is now [%d].", aasamp);
   Con::setIntVariable("$pref::OpenGL::numFSAASamples", aasamp);
   if (Con::getIntVariable("$pref::TradeshowDemo", 0))
      Con::executef(2, "setFSAABadge", aasamp>1?"true":"false");
}
