
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

all: max2mapExporter map2dif buildWad texture2bm8

ifeq "$(COMPILER)" "VC6"
# The MAX31 SDK requires VC++ :(
include targets.max2dtsExporter.mk
include targets.max2mapExporter.mk
else
# dummy target for other compilers
max2dtsExporter: ;

max2mapExporter: ;
endif

include targets.map2dif.mk
include targets.buildWad.mk
include targets.texture2bm8.mk

engine$(EXT.LIB):
	@make -s -C ../engine $(DIR.OBJ)/engine$(EXT.LIB)
	@make -s -C ../lib default

include ../mk/conf.common.mk


#default:
#	echo default.

ifneq ($(MAKECMDGOALS),clean)
-include $(addprefix $(DIR.OBJ)/, $(addsuffix $(DEP), $(basename $(filter %.cc %.c,$(SOURCE.ALL)))))
endif

