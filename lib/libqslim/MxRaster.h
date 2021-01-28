#ifndef MXRASTER_INCLUDED // -*- C++ -*-
#define MXRASTER_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  MxRaster

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxRaster.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "raster.h"

typedef ByteRaster MxRaster;

////////////////////////////////////////////////////////////////////////
//
// OpenGL support
//
#ifdef MXGL_INCLUDED
inline void glTexImage(const MxRaster& tex, GLenum target=GL_TEXTURE_2D)
{
    glTexImage2D(target, 0, 3, tex.width(), tex.height(),
		 0, GL_RGB, GL_UNSIGNED_BYTE, tex.head());
}

inline void gluMipmaps(const MxRaster& tex, GLenum target=GL_TEXTURE_2D)
{
    gluBuild2DMipmaps(target, 3, tex.width(), tex.height(),
		      GL_RGB, GL_UNSIGNED_BYTE, tex.head());
}

inline void glTexImage(const MxRaster *tex, GLenum target=GL_TEXTURE_2D)
	{ glTexImage(*tex, target); }
inline void gluMipmaps(const MxRaster *tex, GLenum target=GL_TEXTURE_2D)
	{ gluMipmaps(*tex, target); }
#endif

// MXRASTER_INCLUDED
#endif
