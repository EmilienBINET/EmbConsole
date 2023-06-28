#include "Terminal.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <condition_variable>

namespace emb {
    namespace console {
        using namespace std;

        Terminal::Terminal(ConsoleSessionWithTerminal& a_Console) noexcept
            : m_rConsoleSession{a_Console}
            , m_pFunctions{ make_shared<Functions>(m_rConsoleSession) }
            , m_strCurrentUser{ "user" }
            , m_strCurrentMachine{ "machine" }
            , m_strCurrentFolder{ "/" } {
            m_pFunctions->addCommand(UserCommandInfo("/cd", "Change the shell working directory"), [&](UserCommandData const& a_CmdData) {
                if (1 != a_CmdData.args.size()) {
                    a_CmdData.console.printError("Usage: cd <dir>");
                }
                else {
                    string const& folder = a_CmdData.args.at(0);
                    assert(folder.size() > 0);
                    if ('/' == folder.at(0)) {
                        m_strCurrentFolder = Functions::getCanonicalPath(folder);
                    }
                    else {
                        m_strCurrentFolder = Functions::getCanonicalPath(m_strCurrentFolder + "/" + folder);
                    }
                }
                onTerminalSizeChanged();
            });
        }
        /*Terminal::Terminal(Terminal const&) noexcept {
        }*/
        /*Terminal::Terminal(Terminal&&) noexcept {
        }*/
        Terminal::~Terminal() noexcept {
        }
        /*Terminal& Terminal::operator= (Terminal const&) noexcept {
            return *this;
        }
        Terminal& Terminal::operator= (Terminal&&) noexcept {
            return *this;
        }*/

        void Terminal::start() noexcept {
        }

        void Terminal::stop() noexcept {
            setPromptEnabled(false);
            begin();
            setCursorVisible(true);
            commit();
            processPrintCommands(m_vpPrintCommands);
        }

        void Terminal::setPrintCommands(PrintCommand::VPtr const& a_vpPrintCommands, bool a_bInstantPrint) noexcept {
            if (a_bInstantPrint /* true if attached to gdb and if option set*/) {
                processPrintCommands(a_vpPrintCommands);
            }
            else {
                lock_guard<recursive_mutex> l{ m_Mutex };
                m_vpPrintCommands.insert(m_vpPrintCommands.end(), a_vpPrintCommands.begin(), a_vpPrintCommands.end());
            }
        }

        template<typename T>
        size_t countType(PromptCommand::VPtr const& a_vpPromptCommands) noexcept {
            size_t uiRes{ 0 };
            for (auto const& elm : a_vpPromptCommands) {
                if (auto const& elm2 = dynamic_pointer_cast<T>(elm)) {
                    ++uiRes;
                }
            }
            return uiRes;
        }

        template<typename T>
        shared_ptr<T> getType(PromptCommand::VPtr const& a_vpPromptCommands) noexcept {
            for (auto const& elm : a_vpPromptCommands) {
                if (auto const& elm2 = dynamic_pointer_cast<T>(elm)) {
                    return elm2;
                }
            }
            return nullptr;
        }

        template<typename T>
        shared_ptr<T> getType(PromptCommand::VPtr const& a_vpPromptCommands, size_t const& a_uiIdx) noexcept {
            size_t uiCount{ 0 };
            for (auto const& elm : a_vpPromptCommands) {
                if (auto const& elm2 = dynamic_pointer_cast<T>(elm)) {
                    if (a_uiIdx == uiCount) {
                        return elm2;
                    }
                    ++uiCount;
                }
            }
            return nullptr;
        }

