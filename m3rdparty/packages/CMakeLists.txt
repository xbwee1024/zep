cmake_minimum_required(VERSION 3.2)

message(STATUS " CMakeLists: SDL2")

project(PackageBuilder)

INCLUDE(ExternalProject)
ExternalProject_Add(
  sdl2
  URL https://www.libsdl.org/release/SDL2-2.0.10.zip
  INSTALL_DIR ${PROJECT_BINARY_DIR}/sdl2-package
  CMAKE_ARGS -DCMAKE_STATIC=ON -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)

add_library(PackageBuilder STATIC builder.cpp)
add_dependencies(PackageBuilder sdl2)
