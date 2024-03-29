//------------------------------------------------------------------------------
// Localization support scripts
//------------------------------------------------------------------------------
//exec( "./controlerIcons.cs" );

function getLanguageList()
{
   // TODO: Unicode characters don't work, fix them and re-enable these languages
   return "english" TAB "french" TAB "german" TAB "italian" TAB "spanish" TAB "polish";// TAB "chinese" TAB "japanese" TAB "korean";
}

function getLanguageDisplayList()
{
   // TODO: Unicode characters don't work, fix them and re-enable these languages
   return $Text::LangEnglish TAB $Text::LangFrench TAB $Text::LangGerman TAB $Text::LangItalian TAB $Text::LangSpanish TAB $Text::LangPolish;// TAB $Text::LangChinese TAB $Text::LangJapanese TAB $Text::LangKorean;
}

function isEnglish()
{
	return (getLanguage() $= "english");	
}

function isJapanese()
{
	return (getLanguage() $= "japanese");	
}

function avar(%str,%r1,%r2)
{
   return strreplace(strreplace(%str,"%1",%r1),"%2",%r2);
}

function loadLocaleInf(%filename)
{
   echo("Loading Locale Inf file:" SPC %filename);
   %fo = new FileObject();
   
   if (!%fo.openForRead(%filename))
   {
      error("   File not found:" SPC %filename);
      %fo.delete();
      return;
   }
   
   //--------
   // Dupe line check -pw
   %globalVarIds[0] = "";
   %lineIdx = 0;
   //--------
   
   while(!%fo.isEOF())
   {
      %line = %fo.readLine();
      %line = trim(%line);
      
      // if the line is empty, ignore
      if (%line $= "")
      {
         %lineIdx++; // Dupe check -pw
         continue;
      }
         
      // if its a section header, ignore
      // This was changed for UTF8 lead-in characters
      //if (startswith(%line, "[") && endswith(%line, "]"))
      if (endswith(%line, "]"))
      {
         %lineIdx++; // Dupe check -pw
         continue;
      }
      
      // if its a comment, ignore
      if (startswith(%line, ";"))
      {
         %lineIdx++; // Dupe check -pw
         continue;
      }
         
      // chop off anything after the last semicolon (treat it as a comment)
      %semiPos = strrchrpos(%line, ";");
      
      if (%semiPos != -1)
         %line = getSubStr(%line, 0, %semiPos + 1); // + 1 includes the semicolon
      
      // it should now be a valid "$var = value" torquescript string...but we'll give them some slack if they
      // are missing a semicolon
      if (!endswith(%line, ";"))
         %line = %line @ ";";
      
      //--------
      // Check to see if this has duplicate id -pw
      if( isPCBuild() || !isRelease() )
      {        
         %eqPos = strrchrpos( %line, "=" );
         if( %eqPos != -1 )
         {
            %varId = getSubStr(%line, 0, %eqPos);
            %varId = trim( %varId );
         }
         %globalVarIds[%lineIdx] = %varId;
         for( %i = 0; %i < %lineIdx; %i++ )
         {
            if( %globalVarIds[%i] $= %varId )
            {
               %msg = "ERROR DUPLICATE LOC VAR (" @ %i + 1 @ "): '" @ %globalVarIds[%i] @ "' and (" @ %lineIdx + 1 @ "): '" @ %varId @ "'";
               $dupLocGlobals = $dupLocGlobals @ %msg @ "\n";
               error(%msg);
            }
         }
         %lineIdx++;
      }
      //--------
      
      // replace "environment" Variables
      %line = strreplace(%line, "$(LANGUAGE)", getLanguage());
      
      // eval it
      //error("eval:" SPC %line);
      $ScriptError = "";
      %result = eval(%line);
      if ($ScriptError !$= "")
      {
         $LocaleErrors = $LocaleErrors @ %line @ "\r\n";
         error("   Error In Locale File:" SPC %line);
         //outputdebugline("   Script Error:" SPC %line);
      }
   }
   
   %fo.delete();
}

// this function is used for hacking the guis
function isJapanese()
{
   return getLanguage() $= "japanese";
}

function isKorean()
{
   return getLanguage() $= "korean";
}

outputdebugline( "WARNING: LOCALIZATION DEBUG BUILD, DO NOT SHIP" );

loadLocaleInf("common/local/languageStrings.inf"); 

function setLanguage(%lang)
{
   echo("Setting Language to " @ %lang @ "...");
      
   loadLocaleInf("common/local/" @ %lang @ "Strings.inf"); 
   loadLocaleInf("common/local/" @ %lang @ "Strings_U1.inf");
   
   $pref::Language = %lang;
}

function initLanguage()
{
   echo("Initializing Language...");   
   
   // Always load english strings first so that if a localization string is missing it isn't blank
   loadLocaleInf("common/local/englishStrings.inf"); 
   
   %language = getLanguage();
   if( $platform $= "windows" && $locLanguage !$= "" )
   {
      %language = $locLanguage;
      // redefine getLanguage function
      %str = "function getLanguage() { return \"" @ $locLanguage @ "\"; }";
      eval(%str);
   }

   if(%language $= "portuguese")
      %language = "english";
      
   // TODO: Unicode characters don't work, fix them and re-enable these languages
   if(%language $= "chinese" || %language $= "japanese" || %language $= "korean")
      %language = "english";
      
   setLanguage(%language);
   
   if (isJapanese())
      $GFXDevice::FontDrawYOffset = 2;
      
   // it may be safe to leave this enabled in all cases, but so that we don't rock the boat...
   $GFXDevice::FontAllowNoHangulWrap = isKorean(); 
      
   if ($Test::ForceFontDrawYOffset !$= "")
   {
      echo("Applying forced font y offset:" SPC $Test::ForceFontDrawYOffset);
      $GFXDevice::FontDrawYOffset = $Test::ForceFontDrawYOffset;
   }
      
   if ($Test::ForceAllowNoHangulWrap !$= "")
   {
      echo("Applying forced nohangulwrap:" SPC $Test::ForceAllowNoHangulWrap);
      $GFXDevice::FontAllowNoHangulWrap = $Test::ForceAllowNoHangulWrap;
   }

   echo("FontDrawYOffset:" SPC $GFXDevice::FontDrawYOffset);
   echo("FontAllowNoHangulWrap:" SPC $GFXDevice::FontAllowNoHangulWrap);
}
