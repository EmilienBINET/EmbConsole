// https://stackoverflow.com/a/68348821
#ifndef STDCAPTURE_H
#define STDCAPTURE_H

#include <string>
#include <mutex>

class StdCapture
{
public:

    StdCapture();

    void BeginCapture();
    bool IsCapturing();
    bool EndCapture();
    std::string GetCapture();

private:
    enum PIPES { READ, WRITE };
    
    int secure_dup(int src);
    void secure_pipe(int * pipes);
    void secure_dup2(int src, int dest);
    void secure_close(int & fd);

    int m_pipe[2];
    int m_oldStdOut;
    int m_oldStdErr;
    bool m_capturing;
    std::mutex m_mutex;
    std::string m_captured;
};

#endif // STDCAPTURE_H

