O=.obj
# there is no "exe" for x86 UNIX, but the makefiles need some extension so use this for now.
EXT.EXE=$(BUILD_SUFFIX).exe
EXT.LIB=$(BUILD_SUFFIX).a
PRE.LIBRARY.LIB=../lib/$(DIR.OBJ)/
PRE.ENGINE.LIB=../engine/$(DIR.OBJ)/

# we won't be needing any DLLs, but just leave the def'n in place.
EXT.DLL=$(BUILD_SUFFIX).dll
EXT.DLE=$(BUILD_SUFFIX).dle
EXT.DEP=.d

MKDIR=mkdir $(@:/=)
RM=rm -f
RMDIR=rm -rf
CP=cp
LN=cp

COMPILER.c      =gcc
COMPILER.cc     =g++
COMPILER.asm    =../bin/nasm/nasmw.exe
LINK            =ar
LINK.cc         =ld

# NOTE: if the warnings bother you, you can added a -w here to shut them off.  
# be warned that you will then get no warnings.
CFLAGS.GENERAL    = -Wno-conversion -MD -march=i586 -fvtable-thunks -fno-exceptions -fpermissive
CFLAGS.RELEASE    =-O2 -finline-functions -fomit-frame-pointer \
				  # -malign-double -ffast-math  # these haven't been tested
CFLAGS.DEBUG      =-g -DTORQUE_DEBUG

ASMFLAGS          =-f win32 

LFLAGS.GENERAL    = -m i386pe
LFLAGS.RELEASE    =
LFLAGS.DEBUG      =

LINK.LIBS.GENERAL = -lmingw32 -lcomdlg32 -luser32 -ladvapi32 -lgdi32 -lwinmm -lwsock32 -lvfw32
LINK.LIBS.SERVER  = 
LINK.LIBS.RELEASE = 
LINK.LIBS.DEBUG   = 

PATH.H.SYS        =

COMPILE_D3D=false

