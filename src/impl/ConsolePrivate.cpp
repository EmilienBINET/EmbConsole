#include "ConsolePrivate.hpp"
#include "EmbConsole.hpp"
#include "StdCapture.hpp"
#ifdef WIN32
#include "win/TerminalWindows.hpp"
#include "win/TerminalWindowsLegacy.hpp"
#endif
#ifdef unix
#include "unix/TerminalUnix.hpp"
#include "unix/TerminalUnixSocket.hpp"
#endif
#include "base/TerminalFile.hpp"
#include "base/TerminalSyslog.hpp"
#include "base/TerminalLocalTcp.hpp"
#include "Tools.hpp"

namespace emb {
    namespace console {
        using namespace std;
        using namespace std::chrono_literals;

        StdCapture ConsoleSessionWithTerminal::m_StdCapture{};
        StandardOutputFunctor ConsoleSessionWithTerminal::m_funcCaptureFunctor{};
        std::thread ConsoleSessionWithTerminal::m_CaptureThread{};
        std::atomic<bool> ConsoleSessionWithTerminal::m_bStopThread{ true };
        std::function<void(void)> ConsoleSessionWithTerminal::m_funcPeriodicCapture{};

        ConsoleSession::Private::Private(TerminalPtr a_pTerminal) noexcept
            : m_pTerminal{ a_pTerminal } {
        }

        //ConsoleSession::Private::Private(Private const& a_Obj) noexcept = default;

        //ConsoleSession::Private::Private(Private&& a_Obj) noexcept = default;

        ConsoleSession::Private::~Private() noexcept = default;

        //ConsoleSession::Private& ConsoleSession::Private::operator= (Private const&) noexcept = default;

        //ConsoleSession::Private& ConsoleSession::Private::operator= (Private&&) noexcept = default;

        ConsoleSession::Private& ConsoleSession::Private::operator<< (PrintCommand::Ptr const& a_pCmd) noexcept {
            if (dynamic_pointer_cast<Begin>(a_pCmd))
            {
                m_PrintMutex.lock();
                m_bLocalInstantPrint = false;
            }
            lock_guard<mutex> l{ m_Mutex };
            if (dynamic_pointer_cast<InstantPrint>(a_pCmd)) {
                m_bLocalInstantPrint = true;
            }
            else {
                m_PrintCommands.push_back(a_pCmd);
            }
            if (dynamic_pointer_cast<Commit>(a_pCmd))
            {
                m_pTerminal->setPrintCommands(m_PrintCommands, m_bInstantPrint | m_bLocalInstantPrint);
                m_PrintMutex.unlock();
                m_PrintCommands.clear();
            }
            return *this;
        }

        ConsoleSession::Private& ConsoleSession::Private::operator<<(PromptCommand::Ptr const& a_Cmd) noexcept {
            if (dynamic_pointer_cast<BeginPrompt>(a_Cmd))
            {
                m_PromptMutex.lock();
            }
            lock_guard<mutex> l{ m_Mutex2 };
            m_PromptCommands.push_back(a_Cmd);
            if (dynamic_pointer_cast<CommitPrompt>(a_Cmd))
            {
                m_pTerminal->setPromptCommands(m_PromptCommands);
                m_PromptMutex.unlock();
                m_PromptCommands.clear();
            }
            return *this;
        }

        void ConsoleSession::Private::setInstantPrint(bool a_bInstantPrint) noexcept {
            m_bInstantPrint = a_bInstantPrint;
        }

        string ConsoleSession::Private::getCurrentPath() const noexcept {
            return m_pTerminal->getCurrentPath();
        }

        Console::Private::Private(Console& a_rConsole, Options const& a_Options) noexcept
            : m_Options{ a_Options } {
            applyOptions(false);
            m_Thread = std::thread{ &Private::run, this };
            emb::tools::thread::set_thread_name(m_Thread, "Console");
        }

        Console::Private::Private(Private const& a_Obj) noexcept {
        }

        Console::Private::Private(Private&& a_Obj) noexcept {
        }

        Console::Private::~Private() noexcept {
            m_Stop = true;
            m_Thread.join();
        }

        Console::Private& Console::Private::operator= (Private const&) noexcept {
            return *this;
        }

        Console::Private& Console::Private::operator= (Private&&) noexcept {
            return *this;
        }

        Console::Private& Console::Private::operator<< (PrintCommand const& a_Cmd) noexcept {
            for (auto const& console : m_ConsolesVector) {
                *console << a_Cmd;
            }
            return *this;
        }

        void Console::Private::showWindowsStdConsole() noexcept {
#ifdef WIN32
            // Create the external STD console
            TerminalWindows::createStdConsole();
            // Reapply options so that the windows terminal can be created
            applyOptions(true);
#endif
        }

        void Console::Private::hideWindowsStdConsole() noexcept {
#ifdef WIN32
            // Remove the windows terminal from the list
            removeTerminalIfExists<TerminalWindows>();
            removeTerminalIfExists<TerminalWindowsLegacy>();
            // Destroy the external STD console
            TerminalWindows::destroyStdConsole();
#endif
        }

