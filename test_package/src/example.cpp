#include "EmbConsole.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
//#include <sstream>
#ifdef USE_VLD
#include <vld.h>
#endif

using namespace std::chrono_literals;
namespace cs = emb::console;

int main() {
    //std::stringstream buffer;
    //std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    std::cout << "version " << cs::version() << std::endl;

    //std::string text = buffer.str(); // text will now contain "Bla\n"

    /*

app --consoles=local:client
app --consoles=local:server+telnet:1234+std+file:/home/test.txt
app --console=local --console=server --console=telnet:1234 --console=std --console=file:/home/test.txt

*/
    {
        auto console = cs::Console::create();
        if (console) {
            console->print("start");

            std::atomic<bool> stop{ false };
            std::atomic<long> i{ 0 };
            std::atomic<long> j{ 0 };
            std::condition_variable cv{};

            //console->addCommand("", []() {});
            //console->addCommand("gdf", []() {});
            //console->addCommand("/", []() {});
            //console->addCommand("/fgfd/", []() {});

            console->addCommand(cs::UserCommandInfo("/stop", "Stop the program"), [&stop, &cv]() {
                stop = true;
                cv.notify_all();
            });
            console->addCommand(cs::UserCommandInfo("/test", "Test command to show coucou1"), [](cs::UserCommandData const& a_CmdData) {
                a_CmdData.console.print("==== TESTING PROMPT ====");

                std::string strResult;
                if (a_CmdData.console.promptString("Question String?", strResult)) {
                    a_CmdData.console.print("=> VALIDATED: " + strResult);
                }
                else {
                    a_CmdData.console.print("=> CANCELED");
                }

                bool bResult = false;
                if (a_CmdData.console.promptYesNo("Question Yes/No?", bResult)) {
                    a_CmdData.console.print("=> VALIDATED: " + std::to_string(bResult));
                }
                else {
                    a_CmdData.console.print("=> CANCELED");
                }

                std::unordered_map<std::string, std::string> choicesMap{
                    { "Choice&1", "H11" },
                    { "Choice&2", "H12" },
                    { "Choice&8", "B18" },
                    { "Choice&9", "B19" },
                };
                if (a_CmdData.console.promptChoice("Question choice?", choicesMap, strResult)) {
                    a_CmdData.console.print("=> VALIDATED: " + strResult);
                }
                else {
                    a_CmdData.console.print("=> CANCELED");
                }

                a_CmdData.console.print("==== TESTING PROMPT END ====");
            });
            console->addCommand("/a/b", [](cs::UserCommandData const& a_CmdData) {
                a_CmdData.console.print("coucou2");
            });
            console->addCommand("/a/c", [](cs::UserCommandData const& a_CmdData) {
                a_CmdData.console.print("coucou3");
            });
            console->addCommand("/e/f", [](cs::UserCommandData const& a_CmdData) {
                a_CmdData.console.print("coucou4");
            });
            console->addCommand("/windows/title", [](cs::UserCommandData const& a_CmdData) {
                if (a_CmdData.args.size() > 0) {
                    a_CmdData.console
                        << cs::Begin()
                        << cs::SetWindowTitle(a_CmdData.args.at(0))
                        << cs::Commit();
                }
            });

            std::thread first([console, &stop, &i, &cv]() {
                std::mutex m{};
                while (!stop) {
                    console->print("abcdefghijklmnopqrstuvwxyz " + std::to_string(++i));

                    std::unique_lock<std::mutex> lk{ m };
                    cv.wait_for(lk, 10s);
                }
            });
            std::thread second([console, &stop, &j, &cv]() {
                std::mutex m{};
                while (!stop) {
                    console->print("ABCDEFGHI " + std::to_string(++j));

                    std::unique_lock<std::mutex> lk{ m };
                    cv.wait_for(lk, 5s);
                }
            });

            first.join();
            second.join();

            console->print("stop");
        }
    }
}

#if 0
void cmd1()
{
}

void cmd2(ConsoleCommandArgs args)
{
}

int main(int argc, char** argv)
{
    Console::start(argc, argv);

    Console::addCommand("/dsf/dsf/df", std::bind(&cmd1));
    Console::addCommand("/dsf/dsf/df", std::bind(&cmd2));

    Logger("name")::Debug <<

        Console::stop();
    return 0;
}

#include "embconsole.h"

int main(int argc, char* argv[]) {
    emb::Console::Options consoleOptions{
    };
    emb::Console::Initialize(consoleOptions, emb::Console::Options(argc, argv));

    emb::Console::AddCommand("/cmd1/cmd2/cmd", []() {
        emb::Console::Print("test");
    });
    emb::Console::DelCommand("/cmd1/cmd2/cmd");

    auto command = emb::Console::AddCommand("/cmd1/cmd2/cmd", []() {
        emb::Console::Print("test");
    });
    command.Trigger();
    emb::Console::DelCommand(command);

    emb::Console::AddCommand("/cmd1/cmd2/cmd3", [](emb::Console:CommandArgs const& aArgs) {
        emb::Console::Print("test");
        emb::Console::Question();
    }, emb::Console::CommandOptions());

    auto group = emb::Console::AddGroup("/cmd1/cmd2/cmd3");
    group.AddCommand("/cmd1/cmd2/cmd3", [](emb::Console:CommandArgs const& aArgs) {
        emb::Console::Print("test");
        emb::Console::Question();
    }, emb::Console::CommandOptions());

    emb::Console::Print("gdfgdf");

    emb::Console::Uninitialize();
}

int main(int argc, char* argv[]) {
    emb::Logger::Options opt{
        emb::Logger::Option::LogToFile();
        emb::Logger::Option::LogToConsole(emb::Console::Stream());
        emb::Logger::Option::CaptureStdOutToLevel(Debug);
    }
    emb::Logger::Initialize(opt);

    emb::Logger::Debug(emb::SourcesInfo(), "", "");

    emb::Logger::Uninitialize();

    emb::Settings(emb::Setting::TrucMuche).Set(0);
    emb::Settings(emb::Setting::TrucMuche).Get(0);
    emb::Settings(emb::Setting::TrucMuche).GetDefault(0);
    emb::Settings(emb::Setting::TrucMuche).GetOrDefault(0);
    (emb::Setting::TrucMuche)_setting.
}

#endif