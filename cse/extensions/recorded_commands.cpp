#include "recorded_commands.h"

#include <string>

#include <Windows.h>

namespace cse::extensions {

RecordedCommands::RecordedCommands()
{
  addRawCommand("vscode",     "opens VScode",     "code");
  addRawCommand("note",       "opens Notepad++",  R"(C:\Program Files (x86)\Notepad++\notepad++.exe)");
  addShellCommand("python",   "python shell",     R"(C:\Windows\System32\cmd.exe)", "/c python");
  addShellCommand("js",       "javascript shell", R"(C:\Windows\System32\cmd.exe)", "/c node");
  addWebCommand("regex",      "Regex101 substitution and matching", "https://regex101.com/");
  addWebCommand("excalidraw", "Excalidraw sketches and schema",     "https://excalidraw.com/");

  cse::commands::addCommand({
    "g",
    "google search",
    { std::make_shared<CommandTextPart>("search") },
    runLater([](const auto &args) {
      std::string url = "https://www.google.com/search?q=";
      url += args[0];
      ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    })
  });
  cse::commands::addCommand({
    "g!",
    "web search w/ immediate url",
    { std::make_shared<CommandTextPart>("url") },
    runLater([](const auto &args) {
      std::string url{ args[0] };
      if (!url.starts_with("http"))
        url = "https://" + url;
      ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    })
  });
}

void RecordedCommands::addRawCommand(const char *cseCommand, const char *tooltip, const char *systemCommand)
{
  cse::commands::addCommand({
    cseCommand,
    tooltip,
    { /* no arguments */ },
    runLater([systemCommand](const auto &args) {
      auto success = ShellExecuteA(NULL, "open", systemCommand, NULL, NULL, SW_HIDE);
      cse::log("Executing " + std::string(systemCommand) + ", got " + std::to_string((long long)success));
    })
  });
}

void RecordedCommands::addShellCommand(const char *cseCommand, const char *tooltip, const char *executablePath, const char *arguments)
{
  cse::commands::addCommand({
    cseCommand,
    tooltip,
    { /* no arguments */ },
    [executablePath, arguments](const auto &args) {
      auto success = ShellExecuteA(NULL, "open", executablePath, arguments, NULL, SW_SHOWNORMAL);
      cse::log("Executing " + std::string(executablePath) + " " + arguments + ", got " + std::to_string((long long)success));
    }
  });
}

void RecordedCommands::addWebCommand(const char *cseCommand, const char *tooltip, const char *url)
{
  cse::commands::addCommand({
    cseCommand,
    tooltip,
    { /* no args */ },
    runLater([url](const auto &args) {
      cse::extensions::openWebPage(url);
    })
  });
}

}
