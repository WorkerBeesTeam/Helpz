# -------------------------------------------------------------------
# Build definitions for building Helpz submodules
# -------------------------------------------------------------------

if (ANDROID)
    set(LIBRARY_TYPE_STATIC TRUE)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..) # TODO: is it a dirty hack?
set(TARGET Helpz${PROJECT_NAME})

if (LIBRARY_TYPE_STATIC)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif ()

add_definitions(-DVER_MJ=${PROJECT_VERSION_MAJOR} -DVER_MN=${PROJECT_VERSION_MINOR} -DVER_B=${PROJECT_VERSION_BUILD})
add_library(${TARGET} ${LIBRARY_TYPE} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES PUBLIC_HEADER "${HEADERS}")
