# Build the zep demo
IF (BUILD_DEMOS AND BUILD_IMGUI)

set(DEMO_NAME ZepDemo)

find_package(SDL2 REQUIRED NO_DEFAULT_PATH PATHS ${M3RDPARTY_DIR}/packages/build/sdl2-package)

IF(WIN32)
# Let windows know that the app is DPI Aware
set(MANIFEST_SOURCE ${ZEP_ROOT}/dpiawarescaling.manifest)
ENDIF()
set(DEMO_SOURCE_IMGUI
    ${ZEP_ROOT}/demo_imgui/main.cpp
    ${ZEP_ROOT}/demo_imgui/CMakeLists.txt
    ${M3RDPARTY_DIR}/imgui/examples/imgui_impl_sdl.cpp
    ${M3RDPARTY_DIR}/imgui/examples/imgui_impl_opengl3.cpp
    ${M3RDPARTY_DIR}/imgui/examples/libs/gl3w/GL/gl3w.c
    ${M3RDPARTY_DIR}/imgui/imgui_demo.cpp
    ${M3RDPARTY_DIR}/imgui/imgui_widgets.cpp
    ${M3RDPARTY_DIR}/imgui/imgui_draw.cpp
    ${M3RDPARTY_DIR}/imgui/imgui.cpp)

SET (IMGUI_INCLUDE 
    m3rdparty/imgui/imgui.h
    m3rdparty/imgui/imgui_dock.h
    m3rdparty/imgui/imgui_orient.h)

add_executable (${DEMO_NAME} WIN32 # Win32 ignored on windows 
    ${DEMO_SOURCE_IMGUI}
    ${IMGUI_SOURCE}
    ${MANIFEST_SOURCE}
    ${M3RDPARTY_DIR}/tfd/tinyfiledialogs.c) 

target_include_directories(${DEMO_NAME}
    PRIVATE
        include
        ${SDL2_ROOT_DIR}
        ${CMAKE_BINARY_DIR}
        ${M3RDPARTY_DIR}
        ${M3RDPARTY_DIR}/tclap/include
        ${M3RDPARTY_DIR}/imgui
        ${M3RDPARTY_DIR}/tfd
        ${M3RDPARTY_DIR}/imgui/examples/libs/gl3w)

copy_existing_files(${DEMO_NAME} m3rdparty/imgui/misc/fonts/ProggyClean.ttf ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

target_link_libraries (${DEMO_NAME} 
    PRIVATE
        ZepImGui
        Zep
        Mal
        SDL2::SDL2-static
        SDL2::SDL2main
        ${PLATFORM_LINKLIBS})

# Install the font
install(FILES m3rdparty/imgui/misc/fonts/ProggyClean.ttf
    DESTINATION ${CMAKE_INSTALL_BINDIR}/imgui
)

install(TARGETS ${DEMO_NAME}
    EXPORT zep-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/imgui
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
)

source_group (Zep FILES ${DEMO_SOURCE_IMGUI})

ENDIF() # IMGUI
