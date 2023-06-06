#include "TerminalUnixSocket.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

/* CLIENT:
 * #!/bin/bash
 * stty -icanon -echo && nc -U /tmp/mysocket
 */
namespace emb {
    namespace console {
        using namespace std;

        static constexpr char s_strSocketPath[] = "/tmp/mysocket";

        TerminalUnixSocket::TerminalUnixSocket(ConsoleSessionWithTerminal& a_rConsoleSession) noexcept : TerminalAnsi{ a_rConsoleSession } {
        }
        //TerminalUnixSocket::TerminalUnixSocket(TerminalUnixSocket const&) noexcept = default;
        //TerminalUnixSocket::TerminalUnixSocket(TerminalUnixSocket&&) noexcept = default;
        TerminalUnixSocket::~TerminalUnixSocket() noexcept {
            string key{};
            while (read(key)) {}
        }
        //TerminalUnixSocket& TerminalUnixSocket::operator= (TerminalUnixSocket const&) noexcept = default;
        //TerminalUnixSocket& TerminalUnixSocket::operator= (TerminalUnixSocket&&) noexcept = default;

        void TerminalUnixSocket::start() const noexcept {
            m_iServerSocket = socket(AF_UNIX, SOCK_STREAM, 0);
            if (-1 == m_iServerSocket) {
                perror("TerminalUnixSocket::start(1)");
                return;
            }

            struct sockaddr_un local;
            local.sun_family = AF_UNIX;
            strcpy(local.sun_path, s_strSocketPath);
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

            chmod(s_strSocketPath,  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

            m_ServerThread = std::thread{ &TerminalUnixSocket::serverLoop, const_cast<TerminalUnixSocket*>(this) };

            Terminal::start();
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
            requestTerminalSize();
        }

        void TerminalUnixSocket::stop() const noexcept {
            shutdown(m_iServerSocket, SHUT_RDWR);
            shutdown(m_iClientSocket, SHUT_RDWR);
            m_bStopClient = true;
            m_bStop = true;
            m_ConditionVariableTx.notify_one();
            m_ServerThread.join();
        }

        bool TerminalUnixSocket::supportsInteractivity() const noexcept {
            return true;
        }

        bool TerminalUnixSocket::supportsColor() const noexcept {
            return true;
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
            m_strDataToSend += a_strDataToPrint;
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
                    m_ClientThreadTx = thread{ std::bind(&TerminalUnixSocket::clientLoopTx, this, iClientSocket) };

                    m_ClientThreadRx.join();
                    m_ClientThreadTx.join();

                    close(iClientSocket);
                }
            }
        }

        void TerminalUnixSocket::clientLoopRx(int a_iClientSocket) noexcept {
            while (!m_bStopClient) {
                char recv_buf[100];
                memset(recv_buf, 0, 100*sizeof(char));

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

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

static const char* socket_path = "/home/mysocket";
static const unsigned int nIncomingConnections = 5;

int main()
{
    //create server side
    int s = 0;
    int s2 = 0;
    struct sockaddr_un local, remote;
    int len = 0;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if( -1 == s )
    {
        printf("Error on socket() call \n");
        return 1;
    }

    local.sun_family = AF_UNIX;
    strcpy( local.sun_path, socket_path );
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if( bind(s, (struct sockaddr*)&local, len) != 0)
    {
        printf("Error on binding socket \n");
        return 1;
    }

    if( listen(s, nIncomingConnections) != 0 )
    {
        printf("Error on listen call \n");
    }

    bool bWaiting = true;
    while (bWaiting)
    {
        unsigned int sock_len = 0;
        printf("Waiting for connection.... \n");
        if( (s2 = accept(s, (struct sockaddr*)&remote, &sock_len)) == -1 )
        {
            printf("Error on accept() call \n");
            return 1;
        }

        printf("Server connected \n");

        int data_recv = 0;
        char recv_buf[100];
        char send_buf[200];
        do{
            memset(recv_buf, 0, 100*sizeof(char));
            memset(send_buf, 0, 200*sizeof(char));
            data_recv = recv(s2, recv_buf, 100, 0);
            if(data_recv > 0)
            {
                printf("Data received: %d : %s \n", data_recv, recv_buf);
                strcpy(send_buf, "Got message: ");
                strcat(send_buf, recv_buf);

                if(strstr(recv_buf, "quit")!=0)
                {
                    printf("Exit command received -> quitting \n");
                    bWaiting = false;
                    break;
                }

                if( send(s2, send_buf, strlen(send_buf)*sizeof(char), 0) == -1 )
                {
                    printf("Error on send() call \n");
                }
            }
            else
            {
                printf("Error on recv() call \n");
            }
        }while(data_recv > 0);

        close(s2);
    }


    return 0;
}
#endif
