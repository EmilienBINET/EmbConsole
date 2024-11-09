#pragma once

#include "EmbConsole.hpp"
#include "Functions.hpp"
#include "StdCapture.hpp"
#include "base/ITerminal.hpp"
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

namespace emb {
    namespace console {
        class ConsoleSessionWithTerminal : public ConsoleSession {
        public:
            TerminalPtr terminal() const {
                return m_pTerminal;
            }
            static void setStandardOutputCapture(StandardOutputFunctor const& a_funcCaptureFunctor) {
                m_funcCaptureFunctor = a_funcCaptureFunctor;
                m_bStopThread = true;
                if (m_CaptureThread.joinable()) {
                    m_CaptureThread.join();
                }
                if (m_funcCaptureFunctor) {
                    m_bStopThread = false;
                    m_CaptureThread = std::thread{ [] {
                        while (!m_bStopThread) {
                            if (m_funcPeriodicCapture) {
                                m_funcPeriodicCapture();
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    } };
                }
            }
            static void beginStdCapture() {
                if (m_funcCaptureFunctor) {
                    m_StdCapture.BeginCapture();

                    std::string strCaptured = m_StdCapture.GetCapture();
                    if (strCaptured.size() > 0) {
                        m_funcCaptureFunctor(strCaptured);
                    }
                }
            }
            static void endStdCapture() {
                if (m_funcCaptureFunctor) {
                    m_StdCapture.EndCapture();
                }
            }
            static void setCaptureEndEvt(std::function<void(void)> const& a_fctCaptureEnd) {
                m_StdCapture.setCaptureEndEvt(a_fctCaptureEnd);
            }
            static void setPeriodicCapture(std::function<void(void)> const& a_funcPeriodicCaptureFunctor) {
                m_funcPeriodicCapture = a_funcPeriodicCaptureFunctor;
            }
            ~ConsoleSessionWithTerminal() {
                m_bStopThread = true;
                if (m_CaptureThread.joinable()) {
                    m_CaptureThread.join();
                }
            }
        protected:
            ConsoleSessionWithTerminal(TerminalPtr a_pTerminal)
                : ConsoleSession{ a_pTerminal }
                , m_pTerminal{ a_pTerminal } {
            }
        private:
            TerminalPtr m_pTerminal{};
            static StdCapture m_StdCapture;
            static StandardOutputFunctor m_funcCaptureFunctor;
            static std::thread m_CaptureThread;
            static std::atomic<bool> m_bStopThread;
            static std::function<void(void)> m_funcPeriodicCapture;
        };

        template<typename TerminalType>
        class TConsoleSessionWithTerminal : public ConsoleSessionWithTerminal {
        public:
            TConsoleSessionWithTerminal()
                : ConsoleSessionWithTerminal{ std::make_unique<TerminalType>(*this) } {
            }
            template<typename... U>
            TConsoleSessionWithTerminal(U&&... u)
                : ConsoleSessionWithTerminal{ std::make_unique<TerminalType>(*this, std::forward<U>(u)...) } {
            }
        };

        class ConsoleSession::Private {
        public:
            Private(TerminalPtr) noexcept;
            Private(Private const&) noexcept = delete;
            Private(Private&&) noexcept = delete;
            virtual ~Private() noexcept;
            Private& operator= (Private const&) noexcept = delete;
            Private& operator= (Private&&) noexcept = delete;
            Private& operator<< (PrintCommand::Ptr const&) noexcept;
            Private& operator<< (PromptCommand::Ptr const&) noexcept;
            void setInstantPrint(bool a_bInstantPrint) noexcept;
            std::string getCurrentPath() const noexcept;
        private:
            TerminalPtr m_pTerminal{};

            std::mutex m_Mutex{};
            std::recursive_mutex m_PrintMutex{};
            PrintCommand::VPtr m_PrintCommands{};

            std::mutex m_Mutex2{};
            std::recursive_mutex m_PromptMutex{};
            PromptCommand::VPtr m_PromptCommands{};

            bool m_bInstantPrint{ false };
            bool m_bLocalInstantPrint{ false };
        };

        class Console::Private : protected ITerminal {
            friend class Console;

        public:
            Private(Console&, Options const& = Options{}) noexcept;
            Private(Private const&) noexcept;
            Private(Private&&) noexcept;
            virtual ~Private() noexcept;
            Private& operator= (Private const&) noexcept;
            Private& operator= (Private&&) noexcept;
            Private& operator<< (PrintCommand const& a_Cmd) noexcept;
            void showWindowsStdConsole() noexcept;
            void hideWindowsStdConsole() noexcept;
            void setUserName(std::string const&) noexcept;
            void setMachineName(std::string const&) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor0 const&, UserCommandAutoCompleteFunctor const&) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor1 const&, UserCommandAutoCompleteFunctor const&) noexcept;
            void delCommand(UserCommandInfo const&) noexcept;
            void delAllCommands() noexcept;
            void execCommand(UserCommandInfo const&, UserCommandData::Args const&) noexcept;
            void setStandardOutputCapture(StandardOutputFunctor const&) noexcept;
            void setPromptEnabled(bool) noexcept;

        private:
            void start() noexcept override;
            void processEvents() noexcept override;
            void stop() noexcept override;
            void run();
            void applyOptions(bool a_bAutoStart);

            template<typename T>
            std::shared_ptr<T> getTerminal() {
                std::shared_ptr<T> pOut{};
                for (auto const& elm : m_ConsolesVector) {
                    if (auto pTerminal = std::dynamic_pointer_cast<T>(elm->terminal())) {
                        pOut = pTerminal;
                    }
                }
                return pOut;
            }

            template<typename T>
            void removeTerminalIfExists() {
                for (decltype(m_ConsolesVector)::reverse_iterator it = m_ConsolesVector.rbegin();
                    it != m_ConsolesVector.rend();
                    ++it) {
                    if (auto pTerminal = std::dynamic_pointer_cast<T>((*it)->terminal())) {
                        (*it)->terminal()->stop();
                        m_ConsolesVector.erase(std::next(it).base());
                    }
                }
            }

        private:
            std::thread m_Thread{};
            volatile std::atomic_bool m_Stop{ false };
            std::vector<std::unique_ptr<ConsoleSessionWithTerminal>> m_ConsolesVector{};
            Options m_Options{};
            bool m_bPromptEnabled{ false };
        };
    } // console
} // emb