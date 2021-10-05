cmake_minimum_required(VERSION 3.19)
project(TerminalInterface)

set(UTF8PROC_DOWNLOAD_VERSION "2.6.1")

file(DOWNLOAD "https://github.com/JuliaStrings/utf8proc/archive/refs/tags/v${UTF8PROC_DOWNLOAD_VERSION}.zip" "${CMAKE_BINARY_DIR}/utf8proc-${UTF8PROC_DOWNLOAD_VERSION}.zip" SHOW_PROGRESS)
file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/utf8proc-${UTF8PROC_DOWNLOAD_VERSION}.zip" DESTINATION "${CMAKE_SOURCE_DIR}/extlibs")

make_directory(extlibs)

add_subdirectory("extlibs/utf8proc-${UTF8PROC_DOWNLOAD_VERSION}")
add_subdirectory("TerminalInterface")
add_subdirectory("demo")