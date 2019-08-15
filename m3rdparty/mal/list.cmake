SET(MAL_DIR ${CMAKE_CURRENT_LIST_DIR})

SET(MAL_INCLUDE
    ${MAL_DIR}/include
)

SET(MAL_CORE
    ${MAL_DIR}/src/core.cpp
    ${MAL_DIR}/src/environment.cpp
    ${MAL_DIR}/src/mal.cpp
    ${MAL_DIR}/src/reader.cpp
    ${MAL_DIR}/src/string.cpp
    ${MAL_DIR}/src/types.cpp
    ${MAL_DIR}/src/validation.cpp
    ${MAL_DIR}/include/mal/debug.h
    ${MAL_DIR}/include/mal/environment.h
    ${MAL_DIR}/include/mal/mal.h
    ${MAL_DIR}/include/mal/refcountedptr.h
    ${MAL_DIR}/include/mal/staticlist.h
    ${MAL_DIR}/include/mal/string.h
    ${MAL_DIR}/include/mal/types.h
    ${MAL_DIR}/include/mal/validation.h
)

SET(MAL_SOURCE
    ${MAL_DIR}/list.cmake
    ${MAL_CORE}
)

ADD_LIBRARY (Mal ${MAL_SOURCE})

TARGET_INCLUDE_DIRECTORIES(Mal
PUBLIC
    ${MAL_INCLUDE}
)

SOURCE_GROUP ("Mal" FILES ${MAL_SOURCE})

