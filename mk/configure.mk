
COMPILER_OPTIONS=CW6 VC6 GCC2 GCC3
BUILD_OPTIONS=DEBUG RELEASE
OS_OPTIONS=WIN32 BEOS LINUX OpenBSD FreeBSD Solaris

OS=
DIR.OBJ=out

-include mk/conf.mk
CONFIG_STATUS=VALID 

ifeq ($(findstring $(OS), $(OS_OPTIONS)),)
	targets += badOS
CONFIG_STATUS=INVALID
endif

ifeq ($(findstring $(COMPILER), $(COMPILER_OPTIONS)),)
	targets +=badCompiler
CONFIG_STATUS=INVALID
endif

ifeq ($(findstring $(BUILD), $(BUILD_OPTIONS)),)
	targets += badBuild
CONFIG_STATUS=INVALID
endif

default: $(targets) print save

badOS:
	@echo ERROR: OS variable not set or is an illegal value

badCompiler:
	@echo ERROR: COMPILER variable not set or is an illegal value

badBuild:
	@echo ERROR: BUILD variable not set or is an illegal value

print:
	@echo  
	@echo "Current Configuration: this config is $(CONFIG_STATUS)"
	@echo "         OS: $(OS)"
	@echo "   COMPILER: $(COMPILER)"
	@echo "      BUILD: $(BUILD)"   
	@echo "    DIR.OBJ: $(DIR.OBJ)"   
	@echo  
	@echo "To change the current configuration type:"
	@echo  
	@echo "make -f mk/configure.mk {arguments, ...}"
	@echo  
	@echo "required arguments:"
	@echo "  OS={$(OS_OPTIONS)}"
	@echo "  COMPILER={$(COMPILER_OPTIONS)}"
	@echo "  BUILD={$(BUILD_OPTIONS)}"
	@echo  
	@echo "optional arguments:"
	@echo "  DIR.OBJ={path to store intermediate obj files}"
	@echo  
	@echo "Note: all arguments are case sensitive." 
	@echo  

save:
	@echo OS=$(OS) > mk/conf.mk
	@echo COMPILER=$(COMPILER) >> mk/conf.mk
	@echo BUILD=$(BUILD) >> mk/conf.mk

	@echo ifeq \"$(BUILD)\" \"DEBUG\" >> mk/conf.mk
	@echo BUILD_SUFFIX:=_DEBUG >> mk/conf.mk
	@echo else >> mk/conf.mk
	@echo BUILD_SUFFIX:= >> mk/conf.mk
	@echo endif >> mk/conf.mk

	@echo CONFIG_STATUS=$(CONFIG_STATUS) >> mk/conf.mk
	@echo DIR.OBJ=$(DIR.OBJ) >> mk/conf.mk

	@echo ifndef COMPILER_OPTIONS >> mk/conf.mk
	@echo DIR.OBJ:=$(DIR.OBJ).$(COMPILER).$(BUILD) >> mk/conf.mk
	@echo endif >> mk/conf.mk
