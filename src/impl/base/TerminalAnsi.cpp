#include "TerminalAnsi.hpp"
#include "../ConsolePrivate.hpp"
#include "../Tools.hpp"
#include <string>
#include <unordered_map>
#if defined _MSC_VER || defined __MINGW32__
#include <io.h>
#define flockfile _lock_file
#define ftrylockfile 0; flockfile
#define funlockfile _unlock_file
#else
#include <stdio.h>
#endif

#if __cplusplus < 201402L // Before C++14, std::map with an enum as key fails
namespace std {
    template<typename E>
    struct hash {
        typedef E argument_type;
        typedef size_t result_type;
        using sfinae = typename std::enable_if<std::is_enum<E>::value>::type;
        result_type operator() (const E& e) const {
            using base_t = typename std::underlying_type<E>::type;
            return std::hash<base_t>()(static_cast<base_t>(e));
        }
    };
}
#endif

namespace emb {
    namespace console {
        using namespace std;

        static const string s_ESC{ "\033" };
        static const string s_CSI{ "\033[" };
        static const string s_OSC{ "\033]" };
        static const string s_ST{ "\033\\" };

        TerminalAnsi::TerminalAnsi(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : Terminal{ a_rConsoleSession } {
            a_rConsoleSession.setPeriodicCapture([this] {
                processCapture();
            });
        }
        //TerminalAnsi::TerminalAnsi(TerminalAnsi const&) noexcept = default;
        //TerminalAnsi::TerminalAnsi(TerminalAnsi&&) noexcept = default;
        TerminalAnsi::~TerminalAnsi() noexcept = default;
        //TerminalAnsi& TerminalAnsi::operator= (TerminalAnsi const&) noexcept = default;
        //TerminalAnsi& TerminalAnsi::operator= (TerminalAnsi&&) noexcept = default;

        void TerminalAnsi::requestTerminalSize() const noexcept {
            if (isPromptEnabled() && tryBegin()) {
                switch (m_eDSRState) {
                case DSRState::SizeRequest:
                    saveCursor();
                    moveCursorToPosition(999, 999);
                    write(s_CSI + "6n"); // Device Status Report : Report Cursor Position
                    restoreCursor();
                    commit();
                    break;
                case DSRState::PositionRequest:
                    write(s_CSI + "6n"); // Device Status Report : Report Cursor Position
                    commit();
                    break;
                }
            }
        }

        bool TerminalAnsi::parseTerminalSizeResponse(std::string& a_strResponse) noexcept {
            string const strTerminalSizeRegex{ "\x1b\\[([0-9]+);([0-9]+)R" };
            vector<string> vstrMatches;
            bool bRes = emb::tools::regex::search(vstrMatches, a_strResponse, strTerminalSizeRegex);
            if (bRes) {
                bRes = 3 == vstrMatches.size();
            }
            if (bRes) {
                switch (m_eDSRState) {
                case DSRState::PositionRequest: {
                    Position newPosition;
                    newPosition.iX = stoi(vstrMatches[2]);
                    newPosition.iY = stoi(vstrMatches[1]);
                    setCurrentCursorPosition(newPosition);
                    m_eDSRState = DSRState::SizeRequest;
                    break;
                }
                case DSRState::SizeRequest: {
                    Size newSize;
                    newSize.iWidth = stoi(vstrMatches[2]);
                    newSize.iHeight = stoi(vstrMatches[1]);
                    bool bSizeChanged = getCurrentSize() != newSize;
                    setCurrentSize(newSize);
                    if (bSizeChanged) {
                        onTerminalSizeChanged();
                    }
                    m_eDSRState = DSRState::PositionRequest;
                    break;
                }
                }
                a_strResponse = emb::tools::regex::replace(a_strResponse, strTerminalSizeRegex, "");

                if (DSRState::SizeRequest == m_eDSRState) {
                    requestTerminalSize();
                }
            }
            return bRes && a_strResponse.empty();
        }

        bool TerminalAnsi::write(std::string const& a_strDataToPrint) const noexcept {
            m_strDataToPrint += a_strDataToPrint;
            return true;
        }

        void TerminalAnsi::processCapture() const noexcept {
            bool bLockOk = 0 == ftrylockfile(stdout);
            ConsoleSessionWithTerminal::endStdCapture();
            ConsoleSessionWithTerminal::beginStdCapture();
            if (bLockOk) {
                funlockfile(stdout);
            }
        }

        void TerminalAnsi::begin() const noexcept {
            Terminal::begin();
            m_strDataToPrint.clear();
        }

        void TerminalAnsi::commit() const noexcept {
            bool bLockOk = 0 == ftrylockfile(stdout);
            ConsoleSessionWithTerminal::endStdCapture();
            fprintf(stdout, "%s", m_strDataToPrint.c_str());
            fflush(stdout);
            ConsoleSessionWithTerminal::beginStdCapture();
            if (bLockOk) {
                funlockfile(stdout);
            }
            Terminal::commit();
        }

        void TerminalAnsi::moveCursorUp(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "A");
        }

