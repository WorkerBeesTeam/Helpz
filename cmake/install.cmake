# -------------------------------------------------------------------
# Install settings
# -------------------------------------------------------------------

install(TARGETS ${TARGET}
        DESTINATION lib
        COMPONENT Libraries)

add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND checkinstall -D --pkgname=libhelpz${TARGET} --pkgversion=1.5.100 --bk --nodoc --default)

#unix : !android { TODO:
#INSTALL_PREFIX = /usr/local
#} else {
#INSTALL_PREFIX = $$[QT_INSTALL_PREFIX]
#}
