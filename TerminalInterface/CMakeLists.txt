cmake_minimum_required(VERSION 3.19)

project (TerminalInterface C)

set(libsrc "queue.c" "terminal_interface.c" "Utf8Iterator.c" "windows/terminal2.c")

add_library(TerminalInterface_object OBJECT
	${libsrc}
	"terminal_interface.h"
)

target_link_libraries(TerminalInterface_object PRIVATE utf8proc)

set_property(TARGET TerminalInterface_object PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(TerminalInterface_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(TerminalInterface_static STATIC $<TARGET_OBJECTS:TerminalInterface_object>)
add_library(TerminalInterface_shared SHARED $<TARGET_OBJECTS:TerminalInterface_object>)

target_link_libraries(TerminalInterface_static PRIVATE utf8proc)

target_include_directories(TerminalInterface_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(TerminalInterface_shared PRIVATE utf8proc)

target_include_directories(TerminalInterface_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
