cmake_minimum_required(VERSION 3.13)
project(apostol)

add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

find_library(GCC_LIBRARY libgcc.a)
find_library(GCC++_LIBRARY libgcc++.a)

ADD_DEFINITIONS(-static-libgcc)

set(PROJECT_NAME "apostol")
set(PROJECT_DESCRIPTION "Apostol Web Service")

message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")
message(STATUS "PROJECT_DESCRIPTION = ${PROJECT_DESCRIPTION}")

#add_definitions(-D_UNICODE)

add_definitions(-DDELPHI_LIB_EXPORTS)
add_definitions(-DSYS_ERRNO_COUNT=135)
add_definitions(-DAPOSTOL_NAME="${PROJECT_NAME}")
add_definitions(-DAPOSTOL_DESCRIPTION="${PROJECT_DESCRIPTION}")
add_definitions(-Dapplication_name="'${PROJECT_DESCRIPTION}'")

add_definitions(-DWWWServerName="${PROJECT_DESCRIPTION}")

find_package(PostgreSQL REQUIRED)

include_directories(src/apostol src/lib/delphi src/mod)

file(GLOB apostol_files version.h src/apostol/*.hpp src/apostol/*.cpp)
file(GLOB lib_files src/lib/*/*.hpp src/lib/*/*.cpp )
file(GLOB mod_files src/mod/Modules.* src/mod/*/*.hpp src/mod/*/*.cpp )

add_executable(apostol ${apostol_files} ${lib_files} ${mod_files})

ADD_CUSTOM_TARGET(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)
ADD_DEPENDENCIES(apostol auto_increment_version)

target_link_libraries(apostol pthread pq)

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
