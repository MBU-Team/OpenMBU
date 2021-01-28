//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMGL_H_
#define _PLATFORMGL_H_

// set npatch enable flag here.  comment out if you don't want the features at all.
#define ENABLE_NPATCH      1

// ON MAC, WE ARE USING THE STD APPLE OPENGL HDRS.
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>

#if !defined(TORQUE_DEBUG)
#if !defined(AGLContext)
typedef struct __AGLContextRec       *AGLContext;
#endif
#include <AGL/aglMacro.h>
extern AGLContext agl_ctx;
#endif

// TODO: switch this over the ARB vertex buffers. this is old stuff for the d3d layer.
#ifndef GL_EXT_vertex_buffer
// don't define it!
//#define GL_EXT_vertex_buffer 1
//#ifdef GL_GLEXT_PROTOTYPES
extern GLboolean glAvailableVertexBufferEXT();
extern GLint glAllocateVertexBufferEXT(GLsizei size, GLint format, GLboolean preserve);
extern void* glLockVertexBufferEXT(GLint handle, GLsizei size);
extern void glUnlockVertexBufferEXT(GLint handle);
extern void glSetVertexBufferEXT(GLint handle);
extern void glOffsetVertexBufferEXT(GLint handle, GLuint offset);
extern void glFillVertexBufferEXT(GLint handle, GLint first, GLsizei count);
extern void glFreeVertexBufferEXT(GLint handle);
//#endif /* GL_GLEXT_PROTOTYPES */
typedef GLboolean (* PFNGLAVAILABLEVERTEXBUFFEREXTPROC) ();
typedef GLint (* PFNGLALLOCATEVERTEXBUFFEREXTPROC) (GLsizei size, GLint format, GLboolean preserve);
typedef void* (* PFNGLLOCKVERTEXBUFFEREXTPROC) (GLint handle, GLsizei size);
typedef void (* PFNGLUNLOCKVERTEXBUFFEREXTPROC) (GLint handle);
typedef void (* PFNGLSETVERTEXBUFFEREXTPROC) (GLint handle);
typedef void (* PFNGLOFFSETVERTEXBUFFEREXTPROC) (GLint handle, GLuint offset);
typedef void (* PFNGLFILLVERTEXBUFFEREXTPROC) (GLint handle, GLint first, GLsizei count);
typedef void (* PFNGLFREEVERTEXBUFFEREXTPROC) (GLint handle);
#endif // GL_EXT_vertex_buffer

#ifndef GL_EXT_fog_coord
// don't define it!
//#define GL_EXT_fog_coord	1
extern void glFogCoordfEXT (GLfloat);
extern void glFogCoordfvEXT (const GLfloat *);
extern void glFogCoorddEXT (GLdouble);
extern void glFogCoorddvEXT (const GLdouble *);
extern void glFogCoordPointerEXT (GLenum, GLsizei, const GLvoid *);
#define GL_FOG_COORDINATE_SOURCE_EXT      0x8450
#define GL_FOG_COORDINATE_EXT             0x8451
#define GL_FRAGMENT_DEPTH_EXT             0x8452
#define GL_CURRENT_FOG_COORDINATE_EXT     0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT  0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT 0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT 0x8456
#define GL_FOG_COORDINATE_ARRAY_EXT       0x8457
#endif

// Apple in their infinite wisdom switched to the ARB version, but
// didn't leave in backward-flagging for the old EXT version.  UGH.
#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine        GL_ARB_texture_env_combine
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE3_RGB_EXT                0x8583
#define GL_SOURCE4_RGB_EXT                0x8584
#define GL_SOURCE5_RGB_EXT                0x8585
#define GL_SOURCE6_RGB_EXT                0x8586
#define GL_SOURCE7_RGB_EXT                0x8587
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_SOURCE3_ALPHA_EXT              0x858B
#define GL_SOURCE4_ALPHA_EXT              0x858C
#define GL_SOURCE5_ALPHA_EXT              0x858D
#define GL_SOURCE6_ALPHA_EXT              0x858E
#define GL_SOURCE7_ALPHA_EXT              0x858F
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND3_RGB_EXT               0x8593
#define GL_OPERAND4_RGB_EXT               0x8594
#define GL_OPERAND5_RGB_EXT               0x8595
#define GL_OPERAND6_RGB_EXT               0x8596
#define GL_OPERAND7_RGB_EXT               0x8597
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#define GL_OPERAND3_ALPHA_EXT             0x859B
#define GL_OPERAND4_ALPHA_EXT             0x859C
#define GL_OPERAND5_ALPHA_EXT             0x859D
#define GL_OPERAND6_ALPHA_EXT             0x859E
#define GL_OPERAND7_ALPHA_EXT             0x859F
#endif

// these are the V12-custom extensions.  really only useful in an external 3d driver.
#define GL_V12MTVFMT_EXT                      0x8702
#define GL_V12MTNVFMT_EXT                     0x8703
#define GL_V12FTVFMT_EXT                      0x8704
#define GL_V12FMTVFMT_EXT                     0x8705

// make sure this is defined, as we need to use it when around.
#ifndef GL_CLAMP_TO_EDGE_EXT // should be, but apple only defined the SGIS version (same hexcode)
#define GL_CLAMP_TO_EDGE_EXT                     0x812F
#endif

/*
 * GL state information.
 */
struct GLState
{
   bool suppARBMultitexture;
   bool suppEXTblendcolor;
   bool suppEXTblendminmax;
   bool suppPackedPixels;
   bool suppTexEnvAdd;
   bool suppLockedArrays;

   bool suppTextureEnvCombine;
   bool suppVertexArrayRange;
   bool suppFogCoord;
   bool suppEdgeClamp;

   bool suppTextureCompression;
   bool suppS3TC;
   bool suppFXT1;
   bool suppTexAnisotropic;

   bool suppPalettedTexture;
   bool suppVertexBuffer;
   bool suppSwapInterval;

   GLint maxFSAASamples;

   unsigned int triCount[4];
   unsigned int primCount[4];
   unsigned int primMode; // 0-3

   GLfloat maxAnisotropy;
   GLint   maxTextureUnits;
};

extern GLState gGLState;

extern bool gOpenGLDisablePT;
extern bool gOpenGLDisableCVA;
extern bool gOpenGLDisableTEC;
extern bool gOpenGLDisableARBMT;
extern bool gOpenGLDisableFC;
extern bool gOpenGLDisableTCompress;
extern bool gOpenGLNoEnvColor;
extern float gOpenGLGammaCorrection;
extern bool gOpenGLNoDrawArraysAlpha;

/* 
 * Inline state helpers.
 */
inline void dglSetRenderPrimType(unsigned int type)
{
   gGLState.primMode = type;
}

inline void dglClearPrimMetrics()
{
   for(int i = 0; i < 4; i++)
      gGLState.triCount[i] = gGLState.primCount[i] = 0;
}

inline bool dglDoesSupportPalettedTexture()
{
   return gGLState.suppPalettedTexture && (gOpenGLDisablePT == false);
}

inline bool dglDoesSupportCompiledVertexArray()
{
   return gGLState.suppLockedArrays && (gOpenGLDisableCVA == false);
}

inline bool dglDoesSupportTextureEnvCombine()
{
   return gGLState.suppTextureEnvCombine && (gOpenGLDisableTEC == false);
}

inline bool dglDoesSupportARBMultitexture()
{
   return gGLState.suppARBMultitexture && (gOpenGLDisableARBMT == false);
}

inline bool dglDoesSupportEXTBlendColor()
{
   return gGLState.suppEXTblendcolor;
}

inline bool dglDoesSupportEXTBlendMinMax()
{
   return gGLState.suppEXTblendminmax;
}

inline bool dglDoesSupportVertexArrayRange()
{
   return gGLState.suppVertexArrayRange;
}

inline bool dglDoesSupportFogCoord()
{
   return gGLState.suppFogCoord && (gOpenGLDisableFC == false);
}

inline bool dglDoesSupportEdgeClamp()
{
   return gGLState.suppEdgeClamp;
}

inline bool dglDoesSupportTextureCompression()
{
   return gGLState.suppTextureCompression && (gOpenGLDisableTCompress == false);
}

inline bool dglDoesSupportS3TC()
{
   return gGLState.suppS3TC;
}

inline bool dglDoesSupportFXT1()
{
   return gGLState.suppFXT1;
}

inline bool dglDoesSupportTexEnvAdd()
{
   return gGLState.suppTexEnvAdd;
}

inline bool dglDoesSupportTexAnisotropy()
{
   return gGLState.suppTexAnisotropic;
}

inline bool dglDoesSupportVertexBuffer()
{
   return false;
}

inline GLfloat dglGetMaxAnisotropy()
{
   return gGLState.maxAnisotropy;
}

inline GLint dglGetMaxTextureUnits()
{
   if (dglDoesSupportARBMultitexture())
      return gGLState.maxTextureUnits;
   else
      return 1; 
}

// for radeon and up:
#define AGLSETINT_ATI_FSAA_SAMPLES  ((unsigned long)510)  // set Full Scene Anti-Aliasing (FSAA) samples: 1x, 2x, 4x
void dglSetFSAASamples(GLint aasamp);

#endif

#if defined(TORQUE_DEBUG)
#ifndef __GL_OUTLINE_FUNCS__
#define __GL_OUTLINE_FUNCS__

extern void (* glDrawElementsProcPtr) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void (* glDrawArraysProcPtr) (GLenum mode, GLint first, GLsizei count);

void glOutlineDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glOutlineDrawArrays(GLenum mode, GLint first, GLsizei count);

extern void (* glNormDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void (* glNormDrawArrays) (GLenum mode, GLint first, GLsizei count);

#ifndef NO_REDEFINE_GL_FUNCS
#define glDrawElements glDrawElementsProcPtr
#define glDrawArrays glDrawArraysProcPtr
#else 
#warning glDrawElements and glDrawArrays not redefined
#endif // NO_REDEFINE_GL_FUNCS
#endif //__GL_OUTLINE_FUNCS__
#endif
