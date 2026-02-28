# cmake/GenerateVersion.cmake
#
# Runs at BUILD time (via add_custom_target in CMakeLists.txt).
# Captures the current git commit hash and renders version.hpp.in
# into the build directory.
#
# Required variables (passed with -D from the custom target):
#   SOURCE_DIR       — path to the project root (where .git lives)
#   PROJECT_VERSION  — semver string from CMake project()
#   PROJECT_VERSION_MAJOR / _MINOR / _PATCH
#   VERSION_IN       — absolute path to cmake/version.hpp.in
#   VERSION_OUT      — absolute path for the generated header

cmake_minimum_required(VERSION 3.25)

# ── Capture git hash ──────────────────────────────────────────────────────────

set(GIT_HASH "unknown")

find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# ── Render template ───────────────────────────────────────────────────────────

configure_file("${VERSION_IN}" "${VERSION_OUT}" @ONLY)

message(STATUS "apostol version: ${PROJECT_VERSION}+${GIT_HASH}")
