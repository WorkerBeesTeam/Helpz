TEMPLATE = subdirs

SUBDIRS = \
    Base \
    DBMeta \
    Database \
    Service

Service.depends = Base
Database.depends = DBMeta

!nobotanandproto {
    SUBDIRS += Network DTLS
    Network.depends = Base
    DTLS.depends = Network

    CONFIG(debug, debug|release) {
        SUBDIRS += tests
        tests.depends = DTLS

        CONFIG ~= s/-O[0123s]//g
        CONFIG += -O0
    }

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
