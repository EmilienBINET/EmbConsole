#pragma once
#include "EmbConsole.hpp"
#include <string>
#include <map>
#include <vector>

namespace emb {
    namespace console {
        class ConsoleSessionWithTerminal;

        class Functions {
        public:
            enum class Error {
                NoError,
                QuoteNotClosed
            };
            struct Command {
                std::string strRawCommand;
                std::string strCurrentPath;
            };
            using VCommands = std::vector<Command>;

        public:
            Functions(ConsoleSessionWithTerminal&) noexcept;
            Functions(Functions const&);
            Functions(Functions&&) noexcept;
            virtual ~Functions() noexcept;
            Functions& operator= (Functions const&);
            Functions& operator= (Functions&&) noexcept;

            void addCommand(UserCommandInfo const&, UserCommandFunctor0 const&) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor1 const&) noexcept;
            void delCommand(UserCommandInfo const&) noexcept;

            Error processEntry(std::string const& a_strRawCommand, std::string const& a_strPath) noexcept;

            bool processAutoCompletion(std::string& a_strCurrentEntry, unsigned int const& a_uiCurrentCursorPosition, std::string const& a_strCurrentFolder, bool const& a_bNext) const noexcept;

            bool folderExists(std::string const&) const noexcept;

        public:
            static std::string getCanonicalPath(std::string const&, bool bEndWithDelimiter = false) noexcept;

        private:
            Error extractElementsFromCommand(std::string& a_rstrCommand, std::vector<std::string>& a_rvstrArguments, std::string const& a_strRawCommand) const noexcept;

        private:
            std::reference_wrapper<ConsoleSessionWithTerminal> m_rConsole;
            struct Functor {
                UserCommandInfo i;
                UserCommandFunctor0 f0;
                UserCommandFunctor1 f1;
            };
            std::map<std::string, Functor> m_mapFunctions;
        };
    } // console
} // emb