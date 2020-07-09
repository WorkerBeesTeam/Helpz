QT += core
QT -= gui

TARGET = Base

SOURCES += \
    logging.cpp \
    consolereader.cpp \
    base_version.cpp \
    data_stream.cpp \
    settings.cpp \
    zfile.cpp \
    zstring.cpp

HEADERS += \
    logging.h \
    consolereader.h \
    base_version.h \
    settings.h \
    simplethread.h \
    settingshelper.h \
    fake_integer_sequence.h \
    apply_parse.h \
    data_stream.h \
    zfile.h \
    zstring.h

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
