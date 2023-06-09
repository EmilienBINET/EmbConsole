cmake_minimum_required(VERSION 3.15)
project(EmbConsole CXX)

if(WIN32)
    set(PLATFORM_SPECIFIC_FILES
        src/impl/win/TerminalWindows.hpp
        src/impl/win/TerminalWindows.cpp
    )
endif()
if(UNIX)
    set(PLATFORM_SPECIFIC_FILES
        src/impl/unix/TerminalUnix.hpp
        src/impl/unix/TerminalUnix.cpp
        src/impl/unix/TerminalUnixSocket.hpp
        src/impl/unix/TerminalUnixSocket.cpp
    )
endif()
add_library(EmbConsole
    src/EmbConsole.cpp 
    
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
    ${PLATFORM_SPECIFIC_FILES}
)
target_include_directories(EmbConsole PUBLIC include)

target_compile_definitions(EmbConsole PUBLIC _EXPORTING)

target_compile_features(EmbConsole PRIVATE cxx_std_17)

set_target_properties(EmbConsole PROPERTIES PUBLIC_HEADER "include/EmbConsole.hpp")
install(TARGETS EmbConsole)
