#ifndef __DTSMILKSHAPESHAPE_H
#define __DTSMILKSHAPESHAPE_H

#include "DTSShape.h"

namespace DTS
{

   class MilkshapeShape : public Shape
   {
   public:

      //! Stores all the configuration data for the Milkshape constructor

      struct ImportConfig
      {
         ImportConfig() ;

         bool        withMaterials ;
         float       scaleFactor ;
         int         minimumSize ;
         int         collisionType ;
         float       collisionComplexity ;
         bool        collisionVisible ;
         bool        animation ;
         int         animationFPS ;
         bool        animationCyclic ;

         struct Sequence
         {
            std::string name;
            int start;
            int end;
            bool cyclic;
            int fps;
         };
         std::vector<Sequence> sequence;

         void reset();
      } ;

      //! Import a milkshape mesh
      MilkshapeShape (struct msModel *, ImportConfig) ; 

   };

}

#endif