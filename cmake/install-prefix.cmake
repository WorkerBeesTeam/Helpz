# -------------------------------------------------------------------------
# Installation prefix settings
# Moved to a separate file because need to use in include/CMakeLists.txt
# -------------------------------------------------------------------------

if (UNIX AND NOT(ANDROID))
    set(CMAKE_INSTALL_PREFIX /usr)
endif()
