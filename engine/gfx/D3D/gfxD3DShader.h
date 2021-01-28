//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3DSHADER_H_
#define _GFXD3DSHADER_H_

#include "gfx/gfxShader.h"

//**************************************************************************
// Shader Manager
//**************************************************************************
class GFXD3DShader : public GFXShader
{
   //--------------------------------------------------------------
   // Data
   //--------------------------------------------------------------
   LPDIRECT3DDEVICE9 mD3DDevice;


   //--------------------------------------------------------------
   // Procedures
   //--------------------------------------------------------------
   void initVertShader( char *vertFile, char *vertTarget );
   void initPixShader( char *vertFile, char *vertTarget );

public:
   IDirect3DVertexShader9 * vertShader;
   IDirect3DPixelShader9 * pixShader;

   GFXD3DShader();
   ~GFXD3DShader();
   
   bool init( char *vertFile, char *pixFile, F32 pixVersion );
   virtual void process();



};


#endif

