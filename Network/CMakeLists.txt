cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(Network)

include(../cmake/project-version.cmake)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES udpclient.cpp waithelper.cpp net_version.cpp net_protocol.cpp net_protocol_sender.cpp
        net_protocol_timer.cpp net_fragmented_message.cpp)
set(HEADERS udpclient.h waithelper.h net_version.h net_protocol.h net_protocol_sender.h net_protocol_timer.h
        net_defs.h net_fragmented_message.h)
set(LIBS HelpzBase)

include(../cmake/build-definitions.cmake)
include(../cmake/resolve-headers.cmake)

set(REQUIRED_PKGS "libboost-system1.71.0,libboost-thread1.71.0,libhelpzbase")

find_package(Boost 1.71.0 COMPONENTS thread system REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../qtservice/src")
target_link_libraries(${TARGET} Helpzqtservice)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core Network REQUIRED)
qt5_use_modules(${TARGET} Core Network)

set(REQUIRED_PKGS_QT "libqt5core5a,libqt5network5")

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install-prefix.cmake)
include(../cmake/install.cmake)
