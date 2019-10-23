cmake_minimum_required(VERSION 3.10)
project(qtservice)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES src/qtservice.cpp)
set(HEADERS src/qtservice.h src/qtservice_p.h)

if (WIN32)
    set(SOURCES ${SOURCES} "src/qtservice_win.cpp")
    set(QT_COMPONENTS Core)
    set(REQUIRED_PKGS_QT "libqt5core5a")
endif()

if (UNIX)
    set(SOURCES ${SOURCES} src/qtservice_unix.cpp src/qtunixserversocket.cpp src/qtunixsocket.cpp)
    set(HEADERS ${HEADERS} src/qtunixsocket.h src/qtunixserversocket.h)
    set(QT_COMPONENTS Core Network)
    set(REQUIRED_PKGS_QT "libqt5core5a,libqt5network5")
endif()

set(LIBRARY_TYPE_STATIC TRUE)

include(../cmake/project-version.cmake)
include(../cmake/build-definitions.cmake)
include(../cmake/resolve-headers.cmake)
include(../cmake/qt-defines.cmake)

find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
qt5_use_modules(${TARGET} ${QT_COMPONENTS})

if (WIN32)
    target_link_libraries(${TARGET} user32)
    if (LIBRARY_TYPE_STATIC)
        add_definitions(QT_QTSERVICE_IMPORT)
    else()
        add_definitions(QT_QTSERVICE_EXPORT)
    endif()
endif()

#win32 { TODO: check: it works?
#qtservice-buildlib:shared:DEFINES += QT_QTSERVICE_EXPORT
#else:qtservice-uselib:DEFINES += QT_QTSERVICE_IMPORT
#}
