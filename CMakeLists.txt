cmake_minimum_required(VERSION 3.13)
project(apostol)

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-modules" )

add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

set(PROJECT_NAME "apostol")
set(PROJECT_DESCRIPTION "Apostol Web Service")

message(STATUS "Project name: ${PROJECT_NAME}")
message(STATUS "Project description: ${PROJECT_DESCRIPTION}")

set(INSTALL_LOCAL   OFF CACHE BOOL "Install local")

set(USE_POSTGRESQL  OFF CACHE BOOL "Build with PostgreSQL")
set(USE_SQLITE3     OFF CACHE BOOL "Build with Sqlite3.")

if (INSTALL_LOCAL)
    set(INSTALL_BIN_PATH "/usr/local/sbin")
    set(PROJECT_PREFIX "~/.${PROJECT_NAME}")
else()
    set(INSTALL_BIN_PATH "/usr/sbin")
    set(PROJECT_PREFIX "/etc/${PROJECT_NAME}")
endif()

if (USE_POSTGRESQL)
    message(STATUS "Using PostgreSQL.")
    find_package(PostgreSQL REQUIRED)
    set(PQ_LIB_NAME "pq")
    add_compile_options("-DDELPHI_POSTGRESQL")
endif()

if (USE_SQLITE3)
    message(STATUS "Using SQLite3.")
    set(SQLITE3_LIB_NAME "sqlite3")
    add_compile_options("-DDELPHI_SQLITE3")
endif()

set(DELPHI_LIB_NAME delphi)
set(CORE_LIB_NAME apostol-core)
set(MODULES_LIB_NAME apostol-modules)

#find_library(GCC_LIBRARY libgcc.a)
#find_library(GCC++_LIBRARY libgcc++.a)

#add_definitions(-static-libgcc)
#add_definitions(-D_UNICODE)

add_custom_target(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)

# Find PkgConfig
#------------------------------------------------------------------------------
find_package( PkgConfig REQUIRED )

set( prefix "${CMAKE_PREFIX_PATH}" )
set( exec_prefix "\${prefix}" )
set( libdir "\${exec_prefix}/lib" )
set( includedir "\${exec_prefix}/include" )

set( pkgconfigdir "${libdir}/pkgconfig" CACHE PATH "Path to pkgconfig directory." )

# Find dl
#------------------------------------------------------------------------------
if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    find_package( Dl REQUIRED )
endif()

# Define project common includes directories
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    include_directories( SYSTEM
            ${dl_INCLUDE_DIRS} )
else()
    include_directories( SYSTEM
            ${dl_INCLUDE_DIRS} )
endif()

# Define project common library directories
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    link_directories(
            ${dl_LIBRARY_DIRS} )
else()
    link_directories(
            ${dl_LIBRARY_DIRS} )
endif()

# Define project common libraries/linker flags.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    link_libraries(
            "-fstack-protector"
            "-fstack-protector-all"
            ${dl_LIBRARIES} )
else()
    link_libraries(
            "-fstack-protector"
            "-fstack-protector-all"
            ${dl_LIBRARIES} )
endif()

# Apostol
# ----------------------------------------------------------------------------------------------------------------------
include_directories(src/lib/delphi src/${PROJECT_NAME} src/core src/modules)

file(GLOB project_files version.h src/${PROJECT_NAME}/*.hpp src/${PROJECT_NAME}/*.cpp)
file(GLOB delphi_files src/lib/delphi/*.hpp src/lib/delphi/*.cpp)
file(GLOB core_files src/core/*.hpp src/core/*.cpp)
file(GLOB modules_files src/modules/Modules.hpp src/modules/*/*.hpp src/modules/*/*.cpp)

# Delphi classes for C++
# ----------------------------------------------------------------------------------------------------------------------
add_library(${DELPHI_LIB_NAME} STATIC ${delphi_files})
target_link_libraries(${DELPHI_LIB_NAME} pthread ${SQLITE3_LIB_NAME} ${PQ_LIB_NAME})
target_compile_definitions(${DELPHI_LIB_NAME} PUBLIC WWWServerName="${PROJECT_DESCRIPTION}")

add_definitions(-DDELPHI_LIB_EXPORTS)
add_definitions(-DSYS_ERRNO_COUNT=135)

# Apostol Core
# ----------------------------------------------------------------------------------------------------------------------
add_library(${CORE_LIB_NAME} STATIC ${core_files})

target_compile_definitions(${CORE_LIB_NAME} PUBLIC
        APP_NAME="${PROJECT_NAME}"
        APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
        APP_DEFAULT_LOCALE="en_US.UTF-8"
        APP_VAR="${PROJECT_NAME}"
        APP_OLDPID_EXT=".oldbin"
        APP_DEFAULT_USER="nobody"
        APP_DEFAULT_GROUP="nogroup"
        APP_PREFIX="${PROJECT_PREFIX}/"
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

target_link_libraries(${CORE_LIB_NAME} ${DELPHI_LIB_NAME})

# Apostol modules
# ----------------------------------------------------------------------------------------------------------------------
add_library(${MODULES_LIB_NAME} STATIC ${modules_files})
target_link_libraries(${MODULES_LIB_NAME} ${CORE_LIB_NAME})

# Apostol Bitcoin
# ----------------------------------------------------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${project_files})
target_link_libraries(${PROJECT_NAME} ${MODULES_LIB_NAME})

add_dependencies(${PROJECT_NAME} auto_increment_version)
# ----------------------------------------------------------------------------------------------------------------------

set(INSTALL_PATH "${PROJECT_PREFIX}")

install(TARGETS ${PROJECT_NAME} CONFIGURATIONS Release DESTINATION ${INSTALL_BIN_PATH})
install(FILES auto/${PROJECT_NAME} DESTINATION /etc/init.d PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY DESTINATION ${INSTALL_PATH})
install(DIRECTORY DESTINATION ${INSTALL_PATH}/conf)
install(DIRECTORY DESTINATION ${INSTALL_PATH}/logs)
install(DIRECTORY doc/www/ DESTINATION ${INSTALL_PATH}/www)
install(FILES conf/${PROJECT_NAME}.conf DESTINATION ${INSTALL_PATH}/conf RENAME default.conf)
install(FILES conf/${PROJECT_NAME}.conf DESTINATION ${INSTALL_PATH})
install(CODE "execute_process(COMMAND update-rc.d ${PROJECT_NAME} defaults)")
