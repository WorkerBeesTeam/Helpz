TEMPLATE = subdirs

SUBDIRS = \
    Base \
    Database \
    Service

Service.depends = Base
DTLS.depends = Network

!nobotanandproto {
    SUBDIRS += \
        DTLS \
        Network
    Network.depends = Base
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
