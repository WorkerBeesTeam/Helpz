QT -= gui
TEMPLATE = lib

TARGET = Helpz$$TARGET

INCLUDEPATH += $$PWD/include

unix : !android {
    INSTALL_PREFIX = /usr/local
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
    CONFIG += c++14 staticlib
} else {
    CONFIG += c++1z
}

HELPZ_INCLUDES = $$INSTALL_PREFIX/include/Helpz

header_files.files = $$HEADERS
header_files.path = $$HELPZ_INCLUDES
INSTALLS += header_files

target.path = $$INSTALL_PREFIX/lib
INSTALLS += target

DESTDIR = $${OUT_PWD}/../

# ByMsx: resolving headers, mkdir for output like /include/Helpz
MKDIR_FLAG=
win32 {
    HAVE_COPY=$$system(copy /?)
    !isEmpty(HAVE_COPY):LINK_METHOD=copy /Y
} else {
    MKDIR_FLAG=-p
}
isEmpty(LINK_METHOD):LINK_METHOD=ln -f -s

!exists($${OUT_PWD}) {
    RET=$$system(mkdir $$MKDIR_FLAG $$system_quote($$system_path($${OUT_PWD})))
}
!exists($${OUT_PWD}/../include/Helpz) {
    RET=$$system(mkdir $$MKDIR_FLAG $$system_quote($$system_path($${OUT_PWD}/../include/Helpz)))
}

# ByMsx: for resolving headers by copying it to other directory.
for(f, HEADERS) {
    FILE_BASE = $$basename(f)

    F_STARTS = $$str_member($$f, 0, 0)
    equals(F_STARTS, "/") {
        FILE_FROM = $$f
    } else {
        FILE_FROM = $${_PRO_FILE_PWD_}/$$f
    }

    RET=$$system($$LINK_METHOD $$system_quote($$system_path($$FILE_FROM)) $$system_quote($$system_path($${OUT_PWD}/../include/Helpz/$$FILE_BASE)))
}

INCLUDEPATH += $${OUT_PWD}/../include
LIBS += -L$${OUT_PWD}/..

include(helpz_version.pri)

# ByMsx: code below is completed
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
