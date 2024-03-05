//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------
// XBOX Profiles
$XBOX::MenuTextColor = "120 120 120";
$XBOX::MenuTextColorSelected = "16 16 16";
$XBOX::MenuTextColorDisabled = "180 180 180";

$XBOX::MenuAButtonColor = "44 195 16";
$XBOX::MenuBButtonColor = "237 32 36";
$XBOX::MenuYButtonColor = "227 182 18";
$XBOX::MenuXButtonColor = "0 144 255";

$XBOX::MenuFont = "Arial Bold";
$XBOX::Headerfont = "Arial Bold";
//-----------------------------------------------------------------------------
// Override base controls
GuiButtonProfile.soundButtonOver = AudioButtonOver;
GuiButtonProfile.soundButtonDown = AudioButtonDown;
GuiButtonProfile.tab = true;
GuiButtonProfile.canKeyFocus = true;
GuiDefaultProfile.soundButtonDown = AudioButtonDown;
//-----------------------------------------------------------------------------
// Chat Hud profiles

new GuiControlProfile ("ChatHudMessageProfile")
{
   fontType = "Arial Bold";
   fontSize = 30;
   fontColor = "235 235 235";      // default color (death msgs, scoring, inventory)
   fontColors[1] = "4 235 105";   // client join/drop, tournament mode
   fontColors[2] = "219 200 128"; // gameplay, admin/voting, pack/deployable
   fontColors[3] = "77 235 95";   // team chat, spam protection message, client tasks
   fontColors[4] = "40 231 235";  // global chat
   fontColors[5] = "200 200 50 200";  // used in single player game
   // WARNING! Colors 6-9 are reserved for name coloring 
   autoSizeWidth = true;
   autoSizeHeight = true;
   shadow = 1;
};

new GuiControlProfile ("ScoreCountHudProfile" )
{
   fontType = $XBOX::Headerfont;
   fontSize = 36;
};

//-----------------------------------------------------------------------------
// Center and bottom print
new GuiControlProfile ("TextTitleProfile")
{
   fontType = "ColiseumRR Medium"; //$XBOX::Headerfont;
   fontSize = 48;
   justify = left; //right;
   opaque = false;
   border = false;
   fontColor = "255 255 255 128";
   autoSizeWidth = false;
   shadow = 0;
};

new GuiControlProfile ("TextTitleRightProfile" : TextTitleProfile)
{
   justify = right;
};

new GuiControlProfile ("TextJapaneseTitleRightProfile" : TextTitleProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = 36;
   justify = right;
};

new GuiControlProfile ( "GameStartTitle" : TextTitleProfile)
{
   justify = center;	
   fontColor = "235 235 235 255";
};

new GuiControlProfile ("TextHeadingProfile")
{
   fontType = $XBOX::Headerfont;
   fontColor = "235 235 235";
   fontSize = 32;
   opaque = false;
   border = false;
   borderColor = "16 16 16";
   autoSizeWidth = false;
   shadow = 1;
};

new GuiControlProfile ("TextHeadingSmallProfile" : TextHeadingProfile)
{
   fontSize = 24;
};

new GuiControlProfile ("TextHeadingSmallNoShadeProfile" : TextHeadingSmallProfile)
{
   fontSize = 24;
   fontColor = "235 235 235 250";
   shadow = 0;
};

new GuiControlProfile ("TextHeadingSmallerProfile" : TextHeadingProfile)
{
   fontSize = 22;
};

new GuiControlProfile ("InfoTextProfile" : TextHeadingProfile)
{
   fontSize = 18;
};

new GuiControlProfile ("BigInfoTextProfile" : TextHeadingProfile)
{
   fontSize = 20;
};

new GuiControlProfile ("LargeInfoTextProfile" : TextHeadingProfile)
{
   fontSize = 24;
};

new GuiControlProfile ("TextHeadingWhiteProfile" : TextHeadingProfile)
{
   fontSize = 24;
   fontColor = "235 235 235";
};

new GuiControlProfile ("TextHeadingYellowProfile" : TextHeadingProfile)
{
   fontSize = 24;
   fontColor = "235 205 16";
};

new GuiControlProfile ("TextListProfile")
{
   fontType = $XBOX::MenuFont;
   fontSize = 22;
   justify = left;
   opaque = false;
   border = 0;
   borderColor = "16 16 16";
   fontColor = "235 235 235 230";
   autoSizeWidth = false;
   shadow = 0;
};

new GuiControlProfile ("TextListRightProfile" : TextListProfile)
{
   justify = right;
};

new GuiControlProfile(LoadingAnimationProfile)
{
   bitmap = "marble/client/ui/xbox/loadingAnimation";
};

new GuiControlProfile(TextMenuListProfile)
{
	fontType = "Arial Bold"; //$XBOX::MenuFont;
	fontSize = 30;
	shadow = 0;
	autoSizeWidth = true;
	autoSizeHeight = true;
	border = false;
	fontColors[0] = $XBOX::MenuTextColor;
	fontColors[1] = $XBOX::MenuTextColorSelected;
	fontColors[2] = $XBOX::MenuTextColorDisabled; // used to disable menu items in demo mode
   canKeyFocus = true;
   //justify = center;
   tab = true;
   textOffset = "92 0"; 
   bitmap = "marble/client/ui/xbox/cursorArray"; // bitmap 0 = unselected, bitmap 1 = selected
   //bitmap = ""; // make sure to uncomment this line if you don't want a bitmap
   soundButtonOver = "AudioButtonOver";
   iconPosition = "42 30";
   rowHeight = 62;
   
   hitArea = "20 76";
};

new GuiControlProfile(TextEditProfile)
{
    fontType = "Arial Bold";
    opaque = true;
	fillColor = "0 0 0 0";
	fillColorHL = "0 0 0 0";
	border = false;
	borderThickness = 2;
	borderColor = "0 0 0";
	fontColor = $XBOX::MenuTextColor;
	fontColorHL = $XBOX::MenuTextColorSelected;
    fontColorNA = $XBOX::MenuTextColorDisabled;
    textOffset = "0 2";
	autoSizeWidth = false;
	autoSizeHeight = true;
	tab = true;
	canKeyFocus = true;

    fontSize = 24;
};

new GuiControlProfile(TextMenuListSmallProfile : TextMenuListProfile)
{
	fontSize = 24;
   bitmap = "marble/client/ui/xbox/cursorArraySmall"; // bitmap 0 = unselected, bitmap 1 = selected
   rowHeight = 44;
   textOffset = "72 0"; 
   iconPosition = "32 20";
};

new GuiControlProfile(PopupMenuListProfile : TextMenuListProfile)
{
	fontSize = 24;
   bitmap = "marble/client/ui/xbox/cursorArraySmall"; // bitmap 0 = unselected, bitmap 1 = selected
   rowHeight = 44;
   textOffset = "72 0"; 
   iconPosition = "32 20";
};

new GuiControlProfile(TextOptionListProfile : TextMenuListProfile)
{
	fontColors[2] = "120 120 120"; // unselected option text color
	fontColors[3] = "0 51 89"; // selected option text color
   textOffset = "0 0"; 
   bitmap = "marble/client/ui/xbox/optionsCursorArray"; 
   soundOptionChanged = "AudioOptionChanged";
   iconPosition = "32 30";
};

new GuiControlProfile(TextOptionListSmallProfile : TextOptionListProfile)
{
	fontSize = 24;
   bitmap = "marble/client/ui/xbox/optionsCursorArraySmall"; // bitmap 0 = unselected, bitmap 1 = selected
   rowHeight = 44;
   iconPosition = "23 20";
};

new GuiControlProfile(TextOptionListSmallWideProfile : TextOptionListSmallProfile)
{
	fontSize = isJapanese() ? 20 : 24;
   bitmap = "marble/client/ui/xbox/optionsCursorArraySmallWide"; // bitmap 0 = unselected, bitmap 1 = selected
   iconPosition = "67 20";
};

new GuiControlProfile(TextOptionListInactiveProfile : TextOptionListProfile)
{
   tab = false; // so that we can set a first responder
   canKeyFocus = false; // ditto.  hopefully we don't have to sacrifice chickens too.
};

new GuiControlProfile(TextOptionListInactiveSmallProfile : TextOptionListSmallProfile)
{
   tab = false; // so that we can set a first responder
   canKeyFocus = false; // ditto.  hopefully we don't have to sacrifice chickens too.	
};

new GuiControlProfile(TextMenuButtonProfile)
{
	fontType = "Arial Bold"; //$XBOX::MenuFont;
	fontSize = 24;
	shadow = 0;
	autoSizeWidth = true;
	autoSizeHeight = true;
	border = false;
	fontColors[0] = $XBOX::MenuTextColor;
	fontColors[1] = $XBOX::MenuTextColorSelected;
	fontColors[2] = $XBOX::MenuTextColorDisabled; // used to disable menu items in demo mode
   canKeyFocus = true;
   justify = center;
   tab = false;//
   textOffset = "92 0"; 
   bitmap = "marble/client/ui/xbox/cursorButtonArray"; // bitmap 0 = unselected, bitmap 1 = selected
   //bitmap = ""; // make sure to uncomment this line if you don't want a bitmap
   soundButtonOver = "AudioButtonOver";
   iconPosition = "42 30";
   rowHeight = 62;
   
   hitArea = "20 76";
};

new GuiControlProfile(TextArrayProfile : TextHeadingProfile)
{
   border = false;
   shadow = 0;
   
   tab = true; // so that we can set a first responder
   canKeyFocus = true; // ditto.  hopefully we don't have to sacrifice chickens too.

   // alternating row bitmap: up to two bitmaps in array.  bitmap zero is displayed on rows 0,2,4 ...  
   // bitmap one (if present) is displayed on rows 1,3,5 ...
   bitmap = "marble/client/ui/xbox/textAlternatingBitmap"; 
   
   fontColor = "16 16 16 255";
   fontColorHL = "242 149 21 255";
	fillColorHL = "235 235 235 190";
};

new GuiControlProfile(LobbyTextArrayProfile : TextArrayProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = 24;
   textOffset = "0 2";
   rowHeight = "26";
};

new GuiControlProfile(FindGamesTextArrayProfile : TextArrayProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = 24;
   fontColors[0] = "235 235 235 255";  
   fontColors[1] = "16 16 16 255";  
   textOffset = "0 2";
   rowHeight = "26";
};


new GuiControlProfile(PlayerListGuiProfile : TextArrayProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = 24;

   tab = false; 
   canKeyFocus = false; 
   bitmap = ""; 	
   shadow = true;
   
   fontColor = "235 235 235";
};

new GuiControlProfile(ScoreboardListGuiProfile : TextArrayProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = isWidescreen()? 24 : 18;
   bitmap = ""; 	
   
   fontColor = "235 235 235";
};

new GuiControlProfile(EndGameStatsListGuiProfile : TextArrayProfile)
{
   fontType = $XBOX::Headerfont;
   fontSize = 24;

   tab = false; 
   canKeyFocus = false; 
   bitmap = ""; 	
   
   fontColor = "235 235 235";
   fontColors[0] = "235 235 235 255";  
	fontColors[1] = "141 255 141 255"; // better time
	fontColors[2] = "255 117 117 255"; // worse time
	fontColors[3] = "136 188 238 255"; // nuetral time
	fontColors[4] = "251 210 0 255"; // GOLD BABY!
};

new GuiControlProfile(bgBoxProfile)
{
   opaque = true;
   border = 2;
   fillColor = "0 0 0 102";
   fillColorHL = "0 0 0 102";
   fillColorNA = "0 0 0 102";
   fontColor = "235 235 235";
   fontColorHL = "235 235 235";
   text = "";
   bitmap = "./xbox/bgBox";
   textOffset = "6 6";
   hasBitmapArray = true;
   justify = "center";
};

new GuiControlProfile(bgBoxTopProfile)
{
   bitmap = "./xbox/bgBoxTop";
   fillColor = "0 0 0 0";
};

// xBox Button Text profiles. 
new GuiControlProfile (TextButtonProfile : TextHeadingSmallProfile)
{
   fontType = $XBOX::MenuFont;
   fontColor = "235 235 235";
   autoSizeWidth = false;
   fontSize = 30;
   shadow = true;
};

new GuiControlProfile (TextAButtonProfile : TextButtonProfile)
{
//    fontColor = $XBOX::MenuAButtonColor;
   fontSize = isWidescreen()? 30 : 24;
   justify = right;
};

new GuiControlProfile (TextBButtonProfile : TextButtonProfile)
{
//    fontColor = $XBOX::MenuBButtonColor;
   fontSize = isWidescreen()? 24 : 18;
   justify = right;
};

new GuiControlProfile (TextXButtonProfile : TextButtonProfile)
{
//    fontColor = $XBOX::MenuXButtonColor;
   fontSize = isWidescreen()? 30 : 24;
};

new GuiControlProfile (TextYButtonProfile : TextButtonProfile)
{
//    fontColor = $XBOX::MenuYButtonColor;
   fontSize = isWidescreen()? 24 : 18;
};

new GuiControlProfile (HelpScrollProfile : GuiScrollProfile)
{
   opaque = 0;
   fillColor = "0 0 0 0";
   border = 0;
   bitmap = "";
   hasBitmapArray = false;
};

new GuiControlProfile ("ControllerTextProfile" : InfoTextProfile)
{
   fontSize = isWidescreen()? 24 : 18;
   fontColor = "235 235 235 255";
   shadow = 1;
};

new GuiControlProfile ("ControllerTextRightProfile" : ControllerTextProfile)
{
   justify = right;
};

new GuiControlProfile ("AchievementDlgProfile")
{
   fontType = "Arial Bold";
   fontColor = "16 16 16";
   fontSize = 20;
   fontType2 = "Arial Bold";
   fontSize2 = 28;
   justify = center;
};

new GuiControlProfile("JoinGameInviteDlgProfile")
{
   fontType = "Arial Bold";
   fontColor = "16 16 16";
   fontSize = 24;
   justify = center;
};
