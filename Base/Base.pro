QT += core
QT -= gui

TARGET = Base

SOURCES += \
    logging.cpp \
    consolereader.cpp \
    base_version.cpp

HEADERS += \
    logging.h \
    consolereader.h \
    base_version.h \
    simplethread.h \
    settingshelper.h \
    applyparse.h

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
