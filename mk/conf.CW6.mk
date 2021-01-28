

CFLAGS    = $(CFLAGS.GENERAL) $(CFLAGS.$(BUILD))
LFLAGS    = $(LFLAGS.GENERAL) $(LFLAGS.$(BUILD))
LINK.LIBS = $(LINK.LIBS.GENERAL) $(LINK.LIBS.$(BUILD))

define DO.COMPILE.C 
	@echo Compiling $< 
	@-mwcc $(CFLAGS) -MMD -I. $(PATH.H.SYS) -c $(<) -o $(dir $@)
endef

define DO.COMPILE.CC 
	@echo Compiling $<
	@-mwcc $(CFLAGS) -MMD -I. $(PATH.H.SYS) -c $(<) -o $(dir $@)
endef

define DO.COMPILE.ASM
	@echo Compiling $<
	nasmw $(ASMFLAGS) $(<) -o $@
endef

define DO.COMPILE.RC
	# Assumes existance of MSVC environment variable set to the root of the 
	# VC++ install directory typically C:\Program Files\Microsoft Visual Studio\VC98
	@echo Compiling $<
	@$(COMPILER.rc) $(RCFLAGS) /i'$(MSVC)\mfc\include' /fo$@ $(<)
endef


define DO.LINK.CONSOLE.EXE
	@echo Linking $@ 
	@mwld $(LFLAGS) -application -subsystem console -o $@ \
	$(addprefix -L, $(LIB.PATH)) \
	$(addprefix -l, $(LINK.LIBS)) \
	$(LINK.SOURCES) 
endef

define DO.LINK.GUI.EXE
	@echo Linking $@ 
	@mwld $(LFLAGS) -application -subsystem windows -o $@ \
	$(addprefix -L, $(LIB.PATH)) \
	$(addprefix -l, $(LINK.LIBS)) \
	$(LINK.SOURCES) 
endef

define DO.LINK.LIB
	@echo Linking $@ 
	@mwld $(LFLAGS) -library -o $@ \
   $(LINK.SOURCES) 
endef

# $(DLLLFLAGS) -osym $(@, R, >.sym) -shared -export dllexport $(DLLXLIBS) 
#  -subsystem windows -o $@ -l$[s," -l",$[t,;," ", $(DLLLIBS)]] $(.SOURCES, M"*.obj")


define DO.LINK.DLL
	@echo Linking $@ 
	mwld $(LFLAGS) -shared -export dllexport \
	$(addprefix -L, $(LIB.PATH)) \
	-subsystem windows \
	-o $@ \
	$(addprefix -l, $(LINK.LIBS)) \
	$(LINK.SOURCES) \
   $(LINK.RESOURCES) \
   $(patsubst %, -f %, $(LINK.DEFS)) 
endef

