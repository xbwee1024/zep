cmake_minimum_required(VERSION 3.2)

message(STATUS " CMakeLists: SDL2")

project(SDL2Builder)

INCLUDE(ExternalProject)
ExternalProject_Add(
  sdl2
  PREFIX "m3rdparty"
  URL https://www.libsdl.org/release/SDL2-2.0.10.zip
  CMAKE_ARGS -DCMAKE_STATIC=ON
  SOURCE_DIR "${M3RDPARTY_DIR}/sdl"
  INSTALL_DIR ${CMAKE_BINARY_DIR}/sdl2
)

add_library(SDL2Builder STATIC builder.cpp)
add_dependencies(SDL2Builder sdl2)
