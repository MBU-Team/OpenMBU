MAP2DIF.SOURCE=\
	map2dif/main.cc \
	map2dif/bspNode.cc \
	map2dif/createLightmaps.cc \
	map2dif/csgBrush.cc \
	map2dif/editFloorPlanRes.cc \
	map2dif/editGeometry.cc \
	map2dif/editInteriorRes.cc \
	map2dif/entityTypes.cc \
	map2dif/exportGeometry.cc \
	map2dif/lmapPacker.cc \
	map2dif/morianUtil.cc \
	map2dif/navGraph.cc \
	map2dif/tokenizer.cc \


MAP2DIF.SOURCE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(MAP2DIF.SOURCE))) )
SOURCE.ALL += $(MAP2DIF.SOURCE)


map2dif: map2dif$(EXT.EXE)

map2dif$(EXT.EXE): CFLAGS += -I../engine -I../lib/zlib -I../lib/lungif -I../lib/lpng -I../lib/ljpeg -I- -I../lib/openal/$(OS) 

map2dif$(EXT.EXE): LIB.PATH += \
	../lib/$(DIR.OBJ) \
	../engine/$(DIR.OBJ) \

map2dif$(EXT.EXE): LINK.LIBS.GENERAL = $(PRE.ENGINE.LIB)engine$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)ljpeg$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lpng$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lungif$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)zlib$(EXT.LIB) \
	$(LINK.LIBS.TOOLS) \

map2dif$(EXT.EXE): dirlist engine$(EXT.LIB) $(MAP2DIF.SOURCE.OBJ)
	$(DO.LINK.CONSOLE.EXE)

