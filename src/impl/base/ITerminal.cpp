#include "ITerminal.hpp"

namespace emb {
    namespace console {
        using namespace std;

        ITerminal::ITerminal() noexcept = default;
        ITerminal::ITerminal(ITerminal const&) noexcept = default;
        ITerminal::ITerminal(ITerminal&&) noexcept = default;
        ITerminal::~ITerminal() noexcept = default;
        ITerminal& ITerminal::operator= (ITerminal const&) noexcept = default;
        ITerminal& ITerminal::operator= (ITerminal&&) noexcept = default;
    } // console
} // emb