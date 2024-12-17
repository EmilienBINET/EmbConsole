#include "EmbConsole.hpp"
#include "ConsolePrivate.hpp"
#include <sstream>
#include <string>

using namespace std;

namespace emb {
    namespace console {
        //////////////////////////////////////////////////
        ///// Console stream object
        //////////////////////////////////////////////////

        IPrintableConsole& IPrintableConsole::operator<< (char const* a_szData) noexcept {
            *this << PrintText(a_szData);
            return *this;
        }

        IPrintableConsole& IPrintableConsole::operator<< (std::string const& a_strData) noexcept {
            *this << PrintText(a_strData);
            return *this;
        }

        void IPrintableConsole::print(std::string const& a_Data) noexcept {
            (*this) << Begin() << ClearLine(ClearLine::Type::All);

            string elm{};
            istringstream input{ a_Data };
            while (std::getline(input, elm, '\n')) {
                (*this) << PrintText(elm) << PrintNewLine();
            }

            (*this) << Commit();
        }

        void IPrintableConsole::printError(std::string const& a_Data) noexcept {
            (*this) << Begin() << ClearLine(ClearLine::Type::All) << SetColor(SetColor::Color::Red);

            string elm{};
            istringstream input{ a_Data };
            while (std::getline(input, elm, '\n')) {
                (*this) << PrintText(elm) << PrintNewLine();
            }

            (*this) << ResetTextFormat() << Commit();
        }

        bool IPromptableConsole::promptString(std::string const& a_strQuestion, std::string& a_rstrResult, std::string const& a_strRegexValidator, std::string const& a_strErrorMessage) noexcept {
            bool bRes = false;
            (*this)
                << BeginPrompt()
                << Question(a_strQuestion)
                << OnCancel([&] { bRes = false; })
                << OnValid([&](std::string const& a_strResult) { bRes = true; a_rstrResult = a_strResult; });
            if (!a_strRegexValidator.empty()) {
                (*this)
                    << Validator(a_strRegexValidator, a_strErrorMessage);
            }
            (*this)
                << CommitPrompt();
            return bRes;
        }

        bool IPromptableConsole::promptIpv4(std::string const& a_strQuestion, std::string& a_rstrResult, std::string const& a_strErrorMessage) noexcept {
            return promptString(a_strQuestion, a_rstrResult, "((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}", a_strErrorMessage);
        }

        bool IPromptableConsole::promptNumber(std::string const& a_strQuestion, long& a_rlResult) noexcept {
            bool bRes = false;
            (*this)
                << BeginPrompt()
                << Question(a_strQuestion)
                << ValidatorNumber()
                << OnCancel([&] { bRes = false; })
                << OnValidLong([&](long const& a_lResult) { bRes = true; a_rlResult = a_lResult; })
                << CommitPrompt();
            return bRes;
        }

        bool IPromptableConsole::promptYesNo(std::string const& a_strQuestion, bool& a_rbResult) noexcept {
            bool bRes = false;
            (*this)
                << BeginPrompt()
                << Question(a_strQuestion)
                << Choice("&Yes", true)
                << Choice("&No", false)
                << OnCancel([&] { bRes = false; })
                << OnValidBool([&](bool const& a_bResult) { bRes = true; a_rbResult = a_bResult; })
                << CommitPrompt();
            return bRes;
        }

        bool IPromptableConsole::promptChoice(std::string const& a_strQuestion, std::unordered_map<std::string, std::string> const& a_mapChoices, std::string& a_rstrResult)
        {
            bool bRes = false;
            (*this)
                << BeginPrompt()
                << Question(a_strQuestion)
                << OnCancel([&] { bRes = false; })
                << OnValidString([&](std::string const& a_strResult) { bRes = true; a_rstrResult = a_strResult; });
            for (auto const& choice : a_mapChoices) {
                (*this) << Choice(choice.first, choice.second);
            }
            (*this) << CommitPrompt();
            return bRes;
        }

        //////////////////////////////////////////////////
        ///// Console session object
        //////////////////////////////////////////////////

        ConsoleSession::ConsoleSession(TerminalPtr a_pTerminal) noexcept
            : m_pPrivateImpl{ emb::tools::memory::make_unique<Private>(a_pTerminal) } {
        }

        //ConsoleSession::ConsoleSession(ConsoleSession const& aOther) noexcept
        //    : m_pPrivateImpl{ emb::tools::memory::make_unique<Private>(*aOther.m_pPrivateImpl) } {
        //}

        ConsoleSession::ConsoleSession(ConsoleSession&&) noexcept = default;

        ConsoleSession::~ConsoleSession() noexcept {
        }

        //ConsoleSession& ConsoleSession::operator= (ConsoleSession const& aOther) noexcept {
        //    *m_pPrivateImpl = *aOther.m_pPrivateImpl;
        //    return *this;
        //}

        ConsoleSession& ConsoleSession::operator= (ConsoleSession&&) noexcept = default;

