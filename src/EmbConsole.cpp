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

        namespace autocompletion {
            // We need a custom getCanonicalPath function to use it on Windows and Unix
            string getCanonicalPath(std::string const& a_strPath, bool a_bEndWithDelimiter = true) {
                string strCanonized =  Functions::getCanonicalPath(a_strPath, a_bEndWithDelimiter);
                if(2 == strCanonized.find(":/") && '/' == strCanonized[0]) {
                    // substr to avoid /C:/ on Windows and to get C:/ instead
                    strCanonized = strCanonized.substr(1);
                }
                return strCanonized;
            };

            vector<string> getChoicesFromFileSystem(string const& a_strPartialPath, FileSystemOptions const& a_Options) noexcept {
                string strPartialPath{a_strPartialPath};

                // If the home path is not given, we suppose it is /
                string strHomePath{a_Options.homePath.empty() ? "/" : a_Options.homePath};
                string strChrootPath{a_Options.chrootPath};

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

#ifdef WIN32
                replace(strHomePath.begin(), strHomePath.end(), '\\', '/');
                replace(strChrootPath.begin(), strChrootPath.end(), '\\', '/');

                // We remove the drive from the path
                if(1 == strPartialPath.find(":/")) {
                    bAbsolutePath = true;
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

                // We need to gather information about what the user is typing
                bool bListChoices{true};
                string strPrefixFolder{}; // what complete folder the user typed
                string strPartialArg{strPartialPath}; // what partial folder the user started to type
                string strCurrentFolder{strHomePath}; // what folder we need to search the choices into

                // We get the position of the last '/' in the argument the user is typing
                // If it exists, it mean the user is trying to create a path recursively
                size_t ulPos = strPartialPath.find_last_of('/');
                if(a_Options.recursive && string::npos != ulPos) {
                    // if a complete folder has already been typed, it is the part before the last '/'
                    strPrefixFolder = strPartialPath.substr(0, ulPos+1);
                    // and then the partial arg is now the part after the last '/'
                    strPartialArg = strPartialPath.substr(ulPos+1);

                    // No chroot so the user can use absolute path
                    if(a_Options.chrootPath.empty()) {
                        // We create the current folder (in which we will look for subfiles/subdirectories later)
                        // from the drive prefix if user is typing an absolute path or from the home path for a relative one
                        // and from the complete folder that has already been entere by the user
                        strCurrentFolder = getCanonicalPath((bAbsolutePath ? strDrivePrefix : strHomePath) + "/" + strPrefixFolder);
                    }
                    // chroot enabled so the user can is kept captive in the chroot path
                    else {
                        // If the user is entering a drive name, we do not list anything
                        if(!strDrivePrefix.empty()) {
                            bListChoices = false;
                        }
                        // If no drive typed, we continue
                        else {
                            // We create the current folder (in which we will look for subfiles/subdirectories later)
                            // from the chroot path if user is typing an absolute path or from the home path for a relative one
                            // and from the complete folder that has already been entere by the user
                            strCurrentFolder = getCanonicalPath((bAbsolutePath ? strChrootPath : strHomePath) + "/" + strPrefixFolder);
                            // If we went beyond the chroot, we do not list anything
                            if(0 != strCurrentFolder.find(strChrootPath)) {
                                bListChoices = false;
                            }
                        }
                    }
                }

                // We can now list possible choices from the information we gathered from the user input
                vector<string> vecChoices{};
                if(bListChoices) {
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
                }
                return vecChoices;
            }

            bool getValueFromFileSystem(string & a_rstrPath, string const& a_strArgument, FileSystemOptions const& a_Options) noexcept {
                // If the home path is not given, we suppose it is /
                string strHomePath{a_Options.homePath.empty() ? "/" : a_Options.homePath};
                string strChrootPath{a_Options.chrootPath};

                // We need to know if user is typing an absolute path (or relative otherwise)
                bool bAbsolutePath{false};
                bool bWithDriveLetter{false};
#ifdef WIN32
                replace(strHomePath.begin(), strHomePath.end(), '\\', '/');
                replace(strChrootPath.begin(), strChrootPath.end(), '\\', '/');
                bWithDriveLetter = 1 == a_strArgument.find(":/");
                bAbsolutePath = bWithDriveLetter || 0 == a_strArgument.find("/");
#else
                bAbsolutePath = 0 == a_strArgument.find("/");
#endif

                bool bRes{false};
                string strPath{};
                // No chroot, the user is free
                if(a_Options.chrootPath.empty()) {
                    // If absolute path, we take is as it
                    if(bAbsolutePath) {
                        strPath = a_strArgument;
                    }
                    // If relative path, we add the home path
                    else {
                        strPath = getCanonicalPath(strHomePath + "/" + a_strArgument, false);
                    }
                    // In both cases, we consider the input as valid
                    bRes = true;
                }
                // With chroot, the user must be kept captive in it
                else {
                    // If there is a drive letter, the input is invalid
                    if(bWithDriveLetter) {
                        bRes = false;
                    }
                    else {
                        // If absolute path, we add it to the chroot take is as it
                        if(bAbsolutePath) {
                            strPath = getCanonicalPath(strChrootPath + "/" + a_strArgument, false);
                        }
                        // If relative path, we add the home path
                        else {
                            strPath = getCanonicalPath(strHomePath + "/" + a_strArgument, false);
                        }
                        // In both cases, we consider the input as valid if it is a subelement of the chroot
                        bRes = 0 == strPath.find(strChrootPath);
                    }
                }

                // If the path is valid, we give it to the caller
                if(bRes) {
                    a_rstrPath = strPath;
                }

                return bRes;
            }

            vector<string> getChoicesFromList(string const& a_strPartialArg, vector<string> const& a_vecChoices) noexcept {
                vector<string> vecChoices{};
                for(auto const& elm : a_vecChoices) {
                    if(0 == elm.find(a_strPartialArg)) {
                        vecChoices.push_back(elm);
                    }
                }
                return vecChoices;
            }

        } // autocompletion

        namespace table {

            void print(IPrintableConsole& a_rConsole, Table const& a_stTable) {

                // Calculate the width of each column
                size_t const ulMaxColumnWidth{100};
                std::vector<size_t> vulColumnsSize{};
                for(auto const& columnTitle : a_stTable.vstrColumnsTitles) {
                    vulColumnsSize.push_back(columnTitle.size());
                }
                for(auto const& row : a_stTable.vvstrRows) {
                    for(size_t ulIdx=0; ulIdx<row.size(); ++ulIdx) {
                        vulColumnsSize[ulIdx] = std::min(ulMaxColumnWidth, std::max(vulColumnsSize[ulIdx], row[ulIdx].strText.size()));
                    }
                }

                // Calculate the total width of the columns
                size_t ulTotalSize{0};
                for(size_t const ulSize : vulColumnsSize) {
                    ulTotalSize += ulSize + 2 + 1; // 2 spaces (one on each side) + one vertical bar
                }
                ulTotalSize += 1; // one final vertical bar

                // Print the header
                a_rConsole
                    << PrintNewLine()
                    // 1st line : lines
                    << PrintSymbol(PrintSymbol::Symbol::TopLeft)
                    << PrintSymbol(PrintSymbol::Symbol::HorizontalBar, ulTotalSize-2)
                    << PrintSymbol(PrintSymbol::Symbol::TopRight)
                    << PrintNewLine()
                    // 2nd line : texts
                    << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                    << PrintText(" FILE    ")
                    << SetColor(SetColor::Color::BrightMagenta)
                    << PrintText(a_stTable.strTitle)
                    << ResetTextFormat()
                    << PrintText(std::string(ulTotalSize-9-a_stTable.strTitle.size()-2, ' '))
                    << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                    << PrintNewLine();

                // Print the columns names : 1st line : lines
                bool bFirst{true};
                for(size_t const ulSize : vulColumnsSize) {
                    a_rConsole
                        << PrintSymbol(bFirst ? PrintSymbol::Symbol::LeftCross : PrintSymbol::Symbol::TopCross)
                        << PrintSymbol(PrintSymbol::Symbol::HorizontalBar, ulSize+2);
                    bFirst = false;
                }
                a_rConsole
                    << PrintSymbol(PrintSymbol::Symbol::RightCross)
                    << PrintNewLine();

                // Print the columns names : 2nd line : columns titles
                for(size_t ulIdx=0; ulIdx<vulColumnsSize.size(); ++ulIdx) {
                    auto columnTitle = a_stTable.vstrColumnsTitles.at(ulIdx);
                    columnTitle = " " + columnTitle;
                    columnTitle.resize(vulColumnsSize.at(ulIdx) + 2, ' ');
                    a_rConsole
                        << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                        << PrintText(columnTitle);
                }
                a_rConsole
                    << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                    << PrintNewLine();

                // Print the columns names : 3rd line : line
                bFirst = true;
                for(size_t const ulSize : vulColumnsSize) {
                    a_rConsole
                        << PrintSymbol(bFirst ? PrintSymbol::Symbol::LeftCross : PrintSymbol::Symbol::Cross)
                        << PrintSymbol(PrintSymbol::Symbol::HorizontalBar, ulSize+2);
                    bFirst = false;
                }
                a_rConsole
                    << PrintSymbol(PrintSymbol::Symbol::RightCross)
                    << PrintNewLine();

                // Print the data
                for(auto const& row : a_stTable.vvstrRows) {
                    for(size_t ulIdx=0; ulIdx<vulColumnsSize.size(); ++ulIdx) {
                        auto columnText = row.at(ulIdx);
                        bool bEllipsis{false};
                        if(columnText.strText.size() > vulColumnsSize.at(ulIdx)) {
                            columnText.strText.resize(vulColumnsSize.at(ulIdx) - 3);
                            bEllipsis = true;
                        }
                        else {
                            columnText.strText.resize(vulColumnsSize.at(ulIdx), ' ');
                        }
                        columnText.strText = " " + columnText.strText;
                        a_rConsole
                            << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                            << SetColor(columnText.eColor)
                            << PrintText(columnText.strText);
                        if(bEllipsis) {
                            a_rConsole
                                << SetColor(SetColor::Color::BrightRed)
                                << PrintText("... ");
                        }
                        else {
                            a_rConsole
                                << PrintText(" ");
                        }
                        a_rConsole
                            << ResetTextFormat();
                    }
                    a_rConsole
                        << PrintSymbol(PrintSymbol::Symbol::VerticalBar)
                        << PrintNewLine();
                }

                // Print the last line
                bFirst = true;
                for(size_t const ulSize : vulColumnsSize) {
                    a_rConsole
                        << PrintSymbol(bFirst ? PrintSymbol::Symbol::BottomLeft : PrintSymbol::Symbol::BottomCross)
                        << PrintSymbol(PrintSymbol::Symbol::HorizontalBar, ulSize+2);
                    bFirst = false;
                }
                a_rConsole
                    << PrintSymbol(PrintSymbol::Symbol::BottomRight)
                    << PrintNewLine();

            }

        } // table

    } // console
} // emb
