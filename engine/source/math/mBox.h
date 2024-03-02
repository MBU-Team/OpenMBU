//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MBOX_H_
#define _MBOX_H_

#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

//------------------------------------------------------------------------------
/// Bounding Box
///
/// A helper class for working with boxes. It runs at F32 precision.
///
/// @see Box3D
class Box3F
{
public:
    Point3F min; ///< Minimum extents of box
    Point3F max; ///< Maximum extents of box

public:
    Box3F() { }

    /// Create a box from two points.
    ///
    /// Normally, this function will compensate for mismatched
    /// min/max values. If you know your values are valid, you
    /// can set in_overrideCheck to true and skip this.
    ///
    /// @param   in_rMin          Minimum extents of box.
    /// @param   in_rMax          Maximum extents of box.
    /// @param   in_overrideCheck  Pass true to skip check of extents.
    Box3F(const Point3F& in_rMin, const Point3F& in_rMax, const bool in_overrideCheck = false);

    /// Create a box from six extent values.
    ///
    /// No checking is performed as to the validity of these
    /// extents, unlike the other constructor.
    Box3F(F32 xmin, F32 ymin, F32 zmin, F32 max, F32 ymax, F32 zmax);

    /// Check to see if a point is contained in this box.
    bool isContained(const Point3F& in_rContained) const;

    /// Check to see if another box overlaps this box.
    bool isOverlapped(const Box3F& in_rOverlap) const;

    /// Check to see if another box is contained in this box.
    bool isContained(const Box3F& in_rContained) const;

    F32 len_x() const;
    F32 len_y() const;
    F32 len_z() const;

    /// Perform an intersection operation with another box
    /// and store the results in this box.
    void intersect(const Box3F& in_rIntersect);
    void intersect(const Point3F& in_rIntersect);

    /// Get the center of this box.
    ///
    /// This is the average of min and max.
    void getCenter(Point3F* center) const;

    /// Collide a line against the box.
    ///
    /// @param   start   Start of line.
    /// @param   end     End of line.
    /// @param   t       Value from 0.0-1.0, indicating position
    ///                  along line of collision.
    /// @param   n       Normal of collision.
    bool collideLine(const Point3F& start, const Point3F& end, F32* t, Point3F* n) const;

    /// Collide a line against the box.
    ///
    /// Returns true on collision.
    bool collideLine(const Point3F& start, const Point3F& end) const;

    /// Collide an oriented box against the box.
    ///
    /// Returns true if "oriented" box collides with us.
    /// Assumes incoming box is centered at origin of source space.
    ///
    /// @param   radii   The dimension of incoming box (half x,y,z length).
    /// @param   toUs    A transform that takes incoming box into our space.
    bool collideOrientedBox(const Point3F& radii, const MatrixF& toUs) const;

    /// Check that the box is valid.
    ///
    /// Currently, this just means that min < max.
    bool isValidBox() const {
        return (min.x <= max.x) &&
            (min.y <= max.y) &&
            (min.z <= max.z);
    }

    /// Return the closest point of the box, relative to the passed point.
    Point3F getClosestPoint(const Point3F& refPt) const;

    /// Return distance of closest point on box to refPt.
    F32 getDistanceFromPoint(const Point3F& refPt) const;

    /// Extend the box to include point.
    /// @see Invalid
    void extend(const Point3F& p);
};

inline Box3F::Box3F(const Point3F& in_rMin, const Point3F& in_rMax, const bool in_overrideCheck)
    : min(in_rMin),
    max(in_rMax)
{
    if (in_overrideCheck == false) {
        min.setMin(in_rMax);
        max.setMax(in_rMin);
    }
}

inline Box3F::Box3F(F32 xMin, F32 yMin, F32 zMin, F32 xMax, F32 yMax, F32 zMax)
    : min(xMin, yMin, zMin),
    max(xMax, yMax, zMax)
{
}

inline bool Box3F::isContained(const Point3F& in_rContained) const
{
    return (in_rContained.x >= min.x && in_rContained.x < max.x) &&
        (in_rContained.y >= min.y && in_rContained.y < max.y) &&
        (in_rContained.z >= min.z && in_rContained.z < max.z);
}

inline bool Box3F::isOverlapped(const Box3F& in_rOverlap) const
{
    if (in_rOverlap.min.x > max.x ||
        in_rOverlap.min.y > max.y ||
        in_rOverlap.min.z > max.z)
        return false;
    if (in_rOverlap.max.x < min.x ||
        in_rOverlap.max.y < min.y ||
        in_rOverlap.max.z < min.z)
        return false;
    return true;
}

