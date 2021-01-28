include ../mk/conf.UNIX.mk

# override defaults

CFLAGS.GENERAL    = -DUSE_FILE_REDIRECT -I/usr/X11R6/include/ -march=i586 \
		    -mcpu=i686 -ffast-math -fno-exceptions #-fno-check-new 

# GLU must be statically linked, otherwise torque will crash.
LINK.LIBS.GENERAL = -Wl,-static -Wl,-lGLU -Wl,-dy -L/usr/X11R6/lib -L/usr/local/lib -lSDL -pthread # -lefence

LINK.LIBS.SERVER  = -pthread