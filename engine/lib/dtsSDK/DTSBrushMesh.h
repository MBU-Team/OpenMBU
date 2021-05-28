#ifndef __DTSBRUSHMESH_H
#define __DTSBRUSHMESH_H

#include "DTSMesh.h"

namespace DTS
{
   class BrushMesh : public Mesh
   {
      public:
         
         //! Creates an standard mesh by default
         BrushMesh() : Mesh(T_Standard) {}

         //! Creates (or recreates) the mesh
         virtual void construct() = 0 ;
         
      protected:
         
         //! Create a new vertex with convenient (altough arbitrary) normal/texture data
         void addVertex (float, float, float) ;
   };

   class CylinderMesh : public BrushMesh
   {
      public:
         
         //! Create a standard mesh as a cylinder (the box is used for the center and Z range)
         CylinderMesh (Box &, float radius, float complexity = 0.25f) ;
         
         //! Creates (or recreates) the mesh
         virtual void construct() ;
         
      private:
         
         float complexity ;
   };

   class BoxMesh : public BrushMesh
   {
      public:
         
         //! Create a standard mesh as a box
         BoxMesh (Box &) ;
         
         //! Creates (or recreates) the mesh
         virtual void construct() ;
   };
}

#endif