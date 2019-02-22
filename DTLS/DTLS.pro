QT += network sql

TARGET = DTLS

debug {
    DEFINES += DEBUG
}

INCLUDEPATH += ../Network/

SOURCES += \
    dtls_version.cpp \
    dtls_tools.cpp \
    dtls_credentials_manager.cpp \
    dtls_session_manager_sql.cpp \
    dtls_client_controller.cpp \
    dtls_client.cpp \
    dtls_client_thread.cpp \
    dtls_controller.cpp \
    dtls_socket.cpp \
    dtls_server_thread.cpp \
    dtls_server.cpp \
    dtls_server_controller.cpp \
    dtls_server_node.cpp \
    dtls_node.cpp
#    OpenSSL/dtlsclient.cpp \
#    OpenSSL/dtlscookie.cpp \
#    OpenSSL/dtlsserver.cpp \
#    OpenSSL/dtlssocket.cpp

HEADERS += \
    dtls_version.h \
    dtls_tools.h \
    dtls_credentials_manager.h \
    dtls_session_manager_sql.h \
    dtls_client_controller.h \
    dtls_client.h \
    dtls_client_thread.h \
    dtls_controller.h \
    dtls_socket.h \
    dtls_server_thread.h \
    dtls_server.h \
    dtls_server_controller.h \
    dtls_server_node.h \
    dtls_node.h
#    OpenSSL/dtlssocket.h \
#    OpenSSL/dtlsserver.h \
#    OpenSSL/dtlscookie.h \
#    OpenSSL/dtlsclient.h

linux-rasp-pi2-g++ {
#    INCLUDEPATH += /mnt/second_drive/build/botan_build/include/botan-1.11
}

win32 {
    QMAKE_CXXFLAGS += -fstack-protector
    QMAKE_LFLAGS += -fstack-protector
}
LIBS += -L$${OUT_PWD}/..
LIBS += -lbotan-2 -lHelpzNetwork -lHelpzDB -lboost_thread

VER_MAJ = 1
VER_MIN = 3
include(../helpz_install.pri)
