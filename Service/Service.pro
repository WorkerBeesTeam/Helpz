QT += core sql

TARGET = Service

INCLUDEPATH += ../qtservice/src/
include(../qtservice/src/qtservice.pri)

SOURCES += \
    service.cpp \
    log.cpp \
    consolereader.cpp \
    srv_version.cpp

HEADERS += \
    service.h \
    logging.h \
    consolereader.h \
    srv_version.h

VER_MAJ = 1
VER_MIN = 2
include(../helpz_install.pri)

qtservice_files.files = ../qtservice/src/qtservice.h
qtservice_files.path = $$ZIMNIKOV_INCLUDES/
INSTALLS += qtservice_files
