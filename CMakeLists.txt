cmake_minimum_required(VERSION 3.15)
project(EmbConsole CXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 4.9)
    set(NEED_PCRE2 ON)
endif()

if(NEED_PCRE2)
    set(PCRE2_SHOW_REPORT           OFF CACHE BOOL "Show the final configuration report")
    set(PCRE2_SUPPORT_LIBBZ2        OFF CACHE BOOL "Enable support for linking pcre2grep with libbz2.")
    set(PCRE2_SUPPORT_LIBZ          OFF CACHE BOOL "Enable support for linking pcre2grep with libz.")
    set(PCRE2_SUPPORT_LIBREADLINE   OFF CACHE BOOL "Enable support for linking pcre2test with libreadline.")
    set(PCRE2_SUPPORT_LIBEDIT       OFF CACHE BOOL "Enable support for linking pcre2test with libedit.")
    set(PCRE2_BUILD_PCRE2GREP       OFF CACHE BOOL "Build pcre2grep")
    set(PCRE2_BUILD_TESTS           OFF CACHE BOOL "Build the tests")
    add_subdirectory(third/pcre2)
endif()

if(WIN32)
    set(PLATFORM_SPECIFIC_FILES
        src/impl/win/TerminalWindows.hpp
        src/impl/win/TerminalWindows.cpp
        src/impl/win/TerminalWindowsLegacy.hpp
        src/impl/win/TerminalWindowsLegacy.cpp
    )
endif()
if(UNIX)
    set(PLATFORM_SPECIFIC_FILES
        src/impl/unix/TerminalUnix.hpp
        src/impl/unix/TerminalUnix.cpp
        src/impl/unix/TerminalUnixSocket.hpp
        src/impl/unix/TerminalUnixSocket.cpp
    )
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
endif()
add_library(EmbConsole
    src/EmbConsole.cpp

	src/impl/Tools.hpp
	src/impl/Tools.cpp

    src/impl/Console.cpp
    src/impl/ConsolePrivate.hpp
    src/impl/ConsolePrivate.cpp
    src/impl/Functions.hpp
    src/impl/Functions.cpp
    src/impl/Options.cpp
    src/impl/StdCapture.hpp
    src/impl/StdCapture.cpp
    src/impl/ConsolePrintCommand.cpp
    src/impl/ConsolePromptCommand.cpp
    src/impl/base/ITerminal.hpp
    src/impl/base/ITerminal.cpp
    src/impl/base/Terminal.hpp
    src/impl/base/Terminal.cpp
    src/impl/base/TerminalAnsi.hpp
    src/impl/base/TerminalAnsi.cpp
    src/impl/base/TerminalFile.hpp
    src/impl/base/TerminalFile.cpp
    src/impl/base/TerminalSyslog.hpp
    src/impl/base/TerminalSyslog.cpp
    src/impl/base/TerminalLocalTcp.hpp
    src/impl/base/TerminalLocalTcp.cpp
    ${PLATFORM_SPECIFIC_FILES}
)
target_include_directories(EmbConsole PUBLIC include)
target_include_directories(EmbConsole PRIVATE third/gulrak-filesystem/include)

target_compile_definitions(EmbConsole PUBLIC _EXPORTING)

target_compile_features(EmbConsole PRIVATE cxx_std_14)

set_target_properties(EmbConsole PROPERTIES PUBLIC_HEADER "include/EmbConsole.hpp")
#install(TARGETS EmbConsole)

if(MSVC)
    target_compile_options(EmbConsole PUBLIC "/Zc:__cplusplus")
endif()
if(UNIX)
    target_link_libraries(EmbConsole Threads::Threads)
    target_link_libraries(EmbConsole stdc++fs)
endif()
if(WIN32)
	target_link_libraries(EmbConsole Ws2_32)
endif()

if(NEED_PCRE2)
    target_link_libraries(EmbConsole pcre2-posix-static)
    target_compile_definitions(EmbConsole PRIVATE "_USE_PCRE2")
endif()
