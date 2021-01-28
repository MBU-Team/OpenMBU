EXE_NAME=torqueDemo
EXE_DEDICATED_NAME=$(EXE_NAME)d
BIN_DIRECTORY=../example

SOURCE.AUDIO=\
	audio/audio.cc \
	audio/audioBuffer.cc \
	audio/audioDataBlock.cc \
	audio/audioFunctions.cc \

SOURCE.COLLISION=\
	collision/abstractPolyList.cc \
	collision/boxConvex.cc \
	collision/clippedPolyList.cc \
	collision/convex.cc \
	collision/depthSortList.cc \
	collision/earlyOutPolyList.cc \
	collision/extrudedPolyList.cc \
	collision/gjk.cc \
	collision/planeExtractor.cc \
	collision/polyhedron.cc \
	collision/polytope.cc 

SOURCE.CONSOLE=\
	console/compiledEval.cc \
	console/compiler.cc \
	console/console.cc \
	console/consoleFunctions.cc \
	console/consoleInternal.cc \
	console/consoleObject.cc \
	console/consoleTypes.cc \
	console/gram.cc \
	console/scan.cc \
	console/scriptObject.cc \
	console/simBase.cc \
	console/simDictionary.cc \
	console/simManager.cc \
	console/telnetConsole.cc \
	console/telnetDebugger.cc \
    console/consoleDoc.cc \
    console/typeValidators.cc \
    console/consoleLogger.cc

#	console/yylex.c \
#	console/yyparse.c

SOURCE.CORE=\
	core/bitTables.cc \
	core/bitRender.cc \
	core/bitStream.cc \
	core/dataChunker.cc \
	core/dnet.cc \
	core/fileObject.cc \
	core/fileStream.cc \
	core/filterStream.cc \
	core/findMatch.cc \
	core/idGenerator.cc \
	core/memStream.cc \
	core/nStream.cc \
	core/nTypes.cc \
	core/resDictionary.cc \
	core/resManager.cc \
	core/resizeStream.cc \
	core/stringTable.cc \
	core/tVector.cc \
	core/zipAggregate.cc \
	core/zipHeaders.cc \
	core/zipSubStream.cc \
	core/crc.cc

SOURCE.DGL=\
	dgl/bitmapBm8.cc \
	dgl/bitmapBmp.cc \
	dgl/bitmapGif.cc \
	dgl/bitmapJpeg.cc \
	dgl/bitmapPng.cc \
	dgl/dgl.cc \
	dgl/dglMatrix.cc \
	dgl/gBitmap.cc \
	dgl/gFont.cc \
	dgl/gPalette.cc \
	dgl/gTexManager.cc \
	dgl/lensFlare.cc \
	dgl/materialList.cc \
	dgl/materialPropertyMap.cc \
	dgl/rectClipper.cc \
	dgl/splineUtil.cc \
	dgl/stripCache.cc 

SOURCE.EDITOR=\
	editor/creator.cc \
	editor/editTSCtrl.cc \
	editor/editor.cc \
	editor/guiTerrPreviewCtrl.cc \
	editor/missionAreaEditor.cc \
	editor/terraformer.cc \
	editor/terraformerTexture.cc \
	editor/terraformerNoise.cc \
	editor/terrainActions.cc \
	editor/terrainEditor.cc \
	editor/worldEditor.cc 

