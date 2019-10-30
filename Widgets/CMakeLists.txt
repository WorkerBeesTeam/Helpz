cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(Widgets)

include(../cmake/project-version.cmake)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

SET(SOURCES datewidget.cpp filterlineedit.cpp lineeditwithplaceholderslot.cpp
        simpledialog.cpp tableselectmultimode.cpp uncheckslotcheckbox.cpp)
SET(HEADERS datewidget.h filterlineedit.h lineeditwithplaceholderslot.h
        simpledialog.h tableselectmultimode.h uncheckslotcheckbox.h)

include(../cmake/build-definitions.cmake)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
qt5_use_modules(${TARGET} Core Widgets)

set(REQUIRED_DEBS_QT "libqt5core5a,libqt5widgets5")
set(REQUIRED_PKGS_QT "qt5-base")

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install.cmake)
