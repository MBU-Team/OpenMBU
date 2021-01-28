//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

new GuiControlProfile (GuiDefaultProfile)
{
   tab = false;
   canKeyFocus = false;
   hasBitmapArray = false;
   mouseOverSelected = false;

   // fill color
   opaque = false;
   fillColor = "127 136 153";
   fillColorHL = "197 202 211";
   fillColorNA = "144 154 171";

   // border color
   border = false;
   borderColor   = "0 0 0"; 
   borderColorHL = "197 202 211";
   borderColorNA = "91 101 119";

   // font
   fontType = "Arial";
   fontSize = 14;
   fontCharset = ANSI;

   fontColor = "0 0 0";
   fontColorHL = "73 82 97";
   fontColorNA = "0 0 0";
   fontColorSEL= "226 237 255";

   // bitmap information
   bitmap = "./demoWindow";
   bitmapBase = "";
   textOffset = "0 0";

   // used by guiTextControl
   modal = true;
   justify = "left";
   autoSizeWidth = false;
   autoSizeHeight = false;
   returnTab = false;
   numbersOnly = false;
   cursorColor = "0 0 0 255";

   // sounds
   soundButtonDown = "";
   soundButtonOver = "";
};

new GuiControlProfile (GuiWindowProfile)
{
   opaque = true;
   border = 2;
   fillColor = "145 154 171";
   fillColorHL = "221 202 173";
   fillColorNA = "221 202 173";
   fontColor = "255 255 255";
   fontColorHL = "255 255 255";
   text = "GuiWindowCtrl test";
   bitmap = "./demoWindow";
   textOffset = "6 6";
   hasBitmapArray = true;
   justify = "center";
};

new GuiControlProfile (GuiScrollProfile)
{
   opaque = true;
   fillColor = "255 255 255";
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   bitmap = "./demoScroll";
   hasBitmapArray = true;
};

$fontColorHL = "55 64 78";
new GuiControlProfile (GuiButtonProfile)
{
   opaque = true;
   border = true;
   fontColor = "0 0 0";
   fontColorHL = $fontColorHL;
   fixedExtent = true;
   justify = "center";
	canKeyFocus = false;
};

new GuiControlProfile (GuiCheckBoxProfile)
{
   opaque = false;
   fillColor = "0 0 0";
   border = false;
   borderColor = "0 0 0";
   fontSize = 14;
   fontColor = "0 0 0";
   fontColorHL = $fontColorHL;
   fixedExtent = true;
   justify = "left";
   bitmap = "./demoCheck";
   hasBitmapArray = true;
};

new GuiControlProfile (GuiRadioProfile)
{
   fontSize = 14;
   fillColor = "0 0 0";
   fontColorHL = $fontColorHL;
   fixedExtent = true;
   bitmap = "./demoRadio";
   hasBitmapArray = true;
};

new GuiControlProfile (GuiTitleProfile)
{
   opaque = false;
   fontType = "Arial Bold";
   fontSize = 32;
   fontColor = "255 255 255";
   fontColorHL = "255 255 255";
   justify = "right";
};

new GuiControlProfile (GuiPopUpTextProfile)
{
   fontType = "Arial";
   fontSize = 14;
   fontColor = "255 255 255";
   fontColorHL = "255 255 255";
   fontColorLink = "255 96 96";
   fontColorLinkHL = "0 0 255";
};




