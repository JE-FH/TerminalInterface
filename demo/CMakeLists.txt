cmake_minimum_required(VERSION 3.19)

project (TerminalInterface_demo C)


add_executable(TerminalInterface_demo
	"main.c"
)

target_link_libraries(TerminalInterface_demo PUBLIC TerminalInterface_static)
