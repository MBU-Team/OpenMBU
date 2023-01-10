project(TorqueShaderEngine)

if( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8 )
    set( TORQUE_CPU_X64 ON )
elseif( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 4 )
    set( TORQUE_CPU_X32 ON )
endif()

if(NOT projectOutDir)
    set(projectOutDir "${CMAKE_SOURCE_DIR}/game")
endif()
set(libDir        "${CMAKE_SOURCE_DIR}/engine/lib")
set(srcDir        "${CMAKE_SOURCE_DIR}/engine/source")
set(cmakeDir      "${CMAKE_SOURCE_DIR}/tools/cmake")

# hide some things
mark_as_advanced(CMAKE_INSTALL_PREFIX)
mark_as_advanced(CMAKE_CONFIGURATION_TYPES)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    add_compile_options (-fdiagnostics-color=always)
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options (-fcolor-diagnostics)
endif ()

#if (WIN32)
#	set(CMAKE_ASM_NASM_COMPILER "${CMAKE_SOURCE_DIR}/engine/bin/nasm/nasmw.exe")
#endif()
#enable_language(ASM_NASM)
#set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <SOURCE> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT>" FORCE)
#SET(ASM_OPTIONS "-x assembler-with-cpp")
#SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

# finds and adds sources files in a folder
macro(addPath dir)
    set(tmp_files "")
    set(glob_config GLOB)
    if(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "REC")
        set(glob_config GLOB_RECURSE)
    endif()
	set(mac_files "")
	if(APPLE)
		set(mac_files ${dir}/*.mm ${dir}/*.m)
	endif()
    file(${glob_config} tmp_files
             ${dir}/*.cpp
             ${dir}/*.c
             ${dir}/*.cc
             ${dir}/*.h
             ${dir}/*.cxx
             ${mac_files}
             #${dir}/*.asm
             )
	#file(${glob_config} asm_files ${dir}/*.asm)
    #foreach(entry ${asm_files})
	#	set_property(SOURCE ${entry} APPEND PROPERTY COMPILE_OPTIONS ${ASM_OPTIONS})
	#endforeach()
    foreach(entry ${BLACKLIST})
 		list(REMOVE_ITEM tmp_files ${dir}/${entry})
 	endforeach()
    LIST(APPEND ${PROJECT_NAME}_files "${tmp_files}")
    LIST(APPEND ${PROJECT_NAME}_paths "${dir}")
    #message(STATUS "addPath ${PROJECT_NAME} : ${tmp_files}")
endmacro()

# adds a file to the sources
macro(addFile filename)
    LIST(APPEND ${PROJECT_NAME}_files "${filename}")
    #message(STATUS "addFile ${PROJECT_NAME} : ${filename}")
endmacro()

# finds and adds sources files in a folder recursively
macro(addPathRec dir)
    addPath("${dir}" "REC")
endmacro()

###############################################################################
### Definition Handling
###############################################################################
macro(__addDef def config)
    # two possibilities: a) target already known, so add it directly, or b) target not yet known, so add it to its cache
    if(TARGET ${PROJECT_NAME})
        #message(STATUS "directly applying defs: ${PROJECT_NAME} with config ${config}: ${def}")
        if("${config}" STREQUAL "")
            set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS ${def})
        else()
            set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:${config}>:${def}>)
        endif()
    else()
        if("${config}" STREQUAL "")
            list(APPEND ${PROJECT_NAME}_defs_ ${def})
        else()
            list(APPEND ${PROJECT_NAME}_defs_ $<$<CONFIG:${config}>:${def}>)
        endif()
        #message(STATUS "added definition to cache: ${PROJECT_NAME}_defs_: ${${PROJECT_NAME}_defs_}")
    endif()
endmacro()

# adds a definition: argument 1: Nothing(for all), _DEBUG, _RELEASE, <more build configurations>
macro(addDef def)
    set(def_configs "")
    if(${ARGC} GREATER 1)
        foreach(config ${ARGN})
            __addDef(${def} ${config})
        endforeach()
    else()
        __addDef(${def} "")
    endif()
endmacro()

# this applies cached definitions onto the target
macro(_process_defs)
    if(DEFINED ${PROJECT_NAME}_defs_)
        set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS ${${PROJECT_NAME}_defs_})
        #message(STATUS "applying defs to project ${PROJECT_NAME}: ${${PROJECT_NAME}_defs_}")
    endif()
endmacro()

###############################################################################
###  Source Library Handling
###############################################################################
macro(addLibSrc libPath)
    set(cached_project_name ${PROJECT_NAME})
    include(${libPath})
    project(${cached_project_name})
endmacro()

###############################################################################
### Linked Library Handling
###############################################################################
macro(addLib libs)
   foreach(lib ${libs})
        # check if we can build it ourselfs
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
            addLibSrc("${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
        endif()
        # then link against it
        # two possibilities: a) target already known, so add it directly, or b) target not yet known, so add it to its cache
        if(TARGET ${PROJECT_NAME})
            target_link_libraries(${PROJECT_NAME} "${lib}")
        else()
            list(APPEND ${PROJECT_NAME}_libs ${lib})
        endif()
   endforeach()
endmacro()

#addLibRelease will add to only release builds
macro(addLibRelease libs)
   foreach(lib ${libs})
        # check if we can build it ourselfs
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
            addLibSrc("${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
        endif()
        # then link against it
        # two possibilities: a) target already known, so add it directly, or b) target not yet known, so add it to its cache
        if(TARGET ${PROJECT_NAME})
            target_link_libraries(${PROJECT_NAME} optimized "${lib}")
        else()
            list(APPEND ${PROJECT_NAME}_libsRelease ${lib})
        endif()
   endforeach()
endmacro()

#addLibDebug will add to only debug builds
macro(addLibDebug libs)
   foreach(lib ${libs})
        # check if we can build it ourselfs
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
            addLibSrc("${CMAKE_CURRENT_SOURCE_DIR}/libraries/${lib}.cmake")
        endif()
        # then link against it
        # two possibilities: a) target already known, so add it directly, or b) target not yet known, so add it to its cache
        if(TARGET ${PROJECT_NAME})
            target_link_libraries(${PROJECT_NAME} debug "${lib}")
        else()
            list(APPEND ${PROJECT_NAME}_libsDebug ${lib})
        endif()
   endforeach()
endmacro()

# this applies cached definitions onto the target
macro(_process_libs)
    if(DEFINED ${PROJECT_NAME}_libs)
        target_link_libraries(${PROJECT_NAME} "${${PROJECT_NAME}_libs}")
    endif()
    if(DEFINED ${PROJECT_NAME}_libsRelease)
        target_link_libraries(${PROJECT_NAME} optimized "${${PROJECT_NAME}_libsRelease}")
    endif()
    if(DEFINED ${PROJECT_NAME}_libsDebug)
        target_link_libraries(${PROJECT_NAME} debug "${${PROJECT_NAME}_libsDebug}")
    endif()

endmacro()

# apple frameworks
macro(addFramework framework)
	if (APPLE)
		addLib("-framework ${framework}")
	endif()
endmacro()

###############################################################################
### Include Handling
###############################################################################
macro(addInclude incPath)
    if(TARGET ${PROJECT_NAME})
        set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES "${incPath}")
    else()
        list(APPEND ${PROJECT_NAME}_includes ${incPath})
    endif()
endmacro()

# this applies cached definitions onto the target
macro(_process_includes)
    if(DEFINED ${PROJECT_NAME}_includes)
        set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INCLUDE_DIRECTORIES "${${PROJECT_NAME}_includes}")
    endif()
endmacro()

###############################################################################

macro(_postTargetProcess)
    _process_includes()
    _process_defs()
    _process_libs()
endmacro()

# adds a path to search for libs
macro(addLibPath dir)
    link_directories(${dir})
endmacro()

# creates a proper filter for VS
macro(generateFilters relDir)
    foreach(f ${${PROJECT_NAME}_files})
        # Get the path of the file relative to ${DIRECTORY},
        # then alter it (not compulsory)
        file(RELATIVE_PATH SRCGR ${relDir} ${f})
        set(SRCGR "${PROJECT_NAME}/${SRCGR}")
        # Extract the folder, ie remove the filename part
        string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})
        # do not have any ../ dirs
        string(REPLACE "../" "" SRCGR ${SRCGR})
        # Source_group expects \\ (double antislash), not / (slash)
        string(REPLACE / \\ SRCGR ${SRCGR})
        #STRING(REPLACE "//" "/" SRCGR ${SRCGR})
        #message(STATUS "FILE: ${f} -> ${SRCGR}")
        source_group("${SRCGR}" FILES ${f})
    endforeach()
endmacro()

# creates a proper filter for VS
macro(generateFiltersSpecial relDir)
    foreach(f ${${PROJECT_NAME}_files})
        # Get the path of the file relative to ${DIRECTORY},
        # then alter it (not compulsory)
        file(RELATIVE_PATH SRCGR ${relDir} ${f})
        set(SRCGR "torque/${SRCGR}")
        # Extract the folder, ie remove the filename part
        string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})
        # do not have any ../ dirs
        string(REPLACE "../" "" SRCGR ${SRCGR})
        IF("${SRCGR}" MATCHES "^torque/My Projects/.*$")
            string(REPLACE "torque/My Projects/${PROJECT_NAME}/" "" SRCGR ${SRCGR})
            string(REPLACE "/source" "" SRCGR ${SRCGR})
        endif()
        # Source_group expects \\ (double antislash), not / (slash)
        string(REPLACE / \\ SRCGR ${SRCGR})
        #STRING(REPLACE "//" "/" SRCGR ${SRCGR})
        IF(EXISTS "${f}" AND NOT IS_DIRECTORY "${f}")
            #message(STATUS "FILE: ${f} -> ${SRCGR}")
            source_group("${SRCGR}" FILES ${f})
        endif()
    endforeach()
endmacro()

# macro to add a static library
macro(finishLibrary)
    # more paths?
    if(${ARGC} GREATER 0)
        foreach(dir ${ARGV0})
            addPath("${dir}")
        endforeach()
    endif()
    # now inspect the paths we got
    set(firstDir "")
    foreach(dir ${${PROJECT_NAME}_paths})
        if("${firstDir}" STREQUAL "")
            set(firstDir "${dir}")
        endif()
    endforeach()
    generateFilters("${firstDir}")

    # set per target compile flags
    if(TORQUE_CXX_FLAGS_${PROJECT_NAME})
        set_source_files_properties(${${PROJECT_NAME}_files} PROPERTIES COMPILE_FLAGS "${TORQUE_CXX_FLAGS_${PROJECT_NAME}}")
    else()
        set_source_files_properties(${${PROJECT_NAME}_files} PROPERTIES COMPILE_FLAGS "${TORQUE_CXX_FLAGS_LIBS}")
    endif()

    if(TORQUE_STATIC)
        add_library("${PROJECT_NAME}" STATIC ${${PROJECT_NAME}_files})
    else()
        add_library("${PROJECT_NAME}" SHARED ${${PROJECT_NAME}_files})
    endif()

    # omg - only use the first folder ... otherwise we get lots of header name collisions
    #foreach(dir ${${PROJECT_NAME}_paths})
    addInclude("${firstDir}")
    #endforeach()

    _postTargetProcess()
endmacro()

# macro to add an executable
macro(finishExecutable)
    # now inspect the paths we got
    set(firstDir "")
    foreach(dir ${${PROJECT_NAME}_paths})
        if("${firstDir}" STREQUAL "")
            set(firstDir "${dir}")
        endif()
    endforeach()
    generateFiltersSpecial("${firstDir}")

    # set per target compile flags
    if(TORQUE_CXX_FLAGS_${PROJECT_NAME})
        set_source_files_properties(${${PROJECT_NAME}_files} PROPERTIES COMPILE_FLAGS "${TORQUE_CXX_FLAGS_${PROJECT_NAME}}")
    else()
        set_source_files_properties(${${PROJECT_NAME}_files} PROPERTIES COMPILE_FLAGS "${TORQUE_CXX_FLAGS_EXECUTABLES}")
    endif()

    if (APPLE)
      set(ICON_FILE "${srcDir}/resources/mac/torque.icns")
        set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        add_executable("${PROJECT_NAME}" MACOSX_BUNDLE ${ICON_FILE} ${${PROJECT_NAME}_files})
    else()
        add_executable("${PROJECT_NAME}" WIN32 ${${PROJECT_NAME}_files})
    endif()
    addInclude("${firstDir}")

    _postTargetProcess()
endmacro()

# always static for now
set(TORQUE_STATIC ON)
#option(TORQUE_STATIC "enables or disable static" OFF)

if(WIN32)
    set(TORQUE_CXX_FLAGS_EXECUTABLES "/wd4018 /wd4100 /wd4121 /wd4127 /wd4130 /wd4244 /wd4245 /wd4389 /wd4511 /wd4512 /wd4800 /wd4995 " CACHE STRING STRING)
    mark_as_advanced(TORQUE_CXX_FLAGS_EXECUTABLES)

    set(TORQUE_CXX_FLAGS_LIBS "/W0" CACHE STRING STRING)
    mark_as_advanced(TORQUE_CXX_FLAGS_LIBS)

    #set(TORQUE_CXX_FLAGS_COMMON_DEFAULT "-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS /MP /O2 /Ob2 /Oi /Ot /Oy /GT /Zi /W4 /nologo /GF /EHsc /GS- /Gy- /Qpar /fp:precise /fp:except- /GR /Zc:wchar_t-" )
	set(TORQUE_CXX_FLAGS_COMMON_DEFAULT "-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS /MP /Oi /Ot /Oy /GT /Zi /nologo /GF /EHsc /GS- /Gy- /Qpar /fp:precise /fp:except- /GR" )

    # Temp: TempFix Use /RTCs and /fp:precise to Fix Physics Bugs in Multiplayer
    #set(TORQUE_CXX_FLAGS_COMMON_DEFAULT "-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS /MP /GT /Zi /nologo /GF /EHsc /GS- /Gy- /Qpar /fp:precise /fp:except- /GR /RTCs /Od" )
    if( TORQUE_CPU_X32 )
       set(TORQUE_CXX_FLAGS_COMMON_DEFAULT "${TORQUE_CXX_FLAGS_COMMON_DEFAULT} /arch:SSE2")
    endif()
    set(TORQUE_CXX_FLAGS_COMMON ${TORQUE_CXX_FLAGS_COMMON_DEFAULT} CACHE STRING STRING)

    mark_as_advanced(TORQUE_CXX_FLAGS_COMMON)

    #STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    #STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od")
    #set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORQUE_CXX_FLAGS_COMMON}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
    # set(CMAKE_EXE_LINKER_FLAGS "/LARGEADDRESSAWARE")
    #set(STATIC_LIBRARY_FLAGS "/OPT:NOREF")

    # Force static runtime libraries
    if(TORQUE_STATIC)
        FOREACH(flag
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_DEBUG_INIT
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_DEBUG_INIT)
            STRING(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
            #STRING(REPLACE "/O2"  "/Od" "${flag}" "${${flag}}") # Temp: TempFix Disable Optimization to Fix Physics Bugs in Multiplayer
            SET("${flag}" "${${flag}} /EHsc")
        ENDFOREACH()
    endif()
else()
    # TODO: improve default settings on other platforms
    set(TORQUE_CXX_FLAGS_EXECUTABLES "" CACHE STRING "")
    mark_as_advanced(TORQUE_CXX_FLAGS_EXECUTABLES)
    set(TORQUE_CXX_FLAGS_LIBS "" CACHE STRING "")
    mark_as_advanced(TORQUE_CXX_FLAGS_LIBS)
    set(TORQUE_CXX_FLAGS_COMMON "" CACHE STRING "")
    mark_as_advanced(TORQUE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORQUE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
endif()

if(UNIX AND NOT APPLE)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${projectOutDir}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${projectOutDir}")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${projectOutDir}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${projectOutDir}")
endif()

if(APPLE)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${projectOutDir}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${projectOutDir}")
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${projectOutDir}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${projectOutDir}")
endif()

# fix the debug/release subfolders on windows
if(MSVC)
    SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY" "${projectOutDir}")
    FOREACH(CONF ${CMAKE_CONFIGURATION_TYPES})
        # Go uppercase (DEBUG, RELEASE...)
        STRING(TOUPPER "${CONF}" CONF)
        #SET("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF}" "${projectOutDir}")
        SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF}" "${projectOutDir}")
    ENDFOREACH()
endif()