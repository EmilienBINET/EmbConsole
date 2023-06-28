#include "TerminalWindows.hpp"
#include "../ConsolePrivate.hpp"
#include <iostream>
#include <conio.h>
#include <unordered_map>
#include <windows.h>

// MinGW 8.1 does not define these
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#  define ENABLE_VIRTUAL_TERMINAL_INPUT 0x200
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#  define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif

namespace emb {
    namespace console {
        using namespace std;

        TerminalWindows::TerminalWindows(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : TerminalAnsi{ a_rConsoleSession } {
        }
        //TerminalWindows::TerminalWindows(TerminalWindows const&) noexcept = default;
        //TerminalWindows::TerminalWindows(TerminalWindows&&) noexcept = default;
        TerminalWindows::~TerminalWindows() noexcept = default;
        //TerminalWindows& TerminalWindows::operator= (TerminalWindows const&) noexcept = default;
        //TerminalWindows& TerminalWindows::operator= (TerminalWindows&&) noexcept = default;

        void TerminalWindows::start() noexcept {
            GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m_ulPreviousInputMode);
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                //ENABLE_ECHO_INPUT |
                //ENABLE_INSERT_MODE |
                //ENABLE_LINE_INPUT |
                //ENABLE_MOUSE_INPUT |
                ENABLE_PROCESSED_INPUT |
                ENABLE_EXTENDED_FLAGS |
                //ENABLE_QUICK_EDIT_MODE |
                //ENABLE_WINDOW_INPUT |
                ENABLE_VIRTUAL_TERMINAL_INPUT |
                0);
            GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &m_ulPreviousOutputMode);
            DWORD ulOutputMode =
                ENABLE_PROCESSED_OUTPUT |
                //ENABLE_WRAP_AT_EOL_OUTPUT |
                ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                //DISABLE_NEWLINE_AUTO_RETURN |
                //ENABLE_LVB_GRID_WORLDWIDE |
                0;
            SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ulOutputMode);
            ConsoleSessionWithTerminal::setCaptureEndEvt([this, ulOutputMode]{
                SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ulOutputMode);
                requestTerminalSize();
            });

            TerminalAnsi::start();
            m_InputThread = std::thread{ &TerminalWindows::inputLoop, this };
        }

        void TerminalWindows::processEvents() noexcept {
            processPrintCommands();
            processUserCommands();

            if(m_bSizeChanged) {
                onTerminalSizeChanged();
            }
        }

        void TerminalWindows::stop() noexcept {
            TerminalAnsi::stop();
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), m_ulPreviousInputMode);
            SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), m_ulPreviousOutputMode);
        }

        bool TerminalWindows::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalWindows::supportsColor() const noexcept {
            return true;
        }

        bool TerminalWindows::read(std::string& a_rstrKey) const noexcept {
            return false;
        }

        void TerminalWindows::inputLoop() noexcept {
            HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
            if(hStdin != INVALID_HANDLE_VALUE) {
                DWORD cNumRead;
                size_t const sizeBuf{128};
                INPUT_RECORD irInBuf[sizeBuf];
                while(true) {
                    if(ReadConsoleInput(hStdin, irInBuf, sizeBuf, &cNumRead)) {
                        string strKeyCode{};
                        for (DWORD i=0; i<cNumRead; ++i) {
                            switch(irInBuf[i].EventType) {
                            case KEY_EVENT: // keyboard input
                                if(TRUE == irInBuf[i].Event.KeyEvent.bKeyDown) {
                                    for(WORD j=0; j<irInBuf[i].Event.KeyEvent.wRepeatCount; ++j) {
                                        strKeyCode += irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
                                    }
                                }
                                break;

                            case MOUSE_EVENT: // mouse input
                                break;

                            case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
                                processCapture();
                                break;

                            case FOCUS_EVENT:  // disregard focus events
                                break;

                            case MENU_EVENT:   // disregard menu events
                                break;

                            default:
                                break;
                            }
                        }
                        if(!strKeyCode.empty()) {
                            processPressedKeyCode(strKeyCode);
                        }
                    }
                }
            }
        }

        void TerminalWindows::requestTerminalSize() noexcept {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if(TRUE == GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
                Size newSize{ csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
                m_bSizeChanged = getCurrentSize() != newSize;
                setCurrentSize(newSize);
            }
        }

    } // console
} // emb