SOURCE.GUI=\
   gui/guiDefaultControlRender.cc \
	gui/guiArrayCtrl.cc \
	gui/guiAviBitmapCtrl.cc \
	gui/guiBackgroundCtrl.cc \
	gui/guiBitmapCtrl.cc \
	gui/guiBitmapBorderCtrl.cc \
	gui/guiBitmapButtonCtrl.cc \
	gui/guiBubbleTextCtrl.cc \
	gui/guiButtonBaseCtrl.cc \
	gui/guiButtonCtrl.cc \
	gui/guiBorderButton.cc \
	gui/guiCanvas.cc \
	gui/guiCheckBoxCtrl.cc \
	gui/guiChunkedBitmapCtrl.cc \
	gui/guiConsole.cc \
	gui/guiConsoleEditCtrl.cc \
	gui/guiConsoleTextCtrl.cc \
	gui/guiControl.cc \
	gui/guiControlListPopup.cc \
	gui/guiDebugger.cc \
	gui/guiEditCtrl.cc \
	gui/guiFadeinBitmapCtrl.cc \
	gui/guiFilterCtrl.cc \
	gui/guiFrameCtrl.cc \
	gui/guiInputCtrl.cc \
	gui/guiInspector.cc \
	gui/guiMLTextCtrl.cc \
	gui/guiMLTextEditCtrl.cc \
	gui/guiMenuBar.cc \
	gui/guiMessageVectorCtrl.cc \
	gui/guiMouseEventCtrl.cc \
	gui/guiPopUpCtrl.cc \
	gui/guiProgressCtrl.cc \
	gui/guiRadioCtrl.cc \
	gui/guiScrollCtrl.cc \
	gui/guiSliderCtrl.cc \
	gui/guiTSControl.cc \
	gui/guiTextCtrl.cc \
	gui/guiTextEditCtrl.cc \
	gui/guiTextEditSliderCtrl.cc \
	gui/guiTextListCtrl.cc \
	gui/guiTreeViewCtrl.cc \
	gui/guiTypes.cc \
	gui/guiWindowCtrl.cc \
	gui/messageVector.cc \

#	gui/guiHelpCtrl.cc \

SOURCE.INTERIOR=\
	interior/floorPlanRes.cc \
	interior/forceField.cc \
	interior/itfdump.asm \
	interior/interior.cc \
	interior/interiorCollision.cc \
	interior/interiorDebug.cc \
	interior/interiorIO.cc \
	interior/interiorInstance.cc \
	interior/interiorLMManager.cc \
	interior/interiorLightAnim.cc \
	interior/interiorRender.cc \
	interior/interiorRes.cc \
	interior/interiorResObjects.cc \
	interior/interiorSubObject.cc \
	interior/lightUpdateGrouper.cc \
	interior/mirrorSubObject.cc \
   interior/pathedInterior.cc

SOURCE.MATH=\
	math/mBox.cc \
	math/mConsoleFunctions.cc \
	math/mMathFn.cc \
	math/mMath_C.cc \
	math/mMatrix.cc \
	math/mPlaneTransformer.cc \
	math/mQuadPatch.cc \
	math/mQuat.cc \
	math/mRandom.cc \
	math/mSolver.cc \
	math/mSplinePatch.cc \
	math/mathTypes.cc \
	math/mathUtils.cc \
	math/mMathAMD.cc \
	math/mMathAMD_ASM.asm \
	math/mMathSSE.cc \
	math/mMathSSE_ASM.asm

SOURCE.PLATFORM=\
	platform/gameInterface.cc \
	platform/platformAssert.cc \
	platform/platformCPU.cc \
	platform/platformCPUInfo.asm \
	platform/platformMemory.cc \
	platform/platformRedBook.cc \
	platform/platformVideo.cc \
	platform/profiler.cc 

SOURCE.PLATFORMPPC=\
	platformPPC/ppcAudio.cc \
	platformPPC/ppcCPUInfo.cc \
	platformPPC/ppcConsole.cc \
	platformPPC/ppcFileio.cc \
	platformPPC/ppcFont.cc \
	platformPPC/ppcGL.cc \
	platformPPC/ppcInput.cc \
	platformPPC/ppcMath.cc \
	platformPPC/ppcMemory.cc \
	platformPPC/ppcNet.cc \
	platformPPC/ppcOGLVideo.cc \
	platformPPC/ppcProcessControl.cc \
	platformPPC/ppcStrings.cc \
	platformPPC/ppcTime.cc \
	platformPPC/ppcUtils.cc \
	platformPPC/ppcWindow.cc 

