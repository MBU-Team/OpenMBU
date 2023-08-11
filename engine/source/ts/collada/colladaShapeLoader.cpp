//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.

/*
   Resource stream -> Buffer
   Buffer -> Collada DOM
   Collada DOM -> TSShapeLoader
   TSShapeLoader installed into TSShape
*/

//-----------------------------------------------------------------------------

#include "math/mMath.h"
#include "core/tDictionary.h"
#include "core/tVector.h"
#include "core/resManager.h"
#include "core/fileStream.h"
#include "core/fileObject.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "materials/matInstance.h"
#include "materials/materialList.h"
#include "ts/tsShapeConstruct.h"
#include "ts/collada/colladaUtils.h"
#include "ts/collada/colladaShapeLoader.h"
#include "ts/collada/colladaAppNode.h"
#include "ts/collada/colladaAppMesh.h"
#include "ts/collada/colladaAppSequence.h"

using namespace ColladaUtils;

//-----------------------------------------------------------------------------

Point3F ColladaShapeLoader::unitScale;
domUpAxisType ColladaShapeLoader::upAxis;

ColladaShapeLoader::ColladaShapeLoader(domCOLLADA* _root)
   : root(_root)
{
   // Extract the global scale and up_axis from the top level <asset> element,
   F32 unit = 1.0f;
   upAxis = UPAXISTYPE_Z_UP;
   if (root->getAsset()) {
      if (root->getAsset()->getUnit())
         unit = root->getAsset()->getUnit()->getMeter();
      if (root->getAsset()->getUp_axis())
         upAxis = root->getAsset()->getUp_axis()->getValue();
   }

   // Store global scale
   unitScale.set(unit, unit, unit);
}

ColladaShapeLoader::~ColladaShapeLoader()
{
   // Delete all of the animation channels
   for (int iAnim = 0; iAnim < animations.size(); iAnim++) {
      for (int iChannel = 0; iChannel < animations[iAnim]->size(); iChannel++)
         delete (*animations[iAnim])[iChannel];
      delete animations[iAnim];
   }
   animations.clear();
}

