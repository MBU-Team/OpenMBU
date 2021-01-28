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

//------------------------------------------------------------------------------
// Creates orientation matrix from a direction vector.  Assumes ( 0 0 1 ) is up.
//------------------------------------------------------------------------------
MatrixF createOrientFromDir( Point3F &direction )
{
	Point3F j = direction;
	Point3F k(0.0, 0.0, 1.0);
	Point3F i;
	
	mCross( j, k, &i );

	if( i.magnitudeSafe() == 0.0 )
	{
		i = Point3F( 0.0, -1.0, 0.0 );
	}

	i.normalizeSafe();
	mCross( i, j, &k );

   MatrixF mat( true );
   mat.setColumn( 0, i );
   mat.setColumn( 1, j );
   mat.setColumn( 2, k );

	return mat;
}


//------------------------------------------------------------------------------
// Creates random direction given angle parameters similar to the particle system.
// The angles are relative to the specified axis.
//------------------------------------------------------------------------------
Point3F randomDir( Point3F &axis, F32 thetaAngleMin, F32 thetaAngleMax,
                                 F32 phiAngleMin, F32 phiAngleMax )
{
   MatrixF orient = createOrientFromDir( axis );
   Point3F axisx;
   orient.getColumn( 0, &axisx );

   F32 theta = (thetaAngleMax - thetaAngleMin) * sgRandom.randF() + thetaAngleMin;
   F32 phi = (phiAngleMax - phiAngleMin) * sgRandom.randF() + phiAngleMin;

   // Both phi and theta are in degs.  Create axis angles out of them, and create the
   //  appropriate rotation matrix...
   AngAxisF thetaRot(axisx, theta * (M_PI / 180.0));
   AngAxisF phiRot(axis,    phi   * (M_PI / 180.0));

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
void getAnglesFromVector( VectorF &vec, F32 &yawAng, F32 &pitchAng )
{
   yawAng = mAtan( vec.x, vec.y );

   if( yawAng < 0.0 )
   {
      yawAng += M_2PI;
   }

   // Rotate out x component, then compare angle between
   // y and z components.  Note - can almost certainly be 
   // done faster with some other way, ie, dot products.
   MatrixF rotMat( EulerF( 0.0, 0.0, -yawAng) );
   Point3F newVec = vec;
   rotMat.mulV( newVec );
   pitchAng = mAtan( (newVec.z), (newVec.y) );


}


//------------------------------------------------------------------------------
// Returns vector from given yaw and pitch angles.  Angles are in RADIANS.
// Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
// The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
// ASSUMES Z AXIS IS UP
//------------------------------------------------------------------------------
void getVectorFromAngles( VectorF &vec, F32 &yawAng, F32 &pitchAng )
{
   VectorF  pnt( 0.0, 1.0, 0.0 );

   EulerF   rot( -pitchAng, 0.0, 0.0 );
   MatrixF  mat( rot );

   rot.set( 0.0, 0.0, yawAng );
   MatrixF   mat2( rot );

   mat.mulV( pnt );
   mat2.mulV( pnt );

   vec = pnt;

}


// transform bounding box making sure to keep original box entirely contained - JK...
void transformBoundingBox(const Box3F &sbox, const MatrixF &mat, Box3F &dbox)
{
	Point3F center;
	Point3F points[8];

	// set transformed center...
	sbox.getCenter(&center);
	mat.mulP(center);
	dbox.min = center;
	dbox.max = center;

	Point3F val;
	for(U32 ix=0; ix<2; ix++)
	{
		if(ix & 0x1)
			val.x = sbox.min.x;
		else
			val.x = sbox.max.x;

		for(U32 iy=0; iy<2; iy++)
		{
			if(iy & 0x1)
				val.y = sbox.min.y;
			else
				val.y = sbox.max.y;

			for(U32 iz=0; iz<2; iz++)
			{
				if(iz & 0x1)
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


} // end namespace MathUtils
