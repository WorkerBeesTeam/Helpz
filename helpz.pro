TEMPLATE = subdirs

# yes
SUBDIRS = \
    Base \
    DBMeta \
    Database \
    Service

Service.depends = Base # yes
Database.depends = DBMeta # yes
DTLS.depends = Network # yes

# yes
!nobotanandproto {
    SUBDIRS += \
        DTLS \
        Network
    Network.depends = Base
} else {
    message('nobotanandproto FOUND');
}

# yes
!nowidgets {
    SUBDIRS += \
        Widgets \
        DBWidgets
} else {
    message('nowidgets FOUND');
}
