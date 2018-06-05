TEMPLATE = subdirs

SUBDIRS = \
    Database \
    Service

DTLS.depends = Network

!nobotanandproto {
    SUBDIRS += \
        DTLS \
        Network
} else {
    message('nobotanandproto FOUND');
}

!nowidgets {
    SUBDIRS += \
        Widgets \
        DBWidgets
} else {
    message('nowidgets FOUND');
}
