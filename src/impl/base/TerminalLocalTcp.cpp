#include "TerminalLocalTcp.hpp"
#include "../Tools.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#define shutdown_socket(__sock) shutdown(__sock, SD_BOTH)
#define close_socket(__sock) closesocket(__sock)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#define shutdown_socket(__sock) shutdown(__sock, SHUT_RDWR)
#define close_socket(__sock) close(__sock)
#endif

/* CLIENT:
 * #!/bin/bash
 * stty -icanon -echo
 * nc 127.0.0.1 <port>
 * stty icanon echo
 */
namespace emb {
    namespace console {
        using namespace std;

        TerminalLocalTcp::TerminalLocalTcp(ConsoleSessionWithTerminal& a_rConsoleSession, std::shared_ptr<OptionLocalTcpServer> const a_pOption) noexcept
            : TerminalAnsi{ a_rConsoleSession }
            , m_pOption{ a_pOption } {
#ifdef unix
            if (!m_pOption->strShellFilePath.empty()) {
                // Create the shell to access unix socket
                ofstream outShell{ m_pOption->strShellFilePath };
                outShell
                    << "#!/bin/bash" << endl
                    << "stty -icanon -echo" << endl
                    << "nc 127.0.0.1 " << m_pOption->iPort << endl
                    << "stty icanon echo" << endl;
                chmod(m_pOption->strShellFilePath.c_str(), ACCESSPERMS);
            }
#endif

            addCommand(emb::console::UserCommandInfo("/exit", "Exit the current shell"), [this] {
                if (m_iClientSocket > 0) {
                    TerminalAnsi::stop();
                    int status = shutdown_socket(m_iClientSocket);
                    if (status == 0) {
                        status = close_socket(m_iClientSocket);
                    }
                    m_bStopClient = true;
                }
            });
        }
        //TerminalLocalTcp::TerminalLocalTcp(TerminalLocalTcp const&) noexcept = default;
        //TerminalLocalTcp::TerminalLocalTcp(TerminalLocalTcp&&) noexcept = default;
        TerminalLocalTcp::~TerminalLocalTcp() noexcept {
            string key{};
            while (read(key)) {}

            //unlink(m_pOption->strShellPath.c_str());
        }
        //TerminalLocalTcp& TerminalLocalTcp::operator= (TerminalLocalTcp const&) noexcept = default;
        //TerminalLocalTcp& TerminalLocalTcp::operator= (TerminalLocalTcp&&) noexcept = default;

        void TerminalLocalTcp::start() noexcept {
            m_iServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (-1 == m_iServerSocket) {
                perror("TerminalLocalTcp::start(1)");
                return;
            }

            struct sockaddr_in local;
            local.sin_family = AF_INET;
            local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            local.sin_port = htons(m_pOption->iPort);
            if (0 != ::bind(m_iServerSocket, (struct sockaddr*)&local, sizeof(local))) {
                perror("TerminalLocalTcp::start(2)");
                return;
            }

            if (0 != listen(m_iServerSocket, 1)) {
                perror("TerminalLocalTcp::start(3)");
                return;
            }

            m_ServerThread = std::thread{ &TerminalLocalTcp::serverLoop, this };
            emb::tools::thread::set_thread_name(m_ServerThread, "TrmTcpSockSrv");
        }

        void TerminalLocalTcp::processEvents() noexcept {
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

            static chrono::time_point<chrono::steady_clock> timepoint{ chrono::steady_clock::now() };
            if (!m_bSupportsColor) {
                Size s;
                s.iWidth = 999;
                s.iHeight = 999;
                setCurrentSize(s);
                write(" \b");
            }
            else if (timepoint <= chrono::steady_clock::now()) {
                requestTerminalSize();
                timepoint = chrono::steady_clock::now() + chrono::milliseconds(1000);
            }
        }

        void TerminalLocalTcp::stop() noexcept {
            int status = shutdown_socket(m_iServerSocket);
            if (status == 0) {
                status = close_socket(m_iServerSocket);
            }
            status = shutdown_socket(m_iClientSocket);
            if (status == 0) {
                status = close_socket(m_iClientSocket);
            }
            m_bStopClient = true;
            m_bStop = true;
            m_ConditionVariableTx.notify_one();
            m_ServerThread.join();
        }

        bool TerminalLocalTcp::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalLocalTcp::supportsColor() const noexcept {
            return m_bSupportsColor;
        }

        bool TerminalLocalTcp::read(std::string& a_rstrKey) const noexcept {
            lock_guard<mutex> const l{ m_Mutex };
            bool bRes = !m_strReceivedData.empty();
            if (bRes) {
                a_rstrKey = m_strReceivedData;
                m_strReceivedData.clear();
            }
            return bRes;
        }

        bool TerminalLocalTcp::write(std::string const& a_strDataToPrint) const noexcept {
            lock_guard<mutex> const l{ m_Mutex };
            if (!m_bStopClient) {
                m_strDataToSend += a_strDataToPrint;
            }
            m_ConditionVariableTx.notify_one();
            return true;
        }

        void TerminalLocalTcp::serverLoop() noexcept {
            while (!m_bStop) {
                // Waiting connection
                int iClientSocket = accept(m_iServerSocket, NULL, NULL);
                if (-1 != iClientSocket) {
                    // One client connected
                    m_bStopClient = false;
                    m_iClientSocket = iClientSocket;
                    m_ClientThreadRx = thread{ std::bind(&TerminalLocalTcp::clientLoopRx, this, iClientSocket) };
                    emb::tools::thread::set_thread_name(m_ClientThreadRx, "TrmTcpSockRx");
                    m_ClientThreadTx = thread{ std::bind(&TerminalLocalTcp::clientLoopTx, this, iClientSocket) };
                    emb::tools::thread::set_thread_name(m_ClientThreadTx, "TrmTcpSockTx");

                    write("Connected\n\r");
                    TerminalAnsi::start();

                    m_ClientThreadRx.join();
                    m_ClientThreadTx.join();

                    close_socket(iClientSocket);
                }
            }
        }

        void TerminalLocalTcp::clientLoopRx(int a_iClientSocket) noexcept {
            while (!m_bStopClient) {
                char recv_buf[100 + 1];
                memset(recv_buf, 0, sizeof(recv_buf));

                int data_recv = recv(a_iClientSocket, recv_buf, 100, 0);
                if (data_recv > 0) {
                    lock_guard<mutex> const l{ m_Mutex };
                    m_strReceivedData += recv_buf;
                }
            }
        }

        void TerminalLocalTcp::clientLoopTx(int a_iClientSocket) noexcept {
            while (!m_bStopClient) {
                {
                    std::unique_lock<mutex> lock{ m_MutexTx };
                    m_ConditionVariableTx.wait(lock);
                }

                lock_guard<mutex> const l{ m_Mutex };
                if (!m_strDataToSend.empty()) {
                    if (-1 == send(a_iClientSocket, m_strDataToSend.c_str(), m_strDataToSend.size() * sizeof(char), MSG_NOSIGNAL)) {
                        //printf("Error on send() call \n");
                        m_bStopClient = true;
                    }
                    m_strDataToSend.clear();
                }
            }
        }
    } // console
} // emb