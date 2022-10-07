cmake_minimum_required(VERSION 3.19)

project(MBUltra)

set(CMAKE_CXX_STANDARD 14)

set(ProjectOutDir "${CMAKE_SOURCE_DIR}/game")

add_subdirectory(engine/lib/OpenMBU-WebAPI)
add_subdirectory(tools/cmake)
