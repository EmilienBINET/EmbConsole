#include "TerminalFile.hpp"
#include <fstream>

namespace emb {
    namespace console {
        using namespace std;

        std::ofstream out{};

        TerminalFile::TerminalFile(ConsoleSessionWithTerminal& a_Console, std::string const& a_strFile) noexcept 
            : Terminal{a_Console}
        {
            out = ofstream{a_strFile, std::ios::ate};
        }
        TerminalFile::~TerminalFile() noexcept {}

        void TerminalFile::start() const noexcept {}
        void TerminalFile::processEvents() noexcept {
            processPrintCommands();
        }
        void TerminalFile::stop() const noexcept {}

        bool TerminalFile::read(std::string& a_rstrKey) const noexcept {
            return false;
        }

        bool TerminalFile::write(std::string const& a_strDataToPrint) const noexcept {
            return false;
        }

        void TerminalFile::printNewLine() const noexcept {
            out << endl;
            out.flush();
        }

        void TerminalFile::printText(std::string const& a_strText) const noexcept {
            out << a_strText;
            // write(a_strText);
        }
    } // console
} // emb
