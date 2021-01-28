#pragma warning ( disable: 4786 )
#include <windows.h>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <cassert>

#include "DTSShape.h"
#include "DTSBrushMesh.h"
#include "DTSMilkshapeMesh.h"
#include "DTSMilkshapeShape.h"

#include "msLib.h"

namespace DTS
{
   
   // -----------------------------------------------------------------------
   // Interpolation methods used to compute tween frames - phdana

   void lerpTranslation(Point *out, Point &a, Point&b, float alpha)
   {
      out->x(a.x() + (b.x()-a.x()) * alpha);
      out->y(a.y() + (b.y()-a.y()) * alpha);
      out->z(a.z() + (b.z()-a.z()) * alpha);
   }

   void lerpTranslation(Point *out, Point &a, Point &b, int outFrame, int aFrame, int bFrame)
   {
      float alpha = (float)(outFrame - aFrame) / (float)(bFrame - aFrame);

      lerpTranslation(out,a,b,alpha);
   }

   void lerpRotation(Quaternion *out, Quaternion &q1, Quaternion &q2, float alpha)
   {
      //-----------------------------------
      // Calculate the cosine of the angle:

      double cosOmega = q1.x() * q2.x() + q1.y() * q2.y() + q1.z() * q2.z() + q1.w() * q2.w();

      //-----------------------------------
      // adjust signs if necessary:

      float sign2;
      if ( cosOmega < 0.0 )
      {
         cosOmega = -cosOmega;
         sign2 = -1.0f;
      }
      else
         sign2 = 1.0f;

      //-----------------------------------
      // calculate interpolating coeffs:

      double scale1, scale2;
      if ( (1.0 - cosOmega) > 0.00001 )
      {
         // standard case
         double omega = acos(cosOmega);
         double sinOmega = sin(omega);
         scale1 = sin((1.0 - alpha) * omega) / sinOmega;
         scale2 = sign2 * sin(alpha * omega) / sinOmega;
      }
      else
      {
         // if quats are very close, just do linear interpolation
         scale1 = 1.0 - alpha;
         scale2 = sign2 * alpha;
      }


      //-----------------------------------
      // actually do the interpolation:

      out->x( float(scale1 * q1.x() + scale2 * q2.x()));
      out->y( float(scale1 * q1.y() + scale2 * q2.y()));
      out->z( float(scale1 * q1.z() + scale2 * q2.z()));
      out->w( float(scale1 * q1.w() + scale2 * q2.w()));
   }

   void lerpRotation(Quaternion *out, Quaternion &a, Quaternion &b,
      int outFrame, int aFrame, int bFrame)
   {
      float alpha = (float)(outFrame - aFrame) / (float)(bFrame - aFrame);

      lerpRotation(out,a,b,alpha);
   }

   // -----------------------------------------------------------------------

   int extractMeshFlags(char * name)
   {
      // Extract comma seperated flags embeded in the name. Anything after
      // the ":" in the name is assumed to be flag arguments and stripped
      // off the name.
      int flags = 0;
      char *ptr = strstr(name,":");
      if (ptr)
      {
         *ptr = 0;
         for (char *tok = strtok(ptr+1,","); tok != 0; tok = strtok(0,","))
         {
            // Strip lead/trailing white space
            for (; isspace(*tok); tok++) ;
            for (char* itr = tok + strlen(tok)-1; itr > tok && isspace(*itr); itr--)
               *itr = 0;

            //
            if (!stricmp(tok,"billboard"))
               flags |= Mesh::Billboard;
               else
            if (!stricmp(tok,"billboardz"))
               flags |= Mesh::Billboard | Mesh::BillboardZ;
               else
            if (!stricmp(tok,"enormals"))
               flags |= Mesh::EncodedNormals;
         }
      }
      return flags;
   }

