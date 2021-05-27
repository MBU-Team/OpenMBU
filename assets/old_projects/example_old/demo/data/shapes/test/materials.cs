//*****************************************************************************
// Sphere
//*****************************************************************************
new Material(SphereGlow)
{
   mapTo = sphere_metal;
   baseTex[0] = "~/data/shapes/test/blobtex1";
   emissive[0] = true;
   glow[0] = true;
};

new Material(SphereKnobGlow)
{
   mapTo = sphere_rubber;
   baseTex[0] = "~/data/shapes/test/glowGrad";
   emissive[0] = true;
   glow[0] = true;

   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 -1.0";
   scrollSpeed[0] = 0.2;
};

new CustomMaterial( DynCubeSphereFallback )
{
   texture[0] = "$dynamicCubemap";
   shader = ReflectSphere1_1;
   version = 1.1;
};

new CustomMaterial( DynCubeSphere )
{
   mapTo = sphere_reflect;
   texture[0] = "$dynamicCubemap";
   shader = ReflectSphere;
   version = 2.0;

   fallback = DynCubeSphereFallback;
};

new CustomMaterial( TendrilFallback )
{
   texture[0] = "$backbuff";
   shader = TendrilShader1_1;

   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 16.0;

   refract = true;
   version = 1.1;
};

new CustomMaterial( Tendril )
{
   mapTo = sphere_refract;
   texture[0] = "$backbuff";
   shader = TendrilShader;

   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 16.0;

   refract = true;
   version = 2.0;
   fallback = TendrilFallback;
};


//*****************************************************************************
// Knot
//*****************************************************************************
new CustomMaterial(OuterKnotFallback)
{
   shader = OuterKnotShader1_1;
   version = 1.1;

   texture[0] = "~/data/shapes/test/knot3";
   texture[1] = "~/data/shapes/test/knot3bump";
   texture[2] = "$cubemap";

   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 1.0";
   scrollSpeed[0] = 0.4;

   cubemap = Chrome;
};

new CustomMaterial(OuterKnot)
{
   mapTo = knot3;
   shader = OuterKnotShader;
   version = 2.0;

   texture[0] = "~/data/shapes/test/knot3";
   texture[1] = "~/data/shapes/test/knot3bump";
   texture[2] = "$cubemap";

   specular[0] = "0.6 0.6 0.6 0.6";
   specularPower[0] = 16.0;

   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 1.0";
   scrollSpeed[0] = 0.4;

   cubemap = Chrome;
   fallback = OuterKnotFallback;
};

new Material(KnotGlow)
{
   mapTo = knot;
   baseTex[0] = "~/data/shapes/test/red";
   emissive[0] = true;
   glow[0] = true;
   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 -1.0";
   scrollSpeed[0] = 0.2;
};

new Material(InnerKnot)
{
   mapTo = knot2;
   baseTex[0] = "~/data/shapes/test/knot2";
   pixelSpecular[0] = true;
   specular[0] = "0.85 0.9 1.0 1.0";
   specularPower[0] = 8.0;
};


//*****************************************************************************
// Blob 
//*****************************************************************************
new CustomMaterial( BlobSurface1Fallback )
{
   texture[0] = "$backbuff";
   texture[1] = "~/data/shapes/test/yellowRedGrad";
   shader = BlobRefractVert1_1;
   version = 1.1;
   refract = true;
};

new CustomMaterial( BlobSurface1 )
{
   mapTo = blobtex1;
   texture[0] = "$backbuff";
   texture[1] = "~/data/shapes/test/yellowRedGrad";
   shader = BlobRefractVert;
   version = 2.0;
   refract = true;

   specular[0] = "0.85 0.85 0.85 0.85";
   specularPower[0] = 16.0;

   fallback = BlobSurface1Fallback;
};

new CustomMaterial( BlobSurface2Fallback )
{
   texture[0] = "$backbuff";
   texture[1] = "~/data/shapes/test/cyanBlueGrad";
   shader = BlobRefractVert1_1;
   version = 1.1;
   refract = true;
};

new CustomMaterial( BlobSurface2 )
{
   mapTo = blobtex2;
   texture[0] = "$backbuff";
   texture[1] = "~/data/shapes/test/cyanBlueGrad";
   texture[2] = "~/data/shapes/test/refractTest2";
   shader = BlobRefractPix;
   version = 2.0;
   refract = true;

   specular[0] = "0.85 0.85 0.85 0.85";
   specularPower[0] = 16.0;

   fallback = BlobSurface2Fallback;
};


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
   texture[1] = "~/data/interiors/test/metalcrate_bump";
   shader = DiffuseBump;
};

new CustomMaterial(Two)
{
   mapTo = six;
   version = 2.0;
   shader = Waves;
   texture[0] = "two";
   texture[2] = "~/data/interiors/test/metalcrate_bump";
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 16;
   fallback = TwoFallback;
};

new Material(ThreeBump)
{
   mapTo = three;
   
   baseTex[0] = "three";
   bumpTex[0] = "~/data/interiors/test/metalcrate_bump";

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
   bumpTex[1] = "~/data/interiors/test/metalcrate_bump";

   pixelSpecular[1] = true;
   specular[1] = "1.0 1.0 1.0 1.0";
   specularPower[1] = 32.0;


};

new Material(FiveBump)
{
   mapTo = five;
   
   baseTex[0] = "five";
   bumpTex[0] = "~/data/interiors/test/metalcrate_bump";

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
   texture[0] = "~/data/interiors/test/metalcrate_bump";
   texture[3] = "$cubemap";

   cubemap = Lobby;
   shader = BumpCubemap;
   version = 1.1;
   pass[0] = SixBumpFallbackPass2;
};

new CustomMaterial(SixBump)
{
   mapTo = two;
   
   texture[0] = "~/data/interiors/test/metalcrate_bump";
   texture[1] = "six";
   texture[3] = "$cubemap";

   cubemap = Lobby;

   shader = BumpCubeDiff;

   specular = "1.0 1.0 1.0 0.0";
   specularPower = 20.0;

   version = 2.0;
   fallback = SixBumpFallback;
};

