#pragma once

#include "EmbConsole.hpp"
#include "ITerminal.hpp"
#include "../Functions.hpp"
#include <string>
#include <mutex>

namespace emb {
    namespace console {
        class ConsoleSessionWithTerminal;
        class Terminal : public ITerminal {
        public:
            struct Size {
                int iWidth{ 0 };
                int iHeight{ 0 };

                bool operator==(Size const& a_Other) const noexcept {
                    return iWidth == a_Other.iWidth && iHeight == a_Other.iHeight;
                }
                bool operator!=(Size const& a_Other) const noexcept {
                    return !(*this == a_Other);
                }
            };
            struct Position {
                int iX{ 0 };
                int iY{ 0 };

                bool operator==(Position const& a_Other) const noexcept {
                    return iX == a_Other.iX && iY == a_Other.iY;
                }
                bool operator!=(Position const& a_Other) const noexcept {
                    return !(*this == a_Other);
                }
            };

        public:
            Terminal(ConsoleSessionWithTerminal&) noexcept;
            Terminal(Terminal const&) noexcept = delete;
            Terminal(Terminal&&) noexcept = delete;
            virtual ~Terminal() noexcept;
            Terminal& operator= (Terminal const&) noexcept = delete;
            Terminal& operator= (Terminal&&) noexcept = delete;

            ConsoleSessionWithTerminal& consoleSession() const {
                return m_rConsoleSession;
            }

            void start() noexcept override;
            void stop() noexcept override;

            void setPrintCommands(PrintCommand::VPtr const& a_vpPrintCommands, bool a_bInstantPrint) noexcept;
            void setPromptCommands(PromptCommand::VPtr const& a_vpPromptCommands) noexcept;

            virtual bool supportsInteractivity() const noexcept { return false; }
            virtual bool supportsColor() const noexcept { return false; }

            //virtual bool getKeyAsync(std::string& a_rstrKey) const noexcept { return false; }
            //virtual bool getKeySync(std::string& a_rstrKey) const noexcept { return false; }

            virtual bool read(std::string& a_rstrKey) const noexcept = 0;
            virtual bool write(std::string const& a_strDataToPrint) const noexcept = 0;

