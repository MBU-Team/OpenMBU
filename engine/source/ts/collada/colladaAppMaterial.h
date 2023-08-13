//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COLLADA_APP_MATERIAL_H_
#define _COLLADA_APP_MATERIAL_H_

#ifndef _APPMATERIAL_H_
#include "ts/loader/appMaterial.h"
#endif
#ifndef _COLLADA_EXTENSIONS_H_
#include "ts/collada/colladaExtensions.h"
#endif

class ColladaAppMaterial : public AppMaterial
{
public:
   const domMaterial*         mat;              ///< Collada <material> element
   domEffect*                 effect;           ///< Collada <effect> element
   ColladaExtension_effect*   effectExt;        ///< effect extension
   String                     name;             ///< Name of this material (cleaned)

   // Settings extracted from the Collada file, and optionally saved to materials.cs
   String                     diffuseMap;
   String                     normalMap;
   String                     specularMap;
   ColorF                     diffuseColor;
   ColorF                     specularColor;
   F32                        specularPower;
   bool                       pixelSpecular;
   bool                       emissive;
   bool                       doubleSided;

   ColladaAppMaterial(const domMaterial* pMat);
   ~ColladaAppMaterial() { delete effectExt; }

   String getName() { return name; }

   void resolveFloat(const domCommon_float_or_param_type* value, F32* dst);
   void resolveColor(const domCommon_color_or_texture_type* value, ColorF* dst);

   // Determine the material transparency
   template<class T> void resolveTransparency(const T shader, F32* dst)
   {
      // Start out by getting the <transparency> element. If it doesn't exist, 0.0
      // (no transparency => fully opaque) will be used
      *dst = 0.0f;
      resolveFloat(shader->getTransparency(), dst);

      // Multiply the transparency by the transparent color
      if (shader->getTransparent() && shader->getTransparent()->getColor()) {
         const domCommon_color_or_texture_type::domColor* color = shader->getTransparent()->getColor();
         if (shader->getTransparent()->getOpaque() == FX_OPAQUE_ENUM_A_ONE) {
            // multiply by alpha value and invert (so 1.0 is fully opaque)
            *dst = 1.0f - (*dst * color->getValue()[3]);
         }
         else {
            // multiply by average of the RGB values
            F32 avg = (color->getValue()[0] + color->getValue()[1] + color->getValue()[2]) / 3;
            *dst *= avg;
         }
      }
   }

   void write(Stream & stream);
};

#endif // _COLLADA_APP_MATERIAL_H_
