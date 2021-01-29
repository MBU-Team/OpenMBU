//*****************************************************************************
// Shaders  ( For Custom Materials )
//*****************************************************************************

//datablock ShaderData( MetallicCartoonShader )
//{
//   DXVertexShaderFile = "shaders/metallicCartoonV.hlsl";
//   DXPixelShaderFile = "shaders/metallicCartoonP.hlsl";
//   pixVersion = 2.0;
//};

//-----------------------------------------------------------------------------
// Lightmap with light-normal and bump maps
//-----------------------------------------------------------------------------
datablock ShaderData( LmapBump )
{
   DXVertexShaderFile 	= "shaders/BaseInteriorV.hlsl";
   DXPixelShaderFile 	= "shaders/BaseInteriorP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Reflect cubemap
//-----------------------------------------------------------------------------
datablock ShaderData( Cubemap )
{
   DXVertexShaderFile 	= "shaders/lmapCubeV.hlsl";
   DXPixelShaderFile 	= "shaders/lmapCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap
//-----------------------------------------------------------------------------
datablock ShaderData( BumpCubemap )
{
   DXVertexShaderFile 	= "shaders/bumpCubeV.hlsl";
   DXPixelShaderFile 	= "shaders/bumpCubeP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Bump
//-----------------------------------------------------------------------------
datablock ShaderData( DiffuseBump )
{
   DXVertexShaderFile 	= "shaders/diffBumpV.hlsl";
   DXPixelShaderFile 	= "shaders/diffBumpP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Diffuse + Fog
//-----------------------------------------------------------------------------
datablock ShaderData( DiffuseFog )
{
   DXVertexShaderFile 	= "shaders/diffuseFogV.hlsl";
   DXPixelShaderFile 	= "shaders/diffuseFogP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Bump reflect cubemap with diffuse texture
//-----------------------------------------------------------------------------
datablock ShaderData( BumpCubeDiff )
{
   DXVertexShaderFile 	= "shaders/bumpCubeDiffuseV.hlsl";
   DXPixelShaderFile 	= "shaders/bumpCubeDiffuseP.hlsl";
   pixVersion = 2.0;
};


//-----------------------------------------------------------------------------
// Fog Test
//-----------------------------------------------------------------------------
datablock ShaderData( FogTest )
{
   DXVertexShaderFile 	= "shaders/fogTestV.hlsl";
   DXPixelShaderFile 	= "shaders/fogTestP.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Planar Reflection
//-----------------------------------------------------------------------------
datablock ShaderData( ReflectBump )
{
   DXVertexShaderFile 	= "shaders/planarReflectBumpV.hlsl";
   DXPixelShaderFile 	= "shaders/planarReflectBumpP.hlsl";
   pixVersion = 2.0;
};

datablock ShaderData( Reflect )
{
   DXVertexShaderFile 	= "shaders/planarReflectV.hlsl";
   DXPixelShaderFile 	= "shaders/planarReflectP.hlsl";
   pixVersion = 1.4;
};

//-----------------------------------------------------------------------------
// Vertex refraction
//-----------------------------------------------------------------------------
datablock ShaderData( RefractVert )
{
   DXVertexShaderFile 	= "shaders/refractVertV.hlsl";
   DXPixelShaderFile 	= "shaders/refractVertP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shaders for blob
//-----------------------------------------------------------------------------
datablock ShaderData( BlobRefractVert )
{
   DXVertexShaderFile 	= "shaders/blobRefractVertV.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractVertP.hlsl";
   pixVersion = 2.0;
};

datablock ShaderData( BlobRefractVert1_1 )
{
   DXVertexShaderFile 	= "shaders/blobRefractVertV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractVertP1_1.hlsl";
   pixVersion = 1.1;
};

datablock ShaderData( BlobRefractPix )
{
   DXVertexShaderFile 	= "shaders/blobRefractPixV.hlsl";
   DXPixelShaderFile 	= "shaders/blobRefractPixP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Custom shader for reflective sphere
//-----------------------------------------------------------------------------
datablock ShaderData( ReflectSphere )
{
   DXVertexShaderFile 	= "shaders/reflectSphereV.hlsl";
   DXPixelShaderFile 	= "shaders/reflectSphereP.hlsl";
   pixVersion = 2.0;
};

datablock ShaderData( ReflectSphere1_1 )
{
   DXVertexShaderFile 	= "shaders/reflectSphereV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/reflectSphereP1_1.hlsl";
   pixVersion = 1.1;
};

datablock ShaderData( TendrilShader )
{
   DXVertexShaderFile 	= "shaders/tendrilV.hlsl";
   DXPixelShaderFile 	= "shaders/tendrilP.hlsl";
   pixVersion = 2.0;
};

datablock ShaderData( TendrilShader1_1 )
{
   DXVertexShaderFile 	= "shaders/tendrilV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/tendrilP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Blank shader - to draw to z buffer before rendering rest of scene
//-----------------------------------------------------------------------------
datablock ShaderData( BlankShader )
{
   DXVertexShaderFile 	= "shaders/blankV.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Outer Knot
//-----------------------------------------------------------------------------
datablock ShaderData( OuterKnotShader )
{
   DXVertexShaderFile 	= "shaders/outerKnotV.hlsl";
   DXPixelShaderFile 	= "shaders/outerKnotP.hlsl";
   pixVersion = 2.0;
};

datablock ShaderData( OuterKnotShader1_1 )
{
   DXVertexShaderFile 	= "shaders/outerKnotV1_1.hlsl";
   DXPixelShaderFile 	= "shaders/outerKnotP1_1.hlsl";
   pixVersion = 1.1;
};

//-----------------------------------------------------------------------------
// Waves
//-----------------------------------------------------------------------------
datablock ShaderData( Waves )
{
   DXVertexShaderFile 	= "shaders/wavesV.hlsl";
   DXPixelShaderFile 	= "shaders/wavesP.hlsl";
   pixVersion = 2.0;
};

// Flag settings - must match material.h

$standardTex = 1;
$detail = 2;
$bumpTex = 3;
$lightmapTex = 6;
$normLightmapTex = 7;
$mask = 8;

$scroll = 1;
$rotate = 2;
$wave   = 4;
$scale  = 8;
$sequence = 16;

$sinWave = 0;
$triangleWave = 1;
$squareWave = 2;

// matches Material::BlendOp
$mul = 1;
$add = 2;


// for writing to z buffer
datablock CustomMaterial( Blank )
{
   shader = BlankShader;
   version = 1.1;
};

