#pragma once

#include "../base/TerminalAnsi.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace emb {
    namespace console {
        class TerminalUnixSocket
            : public TerminalAnsi
        {
        public:
            TerminalUnixSocket(ConsoleSessionWithTerminal&) noexcept;
            TerminalUnixSocket(TerminalUnixSocket const&) noexcept;
            TerminalUnixSocket(TerminalUnixSocket&&) noexcept;
            virtual ~TerminalUnixSocket() noexcept;
            TerminalUnixSocket& operator= (TerminalUnixSocket const&) noexcept;
            TerminalUnixSocket& operator= (TerminalUnixSocket&&) noexcept;

        protected:
            void start() noexcept override;
            bool isKeyPressed(std::string& a_rstrPressedKey) noexcept;
            void processEvents() noexcept override;
            void stop() noexcept override;

            bool supportsInteractivity() const noexcept override;
            bool supportsColor() const noexcept override;

            bool read(std::string& a_rstrKey) const noexcept override;
            bool write(std::string const& a_strDataToPrint) const noexcept override;

        private:
            void serverLoop() noexcept;
            void clientLoopRx(int a_iClientSocket) noexcept;
            void clientLoopTx(int a_iClientSocket) noexcept;

        private:
            mutable std::thread m_ServerThread{};
            mutable std::thread m_ClientThreadRx{};
            mutable std::thread m_ClientThreadTx{};
            mutable std::atomic<bool> m_bStop{ false };
            mutable std::atomic<bool> m_bStopClient{ false };
            mutable std::mutex m_Mutex{};
            mutable std::mutex m_MutexTx{};
            mutable std::condition_variable m_ConditionVariableTx{};
            mutable std::string m_strReceivedData{};
            mutable std::string m_strDataToSend{};
            mutable int m_iServerSocket{ 0 };
            mutable int m_iClientSocket{ 0 };
        };
    } // console
} // emb