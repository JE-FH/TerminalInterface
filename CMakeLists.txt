cmake_minimum_required(VERSION 3.19)
project(TerminalInterface)

set(UTF8PROC_DOWNLOAD_VERSION "2.6.1")

make_directory(extlibs)

add_subdirectory("utf8proc")
add_subdirectory("TerminalInterface")
add_subdirectory("demo")