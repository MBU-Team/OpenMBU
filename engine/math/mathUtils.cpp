//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMath.h"
#include "math/mathUtils.h"
#include "math/mRandom.h"

namespace MathUtils
{

    MRandomLCG sgRandom(0xdeadbeef); ///< Our random number generator.

    // Collide two capsules (sphere swept lines) against each other, reporting only if they intersect or not.
// Based on routine from "Real Time Collision Detection" by Christer Ericson pp 114.
    bool capsuleCapsuleOverlap(const Point3F& a1, const Point3F& b1, F32 rad1, const Point3F& a2, const Point3F& b2, F32 rad2)
    {
        F32 s, t;
        Point3F c1, c2;
        F32 dist = segmentSegmentNearest(a1, b1, a2, b2, s, t, c1, c2);
        return dist <= rad1 * rad1 + rad2 * rad2;
    }

    // Intersect two line segments (p1,q1) and (p2,q2), returning points on lines (c1 & c2) and line parameters (s,t).
    // Based on routine from "Real Time Collision Detection" by Christer Ericson pp 149.
    F32 segmentSegmentNearest(const Point3F& p1, const Point3F& q1, const Point3F& p2, const Point3F& q2, F32& s, F32& t, Point3F& c1, Point3F& c2)
    {
        Point3F d1 = q1 - p1;
        Point3F d2 = q2 - p2;
        Point3F r = p1 - p2;
        F32 a = mDot(d1, d1);
        F32 e = mDot(d2, d2);
        F32 f = mDot(d2, r);

        const F32 EPSILON = 0.001f;

        if (a <= EPSILON && e <= EPSILON)
        {
            s = t = 0.0f;
            c1 = p1;
            c2 = p2;
            return mDot(c1 - c2, c1 - c2);
        }

        if (a <= EPSILON)
        {
            s = 0.0f;
            t = mClampF(f / e, 0.0f, 1.0f);
        }
        else
        {
            F32 c = mDot(d1, r);
            if (e <= EPSILON)
            {
                t = 0.0f;
                s = mClampF(-c / a, 0.0f, 1.0f);
            }
            else
            {
                F32 b = mDot(d1, d2);
                F32 denom = a * e - b * b;
                if (denom != 0.0f)
                    s = mClampF((b * f - c * e) / denom, 0.0f, 1.0f);
                else
                    s = 0.0f;
                F32 tnom = b * s + f;
                if (tnom < 0.0f)
                {
                    t = 0.0f;
                    s = mClampF(-c / a, 0.0f, 1.0f);
                }
                else if (tnom > e)
                {
                    t = 1.0f;
                    s = mClampF((b - c) / a, 0.0f, 1.0f);
                }
                else
                    t = tnom / e;
            }
        }

        c1 = p1 + d1 * s;
        c2 = p2 + d2 * t;
        return mDot(c1 - c2, c1 - c2);
    }