void ColladaShapeLoader::processAnimation(const domAnimation* anim, F32& maxEndTime)
{
   const char* sRGBANames[] =   { ".R", ".G", ".B", ".A", "" };
   const char* sXYZNames[] =    { ".X", ".Y", ".Z", "" };
   const char* sXYZANames[] =   { ".X", ".Y", ".Z", ".ANGLE" };
   const char* sLOOKATNames[] = { ".POSITIONX", ".POSITIONY", ".POSITIONZ", ".TARGETX", ".TARGETY", ".TARGETZ", ".UPX", ".UPY", ".UPZ", "" };
   const char* sSKEWNames[] =   { ".ROTATEX", ".ROTATEY", ".ROTATEZ", ".AROUNDX", ".AROUNDY", ".AROUNDZ", ".ANGLE", "" };
   const char* sNullNames[] =   { "" };

   for (int iChannel = 0; iChannel < anim->getChannel_array().getCount(); iChannel++) {

      // Get the animation elements: <channel>, <sampler>
      domChannel* channel = anim->getChannel_array()[iChannel];
      domSampler* sampler = daeSafeCast<domSampler>(channel->getSource().getElement());
      if (!sampler)
         continue;

      // Find the animation channel target
      daeSIDResolver resolver(channel, channel->getTarget());
      daeElement* target = resolver.getElement();
      if (!target) {
         daeErrorHandler::get()->handleWarning(avar("Failed to resolve animation "
            "target: %s", channel->getTarget()));
         continue;
      }

      // Get the target's animation channels (create them if not already)
      if (!AnimData::getAnimChannels(target)) {
         animations.push_back(new AnimChannels);
         target->setUserData(animations.last());
      }
      AnimChannels* targetChannels = AnimData::getAnimChannels(target);

      // Add a new animation channel to the target
      targetChannels->push_back(new AnimData());
      AnimData& data = *targetChannels->last();

      for (int iInput = 0; iInput < sampler->getInput_array().getCount(); iInput++) {

         const domInputLocal* input = sampler->getInput_array()[iInput];
         const domSource* source = daeSafeCast<domSource>(input->getSource().getElement());

         // @todo:don't care about the input param names for now. Could
         // validate against the target type....
         if (dStrEqual(input->getSemantic(), "INPUT")) {
            data.input.initFromSource(source);
            // Adjust the maximum sequence end time
            maxEndTime = getMax(maxEndTime, data.input.getFloatValue((S32)data.input.size()-1));
         }
         else if (dStrEqual(input->getSemantic(), "OUTPUT"))
            data.output.initFromSource(source);
         else if (dStrEqual(input->getSemantic(), "IN_TANGENT"))
            data.inTangent.initFromSource(source);
         else if (dStrEqual(input->getSemantic(), "OUT_TANGENT"))
            data.outTangent.initFromSource(source);
         else if (dStrEqual(input->getSemantic(), "INTERPOLATION"))
            data.interpolation.initFromSource(source);
      }

      // Determine the number and offset the elements of the target value
      // targeted by this animation
      switch (target->getElementType()) {
         case COLLADA_TYPE::COLOR:        data.parseTargetString(channel->getTarget(), 4, sRGBANames);   break;
         case COLLADA_TYPE::TRANSLATE:    data.parseTargetString(channel->getTarget(), 3, sXYZNames);    break;
         case COLLADA_TYPE::ROTATE:       data.parseTargetString(channel->getTarget(), 4, sXYZANames);   break;
         case COLLADA_TYPE::SCALE:        data.parseTargetString(channel->getTarget(), 3, sXYZNames);    break;
         case COLLADA_TYPE::LOOKAT:       data.parseTargetString(channel->getTarget(), 3, sLOOKATNames); break;
         case COLLADA_TYPE::SKEW:         data.parseTargetString(channel->getTarget(), 3, sSKEWNames);   break;
         case COLLADA_TYPE::MATRIX:       data.parseTargetString(channel->getTarget(), 16, sNullNames);  break;
         case COLLADA_TYPE::FLOAT_ARRAY:  data.parseTargetString(channel->getTarget(), daeSafeCast<domFloat_array>(target)->getCount(), sNullNames); break;
         default:                         data.parseTargetString(channel->getTarget(), 1, sNullNames);   break;
      }
   }

   // Process child animations
   for (int iAnim = 0; iAnim < anim->getAnimation_array().getCount(); iAnim++)
      processAnimation(anim->getAnimation_array()[iAnim], maxEndTime);
}

