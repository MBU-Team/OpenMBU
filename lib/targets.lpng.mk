LPNG.SOURCE=\
	lpng/png.c \
	lpng/pngerror.c \
	lpng/pnggccrd.c \
	lpng/pngget.c \
	lpng/pngmem.c \
	lpng/pngpread.c \
	lpng/pngread.c \
	lpng/pngrio.c \
	lpng/pngrtran.c \
	lpng/pngrutil.c \
	lpng/pngset.c \
	lpng/pngtrans.c \
	lpng/pngvcrd.c \
	lpng/pngwio.c \
	lpng/pngwrite.c \
	lpng/pngwtran.c \
	lpng/pngwutil.c

LPNG.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(LPNG.SOURCE:.c=$O))
SOURCE.ALL += $(LPNG.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.lpng.mk

$(DIR.OBJ)/lpng$(EXT.LIB): CFLAGS+=-Ilpng -Izlib -DPNG_NO_READ_tIME -DPNG_NO_WRITE_tIME
$(DIR.OBJ)/lpng$(EXT.LIB): $(DIR.LIST) $(LPNG.SOURCE.OBJ)
	$(DO.LINK.LIB)

lpngclean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif











