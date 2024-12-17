#include "TerminalUnixSocket.hpp"
#include "../Tools.hpp"
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <chrono>

/* CLIENT:
 * #!/bin/bash
 * stty -icanon -echo
 * nc -U /tmp/mysocket
 * stty icanon echo
 */
namespace emb {
    namespace console {
        using namespace std;

        TerminalUnixSocket::TerminalUnixSocket(ConsoleSessionWithTerminal& a_rConsoleSession,
                                               std::string const& a_strSocketPath,
                                               std::string const& a_strShellPath) noexcept
                : TerminalAnsi{ a_rConsoleSession }
                , m_strSocketPath{ a_strSocketPath }
                , m_strShellPath{ a_strShellPath } {

            if(!m_strShellPath.empty()) {
                // Create the shell to access unix socket
                ofstream outShell{ m_strShellPath };
                outShell
                    << "#!/bin/bash" << endl
                    << "stty -icanon -echo" << endl
                    << "nc -U " << m_strSocketPath << endl
                    << "stty icanon echo" << endl;
                chmod(m_strShellPath.c_str(), ACCESSPERMS);
            }

            addCommand(emb::console::UserCommandInfo("/exit", "Exit the current shell"), [this]{
                if(m_iClientSocket > 0) {
                    TerminalAnsi::stop();
                    shutdown(m_iClientSocket, SHUT_RDWR);
                    m_bStopClient = true;
                }
            });

            addCommand(emb::console::UserCommandInfo("/color", "Enable or disable color codes usage"),
                [this](emb::console::UserCommandData const& d){
                    bool bPrintUsage{false};
                    if(0 == d.args.size()) {
                        bPrintUsage = true;
                    }
                    else if("on" == d.args.at(0)) {
                        m_bSupportsColor = true;
                    }
                    else if("off" == d.args.at(0)) {
                        m_bSupportsColor = false;
                    }
                    else {
                        d.console.printError(
                            "Unknown option " + d.args.at(0)
                        );
                        bPrintUsage = true;
                    }
                    if(bPrintUsage) {
                        d.console.printError(
                            "Usage: color on|off"
                        );
                    }
                },
                [](emb::console::UserCommandAutoCompleteData const& d) -> vector<string> {
                    if(0 == d.args.size()) {
                        // first argument
                        return emb::console::autocompletion::getChoicesFromList(d.partialArg, {"on", "off"});
                    }
                    // other arguments
                    return {};
                }
            );
        }
        //TerminalUnixSocket::TerminalUnixSocket(TerminalUnixSocket const&) noexcept = default;
        //TerminalUnixSocket::TerminalUnixSocket(TerminalUnixSocket&&) noexcept = default;
        TerminalUnixSocket::~TerminalUnixSocket() noexcept {
            string key{};
            while (read(key)) {}

            unlink(m_strShellPath.c_str());
            unlink(m_strSocketPath.c_str());
        }
        //TerminalUnixSocket& TerminalUnixSocket::operator= (TerminalUnixSocket const&) noexcept = default;
        //TerminalUnixSocket& TerminalUnixSocket::operator= (TerminalUnixSocket&&) noexcept = default;

        void TerminalUnixSocket::start() noexcept {
            m_iServerSocket = socket(AF_UNIX, SOCK_STREAM, 0);
            if (-1 == m_iServerSocket) {
                perror("TerminalUnixSocket::start(1)");
                return;
            }

            struct sockaddr_un local;
            local.sun_family = AF_UNIX;
            strcpy(local.sun_path, m_strSocketPath.c_str());
            unlink(local.sun_path);
            int len = strlen(local.sun_path) + sizeof (local.sun_family);
            if (0 != bind(m_iServerSocket, (struct sockaddr*)&local, len)) {
                perror("TerminalUnixSocket::start(2)");
                return;
            }

            if (0 != listen(m_iServerSocket, 1)) {
                perror("TerminalUnixSocket::start(3)");
                return;
            }

            chmod(m_strSocketPath.c_str(),  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

            m_ServerThread = std::thread{ &TerminalUnixSocket::serverLoop, this };
            emb::tools::thread::set_thread_name(m_ServerThread, "TrmUnxSockSrv");
        }

        void TerminalUnixSocket::processEvents() noexcept {
            string keys{};
            string key{};
            while (read(key)) {
                keys += key;
            }
            if (!keys.empty() && !parseTerminalSizeResponse(keys)) {
                processPressedKeyCode(keys);
            }
            processPrintCommands();
            processUserCommands();

            static chrono::time_point<chrono::steady_clock> timepoint{chrono::steady_clock::now()};
            if(!m_bSupportsColor) {
                Size s;
                s.iWidth = 999;
                s.iHeight = 999;
                setCurrentSize(s);
                write(" \b");
            }
            else if(timepoint <= chrono::steady_clock::now()) {
                requestTerminalSize();
                timepoint = chrono::steady_clock::now() + chrono::milliseconds(1000);
            }
        }

        void TerminalUnixSocket::stop() noexcept {
            shutdown(m_iServerSocket, SHUT_RDWR);
            shutdown(m_iClientSocket, SHUT_RDWR);
            m_bStopClient = true;
            m_bStop = true;
            m_ConditionVariableTx.notify_one();
            if(m_ServerThread.joinable()) {
                m_ServerThread.join();
            }
        }

        bool TerminalUnixSocket::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalUnixSocket::supportsColor() const noexcept {
            return m_bSupportsColor;
        }

        bool TerminalUnixSocket::read(std::string& a_rstrKey) const noexcept {
            lock_guard<mutex> const l{m_Mutex};
            bool bRes = ! m_strReceivedData.empty();
            if(bRes) {
                a_rstrKey = m_strReceivedData;
                m_strReceivedData.clear();
            }
            return bRes;
        }

        bool TerminalUnixSocket::write(std::string const& a_strDataToPrint) const noexcept {
            lock_guard<mutex> const l{m_Mutex};
            if(!m_bStopClient) {
                m_strDataToSend += a_strDataToPrint;
            }
            m_ConditionVariableTx.notify_one();
            return true;
        }

        void TerminalUnixSocket::serverLoop() noexcept {
            while (!m_bStop) {
                struct sockaddr_un remote;
                unsigned int sock_len = 0;

                // Waiting connection
                int iClientSocket = accept(m_iServerSocket, (struct sockaddr*)&remote, &sock_len);
                if (-1 != iClientSocket) {
                    // One client connected
                    m_bStopClient = false;
                    m_iClientSocket = iClientSocket;
                    m_ClientThreadRx = thread{ std::bind(&TerminalUnixSocket::clientLoopRx, this, iClientSocket) };
                    emb::tools::thread::set_thread_name(m_ClientThreadRx, "TrmUnxSockRx");
                    m_ClientThreadTx = thread{ std::bind(&TerminalUnixSocket::clientLoopTx, this, iClientSocket) };
                    emb::tools::thread::set_thread_name(m_ClientThreadTx, "TrmUnxSockTx");

                    write("Connected\n\r");
                    TerminalAnsi::start();

                    m_ClientThreadRx.join();
                    m_ClientThreadTx.join();

                    close(iClientSocket);
                }
            }
        }

        void TerminalUnixSocket::clientLoopRx(int a_iClientSocket) noexcept {
            while (!m_bStopClient) {
                char recv_buf[100+1];
                memset(recv_buf, 0, sizeof(recv_buf));

                int data_recv = recv(a_iClientSocket, recv_buf, 100, 0);
                if(data_recv > 0) {
                    lock_guard<mutex> const l{m_Mutex};
                    m_strReceivedData += recv_buf;
                }
            }
        }

        void TerminalUnixSocket::clientLoopTx(int a_iClientSocket) noexcept {
            while (!m_bStopClient) {
                {
                    std::unique_lock<mutex> lock{m_MutexTx};
                    m_ConditionVariableTx.wait(lock);
                }

                lock_guard<mutex> const l{m_Mutex};
                if(!m_strDataToSend.empty()) {
                    if(-1 == send(a_iClientSocket, m_strDataToSend.c_str(), m_strDataToSend.size()*sizeof(char), 0)) {
                        //printf("Error on send() call \n");
                        m_bStopClient = true;
                    }
                    m_strDataToSend.clear();
                }
            }
        }
    } // console
} // emb
