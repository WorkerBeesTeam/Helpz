QT += sql

TARGET = DB

SOURCES += db_base.cpp \
    db_version.cpp \
    db_connectioninfo.cpp
HEADERS += db_base.h \
    db_version.h \
    db_connectioninfo.h

VER_MAJ = 1
VER_MIN = 4
include(../helpz_install.pri)
