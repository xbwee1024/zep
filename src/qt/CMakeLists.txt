if (BUILD_QT)

# Qt Specific
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_PREFIX_PATH $ENV{QT_INSTALL_LOCATION})

find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

set_property(GLOBAL PROPERTY AUTOMOC_FOLDER Automoc)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER Automoc)

set(ZEP_SOURCE_QT
    ${ZEP_ROOT}/src/qt/zepdisplay_qt.cpp
    ${ZEP_ROOT}/src/qt/common-qt.cpp
    ${ZEP_ROOT}/src/qt/zepwidget_qt.cpp
    ${ZEP_ROOT}/src/qt/CMakeLists.txt
    ${ZEP_ROOT}/include/zep/qt/zepwidget_qt.h
    ${ZEP_ROOT}/include/zep/qt/zepdisplay_qt.h
    )

set(ZEP_INCLUDE_QT ${ZEP_ROOT}/src/qt)

add_library(ZepQt STATIC ${ZEP_SOURCE_QT})
add_library(Zep::ZepQt ALIAS ZepQt)

set_property(TARGET ZepQt PROPERTY POSITION_INDEPENDENT_CODE ON) # Qt requires this

target_include_directories(ZepQt
    PRIVATE
        ${ZEP_INCLUDE_QT}
        ${ZEP_ROOT}/include
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Core_INCLUDE_DIRS}
    PUBLIC
        $<INSTALL_INTERFACE:include>
)

set_target_properties(ZepQt PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ZepQt
    EXPORT zep-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
)

endif() # BUILD_QT


