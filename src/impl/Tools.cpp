#include "Tools.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace emb {
    namespace tools {
        namespace thread {
            void set_thread_name(std::thread& a_rThread, std::string const& a_strThreadName) {
#if defined unix || defined __MINGW32__
                pthread_setname_np(a_rThread.native_handle(), a_strThreadName.c_str());
#elif defined _WIN32
                WCHAR wstr[64]{};
                MultiByteToWideChar(0, 0, a_strThreadName.c_str(), -1, wstr, 62);
                SetThreadDescription(static_cast<HANDLE>(a_rThread.native_handle()), wstr);
#endif
            }
        }
    }
}
