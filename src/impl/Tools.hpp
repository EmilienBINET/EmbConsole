#pragma once

#include <string>
#include <thread>
#include <vector>

namespace emb {
    namespace tools {
        namespace thread {
            void set_thread_name(std::thread& a_rThread, std::string const& a_strThreadName);
        }
        namespace regex {
            bool match(std::string const& a_strStringToTest, std::string const& a_strRegexPattern);
            bool search(std::vector<std::string>& a_rvstrMatches, std::string const& a_strStringToTest, std::string const& a_strRegexPattern);
            std::string replace(std::string const& a_strStringToTest, std::string const& a_strRegexPattern, std::string const& a_strStringReplacement);
        }
    }
}