void ColladaShapeLoader::enumerateScene()
{
   // Get animation clips
   Vector<const domAnimation_clip*> animationClips;
   for (int iClipLib = 0; iClipLib < root->getLibrary_animation_clips_array().getCount(); iClipLib++) {
      const domLibrary_animation_clips* libraryClips = root->getLibrary_animation_clips_array()[iClipLib];
      for (int iClip = 0; iClip < libraryClips->getAnimation_clip_array().getCount(); iClip++)
         appSequences.push_back(new ColladaAppSequence(libraryClips->getAnimation_clip_array()[iClip]));
   }

   // Process all animations => this attaches animation channels to the targeted
   // Collada elements, and determines the length of the sequence if it is not
   // already specified in the Collada <animation_clip> element
   for (int iSeq = 0; iSeq < appSequences.size(); iSeq++) {
      ColladaAppSequence* appSeq = dynamic_cast<ColladaAppSequence*>(appSequences[iSeq]);
      for (int iAnim = 0; iAnim < appSeq->getClip()->getInstance_animation_array().getCount(); iAnim++) {
         F32 maxEndTime = 0;
         domAnimation* anim = daeSafeCast<domAnimation>(appSeq->getClip()->getInstance_animation_array()[iAnim]->getUrl().getElement());
         processAnimation(anim, maxEndTime);
         if (appSeq->getEnd() == 0)
            appSeq->setEnd(maxEndTime);
      }
   }

   // Need to be able to handle scenes that don't use the normal DTS layout, so
   // detect if there is a baseXX->startXX hierarchy, and if not, just add a dummy
   // subshape with the whole scene inside.

   // First grab all of the top-level nodes
   Vector<ColladaAppNode*> sceneNodes;
   for (int iSceneLib = 0; iSceneLib < root->getLibrary_visual_scenes_array().getCount(); iSceneLib++) {
      const domLibrary_visual_scenes* libScenes = root->getLibrary_visual_scenes_array()[iSceneLib];
      for (int iScene = 0; iScene < libScenes->getVisual_scene_array().getCount(); iScene++) {
         const domVisual_scene* visualScene = libScenes->getVisual_scene_array()[iScene];
         for (int iNode = 0; iNode < visualScene->getNode_array().getCount(); iNode++)
            sceneNodes.push_back(new ColladaAppNode(visualScene->getNode_array()[iNode], 0));
      }
   }

   // Check if we have a baseXX->startXX hierarchy at the top-level
   bool isDTSScene = false;
   for (int iNode = 0; !isDTSScene && (iNode < sceneNodes.size()); iNode++) {
      ColladaAppNode *node = sceneNodes[iNode];
      if (dStrStartsWith(node->getName(), "base")) {
         for (int iChild = 0; iChild < node->getNumChildNodes(); iChild++) {
            if (dStrStartsWith(node->getChildNode(iChild)->getName(), "start")) {
               isDTSScene = true;
               break;
            }
         }
      }
   }

   if (isDTSScene) {
      // Process the scene as normal
      ColladaAppMesh::fixDetailSize(false);

      for (U32 iNode = 0; iNode < sceneNodes.size(); iNode++)
      {
         ColladaAppNode *node = sceneNodes[iNode];

         // Only allow a single subshape (the baseXXX one)
         if (node->isParentRoot() && node->getNumChildNodes() &&
             !dStrStartsWith(node->getName(), "base"))
             continue;

         if ( !processNode( sceneNodes[iNode] ) )
            delete sceneNodes[iNode];
      }
   }
   else
   {
      // Handle a non-DTS hierarchy
      daeErrorHandler::get()->handleWarning( "Non-DTS scene detected: creating "
         "dummy node to hold subshape" );
      domVisual_scene* visualScene = root->getLibrary_visual_scenes_array()[0]->getVisual_scene_array()[0];

      // Load the rest of the geometry with LOD size forced to 2
      ColladaAppMesh::fixDetailSize( true, 2 );

      // Create a dummy node in the Collada model to hold the subshape
      domNode* dummyNode = daeSafeCast<domNode>( visualScene->createAndPlace( "node" ) );
      dummyNode->setName( "dummy" );

      // Create a dummy subshape and add the whole scene to it. The only
      // exception is the bounds node, which can be processed as normal
      subshapes.push_back( new TSShapeLoader::Subshape );
      subshapes.last()->node = new ColladaAppNode( dummyNode );
      for ( U32 iNode = 0; iNode < sceneNodes.size(); iNode++ )
      {
         ColladaAppNode *node = sceneNodes[iNode];

         // Skip Collision and LOS meshes
         if ( dStrStartsWith( node->getName(), "Collision" ) )
            continue;

         if ( dStrStartsWith( node->getName(), "LOS" ) )
            continue;

         if (node->isBounds())
         {
            if (!processNode(node))
               delete node;
         }
         else
            subshapes.last()->branches.push_back(node);
      }

      // If this shape has collision meshes then create details for them
      for ( U32 iNode = 0; iNode < sceneNodes.size(); iNode++ )
      {
         ColladaAppNode *node = sceneNodes[iNode];

         if ( !dStrStartsWith( node->getName(), "collision" ) &&
              !dStrStartsWith( node->getName(), "LOS" ) )
            continue;

         domNode* collisionNode = daeSafeCast<domNode>( visualScene->createAndPlace( "node" ) );

         collisionNode->setName( node->getName() );

         // Create a dummy subshape and add the whole scene to it. The only
         // exception is the bounds node, which can be processed as normal
         subshapes.push_back(new TSShapeLoader::Subshape);
         subshapes.last()->node = new ColladaAppNode(collisionNode);

         subshapes.last()->branches.push_back(node);
      }
   }
}

//-----------------------------------------------------------------------------
/// Add collada materials to materials.cs

#define writeLine(stream, str) { stream.write((int)dStrlen(str), str); stream.write(2, "\r\n"); }

