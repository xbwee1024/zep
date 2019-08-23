
FILE(GLOB_RECURSE FOUND_TEST_SOURCES "*.test.cpp")

LIST(APPEND TEST_SOURCES
    ${FOUND_TEST_SOURCES}
    tests/main.cpp
)

# Unit tests
# Require SDL/IMgui build to work
# Need to build app to build tests
IF (BUILD_IMGUI_APP)
set(CMAKE_AUTOMOC OFF)
include(tests/list.cmake)
enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_HAS_TR1_TUPLE=0")
set (TEST_SOURCES
    ${M3RDPARTY_DIR}/googletest/googletest/src/gtest-all.cc
    ${TEST_SOURCES}
)
add_executable (unittests ${TEST_SOURCES} )
add_dependencies(unittests sdl2 ZepImGui Zep)
target_link_libraries (unittests PRIVATE ZepImGui Zep ${PLATFORM_LINKLIBS} ${CMAKE_THREAD_LIBS_INIT})
add_test(unittests unittests)
target_include_directories(unittests PRIVATE
    ${M3RDPARTY_DIR}/googletest/googletest/include
    ${M3RDPARTY_DIR}/googletest/googletest
    ${M3RDPARTY_DIR}/googletest/googlemock/include
    ${M3RDPARTY_DIR}/googletest/googlemock
    ${CMAKE_BINARY_DIR}
    include
)
set_target_properties(unittests PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

ENDIF()
    