inline bool Box3F::isContained(const Box3F& in_rContained) const
{
    return (min.x <= in_rContained.min.x) &&
        (min.y <= in_rContained.min.y) &&
        (min.z <= in_rContained.min.z) &&
        (max.x >= in_rContained.max.x) &&
        (max.y >= in_rContained.max.y) &&
        (max.z >= in_rContained.max.z);
}

inline F32 Box3F::len_x() const
{
    return max.x - min.x;
}

inline F32 Box3F::len_y() const
{
    return max.y - min.y;
}

inline F32 Box3F::len_z() const
{
    return max.z - min.z;
}

inline void Box3F::intersect(const Box3F& in_rIntersect)
{
    min.setMin(in_rIntersect.min);
    max.setMax(in_rIntersect.max);
}

inline void Box3F::intersect(const Point3F& in_rIntersect)
{
    min.setMin(in_rIntersect);
    max.setMax(in_rIntersect);
}

inline void Box3F::getCenter(Point3F* center) const
{
    center->x = F32((min.x + max.x) * 0.5);
    center->y = F32((min.y + max.y) * 0.5);
    center->z = F32((min.z + max.z) * 0.5);
}

inline Point3F Box3F::getClosestPoint(const Point3F& refPt) const
{
    Point3F closest;
    if (refPt.x <= min.x) closest.x = min.x;
    else if (refPt.x > max.x) closest.x = max.x;
    else                       closest.x = refPt.x;

    if (refPt.y <= min.y) closest.y = min.y;
    else if (refPt.y > max.y) closest.y = max.y;
    else                       closest.y = refPt.y;

    if (refPt.z <= min.z) closest.z = min.z;
    else if (refPt.z > max.z) closest.z = max.z;
    else                       closest.z = refPt.z;

    return closest;
}

inline F32 Box3F::getDistanceFromPoint(const Point3F& refPt) const
{
    Point3F vec;

    if (refPt.x < min.x)
        vec.x = min.x - refPt.x;
    else if (refPt.x > max.x)
        vec.x = refPt.x - max.x;
    else
        vec.x = 0;

    if (refPt.y < min.y)
        vec.y = min.y - refPt.y;
    else if (refPt.y > max.y)
        vec.y = refPt.y - max.y;
    else
        vec.y = 0;

    if (refPt.z < min.z)
        vec.z = min.z - refPt.z;
    else if (refPt.z > max.z)
        vec.z = refPt.z - max.z;
    else
        vec.z = 0;

    return vec.len();
}

inline void Box3F::extend(const Point3F& p)
{
#define EXTEND_AXIS(AXIS)    \
if (p.AXIS < min.AXIS)       \
   min.AXIS = p.AXIS;        \
else if (p.AXIS > max.AXIS)  \
   max.AXIS = p.AXIS;

    EXTEND_AXIS(x)
        EXTEND_AXIS(y)
        EXTEND_AXIS(z)

#undef EXTEND_AXIS
}
//------------------------------------------------------------------------------
/// Clone of Box3F, using 3D types.
///
/// 3D types use F64.
///
/// @see Box3F
class Box3D
{
public:
    Point3D min;
    Point3D max;

public:
    Box3D() { }
    Box3D(const Point3D& in_rMin, const Point3D& in_rMax, const bool in_overrideCheck = false);

    bool isContained(const Point3D& in_rContained) const;
    bool isOverlapped(const Box3D& in_rOverlap) const;

    F64 len_x() const;
    F64 len_y() const;
    F64 len_z() const;

    void intersect(const Box3D& in_rIntersect);
    void getCenter(Point3D* center) const;
};

inline Box3D::Box3D(const Point3D& in_rMin, const Point3D& in_rMax, const bool in_overrideCheck)
    : min(in_rMin),
    max(in_rMax)
{
    if (in_overrideCheck == false) {
        min.setMin(in_rMax);
        max.setMax(in_rMin);
    }
}

inline bool Box3D::isContained(const Point3D& in_rContained) const
{
    return (in_rContained.x >= min.x && in_rContained.x < max.x) &&
        (in_rContained.y >= min.y && in_rContained.y < max.y) &&
        (in_rContained.z >= min.z && in_rContained.z < max.z);
}

inline bool Box3D::isOverlapped(const Box3D& in_rOverlap) const
{
    if (in_rOverlap.min.x > max.x ||
        in_rOverlap.min.y > max.y ||
        in_rOverlap.min.z > max.z)
        return false;
    if (in_rOverlap.max.x < min.x ||
        in_rOverlap.max.y < min.y ||
        in_rOverlap.max.z < min.z)
        return false;
    return true;
}

inline F64 Box3D::len_x() const
{
    return max.x - min.x;
}

inline F64 Box3D::len_y() const
{
    return max.y - min.y;
}

