#include "Functions.hpp"
#include "ConsolePrivate.hpp"
#include <sstream>
#include <algorithm>
#include <vector>

namespace emb {
    namespace console {
        using namespace std;

        bool Functions::isAbsolutePath(std::string const& a_Path) noexcept {
            // Absolute path if not empty and starts with a '/'
            return a_Path.size() > 0 && '/' == a_Path.at(0);
        }

        bool Functions::isSimpleCommand(std::string const& a_Path) noexcept {
            // Is a simple command if path does not contain any '/'
            return string::npos == a_Path.find("/");
        }

        bool Functions::isRootCommand(string const& a_CommandPath) noexcept {
            // Root command only one '/' is found and and it is the 1st character
            return 0 == a_CommandPath.find_last_of('/');
        }

        bool Functions::isSubCommandOf(string const& a_CommandPath, string const& a_Path) noexcept {
            // Sub command if the command path starts with path
            // getCanonicalPath is used here to add the final '/' if needed
            return 0 == a_CommandPath.find(getCanonicalPath(a_Path, true));
        }

        string Functions::getLocalName(string const& a_strCommandPath, string const& a_strCurrentPath) noexcept {
            string strLocalName{};

            // We search inside the complete command path if the current path can be found
            string strCurrentPath{ Functions::getCanonicalPath(a_strCurrentPath, true) };
            size_t ulPos = a_strCommandPath.find(strCurrentPath);

            // if the current path was found at position 0, it means that the command is a sub command of the current path
            if(0 == ulPos) {
                // Then we keep only what is at the right of the path : the local name
                strLocalName = a_strCommandPath.substr(strCurrentPath.size());

                // Then we search if the local name is a folder (if it as sub command, it will contain the '/' separator)
                ulPos = strLocalName.find_first_of('/');
                if(string::npos != ulPos) {
                    // And we remove the sub command, keeping the local name in the format "directory/"
                    strLocalName = strLocalName.substr(0, ulPos+1);
                }
            }

            return strLocalName;
        }

        string Functions::getCanonicalPath(std::string const& a_strPath, bool a_bEndWithDelimiter) noexcept {
            // List of each element inside the path (each element is either a directory or a command)
            vector<string> vstrResult{};

            // We split the input path at each "/" and run some code on each token
            string strToken{};
            istringstream streamInput{ a_strPath };
            while (getline(streamInput, strToken, '/')) {
                if (".." == strToken) {
                    // If the token is .. we need to remove the last token if it exists
                    if (vstrResult.size() > 0) {
                        vstrResult.pop_back();
                    }
                }
                else if (!strToken.empty() && "." != strToken) {
                    // if the token is empty or ., we do nothing, otherwise, we add the token to the list of elements
                    vstrResult.push_back(strToken);
                }
            }

            // We then join the remaining elements of the list into a single string, elements are separated by a '/'
            string strResult{};
            for (auto const& elm : vstrResult) {
                strResult += "/" + elm;
            }
            if (strResult.empty()) {
                // If it is the root, we keep only a /
                strResult = "/";
            }

            // If it is requested to end with a separator, we add it if needed
            if(a_bEndWithDelimiter && strResult.find_last_of('/') != strResult.size() - 1) {
                strResult += "/";
            }

            return strResult;
        }

