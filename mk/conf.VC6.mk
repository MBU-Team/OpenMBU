

CFLAGS    = $(CFLAGS.GENERAL) $(CFLAGS.$(BUILD))
LFLAGS    = $(LFLAGS.GENERAL) $(LFLAGS.$(BUILD))
LINK.LIBS = $(LINK.LIBS.GENERAL) $(LINK.LIBS.$(BUILD))


define DO.COMPILE.C 
	@-$(COMPILER.c) $(CFLAGS) -I. $(PATH.H.SYS) /c /FD /Tc$(<) /Fo$(dir $@)
endef

define DO.COMPILE.CC 
	@-$(COMPILER.cc) $(CFLAGS) -I. $(PATH.H.SYS) /c /FD /Tp$(<) /Fo$(dir $@)
endef

define DO.COMPILE.ASM
	@echo Compiling $<
	@$(COMPILER.asm) $(ASMFLAGS) $(<) -o $@
endef

define DO.COMPILE.RC
	# Assumes existance of MSVC environment variable set to the root of the 
	# VC++ install directory typically C:\Program Files\Microsoft Visual Studio\VC98
	@echo Compiling $<
	@$(COMPILER.rc) $(RCFLAGS) /i'$(MSVC)\mfc\include' /fo$@ $(<)
endef


define DO.LINK.CONSOLE.EXE
	@echo Linking $@ 
	@$(LINK) $(LFLAGS) $(LFLAGS.EXE.$(BUILD)) /SUBSYSTEM:CONSOLE /OUT:$@ \
	$(patsubst %, /LIBPATH:%, $(LIB.PATH)) \
	$(LINK.SOURCES) \
	$(LINK.LIBS) 
endef

define DO.LINK.GUI.EXE
	@echo Linking $@
	@$(LINK) $(LFLAGS) $(LFLAGS.EXE.$(BUILD)) /SUBSYSTEM:WINDOWS /OUT:$@ \
	$(patsubst %, /LIBPATH:%, $(LIB.PATH)) \
	$(LINK.SOURCES) \
	$(LINK.LIBS) 
endef

define DO.LINK.LIB
	@echo Linking $@ 
	@$(LINK) \
	-LIB $(LFLAGS) /OUT:$@ \
	$(LINK.SOURCES) 
endef

define DO.LINK.DLL
	@echo Linking $@ 
	@$(LINK) \
	$(LINK.LIBS) \
	$(patsubst %, /LIBPATH:%, $(LIB.PATH)) \
	$(LFLAGS) /DLL /SUBSYSTEM:CONSOLE /OUT:$@ \
	$(LINK.SOURCES) \
	$(LINK.RESOURCES) \
	$(patsubst %, /DEF:%, $(LINK.DEFS)) 
endef


