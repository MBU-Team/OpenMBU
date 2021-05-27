//*****************************************************************************
// Shaders  ( For Custom Materials )
//*****************************************************************************

//-----------------------------------------------------------------------------
// Lightmap with light-normal and bump maps
//-----------------------------------------------------------------------------
new ShaderData( LmapBump )
{
   DXVertexShaderFile   = "shaders/BaseInteriorV.hlsl";
   DXPixelShaderFile    = "shaders/BaseInteriorP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Reflect cubemap
//-----------------------------------------------------------------------------
new ShaderData( Cubemap )
{
   DXVertexShaderFile   = "shaders/lmapCubeV.hlsl";
   DXPixelShaderFile    = "shaders/lmapCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap
//-----------------------------------------------------------------------------
new ShaderData( BumpCubemap )
{
   DXVertexShaderFile   = "shaders/bumpCubeV.hlsl";
   DXPixelShaderFile    = "shaders/bumpCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Bump
//-----------------------------------------------------------------------------
new ShaderData( DiffuseBump )
{
   DXVertexShaderFile   = "shaders/diffBumpV.hlsl";
   DXPixelShaderFile    = "shaders/diffBumpP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Fog
//-----------------------------------------------------------------------------
new ShaderData( DiffuseFog )
{
   DXVertexShaderFile   = "shaders/diffuseFogV.hlsl";
   DXPixelShaderFile    = "shaders/diffuseFogP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap with diffuse texture
//-----------------------------------------------------------------------------
new ShaderData( BumpCubeDiff )
{
   DXVertexShaderFile   = "shaders/bumpCubeDiffuseV.hlsl";
   DXPixelShaderFile    = "shaders/bumpCubeDiffuseP.hlsl";
   pixVersion = 2.0;
};


//-----------------------------------------------------------------------------
// Fog Test
//-----------------------------------------------------------------------------
new ShaderData( FogTest )
{
   DXVertexShaderFile   = "shaders/fogTestV.hlsl";
   DXPixelShaderFile    = "shaders/fogTestP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Planar Reflection
//-----------------------------------------------------------------------------
new ShaderData( ReflectBump )
{
   DXVertexShaderFile   = "shaders/planarReflectBumpV.hlsl";
   DXPixelShaderFile    = "shaders/planarReflectBumpP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( Reflect )
{
   DXVertexShaderFile   = "shaders/planarReflectV.hlsl";
   DXPixelShaderFile    = "shaders/planarReflectP.hlsl";
   pixVersion = 1.4;
};

//-----------------------------------------------------------------------------
// Vertex refraction
//-----------------------------------------------------------------------------
new ShaderData( RefractVert )
{
   DXVertexShaderFile   = "shaders/refractVertV.hlsl";
   DXPixelShaderFile    = "shaders/refractVertP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shaders for blob
//-----------------------------------------------------------------------------
new ShaderData( BlobRefractVert )
{
   DXVertexShaderFile   = "shaders/blobRefractVertV.hlsl";
   DXPixelShaderFile    = "shaders/blobRefractVertP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( BlobRefractVert1_1 )
{
   DXVertexShaderFile   = "shaders/blobRefractVertV1_1.hlsl";
   DXPixelShaderFile    = "shaders/blobRefractVertP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( BlobRefractPix )
{
   DXVertexShaderFile   = "shaders/blobRefractPixV.hlsl";
   DXPixelShaderFile    = "shaders/blobRefractPixP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shader for reflective sphere
//-----------------------------------------------------------------------------
new ShaderData( ReflectSphere )
{
   DXVertexShaderFile   = "shaders/reflectSphereV.hlsl";
   DXPixelShaderFile    = "shaders/reflectSphereP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ReflectSphere1_1 )
{
   DXVertexShaderFile   = "shaders/reflectSphereV1_1.hlsl";
   DXPixelShaderFile    = "shaders/reflectSphereP1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData( TendrilShader )
{
   DXVertexShaderFile   = "shaders/tendrilV.hlsl";
   DXPixelShaderFile    = "shaders/tendrilP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( TendrilShader1_1 )
{
   DXVertexShaderFile   = "shaders/tendrilV1_1.hlsl";
   DXPixelShaderFile    = "shaders/tendrilP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Blank shader - to draw to z buffer before rendering rest of scene
//-----------------------------------------------------------------------------
new ShaderData( BlankShader )
{
   DXVertexShaderFile   = "shaders/blankV.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Outer Knot
//-----------------------------------------------------------------------------
new ShaderData( OuterKnotShader )
{
   DXVertexShaderFile   = "shaders/outerKnotV.hlsl";
   DXPixelShaderFile    = "shaders/outerKnotP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( OuterKnotShader1_1 )
{
   DXVertexShaderFile   = "shaders/outerKnotV1_1.hlsl";
   DXPixelShaderFile    = "shaders/outerKnotP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Waves
//-----------------------------------------------------------------------------
new ShaderData( Waves )
{
   DXVertexShaderFile   = "shaders/wavesV.hlsl";
   DXPixelShaderFile    = "shaders/wavesP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Terrain Shaders
//-----------------------------------------------------------------------------
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