        IPrintableConsole& ConsoleSession::operator<< (PrintCommand const& a_Cmd) noexcept {
            *m_pPrivateImpl << a_Cmd.copy();
            return *this;
        }

        IPromptableConsole& ConsoleSession::operator<< (PromptCommand const& a_Cmd) noexcept {
            *m_pPrivateImpl << a_Cmd.copy();
            return *this;
        }

        std::string ConsoleSession::getCurrentPath() const noexcept {
            return m_pPrivateImpl->getCurrentPath();
        }

        void ConsoleSession::setInstantPrint(bool a_bInstantPrint) noexcept {
            m_pPrivateImpl->setInstantPrint(a_bInstantPrint);
        }

        //////////////////////////////////////////////////
        ///// Console object
        //////////////////////////////////////////////////

        std::map<int, std::weak_ptr<Console>> s_mpConsoles{};

        std::shared_ptr<Console> Console::create(int a_iId) noexcept {
            return create(Options{}, a_iId);
        }

        std::shared_ptr<Console> Console::create(Options const& aOptions, int a_iId) noexcept {
            assert(0 == s_mpConsoles.count(a_iId));
            auto pResult = make_shared<Console>(aOptions);
            s_mpConsoles[a_iId] = std::weak_ptr<Console>{ pResult };
            return pResult;
        }

        std::weak_ptr<Console> Console::instance(int a_iId) noexcept {
            auto it = s_mpConsoles.find(a_iId);
            if (it != s_mpConsoles.end()) {
                return it->second;
            }
            return std::weak_ptr<Console>{};
        }

        void Console::showWindowsStdConsole() noexcept {
            for (auto const& elm : s_mpConsoles) {
                if (auto pConsole = elm.second.lock()) {
                    pConsole->m_pPrivateImpl->showWindowsStdConsole();
                }
            }
        }

        void Console::hideWindowsStdConsole() noexcept {
            for (auto const& elm : s_mpConsoles) {
                if (auto pConsole = elm.second.lock()) {
                    pConsole->m_pPrivateImpl->hideWindowsStdConsole();
                }
            }
        }

        Console::Console() noexcept
            : m_pPrivateImpl{ emb::tools::memory::make_unique<Private>(*this) } {
        }

        Console::Console(Console const& aOther) noexcept
            : m_pPrivateImpl{ emb::tools::memory::make_unique<Private>(*aOther.m_pPrivateImpl) } {
        }

        Console::Console(Console&&) noexcept = default;

        Console::Console(int argc, char** argv) noexcept {
        }

        Console::Console(Options const& aOptions) noexcept
            : m_pPrivateImpl{ emb::tools::memory::make_unique<Private>(*this, aOptions) } {
        }

        Console::~Console() noexcept {
        }

        Console& Console::operator= (Console const& aOther) noexcept {
            *m_pPrivateImpl = *aOther.m_pPrivateImpl;
            return *this;
        }

        Console& Console::operator= (Console&&) noexcept = default;

        IPrintableConsole& Console::operator<< (PrintCommand const& a_Cmd) noexcept {
            *m_pPrivateImpl << a_Cmd;
            return *this;
        }

        IPrintableConsole& Console::operator<< (char const* a_szData) noexcept {
            return IPrintableConsole::operator<<(a_szData);
        }

        IPrintableConsole& Console::operator<< (std::string const& a_strData) noexcept {
            return IPrintableConsole::operator<<(a_strData);
        }

        void Console::setUserName(std::string const& a_strUserName) noexcept {
            m_pPrivateImpl->setUserName(a_strUserName);
        }

        void Console::setMachineName(std::string const& a_strMachineName) noexcept {
            m_pPrivateImpl->setMachineName(a_strMachineName);
        }

        void Console::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor,
            UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            a_CommandInfo.validate();
            m_pPrivateImpl->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
        }

        void Console::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor,
            UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            a_CommandInfo.validate();
            m_pPrivateImpl->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
        }

        void Console::delCommand(UserCommandInfo const& a_CommandInfo) noexcept {
            a_CommandInfo.validate();
            m_pPrivateImpl->delCommand(a_CommandInfo);
        }

        void Console::delAllCommands() noexcept {
            m_pPrivateImpl->delAllCommands();
        }

        void Console::execCommand(UserCommandInfo const& a_CommandInfo, UserCommandData::Args const& a_CommandArgs) noexcept {
            m_pPrivateImpl->execCommand(a_CommandInfo, a_CommandArgs);
        }

        void Console::setStandardOutputCapture(StandardOutputFunctor const& a_funcCaptureFunctor) noexcept {
            m_pPrivateImpl->setStandardOutputCapture(a_funcCaptureFunctor);
        }

        void Console::setPromptEnabled(bool a_bPromptEnabled) noexcept {
            m_pPrivateImpl->setPromptEnabled(a_bPromptEnabled);
        }
    } // console
} // emb