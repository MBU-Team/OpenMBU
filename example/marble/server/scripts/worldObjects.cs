// UPDATED ASTROLABE SHAPES (SEPARATE CLOUD LAYERS)

datablock TSShapeConstructor(astrolabe_Shape)
{
  baseShape = "~/data/shapes/astrolabe/astrolabe.dts";
  sequence0 = "~/data/shapes/astrolabe/astrolabe_root.dsq root";
};

datablock StaticShapeData(astrolabeShape)
{
   className = "astrolabe";
   category = "Misc";
   shapeFile = "~/data/shapes/astrolabe/astrolabe.dts";
   scopeAlways = true;
   sortLast = true;
   astrolabe = true;
   astrolabePrime = false;
};

datablock StaticShapeData(astrolabeCloudsBeginnerShape)
{
   className = "clouds";
   category = "Misc";
   shapeFile = "~/data/shapes/astrolabe/astrolabe_clouds_beginner.dts";
   scopeAlways = true;
   astrolabe = true;
   astrolabePrime = true;
};

datablock StaticShapeData(astrolabeCloudsIntermediateShape)
{
   className = "clouds";
   category = "Misc";
   shapeFile = "~/data/shapes/astrolabe/astrolabe_clouds_intermediate.dts";
   scopeAlways = true;
   astrolabe = true;
   astrolabePrime = true;
};

datablock StaticShapeData(astrolabeCloudsAdvancedShape)
{
   className = "clouds";
   category = "Misc";
   shapeFile = "~/data/shapes/astrolabe/astrolabe_clouds_advanced.dts";
   scopeAlways = true;
   astrolabe = true;
   astrolabePrime = true;
};

function astrolabeShape::onAdd(%this,%obj)
{
   %obj.playThread(0,"root");
}

// OTHER SHAPES

datablock StaticShapeData(checkpointShape)
{
   className = "checkpoint";
   category = "pads";
   shapeFile = "~/data/shapes/pads/checkpad.dts";
   scopeAlways = true;
};

datablock StaticShapeData(glass_3shape)
{
   className = "glass_3";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_3.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_6shape)
{
   className = "glass_6";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_6.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_9shape)
{
   className = "glass_9";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_9.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_12shape)
{
   className = "glass_12";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_12.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_15shape)
{
   className = "glass_15";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_15.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_18shape)
{
   className = "glass_18";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_18.dts";
   scopeAlways = true;
   glass = true;
};


datablock StaticShapeData(glass_flat)
{
   className = "glass_flat";
   category = "structures";
   shapeFile = "~/data/shapes/structures/glass_flat.dts";
   scopeAlways = true;
   glass = true;
};
