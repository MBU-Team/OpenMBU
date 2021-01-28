#ifndef __DTS_SHAPE_H
#define __DTS_SHAPE_H

#include "DTSPoint.h"
#include "DTSQuaternion.h"
#include "DTSMesh.h"

namespace DTS
{
   //! Defines a tree node, as stored in the DTS file

   struct Node
   {
      int name ;           //!< Index of its name in the DTS string table
      int parent ;         //!< Number of the parent node, -1 if it is root
      int firstObject ;    //!< Deprecated: set to -1
      int child ;          //!< Deprecated: set to -1
      int sibling ;        //!< Deprecated: set to -1
      Node() {
         // These values unused in data file
         firstObject = child = sibling = -1;
      }
   };

   //! Defines an object, as stored in the DTS file

   struct Object
   {
      int name ;           //!< Index of its name in the DTS string table
      int numMeshes ;      //!< Number of meshes (only one mesh is used for detail level)
      int firstMesh ;      //!< Number of the first mesh (they must be consecutive)
      int node ;           //!< Number of the node where the object is stored
      int sibling ;        //!< Deprecated: set to -1.
      int firstDecal ;     //!< Deprecated: set to -1
      Object() {
         // These values unused in data file
         firstDecal = sibling = -1;
      }
   };

   //! Defines a decal, as stored in the DTS file. DEPRECATED.

   struct Decal
   {
      int name ;
      int numMeshes ;
      int firstMesh ;
      int object ;
      int sibling ;
   };

   //! Defines a material, as stored in the DTS file

   struct Material
   {
      std::string name ;   //!< Texture name. Materials don't use the DTS string table

      int  flags ;         //!< Boolean properties
      int  reflectance ;   //!< Number of reflectance map (?)
      int  bump ;          //!< Number of bump map (?) or -1 if none
      int  detail ;        //!< Number of detail map (?) or -1 if none
      float detailScale ;  //!< ?
      float reflection ;   //!< ?

      enum //!< Material flags
      {
         SWrap             = 0x00000001,
         TWrap             = 0x00000002,
         Translucent       = 0x00000004,
         Additive          = 0x00000008,
         Subtractive       = 0x00000010,
         SelfIlluminating  = 0x00000020,
         NeverEnvMap       = 0x00000040,
         NoMipMap          = 0x00000080,
         MipMapZeroBorder  = 0x00000100,
         IFLMaterial       = 0x00000000,
         IFLFrame          = 0x10000000,
         DetailMap         = 0x20000000,
         BumpMap           = 0x40000000,
         ReflectanceMap    = 0x80000000,
         AuxiliaryMask     = 0xF0000000
      };
   };

   //! Defines an animated material as stored in the DTS file

   struct IFLMaterial
   {
      int name ;
      int slot ;
      int firstFrame ;
      int time ;
      int numFrames ;
   };

   //! Defines a detail level as stored in the DTS file

   struct DetailLevel
   {
      int   name ;            //!< Index of the name in the DTS string table
      int   subshape ;        //!< Number of the subshape it belongs
      int   objectDetail ;    //!< Number of object mesh to draw for each object
      float size ;            //!< Minimum pixel size (store details from big to small)
      float avgError ;        //!< Don't know, use -1
      float maxError ;        //!< Don't know, use -1
      int   polyCount ;       //!< Polygon count of the meshes to draw.
   };  

   //! Defines a subshape as stored in the DTS file. A subshape defines a range
   //! of nodes and objects.

   struct Subshape
   {
      int firstNode ;
      int firstObject ;
      int firstDecal ;
      int numNodes ;
      int numObjects ;
      int numDecals ;
      int firstTranslucent ;  //!< Not used/stored (?)
   };

   //! Defines an object state as stored in the DTS file.
   //! Not sure about what does, but it may be related to animated materials.

   struct ObjectState
   {
      float vis ;
      int frame ;
      int matFrame ;
   };

   //! Defines a decal state as stored in the DTS file. DEPRECATED.

   struct DecalState
   {
      int frame ;
   };

   //! Defines a trigger as stored in the DTS file.

   struct Trigger
   {
      int state ;
      float pos ;
   };

   //! Defines a sequence as stored in the DTS file

   struct Sequence
   {
      int    nameIndex ;
      int    flags ;
      int    numKeyFrames ;
      float  duration ;
      int    priority ;
      int    firstGroundFrame ;
      int    numGroundFrames ;
      int    baseRotation ;
      int    baseTranslation ;
      int    baseScale ;
      int    baseObjectState ;
      int    baseDecalState ;
      int    firstTrigger ;
      int    numTriggers ;
      float  toolBegin ;
      
      enum  /* flags */
      {
         UniformScale    = 0x0001,
         AlignedScale    = 0x0002,
         ArbitraryScale  = 0x0004,
         Blend           = 0x0008,
         Cyclic          = 0x0010,
         MakePath        = 0x0020,
         IFLInit         = 0x0040,
         HasTranslucency = 0x0080
      };

      struct matters_array {
         std::vector<bool> rotation ;
         std::vector<bool> translation ;
         std::vector<bool> scale ;
         std::vector<bool> decal ;
         std::vector<bool> ifl ;
         std::vector<bool> vis ;
         std::vector<bool> frame ;
         std::vector<bool> matframe ;
      }
      matters ;

      Sequence() ;
   };

   //! Defines a DTS shape

   class Shape
   {
   public:

      enum //! ImportConfig collision types
      {
         C_None = 0,
         C_BBox,
         C_Cylinder,
         C_Mesh
      } ;
      
      //! Create an empty shape
      Shape () ;

      //! Write the shape to a file or stream
      void save (std::ostream & out) const ;

      //! Read the shape from a file or stream
      void read (std::istream & in) ;

      Box   getBounds() const     { return bounds; }
      float getRadius() const     { return radius; }
      float getTubeRadius() const { return tubeRadius; }

   protected:

      int  addName (std::string s) ;

      void calculateBounds() ;
      void calculateRadius() ;
      void calculateTubeRadius() ;
      void calculateCenter() ;
      void setSmallestSize(int) ;
      void setCenter(Point &p)     { center = p; }
      void getNodeWorldPosRot(int n,Point &trans, Quaternion &rot);
      void write(std::ostream & out,const std::vector<bool> * ptr) const;

   protected:

      std::vector <Node>         nodes ;
      std::vector <Object>       objects ;
      std::vector <Decal>        decals ;
      std::vector <Subshape>     subshapes ;
      std::vector <IFLMaterial>  IFLmaterials ;
      std::vector <Material>     materials ;
      std::vector <Quaternion>   nodeDefRotations ;
      std::vector <Point>        nodeDefTranslations ;
      std::vector <Quaternion>   nodeRotations ;
      std::vector <Point>        nodeTranslations ;
      std::vector <float>        nodeScalesUniform ;
      std::vector <Quaternion>   nodeScalesAligned ;
      std::vector <Point>        nodeScalesArbitrary ;
      std::vector <Quaternion>   groundRotations ;
      std::vector <Point>        groundTranslations ;
      std::vector <ObjectState>  objectStates ;
      std::vector <DecalState>   decalStates ;
      std::vector <Trigger>      triggers ;
      std::vector <DetailLevel>  detailLevels ;
      std::vector <Mesh>         meshes ;
      std::vector <Sequence>     sequences ;
      std::vector <std::string>  names ;

   private:

      float smallestSize ;
      int   smallestDetailLevel ;

      float radius ;
      float tubeRadius ;
      Point center ;
      Box   bounds ;

   };
}

#endif