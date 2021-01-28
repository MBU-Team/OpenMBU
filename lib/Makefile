
#--------------------------------------
# include and verify the users mk/conf.mk

-include ../mk/conf.mk

ifndef CONFIG_STATUS
doConfigure: 
	$(error Configuration file not defined.  Please run make -f mk/configure.mk)
	#@make --no-print-directory -f ../mk/configure.mk
else
ifeq ($(CONFIG_STATUS),INVALID)
doConfigure:
	$(error Invalid Configuration file.  Please run make -f mk/configure.mk)
	#@make --no-print-directory -f mk/configure.mk
else
	include ../mk/conf.$(COMPILER).$(OS).mk
	include ../mk/conf.$(COMPILER).mk
endif
endif


default: \
   $(DIR.OBJ)/ \
   $(DIR.OBJ)/lpng$(EXT.LIB) \
   $(DIR.OBJ)/zlib$(EXT.LIB) \
   $(DIR.OBJ)/ljpeg$(EXT.LIB) \
   $(DIR.OBJ)/lungif$(EXT.LIB) \
   $(DIR.OBJ)/opengl2d3d$(EXT.DLL) \
   $(DIR.OBJ)/glu2d3d$(EXT.DLL) 


# opengl2d3d currently does not compile with CodeWarrior6
# until the link errors are resolved create dummy targets
COMPILE_D3D ?= true
ifeq "$(COMPILER)" "CW6"
COMPILE_D3D = false
endif

# no need to compile D3D stuff under Linux :)
ifeq "$(OS)" "LINUX"
COMPILE_D3D = false
endif

# or FreeBSD
ifeq "$(OS)" "FreeBSD"
COMPILE_D3D = false
endif

# or OpenBSD!
ifeq "$(OS)" "OpenBSD"
COMPILE_D3D = false
endif


ifeq "$(COMPILE_D3D)" "false"
$(DIR.OBJ)/opengl2d3d$(EXT.DLL): ;
$(DIR.OBJ)/glu2d3d$(EXT.DLL): ;
else
include targets.opengl2d3d.mk
endif

include targets.lpng.mk
include targets.zlib.mk
include targets.ljpeg.mk
include targets.lungif.mk

include ../mk/conf.common.mk

TORQUEclean: ljpegclean zlibclean ljpegclean lungif
   

#default:
#	echo default.

ifneq ($(MAKECMDGOALS),clean)
-include $(addprefix $(DIR.OBJ)/, $(addsuffix $(EXT.DEP), $(basename $(filter %.cc %.c,$(SOURCE.ALL)))))
endif

