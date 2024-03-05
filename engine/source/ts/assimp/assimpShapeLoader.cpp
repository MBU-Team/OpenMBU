//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

/*
   Resource stream -> Buffer
   Buffer -> Collada DOM
   Collada DOM -> TSShapeLoader
   TSShapeLoader installed into TSShape
*/

//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "ts/assimp/assimpShapeLoader.h"
#include "ts/assimp/assimpAppNode.h"
#include "ts/assimp/assimpAppMesh.h"
#include "ts/assimp/assimpAppMaterial.h"
#include "ts/assimp/assimpAppSequence.h"

#include "core/tVector.h"
#include "core/findMatch.h"
#include "core/fileStream.h"
#include "core/fileObject.h"
#include "core/units.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsShapeConstruct.h"
#include "gui/controls/guiTreeViewCtrl.h"

// assimp include files. 
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <assimp/config.h>
#include <exception>
#include <algorithm>

#include <assimp/Importer.hpp>

//MODULE_BEGIN( AssimpShapeLoader )
//   MODULE_INIT_AFTER( ShapeLoader )
//   MODULE_INIT
//   {
//      TSShapeLoader::addFormat("DirectX X", "x");
//      TSShapeLoader::addFormat("Autodesk FBX", "fbx");
//      TSShapeLoader::addFormat("Blender 3D", "blend" );
//      TSShapeLoader::addFormat("3ds Max 3DS", "3ds");
//      TSShapeLoader::addFormat("3ds Max ASE", "ase");
//      TSShapeLoader::addFormat("Wavefront Object", "obj");
//      TSShapeLoader::addFormat("Industry Foundation Classes (IFC/Step)", "ifc");
//      TSShapeLoader::addFormat("Stanford Polygon Library", "ply");
//      TSShapeLoader::addFormat("AutoCAD DXF", "dxf");
//      TSShapeLoader::addFormat("LightWave", "lwo");
//      TSShapeLoader::addFormat("LightWave Scene", "lws");
//      TSShapeLoader::addFormat("Modo", "lxo");
//      TSShapeLoader::addFormat("Stereolithography", "stl");
//      TSShapeLoader::addFormat("AC3D", "ac");
//      TSShapeLoader::addFormat("Milkshape 3D", "ms3d");
//      TSShapeLoader::addFormat("TrueSpace COB", "cob");
//      TSShapeLoader::addFormat("TrueSpace SCN", "scn");
//      TSShapeLoader::addFormat("Ogre XML", "xml");
//      TSShapeLoader::addFormat("Irrlicht Mesh", "irrmesh");
//      TSShapeLoader::addFormat("Irrlicht Scene", "irr");
//      TSShapeLoader::addFormat("Quake I", "mdl" );
//      TSShapeLoader::addFormat("Quake II", "md2" );
//      TSShapeLoader::addFormat("Quake III Mesh", "md3");
//      TSShapeLoader::addFormat("Quake III Map/BSP", "pk3");
//      TSShapeLoader::addFormat("Return to Castle Wolfenstein", "mdc");
//      TSShapeLoader::addFormat("Doom 3", "md5" );
//      TSShapeLoader::addFormat("Valve SMD", "smd");
//      TSShapeLoader::addFormat("Valve VTA", "vta");
//      TSShapeLoader::addFormat("Starcraft II M3", "m3");
//      TSShapeLoader::addFormat("Unreal", "3d");
//      TSShapeLoader::addFormat("BlitzBasic 3D", "b3d" );
//      TSShapeLoader::addFormat("Quick3D Q3D", "q3d");
//      TSShapeLoader::addFormat("Quick3D Q3S", "q3s");
//      TSShapeLoader::addFormat("Neutral File Format", "nff");
//      TSShapeLoader::addFormat("Object File Format", "off");
//      TSShapeLoader::addFormat("PovRAY Raw", "raw");
//      TSShapeLoader::addFormat("Terragen Terrain", "ter");
//      TSShapeLoader::addFormat("3D GameStudio (3DGS)", "mdl");
//      TSShapeLoader::addFormat("3D GameStudio (3DGS) Terrain", "hmp");
//      TSShapeLoader::addFormat("Izware Nendo", "ndo");
//      TSShapeLoader::addFormat("gltf", "gltf");
//      TSShapeLoader::addFormat("gltf binary", "glb");
//   }
//MODULE_END;