        Functions::Functions(ConsoleSessionWithTerminal& a_rConsole) noexcept : m_rConsole{ a_rConsole } {
            addCommand(UserCommandInfo("/ls", "List information about the commands"), [&](UserCommandData const& a_CmdData) {
                string output{};
                bool bAll = a_CmdData.args.size() > 0 && a_CmdData.args.at(0).find('a') != string::npos;
                bool bLongListing = a_CmdData.args.size() > 0 && a_CmdData.args.at(0).find('l') != string::npos;

                if (bAll) {
                    output += "===== Global commands =====\n";
                    for (auto const& elm : m_mapFunctions) {
                        if (isRootCommand(elm.first)) {
                            output += elm.first + "\t" + (bLongListing ? elm.second.i.description + "\n" : "");
                        }
                    }
                    output += std::string(bLongListing ? "" : "\n") + "===== Local commands =====\n";
                    for (auto const& elm : m_mapFunctions) {
                        if (!isRootCommand(elm.first)) {
                            output += elm.first + "\t" + (bLongListing ? elm.second.i.description + "\n" : "");
                        }
                    }
                }
                else {
                    vector<LocalCommandInfo> vecLocalCommand = getCommands(a_CmdData.console.getCurrentPath());
                    for (auto const& elm : vecLocalCommand) {
                        if (elm.bIsRoot) { // root command => available from anywhere
                            output += elm.strName + "\t" + (bLongListing ? elm.strDescription + "\n" : "");
                        }
                    }
                    output += bLongListing ? "" : "\n";
                    for (auto const& elm : vecLocalCommand) {
                        if(!elm.bIsRoot) { // Other command => available from folder
                            if(!elm.bIsDirectory) { // command
                                output += elm.strName + "\t" + (bLongListing ? elm.strDescription + "\n" : "");
                            }
                            else { // folder
                                output += elm.strName + "\t" + (bLongListing ? "\n" : "");
                            }
                        }
                    }
                }
                output += bLongListing ? "" : "\n";

                a_CmdData.console.print(output);
            });

            addCommand(UserCommandInfo("/pwd", "Print the name of the current working directory"), [&](UserCommandData const& a_CmdData) {
                a_CmdData.console.print(a_CmdData.console.getCurrentPath());
            });

            addCommand(UserCommandInfo("/help", "Display information about commands"), [&](UserCommandData const& a_CmdData) {
                if (a_CmdData.args.size() <= 0) {
                    a_CmdData.console.printError("Usage: help <cmd>");
                }
                else {
                    string cmd = a_CmdData.args.at(0);
                    if (a_CmdData.args.at(0).at(0) != '/') {
                        cmd = getCanonicalPath(a_CmdData.console.getCurrentPath() + "/" + cmd);
                    }
                    auto const& it = m_mapFunctions.find(cmd);
                    if (it != m_mapFunctions.end()) {
                        if (it->second.i.help.empty()) {
                            a_CmdData.console.printError("No help available for '" + cmd + "' command");
                        }
                        else {
                            a_CmdData.console.print(it->second.i.help);
                        }
                    }
                    else {
                        a_CmdData.console.printError("Command '" + cmd + "' not found");
                    }
                }
            });
        }

        //Functions::Functions(Functions const&) = default;
        //Functions::Functions(Functions&&) noexcept = default;
        Functions::~Functions() noexcept = default;
        //Functions& Functions::operator= (Functions const&) = default;
        //Functions& Functions::operator= (Functions&&) noexcept = default;

        void Functions::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor,
                                   UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            Functor f;
            f.i = a_CommandInfo;
            f.f0 = a_funcCommandFunctor;
            f.fa = a_funcAutoCompleteFunctor;
            m_mapFunctions[a_CommandInfo.path] = f;
        }

