//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//--------------------------------------------------------------------------

$Gui::fontCacheDirectory = expandFilename("./cache");

// Check for a localized font cache
error("@@@ Checking for localized font cache dir:" SPC $Gui::fontCacheDirectory @ "/" @ getLanguage());
if( IsDirectory( $Gui::fontCacheDirectory @ "/" @ getLanguage() ) )
{
   error( "@@@ Using localized font cache for " @ getLanguage() @ " language." );
   $Gui::fontCacheDirectory = $Gui::fontCacheDirectory @ "/" @ getLanguage();
}
else
   error( "@@@ Using default font cache, since no font cache for " @ getLanguage() @ " exists." );

$Gui::clipboardFile      = expandFilename("./cache/clipboard.gui");

// GuiDefaultProfile is a special case, all other profiles are initialized
// to the contents of this profile first then the profile specific
// overrides are assigned.

if(!isObject(GuiDefaultProfile)) new GuiControlProfile (GuiDefaultProfile)
{
   tab = false;
   canKeyFocus = false;
   hasBitmapArray = false;
   mouseOverSelected = false;

   // fill color
   opaque = false;
   fillColor = ($platform $= "macos") ? "211 211 211" : "192 192 192";
   fillColorHL = ($platform $= "macos") ? "244 244 244" : "220 220 220";
   fillColorNA = ($platform $= "macos") ? "244 244 244" : "220 220 220";

   // border color
   border = false;
   borderColor   = "0 0 0";
   borderColorHL = "128 128 128";
   borderColorNA = "64 64 64";
   
   bevelColorHL = "255 255 255";
   bevelColorLL = "0 0 0";

   // font
   fontType = "Arial";
   fontSize = 14;
   fontType2 = "Arial";
   fontSize2 = 14;
   
   // Changed for loc debug builds
   fontCharset = $locCharacterSet;
   fontCharset2 = $locCharacterSet;

   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fontColorNA = "0 0 0";
   fontColorSEL= "200 200 200";

   // bitmap information
   bitmap = ($platform $= "macos") ? "./osxWindow" : "./darkWindow";
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
   
   shadow = 0;

   // sounds
   soundButtonDown = "";
   soundButtonOver = "";
   soundOptionChanged = "";
};

if(!isObject(PlayerNameProfile)) new GuiControlProfile (PlayerNameProfile )
{
   fontType = "Arial Bold";
   fontSize = 18;
};

if(!isObject(GuiInputCtrlProfile)) new GuiControlProfile( GuiInputCtrlProfile )
{
   tab = true;
   canKeyFocus = true;
};

if(!isObject(GuiDialogProfile)) new GuiControlProfile(GuiDialogProfile);


if(!isObject(GuiSolidDefaultProfile)) new GuiControlProfile (GuiSolidDefaultProfile)
{
   opaque = true;
   border = true;
};

if(!isObject(GuiWindowProfile)) new GuiControlProfile (GuiWindowProfile)
{
   opaque = true;
   border = 2;
   fillColor = ($platform $= "macos") ? "211 211 211" : "192 192 192";
   fillColorHL = ($platform $= "macos") ? "190 255 255" : "64 150 150";
   fillColorNA = ($platform $= "macos") ? "255 255 255" : "150 150 150";
   fontColor = ($platform $= "macos") ? "0 0 0" : "235 235 235";
   fontColorHL = ($platform $= "macos") ? "200 200 200" : "0 0 0";
   text = "GuiWindowCtrl test";
   bitmap = ($platform $= "macos") ? "./osxWindow" : "./darkWindow";
   textOffset = ($platform $= "macos") ? "5 5" : "6 6";
   hasBitmapArray = true;
   justify = ($platform $= "macos") ? "center" : "left";
};

if(!isObject(GuiToolWindowProfile)) new GuiControlProfile (GuiToolWindowProfile)
{
   opaque = true;
   border = 2;
   fillColor = "192 192 192";
   fillColorHL = "64 150 150";
   fillColorNA = "150 150 150";
   fontColor = "235 235 235";
   fontColorHL = "0 0 0";
   bitmap = "./torqueToolWindow";
   textOffset = "6 6";
};

if( !isObject(GuiTabBookProfile) ) new GuiControlProfile (GuiTabBookProfile)
{
   fillColor = "255 255 255";
   fillColorHL = "64 150 150";
   fillColorNA = "150 150 150";
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fontColorNA = "0 0 0";
   justify = "center";
   bitmap = "./darkTab";
   tabWidth = 64;
   tabHeight = 24;
   tabPosition = "Top";
   tabRotation = "Horizontal";
   tab = true;
   cankeyfocus = true;
};

if( !isObject(GuiTabPageProfile) ) new GuiControlProfile (GuiTabPageProfile)
{
   bitmap = "./darkTabPage";
   tab = true;
};


if(!isObject(GuiContentProfile)) new GuiControlProfile (GuiContentProfile)
{
   opaque = true;
   fillColor = "235 235 235";
};

if(!isObject(GuiModelessDialogProfile)) new GuiControlProfile("GuiModelessDialogProfile")
{
   modal = false;
};

if(!isObject(GuiButtonProfile)) new GuiControlProfile (GuiButtonProfile)
{
   opaque = true;
   border = true;
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fixedExtent = true;
   justify = "center";
   canKeyFocus = false;
};

if(!isObject(GuiBorderButtonProfile)) new GuiControlProfile (GuiBorderButtonProfile)
{
   fontColorHL = "0 0 0";
};

if(!isObject(GuiMenuBarProfile)) new GuiControlProfile (GuiMenuBarProfile)
{

   fontType = "Arial";
   fontSize = 15;
   opaque = true;
   fillColor = ($platform $= "macos") ? "211 211 211" : "192 192 192";
   fillColorHL = "0 0 96";
   border = 4;
   fontColor = "0 0 0";
   fontColorHL = "235 235 235";
   fontColorNA = "128 128 128";
   fixedExtent = true;
   justify = "center";
   canKeyFocus = false;
   mouseOverSelected = true;
   bitmap = ($platform $= "macos") ? "./osxMenu" : "./torqueMenu";
   hasBitmapArray = true;
};

if(!isObject(GuiButtonSmProfile)) new GuiControlProfile (GuiButtonSmProfile : GuiButtonProfile)
{
   fontSize = 14;
};

if(!isObject(GuiRadioProfile)) new GuiControlProfile (GuiRadioProfile)
{
   fontSize = 14;
   fillColor = "232 232 232";
   fontColorHL = "32 100 100";
   fixedExtent = true;
   bitmap = ($platform $= "macos") ? "./osxRadio" : "./torqueRadio";
   hasBitmapArray = true;
};

if(!isObject(GuiScrollProfile)) new GuiControlProfile (GuiScrollProfile)
{
   opaque = true;
   fillColor = "235 235 235";
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   bitmap = ($platform $= "macos") ? "./osxScroll" : "./darkScroll";
   hasBitmapArray = true;
};

new GuiControlProfile (GuiAutoScrollProfile)
{
   opaque = true;
   fillColor = "255 255 255";
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   bitmap = "./demoScroll";
   hasBitmapArray = true;
};

if(!isObject(VirtualScrollProfile)) new GuiControlProfile (VirtualScrollProfile : GuiScrollProfile);

if(!isObject(GuiSliderProfile)) new GuiControlProfile (GuiSliderProfile);

if(!isObject(GuiTextProfile)) new GuiControlProfile (GuiTextProfile)
{
   fontColor = "0 0 0";
   fontColorLink = "255 96 96";
   fontColorLinkHL = "0 0 255";
   autoSizeWidth = true;
   autoSizeHeight = true;
};

if(!isObject(GuiMediumTextProfile)) new GuiControlProfile (GuiMediumTextProfile : GuiTextProfile)
{
   fontSize = 24;
};

if(!isObject(GuiBigTextProfile)) new GuiControlProfile (GuiBigTextProfile : GuiTextProfile)
{
   fontSize = 36;
};

if(!isObject(GuiCenterTextProfile)) new GuiControlProfile (GuiCenterTextProfile : GuiTextProfile)
{
   justify = "center";
};

if(!isObject(GuiTextEditProfile)) new GuiControlProfile (GuiTextEditProfile)
{
   opaque = true;
   fillColor = "235 235 235";
   fillColorHL = "128 128 128";
   border = 3;
   borderThickness = 2;
   borderColor = "0 0 0";
   fontColor = "0 0 0";
   fontColorHL = "235 235 235";
   fontColorNA = "128 128 128";
   textOffset = "0 2";
   autoSizeWidth = false;
   autoSizeHeight = true;
   tab = true;
   canKeyFocus = true;
};

if(!isObject(GuiControlListPopupProfile)) new GuiControlProfile (GuiControlListPopupProfile)
{
   opaque = true;
   fillColor = "235 235 235";
   fillColorHL = "128 128 128";
   border = true;
   borderColor = "0 0 0";
   fontColor = "0 0 0";
   fontColorHL = "235 235 235";
   fontColorNA = "128 128 128";
   textOffset = "0 2";
   autoSizeWidth = false;
   autoSizeHeight = true;
   tab = true;
   canKeyFocus = true;
   bitmap = ($platform $= "macos") ? "./osxScroll" : "./darkScroll";
   hasBitmapArray = true;
};

if(!isObject(GuiTextArrayProfile)) new GuiControlProfile (GuiTextArrayProfile : GuiTextProfile)
{
   fontColorHL = "32 100 100";
   fillColorHL = "200 200 200";
};

if(!isObject(GuiTextListProfile)) new GuiControlProfile (GuiTextListProfile : GuiTextProfile) 
{
   tab = true;
   canKeyFocus = true;
};

if(!isObject(GuiBaseTreeViewProfile)) new GuiControlProfile (GuiBaseTreeViewProfile)
{
   fontSize = 13;  // dhc - trying a better fit...
   fontColor = "0 0 0";
   fontColorHL = "64 150 150";
   canKeyFocus = true;
   autoSizeHeight = true;
};

if(!isObject(GuiChatMenuTreeProfile)) new GuiControlProfile (GuiChatMenuTreeProfile : GuiBaseTreeViewProfile);

if(!isObject(GuiTreeViewProfile)) new GuiControlProfile (GuiTreeViewProfile : GuiBaseTreeViewProfile)
{
   fontColorSEL= "250 250 250"; 
   fillColorHL = "0 60 150";
   fontColorNA = "240 240 240";
   bitmap = "./shll_treeView";
};

if(!isObject(GuiDirectoryTreeProfile)) new GuiControlProfile ( GuiDirectoryTreeProfile : GuiTreeViewProfile )
{
   fontColor = "40 40 40";
   fontColorSEL= "250 250 250 175"; 
   fillColorHL = "0 60 150";
   fontColorNA = "240 240 240";
   bitmap = "./shll_treeView";
   fontType = "Arial";
   fontSize = 14;
};

if(!isObject(GuiDirectoryFileListProfile)) new GuiControlProfile ( GuiDirectoryFileListProfile )
{
fontColor = "40 40 40";
fontColorSEL= "250 250 250 175"; 
fillColorHL = "0 60 150";
fontColorNA = "240 240 240";
fontType = "Arial";
fontSize = 14;
};



if(!isObject(GuiCheckBoxProfile)) new GuiControlProfile (GuiCheckBoxProfile)
{
   opaque = false;
   fillColor = "232 232 232";
   border = false;
   borderColor = "0 0 0";
   fontSize = 14;
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fixedExtent = true;
   justify = "left";
   bitmap = ($platform $= "macos") ? "./osxCheck" : "./torqueCheck";
   hasBitmapArray = true;

};

if(!isObject(GuiPopUpMenuProfile)) new GuiControlProfile (GuiPopUpMenuProfile)
{
   opaque = true;
   mouseOverSelected = true;

   border = 4;
   borderThickness = 2;
   borderColor = "0 0 0";
   fontSize = 14;
   fontColor = "0 0 0";
   fontColorHL = "32 100 100";
   fontColorSEL = "32 100 100";
   fixedExtent = true;
   justify = "center";
   bitmap = ($platform $= "macos") ? "./osxScroll" : "./darkScroll";
   hasBitmapArray = true;
};

if(!isObject(LoadTextProfile)) new GuiControlProfile ("LoadTextProfile")
{
   fontColor = "66 219 234";
   autoSizeWidth = true;
   autoSizeHeight = true;
};


if(!isObject(GuiMLTextProfile)) new GuiControlProfile ("GuiMLTextProfile")
{
   fontColorLink = "255 96 96";
   fontColorLinkHL = "0 0 255";
};

if(!isObject(GuiMLTextNoSelectProfile)) new GuiControlProfile ("GuiMLTextNoSelectProfile")
{
   fontColorLink = "255 96 96";
   fontColorLinkHL = "0 0 255";
   modal = false;
};

if(!isObject(GuiMLTextEditProfile)) new GuiControlProfile (GuiMLTextEditProfile) 
{ 
   fontColorLink = "255 96 96"; 
   fontColorLinkHL = "0 0 255"; 

   fillColor = "235 235 235"; 
   fillColorHL = "128 128 128"; 

   fontColor = "0 0 0"; 
   fontColorHL = "235 235 235"; 
   fontColorNA = "128 128 128"; 

   autoSizeWidth = true; 
   autoSizeHeight = true; 
   tab = true; 
   canKeyFocus = true; 
}; 

if(!isObject(GuiToolTipProfile)) new GuiControlProfile (GuiToolTipProfile)
{
   tab = false;
   canKeyFocus = false;
   hasBitmapArray = false;
   mouseOverSelected = false;

   // fill color
   opaque = true;
   fillColor = "255 255 225";

   // border color
   border = true;
   borderColor   = "0 0 0";
   
   fontColor = "0 0 0";
   fontColorHL = "0 0 0";
   fontColorNA = "0 0 0";
   

   // used by guiTextControl
   modal = true;
   justify = "left";
   autoSizeWidth = false;
   autoSizeHeight = false;
   returnTab = false;
   numbersOnly = false;
   cursorColor = "0 0 0 255";

};

//--------------------------------------------------------------------------
// Console Window

if(!isObject(GuiConsoleProfile)) new GuiControlProfile ("GuiConsoleProfile")
{
   fontType = ($platform $= "macos") ? "Monaco" : "Lucida Console";
   fontSize = ($platform $= "macos") ? 13 : 12;
   fontColor = "0 0 0";
   fontColorHL = "130 130 130";
   fontColorNA = "255 0 0";
   fontColors[6] = "50 50 50";
   fontColors[7] = "50 50 0";  
   fontColors[8] = "0 0 50"; 
   fontColors[9] = "0 50 0";   
};


if(!isObject(GuiProgressProfile)) new GuiControlProfile ("GuiProgressProfile")
{
   opaque = false;
   fillColor = "44 152 162 100";
   border = true;
   borderColor   = "78 88 120";
};

if(!isObject(GuiProgressTextProfile)) new GuiControlProfile ("GuiProgressTextProfile")
{
   fontColor = "0 0 0";
   justify = "center";
};

if(!isObject(GuiBitmapBorderProfile)) new GuiControlProfile(GuiBitmapBorderProfile)
{
   bitmap = "./darkBorder";
   hasBitmapArray = true;
};

if(!isObject(GuiPaneProfile)) new GuiControlProfile(GuiPaneProfile)
{
   bitmap = "./torquePane";
   hasBitmapArray = true;
};


//////////////////////////////////////////////////////////////////////////
// Gui Inspector Profiles
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Field Profile
//////////////////////////////////////////////////////////////////////////
if(!isObject(GuiInspectorFieldProfile)) new GuiControlProfile (GuiInspectorFieldProfile)
{
   // fill color
   opaque = false;
   fillColor = "255 255 255";
   fillColorHL = "128 128 128";
   fillColorNA = "244 244 244";

   // border color
   border = false;
   borderColor   = "190 190 190";
   borderColorHL = "156 156 156";
   borderColorNA = "64 64 64";
   
   bevelColorHL = "255 255 255";
   bevelColorLL = "0 0 0";

   // font
   fontType = "Arial";
   fontSize = 14;

   fontColor = "32 32 32";
   fontColorHL = "32 100 100";
   fontColorNA = "0 0 0";

   tab = true;
   canKeyFocus = true;
   
};

if(!isObject(GuiInspectorBackgroundProfile)) new GuiControlProfile (GuiInspectorBackgroundProfile : GuiInspectorFieldProfile)
{
   border = 5;
};

if(!isObject(GuiInspectorDynamicFieldProfile)) new GuiControlProfile (GuiInspectorDynamicFieldProfile : GuiInspectorFieldProfile);

//////////////////////////////////////////////////////////////////////////
// Default Field (TextEdit) Profile
//////////////////////////////////////////////////////////////////////////
if(!isObject(GuiInspectorTextEditProfile)) new GuiControlProfile ("GuiInspectorTextEditProfile")
{
   // Transparent Background
   opaque = false;

   // No Border (Rendered by field control)
   border = false;

   tab = true;
   canKeyFocus = true;

   // font
   fontType = "Arial";
   fontSize = 14;

   fontColor = "32 32 32";
   fontColorHL = "32 100 100";
   fontColorNA = "0 0 0";

};

if(!isObject(InspectorTypeEnumProfile)) new GuiControlProfile (InspectorTypeEnumProfile : GuiInspectorFieldProfile)
{
   mouseOverSelected = true;
   bitmap = ($platform $= "macos") ? "./osxScroll" : "./darkScroll";
   hasBitmapArray = true;
   opaque=true;
   border=true;
};

if(!isObject(InspectorTypeCheckboxProfile)) new GuiControlProfile (InspectorTypeCheckboxProfile : GuiInspectorFieldProfile)
{
   bitmap = ($platform $= "macos") ? "./osxCheck" : "./torqueCheck";
   hasBitmapArray = true;
   opaque=false;
   border=false;
};

// For TypeFileName browse button
if(!isObject(GuiInspectorTypeFileNameProfile)) new GuiControlProfile (GuiInspectorTypeFileNameProfile)
{
   // Transparent Background
   opaque = false;

   // No Border (Rendered by field control)
   border = 5;

   tab = true;
   canKeyFocus = true;

   // font
   fontType = "Arial";
   fontSize = 16;
   
   // Center text
   justify = "center";

   fontColor = "32 32 32";
   fontColorHL = "32 100 100";
   fontColorNA = "0 0 0";

   fillColor = "255 255 255";
   fillColorHL = "128 128 128";
   fillColorNA = "244 244 244";

   borderColor   = "190 190 190";
   borderColorHL = "156 156 156";
   borderColorNA = "64 64 64";


};


//-------------------------------------- Cursors
//
new GuiCursor(DefaultCursor)
{
   hotSpot = "1 1";
   renderOffset = "0 0";
   bitmapName = "./CUR_3darrow";
};

new GuiCursor(LeftRightCursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0";
   bitmapName = "./CUR_leftright";
};

new GuiCursor(UpDownCursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0.5";
   bitmapName = "./CUR_updown";
};

new GuiCursor(NWSECursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0.5";
   bitmapName = "./CUR_nwse";
};

new GuiCursor(NESWCursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0.5";
   bitmapName = "./CUR_nesw";
};

new GuiCursor(MoveCursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0.5";
   bitmapName = "./CUR_move";
};

new GuiCursor(TextEditCursor)
{
   hotSpot = "1 1";
   renderOffset = "0.5 0.5";
   bitmapName = "./CUR_textedit";
};