//-----------------------------------------------------------------------------

AssimpShapeLoader::AssimpShapeLoader() 
{
   mScene = NULL;
}

AssimpShapeLoader::~AssimpShapeLoader()
{

}

void AssimpShapeLoader::releaseImport()
{
   aiReleaseImport(mScene);
}

void AssimpShapeLoader::enumerateScene()
{
   TSShapeLoader::updateProgress(TSShapeLoader::Load_ReadFile, "Reading File");
   Con::printf("[ASSIMP] Attempting to load file: %s", shapePath.getFullPath().c_str());

   // Post-Processing
   unsigned int ppsteps = 
      (ColladaUtils::getOptions().convertLeftHanded ? aiProcess_MakeLeftHanded : 0) |
      (ColladaUtils::getOptions().reverseWindingOrder ? aiProcess_FlipWindingOrder : 0) |
      (ColladaUtils::getOptions().calcTangentSpace ? aiProcess_CalcTangentSpace : 0) |
      (ColladaUtils::getOptions().joinIdenticalVerts ? aiProcess_JoinIdenticalVertices : 0) |
      (ColladaUtils::getOptions().removeRedundantMats ? aiProcess_RemoveRedundantMaterials : 0) |
      (ColladaUtils::getOptions().genUVCoords ? aiProcess_GenUVCoords : 0) |
      (ColladaUtils::getOptions().transformUVCoords ? aiProcess_TransformUVCoords : 0) |
      (ColladaUtils::getOptions().flipUVCoords ? aiProcess_FlipUVs : 0) |
      (ColladaUtils::getOptions().findInstances ? aiProcess_FindInstances : 0) |
      (ColladaUtils::getOptions().limitBoneWeights ? aiProcess_LimitBoneWeights : 0) |
      (ColladaUtils::getOptions().smoothNormals ? (aiProcess_ForceGenNormals | aiProcess_GenSmoothNormals) : 0);

   if (Con::getBoolVariable("$Assimp::OptimizeMeshes", false))
      ppsteps |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

   if (Con::getBoolVariable("$Assimp::SplitLargeMeshes", false))
      ppsteps |= aiProcess_SplitLargeMeshes;

   // Mandatory options
   //ppsteps |= aiProcess_ValidateDataStructure | aiProcess_Triangulate | aiProcess_ImproveCacheLocality;
   ppsteps |= aiProcess_Triangulate;
   //aiProcess_SortByPType              | // make 'clean' meshes which consist of a single typ of primitives

   aiPropertyStore* props = aiCreatePropertyStore();

   struct aiLogStream shapeLog = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
   shapeLog.callback = assimpLogCallback;
   shapeLog.user = 0;
   aiAttachLogStream(&shapeLog);
#ifdef TORQUE_DEBUG
   aiEnableVerboseLogging(true);
#endif

   mScene = (aiScene*)aiImportFileExWithProperties(shapePath.getFullPath().c_str(), ppsteps, NULL, props);

   aiReleasePropertyStore(props);
   
   if ( mScene )
   {
      Con::printf("[ASSIMP] Mesh Count: %d", mScene->mNumMeshes);
      Con::printf("[ASSIMP] Material Count: %d", mScene->mNumMaterials);

      // Setup default units for shape format
      String importFormat;

      char fileExt[10];
      dStrcpy(fileExt, shapePath.getExtension().c_str());
      ;
      const aiImporterDesc* importerDescription = aiGetImporterDesc(dStrlwr(fileExt));
      if (importerDescription && StringTable->insert(importerDescription->mName) == StringTable->insert("Autodesk FBX Importer"))
      {
         ColladaUtils::getOptions().formatScaleFactor = 0.01f;
      }

      // Set import options (if they are not set to override)
      if (ColladaUtils::getOptions().unit <= 0.0f)
      {
         F64 unit;
         if (!getMetaDouble("UnitScaleFactor", unit))
         {
            F32 floatVal;
            S32 intVal;
            if (getMetaFloat("UnitScaleFactor", floatVal))
               unit = (F64)floatVal;
            else if (getMetaInt("UnitScaleFactor", intVal))
               unit = (F64)intVal;
            else
               unit = 1.0;
         }
         ColladaUtils::getOptions().unit = (F32)unit;
      }

      if (ColladaUtils::getOptions().upAxis == UPAXISTYPE_COUNT)
      {
         S32 upAxis;
         if (!getMetaInt("UpAxis", upAxis))
            upAxis = UPAXISTYPE_Z_UP;
         ColladaUtils::getOptions().upAxis = (domUpAxisType) upAxis;
      }

      // Extract embedded textures
      for (U32 i = 0; i < mScene->mNumTextures; ++i)
         extractTexture(i, mScene->mTextures[i]);

      // Load all the materials.
      AssimpAppMaterial::sDefaultMatNumber = 0;
      for ( U32 i = 0; i < mScene->mNumMaterials; i++ )
         AppMesh::appMaterials.push_back(new AssimpAppMaterial(mScene->mMaterials[i]));

      // Setup LOD checks
      detectDetails();

      // Define the root node, and process down the chain.
      AssimpAppNode* node = new AssimpAppNode(mScene, mScene->mRootNode, 0);
      
      if (!processNode(node))
         delete node;

      // Check for animations and process those.
      processAnimations();
   } 
   else 
   {
      TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Import failed");
      Con::printf("[ASSIMP] Import Error: %s", aiGetErrorString());
   }

   aiDetachLogStream(&shapeLog);
}