inline F64 Box3D::len_z() const
{
    return max.z - min.z;
}

inline void Box3D::intersect(const Box3D& in_rIntersect)
{
    min.setMin(in_rIntersect.min);
    max.setMax(in_rIntersect.max);
}

inline void Box3D::getCenter(Point3D* center) const
{
    center->x = (min.x + max.x) * 0.5;
    center->y = (min.y + max.y) * 0.5;
    center->z = (min.z + max.z) * 0.5;
}

/// Bounding Box
///
/// A helper class for working with boxes. It runs at F32 precision.
///
/// @see Box3D
class Box3I
{
public:
    Point3I min; ///< Minimum extents of box
    Point3I max; ///< Maximum extents of box

public:
    Box3I() { }

    /// Create a box from two points.
    ///
    /// Normally, this function will compensate for mismatched
    /// min/max values. If you know your values are valid, you
    /// can set in_overrideCheck to true and skip this.
    ///
    /// @param   in_rMin          Minimum extents of box.
    /// @param   in_rMax          Maximum extents of box.
    /// @param   in_overrideCheck  Pass true to skip check of extents.
    Box3I(const Point3I& in_rMin, const Point3I& in_rMax, const bool in_overrideCheck = false);

    /// Create a box from six extent values.
    ///
    /// No checking is performed as to the validity of these
    /// extents, unlike the other constructor.
    Box3I(S32 xmin, S32 ymin, S32 zmin, S32 max, S32 ymax, S32 zmax);

    /// Check to see if a point is contained in this box.
    bool isContained(const Point3I& in_rContained) const;

    /// Check to see if another box overlaps this box.
    bool isOverlapped(const Box3I& in_rOverlap) const;

    /// Check to see if another box is contained in this box.
    bool isContained(const Box3I& in_rContained) const;

    S32 len_x() const;
    S32 len_y() const;
    S32 len_z() const;

    /// Perform an intersection operation with another box
    /// and store the results in this box.
    void intersect(const Box3I& in_rIntersect);

    /// Get the center of this box.
    ///
    /// This is the average of min and max.
    void getCenter(Point3I* center) const;

    /// Check that the box is valid.
    ///
    /// Currently, this just means that min < max.
    bool isValidBox() const
    {
        return (min.x <= max.x) &&
            (min.y <= max.y) &&
            (min.z <= max.z);
    }
};

inline Box3I::Box3I(const Point3I& in_rMin, const Point3I& in_rMax, const bool in_overrideCheck)
    : min(in_rMin),
    max(in_rMax)
{
    if (in_overrideCheck == false)
    {
        min.setMin(in_rMax);
        max.setMax(in_rMin);
    }
}

inline Box3I::Box3I(S32 xMin, S32 yMin, S32 zMin, S32 xMax, S32 yMax, S32 zMax)
    : min(xMin, yMin, zMin),
    max(xMax, yMax, zMax)
{
}

inline bool Box3I::isContained(const Point3I& in_rContained) const
{
    return (in_rContained.x >= min.x && in_rContained.x < max.x) &&
        (in_rContained.y >= min.y && in_rContained.y < max.y) &&
        (in_rContained.z >= min.z && in_rContained.z < max.z);
}

inline bool Box3I::isOverlapped(const Box3I& in_rOverlap) const
{
    if (in_rOverlap.min.x > max.x ||
        in_rOverlap.min.y > max.y ||
        in_rOverlap.min.z > max.z)
        return false;
    if (in_rOverlap.max.x < min.x ||
        in_rOverlap.max.y < min.y ||
        in_rOverlap.max.z < min.z)
        return false;
    return true;
}

inline bool Box3I::isContained(const Box3I& in_rContained) const
{
    return (min.x <= in_rContained.min.x) &&
        (min.y <= in_rContained.min.y) &&
        (min.z <= in_rContained.min.z) &&
        (max.x >= in_rContained.max.x) &&
        (max.y >= in_rContained.max.y) &&
        (max.z >= in_rContained.max.z);
}

inline S32 Box3I::len_x() const
{
    return max.x - min.x;
}

inline S32 Box3I::len_y() const
{
    return max.y - min.y;
}

inline S32 Box3I::len_z() const
{
    return max.z - min.z;
}

inline void Box3I::intersect(const Box3I& in_rIntersect)
{
    min.setMin(in_rIntersect.min);
    max.setMax(in_rIntersect.max);
}

inline void Box3I::getCenter(Point3I* center) const
{
    center->x = (min.x + max.x) / 0.5;
    center->y = (min.y + max.y) / 0.5;
    center->z = (min.z + max.z) / 0.5;
}

#endif // _DBOX_H_