        void TerminalAnsi::moveCursorDown(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "B");
        }

        void TerminalAnsi::moveCursorForward(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "C");
        }

        void TerminalAnsi::moveCursorBackward(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "D");
        }

        void TerminalAnsi::moveCursorToNextLine(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "E");
        }

        void TerminalAnsi::moveCursorToPreviousLine(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "F");
        }

        void TerminalAnsi::moveCursorToRow(unsigned int const a_uiR) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiR) + "G");
        }

        void TerminalAnsi::moveCursorToColumn(unsigned int const a_uiC) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiC) + "d");
        }

        void TerminalAnsi::moveCursorToPosition(unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {
            if (!supportsColor()) {
                if (1 == a_uiC) {
                    write("\r");
                }
                return;
            }
            write(s_CSI + to_string(a_uiR) + ";" + to_string(a_uiC) + "H");
        }

        void TerminalAnsi::saveCursor() const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_ESC + "7");
        }

        void TerminalAnsi::restoreCursor() const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_ESC + "8");
        }

        void TerminalAnsi::setCursorBlinking(bool const a_bBlinking) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bBlinking) {
                write(s_CSI + "?12h");
            }
            else {
                write(s_CSI + "?12l");
            }
        }

        void TerminalAnsi::setCursorVisible(bool const a_bVisible) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bVisible) {
                write(s_CSI + "?25h");
            }
            else {
                write(s_CSI + "?25l");
            }
        }

        void TerminalAnsi::setCursorShape(SetCursorShape::Shape const a_eShape) const noexcept {
            if (!supportsColor()) {
                return;
            }
            switch (a_eShape)
            {
            case SetCursorShape::Shape::UserDefault:        ///< Default cursor shape configured by the user
                write(s_CSI + "0 q");
                break;
            case SetCursorShape::Shape::BlinkingBlock:      ///< Blinking block cursor shape
                write(s_CSI + "1 q");
                break;
            case SetCursorShape::Shape::SteadyBlock:        ///< Steady block cursor shape
                write(s_CSI + "2 q");
                break;
            case SetCursorShape::Shape::BlinkingUnderline:  ///< Blinking underline cursor shape
                write(s_CSI + "3 q");
                break;
            case SetCursorShape::Shape::SteadyUnderline:    ///< Steady underline cursor shape
                write(s_CSI + "4 q");
                break;
            case SetCursorShape::Shape::BlinkingBar:        ///< Blinking bar cursor shape
                write(s_CSI + "5 q");
                break;
            case SetCursorShape::Shape::SteadyBar:          ///< Steady bar cursor shape
                write(s_CSI + "6 q");
                break;
            }
        }

        void TerminalAnsi::scrollUp(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "S");
        }

        void TerminalAnsi::scrollDown(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "T");
        }

        void TerminalAnsi::insertCharacter(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "@");
        }

        void TerminalAnsi::deleteCharacter(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "P");
        }

        void TerminalAnsi::eraseCharacter(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "X");
        }

        void TerminalAnsi::insertLine(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "L");
        }

        void TerminalAnsi::deleteLine(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "M");
        }

        void TerminalAnsi::clearDisplay(ClearDisplay::Type const a_eType) const noexcept {
            if (!supportsColor()) {
                return;
            }
            switch (a_eType)
            {
            case ClearDisplay::Type::FromCursorToEnd:
                write(s_CSI + "0J");
                break;
            case ClearDisplay::Type::FromBeginningToCursor:
                write(s_CSI + "1J");
                break;
            case ClearDisplay::Type::All:
                write(s_CSI + "2J");
                break;
            }
        }

        void TerminalAnsi::clearLine(ClearLine::Type const a_eType) const noexcept {
            if (!supportsColor()) {
                return;
            }
            switch (a_eType)
            {
            case ClearLine::Type::FromCursorToEnd:
                write(s_CSI + "0K");
                break;
            case ClearLine::Type::FromBeginningToCursor:
                write(s_CSI + "1K");
                break;
            case ClearLine::Type::All:
                write(s_CSI + "2K");
                break;
            }
        }

        void TerminalAnsi::resetTextFormat() const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + "0m");
        }

        void TerminalAnsi::setColor(SetColor::Color const a_eFgColor, SetColor::Color const a_eBgColor) const noexcept {
            if (!supportsColor()) {
                return;
            }
            static const unordered_map<SetColor::Color, pair<string, string>> s_Colors{
                { SetColor::Color::Black,          make_pair("30", "40")   },
                { SetColor::Color::Red,            make_pair("31", "41")   },
                { SetColor::Color::Green,          make_pair("32", "42")   },
                { SetColor::Color::Yellow,         make_pair("33", "43")   },
                { SetColor::Color::Blue,           make_pair("34", "44")   },
                { SetColor::Color::Magenta,        make_pair("35", "45")   },
                { SetColor::Color::Cyan,           make_pair("36", "46")   },
                { SetColor::Color::White,          make_pair("37", "47")   },
                { SetColor::Color::BrightBlack,    make_pair("90", "100")  },
                { SetColor::Color::BrightRed,      make_pair("91", "101")  },
                { SetColor::Color::BrightGreen,    make_pair("92", "102")  },
                { SetColor::Color::BrightYellow,   make_pair("93", "103")  },
                { SetColor::Color::BrightBlue,     make_pair("94", "104")  },
                { SetColor::Color::BrightMagenta,  make_pair("95", "105")  },
                { SetColor::Color::BrightCyan,     make_pair("96", "106")  },
                { SetColor::Color::BrightWhite,    make_pair("97", "107")  },
                { SetColor::Color::Default,        make_pair("37", "")     },
            };
            if (!s_Colors.at(a_eBgColor).second.empty()) {
                write(s_CSI + s_Colors.at(a_eFgColor).first + ";" + s_Colors.at(a_eBgColor).second + "m");
            }
            else {
                write(s_CSI + s_Colors.at(a_eFgColor).first + "m");
            }
        }

        void TerminalAnsi::setNegativeColors(bool const a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_CSI + "7m");
            }
            else {
                write(s_CSI + "27m");
            }
        }

        void TerminalAnsi::setBold(bool const a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_CSI + "1m");
            }
            else {
                write(s_CSI + "22m");
            }
        }

        void TerminalAnsi::setItalic(bool const a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_CSI + "3m");
            }
            else {
                write(s_CSI + "23m");
            }
        }

        void TerminalAnsi::setUnderline(bool const a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_CSI + "4m");
            }
            else {
                write(s_CSI + "24m");
            }
        }

        void TerminalAnsi::setHorizontalTab() const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_ESC + "H");
        }

        void TerminalAnsi::goToHorizontalTabForward(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "I");
        }

        void TerminalAnsi::goToHorizontalTabBackward(unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiN) + "Z");
        }

        void TerminalAnsi::clearHorizontalTab(ClearHorizontalTab::Type const a_eType) const noexcept {
            if (!supportsColor()) {
                return;
            }
            switch (a_eType)
            {
            case ClearHorizontalTab::Type::CurrentColumn:
                write(s_CSI + "0g");
                break;
            case ClearHorizontalTab::Type::AllColumns:
                write(s_CSI + "3g");
                break;
            }
        }

        void TerminalAnsi::setDecCharacterSet(bool const a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_ESC + "(0");
            }
            else {
                write(s_ESC + "(B");
            }
        }

        void TerminalAnsi::printSymbol(PrintSymbol::Symbol const a_eSymbol, unsigned int const a_uiN) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (PrintSymbol::Symbol::Space == a_eSymbol) {
                write(" ");
            }
            else {
                static const unordered_map<PrintSymbol::Symbol, string> s_SymbolsMap{
                        { PrintSymbol::Symbol::BottomRight,    "j" },
                        { PrintSymbol::Symbol::TopRight,       "k" },
                        { PrintSymbol::Symbol::TopLeft,        "l" },
                        { PrintSymbol::Symbol::BottomLeft,     "m" },
                        { PrintSymbol::Symbol::Cross,          "n" },
                        { PrintSymbol::Symbol::HorizontalBar,  "q" },
                        { PrintSymbol::Symbol::LeftCross,      "t" },
                        { PrintSymbol::Symbol::RightCross,     "u" },
                        { PrintSymbol::Symbol::BottomCross,    "v" },
                        { PrintSymbol::Symbol::TopCross,       "w" },
                        { PrintSymbol::Symbol::VerticalBar,    "x" },
                };
                write(s_ESC + "(0");
                for (unsigned int i = 0; i < a_uiN; ++i) {
                    write(s_SymbolsMap.at(a_eSymbol));
                }
                write(s_ESC + "(B");
            }
        }

        void TerminalAnsi::setScrollingRegion(unsigned int const a_uiT, unsigned int const a_uiB) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + to_string(a_uiT) + ";" + to_string(a_uiB) + "r");
        }

        void TerminalAnsi::setWindowTitle(std::string const& a_strTitle) const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_OSC + "0;" + a_strTitle + s_ST);
            // or ? write(s_OSC + "2;" + a_strTitle + s_ST);
        }

        void TerminalAnsi::useAlternateScreenBuffer(bool const& a_bEnabled) const noexcept {
            if (!supportsColor()) {
                return;
            }
            if (a_bEnabled) {
                write(s_CSI + "?1049h");
            }
            else {
                write(s_CSI + "?1049l");
            }
        }

        void TerminalAnsi::softReset() const noexcept {
            if (!supportsColor()) {
                return;
            }
            write(s_CSI + "!p");
        }

        void TerminalAnsi::ringBell() const noexcept {
            write("\a");
        }

        void TerminalAnsi::printNewLine() const noexcept {
            write("\r\n");
        }

        void TerminalAnsi::printText(std::string const& a_strText) const noexcept {
            write(a_strText);
        }

        void TerminalAnsi::printTextAt(std::string const& a_strText, unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {
            assert(false);
        }

        void TerminalAnsi::processPressedKeyCode(string const& a_strKey) noexcept {
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

            if ("\x0d" == a_strKey || "\x0a" == a_strKey) {
                Terminal::processPressedKey(Key::Enter);
            }
            else if ("\x7f" == a_strKey) {
                Terminal::processPressedKey(Key::Back);
            }
            else if ("\x1b\x5b\x33\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::Del);
            }
            else if ("\x09" == a_strKey) {
                Terminal::processPressedKey(Key::Tab);
            }
            else if ("\x1b\x5b\x5a" == a_strKey) {
                Terminal::processPressedKey(Key::ReverseTab);
            }
            else if ("\x1b\x5b\x41" == a_strKey) {
                Terminal::processPressedKey(Key::Up);
            }
            else if ("\x1b\x5b\x42" == a_strKey) {
                Terminal::processPressedKey(Key::Down);
            }
            else if ("\x1b\x5b\x43" == a_strKey) {
                Terminal::processPressedKey(Key::Right);
            }
            else if ("\x1b\x5b\x44" == a_strKey) {
                Terminal::processPressedKey(Key::Left);
            }
            else if ("\x1b\x5b\x48" == a_strKey ||
                "\x1b\x5b\x31\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::Start);
            }
            else if ("\x1b\x5b\x46" == a_strKey ||
                "\x1b\x5b\x34\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::End);
            }
            else if ("\x1b\x5b\x35\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::PageUp);
            }
            else if ("\x1b\x5b\x36\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::PageDown);
            }
            else if ("\x1b\x5b\x32\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::Insert);
            }
            else if ("\x1b" == a_strKey) {
                Terminal::processPressedKey(Key::Escape);
            }
            else if ("\x1b\x4f\x50" == a_strKey ||
                "\x1b\x5b\x31\x31\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F1);
            }
            else if ("\x1b\x4f\x51" == a_strKey ||
                "\x1b\x5b\x31\x32\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F2);
            }
            else if ("\x1b\x4f\x52" == a_strKey ||
                "\x1b\x5b\x31\x33\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F3);
            }
            else if ("\x1b\x4f\x53" == a_strKey ||
                "\x1b\x5b\x31\x34\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F4);
            }
            else if ("\x1b\x5b\x31\x35\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F5);
            }
            else if ("\x1b\x5b\x31\x37\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F6);
            }
            else if ("\x1b\x5b\x31\x38\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F7);
            }
            else if ("\x1b\x5b\x31\x39\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F8);
            }
            else if ("\x1b\x5b\x32\x30\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F9);
            }
            else if ("\x1b\x5b\x32\x31\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F10);
            }
            else if ("\x1b\x5b\x32\x33\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F11);
            }
            else if ("\x1b\x5b\x32\x34\x7e" == a_strKey) {
                Terminal::processPressedKey(Key::F12);
            }
            else {
                for (char const c : a_strKey) {
                    if (stdWrapper(std::isprint, c)) {
                        Terminal::processPressedKey(Key::Printable, std::string{ c });
                    }
                    else if ('\x0d' == c || '\x0a' == c) {
                        Terminal::processPressedKey(Key::Enter);
                    }
                    else {
                        //cerr << "Unknown key pressed : " << a_strKey << " 0x" << string_to_hex(a_strKey) << endl;
                        //system("pause");
                    }
                }
            }
        }
    } // console
} // emb