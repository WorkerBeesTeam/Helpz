QT -= gui

TARGET = DBMeta

SOURCES += \
    db_table.cpp

HEADERS += \
    db_table.h \
    db_meta.h

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
