# -------------------------------------------------------------------
# Debian package generation
# -------------------------------------------------------------------

if(REQUIRED_DEBS OR REQUIRED_DEBS_QT)
    if (REQUIRED_DEBS AND REQUIRED_DEBS_QT)
        set(REQUIRED_DEBS "${REQUIRED_DEBS_QT},${REQUIRED_DEBS}")
    elseif(REQUIRED_DEBS_QT)
        set(REQUIRED_DEBS ${REQUIRED_DEBS_QT})
    endif()
    set(REQUIRES_STR "--requires=\"${REQUIRED_DEBS}\"")
endif()

add_custom_target(${TARGET}-deb
        COMMAND checkinstall -D --pkgname=lib${TARGET} --pkgversion=${PROJECT_VERSION_STRING} ${REQUIRES_STR} --bk --nodoc --default --install=no --pakdir=../
        DEPENDS ${TARGET})