SOURCE.PLATFORMWIN32=\
	platformWin32/winAsmBlit.cc \
	platformWin32/winCPUInfo.cc \
	platformWin32/winConsole.cc \
	platformWin32/winD3DVideo.cc \
	platformWin32/winDInputDevice.cc \
	platformWin32/winDirectInput.cc \
	platformWin32/winFileio.cc \
	platformWin32/winFont.cc \
	platformWin32/winGL.cc \
	platformWin32/winInput.cc \
	platformWin32/winMath.cc \
	platformWin32/winMath_ASM.cc \
	platformWin32/winMemory.cc \
	platformWin32/winMutex.cc \
	platformWin32/winNet.cc \
	platformWin32/winOGLVideo.cc \
	platformWin32/winOpenAL.cc \
	platformWin32/winProcessControl.cc \
	platformWin32/winRedbook.cc \
	platformWin32/winSemaphore.cc \
	platformWin32/winStrings.cc \
	platformWin32/winThread.cc \
	platformWin32/winTime.cc \
	platformWin32/winV2Video.cc \
	platformWin32/winWindow.cc \

SOURCE.SIM=\
	sim/actionMap.cc \
	sim/decalManager.cc \
	sim/frameAllocator.cc \
    sim/connectionStringTable.cc \
	sim/netConnection.cc \
	sim/netDownload.cc \
	sim/netEvent.cc \
    sim/netInterface.cc \
	sim/netGhost.cc \
	sim/netObject.cc \
	sim/netStringTable.cc \
	sim/pathManager.cc \
	sim/sceneObject.cc \
	sim/simPath.cc 

SOURCE.GAME=\
	game/main.cc \
	game/debris.cc \
	game/debugView.cc \
	game/gameFunctions.cc \
	game/ambientAudioManager.cc \
	game/audioEmitter.cc \
	game/banList.cc \
	game/camera.cc \
	game/cameraSpline.cc \
	game/collisionTest.cc \
	game/game.cc \
	game/gameBase.cc \
	game/gameConnection.cc \
	game/gameConnectionEvents.cc \
	game/gameConnectionMoves.cc \
	game/gameProcess.cc \
	game/gameTSCtrl.cc \
	game/guiNoMouseCtrl.cc \
	game/guiPlayerView.cc \
	game/item.cc \
	game/missionArea.cc \
	game/missionMarker.cc \
	game/pathCamera.cc \
	game/physicalZone.cc \
	game/player.cc \
	game/projectile.cc \
	game/rigid.cc \
	game/scopeAlwaysShape.cc \
	game/shadow.cc \
	game/shapeBase.cc \
	game/shapeCollision.cc \
	game/shapeImage.cc \
	game/showTSShape.cc \
	game/sphere.cc \
	game/staticShape.cc \
	game/trigger.cc \
	game/tsStatic.cc \
	game/aiConnection.cc \
	game/aiPlayer.cc \
	game/version.cc

SOURCE.GAME.NET=\
	game/net/net.cc \
	game/net/netTest.cc \
	game/net/serverQuery.cc \
	game/net/httpObject.cc \
	game/net/tcpObject.cc

SOURCE.GAME.FPS=\
	game/fps/guiCrossHairHud.cc \
	game/fps/guiShapeNameHud.cc \
	game/fps/guiHealthBarHud.cc \
	game/fps/guiClockHud.cc

SOURCE.GAME.FX=\
	game/fx/cameraFXMgr.cc \
	game/fx/explosion.cc \
	game/fx/lightning.cc \
	game/fx/particleEmitter.cc \
	game/fx/particleEngine.cc \
	game/fx/precipitation.cc \
	game/fx/splash.cc \
	game/fx/underLava.cc \
	game/fx/fxLight.cc \
	game/fx/fxSunLight.cc \
	game/fx/fxShapeReplicator.cc \
	game/fx/fxFoliageReplicator.cc


SOURCE.GAME.VEHICLES=\
	game/vehicles/vehicle.cc \
	game/vehicles/vehicleBlocker.cc \
	game/vehicles/wheeledVehicle.cc \
	game/vehicles/hoverVehicle.cc \
	game/vehicles/flyingVehicle.cc \
	game/vehicles/guiSpeedometer.cc

