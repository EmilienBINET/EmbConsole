#include "Functions.hpp"
#include "ConsolePrivate.hpp"
#include <sstream>
#include <future>
#include <algorithm>

namespace emb {
    namespace console {
        using namespace std;

        bool isRootCommand(string const& a_CommandPath) {
            return 0 == a_CommandPath.find_last_of('/');
        }
        bool isNotRootCommand(string const& a_CommandPath) {
            return !isRootCommand(a_CommandPath);
        }
        bool isSubCommandOf(string const& a_CommandPath, string const& a_Path) {
            return 0 == a_CommandPath.find(a_Path[a_Path.length() - 1] == '/' ? a_Path : a_Path + "/");
        }

        Functions::Functions(ConsoleSessionWithTerminal& a_rConsole) noexcept : m_rConsole{ a_rConsole } {
            addCommand(UserCommandInfo("/ls", "List information about the commands"), [&](UserCommandData const& a_CmdData) {
                string output{ "===== LIST OF AVAILABLE COMMANDS =====\n" };
                bool bAll = a_CmdData.args.size() > 0 && a_CmdData.args.at(0).find('a') != string::npos;
                bool bLongListing = a_CmdData.args.size() > 0 && a_CmdData.args.at(0).find('l') != string::npos;

                if (bAll) {
                    output += "== Local commands ==\n";
                    for (auto const& elm : m_mapFunctions) {
                        if (isNotRootCommand(elm.first)) {
                            output += elm.first + "\t" + (bLongListing ? elm.second.i.description + "\n" : "");
                        }
                    }
                    output += std::string(bLongListing ? "" : "\n") + "== Global commands ==\n";
                    for (auto const& elm : m_mapFunctions) {
                        if (isRootCommand(elm.first)) {
                            output += elm.first + "\t" + (bLongListing ? elm.second.i.description + "\n" : "");
                        }
                    }
                }
                else {
                    for (auto const& elm : m_mapFunctions) {
                        if (isRootCommand(elm.first) || // root command => available from anywhere
                            isSubCommandOf(elm.first, a_CmdData.console.getCurrentPath())) { // Other command => available from folder
                            output += elm.first + "\t" + (bLongListing ? elm.second.i.description + "\n" : "");
                        }
                    }
                }
                output += std::string(bLongListing ? "" : "\n") + "======================================\n";

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
        Functions::Functions(Functions const&) = default;
        Functions::Functions(Functions&&) noexcept = default;
        Functions::~Functions() noexcept = default;
        Functions& Functions::operator= (Functions const&) = default;
        Functions& Functions::operator= (Functions&&) noexcept = default;

        void Functions::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor) noexcept {
            Functor f;
            f.i = a_CommandInfo;
            f.f0 = a_funcCommandFunctor;
            m_mapFunctions[a_CommandInfo.path] = f;
        }

        void Functions::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor) noexcept {
            Functor f;
            f.i = a_CommandInfo;
            f.f1 = a_funcCommandFunctor;
            m_mapFunctions[a_CommandInfo.path] = f;
        }

        std::future<void> m_future;

        Functions::Error Functions::processEntry(string const& a_strRawCommand, string const& a_strPath) noexcept {
            Error result{ Error::NoError };

            string strCommand{};
            vector<string> vstrArguments{};
            result = extractElementsFromCommand(strCommand, vstrArguments, a_strRawCommand);

            if (Error::NoError == result) {
                string fullPath{ getCanonicalPath(a_strPath + "/" + strCommand) };
                decltype(m_mapFunctions)::iterator it = m_mapFunctions.end();
                auto it1 = m_mapFunctions.find(fullPath);
                if (it1 != m_mapFunctions.end()) {
                    it = it1;
                }
                else {
                    auto it2 = m_mapFunctions.find("/" + strCommand);
                    if (it2 != m_mapFunctions.end()) {
                        it = it2;
                    }
                }
                if (it != m_mapFunctions.end()) {
                    auto& elm = it->second;
                    if (elm.f0) {
                        m_future = std::async(std::launch::async, [=] {
                            elm.f0();
                        });
                    }
                    else if (elm.f1) {
                        m_future = std::async(std::launch::async, [fct = elm.f1, info = it->second.i, terminal = m_rConsole.get().terminal(), vstrArguments]{
                            ConsoleSession session{ terminal };
                            session.setInstantPrint(true);
                            UserCommandData data{ info, session, vstrArguments };
                            fct(data);
                            });
                    }
                }
                else if (!strCommand.empty()) {
                    m_rConsole.get().printError("Cannot find command '" + strCommand + "'");
                }
            }
            return Error::NoError;
        }

        string m_strLastAutoCompletionPrefix{};
        size_t m_ullAutoCompletionPosition{ -1ULL };
        vector<string> m_vstrAutoCompletionChoices{};

        bool Functions::processAutoCompletion(std::string& a_strCurrentEntry, unsigned int const& a_uiCurrentCursorPosition, std::string const& a_strCurrentFolder, bool const& a_bNext) const noexcept {
            string strAutoCompletionPrefix{ a_strCurrentEntry.substr(0, a_uiCurrentCursorPosition) };

            bool bRes = false;
            if (strAutoCompletionPrefix.find(" ") != std::string::npos) {
                // Spaces on the left of the cursor, so the auto completion is for command parameters (and not command itself)
                // We nee to transmit the autocompletion data to the command autocompletion function if it exists
                /// @todo
            }
            else {
                // No space on the left of the cursor, so the auto completion is for the command

                if (m_strLastAutoCompletionPrefix != strAutoCompletionPrefix ||
                    (-1ULL == m_ullAutoCompletionPosition && strAutoCompletionPrefix.empty())) {
                    m_strLastAutoCompletionPrefix = strAutoCompletionPrefix;
                    m_ullAutoCompletionPosition = 0;
                    m_vstrAutoCompletionChoices.clear();

                    // Search all commands in the current folder
                    string strSearch{ getCanonicalPath(a_strCurrentFolder + "/" + strAutoCompletionPrefix) };
                    if (strAutoCompletionPrefix.empty()) {
                        // If nothing is typed yet, we need to ad a "/" to avoid searching outside the folder
                        strSearch += "/";
                    }
                    for (auto const& elm : m_mapFunctions) {
                        // If it starts with the search pattern, it must be in the list
                        if (0 == elm.first.find(strSearch)) {
                            m_vstrAutoCompletionChoices.push_back(elm.first);
                        }
                    }

                    // Search amongst global commands
                    strSearch = getCanonicalPath("/" + strAutoCompletionPrefix);
                    for (auto const& elm : m_mapFunctions) {
                        if (isRootCommand(elm.first)) {
                            if (0 == elm.first.find(strSearch) &&
                                0 == count(m_vstrAutoCompletionChoices.begin(), m_vstrAutoCompletionChoices.end(), elm.first)) {
                                m_vstrAutoCompletionChoices.push_back(elm.first);
                            }
                        }
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
                    a_strCurrentEntry = m_vstrAutoCompletionChoices.at(m_ullAutoCompletionPosition);
                    if (a_strCurrentFolder == "/") {
                        if (strAutoCompletionPrefix.find_first_of('/') == std::string::npos) {
                            a_strCurrentEntry = a_strCurrentEntry.substr(1);
                        }
                    }
                    else {
                        if (strAutoCompletionPrefix.find_first_of('/') == std::string::npos) {
                            if (isRootCommand(a_strCurrentEntry)) {
                                a_strCurrentEntry = a_strCurrentEntry.substr(1);
                            }
                            else {
                                a_strCurrentEntry = a_strCurrentEntry.substr(a_strCurrentFolder.length() + 1);
                            }
                        }
                    }
                    //if (isRootCommand(a_strCurrentEntry)) {
                    //    // base command, juste remove te first character
                    //    a_strCurrentEntry = a_strCurrentEntry.substr(1);
                    //}
                    //else {
                    //    strAutoCompletionPrefix = getCanonicalPath(a_strCurrentFolder + "/" + strAutoCompletionPrefix);
                    //    a_strCurrentEntry = a_strCurrentEntry.substr(a_strCurrentEntry.find(strAutoCompletionPrefix) + strAutoCompletionPrefix.length());
                    //}
                    bRes = true;
                    /*
                     * cas :  DIR    STR     RESULT
                     *        /      ""      retirer le premier /
                     *        /      ab      retirer le premier /
                     *        /      /ab     ne rien retirer
                     *        /a/    ""      retirer /a/ si local, retirer / si global
                     *        /a/    ab      retirer /a/ si local, retirer / si global
                     *        /a/    /ab     ne rien retirer
                     *
                     */
                }
            }
            return bRes;
        }

        string Functions::getCanonicalPath(string const& a_strFullPath) noexcept {
            vector<string> vstrResult{};

            string elm{};
            istringstream input{ a_strFullPath };
            while (getline(input, elm, '/')) {
                if (".." == elm) {
                    if (vstrResult.size() > 0) {
                        vstrResult.pop_back();
                    }
                }
                else if (!elm.empty() && "." != elm) {
                    vstrResult.push_back(elm);
                }
            }

            string strResult{};
            for (auto const& elm : vstrResult) {
                strResult += "/" + elm;
            }
            if (strResult.empty()) {
                strResult = "/";
            }

            return strResult;
        }

        Functions::Error Functions::extractElementsFromCommand(string& a_rstrCommand, vector<string>& a_rvstrArguments, string const& a_strRawCommand) const noexcept {
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
            for (auto const& c : a_strRawCommand) {
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
    } // console
} // emb