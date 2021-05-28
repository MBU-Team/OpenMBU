//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATHUTILS_H_
#define _MATHUTILS_H_

class Point3F;
class MatrixF;
class Box3F;
class RectI;

/// Miscellaneous math utility functions.
namespace MathUtils
{

    /// Creates orientation matrix from a direction vector.  Assumes ( 0 0 1 ) is up.
    MatrixF createOrientFromDir(Point3F& direction);

    /// Creates random direction given angle parameters similar to the particle system.
    ///
    /// The angles are relative to the specified axis. Both phi and theta are in degrees.
    Point3F randomDir(Point3F& axis, F32 thetaAngleMin, F32 thetaAngleMax, F32 phiAngleMin = 0.0, F32 phiAngleMax = 360.0);

    /// Returns yaw and pitch angles from a given vector.
    ///
    /// Angles are in RADIANS.
    ///
    /// Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    ///
    /// The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    ///
    /// <b>ASSUMES Z AXIS IS UP</b>
    void    getAnglesFromVector(VectorF& vec, F32& yawAng, F32& pitchAng);

    /// Returns vector from given yaw and pitch angles.
    ///
    /// Angles are in RADIANS.
    ///
    /// Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    ///
    /// The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    ///
    /// <b>ASSUMES Z AXIS IS UP</b>
    void    getVectorFromAngles(VectorF& vec, F32& yawAng, F32& pitchAng);

    /// Simple reflection equation - pass in a vector and a normal to reflect off of
    inline Point3F reflect(Point3F& inVec, Point3F& norm)
    {
        return inVec - norm * (mDot(inVec, norm) * 2.0);
    }

    bool capsuleCapsuleOverlap(const Point3F& a1, const Point3F& b1, F32 radius1, const Point3F& a2, const Point3F& b2, F32 radius2);
    bool capsuleSphereNearestOverlap(const Point3F& A0, const Point3F A1, F32 radA, const Point3F& B, F32 radB, F32& t);
    F32 segmentSegmentNearest(const Point3F& p1, const Point3F& q1, const Point3F& p2, const Point3F& q2, F32& s, F32& t, Point3F& c1, Point3F& c2);

    inline bool isPow2(const U32 number) { return (number & (number - 1)) == 0; }
    inline U32 getNextBinLog2(U32 number) { return getBinLog2(number) + (isPow2(number) ? 0 : 1); }
    inline U32 getNextPow2(U32 value) { return isPow2(value) ? value : (1 << (getBinLog2(value) + 1)); }


    // transform bounding box making sure to keep original box entirely contained - JK...
    void transformBoundingBox(const Box3F& sbox, const MatrixF& mat, Box3F& dbox);

    bool projectWorldToScreen(const Point3F& in, Point3F& out, const RectI& view, const MatrixF& world, const MatrixF& projection);
    void projectScreenToWorld(const Point3F& in, Point3F& out, const RectI& view, const MatrixF& world, const MatrixF& projection, const F32& far, const F32& near);
}

#endif // _MATHUTILS_H_
