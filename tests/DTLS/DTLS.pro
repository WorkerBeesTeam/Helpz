QT       += core testlib
QT       -= gui

TARGET = tst_DTLS
CONFIG += c++1z console
CONFIG += qt warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_main.cpp \
    client_protocol.cpp \
    server_protocol.cpp

HEADERS += \
    tst_main.h \
    client_protocol.h \
    server_protocol.h

INCLUDEPATH += $${OUT_PWD}/../../include

LIBS += -L$${OUT_PWD}/../.. -lHelpzBase -lHelpzDTLS
LIBS += -lbotan-2 -lHelpzNetwork -lHelpzDB -lHelpzDBMeta -lboost_system -lboost_thread

RESOURCES += \
    main.qrc