void AssimpShapeLoader::processAnimations()
{
   for(U32 n = 0; n < mScene->mNumAnimations; ++n)
   {
      Con::printf("[ASSIMP] Animation Found: %s", mScene->mAnimations[n]->mName.C_Str());

      AssimpAppSequence* newAssimpSeq = new AssimpAppSequence(mScene->mAnimations[n]);
      appSequences.push_back(newAssimpSeq);
   }
}

void AssimpShapeLoader::computeBounds(Box3F& bounds)
{
   TSShapeLoader::computeBounds(bounds);

   // Check if the model origin needs adjusting
   bool adjustCenter = ColladaUtils::getOptions().adjustCenter;
   bool adjustFloor = ColladaUtils::getOptions().adjustFloor;
   if (bounds.isValidBox() && (adjustCenter || adjustFloor))
   {
      // Compute shape offset
      Point3F shapeOffset = Point3F(0, 0, 0);
      if (adjustCenter)
      {
         bounds.getCenter(&shapeOffset);
         shapeOffset = -shapeOffset;
      }
      if (adjustFloor)
         shapeOffset.z = -bounds.min.z;

      // Adjust bounds
      bounds.min += shapeOffset;
      bounds.max += shapeOffset;

      // Now adjust all positions for root level nodes (nodes with no parent)
      for (S32 iNode = 0; iNode < shape->nodes.size(); iNode++)
      {
         if (!appNodes[iNode]->isParentRoot())
            continue;

         // Adjust default translation
         shape->defaultTranslations[iNode] += shapeOffset;

         // Adjust animated translations
         for (S32 iSeq = 0; iSeq < shape->sequences.size(); iSeq++)
         {
            const TSShape::Sequence& seq = shape->sequences[iSeq];
            if (seq.translationMatters.test(iNode))
            {
               for (S32 iFrame = 0; iFrame < seq.numKeyframes; iFrame++)
               {
                  S32 index = seq.baseTranslation + seq.translationMatters.count(iNode)*seq.numKeyframes + iFrame;
                  shape->nodeTranslations[index] += shapeOffset;
               }
            }
         }
      }
   }
}

/// Check if an up-to-date cached DTS is available for this DAE file
bool AssimpShapeLoader::canLoadCachedDTS(const Torque::Path& path)
{
   // Generate the cached filename
   Torque::Path cachedPath(path);
   cachedPath.setExtension("cached.dts");

   // Check if a cached DTS newer than this file is available
   FileTime cachedModifyTime;
   if (Platform::getFileTimes(cachedPath.getFullPath().c_str(), NULL, &cachedModifyTime))
   {
      bool forceLoad = Con::getBoolVariable("$assimp::forceLoad", false);

      FileTime daeModifyTime;
      if (!Platform::getFileTimes(path.getFullPath().c_str(), NULL, &daeModifyTime) ||
         (!forceLoad && (Platform::compareFileTimes(cachedModifyTime, daeModifyTime) >= 0) ))
      {
         // Original file not found, or cached DTS is newer
         return true;
      }
   }

   return false;
}

