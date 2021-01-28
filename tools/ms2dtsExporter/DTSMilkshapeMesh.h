#ifndef __DTSMILKSHAPEMESH_H
#define __DTSMILKSHAPEMESH_H

#include "DTSShape.h"
#include "DTSQuaternion.h"

#include "msLib.h"

namespace DTS
{
   //! Defines a point imported from Milkshape coordinates

   struct MilkshapePoint : public Point
   {
      MilkshapePoint(const msVec3 p) : Point(-p[0], p[2], p[1]) {}
   };

   //! Defines a quaternion imported from Milkshape angles

   struct MilkshapeQuaternion : public Quaternion
   {
      // Torque uses column vectors which means we need to flip
      // the rotations
      MilkshapeQuaternion(const msVec3 p) : Quaternion (p[0], -p[2], -p[1]) {}
   };

   //! Defines a Mesh imported from Milkshape

   class MilkshapeMesh : public Mesh
   {
   public:
      //! Create a standard mesh from a Milkshape group
      MilkshapeMesh (msMesh *, int rootBone, float scaleFactor, bool WithMaterials) ;
   };

}

#endif