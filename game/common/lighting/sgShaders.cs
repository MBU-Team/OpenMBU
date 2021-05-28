//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------


new ShaderData(TerrDynamicLightingShader)
{
   DXVertexShaderFile   = "shaders/legacyTerrain/terrainDynamicLightingV.hlsl";
   DXPixelShaderFile    = "shaders/legacyTerrain/terrainDynamicLightingP.hlsl";
   pixVersion = 1.1;
};

new ShaderData(AtlasDynamicLightingShader)
{
   DXVertexShaderFile   = "shaders/atlas/atlasSurfaceDynamicLightingV.hlsl";
   DXPixelShaderFile    = "shaders/atlas/atlasSurfaceDynamicLightingP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(ShadowBuilderShader_3_0)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(ShadowShader_3_0)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowShaderP_3_0.hlsl";
   pixVersion = 3.0;
};

new ShaderData(ShadowBuilderShader_2_0)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(ShadowShader_2_0)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(ShadowBuilderShader_1_1)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderV_1_1.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowBuilderShaderP_1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData(ShadowShader_1_1)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/ShadowShaderV_1_1.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/ShadowShaderP_1_1.hlsl";
   pixVersion = 1.1;
};

new ShaderData(AlphaBloomShader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/AlphaBloomShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/AlphaBloomShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(DownSample4x4Shader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/DownSample8x4ShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/DownSample8x4ShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(DownSample4x4BloomClampShader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/DownSample4x4BloomClampShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/DownSample4x4BloomClampShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(BloomBlurShader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/BloomBlurShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/BloomBlurShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(DRLFullShader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/DRLShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/DRLShaderP.hlsl";
   pixVersion = 2.0;
};

new ShaderData(DRLOnlyBloomToneShader)
{
   DXVertexShaderFile 	= "shaders/lightingSystem/DRLShaderV.hlsl";
   DXPixelShaderFile 	= "shaders/lightingSystem/DRLOnlyBloomToneShaderP.hlsl";
   pixVersion = 2.0;
};