SOURCE.PLATFORMX86UNIX=\
	platform/platformNetAsync.cc \
	platformX86UNIX/x86UNIXAsmBlit.cc \
	platformX86UNIX/x86UNIXCPUInfo.cc \
	platformX86UNIX/x86UNIXConsole.cc \
	platformX86UNIX/x86UNIXFileio.cc \
	platformX86UNIX/x86UNIXFont.cc \
	platformX86UNIX/x86UNIXGL.cc \
	platformX86UNIX/x86UNIXInput.cc \
	platformX86UNIX/x86UNIXInputManager.cc \
	platformX86UNIX/x86UNIXIO.cc \
	platformX86UNIX/x86UNIXMath.cc \
	platformX86UNIX/x86UNIXMath_ASM.cc \
	platformX86UNIX/x86UNIXMemory.cc \
	platformX86UNIX/x86UNIXMessageBox.cc \
	platformX86UNIX/x86UNIXMutex.cc \
	platformX86UNIX/x86UNIXNet.cc \
	platformX86UNIX/x86UNIXOGLVideo.cc \
	platformX86UNIX/x86UNIXOpenAL.cc \
	platformX86UNIX/x86UNIXProcessControl.cc \
	platformX86UNIX/x86UNIXRedbook.cc \
	platformX86UNIX/x86UNIXSemaphore.cc \
	platformX86UNIX/x86UNIXStrings.cc \
	platformX86UNIX/x86UNIXThread.cc \
	platformX86UNIX/x86UNIXTime.cc \
	platformX86UNIX/x86UNIXWindow.cc \
	platformX86UNIX/x86UNIXUtils.cc 

SOURCE.PLATFORMX86UNIXDEDICATED=\
	platform/platformNetAsync.cc \
	platformX86UNIX/x86UNIXCPUInfo.cc \
	platformX86UNIX/x86UNIXConsole.cc \
	platformX86UNIX/x86UNIXDedicatedStub.cc \
	platformX86UNIX/x86UNIXFileio.cc \
	platformX86UNIX/x86UNIXIO.cc \
	platformX86UNIX/x86UNIXMath.cc \
	platformX86UNIX/x86UNIXMath_ASM.cc \
	platformX86UNIX/x86UNIXMemory.cc \
	platformX86UNIX/x86UNIXMutex.cc \
	platformX86UNIX/x86UNIXNet.cc \
	platformX86UNIX/x86UNIXProcessControl.cc \
	platformX86UNIX/x86UNIXSemaphore.cc \
	platformX86UNIX/x86UNIXStrings.cc \
	platformX86UNIX/x86UNIXThread.cc \
	platformX86UNIX/x86UNIXTime.cc \
	platformX86UNIX/x86UNIXWindow.cc \
	platformX86UNIX/x86UNIXUtils.cc 

SOURCE.PLATFORMLINUX=$(SOURCE.PLATFORMX86UNIX)
SOURCE.PLATFORMLINUXDEDICATED=$(SOURCE.PLATFORMX86UNIXDEDICATED)

SOURCE.PLATFORMOpenBSD=$(SOURCE.PLATFORMX86UNIX)
SOURCE.PLATFORMOpenBSDDEDICATED=$(SOURCE.PLATFORMX86UNIXDEDICATED)

SOURCE.PLATFORMFreeBSD=$(SOURCE.PLATFORMX86UNIX)
SOURCE.PLATFORMFreeBSDDEDICATED=$(SOURCE.PLATFORMX86UNIXDEDICATED)

SOURCE.SCENEGRAPH=\
	sceneGraph/detailManager.cc \
	sceneGraph/lightManager.cc \
	sceneGraph/sceneGraph.cc \
	sceneGraph/sceneLighting.cc \
	sceneGraph/sceneRoot.cc \
	sceneGraph/sceneState.cc \
	sceneGraph/sceneTraversal.cc \
	sceneGraph/sgUtil.cc \
	sceneGraph/shadowVolumeBSP.cc \
	sceneGraph/windingClipper.cc 

