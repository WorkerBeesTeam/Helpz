QT += sql

TARGET = DB

SOURCES += db_base.cpp \
    db_version.cpp
HEADERS += db_base.h \
    db_version.h

VER_MAJ = 1
VER_MIN = 4
include(../helpz_install.pri)