void updateMaterialsScript(const Torque::Path &path)
{
    Con::printf("Not implemented yet");
   //// First see what materials we need to add... if one already
   //// exists then we can ignore it.
   //Vector<ColladaAppMaterial*> materials;
   //for ( U32 iMat = 0; iMat < AppMesh::appMaterials.size(); iMat++ )
   //{
   //   ColladaAppMaterial *mat = dynamic_cast<ColladaAppMaterial*>( AppMesh::appMaterials[iMat] );
   //   if ( MATMGR->getMapEntry( mat->getName() ).isEmpty() )
   //      materials.push_back( mat );      
   //}

   //if ( materials.empty() )
   //   return;

   //Torque::Path scriptPath(path);
   //scriptPath.setFileName("materials");
   //scriptPath.setExtension("cs");

   //// Read the current script (if any) into memory
   //FileObject f;
   //f.readMemory(scriptPath.getFullPath().c_str());

   //FileStream stream;
   //if (stream.open(scriptPath.getFullPath().c_str(), FileStream::AccessMode::Write)) {

   //   String shapeName = TSShapeLoader::getShapePath().getFullFileName();
   //   const char *beginMsg = avar("//--- %s MATERIALS BEGIN ---", shapeName.c_str());

   //   // Write existing file contents up to start of auto-generated materials
   //   while(!f.isEOF()) {
   //      const char *buffer = (const char *)f.readLine();
   //      if (dStricmp(buffer, beginMsg) == 0)
   //         break;
   //      writeLine(stream, buffer);
   //   }

   //   // Write new auto-generated materials
   //   writeLine(stream, beginMsg);
   //   for (int iMat = 0; iMat < AppMesh::appMaterials.size(); iMat++)
   //      dynamic_cast<ColladaAppMaterial*>(AppMesh::appMaterials[iMat])->write(stream);

   //   const char *endMsg = avar("//--- %s MATERIALS END ---", shapeName.c_str());
   //   writeLine(stream, endMsg);
   //   writeLine(stream, "");

   //   // Write existing file contents after end of auto-generated materials
   //   while (!f.isEOF()) {
   //      const char *buffer = (const char *) f.readLine();
   //      if (dStricmp(buffer, endMsg) == 0)
   //         break;
   //   }

   //   // Want at least one blank line after the autogen block, but need to
   //   // be careful not to write it twice, or another blank will be added
   //   // each time the file is written!
   //   if (!f.isEOF()) {
   //      const char *buffer = (const char *) f.readLine();
   //      if (!dStrEqual(buffer, ""))
   //         writeLine(stream, buffer);
   //   }
   //   while (!f.isEOF()) {
   //      const char *buffer = (const char *) f.readLine();
   //      writeLine(stream, buffer);
   //   }
   //   f.close();
   //   stream.close();

   //   // Execute the new script to apply the material settings
   //   if (f.readMemory(scriptPath.getFullPath().c_str()))
   //   {
   //      String instantGroup = Con::getVariable("InstantGroup");
   //      Con::setIntVariable("InstantGroup", RootGroupId);
   //      Con::evaluate((const char*)f, false, scriptPath.getFullPath());
   //      Con::setVariable("InstantGroup", instantGroup.c_str());
   //   }
   //}
}

//-----------------------------------------------------------------------------
// Custom warning/error message handler
class myErrorHandler : public daeErrorHandler
{
	void handleError( daeString msg )
   {
      Con::errorf("Error: %s", msg);
   }

	void handleWarning( daeString msg )
   {
      Con::errorf("Warning: %s", msg);
   }
};