SOURCE.TERRAIN=\
	terrain/fluidQuadTree.cc \
	terrain/fluidRender.cc \
	terrain/fluidSupport.cc \
	terrain/sky.cc \
	terrain/sun.cc \
	terrain/blender.cc \
	terrain/blender_asm.asm \
	terrain/bvQuadTree.cc \
	terrain/terrCollision.cc \
	terrain/terrData.cc \
	terrain/terrLighting.cc \
	terrain/terrRender.cc \
	terrain/terrRender2.cc \
	terrain/waterBlock.cc 

SOURCE.TS=\
	ts/tsAnimate.cc \
	ts/tsCollision.cc \
	ts/tsDecal.cc \
	ts/tsDump.cc \
	ts/tsIntegerSet.cc \
	ts/tsLastDetail.cc \
	ts/tsMaterialList.cc \
	ts/tsMesh.cc \
	ts/tsPartInstance.cc \
	ts/tsShape.cc \
	ts/tsShapeAlloc.cc \
	ts/tsShapeConstruct.cc \
	ts/tsShapeInstance.cc \
	ts/tsShapeOldRead.cc \
	ts/tsSortedMesh.cc \
	ts/tsThread.cc \
	ts/tsTransform.cc 

# jmq: added the stuff after SOURCE.TS for tools build hack
SOURCE.ENGINE =\
	$(SOURCE.COLLISION) \
	$(SOURCE.CONSOLE) \
	$(SOURCE.CORE) \
	$(SOURCE.DGL) \
	$(SOURCE.INTERIOR) \
	$(SOURCE.MATH) \
	$(SOURCE.PLATFORM) \
	$(SOURCE.SCENEGRAPH) \
	$(SOURCE.SIM) \
	$(SOURCE.TERRAIN) \
	$(SOURCE.TS) \
	$(SOURCE.AUDIO) \
	$(SOURCE.GUI) \
	$(SOURCE.GAME) \
	$(SOURCE.GAME.FPS) \
	$(SOURCE.GAME.NET) \
	$(SOURCE.GAME.FX) \
	$(SOURCE.GAME.VEHICLES) 

ifeq "$(OS)" "WIN32"
SOURCE.ENGINE += $(SOURCE.PLATFORM$(OS)) 
else
SOURCE.ENGINE += $(SOURCE.PLATFORM$(OS)DEDICATED) 
endif

SOURCE.TESTAPP =\
	$(SOURCE.AUDIO) \
	$(SOURCE.COLLISION) \
	$(SOURCE.CONSOLE) \
	$(SOURCE.CORE) \
	$(SOURCE.DGL) \
	$(SOURCE.EDITOR) \
	$(SOURCE.GUI) \
	$(SOURCE.GAME) \
	$(SOURCE.GAME.FPS) \
	$(SOURCE.GAME.NET) \
	$(SOURCE.GAME.FX) \
	$(SOURCE.GAME.VEHICLES) \
	$(SOURCE.INTERIOR) \
	$(SOURCE.MATH) \
	$(SOURCE.PLATFORM) \
	$(SOURCE.SCENEGRAPH) \
	$(SOURCE.SIM) \
	$(SOURCE.TERRAIN) \
	$(SOURCE.TS) 

SOURCE.TESTAPP_CLIENT =\
	$(SOURCE.TESTAPP) \
	$(SOURCE.PLATFORM$(OS)) \

SOURCE.TESTAPP_DEDICATED =\
	$(SOURCE.TESTAPP) \
	$(SOURCE.PLATFORM$(OS)DEDICATED) \

SOURCE.TESTAPP_CLIENT.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(SOURCE.TESTAPP_CLIENT))) )
SOURCE.TESTAPP_DEDICATED.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(SOURCE.TESTAPP_DEDICATED))) )
SOURCE.ENGINE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(SOURCE.ENGINE))) )
SOURCE.ALL += $(SOURCE.TESTAPP_CLIENT)
targetsclean += torqueClean

