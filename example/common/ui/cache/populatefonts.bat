@echo off
rem   Please leave these lines alone, see configuration settings below for information on how to 
rem   use this program
set STRING_FILE=
set CHAR_RANGE=

rem  --------------------------------------------------------
rem  Configuration Settings
rem  --------------------------------------------------------

rem   Specifies the source windows font that will be used.
set ARIAL_SOURCE_FONT="Arial Bold"
set COLISEUM_SOURCE_FONT="ColiseumRR Medium"

rem   If specified, use this file as a list of input strings to generate font characters from.
rem   If a range is specified it takes precidence over this.
rem set STRING_FILE=mystrings.txt

rem   If specified, generate characters from this range instead of using a string file
rem   The range is inclusive.
rem set CHAR_RANGE=65274 65276


rem  --------------------------------------------------------
rem  Main program
rem  --------------------------------------------------------

if NOT "%CHAR_RANGE%" == "" goto haverange
if NOT "%STRING_FILE%" == "" goto havefile
goto usage

:haverange
set FONT_OPTION=-r
set FONT_PARAMETERS=%CHAR_RANGE%
goto makefonts

:havefile
set FONT_OPTION=-s
set FONT_PARAMETERS=%STRING_FILE%
goto makefonts

:makefonts
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 18 "Arial Bold 18 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 20 "Arial Bold 20 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 22 "Arial Bold 22 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 24 "Arial Bold 24 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 30 "Arial Bold 30 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 32 "Arial Bold 32 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %ARIAL_SOURCE_FONT% 36 "Arial Bold 36 (ansi).uft" %FONT_PARAMETERS%
fonttool %FONT_OPTION% %COLISEUM_SOURCE_FONT% 48 "ColiseumRR Medium 48 (ansi).uft" %FONT_PARAMETERS%
goto end

:usage
echo You must specify a range or a file but not both (please read ReadMe.doc)

:end
pause
