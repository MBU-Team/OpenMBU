//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

static U32 sCollisionMask = StaticObjectType |
                            AtlasObjectType |
                            InteriorMapObjectType |
                            ShapeBaseObjectType |
                            PlayerObjectType |
                            VehicleBlockerObjectType;

static U32 sContactMask = StaticObjectType |
                          AtlasObjectType |
                          InteriorMapObjectType |
                          ShapeBaseObjectType |
                          PlayerObjectType |
                          VehicleBlockerObjectType;

bool gMarbleAxisSet = false;
Point3F gWorkGravityDir;
Point3F gMarbleSideDir;

Point3D Marble::getVelocityD() const
{
    return mVelocity;
}

void Marble::setVelocityD(const Point3D& vel)
{
    dMemcpy(mVelocity, vel, sizeof(mVelocity));
    mSinglePrecision.mVelocity = vel;

    setMaskBits(MoveMask);
}

void Marble::setVelocityRotD(const Point3D& rot)
{
    dMemcpy(mOmega, rot, sizeof(mOmega));

    setMaskBits(MoveMask);
}

void Marble::applyImpulse(const Point3F& pos, const Point3F& vec)
{
    // TODO: Implement applyImpulse
    Parent::applyImpulse(pos, vec);
}

void Marble::clearMarbleAxis()
{
    gMarbleAxisSet = false;
    mGravityFrame.mulP(Point3F(0.0f, 0.0f, -1.0f), &gWorkGravityDir);
}

void Marble::applyContactForces(const Move* move, bool isCentered, Point3D& aControl, const Point3D& desiredOmega, F64 timeStep, Point3D& A, Point3D& a, F32& slipAmount)
{
    // TODO: Implement applyContactForces
}

void Marble::getMarbleAxis(Point3D& sideDir, Point3D& motionDir, Point3D& upDir)
{
    if (!gMarbleAxisSet)
    {
        MatrixF camMat;
        mGravityFrame.setMatrix(&camMat);


        MatrixF xRot;
        m_matF_set_euler(Point3F(mMouseY, 0, 0), xRot);

        MatrixF zRot;
        m_matF_set_euler(Point3F(0, 0, mMouseX), zRot);

        camMat.mul(zRot);
        camMat.mul(xRot);

        gMarbleMotionDir.x = camMat[1];
        gMarbleMotionDir.y = camMat[5];
        gMarbleMotionDir.z = camMat[9];

        mCross(gMarbleMotionDir, -gWorkGravityDir, gMarbleSideDir);
        m_point3F_normalize(&gMarbleSideDir.x);
        
        mCross(-gWorkGravityDir, gMarbleSideDir, gMarbleMotionDir);
        
        gMarbleAxisSet = 1;
    }

    sideDir = gMarbleSideDir;
    motionDir = gMarbleMotionDir;
    upDir = -gWorkGravityDir;
}

const Point3F& Marble::getMotionDir()
{
    Point3D side;
    Point3D motion;
    Point3D up;
    Marble::getMarbleAxis(side, motion, up);

    return gMarbleMotionDir;
}

bool Marble::computeMoveForces(Point3D& aControl, Point3D& desiredOmega, const Move* move)
{
    // TODO: Implement computeMoveForces

    return false;
}

void Marble::velocityCancel(bool surfaceSlide, bool noBounce, bool& bouncedYet, bool& stoppedPaths, Vector<PathedInterior*>& pitrVec)
{
    // TODO: Implement velocityCancel
}

Point3D Marble::getExternalForces(const Move* move, F64 timeStep)
{
    // TODO: Implement getExternalForces
    return Point3D();
}

void Marble::advancePhysics(const Move*, U32)
{
    // TODO: Implement advancePhysics
}

ConsoleMethod(Marble, setVelocityRot, bool, 3, 3, "(vel)")
{
    Point3F rot;
    dSscanf(argv[2], "%f %f %f", &rot.x, &rot.y, &rot.z);
    object->setVelocityRotD(rot);

    return 1;
}
