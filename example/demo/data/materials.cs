//*****************************************************************************
// Custom Materials
//*****************************************************************************

// for writing to z buffer
new CustomMaterial( Blank )
{
   shader = BlankShader;
   version = 1.1;
};


//*****************************************************************************
// Environmental Materials
//*****************************************************************************

new CustomMaterial(AtlasDynamicLightingMaterial)
{
   texture[4] = "$dynamiclight";

   shader = AtlasDynamicLightingShader;
   version = 1.1;
};

new CustomMaterial(AtlasMaterial)
{  
   shader = AtlasShader;
   version = 1.1;
   
   dynamicLightingMaterial = AtlasDynamicLightingMaterial;
};

new CustomMaterial(AtlasBlender20Material)
{
   shader = AtlasBlender20Shader;
   version = 2.0;
};

new CustomMaterial(TerrainMaterialDynamicLighting)
{
   texture[2] = "$dynamiclight";
   
   shader = TerrDynamicLightingShader;
   version = 1.1;
};

new CustomMaterial(TerrainMaterial)
{
   shader = TerrShader;
   version = 1.1;
   
   dynamicLightingMaterial = TerrainMaterialDynamicLighting;
};

new CustomMaterial(TerrainBlenderPS20Material)
{
   shader = TerrBlender20Shader;
   version = 2.0;
};

new CustomMaterial(TerrainBlenderPS11AMaterial)
{
   shader = TerrBlender11AShader;
   pixVersion = 1.1;
};

new CustomMaterial(TerrainBlenderPS11BMaterial)
{
   shader = TerrBlender11BShader;
   pixVersion = 1.1;
};



