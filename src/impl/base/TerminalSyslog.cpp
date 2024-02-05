#include "TerminalSyslog.hpp"
#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <cstring>
#endif

namespace emb {
    namespace console {
        using namespace std;


        TerminalSyslog::TerminalSyslog(ConsoleSessionWithTerminal& a_Console, std::shared_ptr<OptionSyslog> const a_pOption) noexcept
            : Terminal{a_Console}
            , m_pOption{ a_pOption }
        {
            m_pDestination = new sockaddr_in;
            if (auto pDest = static_cast<sockaddr_in*>(m_pDestination)) {
                memset(pDest, 0, sizeof(sockaddr_in));
                pDest->sin_family = AF_INET;
                pDest->sin_addr.s_addr = inet_addr(m_pOption->strDestination.c_str());
                pDest->sin_port = htons(514);
            }
#ifdef _WIN32
            WSADATA wsa_data;
            WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
        }
        TerminalSyslog::~TerminalSyslog() noexcept {
#ifdef _WIN32
            WSACleanup();
#endif
            if (auto pDest = static_cast<sockaddr_in*>(m_pDestination)) {
                delete pDest;
            }
        }

        void TerminalSyslog::start() noexcept {
            m_iSocket = socket(AF_INET, SOCK_DGRAM, 0);
            bool bc = true;
            setsockopt(m_iSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&bc, sizeof(bool));
        }
        void TerminalSyslog::processEvents() noexcept {
            processPrintCommands();
        }
        void TerminalSyslog::stop() noexcept {
            int status = 0;
#ifdef _WIN32
            status = shutdown(m_iSocket, SD_BOTH);
            if (status == 0) {
                status = closesocket(m_iSocket);
            }
#else
            status = shutdown(m_iSocket, SHUT_RDWR);
            if (status == 0) {
                status = close(m_iSocket);
            }
#endif
        }

        bool TerminalSyslog::read(std::string& a_rstrKey) const noexcept {
            return false;
        }

        bool TerminalSyslog::write(std::string const& a_strDataToPrint) const noexcept {
            if (auto pDest = static_cast<sockaddr*>(m_pDestination)) {
                OptionSyslog::Info info{ m_pOption->getInfo(a_strDataToPrint) };

                std::string strBuffer;
                switch (info.eCriticity) // Always "user" category (1<<3 = 8) + criticity
                {
                case OptionSyslog::Criticity::Emergency:
                    strBuffer = "<8>";
                    break;
                case OptionSyslog::Criticity::Alert:
                    strBuffer = "<9>";
                    break;
                case OptionSyslog::Criticity::Critical:
                    strBuffer = "<10>";
                    break;
                case OptionSyslog::Criticity::Error:
                    strBuffer = "<11>";
                    break;
                case OptionSyslog::Criticity::Warning:
                    strBuffer = "<12>";
                    break;
                case OptionSyslog::Criticity::Notice:
                    strBuffer = "<13>";
                    break;
                case OptionSyslog::Criticity::Informational:
                    strBuffer = "<14>";
                    break;
                case OptionSyslog::Criticity::Debugging:
                    strBuffer = "<15>";
                    break;
                }

                if (info.bSendTimestamp) {
                    if (0 == info.ulTimestamp) {
                        info.ulTimestamp = time(NULL);
                    }
                    struct tm* pTime = gmtime(&info.ulTimestamp);
                    size_t const ulMaxSize = 20;
                    char szBuffer[ulMaxSize]{};
                    strftime(szBuffer, ulMaxSize, "%b %d %H:%M:%S ", pTime);
                    strBuffer += szBuffer;
                }

                strBuffer += info.strHostName + " " + info.strTag + ": " + info.strMessage;

                sendto(m_iSocket, strBuffer.c_str(), strBuffer.size(), 0, pDest, sizeof(sockaddr_in));
            }
            return false;
        }

        void TerminalSyslog::printText(std::string const& a_strText) const noexcept {
            write(a_strText);
        }
    } // console
} // emb
