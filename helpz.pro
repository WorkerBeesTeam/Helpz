TEMPLATE = subdirs

SUBDIRS = \
    Base \
    DBMeta \
    Database \
    Service

Service.depends = Base
Database.depends = DBMeta
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
