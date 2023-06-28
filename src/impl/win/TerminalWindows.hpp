#pragma once

#include "../base/TerminalAnsi.hpp"
#include <thread>
#include <atomic>

namespace emb {
    namespace console {
        class TerminalWindows
            : public TerminalAnsi
        {
        public:
            TerminalWindows(ConsoleSessionWithTerminal&) noexcept;
            TerminalWindows(TerminalWindows const&) noexcept = delete;
            TerminalWindows(TerminalWindows&&) noexcept = delete;
            virtual ~TerminalWindows() noexcept;
            TerminalWindows& operator= (TerminalWindows const&) noexcept = delete;
            TerminalWindows& operator= (TerminalWindows&&) noexcept = delete;

        protected:
            void start() noexcept override;
            void processEvents() noexcept override;
            void stop() noexcept override;

            bool supportsInteractivity() const noexcept override;
            bool supportsColor() const noexcept override;

            bool read(std::string& a_rstrKey) const noexcept override;

        private:
            void inputLoop() noexcept;
            void requestTerminalSize() noexcept;

        private:
            mutable unsigned long m_ulPreviousInputMode{};
            mutable unsigned long m_ulPreviousOutputMode{};
            std::thread m_InputThread{};
            std::atomic<bool> m_bSizeChanged{false};
        };
    } // console
} // emb