void AssimpShapeLoader::assimpLogCallback(const char* message, char* user)
{
   Con::printf("[Assimp log message] %s", getUnit(message, 0, "\n"));
}

bool AssimpShapeLoader::ignoreNode(const String& name)
{
   // Do not add AssimpFbx dummy nodes to the TSShape. See: Assimp::FBX::ImportSettings::preservePivots
   // https://github.com/assimp/assimp/blob/master/code/FBXImportSettings.h#L116-L135
   if (name.find("_$AssimpFbx$_") != std::string::npos)
      return true;

   if (FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().alwaysImport.c_str(), name.c_str(), false))
      return false;

   return FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().neverImport.c_str(), name.c_str(), false);
}

bool AssimpShapeLoader::ignoreMesh(const String& name)
{
   if (FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().alwaysImportMesh.c_str(), name.c_str(), false))
      return false;
   else
      return FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().neverImportMesh.c_str(), name.c_str(), false);
}

void AssimpShapeLoader::detectDetails()
{
   // Set LOD option
   bool singleDetail = true;
   switch (ColladaUtils::getOptions().lodType)
   {
   case ColladaUtils::ImportOptions::DetectDTS:
      // Check for a baseXX->startXX hierarchy at the top-level, if we find
      // one, use trailing numbers for LOD, otherwise use a single size
      for (S32 iNode = 0; singleDetail && (iNode < mScene->mRootNode->mNumChildren); iNode++) {
         aiNode* node = mScene->mRootNode->mChildren[iNode];
         if (node && dStrStartsWith(node->mName.C_Str(), "base")) {
            for (S32 iChild = 0; iChild < node->mNumChildren; iChild++) {
               aiNode* child = node->mChildren[iChild];
               if (child && dStrStartsWith(child->mName.C_Str(), "start")) {
                  singleDetail = false;
                  break;
               }
            }
         }
      }
      break;

   case ColladaUtils::ImportOptions::SingleSize:
      singleDetail = true;
      break;

   case ColladaUtils::ImportOptions::TrailingNumber:
      singleDetail = false;
      break;

   default:
      break;
   }

   AssimpAppMesh::fixDetailSize(singleDetail, ColladaUtils::getOptions().singleDetailSize);
}

void AssimpShapeLoader::extractTexture(U32 index, aiTexture* pTex)
{  // Cache an embedded texture to disk
   updateProgress(Load_EnumerateScene, "Extracting Textures...", mScene->mNumTextures, index);
   Con::printf("[Assimp] Extracting Texture %s, W: %d, H: %d, %d of %d, format hint: (%s)", pTex->mFilename.C_Str(),
      pTex->mWidth, pTex->mHeight, index, mScene->mNumTextures, pTex->achFormatHint);

   // Create the texture filename
   String cleanFile = cleanString2(TSShapeLoader::getShapePath().getFileName());
   char buf[256];
   dSprintf(buf, 256, "%s_cachedTex%d", cleanFile.c_str(), index);
   String texName = buf;
   Torque::Path texPath = shapePath;
   texPath.setFileName(texName);

   if (pTex->mHeight == 0)
   {  // Compressed format, write the data directly to disc
      texPath.setExtension(pTex->achFormatHint);
      Stream* outputStream;
      if (ResourceManager->openFileForWrite(outputStream, texPath.getFullPath().c_str(), FileStream::Write))
      {
         outputStream->setPosition(0);
         outputStream->write(pTex->mWidth, pTex->pcData);
         delete outputStream;
      }
   }
   else
   {  // Embedded pixel data, fill a bitmap and save it.
       Con::printf("Embedded pixel data for assimp not supported yet");
      //GFXTexHandle shapeTex;
      //shapeTex.set(pTex->mWidth, pTex->mHeight, GFXFormatR8G8B8A8_SRGB, &GFXDynamicTextureSRGBProfile,
      //   String::ToString("AssimpShapeLoader (%s:%i)", __FILE__, __LINE__), 1, 0);
      //GFXLockedRect *rect = shapeTex.lock();

      //for (U32 y = 0; y < pTex->mHeight; ++y)
      //{
      //   for (U32 x = 0; x < pTex->mWidth; ++x)
      //   {
      //      U32 targetIndex = (y * rect->pitch) + (x * 4);
      //      U32 sourceIndex = ((y * pTex->mWidth) + x) * 4;
      //      rect->bits[targetIndex] = pTex->pcData[sourceIndex].r;
      //      rect->bits[targetIndex + 1] = pTex->pcData[sourceIndex].g;
      //      rect->bits[targetIndex + 2] = pTex->pcData[sourceIndex].b;
      //      rect->bits[targetIndex + 3] = pTex->pcData[sourceIndex].a;
      //   }
      //}
      //shapeTex.unlock();

      //texPath.setExtension("png");
      //shapeTex->dumpToDisk("PNG", texPath.getFullPath());
   }
}

