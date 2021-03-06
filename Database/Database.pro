QT += sql

TARGET = DB

SOURCES += db_base.cpp \
    db_version.cpp \
    db_connection_info.cpp \
    db_delete_row.cpp \
    db_builder.cpp \
    db_thread.cpp
HEADERS += db_base.h \
    db_version.h \
    db_connection_info.h \
    db_delete_row.h \
    db_builder.h \
    db_thread.h

LIBS += -lHelpzDBMeta

VER_MAJ = 1
VER_MIN = 6
include(../helpz_install.pri)
