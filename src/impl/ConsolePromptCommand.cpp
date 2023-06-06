#include "EmbConsole.hpp"
#include "ConsolePrivate.hpp"
#include "base/Terminal.hpp"
#include <iostream>
#include <mutex>
#include <regex>

using namespace std;

namespace emb {
    namespace console {
        //////////////////////////////////////////////////
        ///// PromptCommand Base
        //////////////////////////////////////////////////

        PromptCommand::PromptCommand() noexcept = default;

        PromptCommand::PromptCommand(PromptCommand const& aOther) noexcept = default;

        PromptCommand::PromptCommand(PromptCommand&&) noexcept = default;

        PromptCommand::~PromptCommand() noexcept = default;

        PromptCommand& PromptCommand::operator= (PromptCommand const& aOther) noexcept = default;

        PromptCommand& PromptCommand::operator= (PromptCommand&&) noexcept = default;

        //////////////////////////////////////////////////
        ///// PromptCommands: BeginPrompt / CommitPrompt
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        ///// PromptCommands: Customizing
        //////////////////////////////////////////////////

        std::string Choice::visibleString(bool const& a_bTrimmed) const noexcept {
            std::string strVisibleChoice{ m_strChoice };
            auto pos = strVisibleChoice.find_first_of('&');
            if (std::string::npos != pos && pos + 1 < strVisibleChoice.size()) {
                if (a_bTrimmed) {
                    // ab&cdef becomes abcdef
                    strVisibleChoice.erase(pos, 1);
                }
                else {
                    // ab&cdef becomes ab[c]def
                    strVisibleChoice[pos] = '[';
                    strVisibleChoice.insert(pos + 2, "]");
                }
            }
            return strVisibleChoice;
        }
        bool Choice::isChosen(char const& a_cKey) const noexcept {
            return key() == static_cast<char>(std::toupper(static_cast<unsigned char>(a_cKey)));;
        }
        char Choice::key() const noexcept {
            char cResult{ 0 };
            auto pos = m_strChoice.find_first_of('&');
            if (std::string::npos != pos && pos + 1 < m_strChoice.size()) {
                auto c = m_strChoice[pos + 1];
                cResult = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            return cResult;
        }
        bool Validator::isValid(std::string const& a_strStrToValidate) const noexcept {
            return std::regex_match(a_strStrToValidate, std::regex{ m_strRegexValidator });
        }
    } // console
} // emb