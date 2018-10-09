# Version

win32 {
    CONFIG += skip_target_version_ext
}

defined(VER_MAJ, var):defined(VER_MIN, var) {
    VER_BUILD = 100

    unix {
        DATE_FORMULA=$$system(date +\"%-M + (%-H*60) + (%j*24*60)\")
        VER_BUILD=$$system(echo \"$(($$DATE_FORMULA))\")
    }
    VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_BUILD}

    DEFINES += VER_MJ=$${VER_MAJ} VER_MN=$${VER_MIN} VER_B=$${VER_BUILD}
}
