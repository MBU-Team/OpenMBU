project(rapidjson C CXX)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson
  INTERFACE
    include)
if(MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(rapidjson
    INTERFACE
      -Wno-unknown-warning-option)
endif()
