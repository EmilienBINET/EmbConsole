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

        FILE* TerminalWindows::m_pStdConsoleFile{ NULL };

        TerminalWindows::TerminalWindows::TerminalWindows(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : TerminalAnsi{ a_rConsoleSession } {
        }
        //TerminalWindows::TerminalWindows(TerminalWindows const&) noexcept = default;
        //TerminalWindows::TerminalWindows(TerminalWindows&&) noexcept = default;
        TerminalWindows::~TerminalWindows() noexcept = default;
        //TerminalWindows& TerminalWindows::operator= (TerminalWindows const&) noexcept = default;
        //TerminalWindows& TerminalWindows::operator= (TerminalWindows&&) noexcept = default;

        void TerminalWindows::createStdConsole() {
            if (!m_pStdConsoleFile && AllocConsole()) {
                freopen_s(&m_pStdConsoleFile, "CONOUT$", "w", stdout);
            }
        }

        void TerminalWindows::destroyStdConsole() {
            if (m_pStdConsoleFile) {
                fclose(m_pStdConsoleFile);
                m_pStdConsoleFile = NULL;
                FreeConsole();
            }
        }

        bool TerminalWindows::isCreatable() {
            DWORD ulDummy{ 0 };
            return TRUE == GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &ulDummy);
        }

        bool TerminalWindows::isSupported() {
            bool bRes{ false };

            HANDLE hConsoleIn{ GetStdHandle(STD_INPUT_HANDLE) };
            DWORD ulBackupIn{ 0 };

            if (TRUE == GetConsoleMode(hConsoleIn, &ulBackupIn)) {
                if (ENABLE_VIRTUAL_TERMINAL_INPUT == (ulBackupIn & ENABLE_VIRTUAL_TERMINAL_INPUT)) {
                    // Virtual terminal already enabled => supported
                    bRes = true;
                }
                else {
                    // Virtual terminal not already enables => we need to try to enable it
                    if (TRUE == SetConsoleMode(hConsoleIn, ulBackupIn | ENABLE_VIRTUAL_TERMINAL_INPUT)) {
                        bRes = true;
                        SetConsoleMode(hConsoleIn, ulBackupIn);
                    }
                }
            }

            return bRes;
        }

        void TerminalWindows::start() noexcept {
            HANDLE hConsoleIn{ GetStdHandle(STD_INPUT_HANDLE) };
            HANDLE hConsoleOut{ GetStdHandle(STD_OUTPUT_HANDLE) };
            GetConsoleMode(hConsoleIn, &m_ulPreviousInputMode);
            SetConsoleMode(hConsoleIn,
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
            GetConsoleMode(hConsoleOut, &m_ulPreviousOutputMode);
            DWORD ulOutputMode =
                ENABLE_PROCESSED_OUTPUT |
                //ENABLE_WRAP_AT_EOL_OUTPUT |
                ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                //DISABLE_NEWLINE_AUTO_RETURN |
                //ENABLE_LVB_GRID_WORLDWIDE |
                0;
            SetConsoleMode(hConsoleOut, ulOutputMode);
            ConsoleSessionWithTerminal::setCaptureEndEvt([this, ulOutputMode] {
                SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ulOutputMode);
                requestTerminalSize();
            });

            TerminalAnsi::start();
            m_bStopThread = false;
            m_InputThread = std::thread{ &TerminalWindows::inputLoop, this };
        }

        void TerminalWindows::processEvents() noexcept {
            processPrintCommands();
            processUserCommands();

            if (m_bSizeChanged) {
                m_bSizeChanged = false;
                onTerminalSizeChanged();
            }
        }

        void TerminalWindows::stop() noexcept {
            TerminalAnsi::stop();
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), m_ulPreviousInputMode);
            SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), m_ulPreviousOutputMode);

            m_bStopThread = true;
            // We send a fake event to unblock the ReadConsoleInput function
            {
                INPUT_RECORD irInBuf{};
                irInBuf.EventType = FOCUS_EVENT; // That event is discarded later
                DWORD wNumberOfEventsWritten{ 0 };
                WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &irInBuf, 1, &wNumberOfEventsWritten);
            }
            m_InputThread.join();
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
            if (hStdin != INVALID_HANDLE_VALUE) {
                DWORD cNumRead;
                size_t const sizeBuf{ 128 };
                INPUT_RECORD irInBuf[sizeBuf];
                while (!m_bStopThread) {
                    if (ReadConsoleInput(hStdin, irInBuf, sizeBuf, &cNumRead)) {
                        string strKeyCode{};
                        for (DWORD i = 0; i < cNumRead; ++i) {
                            switch (irInBuf[i].EventType) {
                            case KEY_EVENT: // keyboard input
                                if (TRUE == irInBuf[i].Event.KeyEvent.bKeyDown) {
                                    for (WORD j = 0; j < irInBuf[i].Event.KeyEvent.wRepeatCount; ++j) {
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
                        if (!strKeyCode.empty()) {
                            processPressedKeyCode(strKeyCode);
                        }
                    }
                }
            }
        }

        void TerminalWindows::requestTerminalSize() noexcept {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (TRUE == GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
                Size newSize{ csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
                if (getCurrentSize() != newSize) {
                    setCurrentSize(newSize);
                    m_bSizeChanged = true;
                }
            }
        }
    } // console
} // emb