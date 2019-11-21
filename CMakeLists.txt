cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(Helpz)

add_subdirectory(Base)
add_subdirectory(Database)
add_subdirectory(DBMeta)
add_subdirectory(DTLS)
add_subdirectory(Network)
add_subdirectory(Service)
add_subdirectory(qtservice)

add_subdirectory(DBWidgets)
add_subdirectory(Widgets)

add_subdirectory(include)

add_dependencies(HelpzDTLS HelpzNetwork)
add_dependencies(HelpzNetwork HelpzBase)
add_dependencies(HelpzDB HelpzDBMeta)
add_dependencies(HelpzService HelpzBase Helpzqtservice)

add_dependencies(HelpzDev-deb HelpzBase HelpzDB HelpzDBMeta HelpzDTLS HelpzNetwork HelpzService HelpzDBWidgets HelpzWidgets)

add_custom_target(all-deb
        DEPENDS HelpzDev-deb HelpzBase-deb HelpzDB-deb HelpzDBMeta-deb HelpzDTLS-deb HelpzNetwork-deb HelpzService-deb
        HelpzDBWidgets-deb HelpzWidgets-deb)

add_custom_target(all-pkg
        DEPENDS HelpzDev-pkg HelpzBase-pkg HelpzDB-pkg HelpzDBMeta-pkg HelpzDTLS-pkg HelpzNetwork-pkg HelpzService-pkg
        HelpzDBWidgets-pkg HelpzWidgets-pkg)
