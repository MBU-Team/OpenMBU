//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RIGID_H_
#define _RIGID_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif

//----------------------------------------------------------------------------

class Rigid
{
public:
   Point3F force;
   Point3F torque;

   Point3F linVelocity;          ///< Linear velocity
   Point3F linPosition;          ///< Current position
   Point3F linMomentum;          ///< Linear momentum
   Point3F angVelocity;          ///< Angular velocity
   QuatF   angPosition;          ///< Current rotation
   Point3F angMomentum;          ///< Angular momentum

   MatrixF objectInertia;        ///< Moment of inertia
   MatrixF invObjectInertia;     ///< Inverse moment of inertia
   MatrixF invWorldInertia;      ///< Inverse moment of inertia in world space

   Point3F centerOfMass;         ///< Center of mass in object space
   Point3F worldCenterOfMass;    ///< CofM in world space
   F32 mass;                     ///< Rigid body mass
   F32 oneOverMass;              ///< 1 / mass
   F32 restitution;              ///< Collision restitution
   F32 friction;                 ///< Friction coefficient
   bool atRest;

private:
   void translateCenterOfMass(const Point3F &oldPos,const Point3F &newPos);

public:
   //
   Rigid();
   void clearForces();
   void integrate(F32 delta);

   void updateInertialTensor();
   void updateVelocity();
   void updateCenterOfMass();

   void applyImpulse(const Point3F &v,const Point3F &impulse);
   bool resolveCollision(const Point3F& p,Point3F normal,Rigid*);
   bool resolveCollision(const Point3F& p,Point3F normal);

   F32  getZeroImpulse(const Point3F& r,const Point3F& normal);
   F32  getKineticEnergy();
   void getOriginVector(const Point3F &r,Point3F* v);
   void setCenterOfMass(const Point3F &v);
   void getVelocity(const Point3F &p,Point3F* r);
   void getTransform(MatrixF* mat);
   void setTransform(const MatrixF& mat);

   void setObjectInertia(const Point3F& r);
   void setObjectInertia();
   void invertObjectInertia();

   bool checkRestCondition();
   void setAtRest();
};


#endif
