#include "TerminalFile.hpp"
#include <fstream>

namespace emb {
    namespace console {
        using namespace std;

        std::ofstream out{};

        TerminalFile::TerminalFile(ConsoleSessionWithTerminal& a_Console, std::string const& a_strFile) noexcept
            : Terminal{a_Console}
        {
            out.open(a_strFile, std::ios::app);
        }
        TerminalFile::~TerminalFile() noexcept {
            out.flush();
            out.close();
        }

        void TerminalFile::start() noexcept {}
        void TerminalFile::processEvents() noexcept {
            processPrintCommands();
        }
        //void TerminalFile::stop() noexcept {}

        bool TerminalFile::read(std::string& a_rstrKey) const noexcept {
            return false;
        }

        bool TerminalFile::write(std::string const& a_strDataToPrint) const noexcept {
            return false;
        }

        void TerminalFile::printNewLine() const noexcept {
            if(out.is_open()) {
                out << endl;
                out.flush();
            }
        }

        void TerminalFile::printText(std::string const& a_strText) const noexcept {
            if(out.is_open()) {
                out << a_strText;
                out.flush();
            }
        }
    } // console
} // emb
