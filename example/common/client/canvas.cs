//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function to construct and initialize the default canvas window
// used by the games

function initCanvas(%windowName)
{
   videoSetGammaCorrection($pref::OpenGL::gammaCorrection);
   if (!$canvasCreated) {
      quit();
      return;
   }

   // Declare default GUI Profiles.
   exec("~/ui/defaultProfiles.cs");
   
   // exec platform specific profiles
   exec("~/ui/" @ $platform @ "DefaultProfiles.cs");

   // Common GUI's
   exec("~/ui/ConsoleDlg.gui");
   exec("~/ui/LoadFileDlg.gui");
   exec("~/ui/SaveFileDlg.gui");
   exec("~/ui/MessageBoxOkDlg.gui");
   exec("~/ui/MessageBoxYesNoDlg.gui");
   exec("~/ui/MessageBoxOKCancelDlg.gui");
   exec("~/ui/MessagePopupDlg.gui");
   exec("~/ui/HelpDlg.gui");
   exec("~/ui/RecordingsDlg.gui");
   exec("~/ui/NetGraphGui.gui");
   
   // Commonly used helper scripts
   exec("./metrics.cs");
   exec("./messageBox.cs");
   exec("./screenshot.cs");
   exec("./cursor.cs");
   exec("./help.cs");
   //exec("./recordings.cs"); // use the marble blast one
   
   // Misc
   exec( "./shaders.cs" );
   exec("./materials.cs");

   // Init the audio system
   sfxStartup();
}

function resetCanvas()
{
   if (isObject(Canvas))
   {
      Canvas.repaint(); 
   }
}
