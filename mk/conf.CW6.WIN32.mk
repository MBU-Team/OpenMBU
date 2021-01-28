
O=.obj
EXT.EXE=$(BUILD_SUFFIX).exe
EXT.LIB=$(BUILD_SUFFIX).lib
EXT.DLL=$(BUILD_SUFFIX).dll
EXT.DLE=$(BUILD_SUFFIX).dle
EXT.DEP=.dep

MKDIR=mkdir $(@:/=)
RM=rm -f
RMDIR=rm -rf
CP=cp
LN=cp

COMPILER.c 		=mwcc.exe
COMPILER.cc		=mwcc.exe
COMPILER.asm	=..\bin\nasm\nasmw.exe
COMPILER.rc     =rc.exe        # codeWarrior does NOT have a commandline resource compiler, use VC++ if exists
LINK=mwld.exe

#-nodefaults
CFLAGS.GENERAL    =-msgstyle gcc -enum int -align 4 -proc 586 -inst mmx -nosyspath \
                   -warn on -warn nounusedexpr -warn nounusedarg -warn nounusedvar -warn noemptydecl \
                   -maxerrors 10 -maxwarnings 10 
CFLAGS.RELEASE    =-opt full -sym on 
CFLAGS.DEBUG      =-opt level=0 -inline off -sym on -DTORQUE_DEBUG
CFLAGS.DEBUGFAST  =-opt level=2, schedule -inline smart -sym on 

ASMFLAGS          = -f win32

LFLAGS.GENERAL    =
LFLAGS.RELEASE    =-sym codeview
LFLAGS.DEBUG      =-sym codeview

#dxguid.lib dxerr8.lib ole32.lib shell32.lib

#winmm.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib 

LINK.LIBS.GENERAL      = gdi32.lib kernel32.lib user32.lib wsock32.lib comdlg32.lib winmm.lib advapi32.lib uuid.lib vfw32.lib
LINK.LIBS.RELEASE      = mwcrtl.lib  ansiCx86.lib  ansiCPPx86.lib
LINK.LIBS.DEBUG        = mwcrtld.lib ansiCx86d.lib ansiCPPx86d.lib 

PATH.H.SYS        =
#PATH.H.SYS        =-I- -I"$(CWINSTALL)\MSL\MSL_C\MSL_Common\Include" \
#                       -I"$(CWINSTALL)\MSL\MSL_C++\MSL_Common\Include" \
#                       -I"$(CWINSTALL)\Win32-x86 Support\Headers\Win32 SDK" \
#                       -I"$(CWINSTALL)\Win32-x86 Support\Headers\precompiled headers" \
#                       -I"$(CWINSTALL)\MSL\MSL_C\MSL_Win32\Include"

