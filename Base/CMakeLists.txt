cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(Base)

include(../cmake/project-version.cmake)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 2)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES logging.cpp consolereader.cpp base_version.cpp)
set(HEADERS logging.h consolereader.h base_version.h simplethread.h settingshelper.h fake_integer_sequence.h apply_parse.h)

include(../cmake/build-definitions.cmake)
include(../cmake/resolve-headers.cmake)

# -------------------------------------------------------------------
# Qt integrration
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
