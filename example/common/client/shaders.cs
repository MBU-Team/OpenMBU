//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  This file contains shader data necessary for various engine utility functions
//-----------------------------------------------------------------------------


exec("common/lighting/sgShaders.cs");


new ShaderData( _DebugInterior_ )
{
   DXVertexShaderFile   = "shaders/debugInteriorsV.hlsl";
   DXPixelShaderFile    = "shaders/debugInteriorsP.hlsl";
   pixVersion = 1.1;
};
