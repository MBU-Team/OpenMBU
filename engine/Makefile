
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

include targets.torque.mk

include ../mk/conf.common.mk


#default:
#	echo default.

ifneq ($(MAKECMDGOALS),clean)
-include $(addprefix $(DIR.OBJ)/, $(addsuffix $(EXT.DEP), $(basename $(filter %.cc %.c,$(SOURCE.ALL)))))
endif