bool AssimpShapeLoader::getMetabool(const char* key, bool& boolVal)
{
   if (!mScene || !mScene->mMetaData)
      return false;

   StringTableEntry keyStr = StringTable->insert(key);
   for (U32 n = 0; n < mScene->mMetaData->mNumProperties; ++n)
   {
       if (keyStr == StringTable->insert(mScene->mMetaData->mKeys[n].C_Str()))
      {
         if (mScene->mMetaData->mValues[n].mType == AI_BOOL)
         {
            boolVal = (bool)mScene->mMetaData->mValues[n].mData;
            return true;
         }
      }
   }
   return false;
}

bool AssimpShapeLoader::getMetaInt(const char* key, S32& intVal)
{
   if (!mScene || !mScene->mMetaData)
      return false;

   StringTableEntry keyStr = StringTable->insert(key);
   for (U32 n = 0; n < mScene->mMetaData->mNumProperties; ++n)
   {
       if (keyStr == StringTable->insert(mScene->mMetaData->mKeys[n].C_Str()))
      {
         if (mScene->mMetaData->mValues[n].mType == AI_INT32)
         {
            intVal = *((S32*)(mScene->mMetaData->mValues[n].mData));
            return true;
         }
      }
   }
   return false;
}

bool AssimpShapeLoader::getMetaFloat(const char* key, F32& floatVal)
{
   if (!mScene || !mScene->mMetaData)
      return false;

   StringTableEntry keyStr = StringTable->insert(key);
   for (U32 n = 0; n < mScene->mMetaData->mNumProperties; ++n)
   {
       if (keyStr == StringTable->insert(mScene->mMetaData->mKeys[n].C_Str()))
      {
         if (mScene->mMetaData->mValues[n].mType == AI_FLOAT)
         {
            floatVal = *((F32*)mScene->mMetaData->mValues[n].mData);
            return true;
         }
      }
   }
   return false;
}

bool AssimpShapeLoader::getMetaDouble(const char* key, F64& doubleVal)
{
   if (!mScene || !mScene->mMetaData)
      return false;

   StringTableEntry keyStr = StringTable->insert(key);
   for (U32 n = 0; n < mScene->mMetaData->mNumProperties; ++n)
   {
       if (keyStr == StringTable->insert(mScene->mMetaData->mKeys[n].C_Str()))
      {
         if (mScene->mMetaData->mValues[n].mType == AI_DOUBLE)
         {
            doubleVal = *((F64*)mScene->mMetaData->mValues[n].mData);
            return true;
         }
      }
   }
   return false;
}

