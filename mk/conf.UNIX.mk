O=.obj
# there is no "exe" for x86 UNIX, but the makefiles need some extension so use this for now.
EXT.EXE=$(BUILD_SUFFIX).bin
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
COMPILER.asm    =nasm
LINK            =ar
LINK.cc         =ld

# Noteworthy compiler options:

# -DUSE_FILE_REDIRECT: enable fileio redirection to home directory.  The 
#   exact location is ~/.PREF_DIR_ROOT/PREF_DIR_GAME_NAME.  These are set in
#   platformX86UNIX.h.  You can disable redirection at run time with the
#   -nohomedir command line paramater.  Files will then be stored in the game
#   directory, like the windows version.
# -MD: generate .d dependency makefiles.  These are necessary if you want
#   to cleanly do partial builds.  
# -w: disable all warnings
# -fno-exceptions is safe to use (if your code doesn't use exceptions), 
#    but it is disabled by default
# -fno-check-new is not tested
CFLAGS.GENERAL    = -DUSE_FILE_REDIRECT -I/usr/X11R6/include/ -MD -march=i586 \
		    -mcpu=i686 -ffast-math -pipe -fpermissive 
		    #-w -fno-exceptions -fno-check-new 
CFLAGS.RELEASE    = -O2 -finline-functions -fomit-frame-pointer 
CFLAGS.DEBUG      = -g -DTORQUE_DEBUG # -DTORQUE_DISABLE_MEMORY_MANAGER
CFLAGS.DEBUGFAST  = -O -g -finline-functions 

ASMFLAGS          = -f elf -dLINUX

LFLAGS.GENERAL    = 
LFLAGS.RELEASE    =
LFLAGS.DEBUG      =

# GLU must be statically linked, otherwise torque will crash.
LINK.LIBS.GENERAL = -Wl,-static -Wl,-lGLU -Wl,-dy -L/usr/X11R6/lib -lSDL -lpthread -ldl # -lefence

LINK.LIBS.TOOLS   = -Wl,-static -Wl,-lGLU -Wl,-dy -L/usr/X11R6/lib -lSDL -lpthread -ldl # -lefence
# -lefence is useful for finding memory corruption problems
LINK.LIBS.SERVER  = -lpthread
LINK.LIBS.RELEASE = 
LINK.LIBS.DEBUG   = 

PATH.H.SYS        =