   void extractMaterialFlags(char * name, int& flags)
   {
      // Extract comma seperated flags embeded in the name. Anything after
      // the ":" in the name is assumed to be flag arguments and stripped
      // off the name.
      char *ptr = strstr(name,":");
      if (ptr)
      {
         *ptr = 0;
         for (char *tok = strtok(ptr+1,","); tok != 0; tok = strtok(0,","))
         {
            // Strip lead/trailing white space
            for (; isspace(*tok); tok++) ;
            for (char* itr = tok + strlen(tok)-1; itr > tok && isspace(*itr); itr--)
               *itr = 0;

			if (!stricmp(tok,"add"))
				flags |= Material::Additive;
				else
			if (!stricmp(tok,"sub"))
                flags |= Material::Subtractive;
				else
			if (!stricmp(tok,"illum"))
                flags |= Material::SelfIlluminating;
				else
			if (!stricmp(tok,"nomip"))
				flags |= Material::NoMipMap;
				else
			if (!stricmp(tok,"mipzero"))
				flags |= Material::MipMapZeroBorder;
         }
      }
   }

   // -----------------------------------------------------------------------

   bool isBoneAnimated(msBone *bone, int start, int end)
   {
      // Returns true if the bone contains any key frames
      // within the given time range.
      int numPKeys  = msBone_GetPositionKeyCount(bone);
      for (int j = 0 ; j < numPKeys ; j++) {
         msPositionKey * msKey = msBone_GetPositionKeyAt (bone, j);
         // MS is one based, start/end 0 based..
         if (msKey->fTime > start && msKey->fTime <= end)
            return true;
      }
      int numRKeys  = msBone_GetRotationKeyCount(bone);
      for (int i = 0 ; i < numRKeys ; i++)
      {
         msRotationKey * msKey = msBone_GetRotationKeyAt (bone, i);
         // MS is one based, start/end 0 based..
         if (msKey->fTime > start && msKey->fTime <= end)
            return true;
      }
      return false;
   }


   // -----------------------------------------------------------------------
   //  Imports a Milkshape Model
   // -----------------------------------------------------------------------

   MilkshapeShape::ImportConfig::ImportConfig ()
   {
      withMaterials       = true ;
      collisionType       = C_None ;
      collisionComplexity = 0.2f ; 
      collisionVisible    = false ;
      animation           = true ;
      reset();
   }

   void MilkshapeShape::ImportConfig::reset()
   {
      // Reset values that must be specified using opt: or seq:
      scaleFactor         = 0.1f ;
      minimumSize         = 0 ;
      animationFPS        = 15 ;
      animationCyclic     = false;
      sequence.resize(0);
   }


