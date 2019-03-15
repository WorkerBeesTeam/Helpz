QT += network
QT -= gui

TARGET = Network

SOURCES += \  
    udpclient.cpp \
    waithelper.cpp \
    net_version.cpp \
    net_protocol.cpp \
    net_protocol_sender.cpp \
    net_protocol_timer.cpp

HEADERS += \
    udpclient.h \
    waithelper.h \
    net_version.h \
    net_protocol.h \
    net_protocol_sender.h \
    net_protocol_timer.h \
    net_defs.h

LIBS += -lHelpzBase

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)
