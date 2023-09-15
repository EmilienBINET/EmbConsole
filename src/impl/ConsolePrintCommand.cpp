#include "EmbConsole.hpp"
#include "ConsolePrivate.hpp"
#include "base/Terminal.hpp"
#include <iostream>
#include <mutex>

using namespace std;

namespace emb {
    namespace console {
        //////////////////////////////////////////////////
        ///// PrintCommand Base
        //////////////////////////////////////////////////

        PrintCommand::PrintCommand() noexcept = default;

        PrintCommand::PrintCommand(PrintCommand const& aOther) noexcept = default;

        PrintCommand::PrintCommand(PrintCommand&&) noexcept = default;

        PrintCommand::~PrintCommand() noexcept = default;

        PrintCommand& PrintCommand::operator= (PrintCommand const& aOther) noexcept = default;

        PrintCommand& PrintCommand::operator= (PrintCommand&&) noexcept = default;

        //////////////////////////////////////////////////
        ///// PrintCommands: Begin / Commit
        //////////////////////////////////////////////////

        void Begin::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.begin();
        }
        void Commit::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.commit();
        }
        void InstantPrint::process(Terminal& a_rTerminal) const noexcept {
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Positioning
        //////////////////////////////////////////////////

        void MoveCursorUp::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorUp(m_uiN);
        }
        void MoveCursorDown::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorDown(m_uiN);
        }
        void MoveCursorForward::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorForward(m_uiN);
        }
        void MoveCursorBackward::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorBackward(m_uiN);
        }
        void MoveCursorToNextLine::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorToNextLine(m_uiN);
        }
        void MoveCursorToPreviousLine::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorToPreviousLine(m_uiN);
        }
        void MoveCursorToRow::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorToRow(m_uiR);
        }
        void MoveCursorToColumn::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorToColumn(m_uiC);
        }
        void MoveCursorToPosition::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.moveCursorToPosition(m_uiR, m_uiC);
        }
        void SaveCursor::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.saveCursor();
        }
        void RestoreCursor::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.restoreCursor();
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Visibility
        //////////////////////////////////////////////////

        void SetCursorBlinking::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setCursorBlinking(m_bBlinking);
        }
        void SetCursorVisible::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setCursorVisible(m_bVisible);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Shape
        //////////////////////////////////////////////////

        void SetCursorShape::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setCursorShape(m_eShape);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Viewport Positioning
        //////////////////////////////////////////////////

        void ScrollUp::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.scrollUp(m_uiN);
        }
        void ScrollDown::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.scrollDown(m_uiN);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Text Modification
        //////////////////////////////////////////////////

        void InsertCharacter::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.insertCharacter(m_uiN);
        }
        void DeleteCharacter::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.deleteCharacter(m_uiN);
        }
        void EraseCharacter::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.eraseCharacter(m_uiN);
        }
        void InsertLine::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.insertLine(m_uiN);
        }
        void DeleteLine::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.deleteLine(m_uiN);
        }
        void ClearDisplay::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.clearDisplay(m_eType);
        }
        void ClearLine::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.clearLine(m_eType);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Text Formatting
        //////////////////////////////////////////////////

        void ResetTextFormat::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.resetTextFormat();
        }
        void SetColor::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setColor(m_eFgColor, m_eBgColor);
        }
        void SetNegativeColors::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setNegativeColors(m_bEnabled);
        }
        void SetBold::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setBold(m_bEnabled);
        }
        void SetItalic::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setItalic(m_bEnabled);
        }
        void SetUnderline::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setUnderline(m_bEnabled);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Tabs
        //////////////////////////////////////////////////

        void SetHorizontalTab::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setHorizontalTab();
        }
        void HorizontalTabForward::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.goToHorizontalTabForward(m_uiN);
        }
        void HorizontalTabBackward::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.goToHorizontalTabBackward(m_uiN);
        }
        void ClearHorizontalTab::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.clearHorizontalTab(m_eType);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Designate Character Set
        //////////////////////////////////////////////////

        void SetDecCharacterSet::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setDecCharacterSet(m_bEnabled);
        }
        void PrintSymbol::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.printSymbol(m_eSymbol, m_uiN);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Scrolling Margins
        //////////////////////////////////////////////////

        void SetScrollingRegion::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setScrollingRegion(m_uiT, m_uiB);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Window Title
        //////////////////////////////////////////////////

        void SetWindowTitle::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.setWindowTitle(m_strTitle);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Alternate Screen Buffer
        //////////////////////////////////////////////////

        void UseAlternateScreenBuffer::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.useAlternateScreenBuffer(m_bEnabled);
        }

        //////////////////////////////////////////////////
        ///// PrintCommands: Miscellaneous
        //////////////////////////////////////////////////

        void RingBell::process(Terminal& a_rTerminal) const noexcept {
            a_rTerminal.ringBell();
        }
        void PrintNewLine::process(Terminal& a_rTerminal) const noexcept {
            for (unsigned int i = 0; i < m_uiN; ++i) {
                a_rTerminal.printNewLine();
            }
        }
        void PrintText::process(Terminal& a_rTerminal) const noexcept {
            if (0 == m_uiR || 0 == m_uiC) {
                a_rTerminal.printText(m_strText);
            }
            else {
                a_rTerminal.printTextAt(m_strText, m_uiR, m_uiC);
            }
        }
    } // console
} // emb