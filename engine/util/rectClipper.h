//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RECTCLIPPER_H_
#define _RECTCLIPPER_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif


class RectClipper
{
   RectI m_clipRect;

  public:
   RectClipper(const RectI& in_rRect);

   bool  clipPoint(const Point2I& in_rPoint) const;
   bool  clipLine(const Point2I& in_rStart,
                  const Point2I& in_rEnd,
                  Point2I&       out_rStart,
                  Point2I&       out_rEnd) const;
   bool  clipRect(const RectI& in_rRect,
                  RectI&       out_rRect) const;
};

//------------------------------------------------------------------------------
//-------------------------------------- INLINES
//
inline
RectClipper::RectClipper(const RectI& in_rRect)
 : m_clipRect(in_rRect)
{
   //
}

inline bool
RectClipper::clipPoint(const Point2I& in_rPoint) const
{
   if ((in_rPoint.x < m_clipRect.point.x) ||
       (in_rPoint.y < m_clipRect.point.y) ||
       (in_rPoint.x >= m_clipRect.point.x + m_clipRect.extent.x) ||
       (in_rPoint.y >= m_clipRect.point.y + m_clipRect.extent.y))
      return false;
   return true;
}

#endif //_RECTCLIPPER_H_
