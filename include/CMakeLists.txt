project(Dev)

include(../cmake/project-version.cmake)
include(../cmake/install-prefix.cmake)

add_custom_target(HelpzDev
        SOURCES Helpz/)

install(DIRECTORY Helpz
        DESTINATION include
        COMPONENT Development
        FILES_MATCHING
        PATTERN *.h*)
add_custom_command(TARGET HelpzDev
        POST_BUILD
        COMMAND checkinstall -D --pkgname=libhelpz-dev --pkgversion=${PROJECT_VERSION_STRING} --bk --nodoc --default --install=no --pakdir=../ --requires="libhelpzbase,libhelpzdb,libhelpzdbmeta,libhelpzwidgets,libhelpzdtls,libhelpznetwork,libhelpzservice,libhelpzwidgets,libboost-thread1.67-dev,libboost-system1.67-dev,libqt5serialbus5-dev,qt5-default,qt5-qmake,libbotan-2-dev,libqt5serialport5-dev")
