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

        void IPrintableConsole::print(std::string const& a_Data) noexcept {
            (*this) << Begin();

            string elm{};
            istringstream input{ a_Data };
            while (std::getline(input, elm, '\n')) {
                (*this) << PrintText(elm) << PrintNewLine();
            }

            (*this) << Commit();
        }

        void IPrintableConsole::printError(std::string const& a_Data) noexcept {
            (*this)
                << Begin()
                << SetColor(SetColor::Color::Red) << PrintText(a_Data) << ResetTextFormat()
                << PrintNewLine()
                << Commit();
        }

        bool IPromptableConsole::promptString(std::string const& a_strQuestion, std::string& a_rstrResult, std::string const& a_strRegexValidator) noexcept {
            bool bRes = false;
            (*this)
                << BeginPrompt()
                << Question(a_strQuestion)
                << OnCancel([&] { bRes = false; })
                << OnValid([&](std::string const& a_strResult) { bRes = true; a_rstrResult = a_strResult; });
            //if (!a_strRegexValidator.empty()) {
            //    (*this)
            //        << Validator(a_strRegexValidator);
            //}
            (*this)
                << CommitPrompt();
            return bRes;
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
            : m_pPrivateImpl{ make_unique<Private>(a_pTerminal) } {
        }

        //ConsoleSession::ConsoleSession(ConsoleSession const& aOther) noexcept
        //    : m_pPrivateImpl{ make_unique<Private>(*aOther.m_pPrivateImpl) } {
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
            return "/";
        }

        void ConsoleSession::setInstantPrint(bool a_bInstantPrint) noexcept {
            m_pPrivateImpl->setInstantPrint(a_bInstantPrint);
        }

        //////////////////////////////////////////////////
        ///// Console object
        //////////////////////////////////////////////////

        std::map<int, std::weak_ptr<Console>> s_mpConsoles{};

        std::shared_ptr<Console> Console::create(int a_iId) noexcept {
            assert(0 == s_mpConsoles.count(a_iId));
            auto pResult = make_shared<Console>();
            s_mpConsoles[a_iId] = std::weak_ptr<Console>{ pResult };
            return pResult;
        }

        std::shared_ptr<Console> Console::create(Options const& aOptions, int a_iId) noexcept {
            return make_shared<Console>(aOptions);
        }

        std::weak_ptr<Console> Console::instance(int a_iId) noexcept {
            auto it = s_mpConsoles.find(a_iId);
            if (it != s_mpConsoles.end()) {
                return it->second;
            }
            return std::weak_ptr<Console>{};
        }

        Console::Console() noexcept
            : m_pPrivateImpl{ make_unique<Private>(*this) } {
        }

        Console::Console(Console const& aOther) noexcept
            : m_pPrivateImpl{ make_unique<Private>(*aOther.m_pPrivateImpl) } {
        }

        Console::Console(Console&&) noexcept = default;

        Console::Console(int argc, char** argv) noexcept {
        }

        Console::Console(Options const& aOptions) noexcept {
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

        //string Console::getCurrentPath() const noexcept {
        //    return m_pPrivateImpl->getCurrentPath();
        //}

        void Console::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor) noexcept {
            a_CommandInfo.validate();
            m_pPrivateImpl->addCommand(a_CommandInfo, a_funcCommandFunctor);
        }

        void Console::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor) noexcept {
            a_CommandInfo.validate();
            m_pPrivateImpl->addCommand(a_CommandInfo, a_funcCommandFunctor);
        }

        void Console::setStandardOutputCapture(StandardOutputFunctor const& a_funcCaptureFunctor) noexcept {
            m_pPrivateImpl->setStandardOutputCapture(a_funcCaptureFunctor);
        }
    } // console
} // emb