bool AssimpShapeLoader::getMetaString(const char* key, String& stringVal)
{
   if (!mScene || !mScene->mMetaData)
      return false;

   StringTableEntry keyStr = StringTable->insert(key);
   for (U32 n = 0; n < mScene->mMetaData->mNumProperties; ++n)
   {
      if (keyStr == StringTable->insert(mScene->mMetaData->mKeys[n].C_Str()))
      {
         if (mScene->mMetaData->mValues[n].mType == AI_AISTRING)
         {
            aiString valString;
            mScene->mMetaData->Get<aiString>(mScene->mMetaData->mKeys[n], valString);
            stringVal = valString.C_Str();
            return true;
         }
      }
   }
   return false;
}
//-----------------------------------------------------------------------------
/// This function is invoked by the resource manager based on file extension.
TSShape* assimpLoadShape(const Torque::Path &path)
{
   // TODO: add .cached.dts generation.
   // Generate the cached filename
   Torque::Path cachedPath(path);
   cachedPath.setExtension("cached.dts");

   // Check if an up-to-date cached DTS version of this file exists, and
   // if so, use that instead.
   if (AssimpShapeLoader::canLoadCachedDTS(path))
   {
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
            Con::printf("Loaded cached shape from %s", cachedPath.getFullPath().c_str());
         #endif
            return shape;
         }
         else
            delete shape;
      }

      Con::warnf("Failed to load cached shape from %s", cachedPath.getFullPath().c_str());
   }

   if (!Platform::isFile(path.getFullPath().c_str()))
   {
      // File does not exist, bail.
      return NULL;
   }

   // Allow TSShapeConstructor object to override properties
   // ColladaUtils::getOptions().reset();
   TSShapeConstructor* tscon = TSShapeConstructor::findShapeConstructor(path.getFullPath().c_str());
   //if (tscon)
   //{
   //   ColladaUtils::getOptions() = tscon->mOptions;
   //}

   AssimpShapeLoader loader;
   TSShape* tss = loader.generateShape(path);
   if (tss)
   {
      TSShapeLoader::updateProgress(TSShapeLoader::Load_Complete, "Import complete");
      Con::printf("[ASSIMP] Shape created successfully.");

      // Cache the model to a DTS file for faster loading next time.
      FileStream dtsStream;
      if (dtsStream.open(cachedPath.getFullPath().c_str(), FileStream::Write))
      {
         Con::printf("Writing cached shape to %s", cachedPath.getFullPath().c_str());
         tss->write(&dtsStream);
      }

      // loader.updateMaterialsScript(path);
   }
   loader.releaseImport();
   return tss;
}

ConsoleFunction(convertAssimp, const char*, 2, 2, "convertAssimp(path)")
{
    Torque::Path p = argv[1];
    TSShape* shape = assimpLoadShape(p);
    Torque::Path cachedPath(p);
    cachedPath.setExtension("cached.dts");

    if (shape)
    {
        delete shape;
        char* buf = Con::getReturnBuffer(256);
        sprintf(buf, "%s", cachedPath.getFullPath().c_str());
        return buf;
    }
    return "";
}

ConsoleFunction(setImportSettings, void, 14, 14, "setImportSettings(...)")
{
    ColladaUtils::ImportOptions& opts = ColladaUtils::getOptions();
    opts.convertLeftHanded = atoi(argv[1]);
    opts.genUVCoords = atoi(argv[2]);
    opts.transformUVCoords = atoi(argv[3]);
    opts.flipUVCoords = atoi(argv[4]);
    opts.joinIdenticalVerts = atoi(argv[5]);
    opts.reverseWindingOrder = atoi(argv[6]);
    opts.invertNormals = atoi(argv[7]);
    opts.smoothNormals = atoi(argv[8]);
    opts.adjustCenter = atoi(argv[9]);
    opts.adjustFloor = atoi(argv[10]);
    opts.formatScaleFactor = atof(argv[11]);
    if (isnan(opts.formatScaleFactor) || opts.formatScaleFactor == 0)
        opts.formatScaleFactor = 1.0;
    opts.unit = atof(argv[12]);
    if (isnan(opts.unit) || opts.unit == 0)
        opts.unit = -1.0;
    switch (atoi(argv[13]))
    {
    case 0:
        opts.upAxis = domUpAxisType::UPAXISTYPE_X_UP;
        break;
    case 1:
        opts.upAxis = domUpAxisType::UPAXISTYPE_Y_UP;
        break;
    default:
    case 2:
        opts.upAxis = domUpAxisType::UPAXISTYPE_Z_UP;
        break;
    }
}