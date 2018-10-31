QT += network
QT -= gui

TARGET = Network

SOURCES += \  
    udpclient.cpp \
    prototemplate.cpp \
    waithelper.cpp \
    net_version.cpp

HEADERS += \
    udpclient.h \
    prototemplate.h \
    waithelper.h \
    net_version.h

LIBS += -lHelpzBase

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
