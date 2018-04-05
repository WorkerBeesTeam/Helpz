TEMPLATE = subdirs

SUBDIRS = \
    Database \
    Service \
    Widgets \
    DBWidgets

DTLS.depends = Network

!nobotanandproto {
    SUBDIRS += \
        DTLS \
        Network
    message('nobotanandproto');
}

nobotanandproto {
    message('nobotanandproto FOUND');
}
