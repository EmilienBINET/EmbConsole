#include "TerminalWindowsLegacy.hpp"
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

        TerminalWindowsLegacy::TerminalWindowsLegacy(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : Terminal{ a_rConsoleSession } {
        }
        //TerminalWindowsLegacy::TerminalWindowsLegacy(TerminalWindowsLegacy const&) noexcept = default;
        //TerminalWindowsLegacy::TerminalWindowsLegacy(TerminalWindowsLegacy&&) noexcept = default;
        TerminalWindowsLegacy::~TerminalWindowsLegacy() noexcept = default;
        //TerminalWindowsLegacy& TerminalWindowsLegacy::operator= (TerminalWindowsLegacy const&) noexcept = default;
        //TerminalWindowsLegacy& TerminalWindowsLegacy::operator= (TerminalWindowsLegacy&&) noexcept = default;

        void TerminalWindowsLegacy::processEvents() noexcept {
            std::string strKeys{};
            std::string strKey{};
            while (read(strKey)) {
                strKeys += strKey;
            }
            if (!strKeys.empty()) {
                processPressedKeyCode(strKeys);
            }
            processPrintCommands();
            processUserCommands();
        }

        bool TerminalWindowsLegacy::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalWindowsLegacy::supportsColor() const noexcept {
            return true;
        }

        bool TerminalWindowsLegacy::read(std::string& a_rstrKey) const noexcept {
            if (_kbhit()) {
                a_rstrKey = _getch();
                return true;
            }
            return false;
        }

        bool TerminalWindowsLegacy::write(std::string const& a_strDataToPrint) const noexcept {
            std::cout << a_strDataToPrint;
            std::cout.flush();
            return false;
        }

        WORD getAttributes(SetColor::Color const a_eFgColor, SetColor::Color const a_eBgColor)
        {
            WORD wAttributes = 0;
            switch (a_eFgColor)
            {
            case SetColor::Color::Black:        wAttributes |= 0; break;
            case SetColor::Color::Red:          wAttributes |= FOREGROUND_RED; break;
            case SetColor::Color::Green:        wAttributes |= FOREGROUND_GREEN; break;
            case SetColor::Color::Yellow:       wAttributes |= FOREGROUND_RED | FOREGROUND_GREEN; break;
            case SetColor::Color::Blue:         wAttributes |= FOREGROUND_BLUE; break;
            case SetColor::Color::Magenta:      wAttributes |= FOREGROUND_RED | FOREGROUND_BLUE; break;
            case SetColor::Color::Cyan:         wAttributes |= FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            case SetColor::Color::White:        wAttributes |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;

            case SetColor::Color::BrightBlack:  wAttributes |= FOREGROUND_INTENSITY; break;
            case SetColor::Color::BrightRed:    wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_RED; break;
            case SetColor::Color::BrightGreen:  wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_GREEN; break;
            case SetColor::Color::BrightYellow: wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN; break;
            case SetColor::Color::BrightBlue:   wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_BLUE; break;
            case SetColor::Color::BrightMagenta:wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE; break;
            case SetColor::Color::BrightCyan:   wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            case SetColor::Color::BrightWhite:  wAttributes |= FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            }
            switch (a_eBgColor)
            {
            case SetColor::Color::Black:        wAttributes |= 0; break;
            case SetColor::Color::Red:          wAttributes |= BACKGROUND_RED; break;
            case SetColor::Color::Green:        wAttributes |= BACKGROUND_GREEN; break;
            case SetColor::Color::Yellow:       wAttributes |= BACKGROUND_RED | BACKGROUND_GREEN; break;
            case SetColor::Color::Blue:         wAttributes |= BACKGROUND_BLUE; break;
            case SetColor::Color::Magenta:      wAttributes |= BACKGROUND_RED | BACKGROUND_BLUE; break;
            case SetColor::Color::Cyan:         wAttributes |= BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case SetColor::Color::White:        wAttributes |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;

            case SetColor::Color::BrightBlack:  wAttributes |= BACKGROUND_INTENSITY; break;
            case SetColor::Color::BrightRed:    wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_RED; break;
            case SetColor::Color::BrightGreen:  wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_GREEN; break;
            case SetColor::Color::BrightYellow: wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN; break;
            case SetColor::Color::BrightBlue:   wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_BLUE; break;
            case SetColor::Color::BrightMagenta:wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE; break;
            case SetColor::Color::BrightCyan:   wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case SetColor::Color::BrightWhite:  wAttributes |= BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            }
            return wAttributes;
        }

        void TerminalWindowsLegacy::begin() const noexcept {}
        void TerminalWindowsLegacy::commit() const noexcept {}
        void TerminalWindowsLegacy::moveCursorUp(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorDown(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorForward(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorBackward(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorToNextLine(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorToPreviousLine(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::moveCursorToRow(unsigned int const a_uiR) const noexcept {}
        void TerminalWindowsLegacy::moveCursorToColumn(unsigned int const a_uiC) const noexcept {}
        void TerminalWindowsLegacy::moveCursorToPosition(unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {
            COORD pos = { a_uiC - 1, a_uiR - 1 };
            HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO info{};
            GetConsoleScreenBufferInfo(output, &info);
            if (pos.X < 0) {
                pos.X = 0;
            }
            else if (pos.X > info.srWindow.Right) {
                pos.X = info.srWindow.Right;
            }
            if (pos.Y < 0) {
                pos.Y = 0;
            }
            else if (pos.Y > info.srWindow.Bottom) {
                pos.Y = info.srWindow.Bottom;
            }
            SetConsoleCursorPosition(output, pos);
        }
        void TerminalWindowsLegacy::saveCursor() const noexcept {
            CONSOLE_SCREEN_BUFFER_INFO info{};
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
            wCursorPositionX = info.dwCursorPosition.X;
            wCursorPositionY = info.dwCursorPosition.Y;
        }
        void TerminalWindowsLegacy::restoreCursor() const noexcept {
            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { wCursorPositionX, wCursorPositionY });
        }
        void TerminalWindowsLegacy::setCursorBlinking(bool const a_bBlinking) const noexcept {}
        void TerminalWindowsLegacy::setCursorVisible(bool const a_bVisible) const noexcept {
            HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_CURSOR_INFO info{};
            GetConsoleCursorInfo(output, &info);
            info.bVisible = a_bVisible ? TRUE : FALSE;
            SetConsoleCursorInfo(output, &info);
        }
        void TerminalWindowsLegacy::setCursorShape(SetCursorShape::Shape const a_eShape) const noexcept {}
        void TerminalWindowsLegacy::scrollUp(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::scrollDown(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::insertCharacter(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::deleteCharacter(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::eraseCharacter(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::insertLine(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::deleteLine(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::clearDisplay(ClearDisplay::Type const a_eType) const noexcept {
            DWORD cCharsWritten;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

            // Get the number of character cells in the current buffer.
            if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
                return;
            }

            switch (a_eType)
            {
            case ClearDisplay::Type::FromCursorToEnd:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                    (csbi.dwSize.Y - csbi.dwCursorPosition.Y + 1) * csbi.dwSize.X + csbi.dwCursorPosition.X,       // Number of cells to write
                    csbi.dwCursorPosition,     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            case ClearDisplay::Type::FromBeginningToCursor:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                     csbi.dwCursorPosition.Y * csbi.dwSize.X + csbi.dwCursorPosition.X + 1,       // Number of cells to write
                    COORD{ 0, 0 },     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            case ClearDisplay::Type::All:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                    csbi.dwSize.X * csbi.dwSize.Y,       // Number of cells to write
                    COORD{ 0, 0 },     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            }
        }
        void TerminalWindowsLegacy::clearLine(ClearLine::Type const a_eType) const noexcept {
            DWORD cCharsWritten;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

            // Get the number of character cells in the current buffer.
            if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
                return;
            }

            switch (a_eType)
            {
            case ClearLine::Type::FromCursorToEnd:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                    csbi.dwSize.X - csbi.dwCursorPosition.X,       // Number of cells to write
                    csbi.dwCursorPosition,     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            case ClearLine::Type::FromBeginningToCursor:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                    csbi.dwCursorPosition.X + 1,       // Number of cells to write
                    COORD{ 0, csbi.dwCursorPosition.Y },     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            case ClearLine::Type::All:
                FillConsoleOutputCharacter(
                    hConsole,        // Handle to console screen buffer
                    (TCHAR)' ',      // Character to write to the buffer
                    csbi.dwSize.X,       // Number of cells to write
                    COORD{ 0, csbi.dwCursorPosition.Y },     // Coordinates of first cell
                    &cCharsWritten   // Receive number of characters written
                );
                break;
            }
        }
        void TerminalWindowsLegacy::resetTextFormat() const noexcept {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, getAttributes(SetColor::Color::White, SetColor::Color::Black));
        }

        void TerminalWindowsLegacy::setColor(SetColor::Color const a_eFgColor, SetColor::Color const a_eBgColor) const noexcept {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, getAttributes(a_eFgColor, a_eBgColor));
        }

        void TerminalWindowsLegacy::setNegativeColors(bool const a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::setBold(bool const a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::setItalic(bool const a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::setUnderline(bool const a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::setHorizontalTab() const noexcept {}
        void TerminalWindowsLegacy::goToHorizontalTabForward(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::goToHorizontalTabBackward(unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::clearHorizontalTab(ClearHorizontalTab::Type const a_eType) const noexcept {}
        void TerminalWindowsLegacy::setDecCharacterSet(bool const a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::printSymbol(PrintSymbol::Symbol const a_eSymbol, unsigned int const a_uiN) const noexcept {}
        void TerminalWindowsLegacy::setScrollingRegion(unsigned int const a_uiT, unsigned int const a_uiB) const noexcept {}
        void TerminalWindowsLegacy::setWindowTitle(std::string const& a_strTitle) const noexcept {}
        void TerminalWindowsLegacy::useAlternateScreenBuffer(bool const& a_bEnabled) const noexcept {}
        void TerminalWindowsLegacy::softReset() const noexcept {}
        void TerminalWindowsLegacy::ringBell() const noexcept {}
        void TerminalWindowsLegacy::printNewLine() const noexcept {
            write("\n");
        }
        void TerminalWindowsLegacy::printText(std::string const& a_strText) const noexcept {
            write(a_strText);
        }
        void TerminalWindowsLegacy::printTextAt(std::string const& a_strText, unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {}

        void TerminalWindowsLegacy::processPressedKeyCode(std::string const& a_strKey) noexcept {
            /*auto string_to_hex = [](const std::string& input) {
                static const char hex_digits[] = "0123456789ABCDEF";
                std::string output;
                output.reserve(input.length() * 2);
                for (unsigned char c : input)
                {
                    output.push_back(hex_digits[c >> 4]);
                    output.push_back(hex_digits[c & 15]);
                }
                return output;
            };*/

            auto stdWrapper = [](int(&stdFunction)(int), int const& param) {
                bool bRes = false;
                try {
                    bRes = 0 != stdFunction(static_cast<unsigned char>(param));
                }
                catch (...) {
                }
                return bRes;
            };

            if ("\x0d" == a_strKey) {
                Terminal::processPressedKey(Key::Enter);
            }
            else if ("\x08" == a_strKey || "\x7f" == a_strKey) {
                Terminal::processPressedKey(Key::Back);
            }
            else if (std::string{ "\xe0\x53", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::Del);
            }
            else if ("\x09" == a_strKey) {
                Terminal::processPressedKey(Key::Tab);
            }
            //else if ("\x1b\x5b\x5a" == a_strKey) {
            //    Terminal::processPressedKey(Key::ReverseTab);
            //}
            else if ("\xe0\x48" == a_strKey) {
                Terminal::processPressedKey(Key::Up);
            }
            else if ("\xe0\x50" == a_strKey) {
                Terminal::processPressedKey(Key::Down);
            }
            else if ("\xe0\x4d" == a_strKey) {
                Terminal::processPressedKey(Key::Right);
            }
            else if ("\xe0\x4b" == a_strKey) {
                Terminal::processPressedKey(Key::Left);
            }
            else if ("\xe0\x47" == a_strKey) {
                Terminal::processPressedKey(Key::Start);
            }
            else if ("\xe0\x4f" == a_strKey) {
                Terminal::processPressedKey(Key::End);
            }
            else if ("\xe0\x49" == a_strKey) {
                Terminal::processPressedKey(Key::PageUp);
            }
            else if ("\xe0\x51" == a_strKey) {
                Terminal::processPressedKey(Key::PageDown);
            }
            else if ("\xe0\x52" == a_strKey) {
                Terminal::processPressedKey(Key::Insert);
            }
            else if ("\x1b" == a_strKey) {
                Terminal::processPressedKey(Key::Escape);
            }
            else if (std::string{ "\x00\x3b", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F1);
            }
            else if (std::string{ "\x00\x3c", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F2);
            }
            else if (std::string{ "\x00\x3d", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F3);
            }
            else if (std::string{ "\x00\x3e", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F4);
            }
            else if (std::string{ "\x00\x3f", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F5);
            }
            else if (std::string{ "\x00\x40", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F6);
            }
            else if (std::string{ "\x00\x41", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F7);
            }
            else if (std::string{ "\x00\x42", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F8);
            }
            else if (std::string{ "\x00\x43", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F9);
            }
            else if (std::string{ "\x00\x44", 2 } == a_strKey) {
                Terminal::processPressedKey(Key::F10);
            }
            else if ("\xe0\x85" == a_strKey) {
                Terminal::processPressedKey(Key::F11);
            }
            else if ("\xe0\x86" == a_strKey) {
                Terminal::processPressedKey(Key::F12);
            }
            else if (1 == a_strKey.size() && stdWrapper(std::isprint, a_strKey.at(0))) {
                Terminal::processPressedKey(Key::Printable, a_strKey);
            }
            else {
                //cerr << "Unknown key pressed : " << a_strKey << " 0x" << string_to_hex(a_strKey) << endl;
                //system("pause");
            }
        }
    } // console
} // emb
