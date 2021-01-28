//*****************************************************************************
// Materials
//*****************************************************************************

new Material(wall_light)
{
   baseTex[0] = "wall_light";

   emissive[0] = true;
   glow[0] = true;
};

new Material(plating)
{
   baseTex[0] = "plating";
   bumpTex[0] = "plating_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(t2plate)
{
   baseTex[0] = "t2plate";
   bumpTex[0] = "t2plate_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(solidmetal)
{
   baseTex[0] = "solidmetal";
   bumpTex[0] = "solidmetal_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(corridor_trim)
{
   baseTex[0] = "corridor_trim";
   bumpTex[0] = "corridor_trim_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(solidrust)
{
   baseTex[0] = "solidrust";
   bumpTex[0] = "solidrust_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(concrete)
{
   baseTex[0] = "concrete";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(ggsolidmetal)
{
   baseTex[0] = "ggsolidmetal";
   bumpTex[0] = "gg_bump";

   PixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(solidmetal_lights)
{
   mapTo = solidmetal_lights;
   baseTex[0] = "solidmetal_lights_glow";

   emmisive[0] = true;
   glow[0] = true;
   animFlags[0] = $sequence;
   sequenceFramePerSec[0] = 0.25;
   sequenceSegmentSize[0] = 0.25;

   baseTex[1] = "solidmetal_lights";
   bumpTex[1] = "solidmetal_lights_bump";
};

new Material(metalgrate)
{
   baseTex[0] = "metalgrate";
   bumpTex[0] = "metalgrate_bump";

   pixelSpecular[0] = true;
   specular[0] = "1 1 1 0.1";
   specularPower[0] = 16.0;
};

new Material(wall)
{
   baseTex[0] = "wall";
   bumpTex[0] = "wall_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(wall_flat)
{
   baseTex[0] = "wall_flat";
   bumpTex[0] = "wall_flat_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

new Material(aquagrad)
{
   mapTo = aquagrad;
   
   // stage 0
   baseTex[0] = "aquagrad_glow";

   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 1.0";
   scrollSpeed[0] = 0.1;

    glow[0] = true;
    emissive[0] = true;

   // stage 1
   baseTex[1] = "aquagrad";
   emmisive[1] = true;
};
