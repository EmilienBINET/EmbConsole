#pragma once

#include "Terminal.hpp"

namespace emb {
    namespace console {

        class ConsoleSessionWithTerminal;

        class TerminalFile : public Terminal {
        public:
            TerminalFile(ConsoleSessionWithTerminal&, std::string const&) noexcept;
            TerminalFile(TerminalFile const&) noexcept = delete;
            TerminalFile(TerminalFile&&) noexcept = delete;
            virtual ~TerminalFile() noexcept;
            TerminalFile& operator= (TerminalFile const&) noexcept = delete;
            TerminalFile& operator= (TerminalFile&&) noexcept = delete;

            void start() noexcept override;
            void processEvents() noexcept override;
            void stop() noexcept override;

            bool supportsInteractivity() const noexcept override { return false; }
            bool supportsColor() const noexcept override { return false; }

            bool read(std::string& a_rstrKey) const noexcept override;
            bool write(std::string const& a_strDataToPrint) const noexcept override;
            
            void printNewLine() const noexcept override;
            void printText(std::string const& a_strText) const noexcept override;
        };
    } // console
} // emb