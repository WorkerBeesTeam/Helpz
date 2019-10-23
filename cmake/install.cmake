# -------------------------------------------------------------------
# Install settings
# -------------------------------------------------------------------

install(TARGETS ${TARGET} DESTINATION lib)

if(REQUIRED_PKGS OR REQUIRED_PKGS_QT)
    if (REQUIRED_PKGS AND REQUIRED_PKGS_QT)
        set(REQUIRED_PKGS "${REQUIRED_PKGS_QT},${REQUIRED_PKGS}")
    elseif(REQUIRED_PKGS_QT)
        set(REQUIRED_PKGS ${REQUIRED_PKGS_QT})
    endif()
    set(REQUIRES_STR "--requires=\"${REQUIRED_PKGS}\"")
endif()

message("checkinstall -D --pkgname=lib${TARGET} --pkgversion=${PROJECT_VERSION_STRING} ${REQUIRES_STR} --bk --nodoc --default --install=no --pakdir=../")
add_custom_target(${TARGET}-deb
        COMMAND checkinstall -D --pkgname=lib${TARGET} --pkgversion=${PROJECT_VERSION_STRING} ${REQUIRES_STR} --bk --nodoc --default --install=no --pakdir=../
        DEPENDS ${TARGET})
