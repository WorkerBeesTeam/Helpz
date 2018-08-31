QT += network
QT -= gui

TARGET = Network

SOURCES += \  
    udpclient.cpp \
    prototemplate.cpp \
    waithelper.cpp \
    net_version.cpp

HEADERS += \
    simplethread.h \
    udpclient.h \
    prototemplate.h \
    waithelper.h \
    settingshelper.h \
    net_version.h \
    applyparse.h

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
