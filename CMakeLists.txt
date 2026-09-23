cmake_minimum_required(VERSION 3.21)
project(Cerverus C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # clangd's compile_commands.json, always fresh

# Go over these these and learn what exactly they all do.
# Flags from the old .mak files
add_compile_options(-g)
add_compile_definitions(_POSIX_C_SOURCE=200809L _GNU_SOURCE _DEBUG)
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fdeclspec)     # clang-only; guarded so gcc also works
endif()

# webserver (was Makefile.webserver.linux.mak)
file(GLOB_RECURSE WEBSERVER_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/webserver/src/*.c")
add_library(webserver SHARED ${WEBSERVER_SOURCES})
target_include_directories(webserver PUBLIC webserver/src)

# liburing, detected at configure time (was: bare -luring)
find_package(PkgConfig REQUIRED)
pkg_check_modules(URING REQUIRED IMPORTED_TARGET liburing)

# project (was Makefile.project.linux.mak)
add_executable(project project/src/main.c)
target_link_libraries(project PRIVATE webserver PkgConfig::URING)

# $ORIGIN rpath only: binary finds libwebserver.so next to itself (works in build/ and bin/)
set_target_properties(project PROPERTIES BUILD_RPATH "$ORIGIN")

# post-build.sh equivalent: stage runtime files into bin/ (binaries + assets only)
set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/bin)
add_custom_command(TARGET project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:project> $<TARGET_FILE:webserver> ${RUNTIME_DIR}/
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${RUNTIME_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets ${RUNTIME_DIR}/assets
    COMMENT "Staging runtime files into bin/")