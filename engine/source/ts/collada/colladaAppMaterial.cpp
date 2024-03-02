//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/loader/tsShapeLoader.h"
#include "ts/collada/colladaAppMaterial.h"
#include "ts/collada/colladaUtils.h"
#include <algorithm>

using namespace ColladaUtils;

String cleanString(const String& str)
{
   String cleanStr(str);
   const String badChars(" -,.+=*/");
   for (char i = 0; i < badChars.length(); i++)
       std::replace(cleanStr.begin(), cleanStr.end(), badChars[i], '_');
   return cleanStr;
}

//------------------------------------------------------------------------------

ColladaAppMaterial::ColladaAppMaterial(const domMaterial *pMat)
:  mat(pMat),
   diffuseColor(ColorF(1,1,1,1)),
   specularColor(ColorF(1,1,1,1)),
   specularPower(8.0f),
   pixelSpecular(false),
   emissive(false),
   doubleSided(false)
{
   // Get the material name
   name = ColladaUtils::getOptions().matNamePrefix + _GetNameOrId(mat);

   // Get the effect element for this material
   effect = daeSafeCast<domEffect>(mat->getInstance_effect()->getUrl().getElement());
   effectExt = new ColladaExtension_effect(effect);

   // Get the <profile_COMMON>, <diffuse> and <specular> elements
   const domProfile_COMMON* commonProfile = ColladaUtils::findEffectCommonProfile(effect);
   const domCommon_color_or_texture_type_complexType* domDiffuse = findEffectDiffuse(effect);
   const domCommon_color_or_texture_type_complexType* domSpecular = findEffectSpecular(effect);

   // Wrap flags
   if (effectExt->wrapU)
      flags |= TSMaterialList::S_Wrap;
   if (effectExt->wrapV)
      flags |= TSMaterialList::T_Wrap;

   // MipMap flags
   const domFx_sampler2D_common_complexType* sampler2D = getTextureSampler(effect, domDiffuse);
   if (sampler2D) {
      if (sampler2D->getMipmap_maxlevel() &&
         sampler2D->getMipmap_maxlevel()->getValue() == 0)
         flags |= TSMaterialList::NoMipMap;

      if (sampler2D->getBorder_color() &&
         sampler2D->getBorder_color()->getValue()[0] == 0 &&
         sampler2D->getBorder_color()->getValue()[1] == 0 &&
         sampler2D->getBorder_color()->getValue()[2] == 0)
         flags |= TSMaterialList::MipMap_ZeroBorder;
   }

   // Set material attributes
   if (commonProfile) {

      F32 transparency = 0.0f;
      if (commonProfile->getTechnique()->getConstant()) {
         const domProfile_COMMON::domTechnique::domConstant* constant = commonProfile->getTechnique()->getConstant();
         diffuseColor.set(1.0f, 1.0f, 1.0f, 1.0f);
         resolveColor(constant->getReflective(), &specularColor);
         resolveFloat(constant->getReflectivity(), &specularPower);
         resolveTransparency(constant, &transparency);

         flags |= TSMaterialList::SelfIlluminating;   // material is SelfIlluminated if it uses the <constant> shader
         emissive = true;                             // set emissive flag because constant material is not affected by lighting
      }
      else if (commonProfile->getTechnique()->getLambert()) {
         const domProfile_COMMON::domTechnique::domLambert* lambert = commonProfile->getTechnique()->getLambert();
         resolveColor(lambert->getDiffuse(), &diffuseColor);
         resolveColor(lambert->getReflective(), &specularColor);
         resolveFloat(lambert->getReflectivity(), &specularPower);
         resolveTransparency(lambert, &transparency);
         emissive = false;
      }
      else if (commonProfile->getTechnique()->getPhong()) {
         const domProfile_COMMON::domTechnique::domPhong* phong = commonProfile->getTechnique()->getPhong();
         resolveColor(phong->getDiffuse(), &diffuseColor);
         resolveColor(phong->getSpecular(), &specularColor);
         resolveFloat(phong->getShininess(), &specularPower);
         resolveTransparency(phong, &transparency);
         emissive = false;
      }
      else if (commonProfile->getTechnique()->getBlinn()) {
         const domProfile_COMMON::domTechnique::domBlinn* blinn = commonProfile->getTechnique()->getBlinn();
         resolveColor(blinn->getDiffuse(), &diffuseColor);
         resolveColor(blinn->getSpecular(), &specularColor);
         resolveFloat(blinn->getShininess(), &specularPower);
         resolveTransparency(blinn, &transparency);
         emissive = false;
      }

      // Set translucency flags
      if (transparency != 0.0f) {
         flags |= TSMaterialList::Translucent;
         if (transparency > 1.0f) {
            flags |= TSMaterialList::Additive;
            diffuseColor.alpha = transparency - 1.0f;
         }
         else if (transparency < 0.0f) {
            flags |= TSMaterialList::Subtractive;
            diffuseColor.alpha = -transparency;
         }
         else {
            diffuseColor.alpha = transparency;
         }
      }
      else
         diffuseColor.alpha = 1.0f;
   }

   // Disable environment mapping if material reflectivity is zero
   if (specularPower == 0)
      flags |= TSMaterialList::NeverEnvMap;

   // Double-sided flag
   doubleSided = effectExt->double_sided;

   // Set pixelSpecular flag to ensure geometry is rendered even if the material
   // texture cannot be found
   //
   // MDF: pixelSpecular = true; looks very wrong in most cases so I am disabling it
   pixelSpecular = false;

   // Get the paths for the various textures => Collada indirection at its finest!
   // <texture>.<newparam>.<sampler2D>.<source>.<newparam>.<surface>.<init_from>.<image>.<init_from>
   diffuseMap = getSamplerImagePath(effect, getTextureSampler(effect, domDiffuse));
   specularMap = getSamplerImagePath(effect, getTextureSampler(effect, domSpecular));
   normalMap = getSamplerImagePath(effect, effectExt->bumpSampler);

   // If the diffuse texture could not be found, set the 'emissive' flag so
   // Torque colors the (blank) material using the diffuse color
   // @todo: This isn't right, but Torque seems to ignore the diffuse color
   // if emissive is not set
   if (diffuseMap.empty())
      emissive = true;
}

