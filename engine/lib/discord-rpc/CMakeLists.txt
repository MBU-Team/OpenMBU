project(discord-rpc C CXX)

if (WIN32)
    add_library(discord-rpc STATIC
      include/discord_register.h
      include/discord_rpc.h
      src/backoff.h
      src/connection.h
      src/discord_rpc.cpp
      src/discord_register.h
      src/msg_queue.h
      src/rpc_connection.cpp
      src/rpc_connection.h
      src/serialization.cpp
      src/serialization.h

      src/connection_win.cpp
      src/discord_register_win.cpp)
elseif(APPLE)
    add_library(discord-rpc STATIC
      include/discord_register.h
      include/discord_rpc.h
      src/backoff.h
      src/connection.h
      src/discord_rpc.cpp
      src/discord_register.h
      src/msg_queue.h
      src/rpc_connection.cpp
      src/rpc_connection.h
      src/serialization.cpp
      src/serialization.h

      src/connection_unix.cpp
      src/discord_register_osx.m)
else()
    add_library(discord-rpc STATIC
      include/discord_register.h
      include/discord_rpc.h
      src/backoff.h
      src/connection.h
      src/discord_rpc.cpp
      src/discord_register.h
      src/msg_queue.h
      src/rpc_connection.cpp
      src/rpc_connection.h
      src/serialization.cpp
      src/serialization.h

      src/connection_unix.cpp
      src/discord_register_linux.cpp)
endif()

target_compile_definitions(discord-rpc
  PUBLIC
    DISCORD_DISABLE_IO_THREAD)

target_include_directories(discord-rpc
  PUBLIC
    include)

target_link_libraries(discord-rpc
  PRIVATE
    rapidjson)

if(WIN32)
  target_compile_definitions(discord-rpc
    PRIVATE
      DISCORD_WINDOWS)
elseif(APPLE)
  target_compile_definitions(discord-rpc
    PRIVATE
      DISCORD_OSX)
  target_link_libraries(discord-rpc
    PRIVATE
      AppKit
      pthread)
endif()