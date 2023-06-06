#include "TerminalWindows.hpp"
#include <iostream>
#include <conio.h>
#include <unordered_map>
#include <windows.h>

namespace emb {
    namespace console {
        using namespace std;

        TerminalWindows::TerminalWindows(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : TerminalAnsi{ a_rConsoleSession } {
        }
        TerminalWindows::TerminalWindows(TerminalWindows const&) noexcept = default;
        TerminalWindows::TerminalWindows(TerminalWindows&&) noexcept = default;
        TerminalWindows::~TerminalWindows() noexcept {
            string keys{};
            while (read(keys)) {}  // purge cin
        }
        TerminalWindows& TerminalWindows::operator= (TerminalWindows const&) noexcept = default;
        TerminalWindows& TerminalWindows::operator= (TerminalWindows&&) noexcept = default;

        void TerminalWindows::start() const noexcept {
            //    cout << "\033[1;10r";
            GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m_ulPreviousInputMode);
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                //ENABLE_ECHO_INPUT |
                //ENABLE_INSERT_MODE |
                //ENABLE_LINE_INPUT |
                ENABLE_MOUSE_INPUT |
                ENABLE_PROCESSED_INPUT |
                ENABLE_EXTENDED_FLAGS |
                //ENABLE_QUICK_EDIT_MODE |
                ENABLE_WINDOW_INPUT |
                ENABLE_VIRTUAL_TERMINAL_INPUT |
                0);
            GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &m_ulPreviousOutputMode);
            SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
                ENABLE_PROCESSED_OUTPUT |
                //ENABLE_WRAP_AT_EOL_OUTPUT |
                ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                //DISABLE_NEWLINE_AUTO_RETURN |
                //ENABLE_LVB_GRID_WORLDWIDE |
                0);
            Terminal::start();
        }

        void TerminalWindows::processEvents() noexcept {
            string keys{};
            while (read(keys)) {}
            if (!keys.empty() && !parseTerminalSizeResponse(keys)) {
                processPressedKeyCode(keys);
            }
            processPrintCommands();
            processUserCommands();
            requestTerminalSize();
        }

        void TerminalWindows::stop() const noexcept {
            const_cast<TerminalWindows*>(this)->setPromptEnabled(false);
            softReset();
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
            bool bRes = 0 != _kbhit();
            if (bRes) {
                a_rstrKey += _getch();
            }
            return bRes;
        }
    } // console
} // emb