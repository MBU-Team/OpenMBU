

CFLAGS    = $(CFLAGS.GENERAL) $(CFLAGS.$(BUILD))
LFLAGS    = $(LFLAGS.GENERAL) $(LFLAGS.$(BUILD))
LINK.LIBS = $(LINK.LIBS.GENERAL) $(LINK.LIBS.$(BUILD))


define DO.COMPILE.C 
	@echo "--> Compiling $(<)"
	@$(COMPILER.c) $(CFLAGS) -I. $(PATH.H.SYS) -c -o $(@) $(<)
endef

define DO.COMPILE.CC 
	@echo "--> Compiling $(<)"
	@$(COMPILER.cc) $(CFLAGS) -I. $(PATH.H.SYS) -c -o $(@) $(<)
endef

define DO.COMPILE.ASM
	@echo "--> Assembling $(<)"
	@$(COMPILER.asm) $(ASMFLAGS) -o $@ $(<)
endef

define DO.LINK.CONSOLE.EXE
	@echo "--> Linking $@"
	@$(COMPILER.cc) -o $@ $(LFLAGS) $(LINK.SOURCES) $(LINK.LIBS)
endef

define DO.LINK.LIB
	@echo "Creating library $@"
	@$(LINK) -cr $@ $(LINK.SOURCES)
endef
