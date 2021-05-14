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

//-----------------------------------------------------------------------------
// Blank shader - to draw to z buffer before rendering rest of scene
//-----------------------------------------------------------------------------
new ShaderData( BlankShader )
{
   DXVertexShaderFile 	= "shaders/blankV.hlsl";
   pixVersion = 1.1;
};

// for writing to z buffer
new CustomMaterial( Blank )
{
   shader = BlankShader;
   version = 1.1;
};

//-----------------------------------------------------------------------------
// Blur shader for 1.1 pixel shaders
//-----------------------------------------------------------------------------
new ShaderData( Blur )
{
   DXVertexShaderFile   = "shaders/blurV.hlsl";
   DXPixelShaderFile    = "shaders/blurP.hlsl";
   pixVersion = 1.1;
};

new ShaderData( XBlur )
{
   DXVertexShaderFile 	= "shaders/xBlurV.hlsl";
   DXPixelShaderFile 	= "shaders/xBlurP.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Glow Buffer Datablock
//-----------------------------------------------------------------------------
new GlowBuffer(GlowBufferData)
{
   shader = XBlur; //Blur;
   xenonShader = XBlur;
};

//-----------------------------------------------------------------------------
// Stencil shadow shader
//-----------------------------------------------------------------------------
new ShaderData( Shader_Stencil )
{
   DXVertexShaderFile     = "shaders/stencilV.hlsl";
//   DXPixelShaderFile      = "shaders/stencilP.hlsl";
   pixVersion = 1.1;
};

new CustomMaterial( Material_Stencil )
{
   shader = Shader_Stencil;
   version = 1.1;
};

//-----------------------------------------------------------------------------
// Ambient + diffuse map + lightmap + fog for non-material mapped interiors
//-----------------------------------------------------------------------------
new ShaderData( Shader_InteriorBasic )
{
   DXVertexShaderFile     = "shaders/interiorBasicV.hlsl";
   DXPixelShaderFile      = "shaders/interiorBasicP.hlsl";
   pixVersion = 1.1;
};

new CustomMaterial( Material_InteriorBasic )
{
   shader = Shader_InteriorBasic;
   version = 1.1;
};
