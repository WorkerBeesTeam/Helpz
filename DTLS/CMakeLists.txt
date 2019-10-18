cmake_minimum_required(VERSION 3.10)

# -------------------------------------------------------------------
# Project common information
# -------------------------------------------------------------------

project(DTLS)

include(../cmake/project-version.cmake)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 5)

# -------------------------------------------------------------------
# Build information
# -------------------------------------------------------------------

set(SOURCES dtls_version.cpp dtls_tools.cpp dtls_credentials_manager.cpp dtls_session_manager_sql.cpp
        dtls_client_controller.cpp dtls_client.cpp dtls_client_thread.cpp dtls_controller.cpp dtls_socket.cpp
        dtls_server_thread.cpp dtls_server.cpp dtls_server_controller.cpp dtls_server_node.cpp dtls_node.cpp
        dtls_client_node.cpp)
set(HEADERS dtls_version.h dtls_tools.h dtls_credentials_manager.h dtls_session_manager_sql.h dtls_client_controller.h
        dtls_client.h dtls_client_thread.h dtls_controller.h dtls_socket.h dtls_server_thread.h dtls_server.h
        dtls_server_controller.h dtls_server_node.h dtls_node.h dtls_client_node.h)
set(LIBS botan-2 HelpzNetwork HelpzDB boost_system boost_thread)

find_package(Boost COMPONENTS thread system REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_definitions(-DDEBUG)
endif()

if(WIN32)
    add_compile_options(-fstack-protector)
    add_link_options(-fstack-protector)
endif()

include(../cmake/build-definitions.cmake)
include(../cmake/resolve-headers.cmake)

target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../qtservice/src")
target_link_libraries(${TARGET} Helpzqtservice)

# -------------------------------------------------------------------
# Qt integration
# -------------------------------------------------------------------

include(../cmake/qt-defines.cmake)
find_package(Qt5 COMPONENTS Core Network Sql REQUIRED)
qt5_use_modules(${TARGET} Core Network Sql)

# -------------------------------------------------------------------
# Installation
# -------------------------------------------------------------------

include(../cmake/install-prefix.cmake)
include(../cmake/install.cmake)
