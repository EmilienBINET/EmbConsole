#include "Terminal.hpp"
#include <iostream>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <condition_variable>
#include <sstream>

namespace emb {
    namespace console {
        using namespace std;

        Terminal::Terminal(ConsoleSessionWithTerminal& a_Console) noexcept
            : m_rConsoleSession(a_Console)
            , m_pFunctions{ make_shared<Functions>(m_rConsoleSession) }
            , m_strCurrentUser{ "user" }
            , m_strCurrentMachine{ "machine" }
            , m_strCurrentFolder{ "/" } {

            // the "cd" command allows the user to navigate among console directories
            m_pFunctions->addCommand(UserCommandInfo("/cd", "Change the shell working directory"),
                //---------- The 1rs lamba is called when the user call the "cd" command ----------
                [this](UserCommandData const& a_CmdData) {
                    if (1 != a_CmdData.args.size()) {
                        // "cd" command is called with an invalid number of parameter
                        a_CmdData.console.printError("Usage: cd <dir>");
                    }
                    else {
                        // "cd" command is called with a valid number of parameter:
                        // the 1st parameter is the destination folder
                        string strDestinationPath = a_CmdData.args.at(0);
                        assert(strDestinationPath.size() > 0);

                        // Parse the destination folder into a usable path
                        if (Functions::isAbsolutePath(strDestinationPath)) {
                            // If the path is absolute, we just canonize it
                            strDestinationPath = Functions::getCanonicalPath(strDestinationPath);
                        }
                        else {
                            // If the path is relative, we need to a the current folder before the canonization
                            strDestinationPath = Functions::getCanonicalPath(m_strCurrentFolder + "/" + strDestinationPath);
                        }

                        if(m_pFunctions->folderExists(strDestinationPath)) {
                            // If the destination exists in the list of folders, we change the current folder and update the terminal size
                            // because the new folder my have an impact on the size of the command line prompt
                            m_strCurrentFolder = strDestinationPath;
                            onTerminalSizeChanged();
                        }
                        else {
                            // If the destination does not exist, we print the error
                            a_CmdData.console.printError("Directory " + strDestinationPath + " does not exists.");
                        }
                    }
                },
                //---------- The 2nd lamba is called when the user tries to autocomplete the "cd command" ----------
                [this](UserCommandAutoCompleteData const& a_AcData) {
                    // We need to create a choices list among which the user can naviguate using <Tab> and <RTab> keyboard keys
                    vector<string> vecChoices{};

                    // We get the position of the last '/' in the argument the user is typing
                    size_t ulPos = a_AcData.partialArg.find_last_of('/');

                    // We need to find information about what the user is typing:
                    string strPrefixFolder{}; // what complete folder the user typed
                    string strPartialArg{a_AcData.partialArg}; // what partial folder the user started to typed
                    string strCurrentFolder{m_strCurrentFolder}; // what folder we need to search the choices into
                    if(string::npos != ulPos) {
                        // if a complete folder has already been typed, it is the part before the last '/'
                        strPrefixFolder = a_AcData.partialArg.substr(0, ulPos+1);
                        // and then the partial arg is now the part after the last '/'
                        strPartialArg = a_AcData.partialArg.substr(ulPos+1);
                        // the current folder we need to search the choices into is constructed from the current folder where the "cd"
                        // command is typed and the complete folder the user typed as an argument of "cd"
                        strCurrentFolder = Functions::getCanonicalPath(strCurrentFolder + "/" + strPrefixFolder);
                        // e.g. if the current directory is /w/x
                        // cd abcd/ef<Tab> => strPrefixFolder is "abcd/", strPartialArg is "ef", strCurrentFolder is /w/x/abcd
                    }

                    // We search the commands available from the current folder
                    auto vecLocalCommands = m_pFunctions->getCommands(strCurrentFolder);

                    // We filter the found command and only keep the directories that start with strPartialArg
                    for(auto const& elm : vecLocalCommands) {
                        if(elm.bIsDirectory && 0 == elm.strName.find(strPartialArg)) {
                            vecChoices.push_back(strPrefixFolder + elm.strName);
                        }
                    }

                    // We return the generated list of the possible choices to the user
                    return vecChoices;
                }
            );
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
            begin();
            setCursorVisible(false);
            commit();
            onTerminalSizeChanged();
        }

