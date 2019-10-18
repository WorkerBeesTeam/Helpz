cmake_minimum_required(VERSION 3.10)
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

add_dependencies(HelpzDTLS HelpzNetwork)
add_dependencies(HelpzNetwork HelpzBase)
add_dependencies(HelpzDB HelpzDBMeta)
add_dependencies(HelpzService HelpzBase Helpzqtservice)

add_custom_target(libhelpz-dev
        DEPENDS HelpzBase HelpzDB HelpzDBMeta HelpzDTLS HelpzNetwork HelpzService HelpzDBWidgets HelpzWidgets
        WORKING_DIRECTORY include)

install(TARGET libhelpz-dev
        ALL
        PUBLIC_HEADERS
        DESTINATION include
        COMPONENT Development)
