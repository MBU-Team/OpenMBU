//*****************************************************************************
// Orc Materials
//*****************************************************************************
new Material(OrcEye)
{
   baseTex[0] = "~/data/shapes/spaceOrc/orc_ID6_eye";
   emissive[0] = true;
   glow[0] = true;
};

new Material(SpaceOrc)
{
   baseTex[0] = "~/data/shapes/SpaceOrc/Orc_Material";
   bumpTex[0] = "~/data/shapes/SpaceOrc/Orc_Material_normal";
   cubemap = WChrome;
   pixelSpecular[0] = true;
   specular[0] = "0.5 0.5 0.5 0.5";
   specularPower[0] = 8.0;
};

//*****************************************************************************
// Gun Materials
//*****************************************************************************
new Material(GunMetal)
{
   baseTex[0] = "~/data/shapes/spaceOrc/Gun_Material";
   bumpTex[0] = "~/data/shapes/spaceOrc/Gun_Material_Normal";
//   cubemap = WChrome;
   pixelSpecular[0] = true;
//   specular[0] = "0.58 0.65 0.71 1.0";
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
   //emissive[1] = true;
   //glow[1] = true;
};

new Material(GunCamEyeThing)
{
   mapTo = "gun_ID5_cameyething";
   baseTex[0] = "~/data/shapes/spaceOrc/orc_ID6_eye";
   emissive[0] = true;
   glow[0] = true;
};

