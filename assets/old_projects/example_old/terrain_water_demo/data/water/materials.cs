//*****************************************************************************
// Water Materials
//*****************************************************************************
new CustomMaterial( Water1_1FogPass )
{
   texture[1] = "waterFres04";
   texture[2] = "$cubemap";
   texture[3] = "$fog";
   shader = WaterFres1_1;
   blendOp = LerpAlpha;
   cubemap = Sky_Day_Blur02;
   version = 1.1;

   translucent = true;
   translucentBlendOp = LerpAlpha;
};


new CustomMaterial( WaterBlendMat )
{
   texture[0] = "noise03";
   texture[1] = "noise03";
   shader = WaterBlend;
   version = 1.1;
};

// Main 1.1 water surface
new CustomMaterial( WaterFallback1_1 )
{
   texture[0] = "$miscbuff";
   texture[3] = "$cubemap";
   shader = Water1_1;
   cubemap = Sky_Day_Blur02;
   version = 1.1;
};


// Main 2.0 water surface
new CustomMaterial( Water )
{
   texture[0] = "noise02";
   texture[1] = "$reflectbuff";
   texture[2] = "$backbuff";
   texture[3] = "$fog";
   texture[4] = "$cubemap";
   cubemap = Sky_Day_Blur02;
   shader = WaterCubeReflectRefract;
   specular = "0.75 0.75 0.75 1.0";
   specularPower = 48.0;
   fallback = WaterFallback1_1;
   version = 2.0;
};

// This water material/shader is better suited for use inside interiors
new CustomMaterial( Water_Interior )
{
   texture[0] = "noise02";
   texture[1] = "$reflectbuff";
   texture[2] = "$backbuff";
   texture[3] = "$fog";
   shader = WaterReflectRefract;
   specular = "0.75 0.75 0.75 1.0";
   specularPower = 48.0;
   fallback = WaterFallback1_1;
   version = 2.0;
};


new CustomMaterial( Underwater1_1 )
{
   texture[0] = "$miscbuff";
   texture[3] = "$cubemap";
   shader = Water1_1;
   cubemap = Sky_Day_Blur02;
   version = 1.1;
   translucent = true;
   translucentBlendOp = LerpAlpha;
};

new CustomMaterial( Underwater )
{
   texture[0] = "noise01";
   texture[1] = "$reflectbuff";
   texture[2] = "$backbuff";
   texture[3] = "$fog";
   shader = WaterUnder2_0;
   version = 2.0;
   fallback = Underwater1_1;
};