        void Terminal::setPromptCommands(PromptCommand::VPtr const& a_vpPromptCommands) noexcept {
            size_t questionsCount = countType<Question>(a_vpPromptCommands);
            auto const& question = getType<Question>(a_vpPromptCommands);
            size_t choicesCount = countType<Choice>(a_vpPromptCommands);
            size_t validatorsCount = countType<Validator>(a_vpPromptCommands);
            auto const& validator = getType<Validator>(a_vpPromptCommands);

            assert(questionsCount <= 1 && "PromptCommand: There cannot be more than one question.");
            assert(validatorsCount <= 1 && "PromptCommand: There cannot be more than one validator.");

            // Stops print commands
            {
                lock_guard<recursive_mutex> l{ m_Mutex };
                m_bPrintCommandEnabled = false;
            }

            if (question) {
                begin();
                printText(question->m_strQuestion);
                commit();
            }
            if (0 == choicesCount) {
                std::condition_variable cv;
                m_strCurrentPrompt = "Answer here";
                setPromptMode(PromptMode::QuestionNormal, [&](bool const& a_bValid, std::string const& a_strUserEntry) {
                    begin();

                    setColor(SetColor::Color::BrightBlue, SetColor::Color::Black);
                    printText(" " + a_strUserEntry);
                    resetTextFormat();
                    printNewLine();

                    bool bUserEntryIsValid = true;
                    if (validatorsCount >= 1 && validator) {
                        // There is a validator, we must test it
                        bUserEntryIsValid = validator->isValid(a_strUserEntry);
                    }

                    if (bUserEntryIsValid) {
                        cv.notify_one();

                        if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                            (*onValid)(a_strUserEntry);
                        }
                    }
                    else {
                        setColor(SetColor::Color::BrightRed, SetColor::Color::Black);
                        printText(validator->m_strErrorMessage.empty() ? "User entry did not pass validation." : validator->m_strErrorMessage);
                        resetTextFormat();
                        printNewLine();
                        printText(question->m_strQuestion);
                    }

                    commit();
                    return bUserEntryIsValid;
                }, [&](Key const& a_eKey, std::string const& a_strPrintableData) {
                    if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Black);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        cv.notify_one();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }
                    }
                    return true;
                });

                std::mutex m;
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk);
                m_strCurrentPrompt = "";
                setPromptMode(PromptMode::Normal);
                // Starts print commands
                {
                    lock_guard<recursive_mutex> l{ m_Mutex };
                    m_bPrintCommandEnabled = true;
                }
            }
            else if (choicesCount <= 2) {
                std::string promptText{ "Type one of [" };
                for (size_t idx = 0; idx < choicesCount; ++idx) {
                    auto choice = getType<Choice>(a_vpPromptCommands, idx);
                    printText(" " + choice->visibleString(false));
                    promptText += choice->key();
                }
                promptText += "]";

                std::condition_variable cv;
                m_strCurrentPrompt = promptText;
                setPromptMode(PromptMode::QuestionMonoChoice, nullptr,
                    [&](Key const& a_eKey, std::string const& a_strPrintableData) {
                    bool bUserEntryIsValid{ false };
                    if (Key::Printable == a_eKey) {
                        for (size_t idx = 0; idx < choicesCount; ++idx) {
                            auto choice = getType<Choice>(a_vpPromptCommands, idx);
                            if (choice->isChosen(a_strPrintableData.at(0))) {
                                begin();
                                moveCursorToRow(1);
                                clearLine(ClearLine::Type::All);
                                printText(question->m_strQuestion);
                                setColor(SetColor::Color::BrightBlue, SetColor::Color::Black);
                                printText(" " + choice->visibleString(true));
                                resetTextFormat();
                                printNewLine();
                                commit();

                                cv.notify_one();

                                if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                                    (*onValid)(choice->m_strValue);
                                }

                                break;
                            }
                        }
                    }
                    else if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Black);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        cv.notify_one();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }
                    }
                    return bUserEntryIsValid;
                });

                std::mutex m;
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk);
                m_strCurrentPrompt = "";
                setPromptMode(PromptMode::Normal);
                // Starts print commands
                {
                    lock_guard<recursive_mutex> l{ m_Mutex };
                    m_bPrintCommandEnabled = true;
                }
            }
            else {
                std::string promptText{ "Type on of [" };
                for (size_t idx = 0; idx < choicesCount; ++idx) {
                    auto choice = getType<Choice>(a_vpPromptCommands, idx);
                    printNewLine();
                    printText(" - " + choice->visibleString(false));
                    promptText += choice->key();
                }
                promptText += "]";

                std::condition_variable cv;
                m_strCurrentPrompt = promptText;
                setPromptMode(PromptMode::QuestionMonoChoice, nullptr,
                    [&](Key const& a_eKey, std::string const& a_strPrintableData) {
                    bool bUserEntryIsValid{ false };
                    if (Key::Printable == a_eKey) {
                        for (size_t idx = 0; idx < choicesCount; ++idx) {
                            auto choice = getType<Choice>(a_vpPromptCommands, idx);
                            if (choice->isChosen(a_strPrintableData.at(0))) {
                                begin();
                                moveCursorUp(choicesCount);
                                moveCursorToRow(0);
                                printText(question->m_strQuestion);
                                for (size_t idx = 0; idx < choicesCount; ++idx) {
                                    auto choice = getType<Choice>(a_vpPromptCommands, idx);
                                    printNewLine();
                                    clearLine(ClearLine::Type::All);
                                    printText(" - ");
                                    if (choice->isChosen(a_strPrintableData.at(0))) {
                                        setColor(SetColor::Color::BrightBlue, SetColor::Color::Black);
                                        printText(choice->visibleString(true));
                                        resetTextFormat();
                                    }
                                    else {
                                        printText(choice->visibleString(true));
                                    }
                                }
                                printNewLine();
                                commit();

                                cv.notify_one();

                                if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                                    (*onValid)(choice->m_strValue);
                                }

                                break;
                            }
                        }
                    }
                    else if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Black);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        cv.notify_one();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }
                    }
                    return bUserEntryIsValid;
                });

                std::mutex m;
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk);
                m_strCurrentPrompt = "";
                setPromptMode(PromptMode::Normal);
                // Starts print commands
                {
                    lock_guard<recursive_mutex> l{ m_Mutex };
                    m_bPrintCommandEnabled = true;
                }
            }
            //printNewLine();
            //commit();
        }

        void Terminal::onTerminalSizeChanged() noexcept {
            if (m_strCurrentPrompt.empty()) {
                m_uiMaxPromptSize =
                    m_CurrentSize.iWidth
                    - m_strCurrentUser.size()
                    - m_strCurrentMachine.size()
                    - m_strCurrentFolder.size()
                    - 5; // @ + : + $ + space + last char
            }
            else {
                m_uiMaxPromptSize =
                    m_CurrentSize.iWidth
                    - m_strCurrentPrompt.size()
                    - 3; // + > + space + last char
            }

            if (m_strCurrentEntry.size() <= m_uiMaxPromptSize) {
                // enough room to show all command line
                m_uiCurrentWindowPosition = 0;
                m_uiCurrentWindowSize = m_strCurrentEntry.size();
            }
            else {
                // not enough room to show all command line
                // so truncate the command line by the right by reducing the shown window size
                // after that step, the cursor might be outside of the window
                m_uiCurrentWindowSize = m_uiMaxPromptSize;

                if (m_uiCurrentCursorPosition > m_uiCurrentWindowPosition + m_uiCurrentWindowSize) {
                    m_uiCurrentWindowPosition = m_uiCurrentCursorPosition - m_uiCurrentWindowSize;
                }
                else if (m_uiCurrentCursorPosition > m_uiCurrentWindowPosition) {
                    m_uiCurrentWindowPosition = m_strCurrentEntry.size() - m_uiCurrentWindowSize;
                }
                else if (m_uiCurrentCursorPosition > m_strCurrentEntry.size() - m_uiCurrentWindowSize) {
                    m_uiCurrentWindowPosition = m_strCurrentEntry.size() - m_uiCurrentWindowSize;
                }
            }

            auto size = getCurrentSize();
            auto pos = getCurrentCursorPosition();
            if (!m_bScrollingRegionSet) {
                m_bScrollingRegionSet = true;
                begin();
                for (int i = size.iHeight - pos.iY; i > 0; --i) {
                    printNewLine();
                }
                commit();
            }
            setScrollingRegion(1, size.iHeight - 1);
            if (pos.iY < size.iHeight) {
                moveCursorToPosition(pos.iY, pos.iX);
            }
            else {
                moveCursorToPosition(size.iHeight, pos.iX);
            }

            printCommandLine();
        }

        void Terminal::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor) noexcept {
            return m_pFunctions->addCommand(a_CommandInfo, a_funcCommandFunctor);
        }

        void Terminal::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor) noexcept {
            return m_pFunctions->addCommand(a_CommandInfo, a_funcCommandFunctor);
        }

        void Terminal::delCommand(UserCommandInfo const& a_CommandInfo) noexcept {
            return m_pFunctions->delCommand(a_CommandInfo);
        }

        void Terminal::setPromptEnabled(bool a_bPromptEnabled) {
            lock_guard<recursive_mutex> l{ m_Mutex };
            m_bPromptEnabled = a_bPromptEnabled;
            printCommandLine();
        }

        void Terminal::processPrintCommands(PrintCommand::VPtr const& a_vpPrintCommands) noexcept {
            if (m_bPrintCommandEnabled && !isTerminalBeingResized()) {
                PrintCommand::VPtr vpPrintCommands{ a_vpPrintCommands };
                if (vpPrintCommands.size() <= 0) {
                    lock_guard<recursive_mutex> l{ m_Mutex };
                    vpPrintCommands = m_vpPrintCommands;
                    m_vpPrintCommands.clear();
                }

                for (auto const& printCommand : vpPrintCommands) {
                    printCommand->process(*this);
                }
                if (vpPrintCommands.size() > 0) {
                    printCommandLine();
                }
            }
        }

        void Terminal::processUserCommands() noexcept {
            Functions::VCommands vCommands;
            {
                lock_guard<recursive_mutex> l{ m_Mutex };
                vCommands = m_vCommands;
                m_vCommands.clear();
            }
            for (auto const& elm : vCommands) {
                m_pFunctions->processEntry(elm.strRawCommand, elm.strCurrentPath);
            }
        }

        void Terminal::processPressedKey(Key const& a_eKey, std::string const& a_strValue) noexcept {
            lock_guard<recursive_mutex> l{ m_Mutex };

            const bool enableDebugHistory = false;
            const auto debugHistory = [&]() {
                if (enableDebugHistory) {
                    cout << endl << endl << "==== HISTORY ====" << endl;
                    for (auto const& elm : m_vstrPreviousEntries) {
                        cout << elm << endl;
                    }
                    cout << m_iCurrentPositionInPreviousEntries << endl;
                    cout << "=== !HISTORY! ===" << endl;
                }
            };

            bool bContinue = true;
            if (m_bPromptEnabled && PromptMode::Hidden != m_eCurrentPromptMode) {
                if (m_fctKeyPressed) {
                    bContinue = m_fctKeyPressed(a_eKey, a_strValue);
                }
            }

            if (bContinue) {
                switch (a_eKey)
                {
                case Key::Enter:
                    if (m_bPromptEnabled && PromptMode::Normal == m_eCurrentPromptMode) {
                        m_vCommands.push_back(Functions::Command{ m_strCurrentEntry, m_strCurrentFolder });
                        printCommandLine(true);
                        m_iCurrentPositionInPreviousEntries = -1;
                        if (!m_strCurrentEntry.empty()) {
                            m_vstrPreviousEntries.insert(m_vstrPreviousEntries.begin(), m_strCurrentEntry);
                        }
                        m_uiCurrentCursorPosition = 0;
                        m_uiCurrentWindowPosition = 0;
                        m_uiCurrentWindowSize = 0;
                        m_strCurrentEntry.clear();
                        debugHistory();
                    }
                    else if (m_bPromptEnabled && PromptMode::QuestionNormal == m_eCurrentPromptMode) {
                        m_uiCurrentCursorPosition = 0;
                        m_uiCurrentWindowPosition = 0;
                        m_uiCurrentWindowSize = 0;
                        if (m_fctFinished) {
                            m_fctFinished(true, m_strCurrentEntry);
                        }
                        m_strCurrentEntry.clear();
                        debugHistory();
                    }
                    break;
                case Key::Back:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        if (m_uiCurrentCursorPosition > 0) {
                            --m_uiCurrentCursorPosition;
                            m_strCurrentEntry.erase(m_strCurrentEntry.begin() + m_uiCurrentCursorPosition);
                            if (m_uiCurrentWindowPosition > 0) {
                                --m_uiCurrentWindowPosition;
                            }
                            else {
                                --m_uiCurrentWindowSize;
                            }
                        }
                        else {
                            ringBell();
                        }
                    }
                    break;
                case Key::Del:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        if (m_uiCurrentCursorPosition < m_strCurrentEntry.size()) {
                            m_strCurrentEntry.erase(m_strCurrentEntry.begin() + m_uiCurrentCursorPosition);
                            if (m_uiCurrentWindowPosition > 0) {
                                --m_uiCurrentWindowPosition;
                            }
                            else {
                                --m_uiCurrentWindowSize;
                            }
                        }
                        else {
                            ringBell();
                        }
                    }
                    break;
                case Key::Tab:
                case Key::ReverseTab:
                    if (m_bPromptEnabled && PromptMode::Normal == m_eCurrentPromptMode) {
                        if (m_pFunctions->processAutoCompletion(m_strCurrentEntry, m_uiCurrentCursorPosition, m_strCurrentFolder, Key::Tab == a_eKey)) {
                            m_uiCurrentWindowPosition = 0;
                            m_uiCurrentWindowSize = min<unsigned int>(m_uiMaxPromptSize, m_strCurrentEntry.size());
                        }
                    }
                    break;
                case Key::Up:
                    if (m_bPromptEnabled && PromptMode::Normal == m_eCurrentPromptMode) {
                        if (m_iCurrentPositionInPreviousEntries < static_cast<int>(m_vstrPreviousEntries.size() - 1)) {
                            if (-1 == m_iCurrentPositionInPreviousEntries) {
                                m_strSavedEntry = m_strCurrentEntry;
                            }
                            ++m_iCurrentPositionInPreviousEntries;
                            m_uiCurrentCursorPosition = 0;
                            m_strCurrentEntry = m_vstrPreviousEntries.at(m_iCurrentPositionInPreviousEntries);
                            m_uiCurrentWindowPosition = 0;
                            m_uiCurrentWindowSize = min<unsigned int>(m_uiMaxPromptSize, m_strCurrentEntry.size());
                        }
                        else {
                            ringBell();
                        }
                        debugHistory();
                    }
                    break;
                case Key::Down:
                    if (m_bPromptEnabled && PromptMode::Normal == m_eCurrentPromptMode) {
                        if (m_iCurrentPositionInPreviousEntries >= 0) {
                            --m_iCurrentPositionInPreviousEntries;
                            m_uiCurrentCursorPosition = 0;
                            if (-1 == m_iCurrentPositionInPreviousEntries) {
                                m_strCurrentEntry = m_strSavedEntry;
                                m_strSavedEntry.clear();
                            }
                            else {
                                m_strCurrentEntry = m_vstrPreviousEntries.at(m_iCurrentPositionInPreviousEntries);
                            }
                            m_uiCurrentWindowPosition = 0;
                            m_uiCurrentWindowSize = min<unsigned int>(m_uiMaxPromptSize, m_strCurrentEntry.size());
                        }
                        else {
                            ringBell();
                        }
                        debugHistory();
                    }
                    break;
                case Key::Right:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        if (m_uiCurrentCursorPosition < m_strCurrentEntry.size()) {
                            ++m_uiCurrentCursorPosition;
                            if (m_uiCurrentCursorPosition > m_uiCurrentWindowPosition + m_uiCurrentWindowSize ||
                                (m_uiCurrentCursorPosition == m_uiCurrentWindowPosition + m_uiCurrentWindowSize &&
                                    m_uiCurrentCursorPosition < m_strCurrentEntry.size())) {
                                ++m_uiCurrentWindowPosition;
                            }
                        }
                        else {
                            ringBell();
                        }
                    }
                    break;
                case Key::Left:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        if (m_uiCurrentCursorPosition > 0) {
                            --m_uiCurrentCursorPosition;
                            if (m_uiCurrentCursorPosition < m_uiCurrentWindowPosition) {
                                --m_uiCurrentWindowPosition;
                            }
                        }
                        else {
                            ringBell();
                        }
                    }
                    break;
                case Key::Start:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        m_uiCurrentCursorPosition = 0;
                        m_uiCurrentWindowPosition = 0;
                    }
                    break;
                case Key::End:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        m_uiCurrentCursorPosition = m_strCurrentEntry.size();
                        m_uiCurrentWindowPosition = m_strCurrentEntry.size() - m_uiCurrentWindowSize;
                    }
                    break;
                case Key::PageUp:
                    break;
                case Key::PageDown:
                    break;
                case Key::Insert:
                    break;
                case Key::Escape:
                    break;
                case Key::F1:
                    m_bPrintCommandEnabled = !m_bPrintCommandEnabled;
                    break;
                case Key::F2:
                    m_bPromptEnabled = !m_bPromptEnabled;
                    break;
                case Key::F3:
                    break;
                case Key::F4:
                    break;
                case Key::F5:
                    break;
                case Key::F6:
                    break;
                case Key::F7:
                    break;
                case Key::F8:
                    break;
                case Key::F9:
                    break;
                case Key::F10:
                    break;
                case Key::F11:
                    break;
                case Key::F12:
                    break;
                case Key::Printable:
                    if (m_bPromptEnabled && (PromptMode::Normal == m_eCurrentPromptMode || PromptMode::QuestionNormal == m_eCurrentPromptMode)) {
                        m_strCurrentEntry.insert(m_uiCurrentCursorPosition, a_strValue);
                        ++m_uiCurrentCursorPosition;

                        if (m_uiCurrentWindowSize < m_uiMaxPromptSize) {
                            ++m_uiCurrentWindowSize;
                        }
                        else {
                            ++m_uiCurrentWindowPosition;
                        }
                    }
                    break;
                }
            }

            printCommandLine();
        }

        void Terminal::setPromptMode(PromptMode const& a_ePromptMode,
            std::function<bool(bool const&, std::string const&)> const& a_fctFinished,
            std::function<bool(Key const&, std::string const&)> const& a_fctKeyPressed) noexcept {
            m_eCurrentPromptMode = a_ePromptMode;
            m_fctFinished = a_fctFinished;
            m_fctKeyPressed = a_fctKeyPressed;
            printCommandLine();
        }

        void Terminal::printCommandLine(bool const& a_bPrintInText) const noexcept {
            if(!supportsInteractivity()) {
                return;
            }
            
            int const iMinPromptSize = 5;

            auto printCommonPart = [&] {
                if (m_strCurrentPrompt.empty()) {
                    setColor(SetColor::Color::BrightGreen, SetColor::Color::Black);
                    printText(m_strCurrentUser + "@" + m_strCurrentMachine);
                    resetTextFormat();

                    printText(":");

                    setColor(SetColor::Color::BrightBlue, SetColor::Color::Black);
                    printText(m_strCurrentFolder);
                    resetTextFormat();

                    clearLine(ClearLine::Type::FromCursorToEnd);
                    printText("$ ");
                }
                else {
                    setColor(SetColor::Color::White, SetColor::Color::BrightBlue);
                    printText(m_strCurrentPrompt);
                    resetTextFormat();

                    clearLine(ClearLine::Type::FromCursorToEnd);
                    printText("> ");
                }
            };

            begin();
            setCursorVisible(false); // do once
            if (a_bPrintInText) {
                printCommonPart();

                printText(m_strCurrentEntry);
                printNewLine();
            }
            else if (!m_bPromptEnabled) {
                //else if (!m_bPromptEnabled || (m_eCurrentPromptMode != PromptMode::Normal && m_eCurrentPromptMode != PromptMode::QuestionNormal)) {
                saveCursor();
                moveCursorToPosition(999, 1);
                clearLine(ClearLine::Type::FromCursorToEnd);
                restoreCursor();
            }
            else if (m_uiMaxPromptSize < iMinPromptSize) {
                saveCursor();
                moveCursorDown(1);
                clearDisplay(ClearDisplay::Type::FromCursorToEnd);
                moveCursorToPosition(999, 1);
                setColor(SetColor::Color::Red, SetColor::Color::Black);
                printText(" <== INCREASE ==> ");
                resetTextFormat();
                restoreCursor();
            }
            else {
                saveCursor();
                moveCursorDown(1);
                clearDisplay(ClearDisplay::Type::FromCursorToEnd);
                moveCursorToPosition(999, 1);

                printCommonPart();
                if (m_uiCurrentWindowPosition > 0) {
                    setColor(SetColor::Color::BrightYellow, SetColor::Color::Magenta);
                    moveCursorBackward(1);
                    printText("<");
                    resetTextFormat();
                }

                string strTmp{ m_strCurrentEntry + " " };
                printText(strTmp.substr(m_uiCurrentWindowPosition, m_uiCurrentCursorPosition - m_uiCurrentWindowPosition));
                setNegativeColors(true);
                printText(strTmp.substr(m_uiCurrentCursorPosition, 1));
                setNegativeColors(false);
                printText(strTmp.substr(m_uiCurrentCursorPosition + 1, m_uiCurrentWindowPosition + m_uiCurrentWindowSize - m_uiCurrentCursorPosition - 1));

                if (m_uiCurrentWindowPosition + m_uiCurrentWindowSize < m_strCurrentEntry.size()) {
                    setColor(SetColor::Color::BrightYellow, SetColor::Color::Magenta);
                    printText(">");
                    resetTextFormat();
                }

                restoreCursor();
            }
            commit();
        }
    } // console
} // emb