#include "EmbConsole.hpp"
#include "impl/Functions.hpp"
#include <string>
//#if __cplusplus >= 201703L // >= C++17
//#include <filesystem>
//namespace fs = std::filesystem;
//#elif __cplusplus >= 201402L // >= C++14
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
//#else // <= C++11
//#error C++14 minimum is required
//#endif

namespace emb {
    namespace console {
        using namespace::std;

        std::string version() {
            return "0.1.0";
        }

        std::vector<std::string> autocompleteFromFileSystem(std::string const& a_strPartialPath, std::string const& a_strRootPath,
                                                        bool a_bListFiles, bool a_bListDirectories, bool a_bRecursive) {
            std::vector<std::string> vecChoices{};

            // We get the position of the last '/' in the argument the user is typing
            size_t ulPos = a_strPartialPath.find_last_of('/');

            // We need to find information about what the user is typing:
            string strPrefixFolder{}; // what complete folder the user typed
            string strPrefixFolderCanonized{};
            string strPartialArg{a_strPartialPath}; // what partial folder the user started to typed
            string strCurrentFolder{a_strRootPath}; // what folder we need to search the choices into
            if(a_bRecursive && string::npos != ulPos) {
                // if a complete folder has already been typed, it is the part before the last '/'
                strPrefixFolder = a_strPartialPath.substr(0, ulPos+1);
                strPrefixFolderCanonized = Functions::getCanonicalPath(strPrefixFolder, true).substr(1);
                // and then the partial arg is now the part after the last '/'
                strPartialArg = a_strPartialPath.substr(ulPos+1);
                // the current folder we need to search the choices into is constructed from the current folder where the "cd"
                // command is typed and the complete folder the user typed as an argument of "cd"
                strCurrentFolder = Functions::getCanonicalPath(strCurrentFolder + "/" + strPrefixFolderCanonized);
                // e.g. if the current directory is /w/x
                // cd abcd/ef<Tab> => strPrefixFolder is "abcd/", strPartialArg is "ef", strCurrentFolder is /w/x/abcd
            }

            // We iterate (not recursively) in the current folder
            for(const fs::directory_entry& entry: fs::directory_iterator{strCurrentFolder}) {
                // If the file matches the searched type
                if( (a_bListFiles && is_regular_file(entry)) || ((a_bListDirectories || a_bRecursive) && fs::is_directory(entry))) {
                    // We format the output and add it to the list if it starts with the partial user entry
                    std::string path = entry.path().string().substr(a_strRootPath.size());
                    if(0 == path.find(strPrefixFolderCanonized)) {
                        path = path.substr(strPrefixFolderCanonized.size());
                    }
                    if(0 == path.find(strPartialArg)) {
                        vecChoices.push_back(strPrefixFolder + path + (fs::is_directory(entry) ? "/" : "") );
                    }
                }
            }

            return vecChoices;
        }

    } // console
} // emb