// https://stackoverflow.com/a/68348821
#include "StdCapture.hpp"

#if defined _MSC_VER || defined __MINGW32__
#include <windows.h>
#include <io.h>
#define popen _popen 
#define pclose _pclose
#define stat _stat 
#define dup _dup
#define dup2 _dup2
#define fileno _fileno
#define close _close
#define pipe _pipe
#define read _read
#define eof _eof
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <algorithm>

#ifndef STD_OUT_FD 
#define STD_OUT_FD (fileno(stdout)) 
#endif 

#ifndef STD_ERR_FD 
#define STD_ERR_FD (fileno(stderr)) 
#endif

StdCapture::StdCapture():
    m_capturing(false)
{
    // make stdout & stderr streams unbuffered
    // so that we don't need to flush the streams
    // before capture and after capture 
    // (fflush can cause a deadlock if the stream is currently being 
    std::lock_guard<std::mutex> lock(m_mutex);
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
}

void StdCapture::BeginCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_capturing)
        return;

    secure_pipe(m_pipe);
    m_oldStdOut = secure_dup(STD_OUT_FD);
    m_oldStdErr = secure_dup(STD_ERR_FD);
    secure_dup2(m_pipe[WRITE],STD_OUT_FD);
    secure_dup2(m_pipe[WRITE],STD_ERR_FD);
    m_capturing = true;
#if !(defined _MSC_VER || defined __MINGW32__)
    secure_close(m_pipe[WRITE]);
#endif
}
bool StdCapture::IsCapturing()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_capturing;
}
bool StdCapture::EndCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_capturing)
        return true;

    m_captured.clear();
    secure_dup2(m_oldStdOut, STD_OUT_FD);
    secure_dup2(m_oldStdErr, STD_ERR_FD);

#if defined _MSC_VER || defined __MINGW32__
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), m_win32ConsoleMode);
#endif

    const int bufSize = 1025;
    char buf[bufSize];
    int bytesRead = 0;
    bool fd_blocked(false);
    do
    {
        bytesRead = 0;
        fd_blocked = false;
#if defined _MSC_VER || defined __MINGW32__
        if (!eof(m_pipe[READ]))
            bytesRead = read(m_pipe[READ], buf, bufSize-1);
#else
        bytesRead = read(m_pipe[READ], buf, bufSize-1);
#endif
        if (bytesRead > 0)
        {
            buf[bytesRead] = 0;
            m_captured += buf;
        }
        else if (bytesRead < 0)
        {
            fd_blocked = (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);
            if (fd_blocked)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    while(fd_blocked || bytesRead == (bufSize-1));

    secure_close(m_oldStdOut);
    secure_close(m_oldStdErr);
    secure_close(m_pipe[READ]);
#if defined _MSC_VER || defined __MINGW32__
    secure_close(m_pipe[WRITE]);
#endif
    m_capturing = false;
    return true;
}
std::string StdCapture::GetCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_captured.size()>0) {
        m_captured.erase(std::find_if(m_captured.rbegin(), m_captured.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), m_captured.end());
    }
    return m_captured;
}

void StdCapture::setWin32ConsoleMode(unsigned long consoleMode) {
    m_win32ConsoleMode = consoleMode;
}

int StdCapture::secure_dup(int src)
{
    int ret = -1;
    bool fd_blocked = false;
    do
    {
         ret = dup(src);
         fd_blocked = (errno == EINTR ||  errno == EBUSY);
         if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (ret < 0);
    return ret;
}
void StdCapture::secure_pipe(int * pipes)
{
    int ret = -1;
    bool fd_blocked = false;
    do
    {
#if defined _MSC_VER || defined __MINGW32__
        ret = pipe(pipes, 65536, O_BINARY);
#else
        ret = pipe(pipes) == -1;
#endif
        fd_blocked = (errno == EINTR ||  errno == EBUSY);
        if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (ret < 0);
}
void StdCapture::secure_dup2(int src, int dest)
{
    int ret = -1;
    bool fd_blocked = false;
    do
    {
         ret = dup2(src,dest);
         fd_blocked = (errno == EINTR ||  errno == EBUSY);
         if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (ret < 0);
}

void StdCapture::secure_close(int & fd)
{
    int ret = -1;
    bool fd_blocked = false;
    do
    {
         ret = close(fd);
         fd_blocked = (errno == EINTR);
         if (fd_blocked)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (ret < 0);

    fd = -1;
}