        void Console::Private::setUserName(std::string const& a_strUserName) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->setUserName(a_strUserName);
            }
        }

        void Console::Private::setMachineName(std::string const& a_strMachineName) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->setMachineName(a_strMachineName);
            }
        }

        void Console::Private::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor0 const& a_funcCommandFunctor,
            UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
            }
        }

        void Console::Private::addCommand(UserCommandInfo const& a_CommandInfo, UserCommandFunctor1 const& a_funcCommandFunctor,
            UserCommandAutoCompleteFunctor const& a_funcAutoCompleteFunctor) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->addCommand(a_CommandInfo, a_funcCommandFunctor, a_funcAutoCompleteFunctor);
            }
        }

        void Console::Private::delCommand(UserCommandInfo const& a_CommandInfo) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->delCommand(a_CommandInfo);
            }
        }

        void Console::Private::delAllCommands() noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->delAllCommands();
            }
        }

        void Console::Private::execCommand(UserCommandInfo const& a_CommandInfo, UserCommandData::Args const& a_CommandArgs) noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->execCommand(a_CommandInfo, a_CommandArgs);
                break;
            }
        }

        void Console::Private::setStandardOutputCapture(StandardOutputFunctor const& a_funcCaptureFunctor) noexcept {
            ConsoleSessionWithTerminal::setStandardOutputCapture(a_funcCaptureFunctor);
        }

        void Console::Private::setPromptEnabled(bool a_bPromptEnabled) noexcept {
            m_bPromptEnabled = a_bPromptEnabled;
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->setPromptEnabled(a_bPromptEnabled);
            }
        }

        void Console::Private::start() noexcept {
            ConsoleSessionWithTerminal::beginStdCapture();
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->start();
            }
        }

        void Console::Private::processEvents() noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->processEvents();
            }
        }

        void Console::Private::stop() noexcept {
            for (auto const& console : m_ConsolesVector) {
                console->terminal()->stop();
            }
            ConsoleSessionWithTerminal::endStdCapture();
        }

        void Console::Private::run() {
            start();
            while (!m_Stop) {
                processEvents();
                this_thread::sleep_for(10ms);
            }
            stop();
        }

        template<typename T>
        std::shared_ptr<T> getTerminal(std::vector<std::unique_ptr<ConsoleSessionWithTerminal>> & a_rConsoleVector) {
            std::shared_ptr<T> pOut{};
            for (auto const& elm : a_rConsoleVector) {
                if (auto pTerminal = std::dynamic_pointer_cast<T>(elm->terminal())) {
                    pOut = pTerminal;
                }
            }
            return pOut;
        }

        template<typename T>
        void removeTerminalIfExists(std::vector<std::unique_ptr<ConsoleSessionWithTerminal>> & a_rConsoleVector) {
            for (std::vector<std::unique_ptr<ConsoleSessionWithTerminal>>::reverse_iterator it = a_rConsoleVector.rbegin();
                it != a_rConsoleVector.rend();
                ++it) {
                if (auto pTerminal = std::dynamic_pointer_cast<T>((*it)->terminal())) {
                    (*it)->terminal()->stop();
                    a_rConsoleVector.erase(std::next(it).base());
                }
            }
        }

        void Console::Private::applyOptions(bool a_bAutoStart) {
#ifdef WIN32
            auto pOptStd = m_Options.get<OptionStd>();
            if (pOptStd) {
                auto pTerminalWindows = getTerminal<TerminalWindows>();
                auto pTerminalWindowsLegacy = getTerminal<TerminalWindowsLegacy>();
                if (pOptStd->bEnabled && TerminalWindows::isCreatable()) {
                    // Create terminal only if another one does not already exist
                    if (!pTerminalWindows && !pTerminalWindowsLegacy) {
                        if (TerminalWindows::isSupported()) {
                            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalWindows>>());
                        }
                        else {
                            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalWindowsLegacy>>());
                        }
                        if (a_bAutoStart) {
                            if (auto pAddedTerminal = m_ConsolesVector.back()->terminal()) {
                                pAddedTerminal->start();
                                pAddedTerminal->setPromptEnabled(m_bPromptEnabled);
                            }
                        }
                    }
                }
                else {
                    removeTerminalIfExists<TerminalWindows>();
                    removeTerminalIfExists<TerminalWindowsLegacy>();
                }
            }
#endif
#ifdef unix
            auto pOptStd = m_Options.get<OptionStd>();
            if (pOptStd) {
                if (pOptStd->bEnabled && !getTerminal<TerminalUnix>(m_ConsolesVector)) {
                    m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalUnix>>());
                }
                else {
                    removeTerminalIfExists<TerminalUnix>(m_ConsolesVector);
                }
            }
            auto pOptUnixSocket = m_Options.get<OptionUnixSocket>();
            if (pOptUnixSocket) {
                if (pOptUnixSocket->bEnabled && !getTerminal<TerminalUnixSocket>(m_ConsolesVector)) {
                    m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalUnixSocket>>(
                        pOptUnixSocket->strSocketFilePath, pOptUnixSocket->strShellFilePath));
                }
                else {
                    removeTerminalIfExists<TerminalUnixSocket>(m_ConsolesVector);
                }
            }
#endif
            auto pOptFile = m_Options.get<OptionFile>();
            if (pOptFile) {
                if (pOptFile->bEnabled && !getTerminal<TerminalFile>(m_ConsolesVector)) {
                    m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalFile>>(pOptFile->strFilePath));
                }
                else {
                    removeTerminalIfExists<TerminalFile>(m_ConsolesVector);
                }
            }
            auto pOptSyslog = m_Options.get<OptionSyslog>();
            if (pOptSyslog) {
                if (pOptSyslog->bEnabled && !getTerminal<TerminalSyslog>(m_ConsolesVector)) {
                    m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalSyslog>>(pOptSyslog));
                }
                else {
                    removeTerminalIfExists<TerminalSyslog>(m_ConsolesVector);
                }
            }
            auto pOptTcp = m_Options.get<OptionLocalTcpServer>();
            if (pOptTcp) {
                if (pOptTcp->bEnabled && !getTerminal<TerminalLocalTcp>(m_ConsolesVector)) {
                    m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalLocalTcp>>(pOptTcp));
                }
                else {
                    removeTerminalIfExists<TerminalLocalTcp>(m_ConsolesVector);
                }
            }
        }
    } // console
} // emb