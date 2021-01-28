ZLIB.SOURCE=\
	zlib/adler32.c \
	zlib/compress.c \
	zlib/crc32.c \
	zlib/deflate.c \
	zlib/gzio.c \
	zlib/infblock.c \
	zlib/infcodes.c \
	zlib/inffast.c \
	zlib/inflate.c \
	zlib/inftrees.c \
	zlib/infutil.c \
	zlib/trees.c \
	zlib/uncompr.c \
	zlib/zutil.c

ZLIB.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(ZLIB.SOURCE:.c=$O))
SOURCE.ALL += $(ZLIB.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.zlib.mk

$(DIR.OBJ)/zlib$(EXT.LIB): CFLAGS+=-Izlib 
$(DIR.OBJ)/zlib$(EXT.LIB): $(DIR.LIST) $(ZLIB.SOURCE.OBJ)
	$(DO.LINK.LIB)

zlibclean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif
