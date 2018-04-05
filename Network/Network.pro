QT += network
QT -= gui

TARGET = Network

SOURCES += \  
    udpclient.cpp \
    prototemplate.cpp \
    waithelper.cpp

HEADERS += \
    simplethread.h \
    udpclient.h \
    prototemplate.h \
    waithelper.h \
    settingshelper.h

include(../helpz_install.pri)