        void Terminal::stop() noexcept {
            setPromptEnabled(false);
            begin();
            setCursorVisible(true);
            softReset();
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
            (void)questionsCount; // Not used after so can generate a warning in release when assert is not defined

            // Stops print commands
            {
                lock_guard<recursive_mutex> l{ m_Mutex };
                m_bPrintCommandEnabled = false;
            }

            if (question) {
                begin();
                clearLine(ClearLine::Type::All);
                printNewLine();
                moveCursorToPreviousLine(1);
                printText(question->m_strQuestion);
                commit();
            }
            if (0 == choicesCount) {
                std::condition_variable cv;
                m_strCurrentPrompt = "Answer here";
                setPromptMode(PromptMode::QuestionNormal, [&](bool const& a_bValid, std::string const& a_strUserEntry) {
                    begin();

                    setColor(SetColor::Color::BrightBlue, SetColor::Color::Default);
                    printText(" " + a_strUserEntry);
                    resetTextFormat();
                    printNewLine();

                    bool bUserEntryIsValid = true;
                    if (validatorsCount >= 1 && validator) {
                        // There is a validator, we must test it
                        bUserEntryIsValid = validator->isValid(a_strUserEntry);
                    }

                    if (bUserEntryIsValid) {
                        if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                            (*onValid)(a_strUserEntry);
                        }

                        cv.notify_one();
                    }
                    else {
                        setColor(SetColor::Color::BrightRed, SetColor::Color::Default);
                        printText(validator->m_strErrorMessage.empty() ? "User entry did not pass validation." : validator->m_strErrorMessage);
                        resetTextFormat();
                        printNewLine();
                        clearLine(ClearLine::Type::All);
                        printNewLine();
                        moveCursorToPreviousLine(1);
                        printText(question->m_strQuestion);
                    }

                    commit();
                    return bUserEntryIsValid;
                }, [&](Key const& a_eKey, std::string const& a_strPrintableData) {
                    if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Default);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }

                        cv.notify_one();
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
                begin();
                for (size_t idx = 0; idx < choicesCount; ++idx) {
                    auto choice = getType<Choice>(a_vpPromptCommands, idx);
                    printText(" " + choice->visibleString(false));
                    promptText += choice->key();
                }
                commit();
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
                                setColor(SetColor::Color::BrightBlue, SetColor::Color::Default);
                                printText(" " + choice->visibleString(true));
                                resetTextFormat();
                                printNewLine();
                                commit();

                                if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                                    (*onValid)(choice->m_strValue);
                                }

                                cv.notify_one();

                                break;
                            }
                        }
                    }
                    else if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Default);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }

                        cv.notify_one();
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
                begin();
                for (size_t idx = 0; idx < choicesCount; ++idx) {
                    auto choice = getType<Choice>(a_vpPromptCommands, idx);
                    printNewLine();
                    clearLine(ClearLine::Type::All);
                    printNewLine();
                    moveCursorToPreviousLine(1);
                    printText(" - " + choice->visibleString(false));
                    promptText += choice->key();
                }
                commit();
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
                                        setColor(SetColor::Color::BrightBlue, SetColor::Color::Default);
                                        printText(choice->visibleString(true));
                                        resetTextFormat();
                                    }
                                    else {
                                        printText(choice->visibleString(true));
                                    }
                                }
                                printNewLine();
                                commit();

                                if (auto onValid = getType<OnValid>(a_vpPromptCommands)) {
                                    (*onValid)(choice->m_strValue);
                                }

                                cv.notify_one();

                                break;
                            }
                        }
                    }
                    else if (Key::Escape == a_eKey) {
                        begin();
                        setColor(SetColor::Color::BrightYellow, SetColor::Color::Default);
                        printText("<CANCELED>");
                        resetTextFormat();
                        printNewLine();
                        commit();

                        if (auto onCancel = getType<OnCancel>(a_vpPromptCommands)) {
                            (*onCancel)();
                        }

                        cv.notify_one();
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
            lock_guard<recursive_mutex> const l{ m_Mutex };
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


        void Terminal::setUserName(std::string const& a_strUserName) noexcept {
            lock_guard<recursive_mutex> const l{ m_Mutex };
            m_strCurrentUser = a_strUserName;
        }

        void Terminal::setMachineName(std::string const& a_strMachineName) noexcept {
            lock_guard<recursive_mutex> const l{ m_Mutex };
            m_strCurrentMachine = a_strMachineName;
        }

        void Terminal::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor,
                                  UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            return m_pFunctions->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
        }

        void Terminal::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor,
                                  UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            return m_pFunctions->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
        }

        void Terminal::delCommand(UserCommandInfo const& a_CommandInfo) noexcept {
            return m_pFunctions->delCommand(a_CommandInfo);
        }

        void Terminal::delAllCommands() noexcept {
            return m_pFunctions->delAllCommands();
        }

        void Terminal::execCommand(UserCommandInfo const& a_CommandInfo, UserCommandData::Args const& a_CommandArgs) noexcept {
            std::ostringstream joinedArgs;
            std::copy(a_CommandArgs.begin(), a_CommandArgs.end(), std::ostream_iterator<std::string>(joinedArgs, " "));

            lock_guard<recursive_mutex> l{ m_Mutex };
            m_vUserEntries.push_back(Functions::UserEntry{ a_CommandInfo.path + " " + joinedArgs.str(), m_strCurrentFolder });
        }

        void Terminal::setPromptEnabled(bool a_bPromptEnabled) {
            lock_guard<recursive_mutex> l{ m_Mutex };
            m_bPromptEnabled = a_bPromptEnabled;
            printCommandLine();
        }

        void Terminal::processPrintCommands(PrintCommand::VPtr const& a_vpPrintCommands) noexcept {
            if (isPrintCommandEnabled() && !isTerminalBeingResized()) {
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
            Functions::VUserEntries vUserEntries;
            {
                lock_guard<recursive_mutex> l{ m_Mutex };
                vUserEntries = m_vUserEntries;
                m_vUserEntries.clear();
            }
            for (auto const& elm : vUserEntries) {
                m_pFunctions->processEntry(elm);
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
                        m_vUserEntries.push_back(Functions::UserEntry{ m_strCurrentEntry, m_strCurrentFolder });
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
                    setColor(SetColor::Color::BrightGreen, SetColor::Color::Default);
                    printText(m_strCurrentUser + "@" + m_strCurrentMachine);
                    resetTextFormat();

                    printText(":");

                    setColor(SetColor::Color::BrightBlue, SetColor::Color::Default);
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
                setColor(SetColor::Color::Red, SetColor::Color::Default);
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
