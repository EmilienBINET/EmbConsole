#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>
#include <unordered_map>

#ifdef STATIC
#define EmbConsole_EXPORT
#elif defined _WIN32
#ifdef _EXPORTING
#define EmbConsole_EXPORT    __declspec(dllexport)
#else
#define EmbConsole_EXPORT    __declspec(dllimport)
#endif
#else
#define EmbConsole_EXPORT
#endif

namespace emb {
    namespace tools {
        namespace memory {
#if __cplusplus < 201402L // Before C++14, std::make_unique does not exist
            template<class T, class... Args>
            std::unique_ptr<T> make_unique(Args&&... args) {
                return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
            }
#else // From C++14, std::make_unique exists so we just alias it
            template<class T, class... Args>
            std::unique_ptr<T> make_unique(Args&&... args) {
                return std::make_unique<T>(std::forward<Args>(args)...);
            }
#endif
        }
    }
    namespace console {
        /**
         * @brief Gives the library's version
         *
         * @return std::string library's version
         */
        EmbConsole_EXPORT std::string version();

        //////////////////////////////////////////////////
        ///// Some forward declarations
        //////////////////////////////////////////////////

        struct UserCommandData;
        struct UserCommandInfo;
        class Options;
        class ConsoleSession;
        class PrintCommand;
        class PromptCommand;
        class Terminal;
        using TerminalPtr = std::shared_ptr<Terminal>;

        //////////////////////////////////////////////////
        ///// User Console Commands
        //////////////////////////////////////////////////

        struct UserCommandData {
            using Arg = std::string;
            using Args = std::vector<Arg>;

            UserCommandInfo const& info;
            ConsoleSession& console;
            Args args;
        };

        struct UserCommandAutoCompleteData {
            using Arg = std::string;
            using Args = std::vector<Arg>;

            Args args;
            Arg partialArg;
        };

        using UserCommandFunctor0 = std::function<void(void)>;
        using UserCommandFunctor1 = std::function<void(UserCommandData const&)>;
        using UserCommandAutoCompleteFunctor = std::function<std::vector<std::string>(UserCommandAutoCompleteData const&)>;
        using StandardOutputFunctor = std::function<void(std::string const&)>;

        struct UserCommandInfo {
            std::string path;                   ///< Path of the command
            std::string description;            ///< Description of the command
            std::string help;                   ///< Text of the help. displayed with command "help <cmd>"
            std::vector<std::string> users;     ///< Users that can see the command. empty = all users
            UserCommandInfo(char const* a_szPath) : path(a_szPath) {}
            UserCommandInfo(std::string const& a_strPath = "", std::string const& a_strDescription = "",
                std::string const& a_strHelp = "", std::vector<std::string> const& a_vstrUsers = {})
                : path(a_strPath)
                , description(a_strDescription)
                , help(a_strHelp)
                , users(a_vstrUsers)
            {}
            void validate() const noexcept {
                assert(path.size() > 1);
                assert(path.at(0) == '/');
                assert(path.at(path.size() - 1) != '/');
            }
        };

        //////////////////////////////////////////////////
        ///// Autocompletion tools
        //////////////////////////////////////////////////

        namespace autocompletion {
            struct FileSystemOptions {
                bool listFiles{ true };           ///< If set, the autocompletion will propose files names
                bool listDirectories{ false };    ///< If set, the autocompletion will propose directories names
                bool recursive{ false };          ///< If set, the autocompletion will propose subelements of directories (requires listDirectories to be set)
                bool caseSensitive{ true };       ///< If set, the autocompletion will propose subelements matching case
                std::string homePath{ "/" };      ///< Home folder where the autocompletion searches subelements if the user specify a relative path
                std::string chrootPath{};       ///< If set, it defines a folder from witch the user cannot exit when autocompleting file path
            };

            /**
             * @brief Helper function to ease the autocompletion of a command that uses system files
             * @param a_strPartialPath      Partial path that the user is typing
             * @param a_Options             Options for the autocompletion
             * @return std::vector<std::string> List of the possible choices for the user autocompletion
             */
            EmbConsole_EXPORT std::vector<std::string> getChoicesFromFileSystem(
                std::string const& a_strPartialPath, FileSystemOptions const& a_Options = {}) noexcept;

            /**
             * @brief Helper function to get a real path from a user argument that used \c autocompleteFromFileSystem
             * @param a_rstrPath            output computed path
             * @param a_strArgument         input user argument
             * @param a_Options             Options for the autocompletion
             * @return bool true if the argument matches the options, false otherwise
             */
            EmbConsole_EXPORT bool getValueFromFileSystem(std::string& a_rstrPath,
                std::string const& a_strArgument, FileSystemOptions const& a_Options = {}) noexcept;

            /**
             * @brief Helper function to ease the autocompletion of a command amonst multiple choices
             * @param a_strPartialArg       Partial argument that the user is typing
             * @param a_vecChoices          List of possible choices
             * @return std::vector<std::string> List of the possible choices for the user autocompletion
             */
            EmbConsole_EXPORT std::vector<std::string> getChoicesFromList(
                std::string const& a_strPartialArg, std::vector<std::string> const& a_vecChoices) noexcept;
        }

        //////////////////////////////////////////////////
        ///// Console stream object
        //////////////////////////////////////////////////

        /**
         * @brief Represents a console stream
         *
         */
        class EmbConsole_EXPORT IPrintableConsole {
            // public members
        public:
            virtual IPrintableConsole& operator<< (PrintCommand const&) noexcept = 0;
            IPrintableConsole& operator<< (char const*) noexcept;
            IPrintableConsole& operator<< (std::string const&) noexcept;

            void print(std::string const& a_Data) noexcept;
            void printError(std::string const& a_Data) noexcept;
        };

        /**
         * @brief Represents a console stream
         *
         */
        class EmbConsole_EXPORT IPromptableConsole : public IPrintableConsole {
            // public members
        public:
            virtual IPrintableConsole& operator<< (PrintCommand const&) noexcept = 0;
            virtual IPromptableConsole& operator<< (PromptCommand const&) noexcept = 0;

            bool promptString(std::string const& a_strQuestion, std::string& a_rstrResult, std::string const& a_strRegexValidator = "", std::string const& a_strErrorMessage = "Invalid entry") noexcept;
            bool promptIpv4(std::string const& a_strQuestion, std::string& a_rstrResult, std::string const& a_strErrorMessage = "Invalid IP") noexcept;
            bool promptNumber(std::string const& a_strQuestion, long&) noexcept;
            bool promptYesNo(std::string const& a_strQuestion, bool&) noexcept;
            bool promptChoice(std::string const& a_strQuestion, std::unordered_map<std::string, std::string> const& a_mapChoices, std::string& a_rstrResult);
        };

        //////////////////////////////////////////////////
        ///// Console Session object
        //////////////////////////////////////////////////

        /**
         * @brief Represents a console session
         *
         */
        class EmbConsole_EXPORT ConsoleSession : public IPromptableConsole {
            // public members
        public:
            ConsoleSession(TerminalPtr) noexcept;
            ConsoleSession(ConsoleSession const&) noexcept = delete;
            ConsoleSession(ConsoleSession&&) noexcept;
            virtual ~ConsoleSession() noexcept;
            ConsoleSession& operator= (ConsoleSession const&) noexcept = delete;
            ConsoleSession& operator= (ConsoleSession&&) noexcept;

            IPrintableConsole& operator<< (PrintCommand const&) noexcept override;
            IPromptableConsole& operator<< (PromptCommand const&) noexcept override;

            std::string getCurrentPath() const noexcept;
            std::string getUser() const noexcept;
            std::pair<int, int> getTerminalSize() const noexcept;
            std::pair<int, int> getCursorPosition() const noexcept;

            void setInstantPrint(bool a_bInstantPrint) noexcept;

            // private attributes
        private:
            class Private;
            std::unique_ptr<Private> m_pPrivateImpl;
        };

        //////////////////////////////////////////////////
        ///// Console object
        //////////////////////////////////////////////////

        /**
         * @brief Represents a console
         *
         */
        class EmbConsole_EXPORT Console : public IPrintableConsole {
            // public types
        public:
            // public static members
        public:
            static std::shared_ptr<Console> create(int a_iId = 0) noexcept;
            static std::shared_ptr<Console> create(Options const&, int a_iId = 0) noexcept;
            static std::weak_ptr<Console> instance(int a_iId = 0) noexcept;

            static void showWindowsStdConsole() noexcept;
            static void hideWindowsStdConsole() noexcept;

            // public members
        public:
            Console() noexcept;
            Console(Console const&) noexcept;
            Console(Console&&) noexcept;
            Console(int argc, char** argv) noexcept;
            Console(Options const&) noexcept;
            virtual ~Console() noexcept;
            Console& operator= (Console const&) noexcept;
            Console& operator= (Console&&) noexcept;

            // public members
        public:
            //void setInteractive(bool) noexcept;

            IPrintableConsole& operator<< (PrintCommand const&) noexcept override;
            IPrintableConsole& operator<< (char const*) noexcept;
            IPrintableConsole& operator<< (std::string const&) noexcept;

            void setPromptFormat(std::string const&) noexcept;
            void setUserName(std::string const&) noexcept;
            void setMachineName(std::string const&) noexcept;

            void addCommand(UserCommandInfo const&, UserCommandFunctor0 const&, UserCommandAutoCompleteFunctor const& = nullptr) noexcept;
            void addCommand(UserCommandInfo const&, UserCommandFunctor1 const&, UserCommandAutoCompleteFunctor const& = nullptr) noexcept;
            void delCommand(UserCommandInfo const&) noexcept;
            void delAllCommands() noexcept;

            void execCommand(UserCommandInfo const&, UserCommandData::Args const& = {}) noexcept;

            void setStandardOutputCapture(StandardOutputFunctor const&) noexcept;

            void setPromptEnabled(bool) noexcept;

        private:
            class Private;
            std::unique_ptr<Private> m_pPrivateImpl;
        };

        //////////////////////////////////////////////////
        ///// Options
        //////////////////////////////////////////////////

        class EmbConsole_EXPORT Option {
        public:
            Options operator+(Option const& a_other) const;
            Options operator+(Options const& a_other) const;
            virtual std::shared_ptr<Option> copy() const noexcept = 0;
            std::string strDesc{};
        };

        /**
         * @brief Represents Options that can be passed to the library
         *
         */
        class EmbConsole_EXPORT Options {
            // public types
        public:
            //enum class Type {
            //    IsInteractive,            //!< enabled by default, disable with --console-not-interactive
            //    ColorEnabled,             //!< enabled by default, disable with --console-no-color
            //    UnixSocketClientEnabled,  //!< disbaled by default, enable with --console-unix-client
            //    UnixSocketServerEnabled,  //!< disbaled by default, enable with --console-unix-server
            //    CaptureAllStdOutStdErr,
            //    MonoThread
            //};

            // public static members
        public:
            static Options fromMainArgs(int argc, char** argv);
            Options();
            Options(Option const& a_other);
            Options operator+(Option const& a_other) const;
            Options operator+(Options const& a_other) const;
            template<typename T>
            std::shared_ptr<T> get() const {
                for (auto const& pElm : m_vpOptions) {
                    if (auto pCastedElm = std::dynamic_pointer_cast<T>(pElm)) {
                        return pCastedElm;
                    }
                }
                return nullptr;
            }

        private:
            std::vector<std::shared_ptr<Option>> m_vpOptions{};
        };

        class EmbConsole_EXPORT OptionStd : public Option {
        public:
            OptionStd() noexcept { strDesc = "OptionStd()"; };
            OptionStd(bool a_bEnabled) noexcept : bEnabled{ a_bEnabled } { strDesc = "OptionStd(" + std::to_string(a_bEnabled) + ")"; }
            std::shared_ptr<Option> copy() const noexcept override { return emb::tools::memory::make_unique<OptionStd>(bEnabled); }
            bool bEnabled{ true };
        };

        class EmbConsole_EXPORT OptionFile : public Option {
        public:
            OptionFile() noexcept { strDesc = "OptionFile()"; };
            OptionFile(bool a_bEnabled, std::string const& a_strFilePath) noexcept : bEnabled{ a_bEnabled }, strFilePath{ a_strFilePath }
            { strDesc = "OptionFile(" + std::to_string(a_bEnabled) + "," + a_strFilePath + ")"; }
            std::shared_ptr<Option> copy() const noexcept override { return emb::tools::memory::make_unique<OptionFile>(bEnabled, strFilePath); }
            bool bEnabled{ false };
            std::string strFilePath{};
        };

        class EmbConsole_EXPORT OptionUnixSocket : public Option {
        public:
            OptionUnixSocket() noexcept { strDesc = "OptionUnixSocket()"; };
            OptionUnixSocket(bool a_bEnabled, std::string const& a_strSocketFilePath, std::string const& a_strShellFilePath) noexcept
                : bEnabled{ a_bEnabled }, strSocketFilePath{ a_strSocketFilePath }, strShellFilePath{ a_strShellFilePath }
            { strDesc = "OptionUnixSocket(" + std::to_string(a_bEnabled) + "," + strSocketFilePath + "," + a_strShellFilePath + ")"; }
            std::shared_ptr<Option> copy() const noexcept override { return emb::tools::memory::make_unique<OptionUnixSocket>(bEnabled, strSocketFilePath, strShellFilePath); }
            bool bEnabled{ false };
            std::string strSocketFilePath{};
            std::string strShellFilePath{};
        };

        class EmbConsole_EXPORT OptionLocalTcpServer : public Option {
        public:
            OptionLocalTcpServer() noexcept { strDesc = "OptionLocalTcpServer()"; };
            OptionLocalTcpServer(bool a_bEnabled, int a_iPort, std::string const& a_strShellFilePath) noexcept
                : bEnabled{ a_bEnabled }, iPort{ a_iPort }, strShellFilePath{ a_strShellFilePath }
            { strDesc = "OptionLocalTcpServer(" + std::to_string(a_bEnabled) + ",127.0.0.1:" + std::to_string(iPort) + "," + strShellFilePath + ")"; }
            std::shared_ptr<Option> copy() const noexcept override { return emb::tools::memory::make_unique<OptionLocalTcpServer>(bEnabled, iPort, strShellFilePath); }
            bool bEnabled{ false };
            int iPort{};
            std::string strShellFilePath{};
        };

        class EmbConsole_EXPORT OptionSyslog : public Option {
        public:
            enum class Criticity {
                Emergency,
                Alert,
                Critical,
                Error,
                Warning,
                Notice,
                Informational,
                Debugging
            };
            struct Info {
                bool bSendTimestamp{ false };
                time_t ulTimestamp{ 0 };
                Criticity eCriticity{ Criticity::Informational };
                std::string strHostName{ "host" };
                std::string strTag{ "tag" };
                std::string strMessage{};
                Info(std::string const& a_strRawMessage) : strMessage{ a_strRawMessage } {}
            };
        public:
            OptionSyslog() noexcept { strDesc = "OptionSyslog()"; };
            OptionSyslog(bool a_bEnabled, std::string const& a_strDestination, std::function<Info(std::string const&)> const& a_fctGetInfo = nullptr) noexcept
                : bEnabled{ a_bEnabled }, strDestination{ a_strDestination }, m_fctGetInfo{ a_fctGetInfo }
            { strDesc = "OptionSyslog(" + std::to_string(a_bEnabled) + "," + strDestination + ")"; }
            std::shared_ptr<Option> copy() const noexcept override { return emb::tools::memory::make_unique<OptionSyslog>(bEnabled, strDestination, m_fctGetInfo); }
            Info getInfo(std::string const& a_strRawMessage) const noexcept { if (m_fctGetInfo) { return m_fctGetInfo(a_strRawMessage); } return Info{ a_strRawMessage }; }
            bool bEnabled{ false };
            std::string strDestination{};
        private:
            std::function<Info(std::string const&)> m_fctGetInfo{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommand Base
        //////////////////////////////////////////////////

        class EmbConsole_EXPORT PrintCommand {
        public:
            using Ptr = std::shared_ptr<PrintCommand>;
            using VPtr = std::vector<Ptr>;
        public:
            PrintCommand() noexcept;
            PrintCommand(PrintCommand const&) noexcept;
            PrintCommand(PrintCommand&&) noexcept;
            virtual ~PrintCommand() noexcept;
            PrintCommand& operator= (PrintCommand const&) noexcept;
            PrintCommand& operator= (PrintCommand&&) noexcept;
            virtual void process(Terminal&) const noexcept = 0;
            virtual Ptr copy() const noexcept = 0;
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Begin / Commit / EnableInstantPrint
        //////////////////////////////////////////////////

        /// Begins a session of print commands. Commands sent before Begin are not taken into consideration
        class EmbConsole_EXPORT Begin final
            : public PrintCommand{
        public:
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Begin>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Ends a session of print commands. Commands sent between Begin and Commit are treated at the commit.
        class EmbConsole_EXPORT Commit final
            : public PrintCommand{
        public:
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Commit>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Requests the console to print the current list of command instantly when receiving the commit command
        class EmbConsole_EXPORT InstantPrint final
            : public PrintCommand{
        public:
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<InstantPrint>(); }
            void process(Terminal&) const noexcept override;
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Positioning
        //////////////////////////////////////////////////

        /// Cursor up by <n>
        class EmbConsole_EXPORT MoveCursorUp final
            : public PrintCommand{
        public:
            MoveCursorUp(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorUp>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor down by <n>
        class EmbConsole_EXPORT MoveCursorDown final
            : public PrintCommand{
        public:
            MoveCursorDown(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorDown>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor forward (Right) by <n>
        class EmbConsole_EXPORT MoveCursorForward final
            : public PrintCommand{
        public:
            MoveCursorForward(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorForward>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor backward (Left) by <n>
        class EmbConsole_EXPORT MoveCursorBackward final
            : public PrintCommand{
        public:
            MoveCursorBackward(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorBackward>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor down <n> lines from current position
        class EmbConsole_EXPORT MoveCursorToNextLine final
            : public PrintCommand{
        public:
            MoveCursorToNextLine(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorToNextLine>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor up <n> lines from current position
        class EmbConsole_EXPORT MoveCursorToPreviousLine final
            : public PrintCommand{
        public:
            MoveCursorToPreviousLine(unsigned int const a_uiN) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorToPreviousLine>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Cursor moves to <r>th position horizontally in the current line
        class EmbConsole_EXPORT MoveCursorToRow final
            : public PrintCommand{
        public:
            MoveCursorToRow(unsigned int const a_uiR) : m_uiR{ a_uiR } { assert(m_uiR > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorToRow>(m_uiR); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiR{};
        };
        /// Cursor moves to the <c>th position vertically in the current column
        class EmbConsole_EXPORT MoveCursorToColumn final
            : public PrintCommand{
        public:
            MoveCursorToColumn(unsigned int const a_uiC) : m_uiC{ a_uiC } { assert(m_uiC > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorToColumn>(m_uiC); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiC{};
        };
        /// Cursor moves to <c>; <r> coordinate within the viewport, where <c> is the column of the <r> line
        class EmbConsole_EXPORT MoveCursorToPosition final
            : public PrintCommand{
        public:
            MoveCursorToPosition(unsigned int const a_uiR, unsigned int const a_uiC) : m_uiR{ a_uiR }, m_uiC{ a_uiC } { assert(m_uiR > 0 && m_uiC > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<MoveCursorToPosition>(m_uiR, m_uiC); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiR{};
            unsigned int const m_uiC{};
        };
        /// Save Cursor Position in Memory
        class EmbConsole_EXPORT SaveCursor final
            : public PrintCommand{
        public:
            SaveCursor() {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SaveCursor>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Restore Cursor Position from Memory
        class EmbConsole_EXPORT RestoreCursor final
            : public PrintCommand{
        public:
            RestoreCursor() {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<RestoreCursor>(); }
            void process(Terminal&) const noexcept override;
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Visibility
        //////////////////////////////////////////////////

        /// Start/Stop the cursor blinking
        class EmbConsole_EXPORT SetCursorBlinking final
            : public PrintCommand{
        public:
            SetCursorBlinking(bool const a_bBlinking) : m_bBlinking{ a_bBlinking } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetCursorBlinking>(m_bBlinking); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bBlinking{};
        };
        /// Show/Hide the cursor
        class EmbConsole_EXPORT SetCursorVisible final
            : public PrintCommand{
        public:
            SetCursorVisible(bool const a_bVisible) : m_bVisible{ a_bVisible } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetCursorVisible>(m_bVisible); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bVisible{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Cursor Shape
        //////////////////////////////////////////////////

        /// Customize the cursor shape
        class EmbConsole_EXPORT SetCursorShape final
            : public PrintCommand{
        public:
            enum class Shape {
                UserDefault,        ///< Default cursor shape configured by the user
                BlinkingBlock,      ///< Blinking block cursor shape
                SteadyBlock,        ///< Steady block cursor shape
                BlinkingUnderline,  ///< Blinking underline cursor shape
                SteadyUnderline,    ///< Steady underline cursor shape
                BlinkingBar,        ///< Blinking bar cursor shape
                SteadyBar,          ///< Steady bar cursor shape
            };
        public:
            SetCursorShape(Shape const a_eShape) : m_eShape{ a_eShape } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetCursorShape>(m_eShape); }
            void process(Terminal&) const noexcept override;
        private:
            Shape const m_eShape{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Viewport Positioning
        //////////////////////////////////////////////////

        /// Scroll text up by <n>. Also known as pan down, new lines fill in from the bottom of the screen
        class EmbConsole_EXPORT ScrollUp final
            : public PrintCommand{
        public:
            ScrollUp(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ScrollUp>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Scroll down by <n>. Also known as pan up, new lines fill in from the top of the screen
        class EmbConsole_EXPORT ScrollDown final
            : public PrintCommand{
        public:
            ScrollDown(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ScrollDown>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Text Modification
        //////////////////////////////////////////////////

        /// Insert <n> spaces at the current cursor position, shifting all existing text to the right. Text exiting the screen to the right is removed.
        class EmbConsole_EXPORT InsertCharacter final
            : public PrintCommand{
        public:
            InsertCharacter(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<InsertCharacter>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Delete <n> characters at the current cursor position, shifting in space characters from the right edge of the screen.
        class EmbConsole_EXPORT DeleteCharacter final
            : public PrintCommand{
        public:
            DeleteCharacter(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<DeleteCharacter>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Erase <n> characters from the current cursor position by overwriting them with a space character.
        class EmbConsole_EXPORT EraseCharacter final
            : public PrintCommand{
        public:
            EraseCharacter(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<EraseCharacter>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Inserts <n> lines into the buffer at the cursor position. The line the cursor is on, and lines below it, will be shifted downwards.
        class EmbConsole_EXPORT InsertLine final
            : public PrintCommand{
        public:
            InsertLine(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<InsertLine>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Deletes <n> lines from the buffer, starting with the row the cursor is on.
        class EmbConsole_EXPORT DeleteLine final
            : public PrintCommand{
        public:
            DeleteLine(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<DeleteLine>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Replace all text in the current viewport/screen specified by <type> with space characters.
        class EmbConsole_EXPORT ClearDisplay final
            : public PrintCommand{
        public:
            enum class Type {
                FromCursorToEnd,        // Cursor inclusive
                FromBeginningToCursor,  // Cursor inclusive
                All
            };
        public:
            ClearDisplay(Type const a_eType = Type::All) : m_eType{ a_eType } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ClearDisplay>(m_eType); }
            void process(Terminal&) const noexcept override;
        private:
            Type const m_eType{};
        };
        /// Replace all text on the line with the cursor specified by <type> with space characters.
        class EmbConsole_EXPORT ClearLine final
            : public PrintCommand{
        public:
            enum class Type {
                FromCursorToEnd,        // Cursor inclusive
                FromBeginningToCursor,  // Cursor inclusive
                All
            };
        public:
            ClearLine(Type const a_eType = Type::All) : m_eType{ a_eType } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ClearLine>(m_eType); }
            void process(Terminal&) const noexcept override;
        private:
            Type const m_eType{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Text Formatting
        //////////////////////////////////////////////////

        /// Returns all attributes to the default state prior to modification.
        class EmbConsole_EXPORT ResetTextFormat final
            : public PrintCommand{
        public:
            ResetTextFormat() {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ResetTextFormat>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Applies color to text foregroug and background.
        class EmbConsole_EXPORT SetColor final
            : public PrintCommand{
        public:
            enum class Color {
                Black,
                Red,
                Green,
                Yellow,
                Blue,
                Magenta,
                Cyan,
                White,
                BrightBlack,
                BrightRed,
                BrightGreen,
                BrightYellow,
                BrightBlue,
                BrightMagenta,
                BrightCyan,
                BrightWhite,
                Default
            };
        public:
            SetColor(Color const a_eFgColor, Color const a_eBgColor = Color::Default) : m_eFgColor{ a_eFgColor }, m_eBgColor{ a_eBgColor } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetColor>(m_eFgColor, m_eBgColor); }
            void process(Terminal&) const noexcept override;
        private:
            Color const m_eFgColor{};
            Color const m_eBgColor{};
        };
        /// Enable/Disable the negative foregroud/background colors.
        class EmbConsole_EXPORT SetNegativeColors final
            : public PrintCommand{
        public:
            SetNegativeColors(bool const a_bEnabled) : m_bEnabled{ a_bEnabled } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetNegativeColors>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };
        /// Enable/Disable the bold.
        class EmbConsole_EXPORT SetBold final
            : public PrintCommand{
        public:
            SetBold(bool const a_bEnabled) : m_bEnabled{ a_bEnabled } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetBold>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };
        /// Enable/Disable the italic.
        class EmbConsole_EXPORT SetItalic final
            : public PrintCommand{
        public:
            SetItalic(bool const a_bEnabled) : m_bEnabled{ a_bEnabled } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetItalic>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };
        /// Enable/Disable the underline.
        class EmbConsole_EXPORT SetUnderline final
            : public PrintCommand{
        public:
            SetUnderline(bool const a_bEnabled) : m_bEnabled{ a_bEnabled } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetUnderline>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Tabs
        //////////////////////////////////////////////////

        /// Sets a tab stop in the current column the cursor is in.
        class EmbConsole_EXPORT SetHorizontalTab final
            : public PrintCommand{
        public:
            SetHorizontalTab() {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetHorizontalTab>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Advance the cursor to the next column (in the same row) with a tab stop.
        /// If there are no more tab stops, move to the last column in the row.
        /// If the cursor is in the last column, move to the first column of the next row.
        class EmbConsole_EXPORT HorizontalTabForward final
            : public PrintCommand{
        public:
            HorizontalTabForward(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<HorizontalTabForward>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Move the cursor to the previous column (in the same row) with a tab stop.
        /// If there are no more tab stops, moves the cursor to the first column.
        /// If the cursor is in the first column, doesn't move the cursor.
        class EmbConsole_EXPORT HorizontalTabBackward final
            : public PrintCommand{
        public:
            HorizontalTabBackward(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<HorizontalTabBackward>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Clears the tab stop in the current column, if there is one. Otherwise does nothing.
        /// Or Clears all currently set tab stops.
        class EmbConsole_EXPORT ClearHorizontalTab final
            : public PrintCommand{
        public:
            enum class Type {
                CurrentColumn,
                AllColumns,
            };
        public:
            ClearHorizontalTab(Type const a_eType = Type::AllColumns) : m_eType{ a_eType } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<ClearHorizontalTab>(m_eType); }
            void process(Terminal&) const noexcept override;
        private:
            Type const m_eType{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Designate Character Set
        //////////////////////////////////////////////////

        /// Enables DEC Line Drawing Mode.
        class EmbConsole_EXPORT SetDecCharacterSet final
            : public PrintCommand{
        public:
            SetDecCharacterSet(bool const a_bEnabled) : m_bEnabled{ a_bEnabled } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetDecCharacterSet>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };

        /// Prints symbol into the console.
        class EmbConsole_EXPORT PrintSymbol final
            : public PrintCommand{
        public:
            enum class Symbol {
                Space,
                BottomRight,
                TopRight,
                TopLeft,
                BottomLeft,
                Cross,
                HorizontalBar,
                LeftCross,
                RightCross,
                BottomCross,
                TopCross,
                VerticalBar,
            };
        public:
            PrintSymbol(Symbol const a_eSymbol, unsigned int const a_uiN = 1) : m_eSymbol{ a_eSymbol }, m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<PrintSymbol>(m_eSymbol, m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            Symbol const m_eSymbol{};
            unsigned int const m_uiN{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Scrolling Margins
        //////////////////////////////////////////////////

        /// Set the VT scrolling margins of the viewport.
        class EmbConsole_EXPORT SetScrollingRegion final
            : public PrintCommand{
        public:
            SetScrollingRegion(unsigned int const a_uiT, unsigned int const a_uiB) : m_uiT{ a_uiT }, m_uiB{ a_uiB } { assert(m_uiT > 0 && m_uiB > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetScrollingRegion>(m_uiT, m_uiB); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiT{};
            unsigned int const m_uiB{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Window Title
        //////////////////////////////////////////////////

        /// Sets the console window's title to <string>.
        class EmbConsole_EXPORT SetWindowTitle final
            : public PrintCommand{
        public:
            SetWindowTitle(std::string const& a_strTitle) : m_strTitle{ a_strTitle } { assert(m_strTitle.size() < 255); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<SetWindowTitle>(m_strTitle); }
            void process(Terminal&) const noexcept override;
        private:
            std::string const m_strTitle{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Alternate Screen Buffer
        //////////////////////////////////////////////////

        /// Sets the console window's title to <string>.
        class EmbConsole_EXPORT UseAlternateScreenBuffer final
            : public PrintCommand{
        public:
            UseAlternateScreenBuffer(bool const& a_bEnabled) : m_bEnabled{ a_bEnabled } { }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<UseAlternateScreenBuffer>(m_bEnabled); }
            void process(Terminal&) const noexcept override;
        private:
            bool const m_bEnabled{};
        };

        //////////////////////////////////////////////////
        ///// PrintCommands: Miscellaneous
        //////////////////////////////////////////////////

        /// Rings the console bell.
        class EmbConsole_EXPORT RingBell final
            : public PrintCommand{
        public:
            RingBell() { }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<RingBell>(); }
            void process(Terminal&) const noexcept override;
        };
        /// Prints <n> new lines.
        class EmbConsole_EXPORT PrintNewLine final
            : public PrintCommand{
        public:
            PrintNewLine(unsigned int const a_uiN = 1) : m_uiN{ a_uiN } { assert(m_uiN > 0); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<PrintNewLine>(m_uiN); }
            void process(Terminal&) const noexcept override;
        private:
            unsigned int const m_uiN{};
        };
        /// Prints text into the console.
        class EmbConsole_EXPORT PrintText final
            : public PrintCommand{
        public:
            PrintText(std::string const& a_strText, unsigned int const a_uiR, unsigned int const a_uiC) : m_strText{ a_strText }, m_uiR{ a_uiR }, m_uiC{ a_uiC } { assert(a_uiR > 0 && a_uiC > 0); }
            PrintText(std::string const& a_strText) : m_strText{ a_strText } { }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<PrintText>(m_strText); }
            void process(Terminal&) const noexcept override;
        private:
            std::string const m_strText{};
            unsigned int const m_uiR{ 0 };
            unsigned int const m_uiC{ 0 };
        };

        //////////////////////////////////////////////////
        ///// PromptCommand Base
        //////////////////////////////////////////////////

        class EmbConsole_EXPORT PromptCommand {
        public:
            using Ptr = std::shared_ptr<PromptCommand>;
            using VPtr = std::vector<Ptr>;
        public:
            PromptCommand() noexcept;
            PromptCommand(PromptCommand const&) noexcept;
            PromptCommand(PromptCommand&&) noexcept;
            virtual ~PromptCommand() noexcept;
            PromptCommand& operator= (PromptCommand const&) noexcept;
            PromptCommand& operator= (PromptCommand&&) noexcept;
            virtual Ptr copy() const noexcept = 0;
        };

        //////////////////////////////////////////////////
        ///// PromptCommands: BeginPrompt / CommitPrompt
        //////////////////////////////////////////////////

        /// Begins a session of prompt commands. Prompt commands sent before BeginPrompt are not taken into consideration
        class EmbConsole_EXPORT BeginPrompt final
            : public PromptCommand{
        public:
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<BeginPrompt>(); }
        };
        /// Ends a session of prompt commands. Commands sent between BeginPrompt and CommitPrompt are treated at the commit.
        class EmbConsole_EXPORT CommitPrompt final
            : public PromptCommand{
        public:
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<CommitPrompt>(); }
        };

        //////////////////////////////////////////////////
        ///// PromptCommands: Customizing
        //////////////////////////////////////////////////

        /// Set the question to prompt
        class EmbConsole_EXPORT Question final
            : public PromptCommand{
        public:
            Question(std::string const& a_strQuestion) : m_strQuestion{ a_strQuestion } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Question>(m_strQuestion); }
        private:
            std::string const m_strQuestion{};
            friend class Terminal;
        };
        /// Add a response choice to the question
        class EmbConsole_EXPORT Choice final
            : public PromptCommand{
        public:
            Choice(std::string const& a_strChoice, std::string const& a_strValue) : m_strChoice{ a_strChoice }, m_strValue{ a_strValue } {}
            template<typename T>
            Choice(std::string const& a_strChoice, T const& a_tValue) : m_strChoice{ a_strChoice }, m_strValue{ std::to_string(a_tValue) } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Choice>(m_strChoice, m_strValue); }
            std::string visibleString(bool const& a_bTrimmed) const noexcept;
            bool isChosen(char const&) const noexcept;
            char key() const noexcept;
        private:
            std::string const m_strChoice{};
            std::string const m_strValue{};
            friend class Terminal;
        };
        /// Set the default response to the question
        class EmbConsole_EXPORT Default final
            : public PromptCommand{
        public:
            Default(std::string const& a_strChoice) : m_strChoice{ a_strChoice } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Default>(m_strChoice); }
        private:
            std::string const m_strChoice{};
        };
        /// Set response validator to avoid bad entries to the prompt
        class Validator
            : public PromptCommand {
        public:
            Validator(std::string const& a_strRegexValidator, std::string const& a_strErrorMessage) : m_strRegexValidator{ a_strRegexValidator }, m_strErrorMessage{ a_strErrorMessage } {}
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<Validator>(m_strRegexValidator, m_strErrorMessage); }
            bool isValid(std::string const&) const noexcept;
        private:
            std::string const m_strRegexValidator{};
            std::string const m_strErrorMessage{};
            friend class Terminal;
        };
        ///// Response validator that only accept numbers
        class EmbConsole_EXPORT ValidatorNumber final
            : public Validator{
        public:
            ValidatorNumber() : Validator{ "^[0-9]+$", "The value must be a number" } {}
        };
        /// Set the callback to be called when user cancels the prompt
        class EmbConsole_EXPORT OnCancel final
            : public PromptCommand{
        public:
            OnCancel(std::function<void(void)> const& a_clbkCancel) : m_clbkCancel{ a_clbkCancel } { assert(m_clbkCancel); }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<OnCancel>(m_clbkCancel); }
            void operator()() const noexcept { if (m_clbkCancel) { m_clbkCancel(); } }
        private:
            std::function<void(void)> const m_clbkCancel{};
        };
        /// Set the callback to be called when user valids the prompt
        class EmbConsole_EXPORT OnValid
            : public PromptCommand {
        public:
            OnValid(std::function<void(std::string const&)> const& a_clbkValid)
                : m_clbkValid{ a_clbkValid } {
                assert(m_clbkValid);
            }
            Ptr copy() const noexcept override { return emb::tools::memory::make_unique<OnValid>(m_clbkValid); }
            void operator()(std::string const& a_strResult) const noexcept { if (m_clbkValid) { m_clbkValid(a_strResult); } }
        private:
            std::function<void(std::string const&)> const m_clbkValid{};
        };
        using OnValidString = OnValid;
        /// Set the callback to be called when user valids the prompt
        class EmbConsole_EXPORT OnValidLong final
            : public OnValidString{
        public:
            OnValidLong(std::function<void(long const&)> const& a_clbkValid)
                : OnValidString{ [a_clbkValid](std::string const& a_strResult) { a_clbkValid(std::stol(a_strResult)); } } {
            }
        };
        /// Set the callback to be called when user valids the prompt
        class EmbConsole_EXPORT OnValidBool final
            : public OnValidString{
        public:
            OnValidBool(std::function<void(bool const&)> const& a_clbkValid)
                : OnValidString{ [a_clbkValid](std::string const& a_strResult) { a_clbkValid("1" == a_strResult); } } {
            }
        };

        //////////////////////////////////////////////////
        ///// Table tools
        //////////////////////////////////////////////////

        namespace table {
            /**
             * @brief Represent a cell of a table
             */
            struct Cell {
                std::string strText{};
                SetColor::Color eColor{ SetColor::Color::White };
                Cell(std::string const& a_strText, SetColor::Color a_eColor = SetColor::Color::White)
                    : strText{ a_strText }, eColor{ a_eColor }
                {}
            };

            /**
             * @brief Represent a row of a table
             *
             */
            using Row = std::vector<Cell>;

            /**
             * @brief Represent a table
             */
            struct Table {
                std::string strTitle{};                         ///< Title of the table
                std::vector<std::string> vstrColumnsTitles{};   ///< List of the titles
                std::vector<Row> vvstrRows{};                   ///< List of the rows
            };

            /**
             * @brief Print a table on the given console session
             * @param a_rConsole    Console to print the table onto
             * @param a_stTable     Table to print
             */
            void print(ConsoleSession& a_rConsole, Table const& a_stTable);
        }
    } // console
} // emb