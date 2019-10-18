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
        COMMAND checkinstall -D --pkgname=libhelpz-dev --pkgversion=${PROJECT_VERSION_STRING} --bk --nodoc --default --install=no --pakdir=../ > checkinstall.log)
