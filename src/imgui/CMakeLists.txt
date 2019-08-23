if (BUILD_IMGUI)

set(ZEP_SOURCE_IMGUI
    ${ZEP_ROOT}/src/imgui/CMakeLists.txt
    ${ZEP_ROOT}/src/imgui/display_imgui.cpp
    ${ZEP_ROOT}/src/imgui/editor_imgui.cpp
    ${ZEP_ROOT}/include/zep/imgui/editor_imgui.h
    ${ZEP_ROOT}/include/zep/imgui/display_imgui.h)

add_library(ZepImGui STATIC ${ZEP_SOURCE_IMGUI})
add_library(Zep::ZepImGui ALIAS ZepImGui)

target_include_directories(ZepImGui 
    PRIVATE 
        ${M3RDPARTY_DIR}/imgui
        ${ZEP_ROOT}/include
    PUBLIC 
        $<INSTALL_INTERFACE:include>
    )

set_target_properties(ZepImGui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ZepImGui
    EXPORT zep-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
)

SOURCE_GROUP ("m3rdparty\\imgui" REGULAR_EXPRESSION "(imgui.cpp|imgui_)+.*")
SOURCE_GROUP ("m3rdparty\\gl" REGULAR_EXPRESSION "(gl3w)+.*")

endif() # BUILD_IMGUI