        void Functions::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor,
                                   UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            Functor f;
            f.i = a_CommandInfo;
            f.f1 = a_funcCommandFunctor;
            f.fa = a_funcAutoCompleteFunctor;
            m_mapFunctions[a_CommandInfo.path] = f;
        }

        void Functions::delCommand(UserCommandInfo const& a_CommandInfo) noexcept {
            m_mapFunctions.erase(a_CommandInfo.path);
        }

        void Functions::delAllCommands() noexcept {
            m_mapFunctions.clear();
        }

        Functions::Error Functions::processEntry(UserEntry const& a_UserEntry) noexcept {
            Functor f;
            string strCommand{};
            vector<string> vstrArguments{};
            Error result{ searchCommand(f, strCommand, vstrArguments, a_UserEntry.strUserEntry, a_UserEntry.strCurrentPath) };

            if (Error::NoError == result) {
                if (f.f0) {
                    m_future = std::async(std::launch::async, [=] {
                        f.f0();
                    });
                }
                else if (f.f1) {
                    m_future = std::async(std::launch::async, [fct = f.f1, info = f.i, terminal = m_rConsole.get().terminal(), vstrArguments]{
                        ConsoleSession session{ terminal };
                        session.setInstantPrint(true);
                        UserCommandData data{ info, session, vstrArguments };
                        fct(data);
                    });
                }
            }
            else if (Error::EmptyCommand != result) {
                m_rConsole.get().printError("Cannot find command '" + strCommand + "'");
            }

            return result;
        }

        bool Functions::processAutoCompletion(std::string& a_strCurrentEntry, unsigned int& a_uiCurrentCursorPosition, std::string const& a_strCurrentFolder, bool const& a_bNext) noexcept {
            string strAutoCompletionPrefix{ a_strCurrentEntry.substr(0, a_uiCurrentCursorPosition) };

            bool bRes = false;
            if (strAutoCompletionPrefix.find(" ") != std::string::npos) {
                // Spaces on the left of the cursor, so the auto completion is for command parameters (and not command itself)
                // We need to transmit the autocompletion data to the command autocompletion function if it exists
                if (m_strLastAutoCompletionPrefix != strAutoCompletionPrefix ||
                    (-1ULL == m_ullAutoCompletionPosition && strAutoCompletionPrefix.empty())) {
                    m_strLastAutoCompletionPrefix = strAutoCompletionPrefix;
                    m_strLastAutoCompletionPrefixWithoutPartialArg = m_strLastAutoCompletionPrefix.substr(0, m_strLastAutoCompletionPrefix.find_last_of(' ') + 1);
                    m_ullAutoCompletionPosition = 0;
                    m_vstrAutoCompletionChoices.clear();

                    string word = m_strLastAutoCompletionPrefix.substr(m_strLastAutoCompletionPrefix.find_last_of(' ') + 1);

                    Functor f;
                    string strCommand{};
                    vector<string> vstrArguments{};
                    Error result{ searchCommand(f, strCommand, vstrArguments, m_strLastAutoCompletionPrefixWithoutPartialArg, a_strCurrentFolder) };

                    if (Error::NoError == result && f.fa) {
                        UserCommandAutoCompleteData data{ vstrArguments, word };
                        m_vstrAutoCompletionChoices = f.fa(data);
                    }
                }
                else {
                    if (a_bNext) {
                        if (m_ullAutoCompletionPosition < m_vstrAutoCompletionChoices.size() - 1) {
                            ++m_ullAutoCompletionPosition;
                        }
                        else {
                            m_ullAutoCompletionPosition = 0;
                        }
                    }
                    else { // previous
                        if (m_ullAutoCompletionPosition > 0) {
                            --m_ullAutoCompletionPosition;
                        }
                        else {
                            m_ullAutoCompletionPosition = m_vstrAutoCompletionChoices.size() - 1;
                        }
                    }
                }
                if (m_ullAutoCompletionPosition < m_vstrAutoCompletionChoices.size()) {
                    a_strCurrentEntry = m_strLastAutoCompletionPrefixWithoutPartialArg + m_vstrAutoCompletionChoices.at(m_ullAutoCompletionPosition);
                    // If only one choice is possible, we move the cursor to the end of the entry
                    if(1 == m_vstrAutoCompletionChoices.size()) {
                        a_uiCurrentCursorPosition = a_strCurrentEntry.size();
                    }
                    bRes = true;
                }
            }
            else {
                // No space on the left of the cursor, so the auto completion is for the command

                if (m_strLastAutoCompletionPrefix != strAutoCompletionPrefix ||
                    (-1ULL == m_ullAutoCompletionPosition && strAutoCompletionPrefix.empty())) {
                    m_strLastAutoCompletionPrefix = strAutoCompletionPrefix;
                    m_ullAutoCompletionPosition = 0;
                    m_vstrAutoCompletionChoices.clear();

                    m_vstrAutoCompletionChoices = getAutoCompleteChoices(strAutoCompletionPrefix, a_strCurrentFolder);
                }
                else {
                    if (a_bNext) {
                        if (m_ullAutoCompletionPosition < m_vstrAutoCompletionChoices.size() - 1) {
                            ++m_ullAutoCompletionPosition;
                        }
                        else {
                            m_ullAutoCompletionPosition = 0;
                        }
                    }
                    else { // previous
                        if (m_ullAutoCompletionPosition > 0) {
                            --m_ullAutoCompletionPosition;
                        }
                        else {
                            m_ullAutoCompletionPosition = m_vstrAutoCompletionChoices.size() - 1;
                        }
                    }
                }
                if (m_ullAutoCompletionPosition < m_vstrAutoCompletionChoices.size()) {
                    a_strCurrentEntry = m_vstrAutoCompletionChoices.at(m_ullAutoCompletionPosition);
                    // If only one choice is possible, we move the cursor to the end of the entry
                    if(1 == m_vstrAutoCompletionChoices.size()) {
                        a_uiCurrentCursorPosition = a_strCurrentEntry.size();
                        // And if the entry is not a folder, we add a space to be able to entrer arguments righ now
                        if('/' != a_strCurrentEntry.at(a_strCurrentEntry.size()-1)) {
                            a_strCurrentEntry += " ";
                            ++a_uiCurrentCursorPosition;
                        }
                    }
                    bRes = true;
                }
            }
            return bRes;
        }

        bool Functions::folderExists(std::string const& a_strFolder) const noexcept {
            string strFolderToTest = getCanonicalPath(a_strFolder, true);
            for (auto const& elm : m_mapFunctions) {
                if(elm.first.find(strFolderToTest) == 0) {
                    return true;
                }
            }
            return false;
        }

        std::vector<Functions::LocalCommandInfo> Functions::getCommands(std::string const& a_strCurrentPath) const noexcept {
            std::vector<LocalCommandInfo> vecCmds{};

            for (auto const& elm : m_mapFunctions) {
                if (isRootCommand(elm.first)) { // root command => available from anywhere
                    vecCmds.push_back(LocalCommandInfo{false, true, elm.first.substr(1), elm.second.i.description});
                }
            }

            vector<string> vecPrintedFolders;
            for (auto const& elm : m_mapFunctions) {
                if(!isRootCommand(elm.first) &&
                    isSubCommandOf(elm.first, a_strCurrentPath)) { // Other command => available from folder
                    string folder = getLocalName(elm.first, a_strCurrentPath);
                    if(std::count(vecPrintedFolders.begin(), vecPrintedFolders.end(), folder) == 0) {
                        vecPrintedFolders.push_back(folder);
                        if(string::npos == folder.find('/')) { // command
                            vecCmds.push_back(LocalCommandInfo{false, false, folder, elm.second.i.description});
                        }
                        else { // folder
                            vecCmds.push_back(LocalCommandInfo{true, false, folder, elm.second.i.description});
                        }
                    }
                }
            }

            return vecCmds;
        }

        Functions::Error Functions::extractElementsFromUserEntry(string& a_rstrCommand, vector<string>& a_rvstrArguments, string const& a_strUserEntry) const noexcept {
            enum class Token
            {
                Command,
                Arguments
            } token = Token::Command;
            unsigned int uiIdxArg{ 0 };
            char cQuote{ 0 };

            auto addCharToArguments = [&](char c) {
                if (a_rvstrArguments.size() <= uiIdxArg) {
                    a_rvstrArguments.push_back(std::string());
                }
                a_rvstrArguments[uiIdxArg] += c;
            };
            auto goToNextArgument = [&]() {
                if (uiIdxArg < a_rvstrArguments.size()) {
                    if (!a_rvstrArguments[uiIdxArg].empty()) {
                        ++uiIdxArg;
                    }
                }
            };

            char c_prev{ 0 };
            for (auto const& c : a_strUserEntry) {
                if (c == ' ' || c == '\t') {
                    if (Token::Command == token) {
                        token = Token::Arguments;
                    }
                    else if (Token::Arguments == token) {
                        if (0 != cQuote) {
                            addCharToArguments(c);
                        }
                        else {
                            goToNextArgument();
                        }
                    }
                }
                else if (c == '\'' || c == '"') {
                    if (Token::Arguments == token) {
                        if (c_prev == '\\' || (cQuote != 0 && cQuote != c)) {
                            addCharToArguments(c);
                        }
                        else {
                            if (0 == cQuote) {
                                cQuote = c;
                            }
                            else if (cQuote == c) {
                                cQuote = 0;
                            }
                        }
                    }
                }
                else if (c == '\\') {
                    if (Token::Arguments == token) {
                        if (c_prev == '\\') {
                            addCharToArguments(c);
                        }
                    }
                }
                else {
                    if (Token::Command == token) {
                        a_rstrCommand += c;
                    }
                    else if (Token::Arguments == token) {
                        addCharToArguments(c);
                    }
                }
                c_prev = c;
            }

            return 0 == cQuote ? Error::NoError : Error::QuoteNotClosed;
        }

        Functions::Error Functions::searchCommand(Functor& a_rFunctor, string& a_rstrCommand, vector<string>& a_rvstrArguments,
                                                  string const& a_strUserEntry, string const& a_strPath) const noexcept {
            Error result{ Error::NoError };

            result = extractElementsFromUserEntry(a_rstrCommand, a_rvstrArguments, a_strUserEntry);

            if (Error::NoError == result) {
                auto it = m_mapFunctions.end();
                if (isAbsolutePath(a_rstrCommand)) {
                    auto it1 = m_mapFunctions.find(getCanonicalPath(a_rstrCommand));
                    if (it1 != m_mapFunctions.end()) {
                        it = it1;
                    }
                }
                else { // relative path
                    string fullPath{ getCanonicalPath(a_strPath + "/" + a_rstrCommand) };
                    auto it1 = m_mapFunctions.find(fullPath);
                    if (it1 != m_mapFunctions.end()) {
                        it = it1;
                    }
                    else if (isSimpleCommand(a_rstrCommand)) {
                        auto it2 = m_mapFunctions.find("/" + a_rstrCommand);
                        if (it2 != m_mapFunctions.end()) {
                            it = it2;
                        }
                    }
                }
                if (it != m_mapFunctions.end()) {
                    a_rFunctor = it->second;
                    result = Error::NoError;
                }
                else if (!a_rstrCommand.empty()) {
                    result = Error::CommandNotFound;
                }
                else {
                    result = Error::EmptyCommand;
                }
            }

            return result;
        }

        vector<string> Functions::getAutoCompleteChoices(string const& a_strPartialCmd, string const& a_strCurrentFolder) const noexcept {
            // We need to create a choices list among which the user can naviguate using <Tab> and <RTab> keyboard keys
            vector<string> vecChoices{};

            // We get the position of the last '/' in the argument the user is typing
            size_t ulPos = a_strPartialCmd.find_last_of('/');

            // We need to find information about what the user is typing:
            string strPrefixFolder{}; // what complete folder the user typed
            string strPartialCmd{a_strPartialCmd}; // what partial folder the user started to typed
            string strCurrentFolder{a_strCurrentFolder}; // what folder we need to search the choices into
            if(string::npos != ulPos) {
                // if a complete folder has already been typed, it is the part before the last '/'
                strPrefixFolder = a_strPartialCmd.substr(0, ulPos+1);
                // and then the partial arg is now the part after the last '/'
                strPartialCmd = a_strPartialCmd.substr(ulPos+1);
                // the current folder we need to search the choices into is constructed from the current folder where the "cd"
                // command is typed and the complete folder the user typed as an argument of "cd". However, if the prefix
                // folder is an absolute directory, the current folder is not used.
                // e.g. if the current directory is /w/x
                // cd abcd/ef<Tab> => strPrefixFolder is "abcd/", strPartialCmd is "ef", strCurrentFolder is /w/x/abcd
                // cd /abcd/ef<Tab> => strPrefixFolder is "/abcd/", strPartialCmd is "ef", strCurrentFolder is /abcd
                if (isAbsolutePath(strPrefixFolder)) {
                    strCurrentFolder = Functions::getCanonicalPath(strPrefixFolder);
                }
                else {
                    strCurrentFolder = Functions::getCanonicalPath(strCurrentFolder + "/" + strPrefixFolder);
                }
            }

            // We search the commands available from the current folder
            auto vecLocalCommands = getCommands(strCurrentFolder);

            // We filter the found command and only keep the directories that start with strPartialCmd
            for(auto const& elm : vecLocalCommands) {
                if(0 == elm.strName.find(strPartialCmd)) {
                    // We filter the root commands when using a full path
                    if(strPrefixFolder.empty() || (!strPrefixFolder.empty() && !elm.bIsRoot) || (strCurrentFolder == "/" && elm.bIsRoot)) {
                        vecChoices.push_back(strPrefixFolder + elm.strName);
                    }
                }
            }

            // We return the generated list of the possible choices to the user
            return vecChoices;
        }

    } // console
} // emb
