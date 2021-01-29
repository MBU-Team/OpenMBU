//*****************************************************************************
// Cube materials
//*****************************************************************************
new Material(BoxEdge)
{
   mapTo = box_edge;
   baseTex[0] = "box_edge";
   bumpTex[0] = "knot3bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;

};

new Material(One)
{
   mapTo = one;
   baseTex[0] = "one";
   emissive[0] = true;
   glow[0] = true;
};

new CustomMaterial( TwoFallback )
{
   version = 1.1;
   texture[0] = "two";
   texture[1] = "metalcrate_bump";
   shader = DiffuseBump;
};

new CustomMaterial(Two)
{
   mapTo = six;
   version = 2.0;
   shader = Waves;
   texture[0] = "two";
   texture[2] = "metalcrate_bump";
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 16;
   fallback = TwoFallback;
};

new Material(ThreeBump)
{
   mapTo = three;
   
   baseTex[0] = "three";
   bumpTex[0] = "metalcrate_bump";

   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;

   animFlags[0] = $scale | $wave;
   waveType[0] = Sin;
   waveFreq[0] = 0.25;
   waveAmp[0] = 1.0;
};

new Material(FourBump)
{
   mapTo = four;

   // stage 0
   baseTex[0] = "vertBar";

   animFlags[0] = $scroll;
   scrollDir[0] = "1.0 0.0";
   scrollSpeed[0] = 8.0;
   emissive[0] = true;

   // stage 1
   baseTex[1] = "testMask";
   bumpTex[1] = "metalcrate_bump";

   pixelSpecular[1] = true;
   specular[1] = "1.0 1.0 1.0 1.0";
   specularPower[1] = 32.0;


};

new Material(FiveBump)
{
   mapTo = five;
   
   baseTex[0] = "five";
   bumpTex[0] = "metalcrate_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.8 0.8";
   specularPower[0] = 16.0;

   animFlags[0] = $rotate | $wave;
   rotPivotOffset[0] = "-0.5 -0.5";
   rotSpeed[0] = 1.0;

   waveType[0] = Sin;
   waveFreq[0] = 0.25;
   waveAmp[0] = 1.0;

};

new CustomMaterial(SixBumpFallbackPass2)
{
   texture[0] = "six";
   texture[1] = "$fog";

   blendOp = Mul;
   shader = DiffuseFog;
   version = 1.1;
};

new CustomMaterial(SixBumpFallback)
{
   texture[0] = "metalcrate_bump";
   texture[3] = "$cubemap";

   cubemap = Lobby;
   shader = BumpCubemap;
   version = 1.1;
   pass[0] = SixBumpFallbackPass2;
};

new CustomMaterial(SixBump)
{
   mapTo = two;
   
   texture[0] = "metalcrate_bump";
   texture[1] = "six";
   texture[3] = "$cubemap";

   cubemap = Lobby;

   shader = BumpCubeDiff;

   specular = "1.0 1.0 1.0 0.0";
   specularPower = 20.0;

   version = 2.0;
   fallback = SixBumpFallback;
};