            virtual bool tryBegin() const noexcept { return m_PrintMutex.try_lock(); }
            virtual void begin() const noexcept { m_PrintMutex.lock(); }
            virtual void commit() const noexcept { m_PrintMutex.unlock(); }
            virtual void moveCursorUp(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorDown(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorForward(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorBackward(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorToNextLine(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorToPreviousLine(unsigned int const a_uiN) const noexcept {}
            virtual void moveCursorToRow(unsigned int const a_uiR) const noexcept {}
            virtual void moveCursorToColumn(unsigned int const a_uiC) const noexcept {}
            virtual void moveCursorToPosition(unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {}
            virtual void saveCursor() const noexcept {}
            virtual void restoreCursor() const noexcept {}
            virtual void setCursorBlinking(bool const a_bBlinking) const noexcept {}
            virtual void setCursorVisible(bool const a_bVisible) const noexcept {}
            virtual void setCursorShape(SetCursorShape::Shape const a_eShape) const noexcept {}
            virtual void scrollUp(unsigned int const a_uiN) const noexcept {}
            virtual void scrollDown(unsigned int const a_uiN) const noexcept {}
            virtual void insertCharacter(unsigned int const a_uiN) const noexcept {}
            virtual void deleteCharacter(unsigned int const a_uiN) const noexcept {}
            virtual void eraseCharacter(unsigned int const a_uiN) const noexcept {}
            virtual void insertLine(unsigned int const a_uiN) const noexcept {}
            virtual void deleteLine(unsigned int const a_uiN) const noexcept {}
            virtual void clearDisplay(ClearDisplay::Type const a_eType) const noexcept {}
            virtual void clearLine(ClearLine::Type const a_eType) const noexcept {}
            virtual void resetTextFormat() const noexcept {}
            virtual void setColor(SetColor::Color const a_eFgColor, SetColor::Color const a_eBgColor) const noexcept {}
            virtual void setNegativeColors(bool const a_bEnabled) const noexcept {}
            virtual void setBold(bool const a_bEnabled) const noexcept {}
            virtual void setItalic(bool const a_bEnabled) const noexcept {}
            virtual void setUnderline(bool const a_bEnabled) const noexcept {}
            virtual void setHorizontalTab() const noexcept {}
            virtual void goToHorizontalTabForward(unsigned int const a_uiN) const noexcept {}
            virtual void goToHorizontalTabBackward(unsigned int const a_uiN) const noexcept {}
            virtual void clearHorizontalTab(ClearHorizontalTab::Type const a_eType) const noexcept {}
            virtual void setDecCharacterSet(bool const a_bEnabled) const noexcept {}
            virtual void printSymbol(PrintSymbol::Symbol const a_eSymbol, unsigned int const a_uiN) const noexcept {}
            virtual void setScrollingRegion(unsigned int const a_uiT, unsigned int const a_uiB) const noexcept {}
            virtual void setWindowTitle(std::string const& a_strTitle) const noexcept {}
            virtual void useAlternateScreenBuffer(bool const& a_bEnabled) const noexcept {}
            virtual void ringBell() const noexcept {}
            virtual void printNewLine() const noexcept {}
            virtual void printText(std::string const& a_strText) const noexcept {}
            virtual void printTextAt(std::string const& a_strText, unsigned int const a_uiR, unsigned int const a_uiC) const noexcept {}

            std::string getCurrentPath() const noexcept { std::lock_guard<std::recursive_mutex> l(m_Mutex); return m_strCurrentFolder; }
            Size getCurrentSize() const noexcept { std::lock_guard<std::recursive_mutex> l(m_Mutex); return m_CurrentSize; }
            Position getCurrentCursorPosition() const noexcept { std::lock_guard<std::recursive_mutex> l(m_Mutex); return m_CurrentCursorPosition; }
            bool isTerminalBeingResized() const noexcept {
                std::lock_guard<std::recursive_mutex> l(m_Mutex);
                return decltype(m_LastResizeEvent)::clock::now() < m_LastResizeEvent + std::chrono::milliseconds(500);
            }
            bool isPromptEnabled() const noexcept {
                std::lock_guard<std::recursive_mutex> l(m_Mutex);
                return m_bPromptEnabled;
            }

            virtual void onTerminalSizeChanged() noexcept;

            void addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor) noexcept;
            void addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor) noexcept;
            void delCommand(UserCommandInfo const& a_CommandInfo) noexcept;

            void setPromptEnabled(bool);

        protected:
            enum class Key {
                Enter,
                Back,
                Del,
                Tab,
                ReverseTab,
                Up,
                Down,
                Right,
                Left,
                Start,
                End,
                PageUp,
                PageDown,
                Insert,
                Escape,
                F1,
                F2,
                F3,
                F4,
                F5,
                F6,
                F7,
                F8,
                F9,
                F10,
                F11,
                F12,
                Printable
            };

        protected:
            void processPrintCommands(PrintCommand::VPtr const& = PrintCommand::VPtr{}) noexcept;
            void processUserCommands() noexcept;
            void processPressedKey(Key const&, std::string const& = {}) noexcept;
            void setCurrentSize(Size const& a_NewSize) noexcept {
                std::lock_guard<std::recursive_mutex> l(m_Mutex);
                if (m_CurrentSize != a_NewSize) {
                    m_LastResizeEvent = decltype(m_LastResizeEvent)::clock::now();
                }
                m_CurrentSize = a_NewSize;
            }
            void setCurrentCursorPosition(Position const& a_NewPosition) noexcept {
                std::lock_guard<std::recursive_mutex> l(m_Mutex);
                m_CurrentCursorPosition = a_NewPosition;
            }

        private:
            enum class PromptMode
            {
                Hidden,
                Normal,
                QuestionNormal,
                QuestionMonoChoice,
                QuestionMultiChoices
            };

        private:
            void setPromptMode(PromptMode const& a_ePromptMode,
                std::function<bool(bool const&, std::string const&)> const& a_fctFinished = nullptr,
                std::function<bool(Key const&, std::string const&)> const& a_fctKeyPressed = nullptr
            ) noexcept;
            void printCommandLine(bool const& a_bPrintInText = false) const noexcept;

        private:
            ConsoleSessionWithTerminal& m_rConsoleSession;
            mutable std::recursive_mutex m_Mutex{};
            mutable std::mutex m_PrintMutex{};
            bool m_bPrintCommandEnabled{ true };
            PrintCommand::VPtr m_vpPrintCommands{};
            std::shared_ptr<Functions> m_pFunctions;
            Functions::VCommands m_vCommands{};
            bool m_bPromptEnabled{ false };
            std::string m_strCurrentPrompt{};
            std::string m_strCurrentUser{};
            std::string m_strCurrentMachine{};
            std::string m_strCurrentFolder{};
            std::string m_strCurrentEntry{};
            unsigned int m_uiCurrentCursorPosition{ 0 };
            unsigned int m_uiMaxPromptSize{ 0 };
            unsigned int m_uiCurrentWindowPosition{ 0 };
            unsigned int m_uiCurrentWindowSize{ 0 };
            std::vector<std::string> m_vstrPreviousEntries{};
            int m_iCurrentPositionInPreviousEntries{ -1 };
            std::string m_strSavedEntry{};
            Size m_CurrentSize{};
            Position m_CurrentCursorPosition{};
            std::chrono::steady_clock::time_point m_LastResizeEvent{};
            PromptMode m_eCurrentPromptMode{ PromptMode::Normal };
            std::function<bool(bool const&, std::string const&)> m_fctFinished{};
            std::function<bool(Key const&, std::string const&)> m_fctKeyPressed{};
            bool m_bScrollingRegionSet{ false };
        };
    } // console
} // emb