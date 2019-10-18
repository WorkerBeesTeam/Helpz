# -------------------------------------------------------------------
# Install settings
# -------------------------------------------------------------------

install(TARGETS ${TARGET} DESTINATION lib)

add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND checkinstall -D --pkgname=lib${TARGET} --pkgversion=${PROJECT_VERSION_STRING} --bk --nodoc --default --install=no --pakdir=../ > checkinstall.log)
