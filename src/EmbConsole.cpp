#include "EmbConsole.hpp"
#include "impl/Functions.hpp"
#include <string>
#if __cplusplus >= 201703L // >= C++17
#include "impl/filesystem.hpp"
namespace fs = std::filesystem;
#elif __cplusplus >= 201103L // >= C++11
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else // < C++11
#error Cannot use filesystem
#endif

namespace emb {
    namespace console {
        using namespace::std;

        string version() {
            return "0.1.0";
        }

        vector<string> autocompleteFromFileSystem(string const& a_strPartialPath, AutoCompleteFromFileSystemOptions const& a_Options) noexcept {
            vector<string> vecChoices{};
            string strPartialPath{a_strPartialPath};

            // If the home path is not given, we suppose it is /
            string strHomePath{a_Options.homePath.empty() ? "/" : a_Options.homePath};

            // Test options
            assert(!(a_Options.listDirectories == false && a_Options.recursive == true)
                   && "Cannot recurse without listing directories");
            assert(!(!a_Options.chrootPath.empty() && 0 != a_Options.homePath.find(a_Options.chrootPath))
                   && "homePath is not a subdirectory of chrootPath");
            assert(!(!a_Options.chrootPath.empty() && !fs::exists(a_Options.chrootPath))
                   && "chrootPath does not exist");
            assert(fs::exists(strHomePath)
                   && "homePath does not exist");

            // We need to know if user is typing an absolute path (or relative otherwise)
            bool bAbsolutePath{false};
            string strDrivePrefix{};
            bool bDrivePrefixPresent{false};

#ifdef WIN32
            // We remove the drive from the path
            if(1 == strPartialPath.find(":/")) {
                bAbsolutePath = true;
                bDrivePrefixPresent = true;
                strDrivePrefix = strPartialPath.substr(0, 2);
                strPartialPath = strPartialPath.substr(2);
            }
            else if(0 == strPartialPath.find("/")) {
                bAbsolutePath = true;
            }
#else
            if(0 == strPartialPath.find("/")) {
                bAbsolutePath = true;
            }
#endif

            // We need a custom getCanonicalPath function to use it on Windows and Unix
            auto getCanonicalPath = [&](std::string const& a_strPath, bool a_bEndWithDelimiter = true) {
                return Functions::getCanonicalPath(a_strPath, a_bEndWithDelimiter)
                    .substr(bDrivePrefixPresent ? 1 : 0); // substr to avoid /C: on Windows and to get C: instead
            };

            // We also need a filename comparison function
            auto filenameStartsWith = [&](std::string const& a_strFilename, std::string const& a_strPrefix) {
                bool bRes{true};
                if(a_Options.caseSensitive) {
                    // If case sensitive comparison
                    bRes = 0 == a_strFilename.find(a_strPrefix);
                }
                else {
                    // If case insensitive comparison
                    for(size_t i = 0; bRes && i < a_strFilename.size() && i < a_strPrefix.size(); ++i) {
                        if(tolower(a_strFilename[i]) != tolower(a_strPrefix[i])) {
                            bRes = false;
                        }
                    }
                }
                return bRes;
            };

            // We get the position of the last '/' in the argument the user is typing
            size_t ulPos = strPartialPath.find_last_of('/');

            // We need to find information about what the user is typing:
            string strPrefixFolder{}; // what complete folder the user typed
            string strPartialArg{strPartialPath}; // what partial folder the user started to type
            string strCurrentFolder{strHomePath}; // what folder we need to search the choices into
            if(a_Options.recursive && string::npos != ulPos) {
                // if a complete folder has already been typed, it is the part before the last '/'
                strPrefixFolder = strPartialPath.substr(0, ulPos+1);
                // and then the partial arg is now the part after the last '/'
                strPartialArg = strPartialPath.substr(ulPos+1);

                // No chroot so the user can use absolute path
                if(a_Options.chrootPath.empty()) {
                    if(bAbsolutePath) {
                        // the current folder we need to search the choices into is constructed from the drive and the complete folder the user typed
                        strCurrentFolder = getCanonicalPath(strDrivePrefix + "/" + strPrefixFolder);
                    }
                    else {
                        // the current folder we need to search the choices into is constructed from the homePath and the complete folder the user typed
                        strCurrentFolder = getCanonicalPath(strHomePath + "/" + strPrefixFolder);
                    }
                    // If we went beyond the root, we need to stay at the root
                    if(strCurrentFolder.empty()) {
                        strCurrentFolder = "/";
                    }
                }
                // chroot enabled so the user can is kept captive in the chroot path
                else {
                    if(bAbsolutePath) {
                        // the current folder we need to search the choices into is constructed from the chrootPath and the complete folder the user typed
                        strCurrentFolder = getCanonicalPath(a_Options.chrootPath + "/" + strPrefixFolder);
                    }
                    else {
                        // the current folder we need to search the choices into is constructed from the homePath and the complete folder the user typed
                        strCurrentFolder = getCanonicalPath(strCurrentFolder + "/" + strPrefixFolder);
                    }
                    if(0 != strCurrentFolder.find(a_Options.chrootPath)) {
                        // If we went beyond the chroot, we nee to stay in the chroot
                        strCurrentFolder = a_Options.chrootPath;
                    }
                }
            }

            try {
                // We iterate (not recursively) in the current folder
                for(const fs::directory_entry& entry: fs::directory_iterator{strCurrentFolder}) {
                    try {
                        // If the file matches the searched type
                        if( (a_Options.listFiles && is_regular_file(entry)) || (a_Options.listDirectories && fs::is_directory(entry))) {
                            // We format the output and add it to the list if it starts with the partial user entry
                            string filename = entry.path().filename().string() + (fs::is_directory(entry) ? "/" : "");
                            if(filenameStartsWith(filename, strPartialArg)) {
                                vecChoices.push_back((bAbsolutePath ? strDrivePrefix : "") + strPrefixFolder + filename);
                            }
                        }
                    }
                    catch(...) {
                    }
                }
            }
            catch(...) {
            }

            return vecChoices;
        }

    } // console
} // emb
