$THIS_VERSION = "1.3";
$SERVER_VERSION = "1.3";
$BUILD_VERSION = 1;
$VERSION_MESSAGE = "";

//-------------------------------------
function startVersionCheck()
{
   if($pref::checkMOTDAndVersion)
   {
      new HTTPObject ( Version );
      Version.get("www.garagegames.com:80", "/marbleblast/version.html");
      echo("Checking Server Version");
   }
}

//-------------------------------------
function Version::onLine(%this, %line)
{
   %cmd  = firstWord(%line);
   %rest = restWords(%line);
   if (%cmd $= "VERSION")
      %this.handleVersion(%rest);
}

//-------------------------------------
function Version::handleVersion(%this, %line)
{
   $SERVER_VERSION  = firstWord(%line); 
   $VERSION_MESSAGE = restWords(%line);
   echo("Server Version = " @ $SERVER_VERSION);
   echo("This Version = " @ $THIS_VERSION);
   %this.check();
}

//-------------------------------------
function Version::check()
{
   // only pop-up the notice on the "MainMenuGui" which calls this onWake 
   // just in case we missed it.
   $content = canvas.getContent().getName();
   if (($SERVER_VERSION > $THIS_VERSION) &&
       ($content $= "MainMenuGui") && !$VersionCheckDoneOnce)
   {
      $VersionCheckDoneOnce = true;
      //StopDemoTimer();
      MOTDGuiText.setText("New Version Available!\n" @ $VERSION_MESSAGE);
      Canvas.pushDialog(MOTDGui);
   }
}

