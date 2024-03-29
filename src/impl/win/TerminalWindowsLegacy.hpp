#pragma once

#include "../base/TerminalAnsi.hpp"
#include <thread>
#include <atomic>

namespace emb {
    namespace console {
        class TerminalWindowsLegacy
            : public Terminal
        {
        public:
            TerminalWindowsLegacy(ConsoleSessionWithTerminal&) noexcept;
            TerminalWindowsLegacy(TerminalWindowsLegacy const&) noexcept = delete;
            TerminalWindowsLegacy(TerminalWindowsLegacy&&) noexcept = delete;
            virtual ~TerminalWindowsLegacy() noexcept;
            TerminalWindowsLegacy& operator= (TerminalWindowsLegacy const&) noexcept = delete;
            TerminalWindowsLegacy& operator= (TerminalWindowsLegacy&&) noexcept = delete;

        protected:
            //void start() noexcept override;
            void processEvents() noexcept override;
            //void stop() noexcept override;

            bool supportsInteractivity() const noexcept override;
            bool supportsColor() const noexcept override;

            bool read(std::string& a_rstrKey) const noexcept override;
            bool write(std::string const& a_strDataToPrint) const noexcept override;

            void processCapture() const noexcept;

            void begin() const noexcept override;
            void commit() const noexcept override;
            void moveCursorUp(unsigned int const a_uiN) const noexcept override;
            void moveCursorDown(unsigned int const a_uiN) const noexcept override;
            void moveCursorForward(unsigned int const a_uiN) const noexcept override;
            void moveCursorBackward(unsigned int const a_uiN) const noexcept override;
            void moveCursorToNextLine(unsigned int const a_uiN) const noexcept override;
            void moveCursorToPreviousLine(unsigned int const a_uiN) const noexcept override;
            void moveCursorToRow(unsigned int const a_uiR) const noexcept override;
            void moveCursorToColumn(unsigned int const a_uiC) const noexcept override;
            void moveCursorToPosition(unsigned int const a_uiR, unsigned int const a_uiC) const noexcept override;
            void saveCursor() const noexcept override;
            void restoreCursor() const noexcept override;
            void setCursorBlinking(bool const a_bBlinking) const noexcept override;
            void setCursorVisible(bool const a_bVisible) const noexcept override;
            void setCursorShape(SetCursorShape::Shape const a_eShape) const noexcept override;
            void scrollUp(unsigned int const a_uiN) const noexcept override;
            void scrollDown(unsigned int const a_uiN) const noexcept override;
            void insertCharacter(unsigned int const a_uiN) const noexcept override;
            void deleteCharacter(unsigned int const a_uiN) const noexcept override;
            void eraseCharacter(unsigned int const a_uiN) const noexcept override;
            void insertLine(unsigned int const a_uiN) const noexcept override;
            void deleteLine(unsigned int const a_uiN) const noexcept override;
            void clearDisplay(ClearDisplay::Type const a_eType) const noexcept override;
            void clearLine(ClearLine::Type const a_eType) const noexcept override;
            void resetTextFormat() const noexcept override;
            void setColor(SetColor::Color const a_eFgColor, SetColor::Color const a_eBgColor) const noexcept override;
            void setNegativeColors(bool const a_bEnabled) const noexcept override;
            void setBold(bool const a_bEnabled) const noexcept override;
            void setItalic(bool const a_bEnabled) const noexcept override;
            void setUnderline(bool const a_bEnabled) const noexcept override;
            void setHorizontalTab() const noexcept override;
            void goToHorizontalTabForward(unsigned int const a_uiN) const noexcept override;
            void goToHorizontalTabBackward(unsigned int const a_uiN) const noexcept override;
            void clearHorizontalTab(ClearHorizontalTab::Type const a_eType) const noexcept override;
            void setDecCharacterSet(bool const a_bEnabled) const noexcept override;
            void printSymbol(PrintSymbol::Symbol const a_eSymbol, unsigned int const a_uiN) const noexcept override;
            void setScrollingRegion(unsigned int const a_uiT, unsigned int const a_uiB) const noexcept override;
            void setWindowTitle(std::string const& a_strTitle) const noexcept override;
            void useAlternateScreenBuffer(bool const& a_bEnabled) const noexcept override;
            void softReset() const noexcept override;
            void ringBell() const noexcept override;
            void printNewLine() const noexcept override;
            void printText(std::string const& a_strText) const noexcept override;
            void printTextAt(std::string const& a_strText, unsigned int const a_uiR, unsigned int const a_uiC) const noexcept override;
            void processPressedKeyCode(std::string const& a_strKey) noexcept;

        private:
            mutable short wCursorPositionX{};
            mutable short wCursorPositionY{};
            mutable short wAttributes{};
        };
    } // console
} // emb
