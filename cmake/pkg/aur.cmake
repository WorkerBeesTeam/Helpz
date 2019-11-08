# -------------------------------------------------------------------
# AUR (Pacman) package generation
# -------------------------------------------------------------------
if(REQUIRED_PKGS OR REQUIRED_PKGS_QT)
    if (REQUIRED_PKGS AND REQUIRED_PKGS_QT)
        set(REQUIRED_PKGS "${REQUIRED_PKGS_QT} ${REQUIRED_PKGS}")
    elseif(REQUIRED_PKGS_QT)
        set(REQUIRED_PKGS ${REQUIRED_PKGS_QT})
    endif()

    set(DEPENDS_STR "${REQUIRED_PKGS}")
endif()

add_custom_target(${TARGET}-pkg-prepare-root
        COMMAND make DESTDIR=${TARGET}-tmp-install install && cd ${TARGET}-tmp-install && tar czvf ../root-${TARGET}.tar.gz **
        DEPENDS ${TARGET})

add_custom_target(${TARGET}-pkg-prepare-pkgbuild
        COMMAND sed 's@pkgname=@pkgname=${TARGET}@\; s@pkgver=@pkgver=${PROJECT_VERSION_STRING}@\; s@root.tar.gz@root-${TARGET}.tar.gz@\; s@depends=\(\)@depends=\(${DEPENDS_STR}\)@' ${CMAKE_SOURCE_DIR}/PKGBUILD > ${CMAKE_CURRENT_BINARY_DIR}/PKGBUILD-${TARGET}
        DEPENDS ${TARGET})

add_custom_target(${TARGET}-pkg
        COMMAND makepkg -dp PKGBUILD-${TARGET}
        DEPENDS ${TARGET}-pkg-prepare-root ${TARGET}-pkg-prepare-pkgbuild)
