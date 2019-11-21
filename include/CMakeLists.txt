project(Dev)

include(../cmake/project-version.cmake)
include(../cmake/install-prefix.cmake)

set(PROJECT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_BUILD}")

set(TARGET HelpzDev)

add_custom_target(${TARGET}
        SOURCES Helpz/)

install(DIRECTORY Helpz
        DESTINATION include
        COMPONENT Development
        FILES_MATCHING
        PATTERN *.h*)
add_custom_target(HelpzDev-deb
        COMMAND checkinstall -D --pkgname=libhelpz-dev --pkgversion=${PROJECT_VERSION_STRING} --bk --nodoc --default --install=no --pakdir=../ --requires="libhelpzbase,libhelpzdb,libhelpzdbmeta,libhelpzwidgets,libhelpzdtls,libhelpznetwork,libhelpzservice,libhelpzwidgets,libboost-thread1.67-dev,libboost-system1.67-dev,libqt5serialbus5-dev,libbotan-2-dev"
        DEPENDS HelpzDev)

set(REQUIRED_PKGS "HelpzBase HelpzDB HelpzDBMeta HelpzDBWidgets HelpzDTLS HelpzNetwork HelpzService HelpzWidgets")
set(REQUIRED_PKGS_QT "")

include(../cmake/pkg/aur.cmake)
