#include "TerminalUnix.hpp"
#include "../ConsolePrivate.hpp"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <memory.h>
#include <csignal>

namespace emb {
    namespace console {
        using namespace std;

        TerminalUnix* _this{ nullptr };

        TerminalUnix::TerminalUnix(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : TerminalAnsi{ a_rConsoleSession } {
            _this = this;
        }
        //TerminalUnix::TerminalUnix(TerminalUnix const&) noexcept = default;
        //TerminalUnix::TerminalUnix(TerminalUnix&&) noexcept = default;
        TerminalUnix::~TerminalUnix() noexcept {
            _this = nullptr;
            if(m_bStarted && m_bInputEnabled) {
                string key{};
                while (read(key)) {
                }
            }
        }
        //TerminalUnix& TerminalUnix::operator= (TerminalUnix const&) noexcept = default;
        //TerminalUnix& TerminalUnix::operator= (TerminalUnix&&) noexcept = default;

        void TerminalUnix::start() noexcept {
            if(m_bStarted) {
                TerminalAnsi::start();
                // Terminal unix input only starts if stdin is valid, meaning the software is not launched headless
                if(isatty(fileno(stdin))) {
                    m_bInputEnabled = true;
                    struct termios old;
                    memset(&old, 0, sizeof(struct termios));
                    if(tcgetattr(STDIN_FILENO, &old) < 0) {
                        perror("TerminalUnix::start(1)");
                    }
                    old.c_lflag &= ~ICANON;
                    old.c_lflag &= ~ECHO;
                    old.c_cc[VMIN] = 1;
                    old.c_cc[VTIME] = 0;
                    if(tcsetattr(STDIN_FILENO, TCSANOW, &old) < 0) {
                        perror("TerminalUnix::start(2)");
                    }
                }
                Terminal::start();
                std::signal(SIGWINCH, [](int){
                    if(_this) {
                        _this->requestTerminalSize();
                    }
                });
                requestTerminalSize();
            }
        }

        void TerminalUnix::processEvents() noexcept {
            if(m_bStarted) {
                if(m_bInputEnabled) {
                    string keys{};
                    string key{};
                    while (read(key)) {
                        keys += key;
                    }
                    if (!keys.empty() && !parseTerminalSizeResponse(keys)) {
                        processPressedKeyCode(keys);
                    }
                }
                processPrintCommands();
                processUserCommands();
            }
        }

        void TerminalUnix::stop() noexcept {
            if(m_bStarted) {
                TerminalAnsi::stop();
                if(m_bInputEnabled) {
                    struct termios old;
                    memset(&old, 0, sizeof(struct termios));
                    if (tcgetattr(STDIN_FILENO, &old) < 0) {
                        perror("TerminalUnix::stop(1)");
                    }
                    old.c_lflag |= ICANON;
                    old.c_lflag |= ECHO;
                    old.c_cc[VMIN] = 1;
                    old.c_cc[VTIME] = 0;
                    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) < 0) {
                        perror("TerminalUnix::stop(2)");
                    }
                }
                std::signal(SIGWINCH, SIG_DFL);
                softReset();
            }
        }

        bool TerminalUnix::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalUnix::supportsColor() const noexcept {
            return true;
        }

        bool TerminalUnix::read(std::string& a_rstrKey) const noexcept {
            struct timeval tv = { 0L, 0L };
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            bool bKeyPressed = select(1, &fds, NULL, NULL, &tv);

            if(bKeyPressed) {
                unsigned char c;
                auto r = ::read(STDIN_FILENO, &c, sizeof(c));
                (void)r;
                a_rstrKey = c;
                return true;
            }
            return false;
        }

        void TerminalUnix::printNewLine() const noexcept {
            write("\n");
        }
    } // console
} // emb