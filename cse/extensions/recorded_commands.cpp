#include "recorded_commands.h"

#include <iostream>
#include <string>

#include <Windows.h>

namespace cse::extensions {

static void addRawCommand(const char *cseCommand, const char *tooltip, const char *systemCommand)
{
  cse::addCommand({
    cseCommand,
    tooltip,
    { /* no arguments */ },
    runLater([systemCommand](const auto &args) {
      auto success = ShellExecuteA(NULL, "open", systemCommand, NULL, NULL, SW_HIDE);
      cse::log("Executing " + std::string(systemCommand) + ", got " + std::to_string((long long)success));
    })
  });
}

static void addShellCommand(const char *cseCommand, const char *tooltip, const char *executablePath, const char *arguments)
{
  cse::addCommand({
    cseCommand,
    tooltip,
    { /* no arguments */ },
    [executablePath, arguments](const auto &args) {
      auto success = ShellExecuteA(NULL, "open", executablePath, arguments, NULL, SW_SHOWNORMAL);
      cse::log("Executing " + std::string(executablePath) + " " + arguments + ", got " + std::to_string((long long)success));
    }
  });
}

static void addGoogleSearchCommand()
{
  cse::addCommand({
    "g",
    "google search",
    { new CommandTextPart("url") },
    runLater([](const auto &args) {
      std::string url = "https://www.google.com/search?q=";
      url += args[0];
      ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    })
  });
}

RecordedCommands::RecordedCommands()
{
  addRawCommand("vscode", "opens VScode",    "code");
  addRawCommand("note",   "opens Notepad++", "C:\\Program Files (x86)\\Notepad++\\notepad++.exe");
  addShellCommand("python", "python shell",     "C:\\Windows\\System32\\cmd.exe", "/c python");
  addShellCommand("js",     "javascript shell", "C:\\Windows\\System32\\cmd.exe", "/c node");
  addGoogleSearchCommand();
}

RecordedCommands::~RecordedCommands()
{
}

}