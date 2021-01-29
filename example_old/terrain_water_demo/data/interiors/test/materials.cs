//*****************************************************************************
// Materials
//*****************************************************************************
new Material(solidmetal)
{
   mapTo = smt_cemwll;
   
   baseTex[0] = "solidmetal";
   bumpTex[0] = "solidmetal_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.1";
   specularPower[0] = 32.0;
};

