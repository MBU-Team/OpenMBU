
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

COMPILER.c 		=cl.exe
COMPILER.cc		=cl.exe
COMPILER.asm	=../bin/nasm/nasmw.exe
COMPILER.rc     =rc.exe
LINK=link.exe

CFLAGS.GENERAL    =/nologo /GR 
CFLAGS.RELEASE    =/Ox /Zi
CFLAGS.DEBUG      =/Odib0 /Zi -DTORQUE_DEBUG 
CFLAGS.DEBUGFAST  =

ASMFLAGS          = -f win32

# assumes VC++ LIBPATH variable LIB is set in environment
LFLAGS.GENERAL    = /nologo /MACHINE:I386 
LFLAGS.RELEASE    = 
LFLAGS.DEBUG      = 

LFLAGS.EXE.RELEASE= /debug 
LFLAGS.EXE.DEBUG  = /debug 


# VC++ complains about the /DEBUG flag but if not present it DOES NOT create the associated debugging files

LINK.LIBS.GENERAL      = COMCTL32.LIB COMDLG32.LIB USER32.LIB ADVAPI32.LIB GDI32.LIB WINMM.LIB WSOCK32.LIB vfw32.lib
LINK.LIBS.RELEASE      = 
LINK.LIBS.DEBUG        = 

PATH.H.SYS        =


MSVC=C:\Program Files\Microsoft Visual Studio\VC98

