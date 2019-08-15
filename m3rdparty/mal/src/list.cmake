SET(MAL_DIR ${CMAKE_CURRENT_LIST_DIR})

SET(MAL_INCLUDE
    ${MAL_DIR}/include
)

SET(MAL_MODEL
    ${MAL_DIR}/src/model/nodegraph.cpp
    ${MAL_DIR}/include/nodegraph/model/nodegraph.h
)

SET(MAL_VIEW
    ${MAL_DIR}/src/view/imgui_canvas.cpp
    ${MAL_DIR}/src/view/imgui_node_editor.cpp
    ${MAL_DIR}/src/view/Builders.cpp
    ${MAL_DIR}/src/view/node_editor.cpp
    ${MAL_DIR}/src/view/ImNodes.cpp
    ${MAL_DIR}/src/view/ImNodesEz.cpp
    ${MAL_DIR}/include/nodegraph/view/imgui_canvas.h
    ${MAL_DIR}/include/nodegraph/view/imgui_node_editor.h
    ${MAL_DIR}/include/nodegraph/view/Builders.h
    ${MAL_DIR}/include/nodegraph/view/imgui_canvas.h
    ${MAL_DIR}/include/nodegraph/view/imgui_extra_math.h
    ${MAL_DIR}/include/nodegraph/view/imgui_extra_widgets.h
)

SET(MAL_SOURCE
    ${MAL_DIR}/list.cmake

    ${MAL_MODEL}
    ${MAL_VIEW}

)

LIST(APPEND TEST_SOURCES
    ${MAL_DIR}/src/model/nodegraph.test.cpp
)

ADD_LIBRARY (NodeGraph ${MAL_SOURCE})
TARGET_LINK_LIBRARIES(NodeGraph ImGui)

TARGET_INCLUDE_DIRECTORIES(NodeGraph
PUBLIC
    ${MAL_INCLUDE}
)

SOURCE_GROUP ("model" FILES ${MAL_MODEL})
SOURCE_GROUP ("view" FILES ${MAL_VIEW})

