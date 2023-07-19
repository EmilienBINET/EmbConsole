#include "Tools.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace emb {
    namespace tools {
        namespace thread {
            void set_thread_name(std::thread& a_rThread, std::string const& a_strThreadName) {
#ifdef _WIN32
                WCHAR wstr[64]{};
                MultiByteToWideChar(0, 0, a_strThreadName.c_str(), -1, wstr, 62);
                SetThreadDescription(static_cast<HANDLE>(a_rThread.native_handle()), wstr);
#else
                pthread_setname_np(arThread.native_handle(), aThreadName.c_str());
#endif
            }
        }
    }
}