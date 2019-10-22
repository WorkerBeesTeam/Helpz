cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(DBMeta)

include(../cmake/project-version.cmake)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES db_table.cpp)
set(HEADERS db_meta.h db_table.h)

include(../cmake/build-definitions.cmake)
include(../cmake/resolve-headers.cmake)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core REQUIRED)
qt5_use_modules(${TARGET} Core)

set(REQUIRED_PKGS_QT "libqt5core5a")

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install-prefix.cmake)
include(../cmake/install.cmake)
