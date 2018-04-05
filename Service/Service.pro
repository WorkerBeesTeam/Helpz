QT += core sql

TARGET = Service

INCLUDEPATH += ../qtservice/src/
include(../qtservice/src/qtservice.pri)

SOURCES += \
    service.cpp \
    log.cpp \
    consolereader.cpp

HEADERS += \
    service.h \
    logging.h \
    consolereader.h

include(../helpz_install.pri)

qtservice_files.files = ../qtservice/src/qtservice.h
qtservice_files.path = $$ZIMNIKOV_INCLUDES/
INSTALLS += qtservice_files