    //------------------------------------------------------------------------------
    // Return capsule-sphere overlap.  Returns time of first overlap, where time
    // is viewed as a sphere of radius radA moving from point A0 to A1.
    //------------------------------------------------------------------------------
    bool capsuleSphereNearestOverlap(const Point3F& A0, const Point3F A1, F32 radA, const Point3F& B, F32 radB, F32& t)
    {
        Point3F V = A1 - A0;
        Point3F A0B = A0 - B;
        F32 d1 = mDot(A0B, V);
        F32 d2 = mDot(A0B, A0B);
        F32 d3 = mDot(V, V);
        F32 R2 = (radA + radB) * (radA + radB);
        if (d2 < R2)
        {
            // starting in collision state
            t = 0;
            return true;
        }
        if (d3 < 0.01f)
            // no movement, and don't start in collision state, so no collision
            return false;

        F32 b24ac = mSqrt(d1 * d1 - d2 * d3 + d3 * R2);
        F32 t1 = (-d1 - b24ac) / d3;
        if (t1 > 0 && t1 < 1.0f)
        {
            t = t1;
            return true;
        }
        F32 t2 = (-d1 + b24ac) / d3;
        if (t2 > 0 && t2 < 1.0f)
        {
            t = t2;
            return true;
        }
        if (t1 < 0 && t2>0)
        {
            t = 0;
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    // Creates orientation matrix from a direction vector.  Assumes ( 0 0 1 ) is up.
    //------------------------------------------------------------------------------
    MatrixF createOrientFromDir(Point3F& direction)
    {
        Point3F j = direction;
        Point3F k(0.0, 0.0, 1.0);
        Point3F i;

        mCross(j, k, &i);

        if (i.magnitudeSafe() == 0.0)
        {
            i = Point3F(0.0, -1.0, 0.0);
        }

        i.normalizeSafe();
        mCross(i, j, &k);

        MatrixF mat(true);
        mat.setColumn(0, i);
        mat.setColumn(1, j);
        mat.setColumn(2, k);

        return mat;
    }


    //------------------------------------------------------------------------------
    // Creates random direction given angle parameters similar to the particle system.
    // The angles are relative to the specified axis.
    //------------------------------------------------------------------------------
    Point3F randomDir(Point3F& axis, F32 thetaAngleMin, F32 thetaAngleMax,
        F32 phiAngleMin, F32 phiAngleMax)
    {
        MatrixF orient = createOrientFromDir(axis);
        Point3F axisx;
        orient.getColumn(0, &axisx);

        F32 theta = (thetaAngleMax - thetaAngleMin) * sgRandom.randF() + thetaAngleMin;
        F32 phi = (phiAngleMax - phiAngleMin) * sgRandom.randF() + phiAngleMin;

        // Both phi and theta are in degs.  Create axis angles out of them, and create the
        //  appropriate rotation matrix...
        AngAxisF thetaRot(axisx, theta * (M_PI / 180.0));
        AngAxisF phiRot(axis, phi * (M_PI / 180.0));

        Point3F ejectionAxis = axis;

        MatrixF temp(true);
        thetaRot.setMatrix(&temp);
        temp.mulP(ejectionAxis);
        phiRot.setMatrix(&temp);
        temp.mulP(ejectionAxis);

        return ejectionAxis;
    }


    //------------------------------------------------------------------------------
    // Returns yaw and pitch angles from a given vector.  Angles are in RADIANS.
    // Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    // The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    // ASSUMES Z AXIS IS UP
    //------------------------------------------------------------------------------
    void getAnglesFromVector(VectorF& vec, F32& yawAng, F32& pitchAng)
    {
        yawAng = mAtan(vec.x, vec.y);

        if (yawAng < 0.0)
        {
            yawAng += M_2PI;
        }

        // Rotate out x component, then compare angle between
        // y and z components.  Note - can almost certainly be 
        // done faster with some other way, ie, dot products.
        MatrixF rotMat(EulerF(0.0, 0.0, -yawAng));
        Point3F newVec = vec;
        rotMat.mulV(newVec);
        pitchAng = mAtan((newVec.z), (newVec.y));


    }


    //------------------------------------------------------------------------------
    // Returns vector from given yaw and pitch angles.  Angles are in RADIANS.
    // Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    // The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    // ASSUMES Z AXIS IS UP
    //------------------------------------------------------------------------------
    void getVectorFromAngles(VectorF& vec, F32& yawAng, F32& pitchAng)
    {
        VectorF  pnt(0.0, 1.0, 0.0);

        EulerF   rot(-pitchAng, 0.0, 0.0);
        MatrixF  mat(rot);

        rot.set(0.0, 0.0, yawAng);
        MatrixF   mat2(rot);

        mat.mulV(pnt);
        mat2.mulV(pnt);

        vec = pnt;

    }


    // transform bounding box making sure to keep original box entirely contained - JK...
    void transformBoundingBox(const Box3F& sbox, const MatrixF& mat, Box3F& dbox)
    {
        Point3F center;
        Point3F points[8];

        // set transformed center...
        sbox.getCenter(&center);
        mat.mulP(center);
        dbox.min = center;
        dbox.max = center;

        Point3F val;
        for (U32 ix = 0; ix < 2; ix++)
        {
            if (ix & 0x1)
                val.x = sbox.min.x;
            else
                val.x = sbox.max.x;

            for (U32 iy = 0; iy < 2; iy++)
            {
                if (iy & 0x1)
                    val.y = sbox.min.y;
                else
                    val.y = sbox.max.y;

                for (U32 iz = 0; iz < 2; iz++)
                {
                    if (iz & 0x1)
                        val.z = sbox.min.z;
                    else
                        val.z = sbox.max.z;

                    Point3F newval;
                    mat.mulP(val, &newval);
                    dbox.min.setMin(newval);
                    dbox.max.setMax(newval);
                }
            }
        }
    }

    bool projectWorldToScreen(const Point3F& in, Point3F& out, const RectI& view, const MatrixF& world, const MatrixF& projection)
    {
        MatrixF worldProjection = projection;
        worldProjection.mul(world);

        Point4F temp(in.x, in.y, in.z, 1.0f);
        worldProjection.mul(temp);
        temp.x /= temp.w;
        temp.y /= temp.w;
        temp.z /= temp.w;

        out.x = (temp.x + 1.0f) / 2.0f * view.extent.x + view.point.x;
        out.y = (1.0f - temp.y) / 2.0f * view.extent.y + view.point.y;
        out.z = temp.z;

        if (out.z < 0.0f || out.z > 1.0f)
            return false;
        return true;
    }

    void projectScreenToWorld(const Point3F& in, Point3F& out, const RectI& view, const MatrixF& world, const MatrixF& projection, const F32& far, const F32& near)
    {
        MatrixF invWorldProjection = projection;
        invWorldProjection.mul(world);
        invWorldProjection.inverse();

        Point3F vec;
        vec.x = (in.x - view.point.x) * 2.0f / view.extent.x - 1.0f;
        vec.y = -(in.y - view.point.y) * 2.0f / view.extent.y + 1.0f;
        vec.z = (near + in.z * (far - near)) / far;

        invWorldProjection.mulV(vec);
        vec *= 1.0f + in.z * far;

        invWorldProjection.getColumn(3, &out);
        out += vec;
    }

} // end namespace MathUtils
