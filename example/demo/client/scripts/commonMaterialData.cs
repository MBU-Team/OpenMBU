//-----------------------------------------------------------------------------
// Anim flag settings - must match material.h
// These cannot be enumed through script becuase it cannot
// handle the "|" operation for combining them together
// ie. Scroll | Wave does not work.
//-----------------------------------------------------------------------------
$scroll = 1;
$rotate = 2;
$wave   = 4;
$scale  = 8;
$sequence = 16;

//*****************************************************************************
// Data
//*****************************************************************************
new CubemapData( Lobby )
{
   cubeFace[0] = "~/data/textures/lobbyxpos2";
   cubeFace[1] = "~/data/textures/lobbyxneg2";
   cubeFace[2] = "~/data/textures/lobbyzneg2";
   cubeFace[3] = "~/data/textures/lobbyzpos2";
   cubeFace[4] = "~/data/textures/lobbyypos2";
   cubeFace[5] = "~/data/textures/lobbyyneg2";
};

new CubemapData( Chrome )
{
   cubeFace[0] = "~/data/textures/chromexpos2";
   cubeFace[1] = "~/data/textures/chromexneg2";
   cubeFace[2] = "~/data/textures/chromezneg2";
   cubeFace[3] = "~/data/textures/chromezpos2";
   cubeFace[4] = "~/data/textures/chromeypos2";
   cubeFace[5] = "~/data/textures/chromeyneg2";
};

new CubemapData( WChrome )
{
   cubeFace[0] = "~/data/textures/wchromexpos";
   cubeFace[1] = "~/data/textures/wchromexneg";
   cubeFace[2] = "~/data/textures/wchromezneg";
   cubeFace[3] = "~/data/textures/wchromezpos";
   cubeFace[4] = "~/data/textures/wchromeypos";
   cubeFace[5] = "~/data/textures/wchromeyneg";
};

