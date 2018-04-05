QT += network

TARGET = DTLS

debug {
    DEFINES += DEBUG
}

INCLUDEPATH += ../Network/

SOURCES += \
    dtlsproto.cpp \
    dtlsserver.cpp \
    dtlsclient.cpp \
    dtlsbasic.cpp \
    dtlsservernode.cpp
#    OpenSSL/dtlsclient.cpp \
#    OpenSSL/dtlscookie.cpp \
#    OpenSSL/dtlsserver.cpp \
#    OpenSSL/dtlssocket.cpp

HEADERS += \
    dtlsproto.h \
    dtlsserver.h \
    dtlsclient.h \
    dtlsbasic.h \
    dtlsservernode.h
#    OpenSSL/dtlssocket.h \
#    OpenSSL/dtlsserver.h \
#    OpenSSL/dtlscookie.h \
#    OpenSSL/dtlsclient.h

linux-rasp-pi2-g++ {
#    INCLUDEPATH += /mnt/second_drive/build/botan_build/include/botan-1.11
}

LIBS += -L$${OUT_PWD}/..
LIBS += -lbotan-2 -lHelpzNetwork

include(../helpz_install.pri)
