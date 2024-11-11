#include "Tools.hpp"
#include <regex>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
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
#if 0
            bool match(std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                return std::regex_match(a_strStringToTest, std::regex{ a_strRegexPattern });
            }
            bool search(std::vector<std::string>& a_rvstrMatches, std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                std::regex const rx{ a_strRegexPattern };
                std::smatch sm;
                bool bRes = std::regex_search(a_strStringToTest, sm, rx);
                if (bRes) {
                    a_rvstrMatches.clear();
                    for (int i = 0; i < sm.size(); ++i) {
                        a_rvstrMatches.push_back(sm[i].str());
                    }
                }
                return bRes;
            }
            std::string replace(std::string const& a_strStringToTest, std::string const& a_strRegexPattern, std::string const& a_strStringReplacement) {
                return std::regex_replace(a_strStringToTest, std::regex{ a_strRegexPattern }, a_strStringReplacement);
            }
#else
#define STR2STPR8(__str) reinterpret_cast<unsigned char const*>(__str.c_str())

            bool match(std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                std::vector<std::string> vstrMatches{};
                return search(vstrMatches, a_strStringToTest, "^" + a_strRegexPattern + "$");
            }
            bool search(std::vector<std::string>& a_rvstrMatches, std::string const& a_strStringToTest, std::string const& a_strRegexPattern) {
                bool bRes{ true };
                pcre2_code* pRegex{ NULL };
                pcre2_match_data* pMatchData{ NULL };
                int iNbMatches{ 0 };

                // Start by compiling the regular expression
                if (bRes) {
                    int iErrorCode{ 0 };
                    PCRE2_SIZE ulErrorOffset{ 0 };

                    // Compile regex
                    pRegex = pcre2_compile(
                        STR2STPR8(a_strRegexPattern),   // The pattern to compile
                        PCRE2_ZERO_TERMINATED,          // Indicates that the pattern is zero-terminated
                        0,                              // Default options
                        &iErrorCode,                    // For error number
                        &ulErrorOffset,                 // For error offset
                        NULL                            // Use default compile context
                    );

                    // Succeded if the result is not NULL
                    bRes = NULL != pRegex;

                    // We do not use error code here
                    (void)iErrorCode;
                    (void)ulErrorOffset;
                }

                // Then try to match the compiled regex against the string to test
                if (bRes) {
                    // Using this function ensures that the block is exactly the right size for
                    // the number of capturing parentheses in the pattern
                    pMatchData = pcre2_match_data_create_from_pattern(pRegex, NULL);

                    // Match regex against the subject string
                    iNbMatches = pcre2_match(
                        pRegex,                       // The compiled pattern
                        STR2STPR8(a_strStringToTest), // The subject string
                        a_strStringToTest.length(),   // The length of the subject
                        0,                            // Start at offset 0 in the subject
                        0,                            // Default options
                        pMatchData,                   // Block for storing the result
                        NULL                          // Use default match context
                    );

                    // Succedded if at least one match found
                    bRes = iNbMatches > 0;
                }

                // Finally create the output vector of matches
                if (bRes) {
                    // Get a pointer to the output vector, where string offsets are stored
                    PCRE2_SIZE* pOutputVector = pcre2_get_ovector_pointer(pMatchData);
                    // For each match
                    for (int i = 0; i < iNbMatches; i++) {
                        // Get the substring matching
                        a_rvstrMatches.push_back(a_strStringToTest.substr(
                            pOutputVector[2 * i],                           // Start
                            pOutputVector[2 * i + 1] - pOutputVector[2 * i] // Length
                        ));
                    }
                }

                // And clean memory
                if (pMatchData) {
                    pcre2_match_data_free(pMatchData);
                    pMatchData = NULL;
                }
                if (pRegex) {
                    pcre2_code_free(pRegex);
                    pRegex = NULL;
                }

                return bRes;
            }
            std::string replace(std::string const& a_strStringToTest, std::string const& a_strRegexPattern, std::string const& a_strStringReplacement) {
                bool bRes{ true };
                std::string strRes{};
                pcre2_code* pRegex{ NULL };

                // Start by compiling the regular expression
                if (bRes) {
                    int iErrorCode{ 0 };
                    PCRE2_SIZE ulErrorOffset{ 0 };

                    // Compile regex
                    pRegex = pcre2_compile(
                        STR2STPR8(a_strRegexPattern),   // The pattern to compile
                        PCRE2_ZERO_TERMINATED,          // Indicates that the pattern is zero-terminated
                        0,                              // Default options
                        &iErrorCode,                    // For error number
                        &ulErrorOffset,                 // For error offset
                        NULL                            // Use default compile context
                    );

                    // Succeded if the result is not NULL
                    bRes = NULL != pRegex;

                    // We do not use error code here
                    (void)iErrorCode;
                    (void)ulErrorOffset;
                }

                // Then try to replace the compiled regex against the string to test
                if (bRes) {
                    PCRE2_UCHAR pOutput[1024]{ "" };
                    PCRE2_SIZE ulOutputLen = sizeof(pOutput) / sizeof(PCRE2_UCHAR);

                    // Substitute each occurence
                    int iRes = pcre2_substitute(
                        pRegex,                             // The compiled pattern
                        STR2STPR8(a_strStringToTest),       // The subject string
                        a_strStringToTest.length(),         // The length of the subject
                        0,                                  // Offset in the subject at which to start matching
                        PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED, // Option bits
                        NULL,                               // No match data needed
                        NULL,                               // Use default match context
                        STR2STPR8(a_strStringReplacement),  // Points to the replacement string
                        a_strStringReplacement.length(),    // Length of the replacement string
                        pOutput,                            // Points to the output buffer
                        &ulOutputLen                        // Points to the length of the output buffer
                    );

                    // Succedded if no negative code
                    if (iRes > 0) {
                        strRes = reinterpret_cast<char const*>(pOutput);
                        bRes = true;
                    }
                    else if (iRes >= 0) {
                        // no substitution but no error
                        strRes = a_strStringToTest;
                        bRes = true;
                    }
                    else {
                        // error
                        bRes = false;
                    }
                }

                // And clean memory
                if (pRegex) {
                    pcre2_code_free(pRegex);
                    pRegex = NULL;
                }

                return strRes;
            }
        }
#endif
    }
}