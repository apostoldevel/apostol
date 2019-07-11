cmake_minimum_required(VERSION 3.13)
project(apostol)

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-mod" )

add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

set(DELPHI_LIB_NAME delphi)
set(APOSTOL_CORE apostol-core)

find_library(GCC_LIBRARY libgcc.a)
find_library(GCC++_LIBRARY libgcc++.a)

#add_definitions(-static-libgcc)
#add_definitions(-D_UNICODE)

add_definitions(-DAPP_DEFAULT_LOCALE="en_US.UTF-8")
add_definitions(-DDELPHI_LIB_EXPORTS)
add_definitions(-DSYS_ERRNO_COUNT=135)

add_custom_target(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)

find_package(PostgreSQL REQUIRED)

include_directories(src/lib/delphi src/apostol-core src/apostol-aws src/apostol-bitcoin src/utils)

file(GLOB delphi_files src/lib/${DELPHI_LIB_NAME}/*.hpp src/lib/${DELPHI_LIB_NAME}/*.cpp)
file(GLOB apostol-core_files src/${APOSTOL_CORE}/*.hpp src/${APOSTOL_CORE}/*.cpp)

file(GLOB utils_files src/utils/*.hpp src/utils/*.cpp)

file(GLOB apostol-aws_files version.h src/apostol-aws/*.hpp src/apostol-aws/*.cpp)
file(GLOB modules-aws_files src/modules-aws/Modules.hpp src/modules-aws/*/*.hpp src/modules-aws/*/*.cpp)

file(GLOB apostol-bitcoin_files version.h src/apostol-bitcoin/*.hpp src/apostol-bitcoin/*.cpp)
file(GLOB modules-bitcoin_files src/modules-bitcoin/Modules.hpp src/modules-bitcoin/*/*.hpp src/modules-bitcoin/*/*.cpp)

link_libraries(pq dl)

# Delphi classes for C++
# ----------------------------------------------------------------------------------------------------------------------
add_definitions(-Dapplication_name="'${PROJECT_DESCRIPTION}'")
add_definitions(-DWWWServerName="${PROJECT_DESCRIPTION}")

add_library(${DELPHI_LIB_NAME} STATIC ${delphi_files})
target_link_libraries(${DELPHI_LIB_NAME} pthread)

# Apostol Web Service
# ----------------------------------------------------------------------------------------------------------------------
set(PROJECT_NAME "apostol")
set(PROJECT_DESCRIPTION "Apostol Web Service")
set(MODULE_LIB_NAME modules-aws)

message(STATUS "Project name: ${PROJECT_NAME}")
message(STATUS "Project description: ${PROJECT_DESCRIPTION}")

add_library(${MODULE_LIB_NAME} STATIC ${utils_files} ${modules-aws_files})
target_link_libraries(${MODULE_LIB_NAME} ${DELPHI_LIB_NAME} crypto curl)

add_executable(${PROJECT_NAME} ${apostol-core_files} ${apostol-aws_files})

target_link_libraries(${PROJECT_NAME} ${DELPHI_LIB_NAME} ${MODULE_LIB_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC
        APP_APOSTOL=1
        APP_NAME="${PROJECT_NAME}"
        APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
        APP_VAR="${PROJECT_NAME}"
        APP_OLDPID_EXT=".oldbin"
        APP_PREFIX="/usr/local/${PROJECT_NAME}/"
        APP_CONF_PREFIX="conf/"
        APP_CACHE_PREFIX="cache/"
        APP_SBIN_PATH="sbin/${PROJECT_NAME}"
        APP_CONF_FILE="${PROJECT_NAME}.conf"
        APP_PID_FILE="logs/${PROJECT_NAME}.pid"
        APP_LOCK_FILE="logs/${PROJECT_NAME}.lock"
        APP_ERROR_LOG_FILE="logs/error.log"
        APP_ACCESS_LOG_FILE="logs/access.log"
        APP_POSTGRES_LOG_FILE="logs/postgres.log"
        APP_DOC_ROOT="www/"
        )

add_dependencies(${PROJECT_NAME} auto_increment_version)
# ----------------------------------------------------------------------------------------------------------------------

# Apostol Bitcoin
# ----------------------------------------------------------------------------------------------------------------------
set(PROJECT_NAME "abc")
set(PROJECT_DESCRIPTION "Apostol Bitcoin")
set(MODULE_LIB_NAME modules-bitcoin)

message(STATUS "Project name: ${PROJECT_NAME}")
message(STATUS "Project description: ${PROJECT_DESCRIPTION}")

add_library(${MODULE_LIB_NAME} STATIC ${utils_files} ${modules-bitcoin_files})
#target_link_libraries(${MODULE_LIB_NAME} ${DELPHI_LIB_NAME} crypto curl)

add_executable(${PROJECT_NAME} ${apostol-core_files} ${apostol-bitcoin_files})

target_link_libraries(${PROJECT_NAME} ${DELPHI_LIB_NAME} ${MODULE_LIB_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC
        APP_ABC=1
        APP_NAME="${PROJECT_NAME}"
        APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
        APP_VAR="${PROJECT_NAME}"
        APP_OLDPID_EXT=".oldbin"
        APP_PREFIX="/usr/local/${PROJECT_NAME}/"
        APP_CONF_PREFIX="conf/"
        APP_CACHE_PREFIX="cache/"
        APP_SBIN_PATH="sbin/${PROJECT_NAME}"
        APP_CONF_FILE="${PROJECT_NAME}.conf"
        APP_PID_FILE="logs/${PROJECT_NAME}.pid"
        APP_LOCK_FILE="logs/${PROJECT_NAME}.lock"
        APP_ERROR_LOG_FILE="logs/error.log"
        APP_ACCESS_LOG_FILE="logs/access.log"
        APP_POSTGRES_LOG_FILE="logs/postgres.log"
        APP_DOC_ROOT="www/"
        )

add_dependencies(${PROJECT_NAME} auto_increment_version)
# ----------------------------------------------------------------------------------------------------------------------

set(INSTALL_PATH "/etc/apostol")

add_definitions(-DAWS_PREFIX="${INSTALL_PATH}")

install(TARGETS apostol CONFIGURATIONS Release DESTINATION /usr/sbin)
install(FILES auto/apostol DESTINATION /etc/init.d PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY DESTINATION ${INSTALL_PATH})
install(DIRECTORY DESTINATION ${INSTALL_PATH}/conf)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/logs)
install(DIRECTORY doc/www/ DESTINATION ${INSTALL_PATH}/www)
install(FILES conf/apostol.conf DESTINATION ${INSTALL_PATH}/conf RENAME default.conf)
install(FILES conf/apostol.conf DESTINATION ${INSTALL_PATH})
install(CODE "execute_process(COMMAND update-rc.d apostol defaults)")
