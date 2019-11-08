cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(DBWidgets)

include(../cmake/project-version.cmake)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

SET(SOURCES dateeditdelegate.cpp)
SET(HEADERS dateeditdelegate.h dbwidgets_global.h)

include(../cmake/build-definitions.cmake)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core Sql Widgets REQUIRED)
qt5_use_modules(${TARGET} Core Sql Widgets)

set(REQUIRED_DEBS_QT "libqt5core5a,libqt5sql5,libqt5widgets5")
set(REQUIRED_PKGS_QT "qt5-base")

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install.cmake)
