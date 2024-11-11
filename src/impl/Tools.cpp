#include "Tools.hpp"
#include <regex>
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
        namespace regex {
            bool match(std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                return std::regex_match(a_strStringToTest, std::regex{ a_strRegexPattern });
            }
            bool search(std::vector<std::string>& a_rvstrMatches, std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                std::regex const rx{ a_strRegexPattern };
                std::smatch sm;
                bool bRes = std::regex_search(a_strStringToTest, sm, rx);
                if(bRes) {
                    a_rvstrMatches.clear();
                    for(int i=0; i<sm.size(); ++i) {
                        a_rvstrMatches.push_back(sm[i].str());
                    }
                }
                return bRes;
            }
            std::string replace(std::string const& a_strStringToTest, std::string const& a_strRegexPattern, std::string const& a_strStringReplacement) {
                return std::regex_replace(a_strStringToTest, std::regex{ a_strRegexPattern }, a_strStringReplacement);
            }
        }
    }
}
