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
#define TORQUE_PBR_MATERIALS

#include "platform/platform.h"
#include "ts/loader/appSequence.h"
#include "ts/assimp/assimpAppMaterial.h"
#include "ts/assimp/assimpAppMesh.h"
#include "ts/collada/colladaUtils.h"
#include <algorithm>

// assimp include files. 
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

U32 AssimpAppMaterial::sDefaultMatNumber = 0;

String cleanString2(const String& str)
{
    String cleanStr(str);
    const String badChars(" -,.+=*/[]%$~;:");
    for (char i = 0; i < badChars.length(); i++)
        std::replace(cleanStr.begin(), cleanStr.end(), badChars[i], '_');

    // Prefix with an underscore if string starts with a number
    if ((cleanStr[0] >= '0') && (cleanStr[0] <= '9'))
        cleanStr = "_" + cleanStr;
    return cleanStr;
}

AssimpAppMaterial::AssimpAppMaterial(const char* matName)
{
   name = ColladaUtils::getOptions().matNamePrefix;
   name += matName;
   mAIMat = NULL;
   // Set some defaults
   flags |= TSMaterialList::S_Wrap;
   flags |= TSMaterialList::T_Wrap;
}

AssimpAppMaterial::AssimpAppMaterial(aiMaterial* mtl) :
   mAIMat(mtl)
{
   aiString matName;
   mtl->Get(AI_MATKEY_NAME, matName);
   name = matName.C_Str();
   if (name.empty())
   {
      name = cleanString2(TSShapeLoader::getShapePath().getFileName());
      name += "_defMat";
      name += sDefaultMatNumber;
      sDefaultMatNumber++;
   }
   name = ColladaUtils::getOptions().matNamePrefix + name;
   Con::printf("[ASSIMP] Loading Material: %s", name.c_str());
}

void AssimpAppMaterial::initMaterial(const Torque::Path& path, Material* mat)
{
   String cleanFile = cleanString2(TSShapeLoader::getShapePath().getFileName());
   String cleanName = cleanString2(getName());

   // Determine the blend mode and transparency for this material
   Material::BlendOp blendOp = Material::None;
   bool translucent = false;
   float opacity = 1.0f;
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_OPACITY, opacity))
   {
      if (opacity != 1.0f)
      {
         translucent = true;
         int blendInt;
         blendOp = Material::LerpAlpha;
         if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_BLEND_FUNC, blendInt))
         {
            if (blendInt == aiBlendMode_Additive)
               blendOp = Material::Add;
         }
      }
   }
   else
   {  // No opacity key, see if it's defined as a gltf property
      aiString opacityMode;
      if (AI_SUCCESS == mAIMat->Get("$mat.gltf.alphaMode", 0, 0, opacityMode))
      {
         if (strcmp("MASK", opacityMode.C_Str()) == 0)
         {
            translucent = true;
            blendOp = Material::None;

            float cutoff;
            if (AI_SUCCESS == mAIMat->Get("$mat.gltf.alphaCutoff", 0, 0, cutoff))
            {
               mat->alphaRef = (U32)(cutoff * 255);  // alpha ref 0-255
               mat->alphaTest = true;
            }
         }
         else if (strcmp("BLEND", opacityMode.C_Str()) == 0)
         {
            translucent = true;
            blendOp = Material::LerpAlpha;
            mat->alphaTest = false;
         }
         else
         {  // OPAQUE
            translucent = false;
            blendOp = Material::LerpAlpha; // Make default so it doesn't get written to materials.tscript
         }
      }
   }
   mat->translucent = translucent;
   mat->translucentBlendOp = blendOp;

   // Assign color values.
   ColorF diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
   aiColor3D read_color(1.f, 1.f, 1.f);
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_COLOR_DIFFUSE, read_color))
      diffuseColor.set(read_color.r, read_color.g, read_color.b, opacity);
   mat->diffuse[0] = diffuseColor;

   aiString texName;
   String torquePath;
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texName))
   {
      torquePath = texName.C_Str();
      if (!torquePath.empty())
         mat->baseTexFilename[0] = StringTable->insert(cleanTextureName(torquePath, cleanFile, path, false).c_str(), false);
   }

   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texName))
   {
      torquePath = texName.C_Str();
      if (!torquePath.empty())
         mat->bumpFilename[0] = cleanTextureName(torquePath, cleanFile, path, false).c_str();
   }

   //if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texName))
   //{
   //   torquePath = texName.C_Str();
   //   if (!torquePath.empty())
   //      mat->file[0] = cleanTextureName(torquePath, cleanFile, path, false);
   //}

  ColorF specularColor(1.0f, 1.0f, 1.0f, 1.0f);
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_COLOR_SPECULAR, read_color))
      specularColor.set(read_color.r, read_color.g, read_color.b, opacity);
   mat->specular[0] = specularColor;

   // Specular Power
   F32 specularPower = 1.0f;
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_SHININESS_STRENGTH, specularPower))
      mat->specularPower[0] = specularPower;

   // Specular
   F32 specularStrength = 0.0f;
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_SHININESS, specularStrength))
      mat->specular[0] = ColorF(specularStrength, specularStrength, specularStrength, specularStrength);

   // Double-Sided
   bool doubleSided = false;
   S32 dbl_sided = 0;
   if (AI_SUCCESS == mAIMat->Get(AI_MATKEY_TWOSIDED, dbl_sided))
      doubleSided = (dbl_sided != 0);
   mat->doubleSided = doubleSided;
}

String AssimpAppMaterial::cleanTextureName(String& texName, String& shapeName, const Torque::Path& path, bool nameOnly /*= false*/)
{
   Torque::Path foundPath;
   String cleanStr;

   if (texName[0] == '*')
   {  // It's an embedded texture reference. Make the cached name and return
      cleanStr = shapeName;
      cleanStr += "_cachedTex";
      cleanStr += texName.substr(1);
      return cleanStr;
   }

   // See if the file exists
   bool fileFound = false;
   String testPath = path.getPath();
   testPath += '/';
   testPath += texName;
   std::replace(testPath.begin(), testPath.end(), '\\', '/');
   fileFound = Platform::isFile(testPath.c_str());

   cleanStr = texName;
   std::replace(cleanStr.begin(), cleanStr.end(), '\\', '/');
   if (fileFound)
   {
      if (cleanStr == texName)
         return cleanStr;
      foundPath = testPath;
   }

   if (fileFound)
   {
       cleanStr = foundPath.getFullFileName();
   }
   else if (nameOnly)
      cleanStr += " (Not Found)";
 
   return cleanStr;
}
