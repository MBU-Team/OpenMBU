//*****************************************************************************
// Shaders  ( For Custom Materials )
//*****************************************************************************

//-----------------------------------------------------------------------------
// Planar Reflection
//-----------------------------------------------------------------------------
new ShaderData( ReflectBump )
{
   DXVertexShaderFile 	= "shaders/planarReflectBumpV.hlsl";
   DXPixelShaderFile 	= "shaders/planarReflectBumpP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( Reflect )
{
   DXVertexShaderFile 	= "shaders/planarReflectV.hlsl";
   DXPixelShaderFile 	= "shaders/planarReflectP.hlsl";
   pixVersion = 1.4;
};

//-----------------------------------------------------------------------------
// Water
//-----------------------------------------------------------------------------
new ShaderData( WaterFres1_1 )
{
   DXVertexShaderFile 	= "shaders/water/WaterFresV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterFresP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( WaterBlend )
{
   DXVertexShaderFile 	= "shaders/water/WaterBlendV.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterBlendP.hlsl";
   pixVersion = 1.1;
};

new ShaderData( Water1_1 )
{
   DXVertexShaderFile 	= "shaders/water/WaterV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( WaterCubeReflectRefract )
{
   DXVertexShaderFile 	= "shaders/water/WaterCubeReflectRefractV.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterCubeReflectRefractP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( WaterReflectRefract )
{
   DXVertexShaderFile 	= "shaders/water/WaterReflectRefractV.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterReflectRefractP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( WaterUnder2_0 )
{
   DXVertexShaderFile 	= "shaders/water/WaterUnderV2_0.hlsl";
   DXPixelShaderFile 	= "shaders/water/WaterUnderP2_0.hlsl";
   pixVersion = 2.0;
};




//-----------------------------------------------------------------------------
// Lightmap with light-normal and bump maps
//-----------------------------------------------------------------------------
new ShaderData( LmapBump )
{
   DXVertexShaderFile 	= "shaders/BaseInteriorV.hlsl";
   DXPixelShaderFile 	= "shaders/BaseInteriorP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Reflect cubemap
//-----------------------------------------------------------------------------
new ShaderData( Cubemap )
{
   DXVertexShaderFile 	= "shaders/lmapCubeV.hlsl";
   DXPixelShaderFile 	= "shaders/lmapCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap
//-----------------------------------------------------------------------------
new ShaderData( BumpCubemap )
{
   DXVertexShaderFile 	= "shaders/bumpCubeV.hlsl";
   DXPixelShaderFile 	= "shaders/bumpCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Bump
//-----------------------------------------------------------------------------
new ShaderData( DiffuseBump )
{
   DXVertexShaderFile 	= "shaders/diffBumpV.hlsl";
   DXPixelShaderFile 	= "shaders/diffBumpP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Fog
//-----------------------------------------------------------------------------
new ShaderData( DiffuseFog )
{
   DXVertexShaderFile 	= "shaders/diffuseFogV.hlsl";
   DXPixelShaderFile 	= "shaders/diffuseFogP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap with diffuse texture
//-----------------------------------------------------------------------------
new ShaderData( BumpCubeDiff )
{
   DXVertexShaderFile 	= "shaders/bumpCubeDiffuseV.hlsl";
   DXPixelShaderFile 	= "shaders/bumpCubeDiffuseP.hlsl";
   pixVersion = 2.0;
};


//-----------------------------------------------------------------------------
// Fog Test
//-----------------------------------------------------------------------------
new ShaderData( FogTest )
{
   DXVertexShaderFile 	= "shaders/fogTestV.hlsl";
   DXPixelShaderFile 	= "shaders/fogTestP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Vertex refraction
//-----------------------------------------------------------------------------
new ShaderData( RefractVert )
{
   DXVertexShaderFile 	= "shaders/refractVertV.hlsl";
   DXPixelShaderFile 	= "shaders/refractVertP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shaders for blob
//-----------------------------------------------------------------------------
new ShaderData( BlobRefractVert )
{
   DXVertexShaderFile 	= "shaders/blobRefractVertV.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractVertP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( BlobRefractVert1_1 )
{
   DXVertexShaderFile 	= "shaders/blobRefractVertV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractVertP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( BlobRefractPix )
{
   DXVertexShaderFile 	= "shaders/blobRefractPixV.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractPixP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shader for reflective sphere
//-----------------------------------------------------------------------------
new ShaderData( ReflectSphere )
{
   DXVertexShaderFile 	= "shaders/reflectSphereV.hlsl";
   DXPixelShaderFile 	= "shaders/reflectSphereP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ReflectSphere1_1 )
{
   DXVertexShaderFile 	= "shaders/reflectSphereV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/reflectSphereP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( TendrilShader )
{
   DXVertexShaderFile 	= "shaders/tendrilV.hlsl";
   DXPixelShaderFile 	= "shaders/tendrilP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( TendrilShader1_1 )
{
   DXVertexShaderFile 	= "shaders/tendrilV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/tendrilP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Blank shader - to draw to z buffer before rendering rest of scene
//-----------------------------------------------------------------------------
new ShaderData( BlankShader )
{
   DXVertexShaderFile 	= "shaders/blankV.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Outer Knot
//-----------------------------------------------------------------------------
new ShaderData( OuterKnotShader )
{
   DXVertexShaderFile 	= "shaders/outerKnotV.hlsl";
   DXPixelShaderFile 	= "shaders/outerKnotP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( OuterKnotShader1_1 )
{
   DXVertexShaderFile 	= "shaders/outerKnotV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/outerKnotP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Waves
//-----------------------------------------------------------------------------
new ShaderData( Waves )
{
   DXVertexShaderFile 	= "shaders/wavesV.hlsl";
   DXPixelShaderFile 	= "shaders/wavesP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Terrain Shaders
//-----------------------------------------------------------------------------

new ShaderData( AtlasShader2 )
{
   DXVertexShaderFile   = "shaders/atlas/atlasSurfaceV2.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasSurfaceP2.hlsl";
   pixVersion = 1.1;
};

new ShaderData( AtlasShader3 )
{
   DXVertexShaderFile   = "shaders/atlas/atlasSurfaceV3.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasSurfaceP3.hlsl";
   pixVersion = 1.1;
};

new ShaderData( AtlasShader4 )
{
   DXVertexShaderFile   = "shaders/atlas/atlasSurfaceV4.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasSurfaceP4.hlsl";
   pixVersion = 1.1;
};

new ShaderData( AtlasShaderFog )
{
   DXVertexShaderFile   = "shaders/atlas/atlasFogV.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasFogP.hlsl";
   pixVersion = 1.1;
};

new ShaderData( AtlasBlender20Shader )
{
   DXVertexShaderFile   = "shaders/atlas/atlasBlenderPS20V.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasBlenderPS20P.hlsl";
   pixVersion = 2.0;
};

new ShaderData( AtlasBlender11AShader )
{
   DXVertexShaderFile     = "shaders/atlas/atlasBlenderPS11VA.hlsl";
   DXPixelShaderFile      = "shaders/atlas/atlasBlenderPS11PA.hlsl";
   pixVersion = 1.1;
};

new ShaderData( AtlasBlender11BShader )
{
   DXVertexShaderFile     = "shaders/atlas/atlasBlenderPS11VB.hlsl";
   DXPixelShaderFile      = "shaders/atlas/atlasBlenderPS11PB.hlsl";
   pixVersion = 1.1;
};

new ShaderData( TerrShader )
{
   DXVertexShaderFile   = "shaders/legacyTerrain/terrainV.hlsl";
   DXPixelShaderFile    = "shaders/legacyTerrain/terrainP.hlsl";
   pixVersion = 1.1;
};

new ShaderData( TerrBlender20Shader )
{
   DXVertexShaderFile     = "shaders/legacyTerrain/terrainBlenderPS20V.hlsl";
   DXPixelShaderFile      = "shaders/legacyTerrain/terrainBlenderPS20P.hlsl";
   pixVersion = 2.0;
};

new ShaderData( TerrBlender11AShader )
{
   DXVertexShaderFile     = "shaders/legacyTerrain/terrainBlenderPS11AV.hlsl";
   DXPixelShaderFile      = "shaders/legacyTerrain/terrainBlenderPS11AP.hlsl";
   pixVersion = 1.1;
};

new ShaderData( TerrBlender11BShader )
{
   DXVertexShaderFile     = "shaders/legacyTerrain/terrainBlenderPS11BV.hlsl";
   DXPixelShaderFile      = "shaders/legacyTerrain/terrainBlenderPS11BP.hlsl";
   pixVersion = 1.1;
};
