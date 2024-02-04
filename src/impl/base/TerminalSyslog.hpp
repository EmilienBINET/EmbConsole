#pragma once

#include "Terminal.hpp"

namespace emb {
    namespace console {

        class ConsoleSessionWithTerminal;

        class TerminalSyslog : public Terminal {
        public:
            TerminalSyslog(ConsoleSessionWithTerminal&, std::shared_ptr<OptionSyslog> const) noexcept;
            TerminalSyslog(TerminalSyslog const&) noexcept = delete;
            TerminalSyslog(TerminalSyslog&&) noexcept = delete;
            virtual ~TerminalSyslog() noexcept;
            TerminalSyslog& operator= (TerminalSyslog const&) noexcept = delete;
            TerminalSyslog& operator= (TerminalSyslog&&) noexcept = delete;

            void start() noexcept override;
            void processEvents() noexcept override;
            void stop() noexcept override;

            bool supportsInteractivity() const noexcept override { return false; }
            bool supportsColor() const noexcept override { return false; }

            bool read(std::string& a_rstrKey) const noexcept override;
            bool write(std::string const& a_strDataToPrint) const noexcept override;
            
            void printText(std::string const& a_strText) const noexcept override;

        private:
            std::shared_ptr<OptionSyslog> m_pOption{};
            int m_iSocket{};
            void* m_pDestination{nullptr};
        };
    } // console
} // emb