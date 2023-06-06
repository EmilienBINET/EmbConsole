cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(WIN32)
	set(PLATFORM_SPECIFIC_FILES
		../../src/impl/win/TerminalWindows.hpp
		../../src/impl/win/TerminalWindows.cpp
	)
endif()
if(UNIX)
	set(PLATFORM_SPECIFIC_FILES
		../../src/impl/unix/TerminalUnix.hpp
		../../src/impl/unix/TerminalUnix.cpp
		../../src/impl/unix/TerminalUnixSocket.hpp
		../../src/impl/unix/TerminalUnixSocket.cpp
	)
endif()

add_executable(example
	../src/example.cpp
	
	../../include/EmbConsole.hpp
	../../src/EmbConsole.cpp
	
	../../src/impl/Console.cpp
	../../src/impl/ConsolePrivate.hpp
	../../src/impl/ConsolePrivate.cpp
	../../src/impl/Functions.hpp
	../../src/impl/Functions.cpp
	../../src/impl/Options.cpp
	../../src/impl/ConsolePrintCommand.cpp
	../../src/impl/ConsolePromptCommand.cpp
	../../src/impl/base/ITerminal.hpp
	../../src/impl/base/ITerminal.cpp
	../../src/impl/base/Terminal.hpp
	../../src/impl/base/Terminal.cpp
	../../src/impl/base/TerminalAnsi.hpp
	../../src/impl/base/TerminalAnsi.cpp
	${PLATFORM_SPECIFIC_FILES}
)
target_include_directories(example PRIVATE ../../include)
target_compile_definitions(example PRIVATE STATIC)
target_link_libraries(example Threads::Threads)

if(WIN32)
    add_definitions(-DUSE_VLD)
    target_include_directories(example PRIVATE "C:/Program Files (x86)/Visual Leak Detector/include")
    target_link_directories(example PRIVATE "C:/Program Files (x86)/Visual Leak Detector/lib/Win64")
endif()
