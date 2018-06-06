# Version
defined(VER_MAJ, var):defined(VER_MIN, var) {
    VER_BUILD = 0
    exists($${OUT_PWD}/build_number) {
        VER_BUILD = $$system(cat $${OUT_PWD}/build_number)
    }
    VERSION = $${VER_MAJ}.$${VER_MIN}.100

    !defined(VER_NS, var) {
      VER_NS = $${TARGET}
    }

    gen_ver.name = Version generator
    gen_ver.CONFIG += target_predeps
    gen_ver.commands = $${PWD}/generate_version.sh "$${OUT_PWD}" "$${VER_NS}" $${VER_MAJ} $${VER_MIN}
    gen_ver.depends = FORCE
    QMAKE_EXTRA_TARGETS += gen_ver
    PRE_TARGETDEPS += gen_ver

#    HEADERS += $${OUT_PWD}/version.h
#    SOURCES += $${OUT_PWD}/version.cpp
}
