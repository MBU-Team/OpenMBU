//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sign base class
//-----------------------------------------------------------------------------

function Sign::onAdd(%this,%obj)
{
   if (%this.skin !$= "")
      %obj.setSkinName(%this.skin);
}

//-----------------------------------------------------------------------------
// Different signs...
//-----------------------------------------------------------------------------

// Unused
//datablock StaticShapeData(SignPlain)
//{
   //// Mission editor category
   //category = "Signs";
   //className = "Sign";
//
   //// Basic Item properties
   //shapeFile = "~/data/shapes/signs/plainsign.dts";
   //mass = 1;
   //friction = 1;
   //elasticity = 0.3;
//};
//
//datablock StaticShapeData(SignPlainUp: SignPlain)
//{
   //skin = "up";
//};
//
//datablock StaticShapeData(SignPlainDown: SignPlain)
//{
   //skin = "down";
//};
//
//datablock StaticShapeData(SignPlainLeft: SignPlain)
//{
   //skin = "left";
//};
//
//datablock StaticShapeData(SignPlainRight: SignPlain)
//{
   //skin = "right";
//};

//-----------------------------------------------------------------------------

datablock StaticShapeData(SignCaution)
{
   // Mission editor category
   category = "Signs";
   className = "Sign";

   // Basic Item properties
   shapeFile = "~/data/shapes/signs/cautionsign.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
};

datablock StaticShapeData(SignCautionCaution: SignCaution)
{
   skin = "caution";
};

datablock StaticShapeData(SignCautionDanger: SignCaution)
{
   skin = "danger";
};

//-----------------------------------------------------------------------------

datablock StaticShapeData(ArrowUp)
{
   // Mission editor category
   category = "Signs";
   className = "Sign";

   // Basic Item properties
   shapeFile = "~/data/shapes/signs/arrowsign_up.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
};

function ArrowUp::onAdd(%this,%obj)
{
   %obj.playThread(0,"ambient");
}

datablock StaticShapeData(ArrowSide)
{
   // Mission editor category
   category = "Signs";
   className = "Sign";

   // Basic Item properties
   shapeFile = "~/data/shapes/signs/arrowsign_side.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
};

function ArrowSide::onAdd(%this,%obj)
{
   %obj.playThread(0,"ambient");
}

datablock StaticShapeData(ArrowDown)
{
   // Mission editor category
   category = "Signs";
   className = "Sign";

   // Basic Item properties
   shapeFile = "~/data/shapes/signs/arrowsign_down.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
};

function ArrowDown::onAdd(%this,%obj)
{
   %obj.playThread(0,"ambient");
}
