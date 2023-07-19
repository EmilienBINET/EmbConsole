#pragma once

#include <string>
#include <thread>

namespace emb {
    namespace tools {
        namespace thread {
            void set_thread_name(std::thread& a_rThread, std::string const& a_strThreadName);
        }
    }
}