   MilkshapeShape::MilkshapeShape (msModel * model, MilkshapeShape::ImportConfig config) 
   {
      int  i, j ;
      char buffer[512] ;

      int numBones     = msModel_GetBoneCount(model) ;
      int numMeshes    = msModel_GetMeshCount(model) ;
      int numMaterials = msModel_GetMaterialCount(model) ;

      // --------------------------------------------------------
      //  Materials (optional)
      // --------------------------------------------------------

      if (config.withMaterials)
      {
         for (i = 0 ; i < numMaterials ; i++)
         {
            msMaterial * msMat = msModel_GetMaterialAt(model, i) ;
            char         textureName[MS_MAX_PATH+1] ;

            // Check for "reserved" names first.  Reserved names
            // are special materials used to encode exporter options...
            // a total hack, but what can you do?
            msMaterial_GetName (msMat, textureName, MS_MAX_PATH);
            if (!strncmp(textureName,"seq:",4) || !strncmp(textureName,"opt:",4))
               continue;

            // If no texture, use the material name
            msMaterial_GetDiffuseTexture (msMat, textureName, MS_MAX_PATH) ;
            if (!textureName[0])
            {
               msMaterial_GetName (msMat, textureName, MS_MAX_PATH) ;

               // If no name, create one
               if (!textureName[0])
                  strcpy (textureName, "Unnamed") ;
            }
            
            // Strip texture file extension
            char * ptr = textureName + strlen(textureName) ;
            while (ptr > textureName && *ptr != '.' && *ptr != '\\') ptr-- ;
            if (*ptr == '.') *ptr = '\0' ;
            
            // Strip texture file path 
            ptr = textureName + strlen(textureName) ;
            while (ptr > textureName && *ptr != '\\') ptr-- ;
            if (*ptr == '\\') memmove (textureName, ptr+1, strlen(ptr)+1) ;

            // Create the material
            Material mat ;
            mat.name        = textureName ;
            mat.flags       = Material::SWrap | Material::TWrap ;
            mat.reflectance = materials.size() ;
            mat.bump        = -1 ;
            mat.detail      = -1 ;
            mat.detailScale = 1.0f ;

            // take the strength of the enviro mapping from the shininess
            float shininess = msMaterial_GetShininess(msMat) / 128.0f;
            if (shininess > 0.0f)
            {
                mat.reflection = shininess;
            }
            else
            {
                mat.flags |= Material::NeverEnvMap;
                mat.reflection = 0.0f;
            }

            // look for tags in material name.. the tags are short because the max
            // length of a material name is only 32 chars
            char materialName[MS_MAX_PATH+1];			
            msMaterial_GetName (msMat, materialName, MS_MAX_PATH);
            extractMaterialFlags(materialName, mat.flags);


            // now check for transparency < 1.0
             if (msMaterial_GetTransparency (msMat) < 1.0f)
                mat.flags |= Material::Translucent;
                
                

            materials.push_back(mat) ;
         }
      }

      // --------------------------------------------------------
      //  Nodes & Bones
      // --------------------------------------------------------

      Node n ;

      // Loop through first and import all the bones.  I assume here
      // that the parent joints always exist before their children.
      // The Torque engine certainly expects nodes to be organized
      // that way. Other parts of the code assume that Milkshape
      // Joint index = Torque node index
      assert(!nodes.size());
      for (int bone_num = 0 ; bone_num < numBones ; bone_num++)
      {
         char bone_name[256];
         char parent_name[256];
         msBone * bone = msModel_GetBoneAt (model, bone_num) ;
         msBone_GetName (bone, bone_name, 256) ;
         msBone_GetParentName (bone, parent_name, 256);

         // Create a node for this bone
         n.parent = msModel_FindBoneByName (model, parent_name); 
         n.name = addName (bone_name) ;
         nodes.push_back (n) ;

         // Create the default position and rotation for the node
         msVec3 pos ;
         msVec3 rot ;
         msBone_GetPosition (bone, pos) ;
         msBone_GetRotation (bone, rot) ;
         nodeDefTranslations.push_back(MilkshapePoint(pos) * config.scaleFactor) ;
         nodeDefRotations.push_back(MilkshapeQuaternion(rot)) ;
      }

      // Add a default "root" node for shapes.  All meshes need to
      // be assigned to a node and the Root will be used to catch
      // any mesh not assigned to a bone.
      n.name        = addName ("Root") ;
      n.parent      = -1;
      nodes.push_back(n) ;

      // Node indexes map one to one with the default rotation and
      // tranlation arrays: push identity transform for the Root.
      nodeDefRotations.push_back(Quaternion(0,0,0,1)) ;
      nodeDefTranslations.push_back(Point(0,0,0)) ;


      // --------------------------------------------------------
      //  Mesh objects.
      // --------------------------------------------------------

      Object o ;

      for (i = 0 ; i < numMeshes ; i++)
      {
         msMesh * mesh = msModel_GetMeshAt(model, i) ;
         msMesh_GetName (mesh, buffer, 510) ;
         int flags = extractMeshFlags(buffer);

         // Ignore meshes reserved for collision
         if (!stricmp(buffer,"Collision"))
            continue;

         // Get object struct ready.  Objects are entities that
         // represent renderable items. Objects can have more
         // than one mesh to represent different detail levels.
         o.name      = addName(buffer) ;
         o.numMeshes = 1 ;
         o.firstMesh = meshes.size() ;
         o.node = nodes.size() - 1;

         // Process the raw data.
         meshes.push_back(MilkshapeMesh(mesh, o.node, config.scaleFactor, config.withMaterials)) ;
         Mesh& m = meshes.back();
         m.setFlag(flags);

         // Rigid meshes can be attached to a single node, in which
         // case we need to transform the vertices into the node's
         // local space.
         if (m.getType() == Mesh::T_Standard)
         {
            o.node = m.getNodeIndex(0);
            if (o.node != -1) {
               // Transform the mesh into node space.  The mesh vertices
               // must all be relative to the bone their attached to.
               Quaternion world_rot;
               Point world_trans;
               getNodeWorldPosRot(o.node,world_trans, world_rot);
               m.translate(-world_trans);
               m.rotate(world_rot.inverse());
            }
         }

         // Skin meshes need transform information to be able to
         // transform vertices into bone space (should fix the Torque
         // engine to calculate these at load time).
         if (m.getType() == Mesh::T_Skin)
         {
            for (int n = 0; n < m.getNodeIndexCount(); n++)
            {
               Quaternion world_rot;
               Point world_trans;
               getNodeWorldPosRot(m.getNodeIndex(n),world_trans, world_rot);
               m.setNodeTransform(n,world_trans,world_rot);
            }
         }

         objects.push_back(o) ;
      }

      // --------------------------------------------------------
      //  Detail levels 
      //  Not fully supported, we'll just export all our current
      //  stuff as single detail level.
      // --------------------------------------------------------

      DetailLevel dl ;
      dl.name         = addName("Detail-1") ;
      dl.size         = 0 ; // Projected size of detail level
      dl.objectDetail = 0 ;
      dl.polyCount    = 0 ;
      dl.subshape     = 0 ;
      dl.maxError     = -1 ;
      dl.avgError     = -1 ;

      std::vector<Mesh>::iterator mesh_ptr ;
      for (mesh_ptr = meshes.begin() ; mesh_ptr != meshes.end() ; mesh_ptr++)
         dl.polyCount += mesh_ptr->getPolyCount() ;

      detailLevels.push_back(dl) ;

      // --------------------------------------------------------
      //  Collision detail levels and meshes (optional)
      // --------------------------------------------------------

      // Update our bounds values as we might use them to build
      // a collision mesh.
      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
      calculateTubeRadius() ;

      // Export every mesh called "Collision" as a collision mesh.
      if (config.collisionType != Shape::C_None)
      {
         // Material for visible meshes
         int meshMaterial = materials.size();
         if (config.collisionVisible) 
         {
               Material mat ;
               mat.name        = "Collision_Mesh" ;
               mat.flags       = Material::NeverEnvMap | Material::SelfIlluminating;
               mat.reflectance = materials.size() ;
               mat.bump        = -1 ;
               mat.detail      = -1 ;
               mat.reflection  = 1.0f ;
               mat.detailScale = 1.0f ;
               materials.push_back(mat) ;
         }
         
         // Loop through all the meshes and extract those that will be used for collision
         if (config.collisionType == Shape::C_Mesh) 
         {
            int numCollisions = 1;
            for (i = 0 ; i < numMeshes ; i++)
            {
               msMesh * mesh = msModel_GetMeshAt(model, i);
               msMesh_GetName (mesh, buffer, 510) ;
               if (stricmp(buffer,"Collision"))
                   continue;
               Mesh* collmesh = new MilkshapeMesh(mesh, nodes.size() - 1, config.scaleFactor, true) ;
            
               // Create the collision detail level
               char detailName[256];
               sprintf(detailName,"Collision-%d",numCollisions++);
               dl.name         = addName(detailName) ;
               dl.size         = -1 ; // -1 sized detail levels are never rendered
               dl.objectDetail = detailLevels.size() ;
               dl.subshape     = 0 ;
               dl.polyCount    = collmesh->getPolyCount() ;
               detailLevels.push_back(dl) ;
            
               // Create an object for the collision mesh and attach it to
               // the "Root" node.
               o.name         = addName("Col") ;
               o.node         = nodes.size() - 1 ;
               o.firstMesh    = meshes.size() ;
               o.numMeshes    = dl.objectDetail + 1 ;
               objects.push_back(o) ;
      
               // Create a renderable copy of the collision meshes
               // for visible detail levels, or a null placeholder
               for (i = 0 ; i < dl.objectDetail ; i++)
                  if (config.collisionVisible)
                  {
                     meshes.push_back(*collmesh) ;
                     meshes[meshes.size()-1].setMaterial(meshMaterial) ;
                  }
                  else
                     meshes.push_back(Mesh(Mesh::T_Null)) ;
      
               // Add the mesh to the list 
               meshes.push_back(*collmesh);
            }
         }
         else 
         {
            // Let's create the specified collision mesh type
            Mesh* collmesh;
            switch (config.collisionType)
            {
               case C_BBox:
                  collmesh = new BoxMesh(getBounds()) ;
                  break ;
               case C_Cylinder:
                  collmesh = new CylinderMesh(getBounds(), getTubeRadius(), config.collisionComplexity) ;
                  break ;
               default:
                  assert (0 && "Invalid collision mesh type") ;
            }
            
            // Create the collision detail level
            dl.name         = addName("Collision-1") ;
            dl.size         = -1 ; // -1 sized detail levels are never rendered
            dl.objectDetail = detailLevels.size() ;
            dl.subshape     = 0 ;
            dl.polyCount    = collmesh->getPolyCount() ;
            detailLevels.push_back(dl) ;
         
            // Create an object for the collision mesh and attach it to
            // the "Root" node.
            o.name         = addName("Col") ;
            o.node         = nodes.size() - 1 ;
            o.firstMesh    = meshes.size() ;
            o.numMeshes    = dl.objectDetail + 1 ;
            objects.push_back(o) ;
   
            // Create a renderable copy of the collision meshes
            // for visible detail levels, or a null placeholder
            for (i = 0 ; i < dl.objectDetail ; i++)
               if (config.collisionVisible)
               {
                  meshes.push_back(*collmesh) ;
                  meshes[meshes.size()-1].setMaterial(meshMaterial) ;
               }
               else
                  meshes.push_back(Mesh(Mesh::T_Null)) ;
   
            // Add the mesh to the list 
            meshes.push_back(*collmesh);
         }
      }

      // --------------------------------------------------------
      //  Subshape and final stuff
      // --------------------------------------------------------

      // Create a subshape with everything

      Subshape s ;
      s.firstObject = 0 ;
      s.firstNode   = 0 ;
      s.firstDecal  = 0 ;
      s.numNodes    = nodes.size() ;
      s.numObjects  = objects.size() ;
      s.numDecals   = decals.size() ;
      subshapes.push_back(s) ;

      // Create an object state for each object (not sure about this)

      ObjectState os ;
      os.vis      = 1.0f ;
      os.frame    = 0 ;
      os.matFrame = 0 ;
      for (i = 0 ; i < objects.size() ; i++)
         objectStates.push_back(os) ;

      // Recalculate bounds (they may have changed)

      calculateBounds() ;
      calculateCenter() ;
      calculateRadius() ;
      calculateTubeRadius() ;

      setSmallestSize(config.minimumSize) ;

      // --------------------------------------------------------
      //  Animation (optional)

      // For each sequence, Torque wants an array of frame * nodes
      // information for all nodes affected by that sequence. Node
      // animation information can be translation, rotation, scale,
      // visibility, material, etc.
      //
      // The sequence contains a "matters" array for each type of
      // animation info.  Each type has it's own array of frame * nodes
      // which contains only the nodes affected by that type of
      // information for the sequence.  Since each array is NxN,
      // if a node is animated on a single frame of the sequence, it
      // will get an entry for every frame.

      // --------------------------------------------------------

      int frameCount = msModel_GetTotalFrames (model);
      if (frameCount && numBones && config.animation)
      {
         // Process all the sequences.
         for (int sc = 0; sc < config.sequence.size(); sc++)
         {
            MilkshapeShape::ImportConfig::Sequence& si = config.sequence[sc];

            // Build the exported sequence structure
            Sequence s;
            s.flags           = Sequence::UniformScale; 
            if (si.cyclic)
               s.flags |= Sequence::Cyclic;
            s.nameIndex       = addName(si.name) ;
            s.numKeyFrames    = si.end - si.start;
            s.duration        = float(s.numKeyFrames) / si.fps;
            s.baseTranslation = nodeTranslations.size() ;
            s.baseRotation    = nodeRotations.size();

            // Count how many nodes are affected by the sequence and
            // set the sequence.matter arrays to indicate which ones.
            s.matters.translation.assign (nodes.size(), false);
            s.matters.rotation.assign    (nodes.size(), false);
            int nodeCount = 0;
            for (i = 0 ; i < numBones ; i++)
            {
               msBone * bone = msModel_GetBoneAt(model, i);
               if (isBoneAnimated(bone,si.start,si.end))
               {
                  // Milkshape seems to always produce rotation & position
                  // keys in pairs, so we'll just deal with them together.
                  s.matters.translation[i] = true;
                  s.matters.rotation[i] = true;
                  nodeCount++;
               }
            }

            // Size arrays to hold keyframe * nodeCount
            nodeTranslations.resize (nodeTranslations.size() + nodeCount * s.numKeyFrames);
            nodeRotations.resize (nodeTranslations.size());

            // Set the keyframe data for each affected bone.  Unaffected
            // bones are skipped so the final NxN array of transforms
            // and rotations is "compressed", sort of.  We could compress
            // both the rotation and the position arrays individually,
            // but MS seems to always generate the values in pairs, so
            // we'll do them together. Though the msKey loops are seperate,
            // just in case.
            int index = 0;
            for (i = 0 ; i < numBones ; i++)
            {
               msBone * bone = msModel_GetBoneAt(model, i) ;
               if (isBoneAnimated(bone,si.start,si.end))
               {
                  int numPKeys  = msBone_GetPositionKeyCount(bone);
                  int numRKeys  = msBone_GetRotationKeyCount(bone);

                  // Insert translation keys into the table.
                  Point *translations = &nodeTranslations[s.baseTranslation + index * s.numKeyFrames];
                  int lastFrame = 0;

                  for (j = 0 ; j < numPKeys ; j++)
                  {
                     msPositionKey * msKey = msBone_GetPositionKeyAt (bone, j);
                     int frame = int(msKey->fTime) - 1;

                     // Only want keys in the sequence range. If it's before
                     // our range, we'll put it into the 0 frame in case we don't
                     // get a key for that frame later.
                     if (frame >= si.end)
                        break;
                     if (frame < si.start)
                        frame = 0;
                     else
                        frame -= si.start;

                     // Store the total translation for the node and fill in
                     // the initial frame if this is the first key frame.
                     translations[frame] = nodeDefTranslations[i] +
                        nodeDefRotations[i].apply(MilkshapePoint(msKey->Position) * config.scaleFactor);
                     if (!j && frame > 0)
                        translations[0] = translations[frame];

                     // Interpolate the missing frames.
                     for (int f = lastFrame + 1; f < frame; f++)
                        lerpTranslation(&translations[f],translations[lastFrame],
                           translations[frame], f, lastFrame, frame);

                     lastFrame = frame;
                  }

                  // Duplicate the last frame to the end.
                  for (int t = lastFrame + 1; t < s.numKeyFrames; t++)
                     translations[t] = translations[lastFrame];
   
                  // Insert rotation keys into the table.
                  Quaternion *rotations = &nodeRotations[s.baseTranslation + index * s.numKeyFrames];
                  lastFrame = 0;

                  for (j = 0 ; j < numRKeys ; j++)
                  {
                     msRotationKey * msKey = msBone_GetRotationKeyAt (bone, j) ;
                     int frame = int(msKey->fTime) - 1;
   
                     // Only want keys in the sequence range. If it's before
                     // our range, we'll put it into the 0 frame in case we don't
                     // get a key for that frame later.
                     if (frame >= si.end)
                        break;
                     if (frame < si.start)
                        frame = 0;
                     else
                        frame -= si.start;

                     // Store the total rotation for the node and fill in
                     // the initial frame if this is the first key frame.
                     rotations[frame] = MilkshapeQuaternion(msKey->Rotation) * nodeDefRotations[i];
                     if (!j && frame > 0)
                        rotations[0] = rotations[frame];

                     // Interpolate the missing frames.
                     for (int f = lastFrame + 1; f < frame; f++)
                        lerpRotation(&rotations[f],rotations[lastFrame],
                           rotations[frame], f, lastFrame, frame);

                     lastFrame = frame;
                  }

                  // Duplicate the last frame to the end.
                  for (int r = lastFrame + 1; r < s.numKeyFrames; r++)
                     rotations[r] = rotations[lastFrame];

                  // Increment the position & rotation array index
                  index++;
               }
            }

            sequences.push_back(s);
         }
      }
   }

} // DTS namespace