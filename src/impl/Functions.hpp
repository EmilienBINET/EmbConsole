#pragma once
#include "EmbConsole.hpp"
#include <string>
#include <map>
#include <vector>
#include <future>

namespace emb {
    namespace console {
        class ConsoleSessionWithTerminal;

        class Functions {

        // public types
        public:
            /**
             * @brief Error that can occur when parsing a user entry
             */
            enum class Error {
                NoError,            ///< No error
                QuoteNotClosed,     ///< An open quote (" or ') was not closed
                CommandNotFound,    ///< Command is not found in the list of locally available commands
                EmptyCommand        ///< Typed command is empty
            };
            /**
             * @brief Represents what a user typed in the console before it is processed
             */
            struct UserEntry {
                std::string strUserEntry;   ///< What was typed by the used
                std::string strCurrentPath; ///< The current folder when the user typed
            };
            using VUserEntries = std::vector<UserEntry>;
            /**
             * @brief Represents a command found locally
             */
            struct LocalCommandInfo {
                bool bIsDirectory{false};       ///< Indicate if the command is a directory (or a real command otherwise)
                bool bIsRoot{false};            ///< Indicate if the command is a root command, accessible from everywhere
                std::string strName{};          ///< Name of the command
                std::string strDescription{};   ///< Description of the command
            };
            using VLocalCommandInfo = std::vector<LocalCommandInfo>;

        // public static methods
        public:
            /**
             * @brief Indicates if a given path is abolute or relative
             * @param a_Path    Path to test
             * @return true     If it is an absolute path
             * @return false    If is is a relative path or if empty
             */
            static bool isAbsolutePath(std::string const& a_Path) noexcept;
            /**
             * @brief Indicates if a given path is simple or a composed command
             * @param a_Path    Path to test
             * @return true     If it is a simple command
             * @return false    If is is a composed command
             */
            static bool isSimpleCommand(std::string const& a_Path) noexcept;
            /**
             * @brief Indicates if a command is a root command (accessible from everywhere)
             * @param a_CommandPath Complete path of the command to test
             * @return true         If the command is a root command
             * @return false        If the command is a local command
             */
            static bool isRootCommand(std::string const& a_CommandPath) noexcept;
            /**
             * @brief Indicates if a command is in a path
             * @param a_CommandPath Complete path of the command to test
             * @param a_Path        Complete path to search the command into
             * @return true         If the command is in path
             * @return false        If not
             */
            static bool isSubCommandOf(std::string const& a_CommandPath, std::string const& a_Path) noexcept;
            /**
             * @brief Gives the local name of a command from the current path
             * @param a_strCommandPath  Complete path of the command to convert to a local name
             * @param a_strCurrentPath  Current path the command is searched into
             * @return std::string  The local name of the command, ending with '/' if it is a folder. Only the name of the command, locally
             *                      accessible from the current path is kept (e.g searching "/a/b/c" in "/a" will return "b/").
             *                      Empty if the command is not a sub command of the current path.
             */
            static std::string getLocalName(std::string const& a_strCommandPath, std::string const& a_strCurrentPath) noexcept;
            /**
             * @brief Gives the canonical path of a path. e.g a/b/.././c will give a/c (or a/c/ if a_bEndWithDelimiter = true)
             * @param a_strPath             Path to compute
             * @param a_bEndWithDelimiter   Indicates if the result string must end with a final '/'
             * @return std::string  Canonical path
             */
            static std::string getCanonicalPath(std::string const& a_strPath, bool a_bEndWithDelimiter = false) noexcept;

        public:
            Functions(ConsoleSessionWithTerminal&) noexcept;
            Functions(Functions const&) = delete;
            Functions(Functions&&) noexcept = delete;
            virtual ~Functions() noexcept;
            Functions& operator= (Functions const&) = delete;
            Functions& operator= (Functions&&) noexcept = delete;

            void addCommand(UserCommandInfo const&, UserCommandFunctor0 const&, UserCommandAutoCompleteFunctor const& = nullptr) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor1 const&, UserCommandAutoCompleteFunctor const& = nullptr) noexcept;
            void delCommand(UserCommandInfo const&) noexcept;
            void delAllCommands() noexcept;

            Error processEntry(UserEntry const& a_UserEntry) noexcept;
            bool processAutoCompletion(std::string& a_strCurrentEntry, unsigned int& a_uiCurrentCursorPosition,
                                       std::string const& a_strCurrentFolder, bool const& a_bNext) noexcept;

            bool folderExists(std::string const&) const noexcept;
            VLocalCommandInfo getCommands(std::string const& a_strCurrentPath) const noexcept;

        // private types
        private:
            struct Functor {
                UserCommandInfo i;
                UserCommandFunctor0 f0;
                UserCommandFunctor1 f1;
                UserCommandAutoCompleteFunctor fa;
            };

        // private methods
        private:
            /**
             * @brief Tries to extract element from a user entry. Extracted elements are : the command, and the arguments.
             * @param a_rstrCommand     Extracted command from the entry
             * @param a_rvstrArguments  Vector of arguments extacted from the entry.
             * @param a_strUserEntry    Input User entry
             * @return Error    Resulting error indicating if the extraction went well
             */
            Error extractElementsFromUserEntry(std::string& a_rstrCommand, std::vector<std::string>& a_rvstrArguments,
                                             std::string const& a_strUserEntry) const noexcept;
            /**
             * @brief Tries to find a command matching a user entry
             * @param a_rFunctor        Functor information from the found command
             * @param a_rstrCommand     Extracted command from the entry
             * @param a_rvstrArguments  Vector of arguments extacted from the entry.
             * @param a_strUserEntry    Input User entry
             * @param a_strPath         Current path
             * @return Error    Resulting error indicating if the extraction went well
             */
            Error searchCommand(Functor& a_rFunctor, std::string& a_rstrCommand, std::vector<std::string>& a_rvstrArguments,
                                std::string const& a_strUserEntry, std::string const& a_strPath) const noexcept;

            std::vector<std::string> getAutoCompleteChoices(std::string const& a_strPartialCmd, std::string const& a_strCurrentFolder) const noexcept;

        private:
            std::reference_wrapper<ConsoleSessionWithTerminal> m_rConsole;
            std::map<std::string, Functor> m_mapFunctions;
            std::future<void> m_future;
            std::string m_strLastAutoCompletionPrefix{};
            std::string m_strLastAutoCompletionPrefixWithoutPartialArg{};
            size_t m_ullAutoCompletionPosition{ -1ULL };
            std::vector<std::string> m_vstrAutoCompletionChoices{};
        };
    } // console
} // emb
