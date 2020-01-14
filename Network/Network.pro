QT += network
QT -= gui

TARGET = Network

SOURCES += \  
    udpclient.cpp \
    waithelper.cpp \
    net_version.cpp \
    net_protocol.cpp \
    net_protocol_sender.cpp \
    net_protocol_timer.cpp \
    net_fragmented_message.cpp \
    net_protocol_writer.cpp \
    net_message_item.cpp

HEADERS += \
    udpclient.h \
    waithelper.h \
    net_version.h \
    net_protocol.h \
    net_protocol_sender.h \
    net_protocol_timer.h \
    net_defs.h \
    net_fragmented_message.h \
    net_protocol_writer.h \
    net_message_item.h

LIBS += -lHelpzBase

VER_MAJ = 1
VER_MIN = 3
include(../helpz_install.pri)
