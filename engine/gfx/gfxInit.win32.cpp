//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "d3d9.h"
#include "gfx/gfxInit.h"

// Platform specific API includes
#include "gfx/D3D/gfxD3DDevice.h"

//#include "gfx/gfxGLDevice.h"
#include "platformWin32/platformWin32.h"
#include "console/console.h"

Vector<GFXAdapter> GFXInit::smAdapters;

void GFXInit::enumerateAdapters() 
{
   // Enumerate D3D adapters - static function
   GFXD3DDevice::enumerateAdapters( GFXInit::smAdapters );



   //-----------------------------------------------------------------------------
   // OpenGL adapter

   // First create a window class
   //WNDCLASS windowclass;
   //dMemset( &windowclass, 0, sizeof( WNDCLASS ) );

   //windowclass.lpszClassName = "IHateW32API";
   //windowclass.style         = CS_OWNDC;
   //windowclass.lpfnWndProc   = DefWindowProc;
   //windowclass.hInstance     = winState.appInstance;

   //AssertFatal( RegisterClass( &windowclass ), "Failed to register the window class for the GL test window." );

   // Now create a window
   //HWND hwnd = CreateWindow( "Window Title", "", WS_POPUP, 0, 0, 640, 480, 
   //                          NULL, NULL, winState.appInstance, NULL );
   //AssertFatal( hwnd != NULL, "Failed to create the window for the GL test window." );

   // Create a device context
   //HDC tempDC = GetDC( hwnd );
   //AssertFatal( tempDC != NULL, "Failed to create device context" );

   // Create pixel format descriptor...
   //PIXELFORMATDESCRIPTOR pfd;
   //CreatePixelFormat( &pfd, 16, 16, 8, false ); // 16 bit color, 16 bit depth, 8 bit stensil...everyone can do this

   
   //AssertFatal( SetPixelFormat( tempDC, ChoosePixelFormat( tempDC, &pfd ), &pfd ), "I don't know who's responcible for this, but I want caught..." );

   // Create a rendering context!
   //HGLRC tempGLRC = qwglCreateContext( tempDC );
   //AssertFatal( qwglMakeCurrent( tempDC, tempGLRC ), "I want them caught and killed." );


   // Because for testing we are running two windows, all this stuff is commeted out
   // because GL is initalised on the main window already
   //GFXAdapter toAdd;
//   toAdd.type = OpenGL;
  // toAdd.index = 0;
   //dStrcpy( toAdd.name, "OpenGL");
   //glGetString = GetProcAddress( 
   //dStrcpy( toAdd.name, (const char *)glGetString( GL_RENDERER ) );

   // Clean up
   //qwglMakeCurrent( NULL, NULL );
   //qwglDeleteContext( tempGLRC );
   //ReleaseDC( hwnd, tempDC );
   //DestroyWindow( hwnd );
   //UnregisterClass( "IHateW32API", winState.appInstance );


   //smAdapters.push_back( toAdd );
}

//-----------------------------------------------------------------------------

void GFXInit::createDevice( GFXAdapter adapter ) 
{
   

   // Change to switch statement? Only 2 types right now though.
   if( adapter.type == Direct3D9 ) 
   {
      GFXDevice *temp = new GFXD3DDevice( Direct3DCreate9( D3D_SDK_VERSION ), adapter.index );

      temp->enumerateVideoModes();
      const Vector<GFXVideoMode>* const modeList = temp->getVideoModeList();
   }
   else if ( adapter.type == OpenGL )
   {     
      AssertISV(false, "OpenGL is not implemented yet.");
/*
      GFXDevice *temp = new GFXGLDevice(adapter.index);
      temp->activate();

      temp->enumerateVideoModes();
      const Vector<GFXVideoMode>* const modeList = temp->getVideoModeList();
*/
   }
   else
      AssertFatal(false, "Bad adapter type");
}

//---------------------------------------------------------------
void GFXInit::getAdapters(Vector<GFXAdapter> *adapters)
{
   adapters->clear();
   for (U32 k = 0; k < smAdapters.size(); k++)
      adapters->push_back(smAdapters[k]);
}
