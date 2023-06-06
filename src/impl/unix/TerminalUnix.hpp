#pragma once

#include "../base/TerminalAnsi.hpp"

namespace emb {
    namespace console {
        class TerminalUnix
            : public TerminalAnsi
        {
        public:
            TerminalUnix(ConsoleSessionWithTerminal&) noexcept;
            TerminalUnix(TerminalUnix const&) noexcept;
            TerminalUnix(TerminalUnix&&) noexcept;
            virtual ~TerminalUnix() noexcept;
            TerminalUnix& operator= (TerminalUnix const&) noexcept;
            TerminalUnix& operator= (TerminalUnix&&) noexcept;

        protected:
            void start() const noexcept override;
            void processEvents() noexcept override;
            void stop() const noexcept override;

            bool supportsInteractivity() const noexcept override;
            bool supportsColor() const noexcept override;

            bool read(std::string& a_rstrKey) const noexcept override;

            void printNewLine() const noexcept override;
        };
    } // console
} // emb