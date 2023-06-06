#pragma once

namespace emb {
    namespace console {
        class ITerminal {
        public:
            ITerminal() noexcept;
            ITerminal(ITerminal const&) noexcept;
            ITerminal(ITerminal&&) noexcept;
            virtual ~ITerminal() noexcept;
            ITerminal& operator= (ITerminal const&) noexcept;
            ITerminal& operator= (ITerminal&&) noexcept;

            virtual void start() const noexcept = 0;
            virtual void processEvents() noexcept = 0;
            virtual void stop() const noexcept = 0;
        };
    } // console
} // emb