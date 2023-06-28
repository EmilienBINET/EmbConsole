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
            static StdCapture& stdCapture() {
                static StdCapture stdCapture;
                return stdCapture;
            }
        protected:
            ConsoleSessionWithTerminal(TerminalPtr a_pTerminal)
                : ConsoleSession{ a_pTerminal }
                , m_pTerminal{ a_pTerminal } {
            }
        private:
            TerminalPtr m_pTerminal{};
        };

        template<typename TerminalType>
        class TConsoleSessionWithTerminal : public ConsoleSessionWithTerminal {
        public:
            TConsoleSessionWithTerminal()
                : ConsoleSessionWithTerminal{ std::make_unique<TerminalType>(*this) } {
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
        private:
            TerminalPtr m_pTerminal{};

            std::mutex m_Mutex{};
            std::recursive_mutex m_PrintMutex{};
            PrintCommand::VPtr m_PrintCommands{};

            std::mutex m_Mutex2{};
            std::recursive_mutex m_PromptMutex{};
            PromptCommand::VPtr m_PromptCommands{};

            bool m_bInstantPrint{ false };
        };

        class Console::Private : protected ITerminal {
            friend class Console;

        public:
            Private(Console&) noexcept;
            Private(Private const&) noexcept;
            Private(Private&&) noexcept;
            virtual ~Private() noexcept;
            Private& operator= (Private const&) noexcept;
            Private& operator= (Private&&) noexcept;
            Private& operator<< (PrintCommand const& a_Cmd) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor0 const&) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor1 const&) noexcept;

        private:
            void start() const noexcept override;
            void processEvents() noexcept override;
            void stop() const noexcept override;
            void run();

        private:
            std::thread m_Thread{};
            volatile std::atomic_bool m_Stop{ false };
            std::vector<std::unique_ptr<ConsoleSessionWithTerminal>> m_ConsolesVector{};
        };
    } // console
} // emb