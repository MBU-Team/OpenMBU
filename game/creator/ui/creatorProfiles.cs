//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

if(!isObject(EditorToolButtonProfile)) new GuiControlProfile (EditorToolButtonProfile)
{
   opaque = true;
   border = 2;
};

if(!isObject(EditorTextProfile)) new GuiControlProfile (EditorTextProfile)
{
   fontType = "Arial Bold";
   fontColor = "0 0 0";
   autoSizeWidth = true;
   autoSizeHeight = true;
};

if(!isObject(EditorTextProfileWhite)) new GuiControlProfile (EditorTextProfileWhite)
{
   fontType = "Arial Bold";
   fontColor = "255 255 255";
   autoSizeWidth = true;
   autoSizeHeight = true;
};

if(!isObject(MissionEditorProfile)) new GuiControlProfile (MissionEditorProfile)
{
   canKeyFocus = true;
};

if(!isObject(EditorScrollProfile)) new GuiControlProfile (EditorScrollProfile)
{
   opaque = true;
   fillColor = "192 192 192 192";
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   bitmap = "common/ui/darkScroll";
   hasBitmapArray = true;
};

if(!isObject(GuiEditorClassProfile)) new GuiControlProfile (GuiEditorClassProfile)
{
   opaque = true;
   fillColor = "232 232 232";
   border = true;
   borderColor   = "0 0 0";
   borderColorHL = "127 127 127";
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fixedExtent = true;
   justify = "center";
   bitmap = ($platform $= "macos") ? "common/ui/osxScroll" : "common/ui/darkScroll";
   hasBitmapArray = true;
};