//-----------------------------------------------------------------------------
/// This function is invoked by the resource manager based on file extension.
TSShape* loadColladaShape(const Torque::Path &path)
{
   myErrorHandler errorHandler;
   daeErrorHandler::setErrorHandler(&errorHandler);

   // Check if a up-to-date cached DTS version of this file exists, and
   // if so, use that instead.

   // If we are inside a zip file, use the path to the zip itself, rather than
   // the internal path, so we don't have to write into the archive.
   Torque::Path objPath(path);
   ResourceObject *ro = ResourceManager->find(path.getFullPath().c_str());
   if (!ro)
   {
      // File doesn't exist, bail.
      return NULL;
   }
   if (ro->zipPath != NULL)
      objPath.setPath(ro->zipPath);

   // Generate the cached filename
   Torque::Path cachedPath(path);
   cachedPath.setExtension("cached.dts");

   // Check the modification times of the DAE and cached DTS files
   FileTime cachedModifyTime;
   if (Platform::getFileTimes(cachedPath.getFullPath().c_str(), NULL, &cachedModifyTime))
   {
      FileTime daeModifyTime;
      Platform::getFileTimes(path.getFullPath().c_str(), NULL, &daeModifyTime);
      if (Platform::compareFileTimes(cachedModifyTime, daeModifyTime) >= 0)
      {
         // Cached DTS is newer => attempt to load shape from there instead
         FileStream cachedStream;
         cachedStream.open(cachedPath.getFullPath().c_str(), FileStream::Read);
         if (cachedStream.getStatus() == Stream::Ok)
         {
            TSShape *shape = new TSShape;
            bool readSuccess = shape->read(&cachedStream);
            cachedStream.close();

            if (readSuccess)
            {
            #ifdef TORQUE_DEBUG
               Con::printf("Loaded cached Collada shape from %s", cachedPath.getFullPath().c_str());
            #endif
               return shape;
            }
            else
               delete shape;
         }

         daeErrorHandler::get()->handleWarning(avar("Failed to load cached COLLADA "
            "shape from %s", cachedPath.getFullPath().c_str()));
      }
   }

   // Load the Collada file into memory
   TSShapeLoader::updateProgress(TSShapeLoader::Load_ReadFile, path.getFullFileName().c_str());
   FileObject fo;
   if (!fo.readMemory(path.getFullPath().c_str()))
   {
      daeErrorHandler::get()->handleError(avar("Could not read %s into memory", path.getFullPath().c_str()));
      TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Import failed");
      return NULL;
   }

   // Read the XML document into the Collada DOM
   TSShapeLoader::updateProgress(TSShapeLoader::Load_ParseFile, "Parsing XML...");
   DAE dae;
   domCOLLADA* root = dae.openFromMemory(path.getFullPath().c_str(), (const char*)fo.buffer());
   if (!root || !root->getLibrary_visual_scenes_array().getCount()) {
      daeErrorHandler::get()->handleError(avar("Could not parse %s", path.getFullPath().c_str()));
      TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Import failed");
      return NULL;
   }

   // Load the collada shape
   ColladaUtils::applyConditioners(root);
   ColladaShapeLoader loader(root);

   // Allow TSShapeConstructor object to override properties
   TSShapeConstructor* tscon = TSShapeConstructor::findShapeConstructor(path.getFullPath());
   if (tscon)
   {
      switch (tscon->mUpAxis)
      {
      case TSShapeConstructor::X_AXIS:    loader.upAxis = UPAXISTYPE_X_UP;    break;
      case TSShapeConstructor::Y_AXIS:    loader.upAxis = UPAXISTYPE_Y_UP;    break;
      case TSShapeConstructor::Z_AXIS:    loader.upAxis = UPAXISTYPE_Z_UP;    break;
      default:                            /* No override */                   break;
      }

      if (tscon->mUnit > 0)
         loader.unitScale = Point3F(tscon->mUnit, tscon->mUnit, tscon->mUnit);

      ColladaAppMaterial::setPrefix(tscon->mMatNamePrefix);
   }
   else
   {
      ColladaAppMaterial::setPrefix("");
   }

   TSShape* tss = loader.generateShape(path);

#if 0
   // Dump the imported Collada model structure to a text file
   {
      FileStream dumpStream;
      if (dumpStream.open("dump.txt", Torque::FS::File::Write)) {
         TSShapeInstance* tsi = new TSShapeInstance(tss, false);
         tsi->dump(dumpStream);
         delete tsi;
      }
   }
#endif

   // Cache the Collada model to a DTS file for faster loading next time.
   if (Con::getBoolVariable("$pref::collada::cacheDts", true))
   {
      FileStream dtsStream;
      if (dtsStream.open(cachedPath.getFullPath().c_str(), FileStream::Write))
      {
         Con::printf("Writing cached COLLADA shape to %s", cachedPath.getFullPath().c_str());
         tss->write(&dtsStream);
      }
   }

   // Add collada materials to materials.cs
   if (Con::getBoolVariable("$pref::collada::updateMaterials", true))
      updateMaterialsScript(path);

   // Close progress dialog
   TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Import complete");

   return tss;
}
