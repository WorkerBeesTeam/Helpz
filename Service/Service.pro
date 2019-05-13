QT += core sql

TARGET = Service

INCLUDEPATH += ../qtservice/src/

include(../qtservice/src/qtservice.pri)

SOURCES += \
    service.cpp \
    srv_version.cpp

HEADERS += \
    service.h \
    srv_version.h

LIBS += -lHelpzBase

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)

qtservice_files.files = ../qtservice/src/qtservice.h
qtservice_files.path = $$HELPZ_INCLUDES/
INSTALLS += qtservice_files
