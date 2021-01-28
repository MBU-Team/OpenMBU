LUNGIF.SOURCE=\
	lungif/dgif_lib.c \
	lungif/egif_lib.c \
	lungif/gif_err.c \
	lungif/gifalloc.c


LUNGIF.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(LUNGIF.SOURCE:.c=$O))
SOURCE.ALL += $(LUNGIF.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.lungif.mk

$(DIR.OBJ)/lungif$(EXT.LIB): CFLAGS+=-Ilungif
$(DIR.OBJ)/lungif$(EXT.LIB): $(DIR.LIST) $(LUNGIF.SOURCE.OBJ)
	$(DO.LINK.LIB)

lungifclean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif


