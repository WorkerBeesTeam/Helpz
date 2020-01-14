QT       += core testlib
QT       -= gui

TARGET = tst_network
CONFIG += c++1z console
CONFIG += qt warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_main.cpp

INCLUDEPATH += $${OUT_PWD}/../../include

LIBS += -L$${OUT_PWD}/../.. -lHelpzBase -lHelpzNetwork -lboost_thread