void ColladaAppMaterial::resolveFloat(const domCommon_float_or_param_type* value, F32* dst)
{
   if (value && value->getFloat()) {
      *dst = value->getFloat()->getValue();
   }
}

void ColladaAppMaterial::resolveColor(const domCommon_color_or_texture_type* value, ColorF* dst)
{
   if (value && value->getColor()) {
      dst->red = value->getColor()->getValue()[0];
      dst->green = value->getColor()->getValue()[1];
      dst->blue = value->getColor()->getValue()[2];
      dst->alpha = value->getColor()->getValue()[3];
   }
}

// Generate a script Material object
#define writeLine(str) { stream.write((int)dStrlen(str), str); stream.write(2, "\r\n"); }

void writeValue(Stream & stream, const char *name, const char *value)
{
   writeLine(avar("\t%s = \"%s\";", name, value));
}

void writeValue(Stream & stream, const char *name, bool value)
{
   writeLine(avar("\t%s = %s;", name, value ? "true" : "false"));
}

void writeValue(Stream & stream, const char *name, F32 value)
{
   writeLine(avar("\t%s = %g;", name, value));
}

void writeValue(Stream & stream, const char *name, const ColorF& color)
{
   writeLine(avar("\t%s = \"%g %g %g %g\";", name, color.red, color.green, color.blue, color.alpha));
}

void ColladaAppMaterial::write(Stream & stream)
{
   String cleanFile = cleanString(TSShapeLoader::getShapePath().getFileName());
   String cleanName = cleanString(getName());

   writeLine(avar("singleton Material(%s_%s)", cleanFile.c_str(), cleanName.c_str()));
   writeLine("{");

   writeValue(stream, "mapTo", getName().c_str());
   writeLine("");

   writeValue(stream, "diffuseMap[0]", diffuseMap.c_str());
   writeValue(stream, "normalMap[0]", normalMap.c_str());
   writeValue(stream, "specularMap[0]", specularMap.c_str());
   writeLine("");

   writeValue(stream, "diffuseColor[0]", diffuseColor);
   writeValue(stream, "specular[0]", specularColor);
   writeValue(stream, "specularPower[0]", specularPower);
   writeValue(stream, "pixelSpecular[0]", pixelSpecular);
   writeValue(stream, "emissive[0]", emissive);
   writeLine("");

   writeValue(stream, "doubleSided", doubleSided);
   writeValue(stream, "translucent", (bool)(flags & TSMaterialList::Translucent));
   writeValue(stream, "translucentBlendOp", (flags & TSMaterialList::Additive) ? "Add" :
                                    ((flags & TSMaterialList::Subtractive) ? "Sub" : "None"));

   writeLine("};");
   writeLine("");
}
