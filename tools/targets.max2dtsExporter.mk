MAX2DTS.SOURCE=\
	max2dtsExporter/main.cc             \
	max2dtsExporter/NVTriStripObjects.cc \
	max2dtsExporter/NVVertexCache.cc    \
	max2dtsExporter/exportUtil.cc       \
	max2dtsExporter/maxUtil.cc          \
	max2dtsExporter/skinHelper.cc       \
	max2dtsExporter/SceneEnum.cc        \
	max2dtsExporter/sequence.cc         \
	max2dtsExporter/ShapeMimic.cc       \
	max2dtsExporter/Stripper.cc         \
	max2dtsExporter/translucentSort.cc  \

MAX2DTS.SOURCE.RC=\
	max2dtsExporter/exporter.res

MAX2DTS.SOURCE.DEF=\
	max2dtsExporter/exporter.def


MAX2DTS.SOURCE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(MAX2DTS.SOURCE))) )
MAX2DTS.SOURCE.RES:=$(addprefix $(DIR.OBJ)/, $(addsuffix .res, $(basename $(MAX2DTS.SOURCE.RC))) )
SOURCE.ALL += $(MAX2DTS.SOURCE)

@echo ALL: $(SOURCE.ALL)

max2dtsExporter: max2dtsExporter$(EXT.DLE)

max2dtsExporter$(EXT.DLE): CFLAGS += -I../engine -I- -I../lib/maxsdk31 -DTORQUE_MAX_LIB

max2dtsExporter$(EXT.DLE): LIB.PATH += \
	../lib/maxsdk31 \
	../lib/$(DIR.OBJ) \
	../engine/$(DIR.OBJ) \

max2dtsExporter$(EXT.DLE): LINK.LIBS.GENERAL += \
	Mesh.lib        \
	Geom.lib        \
	MaxUtil.lib     \
	Gfx.lib         \
	Core.lib        \
	Bmm.lib         \
	ljpeg$(EXT.LIB) \
	lpng$(EXT.LIB) \
	lungif$(EXT.LIB) \
	zlib$(EXT.LIB) \
	engine$(EXT.LIB) \


max2dtsExporter$(EXT.DLE): dirlist engine$(EXT.LIB) $(MAX2DTS.SOURCE.OBJ) $(MAX2DTS.SOURCE.RES) $(MAX2DTS.SOURCE.DEF)
	$(DO.LINK.DLL)




