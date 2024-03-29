new GuiControlProfile (GuiChatDefaultProfile : GuiDefaultProfile)
{
   border = 0;
   bordercolor = "255 0 64";	
   
   opaque = 1;
   fillColor = "0 0 0 150";
};

new GuiControlProfile (ChatTextProfile)
{
	textoffset = "5 5";
	fontColor = "0 0 0";
	fontColorHL = "130 130 130";
	fontColorNA = "255 0 0";
	fontColors[0] = "255 255 255";
	fontColors[1] = "255 255 255";  
	fontColors[2] = "255 255 255"; 
	fontColors[3] = "255 255 255"; 
	fontColors[4] = "255 180 0"; 
	fontColors[5] = "255 255 255"; 
	fontColors[6] = "255 255 255"; 
	fontColors[7] = "0 0 0"; 

	shadow = 0;
	doFontOutline = true;
	fontOutlineColor = "0 0 0 30";
   border = 0;
   borderColor = "0 0 0";

	fontType = "Arial Bold";
	fontSize = 23;
	
   opaque = 0;
   fillColor = "0 0 0 80";
};

new GuiControlProfile (ChatTextShadowProfile : ChatTextProfile)
{
	fontColors[0] = "0 0 0 200";
	fontColors[1] = "0 0 0 200";  
	fontColors[2] = "0 0 0 200"; 
	fontColors[3] = "0 0 0 200"; 
	fontColors[4] = "0 0 0 200"; 
	fontColors[5] = "0 0 0 200"; 
	fontColors[6] = "0 0 0 200"; 
};

new GuiControlProfile (WhoTalkProfile)
{
	textoffset = "3 0";
	fontColor = "0 0 0";
	fontColorHL = "130 130 130";
	fontColorNA = "255 0 0";
	fontColors[0] = "255 180 0";
	fontColors[1] = "255 180 0";  
	fontColors[2] = "255 180 0"; 
	fontColors[3] = "255 180 0"; 
	fontColors[4] = "255 180 0"; 
	fontColors[5] = "255 180 0"; 
	fontColors[6] = "255 0 0"; 
	fontColors[7] = "0 0 0"; 

	shadow = 1;
	doFontOutline = true;
	fontOutlineColor = "0 0 0 30";
   border = 0;

	fontType = "Arial Bold";
	fontSize = 23;
};

new GuiControlProfile (ChatTextEditProfile : ChatTextProfile)
{
	textoffset = "3 0";
	fontColor = "0 0 0";
	fontColorHL = "130 130 130";
	fontColorNA = "255 0 0";
	fontColors[0] = "255 255 255";
	fontColors[1] = "64 64 255";  
	fontColors[2] = "0 255 0"; 
	fontColors[3] = "255 255 0"; 
	fontColors[4] = "0 255 255"; 
	fontColors[5] = "255 0 255"; 
	fontColors[6] = "255 255 255"; 
	fontColors[7] = "0 0 0"; 

	shadow = 1;
	doFontOutline = true;
	fontOutlineColor = "0 0 0 30";
   border = 0;
   opaque = 0;

	fontType = "Arial Bold";
	fontSize = 16;

   tab = true;
   canKeyFocus = true;
};
new GuiControlProfile (ChatChannelProfile : ChatTextProfile)
{
	textoffset = "3 0";
	fontColor = "0 0 0";
	fontColorHL = "130 130 130";
	fontColorNA = "255 0 0";
	fontColors[0] = "255 0 64";
	fontColors[1] = "64 64 255";  
	fontColors[2] = "0 255 0"; 
	fontColors[3] = "255 180 0"; 
	fontColors[4] = "0 255 255"; 
	fontColors[5] = "255 0 255"; 
	fontColors[6] = "255 255 255"; 
	fontColors[7] = "0 0 0"; 

	shadow = 1;
	doFontOutline = true;
	fontOutlineColor = "0 0 0 30";
    border = 0;
   opaque = 0;

	fontType = "Arial Bold";
	fontSize = 16;
};
