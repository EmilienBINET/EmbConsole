#include "ConsolePrivate.hpp"
#include "StdCapture.hpp"
#ifdef WIN32
#include "win/TerminalWindows.hpp"
#endif
#ifdef unix
#include "unix/TerminalUnix.hpp"
#include "unix/TerminalUnixSocket.hpp"
#endif
#include "base/TerminalFile.hpp"

namespace emb {
    namespace console {
        using namespace std;
        using namespace std::chrono_literals;

        StdCapture ConsoleSessionWithTerminal::m_StdCapture{};
        StandardOutputFunctor ConsoleSessionWithTerminal::m_funcCaptureFunctor{};

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
            }
            lock_guard<mutex> l{ m_Mutex };
            m_PrintCommands.push_back(a_pCmd);
            if (dynamic_pointer_cast<Commit>(a_pCmd))
            {
                m_pTerminal->setPrintCommands(m_PrintCommands, m_bInstantPrint);
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

        Console::Private::Private(Console& a_rConsole) noexcept {
#ifdef WIN32
            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalWindows>>());
#endif
#ifdef unix
            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalUnix>>());
            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalUnixSocket>>());
#endif
            m_ConsolesVector.push_back(make_unique<TConsoleSessionWithTerminal<TerminalFile>>("/tmp/mylogs.log"));
            m_Thread = std::thread{ &Private::run, this };
            pthread_setname_np(m_Thread.native_handle(), "Console");
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

        void Console::Private::setStandardOutputCapture(StandardOutputFunctor const& a_funcCaptureFunctor) noexcept {
            ConsoleSessionWithTerminal::setStandardOutputCapture(a_funcCaptureFunctor);
        }

        void Console::Private::setPromptEnabled(bool a_bPromptEnabled) noexcept {
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
    } // console
} // emb
