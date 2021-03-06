cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(Service)

include(../cmake/project-version.cmake)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES service.cpp srv_version.cpp)
set(HEADERS service.h srv_version.h)
set(LIBS HelpzBase)

include(../cmake/build-definitions.cmake)

target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../qtservice/src")
target_link_libraries(${TARGET} Helpzqtservice)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core Sql REQUIRED)
qt5_use_modules(${TARGET} Core Sql)

set(REQUIRED_DEBS_QT "libqt5core5a,libqt5sql5")
set(REQUIRED_PKGS_QT "qt5-base")

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install.cmake)
