QT -= gui
CONFIG += c++1z
TEMPLATE = lib

TARGET = Helpz$$TARGET

INCLUDEPATH += $$PWD/include

unix : !android {
    INSTALL_PREFIX = /usr
} else {
    INSTALL_PREFIX = $$[QT_INSTALL_PREFIX]
}

linux-g++* {
    #INCLUDEPATH += /usr/local/mylibs/include
    #LIBS += -L/usr/local/mylibs/lib
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
}

linux-rasp-pi2-g++ {
    target_rpi.path = $$[QT_INSTALL_LIBS]
    INSTALLS += target_rpi
}

android {
    CONFIG += staticlib

    INCLUDEPATH += /mnt/second_drive/Android/libs/build/include
    LIBS += -L/mnt/second_drive/Android/libs/build/lib
}

HELPZ_INCLUDES = $$INSTALL_PREFIX/include/Helpz

header_files.files = $$HEADERS
header_files.path = $$HELPZ_INCLUDES
INSTALLS += header_files

target.path = $$INSTALL_PREFIX/lib
INSTALLS += target

DESTDIR = ../

win32 {
    HAVE_COPY=$$system(copy /?)
    !isEmpty(HAVE_COPY):LINK_METHOD=copy /Y
}
isEmpty(LINK_METHOD):LINK_METHOD=ln -f -s

!exists($${OUT_PWD}/../include) {
    RET=$$system(mkdir $$system_quote($$system_path($${OUT_PWD}/../include)))
}
!exists($${OUT_PWD}/../include/Helpz) {
    RET=$$system(mkdir $$system_quote($$system_path($${OUT_PWD}/../include/Helpz)))
}
for(f, HEADERS) {
  FILE_BASE = $$basename(f)
  exists($$f) {
    FILE_FROM = $$f
  } else {
    FILE_FROM = $${PWD}/$$f
  }
  RET=$$system($$LINK_METHOD $$system_quote($$system_path($$FILE_FROM)) $$system_quote($$system_path($${OUT_PWD}/../include/Helpz/$$FILE_BASE)))
}

INCLUDEPATH += $${OUT_PWD}/../include

include(helpz_version.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
