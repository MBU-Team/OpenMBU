//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// AABB-triangle overlap test code originally by Tomas Akenine-Möller
//               Assisted by Pierre Terdiman and David Hunt
// http://www.cs.lth.se/home/Tomas_Akenine_Moller/code/
// Ported to TSE by BJG, 2005-4-14
//-----------------------------------------------------------------------------

#ifndef _TRIBOXCHECK_H_
#define _TRIBOXCHECK_H_

#include "math/mPoint.h"
#include "math/mBox.h"

bool triBoxOverlap(Point3F boxcenter, Point3F boxhalfsize, Point3F triverts[3]);

/// Massage stuff into right format for triBoxOverlap test. This is really
/// just a helper function - use the other version if you want to be fast!
inline bool triBoxOverlap(Box3F box, Point3F a, Point3F b, Point3F c)
{
   Point3F halfSize(box.len_x() / 2.f, box.len_y() / 2.f, box.len_z() / 2.f);

   Point3F center;
   box.getCenter(&center);

   Point3F verts[3] = {a,b,c};

   return triBoxOverlap(center, halfSize, verts);
}

#endif