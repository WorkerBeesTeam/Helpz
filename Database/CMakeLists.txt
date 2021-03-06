cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(DB)

include(../cmake/project-version.cmake)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 5)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES db_base.cpp
    db_version.cpp
    db_connection_info.cpp
    db_delete_row.cpp
    db_builder.cpp
    db_thread.cpp)

set(HEADERS db_base.h
    db_version.h
    db_connection_info.h
    db_delete_row.h
    db_builder.h
    db_thread.h)

set(LIBS HelpzDBMeta)

set(REQUIRED_DEBS "libhelpzdbmeta") # (>=1.3.100)
set(REQUIRED_PKGS "HelpzDBMeta")

include(../cmake/build-definitions.cmake)

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
