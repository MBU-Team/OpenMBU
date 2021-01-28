LJPEG.SOURCE=\
	ljpeg/jcapimin.c \
	ljpeg/jcapistd.c \
	ljpeg/jccoefct.c \
	ljpeg/jccolor.c \
	ljpeg/jcdctmgr.c \
	ljpeg/jchuff.c \
	ljpeg/jcinit.c \
	ljpeg/jcmainct.c \
	ljpeg/jcmarker.c \
	ljpeg/jcmaster.c \
	ljpeg/jcomapi.c \
	ljpeg/jcparam.c \
	ljpeg/jcphuff.c \
	ljpeg/jcprepct.c \
	ljpeg/jcsample.c \
	ljpeg/jctrans.c \
	ljpeg/jdapimin.c \
	ljpeg/jdapistd.c \
	ljpeg/jdatadst.c \
	ljpeg/jdatasrc.c \
	ljpeg/jdcoefct.c \
	ljpeg/jdcolor.c \
	ljpeg/jddctmgr.c \
	ljpeg/jdhuff.c \
	ljpeg/jdinput.c \
	ljpeg/jdmainct.c \
	ljpeg/jdmarker.c \
	ljpeg/jdmaster.c \
	ljpeg/jdmerge.c \
	ljpeg/jdphuff.c \
	ljpeg/jdpostct.c \
	ljpeg/jdsample.c \
	ljpeg/jdtrans.c \
	ljpeg/jerror.c \
	ljpeg/jfdctflt.c \
	ljpeg/jfdctfst.c \
	ljpeg/jfdctint.c \
	ljpeg/jidctflt.c \
	ljpeg/jidctfst.c \
	ljpeg/jidctint.c \
	ljpeg/jidctred.c \
	ljpeg/jquant1.c \
	ljpeg/jquant2.c \
	ljpeg/jutils.c \
	ljpeg/jmemmgr.c \
	ljpeg/jmemnobs.c 

#	ljpeg/jmemansi.c \


LJPEG.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(LJPEG.SOURCE:.c=$O))
SOURCE.ALL += $(LJPEG.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.ljpeg.mk

$(DIR.OBJ)/ljpeg$(EXT.LIB): CFLAGS+=-Iljpeg
$(DIR.OBJ)/ljpeg$(EXT.LIB): $(DIR.LIST) $(LJPEG.SOURCE.OBJ)
	$(DO.LINK.LIB)

ljpegclean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif
