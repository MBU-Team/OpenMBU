project(MBUltra)

if(UNIX)
    if(NOT CXX_FLAG32)
        set(CXX_FLAG32 "")
    endif()
    #set(CXX_FLAG32 "-m32") #uncomment for build x32 on OSx64

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAG32} -Wundef -msse -pipe -Wfatal-errors -Wno-return-type-c-linkage -Wno-unused-local-typedef ${TORQUE_ADDITIONAL_LINKER_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CXX_FLAG32} -Wundef -msse -pipe -Wfatal-errors -Wno-return-type-c-linkage -Wno-unused-local-typedef ${TORQUE_ADDITIONAL_LINKER_FLAGS}")
    else()
    # default compiler flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAG32} -Wundef -msse -pipe -Wfatal-errors ${TORQUE_ADDITIONAL_LINKER_FLAGS} -Wl,-rpath,'$$ORIGIN'")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CXX_FLAG32} -Wundef -msse -pipe -Wfatal-errors ${TORQUE_ADDITIONAL_LINKER_FLAGS} -Wl,-rpath,'$$ORIGIN'")

   endif()

	# for asm files
	SET (CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
	ENABLE_LANGUAGE (ASM_NASM)

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

###############################################################################
# options
###############################################################################
if(NOT MSVC AND NOT APPLE) # handle single-configuration generator
    set(TORQUE_BUILD_TYPE "Debug" CACHE STRING "Select one of Debug, Release and RelWithDebInfo")
    set_property(CACHE TORQUE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")

    set(TORQUE_ADDITIONAL_LINKER_FLAGS "" CACHE STRING "Additional linker flags")
    mark_as_advanced(TORQUE_ADDITIONAL_LINKER_FLAGS)
endif()

if(WIN32)
    # warning C4800: 'XXX' : forcing value to bool 'true' or 'false' (performance warning)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")
    # warning C4018: '<' : signed/unsigned mismatch
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018")
    # warning C4244: 'initializing' : conversion from 'XXX' to 'XXX', possible loss of data
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244")

    if( TORQUE_CPU_X64 )
        link_directories("${libDir}/directx9/Lib/x64")
    else()
        link_directories("${libDir}/directx9/Lib/x86")
    endif()

	addLib(d3d9)
	addLib(d3dx9)
	addLib(dxerr)
endif()

# build types
if(NOT MSVC AND NOT APPLE) # handle single-configuration generator
	set(CMAKE_BUILD_TYPE ${TORQUE_BUILD_TYPE})
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(TORQUE_DEBUG TRUE)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(TORQUE_RELEASE TRUE)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(TORQUE_RELEASE TRUE)
    else()
		message(FATAL_ERROR "Please select Debug, Release or RelWithDebInfo for TORQUE_BUILD_TYPE")
	endif()
endif()

###############################################################################
# Always enabled paths first
###############################################################################
addPath("${srcDir}/") # must come first :)
addPath("${srcDir}/autosplitter")
addPath("${srcDir}/collision")
addPath("${srcDir}/console")
addPath("${srcDir}/core")
addPath("${srcDir}/editor")
#addPath("${srcDir}/editor/terrain")
#addPathRec("${srcDir}/atlas")
addPath("${srcDir}/game")
addPath("${srcDir}/game/fps")
addPath("${srcDir}/game/fx")
addPath("${srcDir}/game/marble")
addPath("${srcDir}/game/net")
addPath("${srcDir}/game/vehicles")
addPath("${srcDir}/gfx")
addPath("${srcDir}/gfx/Null")
if (WIN32)
	addPath("${srcDir}/gfx/D3D")
    addPath("${srcDir}/gfx/D3D9")
    addPath("${srcDir}/gfx/D3D9/pc")
endif()
addPath("${srcDir}/gui")
addPath("${srcDir}/gui/containers")
addPath("${srcDir}/gui/controls")
addPath("${srcDir}/gui/core")
addPath("${srcDir}/gui/editor")
addPath("${srcDir}/gui/game")
addPath("${srcDir}/gui/shiny")
addPath("${srcDir}/gui/utility")
addPath("${srcDir}/i18n")
addPath("${srcDir}/interior")
addPath("${srcDir}/lightingSystem")
addPath("${srcDir}/materials")
addPath("${srcDir}/math")
addPath("${srcDir}/platform")
addPath("${srcDir}/renderInstance")
addPath("${srcDir}/sceneGraph")
addPath("${srcDir}/sfx")
addPath("${srcDir}/sfx/vorbis")
addPath("${srcDir}/sfx/null")
addPath("${srcDir}/shaderGen")
addPath("${srcDir}/sim")
#addPath("${srcDir}/terrain")
addPath("${srcDir}/terrain/environment")
addPath("${srcDir}/ts")
addPath("${srcDir}/util")
addPath("${srcDir}/xblive")

###############################################################################
# modular paths
###############################################################################

if(WIN32)
    set(TORQUE_SFX_DIRECTSOUND OFF) # Disable DirectSound for now

    # DirectSound
    if(TORQUE_SFX_DIRECTSOUND AND NOT TORQUE_DEDICATED)
        addPath("${srcDir}/sfx/dsound")
    endif()

    set(TORQUE_SFX_XAUDIO OFF) # Disable XAudio for now

    # XAudio
    if (TORQUE_SFX_XAUDIO AND NOT TORQUE_DEDICATED)
        addPath("${srcDir}/sfx/xaudio")
        addLib(xaudio2)
    endif()
endif()

set(TORQUE_SFX_OPENAL ON)

# OpenAL
if(TORQUE_SFX_OPENAL AND NOT TORQUE_DEDICATED)
    addPath("${srcDir}/sfx/openal")
    if(WIN32)
	   addInclude("${libDir}/openal/win32")
       addPath("${srcDir}/sfx/openal/win32")
    elseif(UNIX)
       addInclude("${libDir}/openal/LINUX")
       addPath("${srcDir}/sfx/openal/linux")
    endif()
endif()

set(TORQUE_SFX_VORBIS ON)

# Vorbis
if(TORQUE_SFX_VORBIS)
    addInclude(${libDir}/libvorbis/include)
    addInclude(${libDir}/libogg/include)
    addInclude(${CMAKE_CURRENT_BINARY_DIR}/libogg/include)
    addDef(TORQUE_OGGVORBIS)
    addLib(libvorbis)
    addLib(libogg)
endif()

#modules dir
file(GLOB modules "modules/*.cmake")
foreach(module ${modules})
	include(${module})
endforeach()

###############################################################################
# platform specific things
###############################################################################
if(WIN32)
    addPath("${srcDir}/platformWin32")
    addPath("${srcDir}/resources")
    # add windows rc file for the icon
    addFile("${srcDir}/resources/TSE.rc")
endif()

if(APPLE)
    addPath("${srcDir}/platformMac")
    addPath("${srcDir}/platformPOSIX")
endif()

if(UNIX AND NOT APPLE)
    # linux_dedicated
    if(TORQUE_DEDICATED)
		addPath("${srcDir}/windowManager/dedicated")
		# ${srcDir}/platformX86UNIX/*.client.* files are not needed
		# @todo: move to separate file
		file( GLOB tmp_files
             ${srcDir}/platformX86UNIX/*.cpp
             ${srcDir}/platformX86UNIX/*.c
             ${srcDir}/platformX86UNIX/*.cc
             ${srcDir}/platformX86UNIX/*.h )
        file( GLOB tmp_remove_files ${srcDir}/platformX86UNIX/*client.* )
        LIST( REMOVE_ITEM tmp_files ${tmp_remove_files} )
        foreach( f ${tmp_files} )
            addFile( ${f} )
        endforeach()
    else()
        addPath("${srcDir}/platformX86UNIX")
        addPath("${srcDir}/platformX86UNIX/nativeDialogs")
    endif()
    # linux
    addPath("${srcDir}/platformPOSIX")
endif()

if(UNIX)
    FIND_PACKAGE(SDL REQUIRED)
    addInclude(${SDL_INCLUDE_DIR})
    #addLib(${SDL_LIBRARY})
    #target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY})
    #message("SDL: " ${SDL_LIBRARY})
endif()

###############################################################################
###############################################################################
finishExecutable()
if (UNIX)
    target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY})
endif()
###############################################################################
###############################################################################

# Set Visual Studio startup project
if((${CMAKE_VERSION} VERSION_EQUAL 3.6.0) OR (${CMAKE_VERSION} VERSION_GREATER 3.6.0) AND MSVC)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MBUltra)
endif()

# configure the relevant files only once

if(APPLE)
  #icon
  if(NOT EXISTS "${srcDir}/resources/mac/torque.icns")
      CONFIGURE_FILE("${cmakeDir}/torque.icns" "${srcDir}/resources/mac/torque.icns" COPYONLY)
  endif()
  #plist
  if(NOT EXISTS "${srcDir}/resources/mac/Info.plist")
      CONFIGURE_FILE("${cmakeDir}/Info.plist.in" "${srcDir}/resources/mac/Info.plist" COPYONLY)
  endif()
  #target properties for mac
  set_target_properties("${PROJECT_NAME}" PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${srcDir}/resources/mac/Info.plist")
else()
  if(NOT EXISTS "${srcDir}/resources/icon.ico")
      CONFIGURE_FILE("${cmakeDir}/icon.ico" "${srcDir}/resources/icon.ico" COPYONLY)
  endif()
endif()

###############################################################################
# Common Libraries
###############################################################################
addLib(lpng)
addLib(ljpeg)
addLib(lmng)
addLib(zlib)
addLib(lungif)
addLib(libqslim)

# curl
set(BUILD_CURL_EXE OFF CACHE INTERNAL "" FORCE)
set(CURL_CA_PATH "none" CACHE INTERNAL "" FORCE)
set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
set(CMAKE_USE_LIBSSH2 OFF CACHE INTERNAL "" FORCE)
set(CURL_STATICLIB ON CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(ENABLE_MANUAL OFF CACHE INTERNAL "" FORCE)
set(ENABLE_UNIX_SOCKETS OFF CACHE INTERNAL "" FORCE)
set(HTTP_ONLY ON CACHE INTERNAL "" FORCE)
set(PICKY_COMPILER ON CACHE INTERNAL "" FORCE)
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE INTERNAL "" FORCE)
    set(CURL_STATIC_CRT ${TORQUE_STATIC} CACHE INTERNAL "" FORCE)
    set(ENABLE_INET_PTON ON CACHE INTERNAL "" FORCE)
elseif(APPLE)
    set(CURL_USE_OPENSSL OFF CACHE INTERNAL "" FORCE)
    set(CURL_USE_SECTRANSP ON CACHE INTERNAL "" FORCE)
endif()
add_subdirectory( ${libDir}/curl ${CMAKE_CURRENT_BINARY_DIR}/curl)

addInclude("${libDir}/curl/include")
addLib(libcurl)


if(WIN32)
    # copy pasted from T3D build system, some might not be needed
    set(TORQUE_EXTERNAL_LIBS "COMCTL32;COMDLG32;USER32;ADVAPI32;GDI32;WINMM;WS2_32;vfw32.lib;Imm32.lib;ole32.lib;shell32.lib;oleaut32.lib;version.lib" CACHE STRING "external libs to link against")
    mark_as_advanced(TORQUE_EXTERNAL_LIBS)
    addLib("${TORQUE_EXTERNAL_LIBS}")

   if(TORQUE_OPENGL)
      addLib(OpenGL32.lib)
   endif()
endif()

if (APPLE)
  addFramework("Cocoa")
  addFramework("OpenGL")
  #These are needed by sdl2 static lib
  addFramework("CoreAudio")
  addFramework("AudioUnit")
  addFramework("ForceFeedback")
  addFramework("IOKit")
  addFramework("CoreVideo")
  #grrr damn you sdl!
  addFramework("Carbon")
  addFramework("AudioToolbox")
  addLib("iconv")
  #set a few arch defaults
  set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX Architecture" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "OSX Deployment target" FORCE)
endif()

if(UNIX AND NOT APPLE)
    # copy pasted from T3D build system, some might not be needed
	set(TORQUE_EXTERNAL_LIBS "dl Xxf86vm Xext X11 Xft stdc++ pthread GL" CACHE STRING "external libs to link against")
	mark_as_advanced(TORQUE_EXTERNAL_LIBS)

    string(REPLACE " " ";" TORQUE_EXTERNAL_LIBS_LIST ${TORQUE_EXTERNAL_LIBS})
    addLib( "${TORQUE_EXTERNAL_LIBS_LIST}" )
endif()

###############################################################################
# Always enabled Definitions
###############################################################################
addDef(TORQUE_DEBUG Debug)
addDef(TORQUE_ENABLE_ASSERTS "Debug;RelWithDebInfo")
addDef(TORQUE_DEBUG_GFX_MODE "RelWithDebInfo")
addDef(UNICODE)
addDef(_UNICODE) # for VS
addDef(TORQUE_UNICODE)
addDef(_CRT_SECURE_NO_WARNINGS)
addDef(_CRT_SECURE_NO_DEPRECATE)

if(UNIX AND NOT APPLE)
	addDef(LINUX)
endif()

if(TORQUE_OPENGL)
	addDef(TORQUE_OPENGL)
endif()

###############################################################################
# Include Paths
###############################################################################
addInclude("${srcDir}/")
addInclude("${libDir}/libqslim")
addInclude("${libDir}/lmng")
addInclude("${libDir}/lpng")
addInclude("${libDir}/ljpeg")
addInclude("${libDir}/lungif")
addInclude("${libDir}/zlib")

if(UNIX AND NOT APPLE)
	addInclude("/usr/include/freetype2/freetype")
	addInclude("/usr/include/freetype2")
endif()

# external things
if(WIN32)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES "${libDir}/directx9/Include")
endif()

if(MSVC)
    if (DEFINED CUSTOM_SUFFIX)
        set(CUSTOM_SUFFIX_REAL _${CUSTOM_SUFFIX})
    else()
        set(CUSTOM_SUFFIX_REAL "")
    endif()

    # Match projectGenerator naming for executables
    set(OUTPUT_CONFIG DEBUG MINSIZEREL RELWITHDEBINFO)
    set(OUTPUT_SUFFIX DEBUG MINSIZE    OPTIMIZEDDEBUG)
    foreach(INDEX RANGE 2)
        list(GET OUTPUT_CONFIG ${INDEX} CONF)
        list(GET OUTPUT_SUFFIX ${INDEX} SUFFIX)
        if (TORQUE_CPU_X64)
            set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME_${CONF} ${PROJECT_NAME}64_${SUFFIX}${CUSTOM_SUFFIX_REAL})
        else()
            set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME_${CONF} ${PROJECT_NAME}_${SUFFIX}${CUSTOM_SUFFIX_REAL})
        endif()
    endforeach()
	if (TORQUE_CPU_X64)
		set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME_RELEASE ${PROJECT_NAME}64${CUSTOM_SUFFIX_REAL})
	endif()
endif()