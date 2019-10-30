# -------------------------------------------------------------------
# Installation settings
# -------------------------------------------------------------------

include(install-prefix)

install(TARGETS ${TARGET} DESTINATION lib)

include(pkg/deb)
include(pkg/aur)