#---------------------------------------
# Set up include variables here.
INCLUDES_BASE = -I../lib/zlib -I../lib/lungif -I../lib/lpng -I../lib/ljpeg -I../lib/directx8
INCLUDES_LINUX = $(INCLUDES_BASE) -I../lib/openal/LINUX
INCLUDES_OpenBSD = $(INCLUDES_BASE) -I../lib/openal/OpenBSD
INCLUDES_FreeBSD = $(INCLUDES_BASE) -I../lib/openal/FreeBSD
INCLUDES_WIN32 = $(INCLUDES_BASE) -I../lib/openal/win32

#----------------------------------------
# normal binary
$(EXE_NAME): $(DIR.OBJ)/$(EXE_NAME)$(EXT.EXE)

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.TESTAPP_CLIENT))))

$(DIR.LIST): targets.torque.mk

$(DIR.OBJ)/$(EXE_NAME)$(EXT.EXE): CFLAGS += $(INCLUDES_$(OS))

$(DIR.OBJ)/$(EXE_NAME)$(EXT.EXE): LIB.PATH +=../lib/$(DIR.OBJ) \

$(DIR.OBJ)/$(EXE_NAME)$(EXT.EXE): LINK.LIBS.GENERAL += \
	$(PRE.LIBRARY.LIB)ljpeg$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lpng$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lungif$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)zlib$(EXT.LIB)

$(DIR.OBJ)/$(EXE_NAME)$(EXT.EXE): $(DIR.OBJ) $(DIR.LIST) $(SOURCE.TESTAPP_CLIENT.OBJ) 
	$(DO.LINK.CONSOLE.EXE)
	$(CP) $(DIR.OBJ)/$(EXE_NAME)$(BUILD_SUFFIX).* $(BIN_DIRECTORY)


#----------------------------------------
# engine library
engine: $(DIR.OBJ)/engine$(EXT.LIB)

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ENGINE))))

$(DIR.LIST): targets.torque.mk

# unix build needs to add DEDICATED to the CFLAGS
EXTRAFLAGS=
ifneq "$(OS)" "WIN32"
EXTRAFLAGS=-DDEDICATED -DTORQUE_ENGINE
endif

$(DIR.OBJ)/engine$(EXT.LIB): CFLAGS += $(EXTRAFLAGS) -DTORQUE_MAX_LIB $(INCLUDES_$(OS))

$(DIR.OBJ)/engine$(EXT.LIB): $(DIR.OBJ) $(DIR.LIST) $(SOURCE.ENGINE.OBJ)
	$(DO.LINK.LIB)

#----------------------------------------
# dedicated server build (unix only)
dedicated: $(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(EXT.EXE)

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.TESTAPP_DEDICATED))))

$(DIR.LIST): targets.torque.mk

$(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(EXT.EXE): CFLAGS += -DDEDICATED $(INCLUDES_$(OS))

$(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(EXT.EXE): LIB.PATH +=../lib/$(DIR.OBJ) \

$(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(EXT.EXE): LINK.LIBS.GENERAL = \
	$(LINK.LIBS.SERVER) \
	$(PRE.LIBRARY.LIB)ljpeg$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lpng$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lungif$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)zlib$(EXT.LIB)

$(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(EXT.EXE): $(DIR.OBJ) $(DIR.LIST) $(SOURCE.TESTAPP_DEDICATED.OBJ)
	$(DO.LINK.CONSOLE.EXE)
	$(CP) $(DIR.OBJ)/$(EXE_DEDICATED_NAME)$(BUILD_SUFFIX).* $(BIN_DIRECTORY)

#----------------------------------------
torqueClean:
ifneq ($(wildcard $(EXE_NAME)_DEBUG.*),)
	-$(RM)  $(EXE_NAME)$(BUILD_SUFFIX)*
endif
ifneq ($(wildcard $(EXE_NAME)_RELEASE.*),)
	-$(RM)  $(EXE_NAME)_RELEASE*
endif
